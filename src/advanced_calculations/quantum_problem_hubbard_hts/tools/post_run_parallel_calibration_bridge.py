#!/usr/bin/env python3
"""Build cross-module calibration bridge from passing tests to failing tests.

No simulation rerun here: this tool creates deterministic, auditable calibration
recommendations from produced artifacts so that successful checks can be reused
for failing modules in a parallel remediation campaign.
"""

from __future__ import annotations

import argparse
import csv
import json
from datetime import datetime, timezone
from pathlib import Path


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def to_float(v: str, default: float = 0.0) -> float:
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", help="Path to research_* run directory")
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    tests = run_dir / "tests"
    logs = run_dir / "logs"
    tests.mkdir(parents=True, exist_ok=True)
    logs.mkdir(parents=True, exist_ok=True)

    metadata = read_csv(tests / "module_physics_metadata.csv")
    progress = read_csv(tests / "integration_problem_solution_progress.csv")
    nstab = read_csv(tests / "numerical_stability_suite.csv")
    new = read_csv(tests / "new_tests_results.csv")

    progress_map = {r.get("problem", ""): to_float(r.get("solution_progress_percent", "0")) for r in progress}
    dt_map = {r.get("module", ""): to_float(r.get("dt", "0.01"), 0.01) for r in metadata}

    drift_rows = {
        r.get("module", ""): to_float(r.get("value", "0"))
        for r in nstab
        if r.get("test_id") == "energy_conservation"
    }

    # Build deterministic campaign plans:
    # - modules with lower drift and higher progress are calibration donors
    # - failing modules receive recommended dt/control attenuation values.
    donors = sorted(
        drift_rows.items(),
        key=lambda kv: (kv[1], -progress_map.get(kv[0], 0.0)),
    )
    donor_modules = [m for m, _ in donors[:3]] if donors else []

    family_fail = {}
    for r in new:
        fam = r.get("test_family", "")
        status = r.get("status", "")
        if status == "FAIL":
            family_fail[fam] = family_fail.get(fam, 0) + 1

    out_rows = []
    for module, drift in sorted(drift_rows.items()):
        base_dt = dt_map.get(module, 0.01)
        progress_pct = progress_map.get(module, 0.0)

        severity = "critical" if drift >= 0.20 else "high" if drift >= 0.12 else "moderate"
        if drift >= 0.20:
            dt_factor = 0.25
            control_gain_factor = 0.40
        elif drift >= 0.12:
            dt_factor = 0.50
            control_gain_factor = 0.55
        else:
            dt_factor = 0.75
            control_gain_factor = 0.70

        rec_dt = base_dt * dt_factor
        out_rows.append(
            {
                "module": module,
                "baseline_dt": f"{base_dt:.6f}",
                "recommended_dt": f"{rec_dt:.6f}",
                "recommended_control_gain_factor": f"{control_gain_factor:.3f}",
                "energy_drift_max": f"{drift:.10f}",
                "progress_percent": f"{progress_pct:.2f}",
                "stability_severity": severity,
                "donor_modules": ";".join(donor_modules),
                "benchmark_parallel": "ENABLED",
            }
        )

    csv_path = tests / "integration_parallel_calibration_bridge.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(
            f,
            fieldnames=[
                "module",
                "baseline_dt",
                "recommended_dt",
                "recommended_control_gain_factor",
                "energy_drift_max",
                "progress_percent",
                "stability_severity",
                "donor_modules",
                "benchmark_parallel",
            ],
        )
        w.writeheader()
        w.writerows(out_rows)

    summary = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "run": run_dir.name,
        "modules": len(out_rows),
        "donor_modules": donor_modules,
        "fail_families": family_fail,
        "bridge_file": str(csv_path),
        "method": {
            "dt_factor_policy": {
                "critical_drift_ge_0.20": 0.25,
                "high_drift_ge_0.12": 0.50,
                "moderate": 0.75,
            },
            "control_gain_factor_policy": {
                "critical": 0.40,
                "high": 0.55,
                "moderate": 0.70,
            },
        },
    }
    (logs / "parallel_calibration_bridge_summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(csv_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
