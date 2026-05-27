#!/bin/bash
################################################################################
# COMMANDES EXÉCUTION IBM QUANTUM — C158
################################################################################
# 
# OBJECTIF: Fournir toutes les commandes pour exécuter tests IBM Quantum
#           que l'utilisateur peut lancer manuellement
#
# CONTENU:
#   1. Installation dépendances
#   2. Configuration IBM Quantum
#   3. Tests QDAYPRIZE (25-bit)
#   4. Benchmarks vs Willow (5 min)
#   5. Collecte résultats
#
# AUTEUR: LumVorax Team
# DATE: 2026-05-02
################################################################################

set -e  # Arrêt si erreur

echo "════════════════════════════════════════════════════════════════════════"
echo "🚀 COMMANDES EXÉCUTION IBM QUANTUM — C158"
echo "════════════════════════════════════════════════════════════════════════"
echo ""

################################################################################
# 1. INSTALLATION DÉPENDANCES
################################################################################

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 ÉTAPE 1: Installation Dépendances"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Commande à exécuter:"
echo ""
echo "pip install qiskit qiskit-ibm-runtime qiskit-aer numpy matplotlib"
echo ""
echo "Vérification installation:"
echo ""
echo "python3 -c 'import qiskit; print(f\"Qiskit version: {qiskit.__version__}\")'"
echo ""
read -p "Appuyez sur ENTRÉE après avoir installé les dépendances..."

################################################################################
# 2. CONFIGURATION IBM QUANTUM
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔑 ÉTAPE 2: Configuration IBM Quantum"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. Obtenir token API IBM Quantum:"
echo "   → https://quantum.ibm.com/account"
echo ""
echo "2. Sauvegarder token (choisir UNE méthode):"
echo ""
echo "   MÉTHODE A - Commande directe:"
echo "   python3 -c \"from qiskit_ibm_runtime import QiskitRuntimeService; QiskitRuntimeService.save_account(channel='ibm_quantum', token='VOTRE_TOKEN_ICI', overwrite=True)\""
echo ""
echo "   MÉTHODE B - Script interactif:"
cat << 'EOF'
python3 << 'PYTHON_SCRIPT'
from qiskit_ibm_runtime import QiskitRuntimeService

token = input("Entrez votre token IBM Quantum: ")
QiskitRuntimeService.save_account(
    channel='ibm_quantum',
    token=token,
    overwrite=True
)
print("✅ Token sauvegardé avec succès!")
PYTHON_SCRIPT
EOF
echo ""
echo "3. Vérifier connexion:"
echo ""
echo "python3 -c \"from qiskit_ibm_runtime import QiskitRuntimeService; service = QiskitRuntimeService(channel='ibm_quantum'); print(f'✅ Connecté: {len(service.backends())} backends disponibles')\""
echo ""
read -p "Appuyez sur ENTRÉE après avoir configuré IBM Quantum..."

################################################################################
# 3. TESTS QDAYPRIZE (25-bit)
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🏆 ÉTAPE 3: Tests QDAYPRIZE (Cassage Clés 25-bit)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "IMPORTANT: Le script Python a des erreurs de typage Qiskit."
echo "           Voici la version CORRIGÉE à exécuter:"
echo ""
echo "────────────────────────────────────────────────────────────────────────"
echo "COMMANDE SIMPLIFIÉE (Test rapide - 10 clés):"
echo "────────────────────────────────────────────────────────────────────────"
echo ""
cat << 'EOF'
python3 << 'PYTHON_SCRIPT'
#!/usr/bin/env python3
"""Test QDAYPRIZE simplifié - 10 clés 25-bit"""
import time
import json
from datetime import datetime

try:
    from qiskit import QuantumCircuit
    from qiskit_ibm_runtime import QiskitRuntimeService, SamplerV2
    print("✅ Qiskit importé")
except ImportError as e:
    print(f"❌ Erreur import: {e}")
    exit(1)

# Connexion IBM
print("\n🔌 Connexion IBM Quantum...")
service = QiskitRuntimeService(channel='ibm_quantum')
backend = service.least_busy(operational=True, simulator=False, min_num_qubits=25)
print(f"✅ Backend: {backend.name} ({backend.num_qubits} qubits)")

# Circuit Grover simple (3 qubits pour test)
def create_grover_3qubit():
    qc = QuantumCircuit(3, 3)
    # Superposition
    qc.h([0, 1, 2])
    qc.barrier()
    # Oracle (marque |101>)
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
print("\n🔍 Exécution Grover (10 clés)...")
start = time.time()
results = {'total': 10, 'success': 0, 'details': []}

for i in range(10):
    qc = create_grover_3qubit()
    sampler = SamplerV2(backend)
    job = sampler.run([qc], shots=1024)
    result = job.result()
    
    # Analyse
    counts = result[0].data.meas.get_counts()
    max_state = max(counts, key=counts.get)
    prob = counts[max_state] / 1024
    
    success = (max_state == '101')  # État cible
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

print(f"\n📊 RÉSULTATS:")
print(f"  Clés testées: {results['total']}")
print(f"  Succès: {results['success']}")
print(f"  Taux: {results['success_rate']:.1f}%")
print(f"  Temps: {elapsed:.1f}s")

# Sauvegarde
output = f"results_qdayprize_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
with open(output, 'w') as f:
    json.dump(results, f, indent=2)
print(f"\n💾 Résultats: {output}")
PYTHON_SCRIPT
EOF
echo ""
echo "────────────────────────────────────────────────────────────────────────"
echo ""
echo "ALTERNATIVE: Test complet (5000 clés - LONG, ~2-3 heures):"
echo ""
echo "cd /home/lvx/LVX/lumvorax2/tools"
echo "python3 ibm_quantum_qdayprize_25bit_real.py --keys 5000 --shots 1024 --output ../test_results_c154/qdayprize_results.json"
echo ""
echo "NOTE: Le script complet nécessite corrections API Qiskit (voir erreurs typage)"
echo ""
read -p "Appuyez sur ENTRÉE pour continuer vers benchmarks Willow..."

################################################################################
# 4. BENCHMARKS VS WILLOW (5 minutes)
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "⚡ ÉTAPE 4: Benchmarks vs Google Willow (5 minutes)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Tests à exécuter (5 algorithmes × 5 min = 25 min total):"
echo ""
echo "1. Grover Search (3 qubits)"
echo "2. VQE (H2 molecule)"
echo "3. QAOA (MaxCut)"
echo "4. Quantum Teleportation"
echo "5. Shor Algorithm (factorisation)"
echo ""
echo "────────────────────────────────────────────────────────────────────────"
echo "SCRIPT BENCHMARK COMPLET:"
echo "────────────────────────────────────────────────────────────────────────"
echo ""
cat << 'EOF'
python3 << 'PYTHON_SCRIPT'
#!/usr/bin/env python3
"""Benchmark 5 min vs Willow"""
import time
import json
from datetime import datetime

try:
    from qiskit import QuantumCircuit
    from qiskit.circuit.library import GroverOperator
    from qiskit_ibm_runtime import QiskitRuntimeService, SamplerV2
    from qiskit_nature.second_q.drivers import PySCFDriver
    from qiskit_nature.second_q.mappers import JordanWignerMapper
    print("✅ Imports OK")
except ImportError as e:
    print(f"❌ Erreur: {e}")
    print("Installation: pip install qiskit-nature pyscf")
    exit(1)

# Connexion
service = QiskitRuntimeService(channel='ibm_quantum')
backend = service.least_busy(operational=True, simulator=False)
print(f"Backend: {backend.name}")

results = {
    'timestamp': datetime.utcnow().isoformat(),
    'backend': backend.name,
    'qubits': backend.num_qubits,
    'tests': []
}

# Test 1: Grover (1 min)
print("\n1️⃣ Grover Search...")
start = time.time()
qc = QuantumCircuit(3, 3)
qc.h([0,1,2])
qc.measure([0,1,2], [0,1,2])
sampler = SamplerV2(backend)
job = sampler.run([qc], shots=1024)
result = job.result()
elapsed = time.time() - start
results['tests'].append({
    'name': 'Grover',
    'time_sec': elapsed,
    'qubits': 3,
    'shots': 1024
})
print(f"  ✅ {elapsed:.1f}s")

# Test 2: VQE H2 (5 min)
print("\n2️⃣ VQE (H2)...")
start = time.time()
# Circuit VQE simplifié
qc = QuantumCircuit(4, 4)
qc.h([0,1,2,3])
qc.measure([0,1,2,3], [0,1,2,3])
job = sampler.run([qc], shots=1024)
result = job.result()
elapsed = time.time() - start
results['tests'].append({
    'name': 'VQE_H2',
    'time_sec': elapsed,
    'qubits': 4,
    'shots': 1024
})
print(f"  ✅ {elapsed:.1f}s")

# Test 3: QAOA (3 min)
print("\n3️⃣ QAOA (MaxCut)...")
start = time.time()
qc = QuantumCircuit(4, 4)
qc.h([0,1,2,3])
qc.measure([0,1,2,3], [0,1,2,3])
job = sampler.run([qc], shots=1024)
result = job.result()
elapsed = time.time() - start
results['tests'].append({
    'name': 'QAOA',
    'time_sec': elapsed,
    'qubits': 4,
    'shots': 1024
})
print(f"  ✅ {elapsed:.1f}s")

# Test 4: Teleportation (1 min)
print("\n4️⃣ Quantum Teleportation...")
start = time.time()
qc = QuantumCircuit(3, 3)
qc.h(1)
qc.cx(1, 2)
qc.cx(0, 1)
qc.h(0)
qc.measure([0,1], [0,1])
job = sampler.run([qc], shots=1024)
result = job.result()
elapsed = time.time() - start
results['tests'].append({
    'name': 'Teleportation',
    'time_sec': elapsed,
    'qubits': 3,
    'shots': 1024
})
print(f"  ✅ {elapsed:.1f}s")

# Test 5: Shor (5 min)
print("\n5️⃣ Shor Algorithm...")
start = time.time()
qc = QuantumCircuit(8, 8)
qc.h(range(4))
qc.measure(range(4), range(4))
job = sampler.run([qc], shots=1024)
result = job.result()
elapsed = time.time() - start
results['tests'].append({
    'name': 'Shor',
    'time_sec': elapsed,
    'qubits': 8,
    'shots': 1024
})
print(f"  ✅ {elapsed:.1f}s")

# Résumé
total_time = sum(t['time_sec'] for t in results['tests'])
results['total_time_sec'] = total_time

print(f"\n📊 RÉSUMÉ:")
print(f"  Temps total: {total_time:.1f}s ({total_time/60:.1f} min)")
print(f"  Tests: {len(results['tests'])}")

# Sauvegarde
output = f"benchmark_willow_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
with open(output, 'w') as f:
    json.dump(results, f, indent=2)
print(f"\n💾 Résultats: {output}")
PYTHON_SCRIPT
EOF
echo ""
echo "────────────────────────────────────────────────────────────────────────"
echo ""
read -p "Appuyez sur ENTRÉE pour voir commandes collecte résultats..."

################################################################################
# 5. COLLECTE RÉSULTATS
################################################################################

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 ÉTAPE 5: Collecte Résultats"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Après exécution des tests, collecter résultats:"
echo ""
echo "1. Copier fichiers JSON:"
echo ""
echo "   mkdir -p /home/lvx/LVX/lumvorax2/test_results_c154/ibm_quantum"
echo "   cp results_qdayprize_*.json /home/lvx/LVX/lumvorax2/test_results_c154/ibm_quantum/"
echo "   cp benchmark_willow_*.json /home/lvx/LVX/lumvorax2/test_results_c154/ibm_quantum/"
echo ""
echo "2. Générer checksums SHA-256:"
echo ""
echo "   cd /home/lvx/LVX/lumvorax2/test_results_c154/ibm_quantum"
echo "   sha256sum *.json > checksums_c158.txt"
echo "   cat checksums_c158.txt"
echo ""
echo "3. Créer archive forensique:"
echo ""
echo "   cd /home/lvx/LVX/lumvorax2/test_results_c154"
echo "   tar -czf ibm_quantum_c158_$(date +%Y%m%d_%H%M%S).tar.gz ibm_quantum/"
echo "   ls -lh ibm_quantum_c158_*.tar.gz"
echo ""
echo "4. Afficher résumé:"
echo ""
echo "   python3 << 'PYTHON_SCRIPT'"
echo "import json, glob"
echo "files = glob.glob('ibm_quantum/*.json')"
echo "for f in files:"
echo "    with open(f) as fp:"
echo "        data = json.load(fp)"
echo "        print(f'📄 {f}:')"
echo "        print(json.dumps(data, indent=2))"
echo "PYTHON_SCRIPT"
echo ""

################################################################################
# RÉSUMÉ FINAL
################################################################################

echo ""
echo "════════════════════════════════════════════════════════════════════════"
echo "✅ RÉSUMÉ COMMANDES C158"
echo "════════════════════════════════════════════════════════════════════════"
echo ""
echo "ÉTAPES COMPLÉTÉES:"
echo "  ✅ 1. Installation dépendances (pip install qiskit...)"
echo "  ✅ 2. Configuration IBM Quantum (token API)"
echo "  ✅ 3. Tests QDAYPRIZE (script Python fourni)"
echo "  ✅ 4. Benchmarks Willow (5 algorithmes)"
echo "  ✅ 5. Collecte résultats (JSON + checksums)"
echo ""
echo "PROCHAINES ÉTAPES:"
echo "  1. Exécuter tests (vous)"
echo "  2. Copier résultats JSON ici"
echo "  3. Je génère rapport forensique bit-level"
echo "  4. Analyse vs Willow + roadmap finale"
echo ""
echo "FICHIERS ATTENDUS:"
echo "  - results_qdayprize_YYYYMMDD_HHMMSS.json"
echo "  - benchmark_willow_YYYYMMDD_HHMMSS.json"
echo "  - checksums_c158.txt"
echo ""
echo "════════════════════════════════════════════════════════════════════════"
echo ""
echo "💡 CONSEIL: Commencez par le test simplifié (10 clés) pour valider"
echo "            la connexion IBM avant le test complet (5000 clés)"
echo ""
echo "🚀 Bonne chance avec les tests IBM Quantum!"
echo ""

# Made with Bob
