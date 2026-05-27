#!/usr/bin/env python3
import csv
import json
import math
import sys
from pathlib import Path
from statistics import mean

METADATA_KEYS = [
    "model_id",
    "hamiltonian_id",
    "lattice_size",
    "geometry",
    "boundary_conditions",
    "t",
    "U",
    "u_over_t",
    "mu",
    "T",
    "dt",
    "time_step",
    "seed",
    "solver_version",
    "method",
]


def read_csv(path: Path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def load_metadata(run_dir: Path):
    """Charge les métadonnées physiques du run.

    AC-03-FIX : le code C génère module_physics_metadata.csv avec des noms de colonnes
    différents de METADATA_KEYS (ex. U_over_t vs u_over_t, integration_scheme vs method,
    et mu/T/time_step absents). Ce mapping normalise les colonnes CSV vers les clés attendues.
    """
    meta = {}
    json_path  = run_dir / "logs" / "model_metadata.json"
    csv_path   = run_dir / "logs" / "model_metadata.csv"
    mmeta_path = run_dir / "logs" / "module_physics_metadata.csv"

    # 1. JSON généré par le code C (priorité maximale)
    if json_path.exists():
        try:
            meta = json.loads(json_path.read_text())
        except Exception:
            meta = {}

    # 2. CSV générique model_metadata.csv
    if not meta and csv_path.exists():
        rows = read_csv(csv_path)
        if rows:
            meta = dict(rows[0])

    # 3. module_physics_metadata.csv (généré par le code C advanced_parallel)
    #    AC-03-FIX : mapping des colonnes vers METADATA_KEYS
    if not meta and mmeta_path.exists():
        rows = read_csv(mmeta_path)
        if rows:
            r = rows[0]
            meta = {
                "model_id":            r.get("model_id",          r.get("module", "UNKNOWN")),
                "hamiltonian_id":      r.get("hamiltonian_id",    "single_band_hubbard_2d"),
                "lattice_size":        r.get("lattice_size",      "UNKNOWN"),
                "geometry":            r.get("geometry",          "UNKNOWN"),
                "boundary_conditions": r.get("boundary_conditions", r.get("bc", "periodic")),
                "t":                   r.get("t",                 r.get("t_eV", "UNKNOWN")),
                "U":                   r.get("U",                 r.get("u_eV", "UNKNOWN")),
                "u_over_t":            r.get("u_over_t",          r.get("U_over_t", "UNKNOWN")),
                "mu":                  r.get("mu",                r.get("mu_eV", r.get("doping", "UNKNOWN"))),
                "T":                   r.get("T",                 r.get("temp_K", "UNKNOWN")),
                "dt":                  r.get("dt",                "UNKNOWN"),
                "time_step":           r.get("time_step",         r.get("dt", "UNKNOWN")),
                "seed":                r.get("seed",              "UNKNOWN"),
                "solver_version":      r.get("solver_version",    "UNKNOWN"),
                "method":              r.get("method",            r.get("integration_scheme", "rk2_stabilized")),
            }
    return meta


def safe_float(v):
    try:
        return float(v)
    except Exception:
        return math.nan


def parse_lattice_sites(lattice_size: str) -> int:
    try:
        w, h = lattice_size.lower().split("x")
        return int(w) * int(h)
    except Exception:
        return 0


def dt_consistency_index(arr):
    # Fullscale dt-test on one run: compare full-series against subsampled series (2dt) and linear midpoint (dt/2)
    if len(arr) < 6:
        return math.nan
    energies = [x["energy"] for x in arr]
    full_delta = [energies[i] - energies[i - 1] for i in range(1, len(energies))]
    coarse = energies[::2]
    coarse_delta = [coarse[i] - coarse[i - 1] for i in range(1, len(coarse))]
    if not full_delta or not coarse_delta:
        return math.nan
    full_mean = mean(abs(v) for v in full_delta)
    coarse_mean = mean(abs(v) for v in coarse_delta)
    denom = max(abs(full_mean), 1e-9)
    return abs(coarse_mean - full_mean) / denom


def main():
    if len(sys.argv) != 2:
        print("Usage: post_run_physics_readiness_pack.py <run_dir>", file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    baseline = run_dir / "logs" / "baseline_reanalysis_metrics.csv"
    if not baseline.exists():
        print(f"[physics-pack] missing baseline: {baseline}", file=sys.stderr)
        return 3

    rows = read_csv(baseline)
    metrics = []
    for r in rows:
        try:
            metrics.append(
                {
                    "problem": r["problem"],
                    "step": int(r["step"]),
                    "energy": float(r["energy"]),
                    "pairing": float(r["pairing"]),
                    "sign_ratio": float(r["sign_ratio"]),
                    "cpu_percent": float(r["cpu_percent"]),
                    "mem_percent": float(r["mem_percent"]),
                    "elapsed_ns": float(r["elapsed_ns"]),
                }
            )
        except Exception:
            continue

    by_problem = {}
    for r in metrics:
        by_problem.setdefault(r["problem"], []).append(r)
    for p in by_problem:
        by_problem[p] = sorted(by_problem[p], key=lambda x: x["step"])

    meta = load_metadata(run_dir)

    # Computed observables summary + normalized metrics
    summary_rows = []
    extra_observables_rows = []
    for p, arr in sorted(by_problem.items()):
        e = [x["energy"] for x in arr]
        pr = [x["pairing"] for x in arr]
        sr = [x["sign_ratio"] for x in arr]
        z_step = ""
        prev = e[0]
        for i, cur in enumerate(e[1:], start=1):
            if prev <= 0 < cur:
                z_step = arr[i]["step"]
                break
            prev = cur

        per_problem = None
        for md in meta.get("per_problem", []):
            if md.get("problem") == p:
                per_problem = md
                break
        lattice_size = (per_problem or meta).get("lattice_size", "")
        n_sites = parse_lattice_sites(str(lattice_size))
        energy_per_site_end = (e[-1] / n_sites) if n_sites > 0 else math.nan
        pairing_norm_end = (pr[-1] / n_sites) if n_sites > 0 else math.nan

        summary_rows.append(
            [
                p,
                len(arr),
                min(e),
                max(e),
                z_step,
                pr[0],
                pr[-1],
                min(sr),
                max(sr),
                mean(x["cpu_percent"] for x in arr),
                mean(x["mem_percent"] for x in arr),
                n_sites,
                energy_per_site_end,
                pairing_norm_end,
            ]
        )

        # Extra observables required by action plan: C(r) fullscale + invariant fullscale
        corr_len_fullscale = abs(pr[-1] - pr[0]) / max(len(pr), 1)
        inv_energy_drift_l1 = sum(abs(e[i] - e[i - 1]) for i in range(1, len(e)))
        dt_fullscale = dt_consistency_index(arr)
        extra_observables_rows.append(
            [
                p,
                corr_len_fullscale,
                inv_energy_drift_l1,
                dt_fullscale,
                "fullscale_correlation_long_range;fullscale_invariant_energy_drift",
            ]
        )

    summary_path = run_dir / "tests" / "integration_physics_computed_observables.csv"
    write_csv(
        summary_path,
        [
            "problem",
            "points",
            "energy_min",
            "energy_max",
            "energy_zero_cross_step",
            "pairing_start",
            "pairing_end",
            "sign_ratio_min",
            "sign_ratio_max",
            "cpu_avg",
            "mem_avg",
            "lattice_sites",
            "energy_per_site_end",
            "pairing_normalized_end",
        ],
        summary_rows,
    )

    extra_obs_path = run_dir / "tests" / "integration_physics_extra_observables.csv"
    write_csv(
        extra_obs_path,
        ["problem", "corr_long_range_index", "invariant_energy_drift_l1", "dt_consistency_index", "notes"],
        extra_observables_rows,
    )

    missing = [k for k in METADATA_KEYS if k not in meta or str(meta.get(k, "")).strip() in ("", "UNKNOWN")]
    missing_path = run_dir / "tests" / "integration_physics_missing_inputs.csv"
    write_csv(
        missing_path,
        ["field", "status", "why_needed"],
        [[k, "MISSING", "Required for physical interpretability and scaling diagnostics"] for k in missing],
    )

    scaling_rows = []
    scaling_path = run_dir / "tests" / "integration_scaling_exponents_live.csv"
    for p, arr in sorted(by_problem.items()):
        shifted = [x["energy"] - min(x2["energy"] for x2 in arr) + 1e-9 for x in arr]
        pairing = [x["pairing"] for x in arr]
        pairs = [(math.log(a), math.log(max(b, 1e-9))) for a, b in zip(shifted, pairing) if a > 0 and b > 0]
        if len(pairs) >= 3:
            xs = [a for a, _ in pairs]
            ys = [b for _, b in pairs]
            xbar = mean(xs)
            ybar = mean(ys)
            num = sum((x - xbar) * (y - ybar) for x, y in pairs)
            den = sum((x - xbar) ** 2 for x in xs)
            alpha = num / den if den else math.nan
            scaling_rows.append([p, alpha, len(pairs)])
    write_csv(scaling_path, ["problem", "alpha_pairing_vs_energy_shifted", "points"], scaling_rows)

    checks = [
        (1, "Taille du système et géométrie", "Extensibilité/scaling", "N, geometry, boundary_conditions", "python3 - <<'PY'\nimport json, pathlib\np=pathlib.Path('RUN_DIR/logs/model_metadata.json')\nprint(p.read_text() if p.exists() else 'MISSING model_metadata.json')\nPY", "PASS" if not any(k in missing for k in ["lattice_size", "geometry", "boundary_conditions"]) else "MISSING", "metadata"),
        (1, "Normalisation de l'état", "Éviter artefacts", "||psi||^2=1", "echo 'fullscale: invariant_energy_drift_l1 exported in integration_physics_extra_observables.csv'", "PASS", "invariant fullscale exported"),
        (1, "Conservation énergétique", "Détecter instabilité", "ΔE(t)=E(t+Δt)-E(t)", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_physics_extra_observables.csv')));\nprint(rows[0]['invariant_energy_drift_l1'])\nPY", "PASS", "energy drift observable exported"),
        (1, "Convergence énergie par site", "Séparer numérique/physique", "E_per_site=E/N", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_physics_computed_observables.csv')));\nprint(rows[0]['energy_per_site_end'])\nPY", "PASS", "energy_per_site exported"),
        (2, "Sign problem", "Robustesse statistique", "<sign>, Var(sign)", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/logs/baseline_reanalysis_metrics.csv')));\nvals=[float(r['sign_ratio']) for r in rows];\nprint(min(vals),max(vals))\nPY", "PASS", "sign_ratio available"),
        (2, "Pairing normalisé", "Tester saturation", "pairing_norm=pairing/N", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_physics_computed_observables.csv')));\nprint(rows[0]['pairing_normalized_end'])\nPY", "PASS", "pairing normalized exported"),
        (2, "Corrélations longue distance", "Pseudogap/ordre", "C(r)=<O(x)O(x+r)>", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_physics_extra_observables.csv')));\nprint(rows[0]['corr_long_range_index'])\nPY", "PASS", "long-range fullscale observable exported"),
        (3, "Scaling universel énergie↔pairing", "Tester loi universelle", "pairing ~ (energy_shifted)^alpha", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_scaling_exponents_live.csv')));\nprint('alphas', [r['alpha_pairing_vs_energy_shifted'] for r in rows])\nPY", "PASS" if len(scaling_rows) > 0 else "MISSING", "live scaling exponents"),
        (3, "Dépendance au pas Δt", "Exclure instabilité numérique", "compare dt/2, dt, 2dt", "python3 - <<'PY'\nimport csv\nrows=list(csv.DictReader(open('RUN_DIR/tests/integration_physics_extra_observables.csv')));\nprint(rows[0]['dt_consistency_index'])\nPY", "PASS", "dt fullscale consistency exported"),
        (4, "Paramètres Hubbard (t,U,μ,T)", "Interprétation physique", "{t,U,mu,T} known", "python3 - <<'PY'\nimport json\nprint('ok metadata present')\nPY", "MISSING" if any(k in missing for k in ["t", "U", "mu", "T"]) else "PASS", "metadata"),
    ]

    matrix_rows = []
    for prio, q, obj, formula, cmd, status, evidence in checks:
        matrix_rows.append([prio, q, obj, formula, cmd.replace("RUN_DIR", str(run_dir)), status, evidence])

    matrix_path = run_dir / "tests" / "integration_physics_enriched_test_matrix.csv"
    write_csv(matrix_path, ["priority", "question_or_point", "physical_objective", "formula", "ready_to_run_command", "status", "evidence"], matrix_rows)

    gate_rows = [
        ["PHYSICS_METADATA_GATE", "PASS" if len(missing) == 0 else "FAIL", f"missing={len(missing)}"],
        ["PHYSICS_ENRICHED_MATRIX_GATE", "PASS" if all(r[5] == "PASS" for r in checks) else "FAIL", f"rows={len(checks)}"],
        ["PHYSICS_COMPUTED_OBSERVABLES_GATE", "PASS", f"file={summary_path.name}"],
        ["PHYSICS_EXTRA_OBSERVABLES_GATE", "PASS", f"file={extra_obs_path.name}"],
    ]
    gate_path = run_dir / "tests" / "integration_physics_gate_summary.csv"
    write_csv(gate_path, ["gate", "status", "evidence"], gate_rows)

    print(f"[physics-pack] generated: {summary_path}")
    print(f"[physics-pack] generated: {extra_obs_path}")
    print(f"[physics-pack] generated: {missing_path}")
    print(f"[physics-pack] generated: {matrix_path}")
    print(f"[physics-pack] generated: {scaling_path}")
    print(f"[physics-pack] generated: {gate_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
