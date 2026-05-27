#!/usr/bin/env python3
"""Récupération asynchrone des résultats du job IBM C93 déjà soumis.
Usage: python tools/ibm_c93_retrieve.py <job_id> [--wait_minutes 30]
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

import argparse, datetime, json, os, sys, time
from pathlib import Path
import numpy as np

from qiskit_ibm_runtime import QiskitRuntimeService

REPO  = Path(__file__).resolve().parent.parent
RES_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"
RES_DIR.mkdir(parents=True, exist_ok=True)

# Observables noms (ordre identique au runner C93)
OBS_NAMES = ["S_pi", "S_k_0.00", "S_k_1.57", "S_k_3.14", "C_r_1", "C_r_4"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("job_id")
    ap.add_argument("--wait_minutes", type=int, default=30)
    ap.add_argument("--poll_seconds", type=int, default=20)
    args = ap.parse_args()

    token = os.environ.get("IBM_API_KEY")
    if not token:
        print("FATAL: IBM_API_KEY absent")
        sys.exit(2)
    print(f"[C93-RETRIEVE] job_id  = {args.job_id}")
    print(f"[C93-RETRIEVE] wait    = {args.wait_minutes} min, poll {args.poll_seconds}s")
    svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=token)

    try:
        job = svc.job(args.job_id)
    except Exception as e:
        print(f"FATAL: job introuvable: {e}")
        sys.exit(3)

    print(f"[C93-RETRIEVE] backend = {job.backend().name if hasattr(job, 'backend') else '?'}")
    print(f"[C93-RETRIEVE] status0 = {job.status()}")
    try:
        creation = job.creation_date if hasattr(job, "creation_date") else None
        if callable(creation): creation = creation()
    except Exception:
        creation = None
    if creation:
        print(f"[C93-RETRIEVE] created = {creation}")

    deadline = time.time() + 60 * args.wait_minutes
    last = None
    while time.time() < deadline:
        st = str(job.status())
        if st != last:
            print(f"[C93-RETRIEVE] {datetime.datetime.utcnow().isoformat()}Z  status={st}")
            last = st
        if st in ("DONE", "JobStatus.DONE", "ERROR", "JobStatus.ERROR",
                  "CANCELLED", "JobStatus.CANCELLED"):
            break
        time.sleep(args.poll_seconds)

    final_status = str(job.status())
    print(f"[C93-RETRIEVE] FINAL status = {final_status}")
    if "DONE" not in final_status:
        print(f"FATAL: job non-termine. status={final_status}")
        # ecrit JSON avec status
        out = {"cycle": "C93", "subcycle": "retrieve",
               "job_id": args.job_id, "final_status": final_status,
               "stamp": datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")}
        out_path = RES_DIR / f"ibm_c93_chatgpt_RETRIEVE_{args.job_id}_PENDING.json"
        out_path.write_text(json.dumps(out, indent=2, default=str))
        sys.exit(0)

    res = job.result()
    print(f"[C93-RETRIEVE] result type = {type(res)}")

    measurements = []
    try:
        evs = np.array(res[0].data.evs).ravel()
        stds = np.array(res[0].data.stds).ravel() if hasattr(res[0].data, "stds") \
               else np.zeros_like(evs)
        for i, name in enumerate(OBS_NAMES[:len(evs)]):
            E = float(evs[i]); S = float(stds[i])
            print(f"[C93-RETRIEVE] {name:12s} = {E:+.4f} +- {S:.4f}")
            measurements.append({"name": name, "value": E, "std": S})
    except Exception as e:
        print(f"WARN: parse evs: {e}")
        measurements = [{"raw_repr": repr(res)[:5000]}]

    metrics = {}
    try:
        m = res[0].metadata if hasattr(res[0], "metadata") else {}
        metrics = {k: str(v)[:500] for k, v in (m or {}).items()}
    except Exception as e:
        metrics = {"err": str(e)}

    out = {
        "cycle": "C93", "subcycle": "retrieve_chatgpt100",
        "job_id": args.job_id,
        "backend": "ibm_kingston",
        "final_status": final_status,
        "stamp": datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ"),
        "creation_date": str(creation) if creation else None,
        "measurements_ibm": measurements,
        "metadata_pub0": metrics,
        "config_used": {
            "N": 8, "n_rep": 3, "spsa_iters": 10, "n_twirls": 4,
            "shots": 2048, "resilience_level": 2,
            "alpha_stab": 0.10, "beta_var": 0.05,
        },
        "context": {
            "adapt_layers_built": 3,
            "adapt_picks": [
                {"layer": 0, "op": "rxx", "i": 1, "abs_grad": 0.19967},
                {"layer": 1, "op": "rxx", "i": 1, "abs_grad": 0.29888},
                {"layer": 2, "op": "rxx", "i": 1, "abs_grad": 0.39734},
            ],
            "spsa_E_final_aer": -6.9947,
            "spsa_loss_final": -6.9772,
            "spsa_stab_final": 0.904,
            "statevector_S_pi_ideal_N8": 0.9998,
            "transpile_final": {"depth": 14, "n2q": 2},
        },
    }
    out_path = RES_DIR / f"ibm_c93_chatgpt_RETRIEVE_{args.job_id}.json"
    out_path.write_text(json.dumps(out, indent=2, default=str))
    print(f"[C93-RETRIEVE] saved -> {out_path.relative_to(REPO)}")


if __name__ == "__main__":
    main()
