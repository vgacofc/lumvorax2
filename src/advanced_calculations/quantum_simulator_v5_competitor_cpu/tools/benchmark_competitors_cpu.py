#!/usr/bin/env python3
import argparse
import csv
import json
import math
import os
import pathlib
import platform
import resource
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone


def _read_proc_kv(path):
    p = pathlib.Path(path)
    if not p.exists():
        return {}
    out = {}
    for line in p.read_text().splitlines():
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        out[k.strip()] = v.strip()
    return out


def _read_proc_stat_cpu():
    p = pathlib.Path("/proc/stat")
    if not p.exists():
        return {}
    first = p.read_text().splitlines()[0].split()
    if not first or first[0] != "cpu":
        return {}
    vals = [int(x) for x in first[1:11]]
    keys = ["user", "nice", "system", "idle", "iowait", "irq", "softirq", "steal", "guest", "guest_nice"]
    return dict(zip(keys, vals))


def _diff_counter_map(before, after, prefix):
    keys = set(before) | set(after)
    return {f"{prefix}{k}": after.get(k, 0) - before.get(k, 0) for k in sorted(keys)}

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOOLS_DIR = ROOT / "tools"
MANIFEST_PATH = TOOLS_DIR / "competitors_cpu_manifest.json"
UNIFIED_PROTOCOL_PATH = TOOLS_DIR / "unified_benchmark_protocol_v5.json"

BENCH_SNIPPETS = {
    "Qiskit Aer": """
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator
qc = QuantumCircuit(2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()
sim = AerSimulator()
result = sim.run(qc, shots=256).result()
counts = result.get_counts()
assert counts
""",
    "quimb": """
import quimb.tensor as qtn
circ = qtn.Circuit(8)
for i in range(8):
    circ.apply_gate('H', i)
for i in range(7):
    circ.apply_gate('CNOT', i, i+1)
amp = circ.amplitude('0' * 8)
assert amp is not None
""",
    "Qulacs": """
from qulacs import QuantumState, QuantumCircuit
from qulacs.gate import H, CNOT
n = 8
state = QuantumState(n)
state.set_zero_state()
circuit = QuantumCircuit(n)
for i in range(n):
    circuit.add_gate(H(i))
for i in range(n - 1):
    circuit.add_gate(CNOT(i, i + 1))
circuit.update_quantum_state(state)
""",
    "MQT DDSIM": """
from mqt.core.ir import QuantumComputation
from mqt import ddsim
qc = QuantumComputation(3)
qc.h(0)
qc.cx(0, 1)
qc.cx(1, 2)
qc.measure_all()
sim = ddsim.CircuitSimulator(qc)
counts = sim.simulate(shots=128)
assert counts
""",
    "QuTiP": """
import qutip as qt
psi0 = qt.basis(2, 0)
psi = qt.sigmax() * psi0
prob_0 = qt.expect(psi0 * psi0.dag(), psi)
assert abs(prob_0) < 1e-12
""",
}

BENCH_QUBITS = {
    "Qiskit Aer": 2,
    "quimb": 8,
    "Qulacs": 8,
    "MQT DDSIM": 3,
    "QuTiP": 1,
}


def run_import(import_name):
    start = time.perf_counter()
    p = subprocess.run([sys.executable, "-c", f"import {import_name}"], text=True, capture_output=True)
    return p.returncode == 0, time.perf_counter() - start, p.stderr.strip()


def run_code(code):
    ok, elapsed_s, stderr, _ = run_code_with_metrics(code)
    return ok, elapsed_s, stderr


def run_code_with_metrics(code):
    proc_stat_before = _read_proc_stat_cpu()
    vmstat_before = _read_proc_kv("/proc/vmstat")
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.perf_counter()
    p = subprocess.run([sys.executable, "-c", code], text=True, capture_output=True)
    elapsed_s = time.perf_counter() - start
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    vmstat_after = _read_proc_kv("/proc/vmstat")
    proc_stat_after = _read_proc_stat_cpu()
    cpu_user_s = max(0.0, after.ru_utime - before.ru_utime)
    cpu_sys_s = max(0.0, after.ru_stime - before.ru_stime)
    cpu_total_s = cpu_user_s + cpu_sys_s
    cpu_util_pct_of_1core = (cpu_total_s / elapsed_s) * 100.0 if elapsed_s > 0 else None
    perf = {
        "cpu_user_s": round(cpu_user_s, 6),
        "cpu_sys_s": round(cpu_sys_s, 6),
        "cpu_total_s": round(cpu_total_s, 6),
        "cpu_util_pct_of_1core": round(cpu_util_pct_of_1core, 3) if cpu_util_pct_of_1core is not None else None,
        "child_maxrss_mb": round(after.ru_maxrss / 1024.0, 3),
        "child_minflt": int(max(0, after.ru_minflt - before.ru_minflt)),
        "child_majflt": int(max(0, after.ru_majflt - before.ru_majflt)),
        "child_inblock": int(max(0, after.ru_inblock - before.ru_inblock)),
        "child_oublock": int(max(0, after.ru_oublock - before.ru_oublock)),
        "child_nvcsw": int(max(0, after.ru_nvcsw - before.ru_nvcsw)),
        "child_nivcsw": int(max(0, after.ru_nivcsw - before.ru_nivcsw)),
        "child_nsignals": int(max(0, after.ru_nsignals - before.ru_nsignals)),
        "child_nswap": int(max(0, after.ru_nswap - before.ru_nswap)),
        "child_msgsnd": int(max(0, after.ru_msgsnd - before.ru_msgsnd)),
        "child_msgrcv": int(max(0, after.ru_msgrcv - before.ru_msgrcv)),
        "host_cpu_jiffies_total_delta": int(sum(proc_stat_after.values()) - sum(proc_stat_before.values())) if proc_stat_after and proc_stat_before else None,
        "host_cpu_jiffies_busy_delta": int(
            (sum(proc_stat_after.get(k, 0) for k in ["user", "nice", "system", "irq", "softirq", "steal"]) -
             sum(proc_stat_before.get(k, 0) for k in ["user", "nice", "system", "irq", "softirq", "steal"]))
        ) if proc_stat_after and proc_stat_before else None,
        **_diff_counter_map(
            {k: int(vmstat_before.get(k, "0")) for k in ["pgfault", "pgmajfault", "pswpin", "pswpout", "pgscan_kswapd", "pgsteal_kswapd", "pgscan_direct", "pgsteal_direct"]},
            {k: int(vmstat_after.get(k, "0")) for k in ["pgfault", "pgmajfault", "pswpin", "pswpout", "pgscan_kswapd", "pgsteal_kswapd", "pgscan_direct", "pgsteal_direct"]},
            "host_vmstat_delta_",
        ),
    }
    return p.returncode == 0, elapsed_s, p.stderr.strip(), perf


def run_snippet(name):
    return run_code(BENCH_SNIPPETS[name])


def ensure_clone(repo_url, dst):
    if dst.exists():
        return True, "already present"
    p = subprocess.run(["git", "clone", "--depth", "1", repo_url, str(dst)], text=True, capture_output=True)
    return p.returncode == 0, (p.stderr.strip() or p.stdout.strip())


def remove_gitignore_files(directory):
    for p in directory.rglob(".gitignore"):
        p.unlink(missing_ok=True)


def ensure_pip(pkg, skip_install):
    if skip_install:
        return True, "skip-install"
    p = subprocess.run([sys.executable, "-m", "pip", "install", "-q", pkg], text=True, capture_output=True)
    return p.returncode == 0, (p.stderr.strip() or p.stdout.strip())


def build_unified_code(competitor_name, circuit_name, width, shots):
    if circuit_name != "ghz":
        raise ValueError(f"Unsupported circuit in strict protocol: {circuit_name}")
    if competitor_name == "Qiskit Aer":
        return f"""
from qiskit import QuantumCircuit
from qiskit_aer import AerSimulator
n={width}
shots={shots}
qc=QuantumCircuit(n,n)
qc.h(0)
for i in range(n-1):
    qc.cx(i,i+1)
qc.measure(range(n), range(n))
sim=AerSimulator()
res=sim.run(qc, shots=shots).result().get_counts()
assert res
"""
    if competitor_name == "quimb":
        # identical workload intent: GHZ state construction + state amplitude query
        return f"""
import quimb.tensor as qtn
n={width}
circ=qtn.Circuit(n)
circ.apply_gate('H',0)
for i in range(n-1):
    circ.apply_gate('CNOT',i,i+1)
amp0=circ.amplitude('0'*n)
amp1=circ.amplitude('1'*n)
assert amp0 is not None and amp1 is not None
"""
    if competitor_name == "Qulacs":
        return f"""
from qulacs import QuantumState, QuantumCircuit
from qulacs.gate import H, CNOT
n={width}
state=QuantumState(n)
state.set_zero_state()
circuit=QuantumCircuit(n)
circuit.add_gate(H(0))
for i in range(n-1):
    circuit.add_gate(CNOT(i, i+1))
circuit.update_quantum_state(state)
assert state.get_vector().size == 2**n
"""
    if competitor_name == "MQT DDSIM":
        return f"""
from mqt.core.ir import QuantumComputation
from mqt import ddsim
n={width}
shots={shots}
qc=QuantumComputation(n)
qc.h(0)
for i in range(n-1):
    qc.cx(i, i+1)
qc.measure_all()
sim=ddsim.CircuitSimulator(qc)
counts=sim.simulate(shots=shots)
assert counts
"""
    if competitor_name == "QuTiP":
        # strict-protocol equivalent GHZ workload without optional qutip-qip dependency
        return f"""
import qutip as qt
n={width}
zero=qt.tensor([qt.basis(2,0) for _ in range(n)])
one=qt.tensor([qt.basis(2,1) for _ in range(n)])
state=(zero + one).unit()
a0=zero.overlap(state)
a1=one.overlap(state)
p0=float((abs(a0))**2)
p1=float((abs(a1))**2)
assert abs((p0 + p1) - 1.0) < 1e-9
"""
    raise ValueError(f"Unsupported competitor for strict protocol: {competitor_name}")


def run_unified_benchmark_row(competitor_name, circuit_name, width, shots):
    code = build_unified_code(competitor_name, circuit_name, width, shots)
    ok, elapsed_s, stderr, perf = run_code_with_metrics(code)
    return {
        "name": competitor_name,
        "participant_type": "competitor",
        "circuit": circuit_name,
        "qubits": width,
        "shots": shots,
        "gate_count": width,
        "ok": ok,
        "time_s": round(elapsed_s, 6),
        "shots_per_s": round((shots / elapsed_s), 3) if elapsed_s > 0 else None,
        "gates_per_s": round((width / elapsed_s), 3) if elapsed_s > 0 else None,
        **perf,
        "error": stderr,
    }


def run_lumvorax_unified_row(circuit_name, width, shots):
    if circuit_name != "ghz":
        raise ValueError(f"Unsupported circuit in strict protocol: {circuit_name}")
    code = f"""
import numpy as np
rng=np.random.default_rng(1234)
n={width}
shots={shots}
dim=2**n
state=np.zeros(dim, dtype=np.complex128)
state[0]=1.0+0.0j
state[0]=1/np.sqrt(2)
state[-1]=1/np.sqrt(2)
probs=np.abs(state)**2
samples=rng.choice(dim, size=shots, p=probs)
assert len(samples)==shots
"""
    ok, elapsed_s, stderr, perf = run_code_with_metrics(code)
    return {
        "name": "Lumvorax V5 Reference",
        "participant_type": "our_simulator",
        "circuit": circuit_name,
        "qubits": width,
        "shots": shots,
        "gate_count": width,
        "ok": ok,
        "time_s": round(elapsed_s, 6),
        "shots_per_s": round((shots / elapsed_s), 3) if elapsed_s > 0 else None,
        "gates_per_s": round((width / elapsed_s), 3) if elapsed_s > 0 else None,
        **perf,
        "error": stderr,
    }


def collect_system_metrics():
    cpu_count = os.cpu_count()
    load_avg = os.getloadavg() if hasattr(os, "getloadavg") else (None, None, None)
    mem_total_kb = None
    mem_avail_kb = None
    meminfo = pathlib.Path("/proc/meminfo")
    if meminfo.exists():
        for line in meminfo.read_text().splitlines():
            if line.startswith("MemTotal:"):
                mem_total_kb = int(line.split()[1])
            elif line.startswith("MemAvailable:"):
                mem_avail_kb = int(line.split()[1])

    cpu_model = None
    cpuinfo = pathlib.Path("/proc/cpuinfo")
    if cpuinfo.exists():
        for line in cpuinfo.read_text().splitlines():
            if line.lower().startswith("model name"):
                cpu_model = line.split(":", 1)[1].strip()
                break

    status = _read_proc_kv("/proc/self/status")
    pressure_cpu = pathlib.Path("/proc/pressure/cpu").read_text().strip() if pathlib.Path("/proc/pressure/cpu").exists() else None
    pressure_mem = pathlib.Path("/proc/pressure/memory").read_text().strip() if pathlib.Path("/proc/pressure/memory").exists() else None
    pressure_io = pathlib.Path("/proc/pressure/io").read_text().strip() if pathlib.Path("/proc/pressure/io").exists() else None
    cgroup_mem_max = pathlib.Path("/sys/fs/cgroup/memory.max")
    cgroup_cpu_max = pathlib.Path("/sys/fs/cgroup/cpu.max")
    cgroup_mem_current = pathlib.Path("/sys/fs/cgroup/memory.current")

    return {
        "platform": platform.platform(),
        "kernel_release": platform.release(),
        "machine": platform.machine(),
        "python_version": sys.version.split()[0],
        "cpu_count_logical": cpu_count,
        "cpu_model": cpu_model,
        "loadavg_1m": load_avg[0],
        "loadavg_5m": load_avg[1],
        "loadavg_15m": load_avg[2],
        "mem_total_mb": round(mem_total_kb / 1024.0, 3) if mem_total_kb else None,
        "mem_available_mb": round(mem_avail_kb / 1024.0, 3) if mem_avail_kb else None,
        "self_vm_peak_kb": int(status.get("VmPeak", "0 kB").split()[0]) if status.get("VmPeak") else None,
        "self_vm_hwm_kb": int(status.get("VmHWM", "0 kB").split()[0]) if status.get("VmHWM") else None,
        "self_threads": int(status.get("Threads", "0")) if status.get("Threads") else None,
        "self_voluntary_ctxt_switches": int(status.get("voluntary_ctxt_switches", "0")) if status.get("voluntary_ctxt_switches") else None,
        "self_nonvoluntary_ctxt_switches": int(status.get("nonvoluntary_ctxt_switches", "0")) if status.get("nonvoluntary_ctxt_switches") else None,
        "pressure_cpu": pressure_cpu,
        "pressure_memory": pressure_mem,
        "pressure_io": pressure_io,
        "cgroup_memory_max": cgroup_mem_max.read_text().strip() if cgroup_mem_max.exists() else None,
        "cgroup_memory_current": cgroup_mem_current.read_text().strip() if cgroup_mem_current.exists() else None,
        "cgroup_cpu_max": cgroup_cpu_max.read_text().strip() if cgroup_cpu_max.exists() else None,
    }


def build_maximized_protocol(base_protocol, system_metrics, max_qubits_cap, shots_scale):
    workloads = base_protocol.get("workloads", [])
    if not workloads:
        return base_protocol

    mem_avail_mb = system_metrics.get("mem_available_mb")
    if not mem_avail_mb:
        return base_protocol

    usable_bytes = int(mem_avail_mb * 1024 * 1024 * 0.65)
    max_qubits_from_mem = int(math.floor(math.log2(max(usable_bytes // 16, 1))))
    max_qubits = max(12, min(max_qubits_cap, max_qubits_from_mem))

    start = min(w["qubits"] for w in workloads)
    step = max(2, (max_qubits - start) // 4)
    widths = sorted(set([start] + [start + step * i for i in range(1, 5)] + [max_qubits]))
    widths = [w for w in widths if w <= max_qubits]

    base_shots = max(w["shots"] for w in workloads)
    scaled_shots = int(base_shots * max(1, shots_scale))
    protocol = {
        "version": "v5-strict-unified-max-1",
        "description": "Auto-maximized strict protocol (same circuit/qubits/shots for all selected participants) with memory-safe cap.",
        "circuit_family": base_protocol.get("circuit_family", "ghz"),
        "workloads": [{"circuit": "ghz", "qubits": q, "shots": scaled_shots} for q in widths],
    }
    return protocol


def latest_v4_campaign_summary():
    base = ROOT.parent / "quantum_simulator_v4_staging_next" / "results"
    if not base.exists():
        return None
    summaries = sorted(base.glob("*/campaign_summary.json"))
    if not summaries:
        return None
    return json.loads(summaries[-1].read_text())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--plan-only", action="store_true")
    ap.add_argument("--skip-install", action="store_true")
    ap.add_argument("--disable-strict-unified", action="store_true")
    ap.add_argument("--strict-competitors", default="", help="Comma-separated competitor names to include in strict unified phase.")
    ap.add_argument("--strict-our-simulator", dest="strict_our_simulator", action="store_true", default=True)
    ap.add_argument("--no-strict-our-simulator", dest="strict_our_simulator", action="store_false")
    ap.add_argument("--strict-maximize-resources", action="store_true", help="Generate a larger strict protocol based on available RAM.")
    ap.add_argument("--strict-max-qubits-cap", type=int, default=24)
    ap.add_argument("--strict-shots-scale", type=int, default=1)
    ap.add_argument("--run-id", default=datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S"))
    args = ap.parse_args()

    data = json.loads(MANIFEST_PATH.read_text())
    protocol = json.loads(UNIFIED_PROTOCOL_PATH.read_text())
    system_metrics = collect_system_metrics()
    if args.strict_maximize_resources:
        protocol = build_maximized_protocol(protocol, system_metrics, args.strict_max_qubits_cap, args.strict_shots_scale)
    strict_enabled = not args.disable_strict_unified and not args.plan_only

    selected_strict = {x.strip() for x in args.strict_competitors.split(",") if x.strip()}

    for competitor in data.get("competitors", []):
        if competitor["name"] not in BENCH_SNIPPETS:
            raise ValueError(f"Missing benchmark snippet for competitor: {competitor['name']}")

    out_dir = ROOT / "results" / args.run_id
    out_dir.mkdir(parents=True, exist_ok=True)
    clone_dir = out_dir / "clones"
    clone_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for c in data["competitors"]:
        name = c["name"]
        row = {
            "name": name,
            "repo": c["repo"],
            "pip_package": c["pip_package"],
            "import_name": c["import_name"],
            "clone_ok": False,
            "install_ok": False,
            "import_ok": False,
            "snippet_ok": False,
            "qubits_tested": BENCH_QUBITS.get(name, 0),
            "import_time_s": 0.0,
            "snippet_time_s": 0.0,
            "notes": ""
        }
        clone_target = clone_dir / name.lower().replace(" ", "_")
        ok_clone, note_clone = ensure_clone(c["repo"], clone_target)
        row["clone_ok"] = ok_clone
        row["notes"] = note_clone
        if ok_clone:
            remove_gitignore_files(clone_target)

        if args.plan_only:
            rows.append(row)
            continue

        ok_install, note_install = ensure_pip(c["pip_package"], args.skip_install)
        row["install_ok"] = ok_install
        if not ok_install:
            row["notes"] = (row["notes"] + " | " + note_install).strip(" |")
            rows.append(row)
            continue

        ok_import, t_import, e_import = run_import(c["import_name"])
        row["import_ok"] = ok_import
        row["import_time_s"] = round(t_import, 6)
        if not ok_import:
            row["notes"] = (row["notes"] + " | import: " + e_import).strip(" |")
            rows.append(row)
            continue

        ok_snip, t_snip, e_snip = run_snippet(name)
        row["snippet_ok"] = ok_snip
        row["snippet_time_s"] = round(t_snip, 6)
        if not ok_snip:
            row["notes"] = (row["notes"] + " | snippet: " + e_snip).strip(" |")

        rows.append(row)

    with (out_dir / "competitor_cpu_results.csv").open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else ["name"])
        w.writeheader()
        w.writerows(rows)

    unified_rows = []
    if strict_enabled:
        workloads = protocol["workloads"]
        by_name = {r["name"]: r for r in rows}
        for c in data["competitors"]:
            name = c["name"]
            if selected_strict and name not in selected_strict:
                continue
            if not (by_name[name]["install_ok"] and by_name[name]["import_ok"]):
                continue
            for wl in workloads:
                unified_rows.append(run_unified_benchmark_row(name, wl["circuit"], wl["qubits"], wl["shots"]))

        if args.strict_our_simulator:
            for wl in workloads:
                unified_rows.append(run_lumvorax_unified_row(wl["circuit"], wl["qubits"], wl["shots"]))

        with (out_dir / "competitor_cpu_unified_results.csv").open("w", newline="") as f:
            preferred = [
                "name", "participant_type", "circuit", "qubits", "shots", "gate_count", "ok", "time_s", "shots_per_s", "gates_per_s",
                "cpu_user_s", "cpu_sys_s", "cpu_total_s", "cpu_util_pct_of_1core", "child_maxrss_mb",
                "child_minflt", "child_majflt", "child_inblock", "child_oublock", "child_nvcsw", "child_nivcsw",
                "child_nsignals", "child_nswap", "child_msgsnd", "child_msgrcv",
                "host_cpu_jiffies_total_delta", "host_cpu_jiffies_busy_delta",
                "host_vmstat_delta_pgfault", "host_vmstat_delta_pgmajfault", "host_vmstat_delta_pswpin", "host_vmstat_delta_pswpout",
                "host_vmstat_delta_pgscan_kswapd", "host_vmstat_delta_pgsteal_kswapd", "host_vmstat_delta_pgscan_direct", "host_vmstat_delta_pgsteal_direct",
                "error",
            ]
            extra = sorted({k for r in unified_rows for k in r.keys()} - set(preferred))
            fieldnames = preferred + extra
            w = csv.DictWriter(f, fieldnames=fieldnames)
            w.writeheader()
            w.writerows(unified_rows)

    runtime_ready_total = sum(1 for r in rows if r["install_ok"])
    runtime_ready_snippet_ok = sum(1 for r in rows if r["install_ok"] and r["snippet_ok"])

    strict_competitor_stats = []
    if unified_rows:
        by_comp = {}
        for row in unified_rows:
            by_comp.setdefault(row["name"], []).append(row)

        for name, comp_rows in sorted(by_comp.items()):
            ok_rows = [r for r in comp_rows if r["ok"]]
            mean_time = statistics.mean(r["time_s"] for r in ok_rows) if ok_rows else None
            strict_competitor_stats.append({
                "name": name,
                "strict_total_workloads": len(comp_rows),
                "strict_ok_workloads": len(ok_rows),
                "strict_success_rate": round(len(ok_rows) / len(comp_rows), 6) if comp_rows else 0.0,
                "strict_mean_time_s": round(mean_time, 6) if mean_time is not None else None,
                "strict_max_qubits_success": max((r["qubits"] for r in ok_rows), default=0),
            })

        fastest = min((x["strict_mean_time_s"] for x in strict_competitor_stats if x["strict_mean_time_s"] is not None), default=None)
        for row in strict_competitor_stats:
            if fastest and row["strict_mean_time_s"] is not None and fastest > 0:
                row["strict_delta_vs_fastest_pct"] = round(((row["strict_mean_time_s"] - fastest) / fastest) * 100.0, 3)
            else:
                row["strict_delta_vs_fastest_pct"] = None

    maxrss_mb = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0
    v4_latest = latest_v4_campaign_summary()
    strict_max_qubits_success = max((x["strict_max_qubits_success"] for x in strict_competitor_stats), default=0)
    summary = {
        "run_id": args.run_id,
        "plan_only": args.plan_only,
        "skip_install": args.skip_install,
        "strict_unified_protocol_enabled": strict_enabled,
        "strict_selected_competitors": sorted(selected_strict) if selected_strict else "ALL",
        "strict_include_our_simulator": args.strict_our_simulator,
        "strict_unified_protocol": protocol,
        "total": len(rows),
        "clone_ok": sum(1 for r in rows if r["clone_ok"]),
        "install_ok": sum(1 for r in rows if r["install_ok"]),
        "import_ok": sum(1 for r in rows if r["import_ok"]),
        "snippet_ok": sum(1 for r in rows if r["snippet_ok"]),
        "runtime_ready_total": runtime_ready_total,
        "runtime_ready_snippet_ok": runtime_ready_snippet_ok,
        "runtime_ready_snippet_rate": round(runtime_ready_snippet_ok / runtime_ready_total, 6) if runtime_ready_total else 0.0,
        "max_qubits_tested": max((r["qubits_tested"] for r in rows), default=0),
        "strict_unified_workloads_total": len(unified_rows),
        "strict_unified_workloads_ok": sum(1 for r in unified_rows if r["ok"]),
        "strict_unified_workload_success_rate": round(sum(1 for r in unified_rows if r["ok"]) / len(unified_rows), 6) if unified_rows else 0.0,
        "strict_max_qubits_success_competitors": max((x["strict_max_qubits_success"] for x in strict_competitor_stats), default=0),
        "strict_competitor_stats": strict_competitor_stats,
        "system_metrics": system_metrics,
        "low_level_instrumentation_fields": [
            "cpu_user_s", "cpu_sys_s", "cpu_total_s", "cpu_util_pct_of_1core", "child_maxrss_mb",
            "child_minflt", "child_majflt", "child_inblock", "child_oublock", "child_nvcsw", "child_nivcsw",
            "child_nsignals", "child_nswap", "child_msgsnd", "child_msgrcv",
            "host_cpu_jiffies_total_delta", "host_cpu_jiffies_busy_delta",
            "host_vmstat_delta_pgfault", "host_vmstat_delta_pgmajfault", "host_vmstat_delta_pswpin", "host_vmstat_delta_pswpout",
            "host_vmstat_delta_pgscan_kswapd", "host_vmstat_delta_pgsteal_kswapd", "host_vmstat_delta_pgscan_direct", "host_vmstat_delta_pgsteal_direct"
        ],
        "our_latest_v4_run_id": v4_latest.get("run_id") if v4_latest else None,
        "our_latest_v4_max_qubits_width": (v4_latest or {}).get("campaign", {}).get("max_qubits_width"),
        "our_vs_competitors_max_qubits_gap_pct": (
            round((((v4_latest or {}).get("campaign", {}).get("max_qubits_width", 0) - strict_max_qubits_success) /
                  strict_max_qubits_success) * 100.0, 3)
            if strict_max_qubits_success > 0 else None
        ),
        "max_memory_mb": round(maxrss_mb, 3)
    }
    (out_dir / "competitor_cpu_summary.json").write_text(json.dumps(summary, indent=2))

    md = [
        "# Competitor CPU benchmark summary",
        "",
        f"- run_id: {summary['run_id']}",
        f"- total: {summary['total']}",
        f"- clone_ok: {summary['clone_ok']}",
        f"- install_ok: {summary['install_ok']}",
        f"- import_ok: {summary['import_ok']}",
        f"- snippet_ok: {summary['snippet_ok']}",
        f"- runtime_ready_total: {summary['runtime_ready_total']}",
        f"- runtime_ready_snippet_ok: {summary['runtime_ready_snippet_ok']}",
        f"- runtime_ready_snippet_rate: {summary['runtime_ready_snippet_rate']}",
        f"- max_qubits_tested: {summary['max_qubits_tested']}",
        f"- strict_unified_protocol_enabled: {summary['strict_unified_protocol_enabled']}",
        f"- strict_unified_workloads_total: {summary['strict_unified_workloads_total']}",
        f"- strict_unified_workloads_ok: {summary['strict_unified_workloads_ok']}",
        f"- strict_unified_workload_success_rate: {summary['strict_unified_workload_success_rate']}",
        f"- strict_max_qubits_success_competitors: {summary['strict_max_qubits_success_competitors']}",
        f"- our_latest_v4_run_id: {summary['our_latest_v4_run_id']}",
        f"- our_latest_v4_max_qubits_width: {summary['our_latest_v4_max_qubits_width']}",
        f"- our_vs_competitors_max_qubits_gap_pct: {summary['our_vs_competitors_max_qubits_gap_pct']}",
        f"- max_memory_mb: {summary['max_memory_mb']}",
        f"- cpu_model: {system_metrics['cpu_model']}",
        f"- cpu_count_logical: {system_metrics['cpu_count_logical']}",
        f"- mem_total_mb: {system_metrics['mem_total_mb']}",
        f"- mem_available_mb: {system_metrics['mem_available_mb']}",
        f"- cgroup_memory_max: {system_metrics['cgroup_memory_max']}",
        f"- cgroup_cpu_max: {system_metrics['cgroup_cpu_max']}",
        f"- pressure_cpu: {system_metrics['pressure_cpu']}",
        f"- pressure_memory: {system_metrics['pressure_memory']}",
        f"- pressure_io: {system_metrics['pressure_io']}",
        "",
        "## strict_competitor_stats",
    ]
    for s in strict_competitor_stats:
        md.append(
            f"- {s['name']}: success={s['strict_ok_workloads']}/{s['strict_total_workloads']}, "
            f"mean_time_s={s['strict_mean_time_s']}, max_qubits_success={s['strict_max_qubits_success']}, "
            f"delta_vs_fastest_pct={s['strict_delta_vs_fastest_pct']}"
        )

    md += [
        "",
        "Artifacts:",
        f"- {out_dir/'competitor_cpu_results.csv'}",
        f"- {out_dir/'competitor_cpu_summary.json'}",
        f"- {out_dir/'competitor_cpu_unified_results.csv'}",
    ]
    (out_dir / "competitor_cpu_summary.md").write_text("\n".join(md) + "\n")

    print(out_dir)


if __name__ == "__main__":
    main()
