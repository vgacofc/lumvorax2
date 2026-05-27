#!/usr/bin/env python3
"""dmrg_trotter1_compare.py — Compare IBM C97 vs DMRG/Aer noiseless Trotter-1.

Cycle C99 — Réponse à la critique experte P1.1 :
    « Test DMRG Trotter-1 critique : si DMRG < IBM, le bruit IBM AJOUTE du
      signal AFM artificiellement → résultat non publiable. »

Reproduit EXACTEMENT le circuit Trotter-1 du runner C97 (ansatz="rxx" brickwork
2 layers, theta=0.30, init Néel) et calcule S(π)/N noiseless via Statevector
(N≤16) ou MPS bond=64 (N≥24).

Usage :
    python3 tools/dmrg_trotter1_compare.py --N_list 8,12,16,24,32 [--out F.json]

Comparaison à effectuer ensuite :
    delta(N) = S(π)/N_IBM_C97 - S(π)/N_aer_noiseless
    Cas 1 : delta ≈ 0    → IBM reproduit la vérité, signal physiquement réel ✅
    Cas 2 : delta > +5%  → bruit IBM ajoute du signal AFM, suspect ⚠️
    Cas 3 : delta < -5%  → bruit IBM dégrade S(π), augmenter resilience C100
"""
import argparse, json, sys, time

import numpy as np
from qiskit import QuantumCircuit
from qiskit.quantum_info import Statevector, Pauli


def build_trotter1_circuit(N: int, theta: float = 0.30, n_layers: int = 2) -> QuantumCircuit:
    """Trotter-1 brickwork : init Néel + n_layers de (RXX(theta) RZZ(theta))."""
    qc = QuantumCircuit(N)
    # Init Néel |1010...⟩
    for j in range(N):
        if j % 2 == 1:
            qc.x(j)
    # Brickwork RXX RZZ
    for d in range(n_layers):
        for i in range(0, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
        for i in range(1, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
    return qc


def compute_S_pi_statevector(qc: QuantumCircuit, N: int) -> float:
    sv = Statevector.from_instruction(qc)
    S_pi = 0.0
    for k in range(N):
        zk = Pauli("I" * (N - 1 - k) + "Z" + "I" * k)
        ev = float(np.real(sv.expectation_value(zk)))
        S_pi += ((-1.0) ** k) * ev
    return S_pi


def compute_S_pi_mps(qc: QuantumCircuit, N: int, bond: int = 64) -> float:
    """Aer MPS pour N grand."""
    from qiskit_aer import AerSimulator
    from qiskit.quantum_info import SparsePauliOp
    sim = AerSimulator(method="matrix_product_state",
                       matrix_product_state_max_bond_dimension=bond)
    qc2 = qc.copy()
    qc2.save_statevector()
    res = sim.run(qc2).result()
    sv = res.get_statevector()
    S_pi = 0.0
    for k in range(N):
        zk = Pauli("I" * (N - 1 - k) + "Z" + "I" * k)
        ev = float(np.real(sv.expectation_value(zk)))
        S_pi += ((-1.0) ** k) * ev
    return S_pi


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--N_list", default="8,12,16",
                   help="liste de tailles, séparées par virgule")
    p.add_argument("--theta", type=float, default=0.30)
    p.add_argument("--n_layers", type=int, default=2)
    p.add_argument("--mps_threshold", type=int, default=20,
                   help="N > seuil → MPS, sinon statevector")
    p.add_argument("--out", default=None)
    args = p.parse_args()

    N_list = [int(x) for x in args.N_list.split(",") if x.strip()]
    rows = []
    for N in N_list:
        t0 = time.time()
        qc = build_trotter1_circuit(N, args.theta, args.n_layers)
        if N <= args.mps_threshold:
            method = "statevector"
            S_pi = compute_S_pi_statevector(qc, N)
        else:
            method = "mps"
            S_pi = compute_S_pi_mps(qc, N)
        dt = time.time() - t0
        S_per_N = S_pi / N
        print(f"N={N:3d}  method={method:12s}  depth={qc.depth():3d}  "
              f"S(pi)={S_pi:+.4f}  S(pi)/N={S_per_N:+.4f}  ({dt:.2f}s)")
        rows.append({
            "N": N, "method": method, "depth": qc.depth(),
            "S_pi": float(S_pi), "S_pi_per_N": float(S_per_N),
            "elapsed_s": float(dt),
        })
    if args.out:
        with open(args.out, "w") as fp:
            json.dump({"theta": args.theta, "n_layers": args.n_layers,
                       "results": rows}, fp, indent=2)
        print(f"→ écrit {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
