"""Python conversion of key behaviors from quantum_simulator.c for Kaggle/Replit usage."""
from __future__ import annotations

from dataclasses import dataclass, field
import cmath
import random
import time
from typing import List


@dataclass
class QuantumConfig:
    decoherence_rate: float = 1e-6
    gate_error_rate: float = 0.001
    enable_noise_model: bool = True
    max_entanglement: int = 64
    use_gpu_acceleration: bool = False
    temperature_kelvin: float = 15.0


@dataclass
class QuantumLum:
    x: int
    y: int
    state_count: int
    amplitudes: List[complex] = field(default_factory=list)
    entangled_ids: List[int] = field(default_factory=list)
    coherence_time: float = 1_000_000.0
    fidelity: float = 1.0
    is_measured: bool = False
    created_ns: int = field(default_factory=time.time_ns)

    def __post_init__(self):
        if self.state_count < 2:
            self.state_count = 2
        if self.state_count > 24:
            self.state_count = 24
        if not self.amplitudes:
            self.amplitudes = [0j] * self.state_count
            self.amplitudes[0] = 1 + 0j


def quantum_config_create_default() -> QuantumConfig:
    return QuantumConfig()


def quantum_lum_create(x: int, y: int, initial_states: int) -> QuantumLum:
    return QuantumLum(x=x, y=y, state_count=initial_states)


def quantum_apply_hadamard(lum: QuantumLum, config: QuantumConfig) -> bool:
    if lum.state_count < 2:
        return False
    inv_sqrt2 = 2 ** -0.5
    a0, a1 = lum.amplitudes[0], lum.amplitudes[1]
    lum.amplitudes[0] = (a0 + a1) * inv_sqrt2
    lum.amplitudes[1] = (a0 - a1) * inv_sqrt2
    lum.fidelity *= 1.0 - config.gate_error_rate
    return True


def quantum_apply_pauli_x(lum: QuantumLum, config: QuantumConfig) -> bool:
    if lum.state_count < 2:
        return False
    lum.amplitudes[0], lum.amplitudes[1] = lum.amplitudes[1], lum.amplitudes[0]
    lum.fidelity *= 1.0 - config.gate_error_rate
    return True


def quantum_measure(lum: QuantumLum, _config: QuantumConfig) -> dict:
    probs = [abs(a) ** 2 for a in lum.amplitudes]
    s = sum(probs) or 1.0
    probs = [p / s for p in probs]
    r = random.random()
    c = 0.0
    idx = 0
    for i, p in enumerate(probs):
        c += p
        if r <= c:
            idx = i
            break
    lum.is_measured = True
    return {
        "success": True,
        "state_count": lum.state_count,
        "measured_state": idx,
        "probabilities": probs,
        "fidelity": lum.fidelity,
    }


def smoke_test() -> dict:
    cfg = quantum_config_create_default()
    lum = quantum_lum_create(10, 20, 2)
    ok_h = quantum_apply_hadamard(lum, cfg)
    res = quantum_measure(lum, cfg)
    return {
        "ok_hadamard": ok_h,
        "measure_success": res["success"],
        "state_count": res["state_count"],
    }
