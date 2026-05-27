import os, json, time, sys
from datetime import datetime, timezone

JOB_ID = sys.argv[1] if len(sys.argv) > 1 else "d7mf66tqrg3c738l6gq0"
NS = [16, 24, 32, 48, 64, 96, 128, 156]
PUB_NAMES = ["S_pi", "S_0", "S_pi2", "E_total", "C_r1", "C_r4"]

print(f"[C97.2] poll job_id={JOB_ID}", flush=True)

from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService(channel="ibm_quantum_platform", token=os.environ["IBM_API_KEY"])
job = service.job(JOB_ID)

for attempt in range(30):
    st = str(job.status())
    print(f"[C97.2] attempt {attempt+1}: {st}", flush=True)
    if st.upper().endswith("DONE") or "ERROR" in st.upper() or "CANCEL" in st.upper():
        break
    time.sleep(15)

if not str(job.status()).upper().endswith("DONE"):
    print(f"[C97.2] NOT DONE after 30 attempts. Status={job.status()}", flush=True)
    sys.exit(2)

print("[C97.2] DONE — extracting results ...", flush=True)
result = job.result()
out = {
    "job_id": JOB_ID,
    "cycle": "C97.2",
    "extracted_at": datetime.now(timezone.utc).isoformat(),
    "ns": NS,
    "pub_names": PUB_NAMES,
    "data": []
}
for n_idx, pub_result in enumerate(result):
    N = NS[n_idx] if n_idx < len(NS) else None
    evs = pub_result.data.evs if hasattr(pub_result.data, 'evs') else None
    stds = pub_result.data.stds if hasattr(pub_result.data, 'stds') else None
    entry = {"N": N, "evs": list(evs) if evs is not None else None,
             "stds": list(stds) if stds is not None else None}
    out["data"].append(entry)
    if evs is not None:
        evs_l = list(evs)
        stds_l = list(stds) if stds is not None else [0]*len(evs_l)
        s = "  ".join(f"{name}={ev:+.4f}±{sd:.4f}" for name, ev, sd in zip(PUB_NAMES, evs_l, stds_l))
        print(f"[C97.2] N={N}: {s}", flush=True)

with open("/tmp/c97_2_results.json", "w") as f:
    json.dump(out, f, indent=2, default=str)
print(f"[C97.2] saved /tmp/c97_2_results.json", flush=True)
print(json.dumps(out, indent=2, default=str)[:5000])
