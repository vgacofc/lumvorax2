#!/usr/bin/env python3
import json
import os
import platform
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src" / "quantum"
RESULTS_ROOT = SRC / "results_vkernel_compare"


def run(cmd, cwd=None):
    p = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True, check=False)
    return {"cmd": " ".join(cmd), "rc": p.returncode, "stdout": p.stdout, "stderr": p.stderr}


def read_meminfo():
    out = {}
    p = Path("/proc/meminfo")
    if p.exists():
        for line in p.read_text(encoding="utf-8", errors="ignore").splitlines():
            if ":" in line:
                k, v = line.split(":", 1)
                out[k.strip()] = v.strip()
    return out


def probe_entropy(path, nbytes=4096):
    t0 = time.perf_counter()
    try:
        with open(path, "rb", buffering=0) as f:
            data = f.read(nbytes)
        elapsed = max(time.perf_counter() - t0, 1e-12)
        return {
            "path": path,
            "ok": True,
            "size": len(data),
            "bytes_per_sec": len(data) / elapsed,
            "error": None,
        }
    except Exception as e:
        return {"path": path, "ok": False, "size": 0, "bytes_per_sec": None, "error": str(e)}


def parse_stdout_metrics(stdout):
    metrics = {}
    for line in stdout.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            k = k.strip()
            v = v.strip()
            try:
                metrics[k] = float(v)
            except ValueError:
                metrics[k] = v
    return metrics


def main():
    RESULTS_ROOT.mkdir(parents=True, exist_ok=True)
    run_id = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
    run_dir = RESULTS_ROOT / run_id
    run_dir.mkdir(parents=True, exist_ok=True)

    baseline_bin = SRC / "v_kernel_quantum_bin"
    nx_bin = SRC / "v_kernel_quantum_nx_bin"
    nx_log = run_dir / "nqubit_forensic_ns.jsonl"

    compile_baseline = run(["gcc", str(SRC / "v_kernel_quantum.c"), "-O2", "-o", str(baseline_bin)])
    compile_nx = run(["gcc", str(SRC / "v_kernel_quantum_nx.c"), "-O2", "-lm", "-o", str(nx_bin)])

    baseline_run = run([str(baseline_bin)]) if compile_baseline["rc"] == 0 else {"cmd": str(baseline_bin), "rc": 1, "stdout": "", "stderr": "baseline compile failed"}
    nx_run = run([str(nx_bin), str(nx_log)]) if compile_nx["rc"] == 0 else {"cmd": str(nx_bin), "rc": 1, "stdout": "", "stderr": "nx compile failed"}

    system_metrics = {
        "timestamp_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "platform": platform.platform(),
        "python": platform.python_version(),
        "cpu_count": os.cpu_count(),
        "loadavg": os.getloadavg() if hasattr(os, "getloadavg") else None,
        "meminfo": read_meminfo(),
    }
    entropy = [probe_entropy("/dev/hwrng"), probe_entropy("/dev/random"), probe_entropy("/dev/urandom")]

    baseline_qubits = 4.0  # v_kernel_quantum: 1 run synthétique de 4 métriques quantum-like
    nx_metrics = parse_stdout_metrics(nx_run["stdout"])

    comparison = {
        "run_id": run_id,
        "paths": {
            "run_dir": str(run_dir),
            "nx_log": str(nx_log),
            "legacy_metrics_log": "logs_AIMO3/v46/hardware_metrics.log",
        },
        "commands": {
            "compile_baseline": compile_baseline,
            "compile_nx": compile_nx,
            "run_baseline": baseline_run,
            "run_nx": nx_run,
        },
        "system_metrics": system_metrics,
        "hardware_entropy": entropy,
        "comparison_metrics": {
            "baseline_qubits_simulated": baseline_qubits,
            "nqubits_simulated": nx_metrics.get("nqubits_simulated", 0.0),
            "nqubit_avg_score": nx_metrics.get("nqubit_avg_score"),
            "baseline_qubit_avg_score": nx_metrics.get("baseline_qubit_avg_score"),
            "nqubit_win_rate": nx_metrics.get("nqubit_win_rate"),
            "simulated_scale_ratio_nqubit_vs_qubit": (
                float(nx_metrics.get("nqubits_simulated", 0.0)) / baseline_qubits if baseline_qubits else None
            ),
        },
    }

    (run_dir / "system_metrics.json").write_text(json.dumps(system_metrics, indent=2) + "\n", encoding="utf-8")
    (run_dir / "hardware_entropy_probe.json").write_text(json.dumps(entropy, indent=2) + "\n", encoding="utf-8")
    (run_dir / "comparison_summary.json").write_text(json.dumps(comparison, indent=2) + "\n", encoding="utf-8")

    md = [
        "# Comparaison V-Kernel Quantum (origine) vs V-Kernel NQubit (copy intégrée)",
        "",
        f"- run_id: `{run_id}`",
        f"- run_dir: `{run_dir}`",
        "",
        "## Résumé métriques",
        f"- baseline_qubits_simulated: {comparison['comparison_metrics']['baseline_qubits_simulated']}",
        f"- nqubits_simulated: {comparison['comparison_metrics']['nqubits_simulated']}",
        f"- ratio nqubit/qubit: {comparison['comparison_metrics']['simulated_scale_ratio_nqubit_vs_qubit']}",
        f"- nqubit_avg_score: {comparison['comparison_metrics']['nqubit_avg_score']}",
        f"- baseline_qubit_avg_score: {comparison['comparison_metrics']['baseline_qubit_avg_score']}",
        f"- nqubit_win_rate: {comparison['comparison_metrics']['nqubit_win_rate']}",
        "",
        "## Entropie hardware",
    ]
    for e in entropy:
        md.append(f"- {e['path']}: ok={e['ok']} size={e['size']} bytes_per_sec={e['bytes_per_sec']} error={e['error']}")

    (run_dir / "comparison_summary.md").write_text("\n".join(md) + "\n", encoding="utf-8")

    latest = {
        "latest_run": run_id,
        "latest_summary": str(run_dir / "comparison_summary.json"),
    }
    (RESULTS_ROOT / "LATEST.json").write_text(json.dumps(latest, indent=2) + "\n", encoding="utf-8")

    print(f"comparison_written={run_dir / 'comparison_summary.json'}")
    print(f"markdown_written={run_dir / 'comparison_summary.md'}")


if __name__ == "__main__":
    main()
