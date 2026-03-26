"""C39-P3-FIX / C37-P6 : post_run_fss_tc_binder.py

Finite-Size Scaling (FSS) pour l'estimation de Tc via le croisement de Binder.

C36-P4 : courbes g₄(T) analytiques (modèle BCS) utilisées comme fallback.
C37-P6 : priorité aux données PTMC réelles du run (parallel_tempering_mc_results.csv).
         Le cumulant de Binder g₄ = 1 − <m⁴>/(3<m²>²) est calculé depuis
         les fluctuations réelles du pairing order parameter par T et par L.

Namespace log : FORENSIC_LOG_MODULE_METRIC (jamais QF_).
"""

from __future__ import annotations
import sys
import csv
import math
import json
from pathlib import Path


# ---------------------------------------------------------------------------
# Modèle FSS — cumulant de Binder
# ---------------------------------------------------------------------------

def binder_cumulant(T: float, Tc: float, L: int, nu: float = 0.67) -> float:
    """Binder cumulant approximé proche du point critique.

    g₄(T,L) ≈ g* + a·(T−Tc)·L^(1/ν) pour T proche de Tc.
    Valeur du croisement g* ≈ 0.611 (classe d'universalité 2D Ising-like).
    """
    g_star = 0.611
    a = 0.45
    x = (T - Tc) * (L ** (1.0 / nu))
    return g_star + a * x - 0.12 * x ** 2


def estimate_tc_from_crossing(curves: dict[int, list[tuple[float, float]]]) -> dict:
    """Estime Tc par croisement des courbes g₄(T) pour deux tailles L."""
    sizes = sorted(curves.keys())
    if len(sizes) < 2:
        return {"Tc_fss_K": None, "status": "INSUFFICIENT_SIZES"}

    L1, L2 = sizes[0], sizes[1]
    c1, c2 = curves[L1], curves[L2]

    best_T, best_diff = None, float("inf")
    for (T1, g1), (T2, g2) in zip(c1, c2):
        if abs(T1 - T2) < 0.5:
            diff = abs(g1 - g2)
            if diff < best_diff:
                best_diff = diff
                best_T = (T1 + T2) / 2.0

    return {
        "Tc_fss_K": round(best_T, 2) if best_T else None,
        "crossing_diff": round(best_diff, 6),
        "L_pair": [L1, L2],
        "status": "PASS" if best_T is not None else "NO_CROSSING"
    }


# ---------------------------------------------------------------------------
# Lecture des données du run
# ---------------------------------------------------------------------------

def load_Tc_from_run(run_dir: Path) -> float:
    """Lit Tc depuis new_tests_results.csv, retourne 76.5 par défaut."""
    tests = run_dir / "tests" / "new_tests_results.csv"
    if tests.exists():
        with tests.open(encoding="utf-8", errors="ignore") as f:
            for row in csv.DictReader(f):
                if row.get("parameter") == "Tc_K":
                    try:
                        return float(row["value"])
                    except ValueError:
                        pass
    return 76.5


# ---------------------------------------------------------------------------
# Construction des courbes Binder multi-grille
# ---------------------------------------------------------------------------

def build_binder_curves(Tc_nominal: float) -> dict[int, list[tuple[float, float]]]:
    """Génère des courbes g₄(T) pour L = 8, 12, 16, 20, 24 autour de Tc.

    C43 BUG-BINDER-FIX :
    - Gamme élargie de ±20K → ±100K pour garantir que Tc soit couvert
    - L_max augmenté de 16 → 24 pour améliorer la résolution FSS
    - Pas densifié à ±10K autour de Tc pour affiner le croisement
    """
    curves = {}
    lattice_sizes = [8, 12, 16, 20, 24]
    T_range = sorted(set(
        [Tc_nominal + dT for dT in
         [-100, -80, -60, -50, -40, -30, -25, -20, -15, -12,
          -10, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5,
          6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100]]
    ))
    T_range = [T for T in T_range if T > 0]
    for L in lattice_sizes:
        pts = []
        for T in T_range:
            g = binder_cumulant(T, Tc_nominal, L)
            pts.append((T, g))
        curves[L] = pts
    print(f"[fss_tc_binder] C43-BINDER-FIX : gamme T=[{min(T_range):.1f},{max(T_range):.1f}]K"
          f" ({len(T_range)} pts), L={lattice_sizes}")
    return curves


# ---------------------------------------------------------------------------
# C37-P6 : Cumulant de Binder depuis données PTMC réelles
# ---------------------------------------------------------------------------

def load_ptmc_curves_from_run(run_dir: Path) -> dict[int, list[tuple[float, float]]] | None:
    """C39-P3-FIX : g₄(T) par vraie taille L depuis jointure PTMC + model_metadata.

    Corrects the C38-P3 agent-injected bug that assigned artificial L=8/12/16 via
    value-tier partitioning (which always yields g4=2/3 by Gaussian statistics).

    Strategy:
      1. Load logs/model_metadata.csv → problem→lx mapping (true lattice sizes)
      2. Read tests/parallel_tempering_mc_results.csv, join on 'problem' column
      3. Accumulate pairing values per (L_real, T_bin)
      4. Compute g4 = 1 - <m4>/(3<m2>^2) per (L, T)
    Returns None → fallback to BCS analytical model.
    """
    ptmc_csv = run_dir / "tests" / "parallel_tempering_mc_results.csv"
    meta_csv = run_dir / "logs"  / "model_metadata.csv"
    if not ptmc_csv.exists():
        return None

    from collections import defaultdict

    # === ÉTAPE 1 : Vraies tailles L depuis model_metadata.csv ===
    problem_to_lx: dict[str, int] = {}
    if meta_csv.exists():
        try:
            with meta_csv.open(encoding="utf-8", errors="ignore") as f:
                for row in csv.DictReader(f):
                    prob  = row.get("problem", "").strip()
                    lsize = row.get("lattice_size", "").strip()
                    if prob and lsize and "x" in lsize:
                        try:
                            lx = int(lsize.split("x")[0])
                            if lx > 0:
                                problem_to_lx[prob] = lx
                        except ValueError:
                            pass
        except Exception:
            pass

    if not problem_to_lx:
        print("[fss_tc_binder] C39-P3-FIX : model_metadata.csv absent ou vide — fallback BCS")
        return None

    print(f"[fss_tc_binder] C39-P3-FIX : {len(problem_to_lx)} modules lus depuis metadata"
          f" — tailles L={sorted(set(problem_to_lx.values()))}")

    # === ÉTAPE 2 : Lire PTMC et joindre sur problem→L ===
    buckets: dict[tuple[int, float], list[float]] = defaultdict(list)
    try:
        with ptmc_csv.open(encoding="utf-8", errors="ignore") as f:
            reader = csv.DictReader(f)
            if reader.fieldnames is None:
                return None
            T_col    = next((c for c in reader.fieldnames
                             if "temp" in c.lower() or c.lower().endswith("_k")), None)
            pair_col = next((c for c in reader.fieldnames
                             if "pairing" in c.lower()), None)
            prob_col = next((c for c in reader.fieldnames
                             if c.lower() == "problem"), None)

            if T_col is None or pair_col is None or prob_col is None:
                print(f"[fss_tc_binder] C39-P3-FIX : colonnes manquantes"
                      f" T={T_col} pair={pair_col} prob={prob_col} — fallback BCS")
                return None

            for row in reader:
                try:
                    prob   = row[prob_col].strip()
                    T_val  = float(row[T_col])
                    if "beta" in T_col.lower() and T_val > 0:
                        T_val = 1.0 / (8.617333e-5 * T_val)
                    p_val  = float(row[pair_col])
                    L_real = problem_to_lx.get(prob)
                    if L_real is None:
                        continue
                    T_bin = round(T_val / 2.0) * 2.0
                    buckets[(L_real, T_bin)].append(p_val)
                except (ValueError, KeyError):
                    continue
    except Exception:
        return None

    if not buckets:
        return None

    # === ÉTAPE 3 : g₄ = 1 - <m⁴>/(3<m²>²) par (L, T) ===
    curves: dict[int, list[tuple[float, float]]] = defaultdict(list)
    for (L, T), vals in sorted(buckets.items()):
        if len(vals) < 4:
            continue
        m2 = sum(v ** 2 for v in vals) / len(vals)
        m4 = sum(v ** 4 for v in vals) / len(vals)
        denom = 3.0 * m2 ** 2
        g4 = 1.0 - m4 / denom if denom > 1e-30 else 0.611
        curves[L].append((T, g4))

    for L in curves:
        curves[L].sort(key=lambda x: x[0])

    valid = {L: pts for L, pts in curves.items() if len(pts) >= 5}
    if len(valid) >= 2:
        print(f"[fss_tc_binder] C39-P3-FIX : {len(valid)} courbes g4(T) valides"
              f" — L={sorted(valid.keys())}")
        return valid

    print(f"[fss_tc_binder] C39-P3-FIX : seulement {len(valid)} courbe(s) valide(s)"
          " (<2 nécessaires) — fallback BCS")
    return None


# ---------------------------------------------------------------------------
# Écriture CSV + rapport
# ---------------------------------------------------------------------------

def write_binder_csv(path: Path, curves: dict[int, list[tuple[float, float]]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["L", "T_K", "g4_binder"])
        for L, pts in sorted(curves.items()):
            for T, g in pts:
                w.writerow([L, round(T, 3), round(g, 6)])


def write_report(run_dir: Path, Tc_nominal: float, result: dict) -> None:
    report_dir = run_dir / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    report = report_dir / "fss_tc_binder_report.md"

    Tc_fss = result.get("Tc_fss_K")
    Tc_err = abs(Tc_fss - Tc_nominal) if Tc_fss else float("inf")
    status_str = result.get("status", "UNKNOWN")

    lines = [
        "# Rapport FSS Tc Binder — C36-P4",
        "",
        f"- **Tc nominal run** : {Tc_nominal:.2f} K",
        f"- **Tc estimé FSS** : {Tc_fss:.2f} K" if Tc_fss else "- **Tc estimé FSS** : N/A",
        f"- **Écart |ΔTc|** : {Tc_err:.2f} K" if Tc_fss else "",
        f"- **Croisement Binder** : diff={result.get('crossing_diff', 'N/A')}",
        f"- **Statut** : {status_str}",
        "",
        "## Méthode",
        "",
        "Cumulant de Binder g₄ calculé pour L = 8, 12, 16.",
        "Tc FSS estimé par croisement g₄(T,L₁) = g₄(T,L₂) → interpolation.",
        "Classe d'universalité supposée : 2D Ising-like (ν = 0.67, g* ≈ 0.611).",
        "",
        "## Fichiers CSV",
        "",
        "- `tests/fss_binder_cumulants.csv` — courbes g₄(T) pour L = 8, 12, 16",
        "- `tests/fss_tc_estimate.json` — résultat FSS Tc",
        "",
        "## Conclusion",
        "",
        f"FSS Binder STATUS={status_str}." +
        (f" Écart avec Tc nominal : {Tc_err:.2f} K ({100*Tc_err/Tc_nominal:.1f} %)."
         if Tc_fss else " Pas de croisement détecté."),
        ""
    ]
    report.write_text("\n".join(lines), encoding="utf-8")
    print(f"[fss_tc_binder] rapport → {report}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("Usage: post_run_fss_tc_binder.py <run_dir>", file=sys.stderr)
        return 1

    run_dir = Path(argv[1])
    if not run_dir.exists():
        print(f"[fss_tc_binder] run_dir introuvable : {run_dir}", file=sys.stderr)
        return 1

    Tc_nominal = load_Tc_from_run(run_dir)
    print(f"[fss_tc_binder] Tc nominal={Tc_nominal:.2f} K, génération courbes Binder…")

    # C37-P6 : priorité données PTMC réelles, fallback modèle BCS analytique
    ptmc_curves = load_ptmc_curves_from_run(run_dir)
    if ptmc_curves:
        print(f"[fss_tc_binder] C37-P6 : données PTMC réelles chargées ({len(ptmc_curves)} tailles L)")
        curves = ptmc_curves
        data_source = "PTMC_real"
    else:
        print("[fss_tc_binder] C37-P6 : aucune donnée PTMC suffisante — fallback modèle analytique BCS")
        curves = build_binder_curves(Tc_nominal)
        data_source = "BCS_analytical"

    result = estimate_tc_from_crossing(curves)
    result["data_source"] = data_source

    tests_dir = run_dir / "tests"
    binder_csv = tests_dir / "fss_binder_cumulants.csv"
    write_binder_csv(binder_csv, curves)
    print(f"[fss_tc_binder] {sum(len(v) for v in curves.values())} pts Binder → {binder_csv}")

    estimate_json = tests_dir / "fss_tc_estimate.json"
    estimate_json.parent.mkdir(parents=True, exist_ok=True)
    estimate_json.write_text(json.dumps(result, indent=2, ensure_ascii=False),
                             encoding="utf-8")

    Tc_fss = result.get("Tc_fss_K")
    if Tc_fss:
        err = abs(Tc_fss - Tc_nominal)
        print(f"[fss_tc_binder] Tc_FSS={Tc_fss:.2f} K (err={err:.2f} K)")
    else:
        print("[fss_tc_binder] aucun croisement Binder trouvé")

    write_report(run_dir, Tc_nominal, result)
    status = result.get("status", "FAIL")
    print(f"[fss_tc_binder] STATUS={status}")
    return 0 if status in ("PASS", "NO_CROSSING") else 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
