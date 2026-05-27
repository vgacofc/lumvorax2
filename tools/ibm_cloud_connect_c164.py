#!/usr/bin/env python3
"""
Connexion IBM Cloud robuste — Cycle C164
Gère les versions 0.9 à 0.30+ de qiskit-ibm-runtime
"""
import os, sys, importlib

# ── Lecture secrets ──────────────────────────────────────────────────────────
token = os.environ.get("IBM_API_KEY") or os.environ.get("IBM_QUANTUM_TOKEN", "")
crn   = os.environ.get(
    "IBM_CRN",
    "crn:v1:bluemix:public:quantum-computing:us-east:"
    "a/b893e3b64af743f9860d3ae175578d19:"
    "31cb02b4-5010-422a-a999-d6302e4d7b06::"
)
backend_name = os.environ.get("IBM_BACKEND", "ibm_fez")

print(f"[INFO] token len={len(token)}")
print(f"[INFO] crn   len={len(crn)}")
print(f"[INFO] crn   = {crn}")

try:
    from qiskit_ibm_runtime import QiskitRuntimeService
    import qiskit_ibm_runtime as qir
    ver = qir.__version__
    print(f"[INFO] qiskit-ibm-runtime version = {ver}")
except ImportError as e:
    print(f"[ERR] Import échoué : {e}")
    sys.exit(1)

# ── Méthode 1 : constructeur direct (versions >= 0.20) ────────────────────
def try_direct():
    print("[TRY] Méthode 1 : QiskitRuntimeService(channel='ibm_cloud', token=..., instance=...)")
    svc = QiskitRuntimeService(channel="ibm_cloud", token=token, instance=crn)
    return svc

# ── Méthode 2 : save_account() puis load (versions intermédiaires) ────────
def try_save_account():
    print("[TRY] Méthode 2 : save_account() puis QiskitRuntimeService(channel='ibm_cloud')")
    QiskitRuntimeService.save_account(
        channel="ibm_cloud",
        token=token,
        instance=crn,
        overwrite=True,
    )
    svc = QiskitRuntimeService(channel="ibm_cloud")
    return svc

# ── Méthode 3 : ibm_quantum fallback (très vieilles versions) ─────────────
def try_ibm_quantum():
    print("[TRY] Méthode 3 : channel='ibm_quantum' (fallback token direct)")
    svc = QiskitRuntimeService(channel="ibm_quantum", token=token)
    return svc

# ── Tentatives en cascade ─────────────────────────────────────────────────
service = None
for name, fn in [("direct", try_direct), ("save_account", try_save_account), ("ibm_quantum", try_ibm_quantum)]:
    try:
        service = fn()
        print(f"[OK] Connexion réussie via méthode '{name}'")
        break
    except Exception as e:
        print(f"[FAIL] Méthode '{name}' : {type(e).__name__}: {e}")

if service is None:
    print("[ERR] Toutes les méthodes de connexion ont échoué")
    sys.exit(1)

# ── Vérification backends ─────────────────────────────────────────────────
try:
    backends = service.backends()
    names = [b.name for b in backends[:15]]
    print(f"[OK] {len(backends)} backends : {names}")

    fez_list = [b for b in backends if b.name == backend_name]
    if fez_list:
        fez = fez_list[0]
        print(f"[OK] {backend_name} trouvé — {fez.num_qubits} qubits")
        try:
            st = fez.status()
            print(f"[OK] {backend_name} status : {st}")
        except Exception as se:
            print(f"[WARN] status() : {se}")
    else:
        print(f"[WARN] {backend_name} absent des backends disponibles")

    sys.exit(0)
except Exception as e:
    print(f"[ERR] backends() : {type(e).__name__}: {e}")
    sys.exit(1)
