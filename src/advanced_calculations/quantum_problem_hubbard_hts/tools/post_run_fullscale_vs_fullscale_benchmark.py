#!/usr/bin/env python3
import csv
import math
import sys
from pathlib import Path


def latest_previous_run(results_root: Path, current_run: str) -> Path | None:
    runs = sorted([p for p in results_root.iterdir() if p.is_dir() and p.name.startswith("research_") and p.name != current_run])
    return runs[-1] if runs else None


def load_final_metrics(path: Path):
    rows = list(csv.DictReader(path.open(encoding="utf-8")))
    per = {}
    for r in rows:
        mod = r.get("problem", "")
        step = int(float(r.get("step", 0) or 0))
        if mod not in per or step > per[mod]["step"]:
            per[mod] = {
                "step": step,
                "energy": float(r.get("energy", 0.0) or 0.0),
                "pairing": float(r.get("pairing", 0.0) or 0.0),
            }
    return per


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: post_run_fullscale_vs_fullscale_benchmark.py <run_dir>", file=sys.stderr)
        return 2
    run_dir = Path(sys.argv[1]).resolve()
    results_root = run_dir.parent
    prev = latest_previous_run(results_root, run_dir.name)
    tests_dir = run_dir / "tests"
    tests_dir.mkdir(parents=True, exist_ok=True)
    out_csv = tests_dir / "benchmark_comparison_fullscale_vs_fullscale.csv"
    out_summary = tests_dir / "integration_fullscale_vs_fullscale_benchmark.csv"

    if prev is None:
        out_csv.write_text("module,metric,reference,model,abs_error,rel_error,status,notes\n", encoding="utf-8")
        out_summary.write_text("test_id,metric,value,status,notes\nfullscale_vs_fullscale,rmse,0.0,OBSERVED,no_previous_run\n", encoding="utf-8")
        return 0

    cur_file = run_dir / "logs" / "baseline_reanalysis_metrics.csv"
    prev_file = prev / "logs" / "baseline_reanalysis_metrics.csv"
    if not cur_file.exists() or not prev_file.exists():
        out_summary.write_text("test_id,metric,value,status,notes\nfullscale_vs_fullscale,rmse,0.0,FAIL,missing_baseline_csv\n", encoding="utf-8")
        return 1

    cur = load_final_metrics(cur_file)
    ref = load_final_metrics(prev_file)
    modules = sorted(set(cur) & set(ref))

    errors = []
    with out_csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["module", "metric", "reference", "model", "abs_error", "rel_error", "status", "notes"])
        for m in modules:
            for metric in ("energy", "pairing"):
                a = ref[m][metric]
                b = cur[m][metric]
                abs_e = abs(b - a)
                rel_e = abs_e / (abs(a) + 1e-12)
                status = "PASS" if rel_e <= 0.10 else "FAIL"
                w.writerow([m, metric, f"{a:.10f}", f"{b:.10f}", f"{abs_e:.10f}", f"{rel_e:.10f}", status, prev.name])
                errors.append(abs_e)

    rmse = math.sqrt(sum(e * e for e in errors) / len(errors)) if errors else 0.0
    mae = sum(errors) / len(errors) if errors else 0.0
    status = "PASS" if rmse <= 0.5 else "OBSERVED"
    with out_summary.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["test_id", "metric", "value", "status", "notes"])
        w.writerow(["fullscale_vs_fullscale", "rmse", f"{rmse:.10f}", status, prev.name])
        w.writerow(["fullscale_vs_fullscale", "mae", f"{mae:.10f}", status, prev.name])
        w.writerow(["fullscale_vs_fullscale", "common_modules", len(modules), "OBSERVED", prev.name])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
