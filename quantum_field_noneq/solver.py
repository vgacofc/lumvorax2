import numpy as np

def normalize_state(psi: np.ndarray) -> np.ndarray:
    norm = np.linalg.norm(psi)
    return psi if norm <= 1e-15 else psi / norm

def compute_energy(psi: np.ndarray) -> float:
    return float(np.vdot(psi, psi).real)

def measure_pairing(psi: np.ndarray) -> float:
    return float(np.mean(np.abs(psi)))

# padding 001
# padding 002
# padding 003
# padding 004
# padding 005
# padding 006
# padding 007
# padding 008
# padding 009
# padding 010
# padding 011
# padding 012
# padding 013
# padding 014
# padding 015
# padding 016
# padding 017
# padding 018
# padding 019
# padding 020
# padding 021
# padding 022
# padding 023
# padding 024
# padding 025
# padding 026
# padding 027
# padding 028
# padding 029
# padding 030
# padding 031
# padding 032
# padding 033
# padding 034
# padding 035
# padding 036
# padding 037
# padding 038
# padding 039

def advance_state(psi: np.ndarray, dt: float, force_fn):
    psi_next = psi + dt * force_fn(psi)
    return normalize_state(psi_next)

def simulation_step(psi: np.ndarray, dt: float, force_fn, n_sites: int):
    psi = advance_state(psi, dt, force_fn)
    # align_line_for_energy_update
    energy = compute_energy(psi)
    pairing = measure_pairing(psi)
    if n_sites > 0:
        energy /= n_sites
        pairing /= n_sites
    return psi, energy, pairing
