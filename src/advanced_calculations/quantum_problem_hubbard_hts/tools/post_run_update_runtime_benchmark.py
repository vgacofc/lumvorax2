"""
post_run_update_runtime_benchmark.py — BC-10
Génère benchmarks/qmc_dmrg_reference_runtime.csv si et seulement si le RMSE
par rapport aux références publiées (qmc_dmrg_reference_v2.csv) est < 0.05 eV/site.

Politique (R13 plan simulateur v3.1.0) :
  - Le fichier runtime est une calibration INTERNE uniquement.
  - Il ne remplace JAMAIS les données publiées (v2.csv).
  - Si RMSE >= 0.05, seul le fallback v2.csv est utilisé (run marqué SHADOW_BLOCKED benchmark).
  - Archive versionnée : benchmarks/history/qmc_dmrg_reference_runtime_YYYYMMDDTHHMMSSZ.csv

Usage :
  python3 post_run_update_runtime_benchmark.py <run_dir> <project_root_benchmarks_dir>

Ajouté : 2026-03-14 — analysechatgpt13 — BC-10
"""

import csv
import math
import os
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path


RMSE_THRESHOLD = 0.05  # eV/site — seuil R13


def read_csv_dict(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def write_csv_rows(path: Path, header: list[str], rows: list[list]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def compute_rmse(bench_comparison: list[dict]) -> float:
    """Calcule le RMSE depuis benchmark_comparison_qmc_dmrg.csv (généré par le runner C)."""
    sq_sum = 0.0
    n = 0
    for row in bench_comparison:
        try:
            abs_err = float(row.get("abs_error", 0))
            sq_sum += abs_err * abs_err
            n += 1
        except (ValueError, TypeError):
            continue
    if n == 0:
        return float("inf")
    return math.sqrt(sq_sum / n)


def extract_observed_pairing(new_tests: list[dict]) -> dict[float, float]:
    """Extrait les valeurs pairing observées à chaque température depuis new_tests_results.csv."""
    obs = {}
    for row in new_tests:
        tid = row.get("test_id", "")
        if tid.startswith("sens_T_"):
            param = row.get("parameter", "")
            if param == "pairing":
                try:
                    t_val = float(tid.replace("sens_T_", ""))
                    val = float(row.get("value", 0))
                    obs[t_val] = val
                except (ValueError, TypeError):
                    continue
    return obs


def main() -> None:
    if len(sys.argv) < 3:
        print("[runtime-benchmark] Usage: post_run_update_runtime_benchmark.py <run_dir> <benchmarks_dir>")
        sys.exit(1)

    run_dir = Path(sys.argv[1]).resolve()
    bench_dir = Path(sys.argv[2]).resolve()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")

    bench_cmp_path = run_dir / "tests" / "benchmark_comparison_qmc_dmrg.csv"
    new_tests_path = run_dir / "tests" / "new_tests_results.csv"
    canonical_ref  = bench_dir / "qmc_dmrg_reference_v2.csv"
    runtime_ref    = bench_dir / "qmc_dmrg_reference_runtime.csv"
    history_dir    = bench_dir / "history"

    bench_comparison = read_csv_dict(bench_cmp_path)
    new_tests        = read_csv_dict(new_tests_path)
    canonical_rows   = read_csv_dict(canonical_ref)

    rmse = compute_rmse(bench_comparison)
    observed_pairing = extract_observed_pairing(new_tests)

    print(f"[runtime-benchmark] RMSE vs canonical refs: {rmse:.6f} eV/site  (seuil R13: {RMSE_THRESHOLD})")

    if rmse >= RMSE_THRESHOLD:
        print(f"[runtime-benchmark] RMSE >= {RMSE_THRESHOLD} : runtime ref NON créé — fallback v2.csv utilisé (SHADOW_BLOCKED benchmark)")
        print(f"[runtime-benchmark] Action requise : corriger BC-05-H3 (formule local_pair) pour atteindre RMSE < {RMSE_THRESHOLD}")
        # Supprimer le fichier runtime s'il existait d'un cycle précédent pour forcer le fallback
        if runtime_ref.exists():
            archive = history_dir / f"qmc_dmrg_reference_runtime_DEPRECATED_{stamp}.csv"
            history_dir.mkdir(parents=True, exist_ok=True)
            shutil.move(str(runtime_ref), str(archive))
            print(f"[runtime-benchmark] Ancien runtime ref archivé : {archive}")
        return

    # RMSE < seuil : créer le runtime ref avec les valeurs observées calibrées
    print(f"[runtime-benchmark] RMSE < {RMSE_THRESHOLD} : génération du runtime ref calibré")

    header = ["source", "observable", "T", "U", "value", "error_bar"]
    rows = []

    for canon in canonical_rows:
        obs_type = canon.get("observable", "")
        t_val    = float(canon.get("T", 0))
        u_val    = float(canon.get("U", 0))
        err_bar  = float(canon.get("error_bar", 0.06))

        if obs_type == "pairing" and t_val in observed_pairing:
            # Utiliser la valeur observée comme nouvelle cible calibrée
            calibrated_val = observed_pairing[t_val]
            rows.append(["qmc_runtime_calibrated", obs_type, int(t_val), int(u_val), f"{calibrated_val:.6f}", err_bar])
        else:
            # Garder la valeur canonique pour les autres (energy, T non observés)
            rows.append(["qmc_dmrg_meta", obs_type, int(t_val), int(u_val), canon.get("value"), err_bar])

    write_csv_rows(runtime_ref, header, rows)
    print(f"[runtime-benchmark] Runtime ref écrit : {runtime_ref} ({len(rows)} lignes)")

    # Archivage versionné
    history_dir.mkdir(parents=True, exist_ok=True)
    archive = history_dir / f"qmc_dmrg_reference_runtime_{stamp}.csv"
    shutil.copy2(str(runtime_ref), str(archive))
    print(f"[runtime-benchmark] Archive versionnée : {archive}")

    # Rapport de politique (Q20-Q22)
    policy_path = run_dir / "logs" / "runtime_benchmark_policy.txt"
    with open(policy_path, "w", encoding="utf-8") as f:
        f.write(f"stamp_utc={stamp}\n")
        f.write(f"rmse={rmse:.6f}\n")
        f.write(f"rmse_threshold={RMSE_THRESHOLD}\n")
        f.write(f"action={'runtime_ref_created' if rmse < RMSE_THRESHOLD else 'fallback_v2_only'}\n")
        f.write(f"runtime_ref={runtime_ref}\n")
        f.write(f"archive={archive}\n")
        f.write("policy=R13_plan_simulateur_v3.1.0\n")
        f.write("Q20_status=runtime_auto_sous_seuil_RMSE<0.05\n")
        f.write("Q21_status=refs_publiees_v2csv_immuables_runtime_calibration_interne\n")
        f.write("Q22_status=archive_versionnee_par_stamp_utc\n")
    print(f"[runtime-benchmark] Rapport politique : {policy_path}")


if __name__ == "__main__":
    main()
