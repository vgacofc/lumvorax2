#!/usr/bin/env python3
"""
post_run_autocorr.py — C33-P3 — Estimateur τ_int de Madras-Sokal
=================================================================
Calcule le temps d'autocorrélation intégré τ_int pour chaque observable
de chaque module en utilisant l'algorithme de la fenêtre de Sokal avec
condition de coupure automatique (Madras & Sokal, J. Stat. Phys. 50, 1988).

INSTRUCTION PERMANENTE 2 : NE JAMAIS RÉDUIRE LES LOGS NI LA FRÉQUENCE.
Aucun filtre modulo. Aucune désactivation. Aucune réduction.

Sortie : tests/autocorr_tau_int_sokal.csv
Colonnes : module,observable,tau_int,tau_int_err,window_M,N_effective,err_correction_factor
"""

import os
import sys
import csv
import math
import glob
import time
import datetime
import argparse

# ── Constantes ────────────────────────────────────────────────────
SOKAL_C_FACTOR = 6.0   # Coupure automatique : M_opt = C * tau_int
MAX_WINDOW     = 2000  # Fenêtre maximale de recherche
MIN_SAMPLES    = 50    # Minimum d'échantillons pour estimer τ_int

# ── Algorithme de Sokal ───────────────────────────────────────────
def sokal_tau_int(x, c=SOKAL_C_FACTOR, max_window=MAX_WINDOW):
    """
    Estimateur de Madras-Sokal pour le temps d'autocorrélation intégré.
    Retourne (tau_int, tau_int_err, window_M, N_effective, err_correction_factor).
    """
    n = len(x)
    if n < MIN_SAMPLES:
        return None, None, 0, 0, 1.0

    # Moyenne et variance
    mean_x = sum(x) / n
    xc = [xi - mean_x for xi in x]

    # Fonction d'autocorrélation normalisée C(t)
    c0 = sum(xi * xi for xi in xc) / n
    if c0 == 0.0:
        return 0.0, 0.0, 0, n, 1.0

    def rho(t):
        if t >= n:
            return 0.0
        return sum(xc[i] * xc[i + t] for i in range(n - t)) / ((n - t) * c0)

    # Fenêtre de Sokal : chercher M tel que M >= c * Gamma(M)
    gamma = 1.0  # Gamma(0) = 0.5 * [1 + rho(0)] = 1.0
    M = 0
    tau_running = 0.5
    for t in range(1, min(max_window, n // 4)):
        r = rho(t)
        tau_running += r
        if M < int(c * tau_running):
            M = t
        if t >= int(c * tau_running):
            M = t
            break

    tau_int = 0.5 + sum(rho(t) for t in range(1, M + 1))
    if tau_int < 0.5:
        tau_int = 0.5

    # Incertitude sur tau_int (formule standard)
    tau_int_err = tau_int * math.sqrt(2.0 * (2 * M + 1) / n)

    # N_effective et facteur de correction
    N_eff = n / (2.0 * tau_int) if tau_int > 0 else n
    err_correction = math.sqrt(2.0 * tau_int) if tau_int > 0 else 1.0

    return tau_int, tau_int_err, M, int(N_eff), err_correction


# ── Lecture des données CSV Lumvorax ──────────────────────────────
def load_lumvorax_csv(csv_path):
    """
    Charge un CSV Lumvorax et extrait les séries temporelles par (module, observable).
    Format CSV : event_type, timestamp_utc, timestamp_ns, pid, module, metric, value, ...
    """
    series = {}  # {(module, observable): [valeurs]}
    try:
        with open(csv_path, 'r', encoding='utf-8', errors='replace') as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) < 7:
                    continue
                event_type = row[0].strip()
                if event_type not in ('METRIC', 'metric'):
                    continue
                module  = row[4].strip() if len(row) > 4 else ''
                metric  = row[5].strip() if len(row) > 5 else ''
                try:
                    value = float(row[6])
                except (ValueError, IndexError):
                    continue
                key = (module, metric)
                if key not in series:
                    series[key] = []
                series[key].append(value)
    except FileNotFoundError:
        pass
    return series


# ── CORRECTION ANOM-C54-02 : Lecture des fichiers step-by-step ────
def load_stepwise_csv(csv_path):
    """
    Charge un CSV step-by-step du runner advanced_parallel et extrait les séries
    temporelles par (module, observable).
    Format : problem,step,energy_norm,pairing_norm,sign_ratio,cpu_percent,mem_percent,elapsed_ns
    Correction C55 : adapte post_run_autocorr au format réel du runner (0 séries METRIC en C54).
    """
    series = {}  # {(module, observable): [valeurs]}
    # Observables numériques à extraire par colonne (index, nom)
    OBSERVABLES = [
        (2, 'energy_norm'),
        (3, 'pairing_norm'),
        (4, 'sign_ratio'),
        (5, 'cpu_percent'),
        (6, 'mem_percent'),
    ]
    try:
        with open(csv_path, 'r', encoding='utf-8', errors='replace') as f:
            reader = csv.reader(f)
            header = None
            for row in reader:
                if not row:
                    continue
                # Sauter les lignes de commentaire ou métadonnées
                if row[0].strip().startswith('#'):
                    continue
                # Première ligne valide = header si contient 'problem' ou 'module'
                if header is None:
                    if row[0].strip().lower() in ('problem', 'module', 'name'):
                        header = [c.strip() for c in row]
                        continue
                    else:
                        # Pas de header — format implicite standard
                        header = ['problem', 'step', 'energy_norm', 'pairing_norm',
                                  'sign_ratio', 'cpu_percent', 'mem_percent', 'elapsed_ns']
                if len(row) < 5:
                    continue
                module = row[0].strip()
                if not module:
                    continue
                for col_idx, obs_name in OBSERVABLES:
                    if col_idx >= len(row):
                        continue
                    try:
                        value = float(row[col_idx])
                    except (ValueError, TypeError):
                        continue
                    key = (module, obs_name)
                    if key not in series:
                        series[key] = []
                    series[key].append(value)
    except FileNotFoundError:
        pass
    return series


# ── Main ──────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description='Estimateur τ_int de Sokal — C33-P3 — Hubbard HTS')
    parser.add_argument('--run-dir', default='.',
                        help='Répertoire du run (contient logs/)')
    parser.add_argument('--output', default='tests/autocorr_tau_int_sokal.csv',
                        help='Fichier de sortie CSV')
    parser.add_argument('--c-factor', type=float, default=SOKAL_C_FACTOR,
                        help='Facteur C de Sokal (défaut 6.0)')
    parser.add_argument('--max-window', type=int, default=MAX_WINDOW,
                        help='Fenêtre maximale')
    args = parser.parse_args()

    # Chercher les fichiers CSV Lumvorax dans run_dir/logs/
    log_dir   = os.path.join(args.run_dir, 'logs')
    csv_files = sorted(glob.glob(os.path.join(log_dir, 'lumvorax_hubbard_hts_*.csv')))

    # Fallback : chercher dans logs/ et répertoire courant
    if not csv_files:
        csv_files = sorted(glob.glob('logs/lumvorax_hubbard_hts_*.csv'))
    if not csv_files:
        csv_files = sorted(glob.glob('lumvorax_hubbard_hts_*.csv'))

    print(f'[post_run_autocorr] Fichiers Lumvorax trouvés : {len(csv_files)}')
    if csv_files:
        for f in csv_files[:5]:
            print(f'  → {f}')

    # Agréger toutes les séries de tous les fichiers Lumvorax
    all_series = {}
    for csv_path in csv_files:
        s = load_lumvorax_csv(csv_path)
        for key, vals in s.items():
            if key not in all_series:
                all_series[key] = []
            all_series[key].extend(vals)
        print(f'[post_run_autocorr] Chargé {csv_path} → {len(s)} séries, '
              f'{sum(len(v) for v in s.values())} points')

    # CORRECTION ANOM-C54-02 : Fallback step-by-step si aucune série METRIC trouvée
    # Le runner advanced_parallel écrit des CSV step-by-step sans colonnes METRIC|
    # Format : problem,step,energy_norm,pairing_norm,sign_ratio,cpu_percent,mem_percent,elapsed_ns
    stepwise_candidates = [
        os.path.join(args.run_dir, 'logs', 'normalized_observables_trace.csv'),
        os.path.join(args.run_dir, 'normalized_observables_trace.csv'),
        os.path.join(args.run_dir, 'logs', 'baseline_reanalysis_metrics.csv'),
        os.path.join(args.run_dir, 'baseline_reanalysis_metrics.csv'),
        'logs/normalized_observables_trace.csv',
        'normalized_observables_trace.csv',
    ]
    n_stepwise_loaded = 0
    for sw_path in stepwise_candidates:
        if os.path.exists(sw_path):
            s = load_stepwise_csv(sw_path)
            if s:
                for key, vals in s.items():
                    if key not in all_series:
                        all_series[key] = []
                    all_series[key].extend(vals)
                n_stepwise_loaded += 1
                print(f'[post_run_autocorr] [C55-ANOM-C54-02] Chargé step-by-step {sw_path} '
                      f'→ {len(s)} séries, {sum(len(v) for v in s.values())} points')
    if n_stepwise_loaded == 0:
        print('[post_run_autocorr] [C55-ANOM-C54-02] Aucun fichier step-by-step trouvé.')

    if not all_series:
        print('[post_run_autocorr] AVERTISSEMENT : Aucune série METRIC ou step-by-step trouvée. '
              'Génération CSV vide avec header.')

    # Créer le répertoire de sortie si nécessaire
    out_dir = os.path.dirname(args.output)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    # Écrire le CSV de résultats
    fieldnames = [
        'module', 'observable', 'n_samples',
        'tau_int', 'tau_int_err', 'window_M',
        'N_effective', 'err_correction_factor',
        'mean', 'std', 'status'
    ]

    timestamp = datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
    n_computed = 0
    n_skipped  = 0

    with open(args.output, 'w', newline='', encoding='utf-8') as outf:
        writer = csv.DictWriter(outf, fieldnames=fieldnames)
        writer.writeheader()

        for (module, observable), vals in sorted(all_series.items()):
            n = len(vals)
            mean_v = sum(vals) / n if n > 0 else 0.0
            variance = sum((v - mean_v) ** 2 for v in vals) / n if n > 1 else 0.0
            std_v = math.sqrt(variance) if variance > 0 else 0.0

            if n < MIN_SAMPLES:
                writer.writerow({
                    'module': module,
                    'observable': observable,
                    'n_samples': n,
                    'tau_int': 'NA',
                    'tau_int_err': 'NA',
                    'window_M': 0,
                    'N_effective': n,
                    'err_correction_factor': 1.0,
                    'mean': f'{mean_v:.10f}',
                    'std': f'{std_v:.10f}',
                    'status': f'SKIP_INSUFFICIENT_N_{n}'
                })
                n_skipped += 1
                continue

            tau_int, tau_int_err, M, N_eff, corr_factor = sokal_tau_int(
                vals, c=args.c_factor, max_window=args.max_window)

            if tau_int is None:
                status = 'FAIL_COMPUTATION'
                tau_int, tau_int_err, M, N_eff, corr_factor = 'NA', 'NA', 0, n, 1.0
            elif tau_int > 500:
                # C57-05 : Distinguer ralentissement critique physique (attendu) vs bug de convergence
                # pairing_norm près de Tc → ralentissement critique (critical slowing down) : WARN_CRITICAL_SLOWING
                # sign_ratio τ élevé loin de Tc → problème de signe fermionique pathologique : WARN_SIGN_PROBLEM
                # Autres observables τ élevé → convergence insuffisante : WARN_TAU_INT_HIGH
                if observable in ('pairing_norm', 'pairing', 'pairing_cold'):
                    status = f'WARN_CRITICAL_SLOWING_{tau_int:.0f}'
                elif observable in ('sign_ratio', 'sign', 'avg_sign'):
                    status = f'WARN_SIGN_PROBLEM_{tau_int:.0f}'
                else:
                    status = f'WARN_TAU_INT_HIGH_{tau_int:.0f}'
            else:
                status = 'OK'

            writer.writerow({
                'module': module,
                'observable': observable,
                'n_samples': n,
                'tau_int': f'{tau_int:.4f}' if isinstance(tau_int, float) else tau_int,
                'tau_int_err': f'{tau_int_err:.4f}' if isinstance(tau_int_err, float) else tau_int_err,
                'window_M': M,
                'N_effective': N_eff,
                'err_correction_factor': f'{corr_factor:.6f}' if isinstance(corr_factor, float) else corr_factor,
                'mean': f'{mean_v:.10f}',
                'std': f'{std_v:.10f}',
                'status': status
            })
            n_computed += 1

            # Log détaillé sur stdout (INSTRUCTION PERMANENTE 2 : aucun filtre)
            print(f'[autocorr] {module} | {observable} | '
                  f'n={n} | tau_int={tau_int} | M={M} | N_eff={N_eff} | '
                  f'corr={corr_factor} | status={status}')

    print(f'\n[post_run_autocorr] TERMINÉ — {n_computed} séries calculées, '
          f'{n_skipped} sautées — sortie : {args.output}')
    print(f'[post_run_autocorr] Timestamp : {timestamp}')
    sys.exit(0)


if __name__ == '__main__':
    main()
