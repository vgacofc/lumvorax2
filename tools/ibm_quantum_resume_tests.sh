#!/bin/bash
# Script reprise tests IBM Quantum C84-C87
# Date: 2026-05-02
# Auteur: Bob (Expert LumVorax)

set -e

echo "=== IBM Quantum Tests Resume C154 ==="
date '+%Y-%m-%d %H:%M:%S'

# Répertoire résultats
RESULTS_DIR="test_results_c154/ibm_quantum_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

# Login IBM Cloud
echo "1. Login IBM Cloud..."
ibmcloud login -a https://cloud.ibm.com -u passcode -p Pjx5QI3KDg 2>&1 | tee "$RESULTS_DIR/ibmcloud_login.log"

# Configuration API Qiskit
echo "2. Configuration Qiskit..."
export QISKIT_IBM_TOKEN="SZBxNgwGXxZQYvRLntLAT8PTgvWOBJMfAZJZe9evwpda"

# Vérifier Python + Qiskit
echo "3. Vérification environnement..."
python3 --version
python3 -c "import qiskit; print(f'Qiskit version: {qiskit.__version__}')" 2>&1 | tee "$RESULTS_DIR/env_check.log"

# Test 1: Grover Search
echo "4. Test Grover Search..."
if [ -f "tools/ibm_quantum_grover.py" ]; then
    python3 tools/ibm_quantum_grover.py 2>&1 | tee "$RESULTS_DIR/grover_results.log"
else
    echo "⚠️ tools/ibm_quantum_grover.py non trouvé, création..."
    cat > tools/ibm_quantum_grover.py << 'EOFGROVER'
#!/usr/bin/env python3
from qiskit import QuantumCircuit, transpile
from qiskit_ibm_runtime import QiskitRuntimeService
import os
import json
from datetime import datetime

# Configuration
token = os.getenv('QISKIT_IBM_TOKEN')
service = QiskitRuntimeService(channel="ibm_quantum", token=token)

# Circuit Grover 3 qubits
qc = QuantumCircuit(3, 3)
qc.h([0, 1, 2])  # Superposition
qc.cz(0, 2)      # Oracle
qc.h([0, 1, 2])  # Diffusion
qc.x([0, 1, 2])
qc.h(2)
qc.ccx(0, 1, 2)
qc.h(2)
qc.x([0, 1, 2])
qc.h([0, 1, 2])
qc.measure([0, 1, 2], [0, 1, 2])

# Exécution
backend = service.least_busy(operational=True, simulator=False)
print(f"Backend: {backend.name}")

transpiled = transpile(qc, backend)
job = backend.run(transpiled, shots=1024)
result = job.result()
counts = result.get_counts()

# Sauvegarde
output = {
    "test": "grover_search",
    "backend": backend.name,
    "shots": 1024,
    "counts": counts,
    "timestamp": datetime.now().isoformat()
}

with open("test_results_c154/grover_results.json", "w") as f:
    json.dump(output, f, indent=2)

print(f"✅ Grover Search: {counts}")
EOFGROVER
    chmod +x tools/ibm_quantum_grover.py
    python3 tools/ibm_quantum_grover.py 2>&1 | tee "$RESULTS_DIR/grover_results.log"
fi

# Test 2: VQE (Variational Quantum Eigensolver)
echo "5. Test VQE..."
if [ -f "tools/ibm_quantum_vqe.py" ]; then
    python3 tools/ibm_quantum_vqe.py 2>&1 | tee "$RESULTS_DIR/vqe_results.log"
else
    echo "⚠️ tools/ibm_quantum_vqe.py non trouvé, skip"
fi

# Test 3: Quantum Teleportation
echo "6. Test Quantum Teleportation..."
if [ -f "tools/ibm_quantum_teleportation.py" ]; then
    python3 tools/ibm_quantum_teleportation.py 2>&1 | tee "$RESULTS_DIR/teleportation_results.log"
else
    echo "⚠️ tools/ibm_quantum_teleportation.py non trouvé, skip"
fi

# Synthèse
echo ""
echo "=== Synthèse Tests IBM Quantum ==="
echo "Résultats sauvegardés: $RESULTS_DIR"
ls -lh "$RESULTS_DIR"

echo "✅ Tests IBM Quantum terminés"
date '+%Y-%m-%d %H:%M:%S'

# Made with Bob
