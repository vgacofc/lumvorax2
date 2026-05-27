#!/usr/bin/env python3
"""Recuperateur asynchrone job IBM C94. Usage:
   python tools/ibm_c94_retrieve.py <job_id> --N 12 --wait_minutes 15
"""
import ctypes as _ct, os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p); break
        except Exception: pass

import argparse, datetime, json, math, os, sys, time
from pathlib import Path
import numpy as np

from qiskit_ibm_runtime import QiskitRuntimeService

REPO    = Path(__file__).resolve().parent.parent
RES_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"

def obs_names_for_N(N):
    # ordre identique au runner C94 build_observables
    return ["S_pi", "S_k_0.00", "S_k_1.57", "S_k_3.14",
            "C_r_1", f"C_r_{N // 2}"]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("job_id")
    ap.add_argument("--N", type=int, default=12)
    ap.add_argument("--wait_minutes", type=int, default=15)
    ap.add_argument("--poll_seconds", type=int, default=20)
    args = ap.parse_args()
    token = os.environ.get("IBM_API_KEY")
    if not token:
        print("FATAL: IBM_API_KEY absent"); sys.exit(2)
    print(f"[C94-RETRIEVE] job_id={args.job_id} N={args.N} wait={args.wait_minutes}min")
    svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=token)
    job = svc.job(args.job_id)
    print(f"[C94-RETRIEVE] backend={job.backend().name} status0={job.status()}")
    try:
        cd = job.creation_date if hasattr(job, "creation_date") else None
        if callable(cd): cd = cd()
    except Exception: cd = None
    if cd: print(f"[C94-RETRIEVE] created={cd}")
    deadline = time.time() + 60 * args.wait_minutes
    last = None
    while time.time() < deadline:
        st = str(job.status())
        if st != last:
            print(f"[C94-RETRIEVE] {datetime.datetime.utcnow().isoformat()}Z status={st}")
            last = st
        if "DONE" in st or "ERROR" in st or "CANCELLED" in st: break
        time.sleep(args.poll_seconds)
    final = str(job.status())
    print(f"[C94-RETRIEVE] FINAL status={final}")
    if "DONE" not in final:
        out = {"cycle":"C94","retrieve":True,"job_id":args.job_id,
               "final_status":final,"N":args.N}
        p = RES_DIR / f"ibm_c94_RETRIEVE_{args.job_id}_PENDING.json"
        p.write_text(json.dumps(out, indent=2, default=str))
        print(f"[C94-RETRIEVE] saved -> {p.relative_to(REPO)}"); sys.exit(0)
    res = job.result()
    names = obs_names_for_N(args.N)
    measurements = []
    pub_idx = 0
    try:
        for i, name in enumerate(names):
            try:
                evs = np.array(res[i].data.evs).ravel()
                stds = np.array(res[i].data.stds).ravel() if hasattr(res[i].data, "stds") \
                       else np.zeros_like(evs)
                E = float(evs[0]); S = float(stds[0])
            except Exception:
                # fallback : tout dans un seul pub
                evs = np.array(res[0].data.evs).ravel()
                stds = np.array(res[0].data.stds).ravel() if hasattr(res[0].data, "stds") \
                       else np.zeros_like(evs)
                E = float(evs[i]); S = float(stds[i] if i < len(stds) else 0.0)
            print(f"[C94-RETRIEVE] {name:12s} = {E:+.4f} +- {S:.4f}")
            measurements.append({"name": name, "value": E, "std": S})
    except Exception as e:
        print(f"WARN parse: {e}")
        measurements = [{"raw_repr": repr(res)[:5000]}]
    metrics = {}
    try:
        m = res[0].metadata if hasattr(res[0], "metadata") else {}
        metrics = {k: str(v)[:500] for k, v in (m or {}).items()}
    except Exception as e:
        metrics = {"err": str(e)}
    out = {
        "cycle":"C94","retrieve":True,"job_id":args.job_id,
        "backend":"ibm_kingston","final_status":final,
        "N":args.N,
        "stamp": datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ"),
        "creation_date": str(cd) if cd else None,
        "measurements_ibm": measurements,
        "metadata_pub0": metrics,
    }
    p = RES_DIR / f"ibm_c94_RETRIEVE_{args.job_id}.json"
    p.write_text(json.dumps(out, indent=2, default=str))
    print(f"[C94-RETRIEVE] saved -> {p.relative_to(REPO)}")

if __name__ == "__main__":
    main()
