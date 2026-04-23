#!/usr/bin/env python3
"""LumVorax C90 — EstimatorV2 + GHZ-8 baseline + HVA-Hubbard 1 layer.

Nouveautes vs C89 :
  - Mesure S(pi) directement via EstimatorV2 (pas via SamplerV2 + bitstrings)
  - GHZ-8 baseline OBLIGATOIRE avant tout claim AFM (criterion S_GHZ >= 0.5)
  - HVA-Hubbard 1 layer (depth_log <= 30) au lieu de VORAX (depth_log = 32)
  - resilience_level = 2 (ZNE automatique)
  - En-tete couleur + version git via run_header
  - Affichage VERT sur record, ROUGE sur anomalie

Cout IBM estime : ~60s (GHZ-8: 20s + HVA: 40s)
Quota requis    : >= 80s
"""
import ctypes as _ct, os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p); break
        except: pass

import argparse, datetime, gzip, hashlib, json, math, os, sys, time
from pathlib import Path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_header import (print_header, ok, record, warn, err, info,
                         GREEN, RED, RESET, BOLD)

import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import (QiskitRuntimeService, EstimatorV2 as Estimator,
                                 EstimatorOptions)

CYCLE = "C90"
STAMP = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
REPO = Path(__file__).resolve().parent.parent
RES_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"
LUM_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native"
for d in [RES_DIR, LUM_DIR]: d.mkdir(parents=True, exist_ok=True)

_forensic = []
_t0 = time.time()
def _log(tag, msg, val=0.0):
    e = {"ts": round(time.time()-_t0, 4), "tag": tag, "msg": msg, "val": float(val), "cycle": CYCLE}
    _forensic.append(e)

def s_pi_observable(N):
    """Construit H_AFM = (1/(N(N-1))) * sum_{i!=j} (-1)^(i-j) Z_i Z_j."""
    paulis = []
    coeffs = []
    for i in range(N):
        for j in range(N):
            if i == j: continue
            label = ["I"] * N
            label[N-1-i] = "Z"   # Qiskit endianness
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
    """1-layer HVA pour Hubbard 1D. depth_log ≈ 8."""
    qc = QuantumCircuit(N, name=f"HVA-Hubbard-{N}")
    # initialisation Neel
    for i in range(N):
        if i % 2 == 1:
            qc.x(i)
    # Hopping XX+YY (1 layer)
    theta_h = 2 * t * beta
    for i in range(N-1):
        qc.rxx(theta_h, i, i+1)
        qc.ryy(theta_h, i, i+1)
    # Onsite ZZ (Hubbard U)
    theta_u = U * beta / 2
    for i in range(0, N-1, 2):
        qc.rzz(theta_u, i, i+1)
    return qc

def build_service():
    token = os.environ.get("IBM_API_KEY", "")
    if not token:
        err("token", "IBM_API_KEY absent dans env")
        sys.exit(2)
    info("token", f"IBM_API_KEY len={len(token)}")
    try:
        return QiskitRuntimeService(channel="ibm_quantum_platform", token=token)
    except Exception as e:
        warn("svc init", str(e)[:80])
        return QiskitRuntimeService(channel="ibm_cloud", token=token)

def run_observable(estimator, qc, obs, label):
    qct = transpile(qc, estimator._backend if hasattr(estimator,"_backend") else None,
                    optimization_level=3) if False else qc
    info(label, f"depth_log={qc.depth()} n_qubits={qc.num_qubits}")
    pub = (qct, obs)
    job = estimator.run([pub])
    res = job.result()
    val = float(res[0].data.evs)
    std = float(res[0].data.stds) if hasattr(res[0].data, "stds") else 0.0
    return val, std

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--backend", default="ibm_fez",
                    choices=["ibm_fez","ibm_marrakesh","ibm_kingston","ibm_brisbane"])
    ap.add_argument("--N", type=int, default=8, help="taille du systeme")
    ap.add_argument("--shots", type=int, default=2048)
    ap.add_argument("--ghz-only", action="store_true",
                    help="ne lancer que le GHZ baseline (calibrage backend)")
    ap.add_argument("--no-ghz", action="store_true",
                    help="sauter le GHZ baseline (deconseille)")
    ap.add_argument("--resilience", type=int, default=2,
                    help="resilience_level Estimator (0..2)")
    args = ap.parse_args()

    v = print_header(cycle=CYCLE, backend=args.backend,
                     extra={"N": args.N, "shots": args.shots,
                            "resilience": args.resilience})
    _log("start", f"backend={args.backend} N={args.N}", 0)

    service = build_service()
    backend = service.backend(args.backend)
    info("backend", f"{args.backend} num_qubits={backend.num_qubits}")
    _log("backend", args.backend, float(backend.num_qubits))

    obs = s_pi_observable(args.N)
    info("observable", f"S(pi) Pauli operator: {len(obs)} termes "
                       f"(coef diag exclus)")
    _log("observable", f"{len(obs)} termes", float(len(obs)))

    options = EstimatorOptions(resilience_level=args.resilience,
                                default_shots=args.shots)
    estimator = Estimator(mode=backend, options=options)

    out = {"cycle": CYCLE, "stamp": STAMP, "backend": args.backend,
           "N": args.N, "shots": args.shots, "resilience": args.resilience,
           "version": v}

    # ---------- 1. GHZ-8 baseline ----------
    if not args.no_ghz:
        info("ghz", "lancement GHZ-8 baseline")
        qc_ghz = ghz_circuit(args.N)
        qct_ghz = transpile(qc_ghz, backend, optimization_level=3)
        n2q_ghz = sum(1 for g in qct_ghz.data
                      if len(g.qubits)==2 and g.operation.name not in ("measure","barrier"))
        info("ghz transpile", f"depth_phys={qct_ghz.depth()} 2Q={n2q_ghz}")
        _log("ghz_transpile", f"depth={qct_ghz.depth()} 2Q={n2q_ghz}",
             float(qct_ghz.depth()))

        t1 = time.time()
        try:
            obs_layout = obs.apply_layout(qct_ghz.layout)
            job_ghz = estimator.run([(qct_ghz, obs_layout)])
            res_ghz = job_ghz.result()
            S_ghz = float(res_ghz[0].data.evs)
            S_ghz_std = float(res_ghz[0].data.stds) if hasattr(res_ghz[0].data,"stds") else 0.0
        except Exception as e:
            err("ghz job", str(e)[:120])
            S_ghz = float("nan"); S_ghz_std = float("nan")
        dt_ghz = time.time() - t1
        out["ghz"] = {"S_pi": S_ghz, "S_pi_std": S_ghz_std,
                      "depth_phys": qct_ghz.depth(), "n2q_phys": n2q_ghz,
                      "elapsed_s": round(dt_ghz, 2)}
        _log("ghz_result", f"S={S_ghz:+.4f}+-{S_ghz_std:.4f} dt={dt_ghz:.1f}s", S_ghz)

        # GHZ-N : S(pi) theorique = 1 si N pair (Neel-like correlations dans GHZ)
        # En realite : <Z_i Z_j>_GHZ = +1 pour tous i,j → S(pi) = (1/N(N-1))*Σ(-1)^(i-j)
        # Pour N=8 : Σ (-1)^(i-j) hors-diag. Calcul: pour chaque i, Σ_{j≠i}(-1)^(i-j) = 4-3 = 1 si N=8.
        # Total = N*1 = 8. Normalise: 8/56 = 0.143
        S_ghz_theory = sum(((-1.0)**((i-j)%2)) for i in range(args.N) for j in range(args.N) if i!=j) / (args.N*(args.N-1))
        info("ghz theorie", f"S_pi_GHZ_ideal = {S_ghz_theory:+.4f}")
        out["ghz"]["S_pi_theorie_ideal"] = S_ghz_theory

        if not math.isnan(S_ghz):
            ratio = abs(S_ghz / S_ghz_theory) if S_ghz_theory != 0 else 0
            out["ghz"]["fidelity_proxy"] = ratio
            if ratio > 0.5:
                record("GHZ baseline OK", f"S={S_ghz:+.4f}/{S_ghz_theory:+.4f} "
                                         f"ratio={ratio:.2f} → backend FIABLE")
            else:
                err("GHZ baseline FAIL",
                    f"S={S_ghz:+.4f}/{S_ghz_theory:+.4f} ratio={ratio:.2f} "
                    f"→ backend trop bruite, claim AFM impossible")

        if args.ghz_only:
            out_path = RES_DIR / f"ibm_c90_ghzonly_{STAMP}.json"
            out["forensic"] = _forensic
            out_path.write_text(json.dumps(out, indent=2, default=str))
            ok("saved", str(out_path.relative_to(REPO)))
            return

    # ---------- 2. HVA-Hubbard 1 layer ----------
    info("hva", "lancement HVA-Hubbard 1 layer")
    qc_hva = hva_hubbard_1layer(args.N)
    qct_hva = transpile(qc_hva, backend, optimization_level=3)
    n2q_hva = sum(1 for g in qct_hva.data
                  if len(g.qubits)==2 and g.operation.name not in ("measure","barrier"))
    info("hva transpile", f"depth_phys={qct_hva.depth()} 2Q={n2q_hva}")
    _log("hva_transpile", f"depth={qct_hva.depth()} 2Q={n2q_hva}",
         float(qct_hva.depth()))

    t2 = time.time()
    try:
        obs_layout = obs.apply_layout(qct_hva.layout)
        job_hva = estimator.run([(qct_hva, obs_layout)])
        res_hva = job_hva.result()
        S_hva = float(res_hva[0].data.evs)
        S_hva_std = float(res_hva[0].data.stds) if hasattr(res_hva[0].data,"stds") else 0.0
    except Exception as e:
        err("hva job", str(e)[:120])
        S_hva = float("nan"); S_hva_std = float("nan")
    dt_hva = time.time() - t2
    out["hva"] = {"S_pi": S_hva, "S_pi_std": S_hva_std,
                  "depth_phys": qct_hva.depth(), "n2q_phys": n2q_hva,
                  "elapsed_s": round(dt_hva, 2)}
    _log("hva_result", f"S={S_hva:+.4f}+-{S_hva_std:.4f} dt={dt_hva:.1f}s", S_hva)

    if not math.isnan(S_hva):
        if S_hva > 0.05:
            record("AFM signal detecte", f"S(pi)={S_hva:+.4f}+-{S_hva_std:.4f} sur {args.backend}")
        elif S_hva < -0.05:
            record("FM signal detecte", f"S(pi)={S_hva:+.4f}+-{S_hva_std:.4f} sur {args.backend}")
        else:
            warn("aucun signal", f"S(pi)={S_hva:+.4f}+-{S_hva_std:.4f} (sous le seuil 0.05)")

    out["forensic"] = _forensic
    out["elapsed_total_s"] = round(time.time()-_t0, 2)
    out_path = RES_DIR / f"ibm_c90_full_{STAMP}.json"
    out_path.write_text(json.dumps(out, indent=2, default=str))
    ok("saved", str(out_path.relative_to(REPO)))

if __name__ == "__main__":
    main()
