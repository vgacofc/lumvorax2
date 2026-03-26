#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
import math
import os
import pathlib
import resource
import time
from datetime import datetime, timezone

C = 299_792_458.0
G = 6.67430e-11


def utc_run_id():
    return datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")


def ns_now():
    return time.time_ns()


def read_proc_kv(path: str):
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


def collect_system_metrics():
    meminfo = read_proc_kv("/proc/meminfo")
    status = read_proc_kv("/proc/self/status")
    cgroup_mem_max = pathlib.Path("/sys/fs/cgroup/memory.max")
    cgroup_mem_current = pathlib.Path("/sys/fs/cgroup/memory.current")
    cgroup_cpu_max = pathlib.Path("/sys/fs/cgroup/cpu.max")

    return {
        "cpu_count_logical": os.cpu_count(),
        "loadavg_1m": os.getloadavg()[0] if hasattr(os, "getloadavg") else None,
        "loadavg_5m": os.getloadavg()[1] if hasattr(os, "getloadavg") else None,
        "loadavg_15m": os.getloadavg()[2] if hasattr(os, "getloadavg") else None,
        "mem_total_mb": round(int(meminfo.get("MemTotal", "0 kB").split()[0]) / 1024.0, 3) if meminfo.get("MemTotal") else None,
        "mem_available_mb": round(int(meminfo.get("MemAvailable", "0 kB").split()[0]) / 1024.0, 3) if meminfo.get("MemAvailable") else None,
        "self_vm_peak_kb": int(status.get("VmPeak", "0 kB").split()[0]) if status.get("VmPeak") else None,
        "self_vm_hwm_kb": int(status.get("VmHWM", "0 kB").split()[0]) if status.get("VmHWM") else None,
        "self_threads": int(status.get("Threads", "0")) if status.get("Threads") else None,
        "cgroup_memory_max": cgroup_mem_max.read_text().strip() if cgroup_mem_max.exists() else None,
        "cgroup_memory_current": cgroup_mem_current.read_text().strip() if cgroup_mem_current.exists() else None,
        "cgroup_cpu_max": cgroup_cpu_max.read_text().strip() if cgroup_cpu_max.exists() else None,
    }


def estimate_negative_energy_joules(radius_m: float, shell_thickness_m: float, v_over_c: float) -> float:
    # Alcubierre-inspired scale proxy (diagnostic only, not full Einstein solution)
    sigma = max(shell_thickness_m, 1e-9)
    return -((C**4) / G) * (v_over_c**2) * (radius_m**2 / sigma)


def default_scenarios(metric_profile: str):
    profiles = {
        "conservative": {"v_over_c": 0.12, "shell_scale": 1.4},
        "balanced": {"v_over_c": 0.25, "shell_scale": 1.0},
        "aggressive": {"v_over_c": 0.45, "shell_scale": 0.7},
    }
    p = profiles.get(metric_profile, profiles["balanced"])
    return [
        {
            "name": f"single_shell_{metric_profile}",
            "model": "gr_1p1d",
            "radius_m": 50.0,
            "shells": [{"radius_m": 50.0, "thickness_m": 2.0 * p["shell_scale"], "amplitude": 1.0}],
            "v_over_c": p["v_over_c"],
        },
        {
            "name": f"multi_shell_{metric_profile}",
            "model": "gr_1p1d",
            "radius_m": 80.0,
            "shells": [
                {"radius_m": 55.0, "thickness_m": 1.8 * p["shell_scale"], "amplitude": 1.0},
                {"radius_m": 80.0, "thickness_m": 1.2 * p["shell_scale"], "amplitude": 1.35},
                {"radius_m": 105.0, "thickness_m": 2.2 * p["shell_scale"], "amplitude": 0.7},
            ],
            "v_over_c": p["v_over_c"] * 1.1,
        },
        {
            "name": f"multi_shell_3p1d_{metric_profile}",
            "model": "gr_3p1d",
            "radius_m": 80.0,
            "shells": [
                {"radius_m": 40.0, "thickness_m": 1.5 * p["shell_scale"], "amplitude": 0.9},
                {"radius_m": 70.0, "thickness_m": 1.0 * p["shell_scale"], "amplitude": 1.2},
            ],
            "v_over_c": p["v_over_c"] * 0.95,
        },
    ]


def shell_source_1d(r, shells):
    s = 0.0
    for sh in shells:
        sigma = max(sh["thickness_m"], 1e-6)
        s += sh["amplitude"] * math.exp(-((r - sh["radius_m"]) ** 2) / (2 * sigma * sigma))
    return s


def run_gr_1p1d(scenario, steps, dt_ns):
    n = 128
    dr = max(scenario["radius_m"] * 3 / n, 1e-3)
    cfl = 0.35
    dt = min(dt_ns * 1e-9, cfl * dr / C)

    phi = [0.0] * n
    phi_prev = [0.0] * n
    traces = []
    prev_hash = "GENESIS"
    t0 = ns_now()

    for k in range(steps):
        t_ns = t0 + k * dt_ns
        new_phi = phi.copy()
        neg_energy_proxy = 0.0
        curvature_max = 0.0

        for i in range(1, n - 1):
            r = i * dr
            src = shell_source_1d(r, scenario["shells"])
            lap = (phi[i + 1] - 2 * phi[i] + phi[i - 1]) / (dr * dr)
            new_phi[i] = 2 * phi[i] - phi_prev[i] + (C * C) * (dt * dt) * (lap - src * 1e-3)
            curvature = abs(lap)
            curvature_max = max(curvature_max, curvature)
            neg_energy_proxy += max(0.0, -new_phi[i])

        phi_prev, phi = phi, new_phi
        v_over_c = scenario["v_over_c"] * (1 + 0.01 * math.sin(k / 9.0))
        e_neg = estimate_negative_energy_joules(scenario["radius_m"], scenario["shells"][0]["thickness_m"], v_over_c) * (1 + neg_energy_proxy * 1e-6)

        row = {
            "scenario": scenario["name"],
            "model": "gr_1p1d",
            "step": k,
            "timestamp_ns": t_ns,
            "grid_points": n,
            "dt_s": dt,
            "dr_m": dr,
            "v_over_c": v_over_c,
            "radius_m": scenario["radius_m"],
            "shell_count": len(scenario["shells"]),
            "curvature_proxy_max": curvature_max,
            "negative_energy_j": e_neg,
            "feasible_now": abs(e_neg) < 1e20,
        }
        row_blob = json.dumps(row, sort_keys=True).encode("utf-8")
        row_hash = hashlib.sha256(prev_hash.encode("utf-8") + row_blob).hexdigest()
        row["prev_hash"] = prev_hash
        row["chain_hash"] = row_hash
        prev_hash = row_hash
        traces.append(row)

    return traces


def run_gr_3p1d(scenario, steps, dt_ns):
    # Coarse 3+1D proxy: scalar metric-potential field over cubic grid
    n = 14
    dx = max(scenario["radius_m"] * 2 / n, 1e-3)
    cfl = 0.2
    dt = min(dt_ns * 1e-9, cfl * dx / C)

    def idx(x, y, z):
        return (x * n + y) * n + z

    size = n * n * n
    phi = [0.0] * size
    phi_prev = [0.0] * size
    traces = []
    prev_hash = "GENESIS"
    t0 = ns_now()

    center = (n - 1) / 2

    for k in range(steps):
        t_ns = t0 + k * dt_ns
        new_phi = phi.copy()
        curvature_max = 0.0
        neg_energy_proxy = 0.0

        for x in range(1, n - 1):
            for y in range(1, n - 1):
                for z in range(1, n - 1):
                    id0 = idx(x, y, z)
                    rx = (x - center) * dx
                    ry = (y - center) * dx
                    rz = (z - center) * dx
                    r = math.sqrt(rx * rx + ry * ry + rz * rz)
                    src = shell_source_1d(r, scenario["shells"])
                    lap = (
                        phi[idx(x + 1, y, z)] + phi[idx(x - 1, y, z)] +
                        phi[idx(x, y + 1, z)] + phi[idx(x, y - 1, z)] +
                        phi[idx(x, y, z + 1)] + phi[idx(x, y, z - 1)] -
                        6 * phi[id0]
                    ) / (dx * dx)
                    new_phi[id0] = 2 * phi[id0] - phi_prev[id0] + (C * C) * (dt * dt) * (lap - src * 1e-3)
                    curvature_max = max(curvature_max, abs(lap))
                    neg_energy_proxy += max(0.0, -new_phi[id0])

        phi_prev, phi = phi, new_phi
        v_over_c = scenario["v_over_c"] * (1 + 0.008 * math.cos(k / 11.0))
        e_neg = estimate_negative_energy_joules(scenario["radius_m"], scenario["shells"][0]["thickness_m"], v_over_c) * (1 + neg_energy_proxy * 1e-7)

        row = {
            "scenario": scenario["name"],
            "model": "gr_3p1d",
            "step": k,
            "timestamp_ns": t_ns,
            "grid_points": size,
            "dt_s": dt,
            "dr_m": dx,
            "v_over_c": v_over_c,
            "radius_m": scenario["radius_m"],
            "shell_count": len(scenario["shells"]),
            "curvature_proxy_max": curvature_max,
            "negative_energy_j": e_neg,
            "feasible_now": abs(e_neg) < 1e20,
        }
        row_blob = json.dumps(row, sort_keys=True).encode("utf-8")
        row_hash = hashlib.sha256(prev_hash.encode("utf-8") + row_blob).hexdigest()
        row["prev_hash"] = prev_hash
        row["chain_hash"] = row_hash
        prev_hash = row_hash
        traces.append(row)

    return traces


def run(args):
    out_dir = pathlib.Path(args.output_dir) / args.run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    scenarios = default_scenarios(args.metric_profile)
    all_rows = []
    wall_start = time.perf_counter()

    for sc in scenarios:
        if sc["model"] == "gr_1p1d":
            all_rows.extend(run_gr_1p1d(sc, args.steps, args.ns_step))
        else:
            all_rows.extend(run_gr_3p1d(sc, args.steps, args.ns_step))

    elapsed = max(time.perf_counter() - wall_start, 1e-9)
    calc_per_s = len(all_rows) / elapsed

    csv_path = out_dir / "warp_feasibility_trace.csv"
    with csv_path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(all_rows[0].keys()))
        w.writeheader()
        w.writerows(all_rows)

    mean_abs_e = sum(abs(r["negative_energy_j"]) for r in all_rows) / len(all_rows)
    max_abs_e = max(abs(r["negative_energy_j"]) for r in all_rows)
    feasible_count = sum(1 for r in all_rows if r["feasible_now"])

    by_model = {}
    for r in all_rows:
        by_model.setdefault(r["model"], {"rows": 0, "feasible": 0, "max_curvature": 0.0})
        by_model[r["model"]]["rows"] += 1
        by_model[r["model"]]["feasible"] += int(r["feasible_now"])
        by_model[r["model"]]["max_curvature"] = max(by_model[r["model"]]["max_curvature"], r["curvature_proxy_max"])

    summary = {
        "run_id": args.run_id,
        "model": "warp_feasibility_proxy_v2_gr1p1d_gr3p1d",
        "metric_profile": args.metric_profile,
        "notes": "Numerical GR-inspired proxy (1+1D and coarse 3+1D), multi-shell scenarios, nanosecond forensic chain logging.",
        "steps_per_scenario": args.steps,
        "scenario_count": len(scenarios),
        "rows_total": len(all_rows),
        "calc_rows_per_second": calc_per_s,
        "mean_abs_negative_energy_j": mean_abs_e,
        "max_abs_negative_energy_j": max_abs_e,
        "feasible_rows": feasible_count,
        "feasible_rate": feasible_count / len(all_rows),
        "forensic_chain_final_hash": all_rows[-1]["chain_hash"],
        "forensic_chain_genesis": "GENESIS",
        "max_memory_mb": round(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0, 3),
        "qubits_simulated": 0,
        "qubits_note": "Warp GR feasibility module is non-quantum-field proxy; qubit count is not applicable.",
        "models_breakdown": by_model,
        "system_metrics": collect_system_metrics(),
        "participating_modules": [
            "run_gr_1p1d",
            "run_gr_3p1d",
            "estimate_negative_energy_joules",
            "collect_system_metrics",
            "hashlib.sha256 forensic chain",
        ],
    }

    (out_dir / "warp_feasibility_summary.json").write_text(json.dumps(summary, indent=2))

    md = [
        "# Warp feasibility forensic summary",
        "",
        f"- run_id: {summary['run_id']}",
        f"- model: {summary['model']}",
        f"- metric_profile: {summary['metric_profile']}",
        f"- scenario_count: {summary['scenario_count']}",
        f"- rows_total: {summary['rows_total']}",
        f"- calc_rows_per_second: {summary['calc_rows_per_second']:.3f}",
        f"- mean_abs_negative_energy_j: {summary['mean_abs_negative_energy_j']:.6e}",
        f"- max_abs_negative_energy_j: {summary['max_abs_negative_energy_j']:.6e}",
        f"- feasible_rate: {summary['feasible_rate']:.6f}",
        f"- forensic_chain_final_hash: {summary['forensic_chain_final_hash']}",
        f"- max_memory_mb: {summary['max_memory_mb']}",
        f"- qubits_simulated: {summary['qubits_simulated']}",
        "",
        "Interpretation:",
        "- If feasible_rate is near 0, current-physics feasibility is not supported by this proxy model.",
        "- 1+1D and 3+1D are included simultaneously to tighten bounds compared to a single-scaling estimate.",
        "- Multi-shell scenarios test whether geometry layering reduces exotic energy demands (it does not here).",
    ]
    (out_dir / "warp_feasibility_summary.md").write_text("\n".join(md) + "\n")

    print(out_dir)


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--run-id", default=utc_run_id())
    p.add_argument("--steps", type=int, default=300)
    p.add_argument("--ns-step", type=int, default=1_000)
    p.add_argument("--metric-profile", choices=["conservative", "balanced", "aggressive"], default="balanced")
    p.add_argument("--output-dir", default="src/advanced_calculations/quantum_simulator_v4_staging_next/results/warp_feasibility")
    run(p.parse_args())
