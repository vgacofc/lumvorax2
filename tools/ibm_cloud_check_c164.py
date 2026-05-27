#!/usr/bin/env python3
"""
Test connexion IBM Cloud — Cycle C164
channel=ibm_cloud + CRN instance b893e3b64af743f9860d3ae175578d19
"""
import os, sys

try:
    from qiskit_ibm_runtime import QiskitRuntimeService
    print(f"[OK] qiskit_ibm_runtime importé")
except ImportError as e:
    print(f"[ERR] Import échoué : {e}")
    sys.exit(1)

token = os.environ.get("IBM_API_KEY") or os.environ.get("IBM_QUANTUM_TOKEN", "")
crn   = os.environ.get(
    "IBM_CRN",
    "crn:v1:bluemix:public:quantum-computing:us-east:"
    "a/b893e3b64af743f9860d3ae175578d19:"
    "31cb02b4-5010-422a-a999-d6302e4d7b06::"
)

print(f"[INFO] token len={len(token)}")
print(f"[INFO] crn   len={len(crn)}")
print(f"[INFO] crn   = {crn}")

try:
    svc = QiskitRuntimeService(
        channel="ibm_cloud",
        token=token,
        instance=crn,
    )
    print("[OK] QiskitRuntimeService(ibm_cloud) connecté")

    backends = svc.backends()
    names    = [b.name for b in backends[:15]]
    print(f"[OK] {len(backends)} backends disponibles : {names}")

    # Vérifier ibm_fez spécifiquement
    fez_list = [b for b in backends if b.name == "ibm_fez"]
    if fez_list:
        fez = fez_list[0]
        print(f"[OK] ibm_fez trouvé — {fez.num_qubits} qubits")
        print(f"[OK] ibm_fez status : {fez.status()}")
    else:
        print(f"[WARN] ibm_fez absent — backends disponibles : {names}")

    sys.exit(0)

except Exception as e:
    print(f"[ERR] Connexion ibm_cloud échouée : {type(e).__name__}: {e}")
    sys.exit(1)
