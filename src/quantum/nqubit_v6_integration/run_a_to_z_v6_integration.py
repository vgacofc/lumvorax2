#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
RESULTS = ROOT / "results"

PHASES = [
    ("A_collect_metrics", [sys.executable, "tools/collect_system_metrics_v6.py"]),
    ("B_acquire_hardware", [sys.executable, "tools/acquire_real_hardware_v6.py", "--output-dir", "results", "--bytes", "32768", "--jitter-samples", "50000"]),
    ("C_stress", [sys.executable, "tools/stress_harness_v6.py", "--seconds", "4", "--workers", "64", "--target", "99", "--output", "results/stress_telemetry_v6.log"]),
    ("D_build_manifest", [sys.executable, "tools/build_manifest_v6.py", "--input-dir", "results", "--output", "results/manifest_forensic_v6.json", "--exclude", "manifest_forensic_v6.json", "manifest_forensic_v6.json.sig"]),
    ("E_verify_manifest", [sys.executable, "tools/verify_manifest_v6.py", "--manifest", "results/manifest_forensic_v6.json"]),
    ("F_generate_report", [sys.executable, "tools/generate_v6_post_run_report.py", "--manifest", "results/manifest_forensic_v6.json", "--adc-report", "results/ADC_REAL_HARDWARE_CAPTURE_V6.json", "--stress-log", "results/stress_telemetry_v6.log", "--system-metrics", "results/system_metrics_v6.json", "--output", "RAPPORT_REPLIT_EXECUTION_V6_FINAL.md"]),
]


def run():
    RESULTS.mkdir(parents=True, exist_ok=True)
    phase_results = []
    ok = True
    for name, cmd in PHASES:
        p = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
        phase_results.append({
            "phase": name,
            "rc": p.returncode,
            "cmd": " ".join(cmd),
            "stdout": p.stdout.strip(),
            "stderr": p.stderr.strip(),
        })
        if p.returncode != 0:
            ok = False
            break

    out = {
        "pipeline": "nqubit_v6_integration_a_to_z",
        "ok": ok,
        "phases": phase_results,
    }
    (RESULTS / "a_to_z_run_summary.json").write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(out, indent=2))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(run())
