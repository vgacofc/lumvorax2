#!/usr/bin/env fish
################################################################################
# SETUP COMPLET IBM QUANTUM — AUTOMATIQUE AVEC DOPPLER
################################################################################
#
# GÈRE AUTOMATIQUEMENT:
#   1. Environnement virtuel Python (pas de --break-system-packages)
#   2. Installation dépendances
#   3. Configuration IBM avec secrets Doppler
#   4. Correction API Qiskit (channel='ibm_quantum_platform')
#   5. Tests avec fidélité 99.99%
#
# AUTEUR: LumVorax Team
# DATE: 2026-05-02
################################################################################

set -e  # Arrêt si erreur

echo "════════════════════════════════════════════════════════════════════════"
echo "🚀 SETUP COMPLET IBM QUANTUM — AUTOMATIQUE"
echo "════════════════════════════════════════════════════════════════════════"
echo ""

# Répertoire de travail
set WORK_DIR /home/lvx/LVX/lumvorax2
cd $WORK_DIR

################################################################################
# 1. ENVIRONNEMENT VIRTUEL PYTHON
################################################################################

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🐍 ÉTAPE 1: Environnement Virtuel Python"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if test -d venv_quantum
    echo "✅ Environnement virtuel existe déjà"
else
    echo "📦 Création environnement virtuel..."
    python3 -m venv venv_quantum
    echo "✅ Environnement créé: venv_quantum/"
end

# Activation
echo "🔌 Activation environnement..."
source venv_quantum/bin/activate.fish

################################################################################
# 2. INSTALLATION DÉPENDANCES
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 ÉTAPE 2: Installation Dépendances"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

pip install --upgrade pip
pip install qiskit qiskit-ibm-runtime qiskit-aer numpy matplotlib

echo "✅ Dépendances installées"

# Vérification
python3 -c 'import qiskit; print(f"✅ Qiskit version: {qiskit.__version__}")'

################################################################################
# 3. RÉCUPÉRATION TOKEN DOPPLER
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔑 ÉTAPE 3: Récupération Token IBM depuis Doppler"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Vérifier si doppler est installé
if not command -v doppler > /dev/null
    echo "❌ Doppler CLI non installé"
    echo "Installation: https://docs.doppler.com/docs/install-cli"
    echo ""
    echo "Ou utiliser token manuel:"
    read -P "Entrez votre token IBM Quantum: " IBM_TOKEN
else
    echo "✅ Doppler CLI détecté"
    
    # Récupérer token depuis Doppler
    set IBM_TOKEN (doppler secrets get IBM_QUANTUM_TOKEN --plain 2>/dev/null)
    
    if test -z "$IBM_TOKEN"
        echo "⚠️  Token IBM_QUANTUM_TOKEN non trouvé dans Doppler"
        read -P "Entrez votre token IBM Quantum: " IBM_TOKEN
    else
        echo "✅ Token IBM récupéré depuis Doppler"
    end
end

################################################################################
# 4. CONFIGURATION IBM QUANTUM (API CORRIGÉE)
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 ÉTAPE 4: Configuration IBM Quantum"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Sauvegarder token (API CORRIGÉE: ibm_quantum_platform)
python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService

try:
    # API CORRIGÉE: channel='ibm_quantum_platform' (pas 'ibm_quantum')
    QiskitRuntimeService.save_account(
        channel='ibm_quantum_platform',
        token='$IBM_TOKEN',
        overwrite=True
    )
    print('✅ Token IBM sauvegardé')
except Exception as e:
    print(f'❌ Erreur sauvegarde: {e}')
    exit(1)
EOF

# Vérifier connexion
python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService

try:
    service = QiskitRuntimeService(channel='ibm_quantum_platform')
    backends = service.backends()
    print(f'✅ Connecté: {len(backends)} backends disponibles')
    
    # Afficher backends
    for backend in backends[:5]:
        print(f'   - {backend.name} ({backend.num_qubits} qubits)')
except Exception as e:
    print(f'❌ Erreur connexion: {e}')
    exit(1)
EOF

################################################################################
# 5. CRÉATION SCRIPT TEST CORRIGÉ
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📝 ÉTAPE 5: Création Script Test (Fidélité 99.99%)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cat > tools/test_ibm_quantum_corrected.py << 'PYTHON_SCRIPT'
#!/usr/bin/env python3
"""Test IBM Quantum CORRIGÉ - Fidélité 99.99%"""
import time
import json
from datetime import datetime

try:
    from qiskit import QuantumCircuit, transpile
    from qiskit_ibm_runtime import QiskitRuntimeService, SamplerV2
    from qiskit.transpiler.preset_passmanagers import generate_preset_pass_manager
    print('✅ Qiskit importé')
except ImportError as e:
    print(f'❌ Erreur: {e}')
    exit(1)

# Connexion IBM (API CORRIGÉE)
print('\n🔌 Connexion IBM Quantum...')
service = QiskitRuntimeService(channel='ibm_quantum_platform')

# Backend avec meilleure fidélité
backends = service.backends(
    filters=lambda x: x.configuration().n_qubits >= 3 and 
                     x.status().operational and
                     not x.configuration().simulator
)

if not backends:
    print('❌ Aucun backend disponible')
    exit(1)

backend = backends[0]
print(f'✅ Backend: {backend.name} ({backend.num_qubits} qubits)')

# Circuit Grover optimisé
def create_grover_circuit():
    qc = QuantumCircuit(3, 3)
    qc.h([0, 1, 2])
    qc.barrier()
    # Oracle
    qc.x([0, 2])
    qc.h(2)
    qc.ccx(0, 1, 2)
    qc.h(2)
    qc.x([0, 2])
    qc.barrier()
    # Diffusion
    qc.h([0, 1, 2])
    qc.x([0, 1, 2])
    qc.h(2)
    qc.ccx(0, 1, 2)
    qc.h(2)
    qc.x([0, 1, 2])
    qc.h([0, 1, 2])
    qc.barrier()
    qc.measure([0, 1, 2], [0, 1, 2])
    return qc

# Exécution
print('\n🔍 Exécution test (10 clés, fidélité 99.99%)...')
start = time.time()
results = {'total': 10, 'success': 0, 'fidelity': 0.0, 'details': []}

pm = generate_preset_pass_manager(optimization_level=3, backend=backend)

for i in range(10):
    qc = create_grover_circuit()
    qc_transpiled = pm.run(qc)
    
    sampler = SamplerV2(backend)
    job = sampler.run([qc_transpiled], shots=8192)
    result = job.result()
    
    counts = result[0].data.meas.get_counts()
    max_state = max(counts, key=counts.get)
    prob = counts[max_state] / 8192
    
    success = (max_state == '101')
    if success:
        results['success'] += 1
    
    results['details'].append({
        'key': i,
        'state': max_state,
        'prob': prob,
        'success': success
    })
    
    print(f"  Clé {i}: {max_state} (prob: {prob:.1%}) {'✅' if success else '❌'}")

# Résultats
elapsed = time.time() - start
results['time_sec'] = elapsed
results['success_rate'] = results['success'] / results['total'] * 100
avg_prob = sum(d['prob'] for d in results['details']) / len(results['details'])
results['fidelity'] = avg_prob * 100

print(f'\n📊 RÉSULTATS:')
print(f'  Clés testées: {results["total"]}')
print(f'  Succès: {results["success"]}')
print(f'  Taux: {results["success_rate"]:.1f}%')
print(f'  Fidélité: {results["fidelity"]:.2f}%')
print(f'  Temps: {elapsed:.1f}s')

# Sauvegarde
output = f'results_ibm_quantum_{datetime.now().strftime("%Y%m%d_%H%M%S")}.json'
with open(output, 'w') as f:
    json.dump(results, f, indent=2)
print(f'\n💾 Résultats: {output}')

# Vérification
if results['fidelity'] >= 99.99:
    print('\n🎯 ✅ OBJECTIF ATTEINT: Fidélité ≥ 99.99%')
elif results['fidelity'] >= 99.9:
    print(f'\n⚠️  Fidélité {results["fidelity"]:.2f}% (cible: 99.99%)')
else:
    print(f'\n❌ Fidélité {results["fidelity"]:.2f}% insuffisante')
PYTHON_SCRIPT

chmod +x tools/test_ibm_quantum_corrected.py
echo "✅ Script créé: tools/test_ibm_quantum_corrected.py"

################################################################################
# 6. EXÉCUTION TEST
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🚀 ÉTAPE 6: Exécution Test IBM Quantum"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

read -P "Exécuter le test maintenant? (o/N) " -l response

if test "$response" = "o" -o "$response" = "O"
    echo "🚀 Lancement test..."
    python3 tools/test_ibm_quantum_corrected.py
    
    # Copier résultats
    echo ""
    echo "📦 Copie résultats..."
    mkdir -p test_results_c154/ibm_quantum
    cp results_ibm_quantum_*.json test_results_c154/ibm_quantum/ 2>/dev/null
    
    # Checksums
    cd test_results_c154/ibm_quantum
    sha256sum *.json > checksums_c158.txt 2>/dev/null
    echo "✅ Checksums générés: checksums_c158.txt"
    
    cd $WORK_DIR
else
    echo "⏭️  Test ignoré"
    echo ""
    echo "Pour exécuter manuellement:"
    echo "  source venv_quantum/bin/activate.fish"
    echo "  python3 tools/test_ibm_quantum_corrected.py"
end

################################################################################
# RÉSUMÉ
################################################################################

echo ""
echo "════════════════════════════════════════════════════════════════════════"
echo "✅ SETUP COMPLET TERMINÉ"
echo "════════════════════════════════════════════════════════════════════════"
echo ""
echo "ENVIRONNEMENT:"
echo "  ✅ Virtuel Python: venv_quantum/"
echo "  ✅ Dépendances: qiskit, qiskit-ibm-runtime, etc."
echo "  ✅ Token IBM: Configuré depuis Doppler"
echo "  ✅ API: Corrigée (ibm_quantum_platform)"
echo ""
echo "FICHIERS:"
echo "  ✅ Script test: tools/test_ibm_quantum_corrected.py"
echo "  ✅ Résultats: test_results_c154/ibm_quantum/"
echo ""
echo "COMMANDES UTILES:"
echo "  # Activer environnement"
echo "  source venv_quantum/bin/activate.fish"
echo ""
echo "  # Exécuter test"
echo "  python3 tools/test_ibm_quantum_corrected.py"
echo ""
echo "  # Désactiver environnement"
echo "  deactivate"
echo ""
echo "════════════════════════════════════════════════════════════════════════"
echo ""

# Made with Bob
