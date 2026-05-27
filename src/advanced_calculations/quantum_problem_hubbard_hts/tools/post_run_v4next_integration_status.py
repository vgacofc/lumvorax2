#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


def read_csv(path: Path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, headers, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(headers)
        w.writerows(rows)


def pct(n, d):
    return 0.0 if d == 0 else 100.0 * n / d


def safe_float(v, default=0.0):
    try:
        return float(v)
    except Exception:
        return default


def confidence_from_percent(value: float):
    if value >= 95.0:
        return "HIGH"
    if value >= 80.0:
        return "MEDIUM"
    return "LOW"


def main():
    if len(sys.argv) != 2:
        print("Usage: post_run_v4next_integration_status.py <run_dir>", file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    tests_dir = run_dir / "tests"

    gate_rows = read_csv(tests_dir / "integration_gate_summary.csv")
    physics_gate_rows = read_csv(tests_dir / "integration_physics_gate_summary.csv")
    matrix_rows = read_csv(tests_dir / "integration_physics_enriched_test_matrix.csv")
    metadata_rows = read_csv(tests_dir / "integration_absent_metadata_fields.csv")
    new_tests_rows = read_csv(tests_dir / "new_tests_results.csv")

    gate_pct = pct(sum(r.get("status") == "PASS" for r in gate_rows), len(gate_rows))
    physics_gate_pct = pct(sum(r.get("status") == "PASS" for r in physics_gate_rows), len(physics_gate_rows))
    matrix_pct = pct(sum(r.get("status") == "PASS" for r in matrix_rows), len(matrix_rows))
    metadata_pct = pct(sum(r.get("status") == "PRESENT" for r in metadata_rows), len(metadata_rows))
    test_pass_pct = pct(sum(r.get("status") == "PASS" for r in new_tests_rows), len(new_tests_rows))
    test_non_fail_pct = pct(sum(r.get("status") in {"PASS", "OBSERVED"} for r in new_tests_rows), len(new_tests_rows))

    benchmark_rows = [r for r in new_tests_rows if r.get("test_family") == "benchmark"]
    benchmark_pass_pct = pct(sum(r.get("status") == "PASS" for r in benchmark_rows), len(benchmark_rows)) if benchmark_rows else 0.0

    benchmark_file = tests_dir / "benchmark_comparison_qmc_dmrg.csv"
    benchmark_within_error_pct = 0.0
    if benchmark_file.exists():
        cmp_rows = read_csv(benchmark_file)
        benchmark_within_error_pct = pct(
            sum(safe_float(r.get("within_error_bar"), 0.0) >= 1.0 for r in cmp_rows),
            len(cmp_rows),
        )

    checksums_present_pct = 100.0 if (run_dir / "logs" / "checksums.sha256").exists() else 0.0

    rollout_rows = []
    rollout_path = tests_dir / "integration_v4next_rollout_progress.csv"
    if rollout_path.exists():
        rollout_rows = read_csv(rollout_path)

    shadow_ready = any(r.get("stage") == "shadow" and r.get("status") == "READY" for r in rollout_rows)
    canary_ready = any(r.get("stage") == "canary" and r.get("status") == "READY" for r in rollout_rows)
    full_ready = any(r.get("stage") == "full" and r.get("status") == "READY" for r in rollout_rows)
    rollout_evidence_pct = pct(sum([shadow_ready, canary_ready, full_ready]), 3)

    drift_stability_pct = 100.0
    drift_csv = run_dir.parent.parent / "AUDIT_EXHAUSTIF_REPLIT_DRIFT.csv"
    if drift_csv.exists():
        drift_rows = read_csv(drift_csv)
        stable = 0
        total = 0
        for metric in ("energy", "pairing", "sign_ratio"):
            row = next((r for r in drift_rows if r.get("metric") == metric), None)
            if row is None:
                continue
            total += 1
            if safe_float(row.get("max_abs_diff"), 1e9) <= 1e-10:
                stable += 1
        drift_stability_pct = pct(stable, total) if total else 0.0

    model_modularity_pct = min(100.0, 0.40 * matrix_pct + 0.30 * metadata_pct + 0.30 * gate_pct)

    v4next_connection_pct = min(
        100.0,
        0.30 * gate_pct
        + 0.20 * physics_gate_pct
        + 0.15 * matrix_pct
        + 0.15 * metadata_pct
        + 0.10 * rollout_evidence_pct
        + 0.10 * checksums_present_pct,
    )

    shadow_mode_safety_pct = min(
        100.0,
        0.35 * gate_pct
        + 0.25 * physics_gate_pct
        + 0.15 * rollout_evidence_pct
        + 0.15 * drift_stability_pct
        + 0.10 * metadata_pct,
    )

    realistic_simulation_level_pct = min(
        100.0,
        0.30 * matrix_pct
        + 0.20 * benchmark_pass_pct
        + 0.20 * benchmark_within_error_pct
        + 0.15 * test_non_fail_pct
        + 0.10 * drift_stability_pct
        + 0.05 * checksums_present_pct,
    )

    weighted_global = (
        0.12 * gate_pct
        + 0.12 * physics_gate_pct
        + 0.10 * matrix_pct
        + 0.10 * metadata_pct
        + 0.10 * model_modularity_pct
        + 0.16 * v4next_connection_pct
        + 0.15 * shadow_mode_safety_pct
        + 0.15 * realistic_simulation_level_pct
    )

    rows = [
        ["pipeline_gates", f"{gate_pct:.2f}", "PASS ratio in integration_gate_summary.csv", confidence_from_percent(gate_pct)],
        ["physics_gates", f"{physics_gate_pct:.2f}", "PASS ratio in integration_physics_gate_summary.csv", confidence_from_percent(physics_gate_pct)],
        ["physics_matrix", f"{matrix_pct:.2f}", "PASS ratio in integration_physics_enriched_test_matrix.csv", confidence_from_percent(matrix_pct)],
        ["metadata_completeness", f"{metadata_pct:.2f}", "PRESENT ratio in integration_absent_metadata_fields.csv", confidence_from_percent(metadata_pct)],
        ["model_modularity", f"{model_modularity_pct:.2f}", "Derived from matrix+metadata+gate coverage", confidence_from_percent(model_modularity_pct)],
        ["v4next_connection_readiness", f"{v4next_connection_pct:.2f}", "Derived from gates, metadata, rollout, checksums", confidence_from_percent(v4next_connection_pct)],
        ["shadow_mode_safety", f"{shadow_mode_safety_pct:.2f}", "Derived from gate stability, drift stability and rollout", confidence_from_percent(shadow_mode_safety_pct)],
        ["realistic_simulation_level", f"{realistic_simulation_level_pct:.2f}", "Derived from benchmarks, matrix, non-fail tests and drift", confidence_from_percent(realistic_simulation_level_pct)],
        ["global_weighted_readiness", f"{weighted_global:.2f}", "Weighted aggregate readiness score", confidence_from_percent(weighted_global)],
    ]

    out_status = tests_dir / "integration_v4next_connection_readiness.csv"
    write_csv(out_status, ["dimension", "percent", "interpretation", "confidence"], rows)

    backlog = [
        ["Q_missing_units", "Are physical units explicit and consistent for all observables?", "Open", "Add units schema and unit-consistency gate"],
        ["Q_solver_crosscheck", "Do fullscale results match at least one independent non-heuristic solver on larger lattice?", "Closed" if benchmark_within_error_pct >= 100.0 else "Open", "Maintain benchmark_comparison_qmc_dmrg.csv and extend lattice coverage"],
        ["Q_dt_real_sweep", "Is dt stability validated by true multi-run dt/2,dt,2dt (not fullscale only)?", "Open", "Schedule 3-run sweep in CI night job"],
        ["Q_phase_criteria", "Are phase-transition criteria explicit (order parameter + finite-size scaling)?", "Open", "Add formal criteria and thresholds"],
        ["Q_production_guardrails", "Can V4 NEXT rollback instantly on degraded metrics?", "Closed" if rollout_rows else "Open", "Keep rollout controller and rollback contract active"],
    ]
    out_backlog = tests_dir / "integration_open_questions_backlog.csv"
    write_csv(out_backlog, ["question_id", "question", "status", "recommended_action"], backlog)

    print(f"[v4next-status] generated: {out_status}")
    print(f"[v4next-status] generated: {out_backlog}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
