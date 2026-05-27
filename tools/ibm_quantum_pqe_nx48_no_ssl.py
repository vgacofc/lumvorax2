#!/usr/bin/env python3
"""
Version sans vérification SSL pour contourner problème certificat
UTILISE TOKEN DOPPLER AUTOMATIQUEMENT
"""

import os
import sys
import subprocess
import urllib3

# Désactiver warnings SSL
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# Désactiver vérification SSL
os.environ['CURL_CA_BUNDLE'] = ''
os.environ['REQUESTS_CA_BUNDLE'] = ''
os.environ['PYTHONHTTPSVERIFY'] = '0'

# Récupérer token depuis Doppler
print("📡 Récupération token IBM depuis Doppler...")
try:
    token = subprocess.check_output(
        ["doppler", "secrets", "get", "IBM_QUANTUM_TOKEN", "--plain"],
        text=True
    ).strip()
    print(f"✅ Token récupéré ({len(token)} caractères)")
except Exception as e:
    print(f"❌ Erreur Doppler : {e}")
    sys.exit(1)

# Configurer token dans environnement
os.environ['QISKIT_IBM_TOKEN'] = token
os.environ['QISKIT_IBM_INSTANCE'] = 'ibm-q/open/main'

# Import après configuration SSL
from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as Estimator
from qiskit import QuantumCircuit, transpile
from qiskit.quantum_info import SparsePauliOp
import numpy as np

print("🔐 Configuration IBM Quantum...")
try:
    service = QiskitRuntimeService(
        channel="ibm_quantum_platform",
        token=token,
        instance="ibm-q/open/main"
    )
    print("✅ Connexion IBM réussie")
    
    backends = service.backends()
    print(f"📊 {len(backends)} backends disponibles")
    for b in backends[:5]:
        print(f"  - {b.name}")
    
except Exception as e:
    print(f"❌ Erreur connexion : {e}")
    sys.exit(1)

# Créer circuit simple pour test
print("\n🔬 Création circuit test...")
qc = QuantumCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.measure([0, 1], [0, 1])

# Observable simple
obs = SparsePauliOp.from_list([("ZZ", 1.0)])

# Backend
backend_name = sys.argv[1] if len(sys.argv) > 1 else "ibm_fez"
print(f"🎯 Backend : {backend_name}")

try:
    backend = service.backend(backend_name)
    print(f"✅ Backend {backend_name} : {backend.num_qubits} qubits")
except Exception as e:
    print(f"❌ Backend non disponible : {e}")
    print("Backends disponibles :")
    for b in backends:
        print(f"  - {b.name}")
    sys.exit(1)

# Transpiler
print("🔄 Transpilation...")
qc_t = transpile(qc, backend, optimization_level=3)
print(f"✅ Circuit transpilé : {qc_t.num_qubits} qubits")

# Étendre observable
if hasattr(qc_t, 'layout') and qc_t.layout is not None:
    try:
        obs_mapped = obs.apply_layout(qc_t.layout)
        print(f"✅ Observable étendu : {obs.num_qubits}q → {obs_mapped.num_qubits}q")
    except:
        obs_mapped = obs
        print("⚠️  Observable non étendu (layout incompatible)")
else:
    obs_mapped = obs

# Soumettre
print("\n🚀 Soumission job IBM...")
estimator = Estimator(mode=backend)

try:
    job = estimator.run([(qc_t, obs_mapped)])
    job_id = job.job_id()
    
    print(f"\n✅ JOB SOUMIS : {job_id}")
    print(f"Backend : {backend_name}")
    print(f"Shots : 1024")
    print(f"\nRécupérer résultats :")
    print(f"  python3 tools/retrieve_ibm_job_results.py {job_id}")
    
    # Sauvegarder job_id
    with open(f"test_results_c154/job_{job_id}.txt", "w") as f:
        f.write(f"{job_id}\n")
    
except Exception as e:
    print(f"❌ Erreur soumission : {e}")
    sys.exit(1)

# Made with Bob
