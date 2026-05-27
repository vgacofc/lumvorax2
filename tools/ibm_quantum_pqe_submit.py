#!/usr/bin/env python3
"""
🧠 PQE — SOUMISSION JOB IBM (rapide)
Soumet le job et retourne immédiatement le job_id
"""

import json
from datetime import datetime
from pathlib import Path

import numpy as np
from qiskit import QuantumCircuit, QuantumRegister, ClassicalRegister, transpile
from qiskit.circuit.library import PauliEvolutionGate
from qiskit.quantum_info import SparsePauliOp
from qiskit_ibm_runtime import QiskitRuntimeService, SamplerV2 as Sampler

# Configuration
N_CORE = 2
N_ANCILLA = 2
J_ZZ, J_XX, J_YY = 1.0, 0.5, 0.3
T_EVOLUTION = 0.3
N_PHASE_STEPS = 12
SHOTS = 1024

def create_pqe_circuit():
    """Créer circuit PQE minimal"""
    qr_core = QuantumRegister(N_CORE, "core")
    qr_anc = QuantumRegister(N_ANCILLA, "anc")
    cr = ClassicalRegister(N_ANCILLA, "c")
    qc = QuantumCircuit(qr_core, qr_anc, cr)
    
    # 1. Superposition
    for i in range(N_CORE):
        qc.h(qr_core[i])
    
    # 2. Hamiltonien
    H = SparsePauliOp.from_list([
        ("ZZ", J_ZZ), ("XX", J_XX), ("YY", J_YY)
    ])
    qc.append(PauliEvolutionGate(H, time=T_EVOLUTION), qr_core)
    
    # 3. Phase géométrique
    for step in range(N_PHASE_STEPS):
        theta = 2 * np.pi * step / N_PHASE_STEPS
        qc.rz(theta, qr_core[0])
        if N_CORE > 1:
            qc.rx(theta/2, qr_core[1])
    
    # 4. Couplage ancilla
    for i in range(min(N_CORE, N_ANCILLA)):
        qc.cx(qr_core[i], qr_anc[i])
    
    # 5. Amplification
    for i in range(N_CORE):
        qc.h(qr_core[i])
    if N_CORE >= 2:
        qc.mcx(qr_core[:N_CORE-1], qr_core[N_CORE-1])
    
    # 6. Mesure
    qc.measure(qr_anc, cr)
    
    return qc

print("🧠 PQE — Soumission job IBM Quantum")
print("="*60)

# Connexion
print("\n[1/4] Connexion IBM...")
service = QiskitRuntimeService(channel='ibm_quantum_platform')
backend = service.backend('ibm_fez')
print(f"✓ Backend: {backend.name}")

# Circuit
print("\n[2/4] Création circuit...")
qc = create_pqe_circuit()
print(f"✓ Circuit: {qc.num_qubits} qubits, {qc.depth()} gates")

# Transpilation
print("\n[3/4] Transpilation...")
qc_t = transpile(qc, backend=backend, optimization_level=3)
print(f"✓ Transpilé: {qc_t.depth()} gates")

# Soumission
print("\n[4/4] Soumission job...")
sampler = Sampler(mode=backend)
job = sampler.run([qc_t], shots=SHOTS)
job_id = job.job_id()

print(f"\n✅ JOB SOUMIS: {job_id}")
print(f"\nRécupérer résultats avec:")
print(f"python3 tools/retrieve_ibm_job_results.py {job_id}")

# Sauvegarder job_id
output_file = f"../test_results_c154/pqe_job_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
Path(output_file).parent.mkdir(parents=True, exist_ok=True)
with open(output_file, 'w') as f:
    f.write(f"JOB_ID: {job_id}\n")
    f.write(f"BACKEND: {backend.name}\n")
    f.write(f"SHOTS: {SHOTS}\n")
    f.write(f"TIMESTAMP: {datetime.now().isoformat()}\n")

print(f"✓ Job ID sauvegardé: {output_file}")
print("="*60)

# Made with Bob
