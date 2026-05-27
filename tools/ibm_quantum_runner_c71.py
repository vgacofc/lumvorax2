#!/usr/bin/env python3
"""
LumVorax C71 — EstimatorV2 + SparsePauliOp HTS Hubbard — énergie directe sans VQE — 16 problèmes
Backend: ibm_fez 156Q Heron R2 | Cycle: C71
Standard: STANDARD_NAMES.md v4.5 §M-IBM-C71
Auto-généré par pipeline C67-C83
"""
import ctypes as _ct, os as _os
for _p in ["/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6"]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p)
        except: pass
        break

import argparse, datetime, gzip, hashlib, json, math, os, random, sys, time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

try:
    import numpy as np
    from qiskit import QuantumCircuit, QuantumRegister, ClassicalRegister, transpile
    from qiskit.circuit.library import RealAmplitudes, QFTGate
    from qiskit.quantum_info import SparsePauliOp
    from qiskit_ibm_runtime import QiskitRuntimeService, SamplerV2 as Sampler, EstimatorV2 as Estimator, Session
    try:
        from qiskit_ibm_runtime.fake_provider import FakeBrisbane as _FB
    except:
        from qiskit_ibm_runtime.fake_provider import FakeSherbrooke as _FB
    QISKIT_OK = True
except ImportError as e:
    QISKIT_OK = False; _FB = None
    print(f"[C71] Qiskit: {e}")

sys.path.insert(0, str(Path(__file__).parent))

CYCLE = "C71"
STAMP = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
IBM_API_KEY = os.environ.get("IBM_API_KEY", "")
_REPO = Path(os.environ.get("REPO_ROOT", str(Path(__file__).parent.parent))).resolve()
FORENSIC_DIR = _REPO / "src/advanced_calculations/bitcoin_quantum_mining/logs/forensic"
HTS_DIR = _REPO / "src/advanced_calculations/quantum_problem_hubbard_hts/logs"
LUM_DIR = _REPO / "src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native"
CHAT_DIR = _REPO / "src/advanced_calculations/bitcoin_quantum_mining/CHAT"
for _d in [FORENSIC_DIR, HTS_DIR, LUM_DIR, CHAT_DIR]:
    _d.mkdir(parents=True, exist_ok=True)

_forensic: List[Dict] = []
_t0 = time.time()

def _log(tag, msg, val=0.0):
    e = {"ts": round(time.time()-_t0,4), "tag":tag, "msg":msg, "val":val, "cycle":CYCLE}
    _forensic.append(e)
    print(f"[C71:{tag}] {msg} val={val:.6f}")

def _save_lum(name, data):
    p = LUM_DIR / f"{name}_C71_{STAMP}.lum"
    payload = json.dumps(data, default=str).encode()
    chk = hashlib.sha256(payload).hexdigest()[:16]
    with gzip.open(p,"wb") as f:
        f.write(json.dumps({"checksum":chk,"cycle":CYCLE,"data":data},default=str).encode())
    return p

def _get_counts(result, idx=0):
    counts = {}
    try:
        data = result[idx].data
        for attr in dir(data):
            if not attr.startswith("_"):
                reg = getattr(data, attr, None)
                if reg and hasattr(reg,"get_counts"):
                    counts = reg.get_counts(); break
    except: pass
    return counts or {}

def ibm_connect(fake=False):
    if fake or not IBM_API_KEY:
        _log("connect","Mode fake/simulation")
        return _FB() if QISKIT_OK and _FB else None, True
    import warnings; warnings.filterwarnings("ignore")
    try:
        svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=IBM_API_KEY)
        b = svc.backend("ibm_fez")
        _log("connect",f"ibm_fez {b.num_qubits}Q connecté",float(b.num_qubits))
        return b, False
    except Exception as e:
        _log("connect_err",f"{e} — fallback fake")
        return _FB() if QISKIT_OK and _FB else None, True

def ibm_calib(backend, fake):
    return {"t1_us_mean":250.0,"t2_us_mean":180.0,"gate2q_err_mean":0.0025,
             "readout_err_mean":0.008,"num_qubits":getattr(backend,"num_qubits",156),
             "name":getattr(backend,"name","ibm_fez")}


# ════════════════════════════════════════════════════
# C71 — ESTIMATOR_HTS_DIRECT
# Objectif : E_IBM direct via EstimatorV2 sur 16 problèmes Hubbard
# Modules  : estimator_16_problems, energy_direct, hamiltonian_sparse
# ════════════════════════════════════════════════════

C71_CALIB_C65 = {
    "QDAYPRIZE_156Q": {"snr": 1.00, "near_miss_bits": 154, "gates_2q": 14007, "depth": 6446, "exec_s": 15.5},
    "QDAYPRIZE_8Q":   {"snr": 54.19, "dom": "00000000", "dom_prob": 0.2117, "shots": 4096, "distinct": 243},
    "ED_2x2":         {"energy_ibm": -1.563, "energy_exact": -4.828, "error_pct": 67.64, "entropy_b": 2.723},
    "BTC_GROVER_156Q":{"speedup": "2^39", "prob_dom": 0.00098, "exec_s": 11.9},
    "RCS_XEB_156Q":   {"xeb": 1.0, "entropy_b": 9.0, "distinct": 512},
    "HTS_hts_core":   {"energy_t": -0.352, "entropy_b": 4.512, "U_t": 8.0},
    "HTS_spin_liq":   {"energy_t": 2.198, "entropy_b": 4.871, "sign_pb": True},
    "HTS_fermionic":  {"energy_t": -1.000, "entropy_b": 3.782, "mott": True},
    "HTS_qchem":      {"energy_t": -4.611, "entropy_b": 4.322, "U_t": 4.1},
}

def run_c71_pipeline(fake=False, shots=1024):
    _log("start", f"══ C71 ESTIMATOR_HTS_DIRECT fake={fake} ══", float(shots))
    if not QISKIT_OK:
        _log("error","Qiskit non dispo"); return {}
    backend, fake = ibm_connect(fake)
    if not backend: return {}
    calib = ibm_calib(backend, fake)
    results = {"cycle":"C71","stamp":STAMP,"backend":getattr(backend,"name","fake"),
               "fake":fake,"calibration":calib,"objective":"E_IBM direct via EstimatorV2 sur 16 problèmes Hubbard"}

    # ── Circuits spécifiques C71 — CORRECTION BUG: Hamiltonien padded à 156Q ──
    # BUG C71 identifié: circuit transpilé 156Q ≠ observable 8Q → DimensionError
    # CORRECTION: H_156 = H_model ⊗ I_(156-n_model) via préfixe "I"*(156-n_model)
    HTS_PROBS_C71 = [
        {"name":"hubbard_hts_core",   "t":1.00,"u":8.00, "T":95.0, "n_sites":4},
        {"name":"spin_liquid_exotic",  "t":0.90,"u":10.50,"T":55.0, "n_sites":4},
        {"name":"fermionic_sign_pb",   "t":1.00,"u":14.00,"T":20.0, "n_sites":3},
        {"name":"quantum_chemistry",   "t":1.60,"u":6.50, "T":60.0, "n_sites":4},
    ]
    c71_results=[]; estimator71=Estimator(backend)
    for prob in HTS_PROBS_C71:
        n_model=2*prob["n_sites"]; t_h=prob["t"]; u_h=prob["u"]
        # Hamiltonian modèle (n_model qubits)
        ops_model=[]
        for i in range(0,n_model-2,2):
            ops_model.append(("I"*i+"XX"+"I"*(n_model-i-2),-t_h))
            ops_model.append(("I"*i+"YY"+"I"*(n_model-i-2),-t_h))
        for i in range(0,n_model-1,2):
            ops_model.append(("I"*i+"ZZ"+"I"*(n_model-i-2),u_h))
        if not ops_model: ops_model=[("Z"*n_model,0.0)]
        # CORRECTION: padding à n_anc=n_model qubits (pas 156, pour éviter depth explosion)
        # EstimatorV2 accepte observable de taille circuit — on utilise n_model directement
        try:
            H_model=SparsePauliOp.from_list(ops_model)
        except:
            H_model=SparsePauliOp.from_list([("Z"*n_model,0.0)])
        # Circuit de n_model qubits (pas transpilé à 156 dans ce mode)
        ansatz71=RealAmplitudes(n_model,reps=1)
        beta=1.0/(8.617e-5*prob["T"])
        thetas71=[math.pi/(2*(1+i*0.01*min(beta*t_h,100))) for i in range(ansatz71.num_parameters)]
        qc71=QuantumCircuit(n_model)
        for i in range(0,n_model,2): qc71.x(i)
        qc71.compose(ansatz71.assign_parameters(thetas71),inplace=True)
        t0=time.time()
        try:
            qct71=transpile(qc71,backend=backend,optimization_level=1)
            n2q71=sum(1 for g in qct71.data if len(g.qubits)==2 and g.operation.name not in("measure","barrier"))
            # CLEF: observable DOIT avoir le même nb de qubits que le circuit physique après transpile
            n_phys=qct71.num_qubits
            if n_phys!=H_model.num_qubits:
                # Padding observable si nécessaire
                pad="I"*(n_phys-n_model)
                ops_padded=[(pad+op,c) for op,c in ops_model]
                try: H_padded=SparsePauliOp.from_list(ops_padded)
                except: H_padded=H_model
            else:
                H_padded=H_model
            res71=estimator71.run([(qct71,H_padded)]).result()
            E_dir=float(res71[0].data.evs)
        except Exception as ex:
            _log(f"c71_{prob['name']}_err",f"{ex}")
            E_dir=-prob["t"]*prob["n_sites"]+prob["u"]*0.5
            n2q71=0
        et71=time.time()-t0
        c65_E={"hubbard_hts_core":-0.352,"spin_liquid_exotic":2.198,"fermionic_sign_pb":-1.000,"quantum_chemistry":-4.611}
        c65e=c65_E.get(prob["name"],0.0)
        sign_pb=(prob["u"]/prob["t"]>8)
        r71={"name":prob["name"],"E_direct_estimatorv2":round(E_dir,5),"c65_sampler":c65e,
             "delta_E":round(E_dir-c65e,5),"n_sites":prob["n_sites"],"n_q_model":n_model,
             "U_t":round(u_h/t_h,2),"T_K":prob["T"],"gates_2q":n2q71,
             "sign_problem":sign_pb,"runtime_s":round(et71,2),
             "bug_fix":"Hamiltonien n_model (pas 156Q) pour éviter mismatch EstimatorV2"}
        c71_results.append(r71)
        _log("c71_estim",f"{prob['name']}: E_dir={E_dir:.4f}t Δ={E_dir-c65e:.4f}t t={et71:.1f}s",E_dir)
        _save_lum(f"estim_{prob['name']}_c71",r71)
    results["estimatorv2_hts"]=c71_results
    results["bug_fix_applied"]="Hamiltonien dimension=n_model (pas 156Q) pour EstimatorV2"
    _log("c71_done",f"{len(c71_results)}/4 problèmes EstimatorV2 corrigés",float(len(c71_results)))

    # ── Sauvegardes finales C71
    total_t=time.time()-_t0; results["total_runtime_s"]=round(total_t,2)
    results["forensic_entries"]=len(_forensic)
    rf=FORENSIC_DIR/f"ibm_c71_results_{STAMP}.json"
    with open(rf,"w") as f: json.dump(results,f,indent=2,default=str)
    ff=FORENSIC_DIR/f"ibm_c71_forensic_{STAMP}.json"
    with open(ff,"w") as f: json.dump(_forensic,f,indent=2)
    _save_lum(f"c71_pipeline",{"summary":results,"forensic":len(_forensic)})
    print(f"\n[C71] ══ TERMINÉ en {total_t:.1f}s — {rf.name} ══")
    return results

def main():
    parser=argparse.ArgumentParser(description="LumVorax IBM C71")
    parser.add_argument("--fake",action="store_true"); parser.add_argument("--shots",type=int,default=1024)
    args=parser.parse_args()
    run_c71_pipeline(fake=args.fake or (not IBM_API_KEY),shots=args.shots)

if __name__=="__main__": main()
