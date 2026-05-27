#!/usr/bin/env python3
"""LumVorax C91 — Scaling N={8,12,16} GHZ + HVA Hubbard, 1 SEUL job batché.

Objectif : utiliser le quota IBM restant (~9 min) en UNE execution condensee.
- 6 pubs (circuit, observable) batches dans 1 seul estimator.run([...])
- 1 seule Session IBM, 1 seul submit
- Sortie : 1 JSON consolide src/.../bitcoin_quantum_mining/results/ibm_c91_scaling_*.json

Cycle = C91 (suite logique de C90).
"""
import ctypes as _ct, os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p); break
        except: pass

import argparse, datetime, json, math, os, sys, time
from pathlib import Path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_header import (print_header, ok, record, warn, err, info)

import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import (QiskitRuntimeService, EstimatorV2 as Estimator,
                                 EstimatorOptions)

CYCLE = "C91"
STAMP = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
REPO = Path(__file__).resolve().parent.parent
RES_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"
RES_DIR.mkdir(parents=True, exist_ok=True)

_forensic = []
_t0 = time.time()
def _log(tag, msg, val=0.0):
    _forensic.append({"ts": round(time.time()-_t0, 4), "tag": tag,
                      "msg": msg, "val": float(val), "cycle": CYCLE})

def s_pi_observable(N):
    paulis, coeffs = [], []
    for i in range(N):
        for j in range(N):
            if i == j: continue
            label = ["I"] * N
            label[N-1-i] = "Z"
            label[N-1-j] = "Z"
            paulis.append("".join(label))
            coeffs.append(((-1.0)**((i-j) % 2)) / (N * (N-1)))
    return SparsePauliOp(paulis, coeffs=coeffs)

def ghz_circuit(N):
    qc = QuantumCircuit(N, name=f"GHZ-{N}")
    qc.h(0)
    for i in range(N-1):
        qc.cx(i, i+1)
    return qc

def hva_hubbard_1layer(N, t=1.0, U=4.0, beta=0.5):
    qc = QuantumCircuit(N, name=f"HVA-Hubbard-{N}")
    for i in range(N):
        if i % 2 == 1: qc.x(i)
    theta_h = 2 * t * beta
    for i in range(N-1):
        qc.rxx(theta_h, i, i+1)
        qc.ryy(theta_h, i, i+1)
    theta_u = U * beta / 2
    for i in range(0, N-1, 2):
        qc.rzz(theta_u, i, i+1)
    return qc

def s_pi_ghz_theory(N):
    return sum(((-1.0)**((i-j)%2)) for i in range(N) for j in range(N) if i!=j) / (N*(N-1))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--backend", default="ibm_kingston",
                    choices=["ibm_fez","ibm_marrakesh","ibm_kingston"])
    ap.add_argument("--Ns", type=str, default="8,12,16")
    ap.add_argument("--shots", type=int, default=2048)
    ap.add_argument("--resilience", type=int, default=2)
    args = ap.parse_args()

    Ns = [int(x) for x in args.Ns.split(",")]
    v = print_header(cycle=CYCLE, backend=args.backend,
                     extra={"Ns": args.Ns, "shots": args.shots,
                            "resilience": args.resilience})
    _log("start", f"backend={args.backend} Ns={Ns}", 0)

    token = os.environ["IBM_API_KEY"]
    info("token", f"IBM_API_KEY len={len(token)}")
    svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=token)
    backend = svc.backend(args.backend)
    info("backend", f"{args.backend} num_qubits={backend.num_qubits}")
    _log("backend", args.backend, float(backend.num_qubits))

    # --- Construction des 2*len(Ns) pubs : (GHZ-N, S_pi_N) puis (HVA-N, S_pi_N) ---
    pubs = []
    meta = []
    for N in Ns:
        obs = s_pi_observable(N)
        # GHZ
        qc_g = ghz_circuit(N)
        qct_g = transpile(qc_g, backend, optimization_level=3)
        n2q_g = sum(1 for g in qct_g.data
                    if len(g.qubits)==2 and g.operation.name not in ("measure","barrier"))
        info(f"transpile N={N} GHZ", f"depth_phys={qct_g.depth()} 2Q={n2q_g}")
        _log("transpile_ghz", f"N={N} depth={qct_g.depth()} 2Q={n2q_g}", float(qct_g.depth()))
        obs_g_layout = obs.apply_layout(qct_g.layout)
        pubs.append((qct_g, obs_g_layout))
        meta.append({"kind": "ghz", "N": N, "depth_phys": qct_g.depth(), "n2q_phys": n2q_g})
        # HVA
        qc_h = hva_hubbard_1layer(N)
        qct_h = transpile(qc_h, backend, optimization_level=3)
        n2q_h = sum(1 for g in qct_h.data
                    if len(g.qubits)==2 and g.operation.name not in ("measure","barrier"))
        info(f"transpile N={N} HVA", f"depth_phys={qct_h.depth()} 2Q={n2q_h}")
        _log("transpile_hva", f"N={N} depth={qct_h.depth()} 2Q={n2q_h}", float(qct_h.depth()))
        obs_h_layout = obs.apply_layout(qct_h.layout)
        pubs.append((qct_h, obs_h_layout))
        meta.append({"kind": "hva", "N": N, "depth_phys": qct_h.depth(), "n2q_phys": n2q_h})

    info("batch", f"{len(pubs)} pubs prets pour 1 seul estimator.run()")
    _log("batch_ready", f"{len(pubs)} pubs", float(len(pubs)))

    options = EstimatorOptions(resilience_level=args.resilience,
                                default_shots=args.shots)
    estimator = Estimator(mode=backend, options=options)

    info("submit", f"resilience={args.resilience} shots={args.shots}")
    t1 = time.time()
    try:
        job = estimator.run(pubs)
        info("job_id", str(job.job_id()))
        _log("job_id", str(job.job_id()), 0)
        res = job.result()
        dt = time.time() - t1
        record("BATCH OK", f"{len(pubs)} pubs en {dt:.1f}s sur {args.backend}")
    except Exception as e:
        err("batch_run", str(e)[:200])
        dt = time.time() - t1
        out = {"cycle": CYCLE, "stamp": STAMP, "backend": args.backend,
               "Ns": Ns, "shots": args.shots, "resilience": args.resilience,
               "version": v, "error": str(e), "elapsed_s": round(dt,2),
               "forensic": _forensic, "meta": meta}
        out_path = RES_DIR / f"ibm_c91_scaling_{STAMP}_ERR.json"
        out_path.write_text(json.dumps(out, indent=2, default=str))
        ok("saved (error)", str(out_path.relative_to(REPO)))
        sys.exit(3)

    # --- Recuperation des resultats ---
    results = []
    for i, m in enumerate(meta):
        S = float(res[i].data.evs)
        Sstd = float(res[i].data.stds) if hasattr(res[i].data,"stds") else 0.0
        m["S_pi"] = S; m["S_pi_std"] = Sstd
        if m["kind"] == "ghz":
            theory = s_pi_ghz_theory(m["N"])
            m["S_pi_theorie_ideal"] = theory
            ratio = abs(S/theory) if theory != 0 else 0
            m["fidelity_proxy"] = ratio
            tag = "OK" if ratio > 0.5 else "FAIL"
            line = f"GHZ N={m['N']} S={S:+.4f}/{theory:+.4f} ratio={ratio:.2f}"
            if ratio > 0.5: record(f"GHZ-{m['N']} {tag}", line)
            else: err(f"GHZ-{m['N']} {tag}", line)
        else:
            line = f"HVA N={m['N']} S(pi)={S:+.4f}+-{Sstd:.4f} (depth={m['depth_phys']} 2Q={m['n2q_phys']})"
            record(f"HVA-{m['N']}", line)
        _log(f"{m['kind']}_result_N{m['N']}",
             f"S={S:+.4f}+-{Sstd:.4f}", S)
        results.append(m)

    # --- Analyse de scaling AFM ---
    afm_by_N = {m["N"]: m["S_pi"] for m in results if m["kind"]=="hva"}
    fid_by_N = {m["N"]: m.get("fidelity_proxy",0) for m in results if m["kind"]=="ghz"}
    info("scaling AFM",
         " | ".join(f"N={N}: S_HVA={afm_by_N[N]:+.4f} (fid_GHZ={fid_by_N[N]:.2f})"
                    for N in Ns))

    out = {"cycle": CYCLE, "stamp": STAMP, "backend": args.backend,
           "Ns": Ns, "shots": args.shots, "resilience": args.resilience,
           "version": v, "results": results,
           "scaling": {"afm_by_N": afm_by_N, "fid_by_N": fid_by_N},
           "elapsed_total_s": round(time.time()-_t0, 2),
           "elapsed_quantum_s": round(dt, 2),
           "forensic": _forensic}
    out_path = RES_DIR / f"ibm_c91_scaling_{STAMP}.json"
    out_path.write_text(json.dumps(out, indent=2, default=str))
    ok("saved", str(out_path.relative_to(REPO)))
    record("C91 SCALING TERMINE", f"total={time.time()-_t0:.1f}s qpu={dt:.1f}s")

if __name__ == "__main__":
    main()
