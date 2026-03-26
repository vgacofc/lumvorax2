#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
import pathlib
import statistics
import subprocess
import sys
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[1]
WARP_TOOL = ROOT / "tools" / "run_warp_feasibility_forensic.py"
RESULT_BASE = ROOT / "results" / "warp_validation_suite"


def run_id():
    return datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")


def verify_chain(trace_csv: pathlib.Path):
    with trace_csv.open() as f:
        rows = list(csv.DictReader(f))
    prev = "GENESIS"
    current_scenario = None
    for r in rows:
        if current_scenario is None or r["scenario"] != current_scenario or int(r["step"]) == 0:
            current_scenario = r["scenario"]
            prev = "GENESIS"
        chain_hash = r["chain_hash"]
        logged_prev = r["prev_hash"]
        if logged_prev != prev:
            return False, f"prev_hash mismatch at scenario={r.get('scenario')} step={r.get('step')}"
        payload = {
            "scenario": r["scenario"],
            "model": r["model"],
            "step": int(r["step"]),
            "timestamp_ns": int(r["timestamp_ns"]),
            "grid_points": int(r["grid_points"]),
            "dt_s": float(r["dt_s"]),
            "dr_m": float(r["dr_m"]),
            "v_over_c": float(r["v_over_c"]),
            "radius_m": float(r["radius_m"]),
            "shell_count": int(r["shell_count"]),
            "curvature_proxy_max": float(r["curvature_proxy_max"]),
            "negative_energy_j": float(r["negative_energy_j"]),
            "feasible_now": r["feasible_now"] == "True",
        }
        blob = json.dumps(payload, sort_keys=True).encode("utf-8")
        expected = hashlib.sha256(prev.encode("utf-8") + blob).hexdigest()
        if expected != chain_hash:
            return False, f"chain_hash mismatch at step={r.get('step')}"
        prev = chain_hash
    return True, f"rows={len(rows)}"


def launch_case(case_name, profile, steps, ns_step):
    cid = f"{case_name}_{run_id()}"
    cmd = [
        sys.executable,
        str(WARP_TOOL),
        "--run-id",
        cid,
        "--metric-profile",
        profile,
        "--steps",
        str(steps),
        "--ns-step",
        str(ns_step),
        "--output-dir",
        str(ROOT / "results" / "warp_feasibility"),
    ]
    p = subprocess.run(cmd, text=True, capture_output=True)
    out_dir = pathlib.Path(p.stdout.strip()) if p.returncode == 0 and p.stdout.strip() else (ROOT / "results" / "warp_feasibility" / cid)
    summary = json.loads((out_dir / "warp_feasibility_summary.json").read_text()) if (out_dir / "warp_feasibility_summary.json").exists() else None
    trace = out_dir / "warp_feasibility_trace.csv"
    chain_ok, chain_msg = verify_chain(trace) if trace.exists() else (False, "trace missing")
    return {
        "case": case_name,
        "profile": profile,
        "steps": steps,
        "ns_step": ns_step,
        "returncode": p.returncode,
        "stdout": p.stdout.strip(),
        "stderr": p.stderr.strip(),
        "out_dir": str(out_dir),
        "chain_ok": chain_ok,
        "chain_msg": chain_msg,
        "summary": summary,
    }


def evaluate(cases):
    rates = [c["summary"]["feasible_rate"] for c in cases if c["summary"]]
    energies = [c["summary"]["mean_abs_negative_energy_j"] for c in cases if c["summary"]]
    cps = [c["summary"]["calc_rows_per_second"] for c in cases if c["summary"]]
    all_ok = all(c["returncode"] == 0 and c["chain_ok"] for c in cases)

    return {
        "suite_run_id": run_id(),
        "cases_total": len(cases),
        "all_cases_executed": all(c["returncode"] == 0 for c in cases),
        "all_chains_valid": all(c["chain_ok"] for c in cases),
        "all_ok": all_ok,
        "feasible_rate_mean": statistics.mean(rates) if rates else None,
        "feasible_rate_max": max(rates) if rates else None,
        "energy_mean_j": statistics.mean(energies) if energies else None,
        "energy_min_j": min(energies) if energies else None,
        "calc_rows_per_s_mean": statistics.mean(cps) if cps else None,
        "decision": "NOT_FEASIBLE_PRESENT_PHYSICS" if (rates and max(rates) == 0.0) else "UNDECIDED",
        "validation_protocol": {
            "criteria": [
                "execution_success",
                "forensic_chain_integrity",
                "cross-profile_consistency",
                "energy_scale_plausibility",
            ],
            "pass_condition": "all_ok and feasible_rate_max > 0 for feasibility claim",
        },
        "cases": cases,
    }


def write_report(report):
    out = RESULT_BASE / report["suite_run_id"]
    out.mkdir(parents=True, exist_ok=True)
    (out / "warp_validation_suite.json").write_text(json.dumps(report, indent=2))

    lines = [
        "# Warp validation suite",
        "",
        f"- suite_run_id: {report['suite_run_id']}",
        f"- cases_total: {report['cases_total']}",
        f"- all_cases_executed: {report['all_cases_executed']}",
        f"- all_chains_valid: {report['all_chains_valid']}",
        f"- feasible_rate_mean: {report['feasible_rate_mean']}",
        f"- feasible_rate_max: {report['feasible_rate_max']}",
        f"- energy_mean_j: {report['energy_mean_j']}",
        f"- energy_min_j: {report['energy_min_j']}",
        f"- calc_rows_per_s_mean: {report['calc_rows_per_s_mean']}",
        f"- decision: {report['decision']}",
        "",
        "## Cases",
    ]
    for c in report["cases"]:
        s = c["summary"] or {}
        lines.append(
            f"- {c['case']} ({c['profile']}): rc={c['returncode']}, chain_ok={c['chain_ok']}, "
            f"feasible_rate={s.get('feasible_rate')}, mean_abs_negative_energy_j={s.get('mean_abs_negative_energy_j')}, out={c['out_dir']}"
        )
    (out / "warp_validation_suite.md").write_text("\n".join(lines) + "\n")
    print(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--steps", type=int, default=260)
    ap.add_argument("--ns-step", type=int, default=1000)
    args = ap.parse_args()

    matrix = [
        ("case_conservative", "conservative"),
        ("case_balanced", "balanced"),
        ("case_aggressive", "aggressive"),
    ]

    cases = [launch_case(name, profile, args.steps, args.ns_step) for name, profile in matrix]
    report = evaluate(cases)
    write_report(report)


if __name__ == "__main__":
    main()
