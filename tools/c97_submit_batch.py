#!/usr/bin/env python3
"""C97.1 — Submit batch IBM multi-N Hubbard 1D AFM ansatz Heisenberg.
N in [16,24,32,48,64,96,128,156]. 1 seul submit Estimator V2.
6 PUBs par N : S(pi), S(0), S(pi/2), E_total, C(r=1), C(r=4).
Output: /tmp/c97_batch_jobid.json + stdout."""
import os, json, sys, time, math
from datetime import datetime, timezone

NS = [16, 24, 32, 48, 64, 96, 128, 156]
LAYERS_1D = 1     # 1 trotter step pour rester dans le quota
SHOTS = 4096
RESILIENCE = 1    # ZNE+TREX léger
OPTIMIZATION = 3
DYN_DECOUPLING = True

def build_hubbard_afm_circuit(N, layers=1):
    """Heisenberg-like ansatz : initial Néel + RZZ+RXX brickwork chain."""
    from qiskit import QuantumCircuit
    qc = QuantumCircuit(N)
    # Néel init
    for i in range(0, N, 2):
        qc.x(i)
    # 1 trotter step : RXX+RZZ pairs (even then odd)
    theta = 0.30
    for layer in range(layers):
        for i in range(0, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
        for i in range(1, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
    return qc

def build_observables(N):
    """6 SparsePauliOp : S(pi), S(0), S(pi/2), E, C(r=1), C(r=4)."""
    from qiskit.quantum_info import SparsePauliOp
    obs = []
    # S(k) = sum_{i,j} cos(k*(i-j)) * Z_i Z_j / N  (structure factor)
    for k_label, k_val in [("S_pi", math.pi), ("S_0", 0.0), ("S_pi2", math.pi / 2)]:
        terms = []
        coeffs = []
        for i in range(N):
            for j in range(N):
                if i == j:
                    continue
                p = ["I"] * N
                p[i] = "Z"
                p[j] = "Z"
                terms.append("".join(reversed(p)))
                coeffs.append(math.cos(k_val * (i - j)) / N)
        obs.append(SparsePauliOp(terms, coeffs=coeffs))
    # E_total = sum_i Z_i Z_{i+1}  (proxy energie AFM)
    terms = []
    coeffs = []
    for i in range(N - 1):
        p = ["I"] * N
        p[i] = "Z"
        p[i + 1] = "Z"
        terms.append("".join(reversed(p)))
        coeffs.append(-1.0)
    obs.append(SparsePauliOp(terms, coeffs=coeffs))
    # C(r=1) = (1/(N-1)) sum_i Z_i Z_{i+1}
    terms = []
    coeffs = []
    for i in range(N - 1):
        p = ["I"] * N
        p[i] = "Z"
        p[i + 1] = "Z"
        terms.append("".join(reversed(p)))
        coeffs.append(1.0 / (N - 1))
    obs.append(SparsePauliOp(terms, coeffs=coeffs))
    # C(r=4) = (1/(N-4)) sum_i Z_i Z_{i+4}
    if N > 4:
        terms = []
        coeffs = []
        for i in range(N - 4):
            p = ["I"] * N
            p[i] = "Z"
            p[i + 4] = "Z"
            terms.append("".join(reversed(p)))
            coeffs.append(1.0 / (N - 4))
        obs.append(SparsePauliOp(terms, coeffs=coeffs))
    else:
        obs.append(SparsePauliOp(["I" * N], coeffs=[0.0]))
    return obs

def main():
    print(f"[C97.1] start {datetime.now(timezone.utc).isoformat()}", flush=True)
    from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2
    from qiskit.transpiler.preset_passmanagers import generate_preset_pass_manager

    token = os.environ["IBM_API_KEY"]
    service = QiskitRuntimeService(channel="ibm_quantum_platform", token=token)
    backend = service.backend("ibm_kingston")
    print(f"[C97.1] backend={backend.name}  num_qubits={backend.num_qubits}  queue={backend.status().pending_jobs}", flush=True)

    pm = generate_preset_pass_manager(optimization_level=OPTIMIZATION, backend=backend)

    pubs = []
    for N in NS:
        if N > backend.num_qubits:
            print(f"[C97.1] SKIP N={N} > {backend.num_qubits}", flush=True)
            continue
        print(f"[C97.1] build N={N} ...", flush=True)
        qc = build_hubbard_afm_circuit(N, layers=LAYERS_1D)
        obs = build_observables(N)
        qc_t = pm.run(qc)
        obs_t = [o.apply_layout(qc_t.layout) for o in obs]
        pubs.append((qc_t, obs_t))
        print(f"[C97.1]   N={N}  depth={qc_t.depth()}  ops_2q={sum(1 for g in qc_t.data if g.operation.num_qubits==2)}  observables={len(obs_t)}", flush=True)

    print(f"[C97.1] submit batch  total_pubs={len(pubs)} ...", flush=True)
    estimator = EstimatorV2(mode=backend)
    estimator.options.default_shots = SHOTS
    estimator.options.resilience_level = RESILIENCE
    estimator.options.dynamical_decoupling.enable = DYN_DECOUPLING
    estimator.options.dynamical_decoupling.sequence_type = "XY4"
    job = estimator.run(pubs)
    job_id = job.job_id()
    print(f"[C97.1] JOB_ID={job_id}", flush=True)
    print(f"[C97.1] status_initial={job.status()}", flush=True)

    out = {
        "job_id": job_id,
        "backend": backend.name,
        "ns": NS,
        "n_pubs": len(pubs),
        "shots": SHOTS,
        "resilience": RESILIENCE,
        "submitted_at": datetime.now(timezone.utc).isoformat(),
        "cycle": "C97.1"
    }
    with open("/tmp/c97_batch_jobid.json", "w") as f:
        json.dump(out, f, indent=2)
    print(f"[C97.1] saved /tmp/c97_batch_jobid.json", flush=True)
    print(json.dumps(out, indent=2))

if __name__ == "__main__":
    main()
