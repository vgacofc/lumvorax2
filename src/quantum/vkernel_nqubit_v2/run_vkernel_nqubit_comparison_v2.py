#!/usr/bin/env python3
import json
import os
import platform
import statistics
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SRC_Q = ROOT / "src" / "quantum"
V2_ROOT = SRC_Q / "vkernel_nqubit_v2"
RESULTS_ROOT = V2_ROOT / "results"


def run(cmd):
    p = subprocess.run(cmd, text=True, capture_output=True, check=False)
    return {"cmd": " ".join(cmd), "rc": p.returncode, "stdout": p.stdout, "stderr": p.stderr}


def parse_kv(stdout):
    out = {}
    for ln in stdout.splitlines():
        if "=" in ln:
            k, v = ln.split("=", 1)
            try:
                out[k.strip()] = float(v.strip())
            except ValueError:
                out[k.strip()] = v.strip()
    return out


def probe_entropy(path, nbytes=4096):
    t0 = time.perf_counter()
    try:
        with open(path, "rb", buffering=0) as f:
            data = f.read(nbytes)
        dt = max(time.perf_counter() - t0, 1e-12)
        return {"path": path, "ok": True, "size": len(data), "bytes_per_sec": len(data) / dt, "error": None}
    except Exception as e:
        return {"path": path, "ok": False, "size": 0, "bytes_per_sec": None, "error": str(e)}


def load_old_reports_reference():
    files = sorted(SRC_Q.glob("*.md"))
    refs = []
    for f in files:
        name = f.name
        if "RAPPORT" in name or "ANALYSE" in name or "PLAN" in name:
            refs.append({"file": str(f.relative_to(ROOT)), "mtime_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(f.stat().st_mtime))})
    return refs


def main():
    RESULTS_ROOT.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
    out_dir = RESULTS_ROOT / run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    baseline_bin = SRC_Q / "v_kernel_quantum_bin"
    nx_v2_bin = V2_ROOT / "v_kernel_quantum_nx_v2_bin"
    nx_log = out_dir / "nqubit_forensic_ns_v2.jsonl"

    c1 = run(["gcc", str(SRC_Q / "v_kernel_quantum.c"), "-O2", "-o", str(baseline_bin)])
    c2 = run(["gcc", str(V2_ROOT / "v_kernel_quantum_nx_v2.c"), "-O2", "-lm", "-o", str(nx_v2_bin)])

    before_log_lines = 0
    legacy_dir = ROOT / "logs_AIMO3" / "v46"
    legacy_dir.mkdir(parents=True, exist_ok=True)
    legacy_log = legacy_dir / "hardware_metrics.log"
    if legacy_log.exists():
        before_log_lines = len(legacy_log.read_text(encoding="utf-8", errors="ignore").splitlines())

    r1 = run([str(baseline_bin)]) if c1["rc"] == 0 else {"rc": 1, "stdout": "", "stderr": "baseline compile failed", "cmd": str(baseline_bin)}
    r2 = run([str(nx_v2_bin), str(nx_log), "360", "1400"]) if c2["rc"] == 0 else {"rc": 1, "stdout": "", "stderr": "nx v2 compile failed", "cmd": str(nx_v2_bin)}

    after_log_lines = before_log_lines
    if legacy_log.exists():
        after_log_lines = len(legacy_log.read_text(encoding="utf-8", errors="ignore").splitlines())

    m2 = parse_kv(r2.get("stdout", ""))
    baseline_metric_lines_added = max(0, after_log_lines - before_log_lines)

    entropy = [probe_entropy("/dev/hwrng"), probe_entropy("/dev/random"), probe_entropy("/dev/urandom")]
    old_runs = []
    for p in sorted((SRC_Q / "results_vkernel_compare").glob("*/comparison_summary.json")):
        try:
            j = json.loads(p.read_text())
            old_runs.append({"run": p.parent.name, "metrics": j.get("comparison_metrics", {})})
        except Exception:
            pass

    summary = {
        "run_id": run_id,
        "paths": {
            "out_dir": str(out_dir.relative_to(ROOT)),
            "nx_log": str(nx_log.relative_to(ROOT)),
            "legacy_log": str(legacy_log.relative_to(ROOT)) if legacy_log.exists() else "logs_AIMO3/v46/hardware_metrics.log",
        },
        "commands": {"compile_baseline": c1, "compile_nx_v2": c2, "run_baseline": r1, "run_nx_v2": r2},
        "metrics": {
            "baseline_metric_lines_added": baseline_metric_lines_added,
            "baseline_qubits_simulated_proxy": float(baseline_metric_lines_added),
            "nqubits_simulated": m2.get("nqubits_simulated", 0.0),
            "nqubits_per_sec": m2.get("nqubits_per_sec", 0.0),
            "nqubit_avg_score": m2.get("nqubit_avg_score"),
            "baseline_qubit_avg_score": m2.get("baseline_qubit_avg_score"),
            "nqubit_win_rate": m2.get("nqubit_win_rate"),
            "ratio_nqubit_vs_baseline_proxy": (m2.get("nqubits_simulated", 0.0) / max(1.0, float(baseline_metric_lines_added))),
        },
        "system": {
            "timestamp_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "platform": platform.platform(),
            "python": platform.python_version(),
            "cpu_count": os.cpu_count(),
            "loadavg": os.getloadavg() if hasattr(os, "getloadavg") else None,
        },
        "entropy": entropy,
        "references_old_reports": load_old_reports_reference(),
        "old_runs_reference": old_runs,
    }

    (out_dir / "comparison_summary_v2.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    md = [
        "# V2 - Comparaison complète baseline C vs NQubit NX copy",
        "",
        f"- run_id: `{run_id}`",
        f"- out_dir: `{str(out_dir.relative_to(ROOT))}`",
        "",
        "## Métriques principales",
        f"- baseline_metric_lines_added: {summary['metrics']['baseline_metric_lines_added']}",
        f"- baseline_qubits_simulated_proxy: {summary['metrics']['baseline_qubits_simulated_proxy']}",
        f"- nqubits_simulated: {summary['metrics']['nqubits_simulated']}",
        f"- nqubits_per_sec: {summary['metrics']['nqubits_per_sec']}",
        f"- nqubit_avg_score: {summary['metrics']['nqubit_avg_score']}",
        f"- baseline_qubit_avg_score: {summary['metrics']['baseline_qubit_avg_score']}",
        f"- nqubit_win_rate: {summary['metrics']['nqubit_win_rate']}",
        f"- ratio_nqubit_vs_baseline_proxy: {summary['metrics']['ratio_nqubit_vs_baseline_proxy']}",
        "",
        "## Références anciens rapports (src/quantum)",
    ]
    for r in summary["references_old_reports"]:
        md.append(f"- {r['file']} (mtime_utc={r['mtime_utc']})")

    (out_dir / "comparison_summary_v2.md").write_text("\n".join(md) + "\n", encoding="utf-8")
    (V2_ROOT / "LATEST_V2.json").write_text(json.dumps({"latest_run": run_id, "summary": str((out_dir / 'comparison_summary_v2.json').relative_to(ROOT))}, indent=2) + "\n", encoding="utf-8")

    print(f"summary_v2={out_dir / 'comparison_summary_v2.json'}")
    print(f"markdown_v2={out_dir / 'comparison_summary_v2.md'}")


if __name__ == "__main__":
    main()
