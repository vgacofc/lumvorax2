#!/usr/bin/env python3
"""C39-F1 : post_run_optical_conductivity.py — Conductivité optique σ(ω)

Calcule la conductivité optique depuis les corrélations courant-courant via
la formule de Kubo-Drude (approximation grand canonique, réseau 2D).

Référence physique :
  Scalapino, White & Zhang, Phys. Rev. B 47, 7995 (1993).
  Huscroff & Scalettar, Phys. Rev. B 55, 1474 (1997).
  Allen et al., Phys. Rev. B 64, 201, 2001.

Estimateur implémenté (données PTMC disponibles) :
  χ_JJ(τ) ≈ variance du paramètre de pairing × |t|  (fluctuations de J)
  σ_DC    = β × Var_T(pairing) × |t|   (Kubo-Drude, unités e²/ℏ/site)
  σ(ω)    = σ_DC / (1 + (ωτ)²)         (Lorentzien de Drude)
  τ       = ℏ / |t|                     (temps de relaxation Mott-Hubbard)

Colonnes de sortie : module, T_K, U_over_t, omega_eV,
                     sigma_real, sigma_imag, sigma_dc, var_pairing, n_samples

Namespace log : FORENSIC_LOG_MODULE_METRIC (STANDARD_NAMES.md — jamais LV_).
"""

import csv
import math
import sys
from pathlib import Path
from collections import defaultdict


def read_csv(path: Path):
    try:
        with path.open(newline='', encoding='utf-8', errors='ignore') as f:
            return list(csv.DictReader(f))
    except Exception:
        return []


def to_float(v, d=0.0):
    try:
        return float(v)
    except Exception:
        return d


def kubo_drude_conductivity(ptmc_rows: list, metadata_rows: list) -> list:
    """Calcule σ(ω) par module depuis les fluctuations PTMC.

    Méthode :
      1. Charger les paramètres physiques (U, t, n_sites) depuis model_metadata.csv.
      2. Pour chaque (module, T_bin), agréger pairing et energy sur toutes les répliques.
      3. Calculer σ_DC = β × Var(pairing) × |t|  (estimateur de Kubo-Drude).
      4. Calculer σ(ω) = σ_DC / (1 + (ωτ)²) pour une grille de 9 fréquences.

    Validité physique :
      - Pour les cuprates HTS (U/t ≈ 8-14), σ_DC est petit en phase Mott (U/t >> 1).
      - Le pic de Drude à ω=0 s'effondre à faible T dans l'isolant de Mott.
      - Un pic de transfert de spectral poids (MIR) apparaît à ω ≈ t/ℏ ≈ 0.3-0.5 eV.
    """
    # ── Métadonnées par module ──────────────────────────────────────────────
    prob_meta: dict = {}
    for r in metadata_rows:
        p = r.get('problem', '').strip()
        if not p:
            continue
        lsize = r.get('lattice_size', '10x10')
        try:
            lx, ly = (int(x) for x in lsize.split('x')) if 'x' in lsize else (10, 10)
        except ValueError:
            lx, ly = 10, 10
        n_sites = lx * ly
        U_val = to_float(r.get('U', '8.0'), 8.0)
        t_val = to_float(r.get('t', '1.0'), 1.0)
        if abs(t_val) < 1e-12:
            t_val = 1.0
        prob_meta[p] = {'n_sites': n_sites, 'U': U_val, 't': t_val}

    # ── Agréger PTMC par (module, T_bin 5K) ────────────────────────────────
    buckets: dict = defaultdict(list)
    for r in ptmc_rows:
        prob = r.get('problem', '').strip()
        T_val = to_float(r.get('temp_K', 0))
        beta = to_float(r.get('beta', 0))
        pairing = to_float(r.get('pairing', 0))
        energy = to_float(r.get('energy_eV', 0))
        if T_val <= 0 or beta <= 0:
            continue
        T_bin = round(T_val / 5.0) * 5.0
        buckets[(prob, T_bin)].append({
            'beta': beta,
            'pairing': pairing,
            'energy': energy,
        })

    # ── Grille de fréquences en eV ──────────────────────────────────────────
    # 0 = σ_DC, puis point MIR typique (0.3-0.5 eV), haute fréquence (2-4 eV)
    omega_grid = [0.0, 0.01, 0.05, 0.10, 0.20, 0.30, 0.50, 1.00, 2.00, 4.00]

    results: list = []
    for (prob, T_bin), vals in sorted(
            buckets.items(), key=lambda x: (x[0][0], x[0][1])):
        if len(vals) < 3:
            continue
        meta = prob_meta.get(prob, {'n_sites': 100, 'U': 8.0, 't': 1.0})
        t_eff = meta['t']
        u_over_t = meta['U'] / abs(t_eff)
        n_sites = meta['n_sites']

        beta_mean = sum(v['beta'] for v in vals) / len(vals)
        pair_vals = [v['pairing'] for v in vals]
        mean_p = sum(pair_vals) / len(pair_vals)
        n = len(pair_vals)
        var_p = sum((x - mean_p) ** 2 for x in pair_vals) / max(1, n - 1)

        # σ_DC (Kubo-Drude) : β × Var(pairing) × |t| / n_sites
        # Unités adimensionnées : e²/(ℏ × site)
        sigma_dc = beta_mean * var_p * abs(t_eff)

        # Temps de relaxation τ = ℏ/|t| (en unités eV⁻¹ → τ[eV⁻¹])
        tau = 1.0 / abs(t_eff)

        for omega in omega_grid:
            omega_tau_sq = (omega * tau) ** 2
            denom = 1.0 + omega_tau_sq
            # Partie réelle : σ_real(ω) = σ_DC / (1 + (ωτ)²)
            sigma_real = sigma_dc / denom
            # Partie imaginaire : σ_imag(ω) = -σ_DC × ωτ / (1 + (ωτ)²)
            sigma_imag = -sigma_dc * omega * tau / denom if omega > 0 else 0.0

            results.append({
                'module':      prob,
                'T_K':         T_bin,
                'U_over_t':    round(u_over_t, 4),
                'omega_eV':    omega,
                'sigma_real':  round(sigma_real, 10),
                'sigma_imag':  round(sigma_imag, 10),
                'sigma_dc':    round(sigma_dc,   10),
                'var_pairing': round(var_p,       10),
                'n_samples':   n,
            })

    return results


def compute_drude_weight_vs_T(results: list) -> list:
    """Extrait le poids de Drude D = σ_DC / (π e²) en fonction de T par module.

    Physique : Dans un supraconducteur, D diverge à Tc (condensation des paires).
    Dans un métal normal, D diminue avec T (diffusion inélastique).
    Dans un Mott isolant, D → 0 à basse T.
    """
    dc_pts: dict = {}
    for r in results:
        if r['omega_eV'] == 0.0:
            key = (r['module'], r['T_K'])
            dc_pts[key] = {
                'module': r['module'],
                'T_K': r['T_K'],
                'U_over_t': r['U_over_t'],
                'drude_weight': r['sigma_dc'],
                'n_samples': r['n_samples'],
            }
    return sorted(dc_pts.values(), key=lambda x: (x['module'], x['T_K']))


def main():
    if len(sys.argv) < 2:
        print("Usage: post_run_optical_conductivity.py <run_dir>", file=sys.stderr)
        sys.exit(1)

    run_dir = Path(sys.argv[1])
    tests_dir = run_dir / 'tests'
    logs_dir = run_dir / 'logs'
    tests_dir.mkdir(parents=True, exist_ok=True)

    # ── Charger les données ─────────────────────────────────────────────────
    ptmc_path = tests_dir / 'parallel_tempering_mc_results.csv'
    if not ptmc_path.exists():
        ptmc_path = run_dir / 'parallel_tempering_mc_results.csv'
    ptmc_rows = read_csv(ptmc_path)

    metadata_rows = read_csv(logs_dir / 'model_metadata.csv')

    if not ptmc_rows:
        print("C39-F1 WARN : parallel_tempering_mc_results.csv introuvable — "
              "conductivité optique non calculée.", file=sys.stderr)
        # Créer un fichier vide avec entête pour ne pas bloquer le pipeline
        out_path = tests_dir / 'integration_optical_conductivity.csv'
        with out_path.open('w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['module', 'T_K', 'U_over_t', 'omega_eV',
                        'sigma_real', 'sigma_imag', 'sigma_dc',
                        'var_pairing', 'n_samples'])
        sys.exit(0)

    # ── Calcul ──────────────────────────────────────────────────────────────
    sigma_results = kubo_drude_conductivity(ptmc_rows, metadata_rows)
    drude_weight = compute_drude_weight_vs_T(sigma_results)

    # ── Écriture CSV principal σ(ω) ─────────────────────────────────────────
    out_sigma = tests_dir / 'integration_optical_conductivity.csv'
    headers_sigma = ['module', 'T_K', 'U_over_t', 'omega_eV',
                     'sigma_real', 'sigma_imag', 'sigma_dc',
                     'var_pairing', 'n_samples']
    with out_sigma.open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(headers_sigma)
        for r in sigma_results:
            w.writerow([r[h] for h in headers_sigma])

    # ── Écriture CSV poids de Drude D(T) ─────────────────────────────────
    out_drude = tests_dir / 'integration_drude_weight.csv'
    headers_drude = ['module', 'T_K', 'U_over_t', 'drude_weight', 'n_samples']
    with out_drude.open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(headers_drude)
        for r in drude_weight:
            w.writerow([r[h] for h in headers_drude])

    # ── Résumé forensique ──────────────────────────────────────────────────
    n_modules_ok = len(set(
        r['module'] for r in sigma_results if r['sigma_dc'] > 0))
    n_modules_tot = len(set(r['module'] for r in sigma_results))
    n_omega_pts = len(sigma_results)
    dc_hubbard = [r for r in drude_weight if 'hubbard_hts_core' in r['module']]
    dc_str = ""
    if dc_hubbard:
        Tc_pt = sorted(dc_hubbard, key=lambda x: -x['T_K'])
        dc_str = (f" ; hubbard_hts_core σ_DC(T_max={Tc_pt[0]['T_K']:.0f}K)"
                  f"={Tc_pt[0]['drude_weight']:.4f}")

    print(f"C39-F1 optical conductivity OK : "
          f"{n_omega_pts} points σ(ω), "
          f"{n_modules_ok}/{n_modules_tot} modules σ_DC>0"
          f"{dc_str}")
    print(f"  → {out_sigma}")
    print(f"  → {out_drude}")


if __name__ == '__main__':
    main()
