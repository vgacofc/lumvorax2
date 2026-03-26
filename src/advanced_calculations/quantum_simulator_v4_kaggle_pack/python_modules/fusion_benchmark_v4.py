"""Python conversion of fusion benchmark from quantum_simulator_fusion_v3.c."""
from __future__ import annotations

import json
import math
import os
import time
from dataclasses import dataclass
from enum import Enum


class RngMode(str, Enum):
    HARDWARE_ONLY = "hardware_only"
    HARDWARE_PREFERRED = "hardware_preferred"
    DETERMINISTIC_SEEDED = "deterministic_seeded"


@dataclass
class FusionSummary:
    scenarios: int
    steps: int
    nqubits_simulated: int
    nqubits_per_sec: float
    nqubit_avg_score: float
    baseline_qubit_avg_score: float
    nqubit_win_rate: float
    nqubit_wins: int
    baseline_wins: int
    elapsed_ns: int
    used_hardware_entropy: bool
    latency_p50_ns: int
    latency_p95_ns: int
    latency_p99_ns: int


def _xorshift64(state: int) -> int:
    state &= 0xFFFFFFFFFFFFFFFF
    state ^= (state << 13) & 0xFFFFFFFFFFFFFFFF
    state ^= (state >> 7) & 0xFFFFFFFFFFFFFFFF
    state ^= (state << 17) & 0xFFFFFFFFFFFFFFFF
    return state & 0xFFFFFFFFFFFFFFFF


def _uniform01(state: int) -> tuple[float, int]:
    state = _xorshift64(state)
    value = (state >> 11) * (1.0 / 9007199254740992.0)
    return value, state


def _gaussian(state: int, sigma: float) -> tuple[float, int]:
    u1, state = _uniform01(state)
    u2, state = _uniform01(state)
    u1 = max(u1, 1e-12)
    radius = math.sqrt(-2.0 * math.log(u1))
    theta = 2.0 * math.pi * u2
    return sigma * radius * math.cos(theta), state


def _seed_from_hardware() -> int | None:
    for p in ("/dev/hwrng", "/dev/random", "/dev/urandom"):
        try:
            with open(p, "rb") as f:
                b = f.read(8)
                if len(b) == 8:
                    s = int.from_bytes(b, "little")
                    if s != 0:
                        return s
        except OSError:
            pass
    return None


def run_forensic_benchmark(output_jsonl: str, scenarios: int = 360, steps: int = 1400,
                          mode: RngMode = RngMode.HARDWARE_PREFERRED,
                          seed: int = 0xC0FFEE) -> FusionSummary:
    used_hw = False
    seed_hw = _seed_from_hardware() if mode in (RngMode.HARDWARE_ONLY, RngMode.HARDWARE_PREFERRED) else None
    if mode == RngMode.HARDWARE_ONLY and seed_hw is None:
        raise RuntimeError("hardware_only requested but no hardware RNG source available")
    if seed_hw is not None:
        seed = seed_hw
        used_hw = True

    rng_nx = seed or 0xC0FFEE12345678
    rng_q = (seed ^ 0xA5A5A5A55A5A5A5A) if seed else 0xABCDEF456789

    t0 = time.time_ns()
    nx_total = 0.0
    q_total = 0.0
    nx_wins = 0
    latencies = []

    os.makedirs(os.path.dirname(output_jsonl), exist_ok=True)
    with open(output_jsonl, "w", encoding="utf-8") as out:
        out.write(json.dumps({
            "event": "run_config",
            "rng_mode": mode.value,
            "used_hardware_entropy": used_hw,
            "scenarios": scenarios,
            "steps": steps,
        }) + "\n")

        for i in range(scenarios):
            st = time.time_ns()
            nx_state = -1.5 + 3.0 * (i / scenarios)
            q_state = nx_state
            target = 0.7 + 0.4 * ((i % 11) / 10.0)
            sigma = 0.02 + 0.001 * (i % 20)
            thermal = 1.0 + 0.02 * (i % 15)
            lyapunov_gain = 0.25

            for _ in range(steps):
                noise_nx, rng_nx = _gaussian(rng_nx, sigma * thermal)
                grad = target - nx_state
                nx_state += noise_nx + lyapunov_gain * math.tanh(grad)

                noise_q, rng_q = _gaussian(rng_q, sigma * 0.7)
                q_state += 0.03 * (target - q_state) + noise_q

            nx_err = abs(target - nx_state)
            q_err = abs(target - q_state)
            nx_score = 1.0 / (1.0 + nx_err)
            q_score = 1.0 / (1.0 + q_err)
            margin = nx_score - q_score
            nx_total += nx_score
            q_total += q_score
            if margin > 0:
                nx_wins += 1

            reason = "none"
            if margin < 0:
                if sigma * thermal > sigma * 0.7 * 1.3:
                    reason = "nx_noise_amplification"
                elif nx_err > q_err * 1.2:
                    reason = "nx_convergence_gap"
                else:
                    reason = "stochastic_flip"

            et = time.time_ns()
            lat = et - st
            latencies.append(lat)
            out.write(json.dumps({
                "ts_ns": et,
                "delta_ns": et - t0,
                "event": "scenario_margin",
                "scenario": i,
                "margin": margin,
                "target": target,
                "sigma": sigma,
                "thermal": thermal,
                "lyapunov_gain": lyapunov_gain,
                "nx_score": nx_score,
                "baseline_score": q_score,
                "nx_error": nx_err,
                "baseline_error": q_err,
                "scenario_latency_ns": lat,
                "failed": margin < 0,
                "fail_reason": reason,
            }) + "\n")

        elapsed = time.time_ns() - t0
        nqubits = scenarios * steps
        rate = nqubits / max(elapsed / 1e9, 1e-12)
        nx_avg = nx_total / scenarios
        q_avg = q_total / scenarios
        wr = nx_wins / scenarios

        l = sorted(latencies)
        p = lambda q: l[min(len(l)-1, int(round(q*(len(l)-1))))] if l else 0
        p50, p95, p99 = p(0.50), p(0.95), p(0.99)

        out.write(json.dumps({
            "event": "summary",
            "nqubits_simulated": nqubits,
            "nqubits_per_sec": rate,
            "elapsed_ns": elapsed,
            "nqubit_avg_score": nx_avg,
            "baseline_qubit_avg_score": q_avg,
            "nqubit_win_rate": wr,
            "nqubit_wins": nx_wins,
            "baseline_wins": scenarios - nx_wins,
            "latency_p50_ns": p50,
            "latency_p95_ns": p95,
            "latency_p99_ns": p99,
        }) + "\n")

    return FusionSummary(
        scenarios=scenarios,
        steps=steps,
        nqubits_simulated=nqubits,
        nqubits_per_sec=rate,
        nqubit_avg_score=nx_avg,
        baseline_qubit_avg_score=q_avg,
        nqubit_win_rate=wr,
        nqubit_wins=nx_wins,
        baseline_wins=scenarios - nx_wins,
        elapsed_ns=elapsed,
        used_hardware_entropy=used_hw,
        latency_p50_ns=p50,
        latency_p95_ns=p95,
        latency_p99_ns=p99,
    )
