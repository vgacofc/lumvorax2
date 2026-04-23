#!/usr/bin/env python3
"""Recupere le resultat d'un job IBM deja soumis (par job_id)."""
import ctypes, os, sys, json, time, datetime
for p in ["/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6"]:
    if os.path.exists(p):
        try: ctypes.CDLL(p); break
        except: pass
from pathlib import Path
from qiskit_ibm_runtime import QiskitRuntimeService

JOBID = sys.argv[1] if len(sys.argv) > 1 else "d7l8pe28ui0s73b646eg"
print(f"[recover] job_id={JOBID}", flush=True)
svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=os.environ["IBM_API_KEY"])
job = svc.job(JOBID)
t0 = time.time()
while True:
    s = str(job.status())
    print(f"[{time.time()-t0:6.1f}s] status={s}", flush=True)
    if s in ("DONE", "CANCELLED", "ERROR", "JobStatus.DONE", "JobStatus.CANCELLED", "JobStatus.ERROR"): break
    if "DONE" in s or "CANCEL" in s or "ERROR" in s: break
    if time.time()-t0 > 1500:
        print("[recover] TIMEOUT 1500s", flush=True); sys.exit(2)
    time.sleep(20)
res = job.result()
out = []
for i, r in enumerate(res):
    out.append({"i": i, "evs": float(r.data.evs),
                "stds": float(r.data.stds) if hasattr(r.data, "stds") else 0.0})
print("[recover] RESULTS", json.dumps(out), flush=True)
stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
meta = [{"kind": "ghz", "N": 8, "depth_phys": 24, "n2q_phys": 7,
         "S_pi": out[0]["evs"], "S_pi_std": out[0]["stds"]},
        {"kind": "hva", "N": 8, "depth_phys": 73, "n2q_phys": 21,
         "S_pi": out[1]["evs"], "S_pi_std": out[1]["stds"]}]
S_ghz_th = sum(((-1.0)**((i-j)%2)) for i in range(8) for j in range(8) if i!=j)/(8*7)
meta[0]["S_pi_theorie_ideal"] = S_ghz_th
meta[0]["fidelity_proxy"] = abs(meta[0]["S_pi"]/S_ghz_th)
p = Path(f"src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c91_recovered_{stamp}.json")
p.write_text(json.dumps({"cycle": "C91", "job_id": JOBID, "backend": "ibm_kingston",
                         "stamp": stamp, "meta": meta, "raw": out}, indent=2))
print(f"[recover] SAVED {p}", flush=True)
