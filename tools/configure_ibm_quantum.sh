#!/bin/bash
# Configuration IBM Quantum avec token Doppler
# Cycle C162 - 2026-05-03

set -e

echo "🔧 Configuration IBM Quantum..."

# Vérifier Doppler
if ! command -v doppler &> /dev/null; then
    echo "❌ Erreur : Doppler CLI non installé"
    echo "   Installation : curl -Ls https://cli.doppler.com/install.sh | sh"
    exit 1
fi

# Récupérer token depuis Doppler
echo "📡 Récupération token depuis Doppler..."
IBM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN --plain 2>/dev/null)

if [ -z "$IBM_TOKEN" ]; then
    echo "❌ Erreur : Token IBM non trouvé dans Doppler"
    echo "   Vérifiez : doppler secrets get IBM_QUANTUM_TOKEN --plain"
    exit 1
fi

echo "✅ Token récupéré depuis Doppler (${#IBM_TOKEN} caractères)"

# Vérifier environnement virtuel
if [ ! -d ".venv-ibm" ]; then
    echo "⚠️  Environnement virtuel .venv-ibm non trouvé"
    echo "   Création..."
    python3 -m venv .venv-ibm
    .venv-ibm/bin/pip install --upgrade pip
    .venv-ibm/bin/pip install qiskit qiskit-aer qiskit-ibm-runtime
    echo "✅ Environnement virtuel créé"
fi

# Activer environnement virtuel
source .venv-ibm/bin/activate

# Configurer Qiskit Runtime
echo "🔐 Configuration Qiskit Runtime..."
python3 << PYTHON
import sys
from qiskit_ibm_runtime import QiskitRuntimeService

try:
    # Sauvegarder compte avec token (sans instance, détection auto)
    QiskitRuntimeService.save_account(
        channel="ibm_quantum_platform",
        token="${IBM_TOKEN}",
        overwrite=True
    )
    print("✅ Compte IBM Quantum configuré")
    
    # Tester connexion
    print("🔍 Test connexion...")
    service = QiskitRuntimeService()
    backends = service.backends()
    
    print(f"✅ Connexion réussie : {len(backends)} backends disponibles")
    print("\n📊 Backends disponibles :")
    for backend in backends[:10]:
        status = backend.status()
        print(f"  - {backend.name:20s} : {backend.num_qubits:3d} qubits, {status.status_msg}")
    
    if len(backends) > 10:
        print(f"  ... et {len(backends) - 10} autres")
    
    sys.exit(0)
    
except Exception as e:
    print(f"❌ Erreur configuration : {e}")
    sys.exit(1)
PYTHON

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Configuration terminée avec succès"
    echo ""
    echo "🚀 Prochaines étapes :"
    echo "   1. Dry-run : .venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --dry-run"
    echo "   2. IBM run : .venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_fez --shots 4096"
else
    echo ""
    echo "❌ Configuration échouée"
    exit 1
fi

# Made with Bob
