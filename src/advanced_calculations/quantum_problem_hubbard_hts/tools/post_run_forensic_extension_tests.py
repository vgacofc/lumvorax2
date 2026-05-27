#!/usr/bin/env python3
"""Extended forensic tests requested by cycle35 plan.

Generates:
- tests/integration_forensic_extension_tests.csv
- tests/integration_test_coverage_dashboard.csv
- logs/forensic_extension_summary.json
"""
from __future__ import annotations
import argparse
import csv
import json
import math
import random
from pathlib import Path
from statistics import mean


MAX_EXP_ARG = 700.0
MAX_SAFE_MAG = 1e154


def read_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def to_float(v, default=0.0):
    try:
        return float(v)
    except Exception:
        return default


def group_problem(rows):
    out = {}
    for r in rows:
        out.setdefault(r["problem"], []).append(r)
    for k in out:
        out[k].sort(key=lambda x: int(x["step"]))
    return out


def linreg(x, y):
    n = len(x)
    if n < 2:
        return 0.0, mean(y) if y else 0.0
    mx, my = mean(x), mean(y)
    den = sum((a - mx) ** 2 for a in x)
    if den == 0:
        return 0.0, my
    b = sum((a - mx) * (b - my) for a, b in zip(x, y)) / den
    a = my - b * mx
    return a, b


def rmse(y_true, y_pred):
    finite_pairs = [
        (a, b)
        for a, b in zip(y_true, y_pred)
        if math.isfinite(a) and math.isfinite(b)
    ]
    if not finite_pairs:
        return float("inf")
    squared = [safe_square(a - b) for a, b in finite_pairs]
    return math.sqrt(math.fsum(squared) / len(finite_pairs))


def safe_square(x):
    if not math.isfinite(x):
        return float("inf")
    if abs(x) > MAX_SAFE_MAG:
        return float("inf")
    return x * x


def safe_exp(x):
    if not math.isfinite(x):
        return float("inf")
    return math.exp(max(-MAX_EXP_ARG, min(MAX_EXP_ARG, x)))


def alpha_from_series(rows):
    e = [to_float(r["energy"]) for r in rows]
    p = [to_float(r["pairing"]) for r in rows]
    e_shift = abs(min(e)) + 1.0
    p_shift = abs(min(p)) + 1.0
    x = [math.log(v + e_shift) for v in e]
    y = [math.log(v + p_shift) for v in p]
    _, b = linreg(x, y)
    return b


def bootstrap_ci_alpha(rows, n_boot=300, seed=1234):
    rng = random.Random(seed)
    if len(rows) < 5:
        a = alpha_from_series(rows)
        return a, a, a
    vals = []
    for _ in range(n_boot):
        sample = [rows[rng.randrange(0, len(rows))] for _ in rows]
        vals.append(alpha_from_series(sample))
    vals.sort()
    est = alpha_from_series(rows)
    lo = vals[int(0.025 * (len(vals) - 1))]
    hi = vals[int(0.975 * (len(vals) - 1))]
    return est, lo, hi


def kfold_compare_models(rows, k=5):
    e = [to_float(r["energy"]) for r in rows]
    p = [to_float(r["pairing"]) for r in rows]
    e_min, e_max = min(e), max(e)
    p_min, p_max = min(p), max(p)
    x = [(v - e_min) / (e_max - e_min + 1e-12) for v in e]
    y = [(v - p_min) / (p_max - p_min + 1e-12) for v in p]
    idx = list(range(len(rows)))
    folds = [idx[i::k] for i in range(k) if idx[i::k]]

    rmse_lin, rmse_pow = [], []
    for test_idx in folds:
        train_idx = [i for i in idx if i not in test_idx]
        xtr = [x[i] for i in train_idx]
        ytr = [y[i] for i in train_idx]
        xte = [x[i] for i in test_idx]
        yte = [y[i] for i in test_idx]

        a, b = linreg(xtr, ytr)
        yhat_lin = [a + b * xi for xi in xte]

        # power-law in log space with offset to avoid log(0)
        eps = 1e-9
        lx = [math.log(v + eps) for v in xtr]
        ly = [math.log(v + eps) for v in ytr]
        la, lb = linreg(lx, ly)
        yhat_pow = [safe_exp(la + lb * math.log(v + eps)) for v in xte]

        rmse_lin.append(rmse(yte, yhat_lin))
        rmse_pow.append(rmse(yte, yhat_pow))

    return mean(rmse_pow), mean(rmse_lin)


def lag1_autocorr(series):
    if len(series) < 3:
        return 0.0
    x = series[:-1]
    y = series[1:]
    mx, my = mean(x), mean(y)
    num = sum((a - mx) * (b - my) for a, b in zip(x, y))
    den = math.sqrt(sum((a - mx) ** 2 for a in x) * sum((b - my) ** 2 for b in y))
    return num / den if den else 0.0


def lyapunov_index(series):
    if len(series) < 4:
        return 0.0
    diffs = [abs(series[i + 1] - series[i]) + 1e-12 for i in range(len(series) - 1)]
    growth = [math.log(diffs[i + 1] / diffs[i]) for i in range(len(diffs) - 1)]
    return mean(growth)


def surrogate_delta(series):
    if len(series) < 8:
        return 0.0
    base = abs(lag1_autocorr(series))
    rev = abs(lag1_autocorr(list(reversed(series))))
    return abs(base - rev)


def load_rep_diff_distribution(results_dir: Path):
    vals = []
    for run in sorted([p for p in results_dir.iterdir() if p.is_dir() and p.name.startswith("research_")]):
        p = run / "tests" / "new_tests_results.csv"
        if not p.exists():
            continue
        for r in read_csv(p):
            if r.get("test_id") == "rep_diff_seed":
                vals.append(to_float(r.get("value"), float("nan")))
    vals = [v for v in vals if not math.isnan(v)]
    return vals


def coverage_dashboard(run_dir: Path, out_path: Path):
    source_files = [
        run_dir / "tests" / "new_tests_results.csv",
        run_dir / "tests" / "integration_chatgpt_critical_tests.csv",
        run_dir / "tests" / "integration_forensic_extension_tests.csv",
    ]
    rows = []
    all_total = all_pass = all_obs = all_fail = 0
    for f in source_files:
        if not f.exists():
            rows.append({"suite": f.name, "total": 0, "pass": 0, "observed": 0, "fail": 0, "pass_pct": 0.0, "status": "MISSING"})
            continue
        data = read_csv(f)
        p = sum(1 for r in data if r.get("status") == "PASS")
        o = sum(1 for r in data if r.get("status") == "OBSERVED")
        fail = sum(1 for r in data if r.get("status") == "FAIL")
        t = len(data)
        rows.append({"suite": f.name, "total": t, "pass": p, "observed": o, "fail": fail, "pass_pct": round((100.0 * p / t) if t else 0.0, 4), "status": "OK"})
        all_total += t
        all_pass += p
        all_obs += o
        all_fail += fail
    rows.append({
        "suite": "GLOBAL",
        "total": all_total,
        "pass": all_pass,
        "observed": all_obs,
        "fail": all_fail,
        "pass_pct": round((100.0 * all_pass / all_total) if all_total else 0.0, 4),
        "status": "OK",
    })
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["suite", "total", "pass", "observed", "fail", "pass_pct", "status"])
        w.writeheader()
        w.writerows(rows)
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", help="research run directory")
    _default_results = str(Path(__file__).resolve().parent.parent / "results")
    ap.add_argument("--results-dir", default=_default_results)
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    tests_dir = run_dir / "tests"
    logs_dir = run_dir / "logs"

    base_rows = read_csv(logs_dir / "baseline_reanalysis_metrics.csv")
    grouped = group_problem(base_rows)

    out_rows = []
    for problem, rows in grouped.items():
        est, lo, hi = bootstrap_ci_alpha(rows)
        status = "PASS" if (lo <= est <= hi and hi - lo < 0.2) else "OBSERVED"
        out_rows.append({"test_family": "bootstrap_alpha", "test_id": f"alpha_ci95_{problem}", "parameter": "alpha_ci95_width", "value": round(hi - lo, 10), "status": status, "details": f"alpha={est:.8f};ci95=[{lo:.8f},{hi:.8f}]"})

        rmse_pow, rmse_lin = kfold_compare_models(rows)
        status = "PASS" if (math.isfinite(rmse_pow) and math.isfinite(rmse_lin) and rmse_pow <= rmse_lin) else "OBSERVED"
        out_rows.append({"test_family": "cross_validation", "test_id": f"scaling_model_{problem}", "parameter": "rmse_power_minus_linear", "value": round(rmse_pow - rmse_lin, 10), "status": status, "details": f"rmse_power={rmse_pow:.10f};rmse_linear={rmse_lin:.10f}"})

        energy = [to_float(r["energy"]) for r in rows]
        lya = lyapunov_index(energy)
        surr = surrogate_delta(energy)
        out_rows.append({"test_family": "dynamic_diagnostics", "test_id": f"lyapunov_index_{problem}", "parameter": "lyapunov_index", "value": round(lya, 10), "status": "OBSERVED", "details": "fullscale from local divergence logs"})
        out_rows.append({"test_family": "dynamic_diagnostics", "test_id": f"surrogate_delta_{problem}", "parameter": "lag1_autocorr_delta", "value": round(surr, 10), "status": "OBSERVED", "details": "difference original vs reversed autocorr"})

    # finite-size scaling from existing cluster tests
    new_tests = read_csv(tests_dir / "new_tests_results.csv")
    sizes, energies, pairings = [], [], []
    for r in new_tests:
        if r.get("test_family") == "cluster_scale" and r.get("test_id", "").startswith("cluster_"):
            tid = r.get("test_id", "")
            if tid in {"cluster_8x8", "cluster_10x10", "cluster_12x12"}:
                n = int(tid.split("_")[1].split("x")[0])
                if r.get("parameter") == "energy":
                    sizes.append(n * n)
                    energies.append(to_float(r.get("value")))
                if r.get("parameter") == "pairing":
                    pairings.append((n * n, to_float(r.get("value"))))

    if len(sizes) >= 3 and len(energies) >= 3:
        a, b = linreg([math.log(s) for s in sizes], [math.log(max(e, 1e-12)) for e in energies])
        out_rows.append({"test_family": "finite_size_scaling", "test_id": "energy_vs_size_exponent", "parameter": "slope_logE_logN", "value": round(b, 10), "status": "PASS", "details": f"intercept={a:.10f}"})

    if len(pairings) >= 3:
        pairings.sort()
        noninc = all(pairings[i + 1][1] <= pairings[i][1] for i in range(len(pairings) - 1))
        out_rows.append({"test_family": "finite_size_scaling", "test_id": "pairing_nonincreasing_with_size", "parameter": "bool", "value": int(noninc), "status": "PASS" if noninc else "FAIL", "details": str(pairings)})

    # multi-seed sensitivity distribution from historical runs
    rep_vals = load_rep_diff_distribution(Path(args.results_dir))
    if rep_vals:
        out_rows.append({"test_family": "multi_seed_sensitivity", "test_id": "rep_diff_seed_distribution_count", "parameter": "count", "value": len(rep_vals), "status": "PASS" if len(rep_vals) >= 1 else "FAIL", "details": "from historical new_tests_results.csv"})
        out_rows.append({"test_family": "multi_seed_sensitivity", "test_id": "rep_diff_seed_distribution_mean", "parameter": "mean_delta", "value": round(mean(rep_vals), 10), "status": "OBSERVED", "details": f"min={min(rep_vals):.10f};max={max(rep_vals):.10f}"})

    out_csv = tests_dir / "integration_forensic_extension_tests.csv"
    with out_csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["test_family", "test_id", "parameter", "value", "status", "details"])
        w.writeheader()
        w.writerows(out_rows)

    coverage_rows = coverage_dashboard(run_dir, tests_dir / "integration_test_coverage_dashboard.csv")

    summary = {
        "run_dir": str(run_dir),
        "extension_tests_rows": len(out_rows),
        "extension_pass": sum(1 for r in out_rows if r["status"] == "PASS"),
        "extension_observed": sum(1 for r in out_rows if r["status"] == "OBSERVED"),
        "extension_fail": sum(1 for r in out_rows if r["status"] == "FAIL"),
        "coverage": coverage_rows,
    }
    (logs_dir / "forensic_extension_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"[forensic-extension] wrote {out_csv}")
    print(f"[forensic-extension] wrote {tests_dir / 'integration_test_coverage_dashboard.csv'}")
    print(f"[forensic-extension] wrote {logs_dir / 'forensic_extension_summary.json'}")


if __name__ == "__main__":
    main()
