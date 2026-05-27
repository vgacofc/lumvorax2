#!/usr/bin/env python3
"""adapt_vqe_p02_batch.py — C99 P0.2 ADAPT-VQE batch multi-N sur IBM Quantum réel.

Cible : tracer la scaling curve ADAPT-VQE pour N=4,6,8,10,12,16,20,24
Backend : ibm_torino (vgaccodex01) / ibm_brisbane / ibm_kyoto selon disponibilité.
Quota visé : ~90 secondes total (instances vgaccodex01 fraîches).

Sortie : ibm_data/c105_adapt_vqe_p02_batch.json
   {
     "backend": str,
     "runs": [
        {"N": 4,  "iterations": int, "operators_added": int,
         "energy_final": float, "exact_diag": float,
         "delta_E": float, "wall_time_s": float, "ibm_seconds": float},
        ...
     ],
     "total_ibm_seconds": float
   }

ATTENTION : à exécuter sur Ubuntu (Replit Qiskit cassé numpy/Py3.13 + Cloudflare 1010 bloque IBM).
   ssh lvx@<ubuntu> 'cd LVX/lumvorax2 && python3 tools/adapt_vqe_p02_batch.py --N 4 6 8 10 12 16 20 24'
"""
import argparse, json, os, sys, time, math
from pathlib import Path

def build_hubbard_hamiltonian(N: int, U: float = 4.0, t: float = 1.0):
    """Construit l'hamiltonien Hubbard 1D de N sites en SparsePauliOp."""
    from qiskit.quantum_info import SparsePauliOp
    n_qubits = 2 * N  # spin up + down
    paulis, coeffs = [], []
    # Hopping terms : -t * (c†_i,σ c_{i+1},σ + h.c.) -> Jordan-Wigner
    for i in range(N - 1):
        for spin in (0, N):  # 0=up, N=down (offset)
            q1, q2 = i + spin, i + 1 + spin
            # JW : c†c + cc† devient mix XX+YY avec string Z
            zstr = ['I'] * n_qubits
            for k in range(q1 + 1, q2):
                zstr[k] = 'Z'
            for op in ('X', 'Y'):
                p = list(zstr)
                p[q1] = op; p[q2] = op
                paulis.append(''.join(reversed(p)))
                coeffs.append(-t / 2)
    # Coulomb on-site : U * n_i,up * n_i,down
    for i in range(N):
        qu, qd = i, i + N
        # n = (I - Z)/2 → n_up * n_dn = (I - Z_u - Z_d + Z_u Z_d)/4
        for op_str, c in (('I' * n_qubits, U / 4),):
            paulis.append(op_str); coeffs.append(c)
        for q in (qu, qd):
            p = ['I'] * n_qubits; p[q] = 'Z'
            paulis.append(''.join(reversed(p))); coeffs.append(-U / 4)
        p = ['I'] * n_qubits; p[qu] = 'Z'; p[qd] = 'Z'
        paulis.append(''.join(reversed(p))); coeffs.append(U / 4)
    return SparsePauliOp(paulis, coeffs).simplify()


def adapt_vqe_run(N: int, backend_name: str, max_iter: int = 8,
                  shots: int = 4096, U: float = 4.0):
    """Exécute ADAPT-VQE sur backend IBM réel ou Aer si IBM_API_KEY absent."""
    from qiskit import QuantumCircuit, transpile
    from qiskit.circuit import Parameter
    from qiskit.quantum_info import SparsePauliOp, Statevector
    from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2

    H = build_hubbard_hamiltonian(N, U=U)
    n_qubits = 2 * N

    # Pool d'opérateurs : single+double excitations Jordan-Wigner
    pool = []
    for i in range(N):
        for j in range(i + 1, N):
            for s in (0, N):
                # excitation simple JW
                p = ['I'] * n_qubits; p[i + s] = 'Y'; p[j + s] = 'X'
                pool.append(SparsePauliOp([''.join(reversed(p))], [1j]))
                p = ['I'] * n_qubits; p[i + s] = 'X'; p[j + s] = 'Y'
                pool.append(SparsePauliOp([''.join(reversed(p))], [-1j]))

    # Initial state HF |10..10..> (N/2 électrons par spin)
    qc = QuantumCircuit(n_qubits)
    for i in range(N // 2):
        qc.x(i); qc.x(i + N)

    selected_ops, selected_thetas = [], []
    energies, wall_t0 = [], time.time()
    ibm_seconds = 0.0

    if backend_name.startswith("aer") or not os.environ.get("IBM_API_KEY"):
        from qiskit_aer import AerSimulator
        backend = AerSimulator(method="statevector")
        actual_backend = "aer_statevector"
        estimator = None
    else:
        svc = QiskitRuntimeService(channel="ibm_quantum_platform",
                                   token=os.environ["IBM_API_KEY"])
        backend = svc.backend(backend_name)
        actual_backend = backend.name
        estimator = EstimatorV2(mode=backend)

    def measure_energy(circ):
        nonlocal ibm_seconds
        if estimator is None:
            sv = Statevector.from_instruction(circ)
            return float(sv.expectation_value(H).real)
        t0 = time.time()
        job = estimator.run([(circ, H)])
        res = job.result()
        ibm_seconds += (time.time() - t0)
        return float(res[0].data.evs)

    e_curr = measure_energy(qc)
    energies.append(e_curr)

    for it in range(max_iter):
        # Sélection : opérateur avec plus grand gradient |⟨[H,A]⟩|
        best_grad, best_op = -1.0, None
        for op in pool:
            commutator = (H @ op - op @ H).simplify()
            if estimator is None:
                sv = Statevector.from_instruction(qc)
                g = abs(complex(sv.expectation_value(commutator)).real)
            else:
                g = abs(complex(measure_energy_op(qc, commutator, estimator)).real)
            if g > best_grad:
                best_grad, best_op = g, op
        if best_grad < 1e-4:
            break
        selected_ops.append(best_op)
        selected_thetas.append(0.0)
        # Optimisation locale du dernier theta (line search rapide)
        from scipy.optimize import minimize_scalar
        def cost(theta_new):
            tc = QuantumCircuit(n_qubits)
            for i in range(N // 2): tc.x(i); tc.x(i + N)
            for op_, th_ in zip(selected_ops[:-1], selected_thetas[:-1]):
                tc.append(op_to_evolution_gate(op_, th_), range(n_qubits))
            tc.append(op_to_evolution_gate(selected_ops[-1], theta_new), range(n_qubits))
            return measure_energy(tc)
        res_opt = minimize_scalar(cost, bracket=(-0.5, 0.5), method='brent',
                                  options={'maxiter': 6, 'xtol': 1e-3})
        selected_thetas[-1] = float(res_opt.x)
        # Reconstruit circuit
        qc = QuantumCircuit(n_qubits)
        for i in range(N // 2): qc.x(i); qc.x(i + N)
        for op_, th_ in zip(selected_ops, selected_thetas):
            qc.append(op_to_evolution_gate(op_, th_), range(n_qubits))
        e_curr = measure_energy(qc)
        energies.append(e_curr)

    # Énergie exacte (diag exacte pour N petit)
    exact = float('nan')
    if N <= 6:
        try:
            exact = min(SparsePauliOp_eigvals(H))
        except Exception:
            pass

    return {
        "N": N,
        "iterations": len(selected_ops),
        "operators_added": len(selected_ops),
        "energy_final": float(energies[-1]),
        "energy_history": [float(e) for e in energies],
        "exact_diag": exact,
        "delta_E": float(energies[-1] - exact) if not math.isnan(exact) else None,
        "wall_time_s": time.time() - wall_t0,
        "ibm_seconds": ibm_seconds,
        "backend": actual_backend,
    }


def op_to_evolution_gate(op, theta):
    """Convertit un opérateur Pauli en porte exp(-i*theta*op)."""
    from qiskit.circuit.library import PauliEvolutionGate
    return PauliEvolutionGate(op, time=theta)


def measure_energy_op(circ, op, estimator):
    job = estimator.run([(circ, op)])
    return job.result()[0].data.evs


def SparsePauliOp_eigvals(H):
    return H.to_matrix().diagonal().real


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--N", nargs="+", type=int, default=[4, 6, 8, 10, 12, 16, 20, 24])
    p.add_argument("--backend", default="ibm_torino",
                   help="ibm_torino|ibm_brisbane|aer_statevector")
    p.add_argument("--max-iter", type=int, default=6)
    p.add_argument("--out", default="ibm_data/c105_adapt_vqe_p02_batch.json")
    args = p.parse_args()

    out_dir = Path(args.out).parent
    out_dir.mkdir(parents=True, exist_ok=True)

    results = {"backend": args.backend, "runs": [],
               "started_iso": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
               "max_iter": args.max_iter, "total_ibm_seconds": 0.0}
    for N in args.N:
        print(f"[ADAPT-VQE] N={N} démarrage...", flush=True)
        try:
            r = adapt_vqe_run(N, args.backend, max_iter=args.max_iter)
            results["runs"].append(r)
            results["total_ibm_seconds"] += r["ibm_seconds"]
            print(f"  ✓ N={N} E={r['energy_final']:.6f} ΔE={r['delta_E']} "
                  f"iters={r['iterations']} ibm={r['ibm_seconds']:.1f}s", flush=True)
        except Exception as e:
            print(f"  ✗ N={N} FAIL: {type(e).__name__}: {e}", flush=True)
            results["runs"].append({"N": N, "error": str(e)})

    results["finished_iso"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    Path(args.out).write_text(json.dumps(results, indent=2))
    print(f"\n[OK] Sauvé {args.out} — total IBM = {results['total_ibm_seconds']:.1f} s")


if __name__ == "__main__":
    sys.exit(main())
