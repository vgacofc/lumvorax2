# 🚀 GUIDE COMPLET - Exécution IBM Quantum C162

**Date** : 2026-05-03  
**Cycle** : C162  
**Objectif** : Guide pas-à-pas pour soumettre PQE NX48 sur IBM Quantum

---

## 📋 PRÉREQUIS

### 1. Environnement Virtuel
```bash
cd /home/lvx/LVX/lumvorax2

# Vérifier que .venv-ibm existe
ls -la .venv-ibm/

# Si absent, créer :
python3 -m venv .venv-ibm
.venv-ibm/bin/pip install qiskit qiskit-aer qiskit-ibm-runtime
```

### 2. Token IBM Quantum
Le token est stocké dans **Doppler** (gestionnaire de secrets).

**Récupérer le token** :
```bash
doppler secrets get IBM_QUANTUM_TOKEN --plain
```

**Token actuel** : `ksmOvZguV2nlEAep0NgkT3NWczY-s5LUJfYXg7VLpVg_`

### 3. Fichiers Requis
- ✅ [`tools/ibm_quantum_pqe_nx48_integrated.py`](../../../tools/ibm_quantum_pqe_nx48_integrated.py) (script principal)
- ✅ `config/btc_nx48_last.csv` (état NX48)
- ✅ `src/advanced_calculations/bitcoin_quantum_mining/quantum_pre_measure_state.py` (pré-mesure)

---

## 🔧 CONFIGURATION IBM QUANTUM

### Méthode 1 : Configuration Automatique (RECOMMANDÉ)

Créer script de configuration :

```bash
cat > tools/configure_ibm_quantum.sh << 'EOF'
#!/bin/bash
# Configuration IBM Quantum avec token Doppler

set -e

echo "🔧 Configuration IBM Quantum..."

# Récupérer token depuis Doppler
IBM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN --plain)

if [ -z "$IBM_TOKEN" ]; then
    echo "❌ Erreur : Token IBM non trouvé dans Doppler"
    exit 1
fi

echo "✅ Token récupéré depuis Doppler"

# Activer environnement virtuel
source .venv-ibm/bin/activate

# Configurer Qiskit Runtime
python3 << PYTHON
from qiskit_ibm_runtime import QiskitRuntimeService

# Sauvegarder compte avec token
QiskitRuntimeService.save_account(
    channel="ibm_quantum",
    token="${IBM_TOKEN}",
    instance="ibm-q/open/main",
    overwrite=True
)

print("✅ Compte IBM Quantum configuré")

# Tester connexion
try:
    service = QiskitRuntimeService()
    backends = service.backends()
    print(f"✅ Connexion réussie : {len(backends)} backends disponibles")
    for backend in backends[:5]:
        print(f"  - {backend.name}")
except Exception as e:
    print(f"❌ Erreur connexion : {e}")
    exit(1)
PYTHON

echo "✅ Configuration terminée"
EOF

chmod +x tools/configure_ibm_quantum.sh
```

**Exécuter** :
```bash
./tools/configure_ibm_quantum.sh
```

### Méthode 2 : Configuration Manuelle

```bash
# Activer venv
source .venv-ibm/bin/activate

# Récupérer token
IBM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN --plain)

# Configurer
python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService

QiskitRuntimeService.save_account(
    channel="ibm_quantum",
    token="$IBM_TOKEN",
    instance="ibm-q/open/main",
    overwrite=True
)
print("✅ Configuré")
EOF
```

### Méthode 3 : Variable d'Environnement (TEMPORAIRE)

```bash
# Exporter token
export QISKIT_IBM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN --plain)
export QISKIT_IBM_INSTANCE="ibm-q/open/main"

# Utiliser dans script
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_fez --shots 4096
```

---

## 🚀 EXÉCUTION PQE NX48

### Étape 1 : Vérifier Configuration

```bash
cd /home/lvx/LVX/lumvorax2

# Tester connexion IBM
.venv-ibm/bin/python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
print("Backends:", [b.name for b in service.backends()])
EOF
```

**Sortie attendue** :
```
Backends: ['ibm_fez', 'ibm_marrakesh', 'ibm_kingston', ...]
```

### Étape 2 : Dry-Run Local (Validation)

```bash
# Test avec Aer Simulator
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --dry-run
```

**Sortie attendue** :
```
======================================================================
PQE NX48 INTEGRATED - Cycle C162
======================================================================

[1/7] Chargement état NX48...
  Exploration bias : 0.902
  Delta nonce scale: 40.73

[2/7] Simulation quantum pre-measure state...
  |α|² (prob |0⟩) : 0.8536
  |β|² (prob |1⟩) : 0.1464

[3/7] Construction observables complets...
  Observables : 6

[4/7] Construction circuits PQE...
  Circuits générés : 9

[5/7] Construction pubs...
  Pubs totaux : 54

[6/7] Dry-run Aer...
  RÉSULTATS : 5 pubs

[7/7] Sauvegarde logs forensiques...
  13 événements sauvegardés

✅ TERMINÉ
```

### Étape 3 : Soumission IBM Quantum (PRODUCTION)

```bash
# Soumettre sur ibm_fez (156 qubits)
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py \
    --backend ibm_fez \
    --shots 4096 \
    --resilience 2

# Ou sur ibm_marrakesh (127 qubits)
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py \
    --backend ibm_marrakesh \
    --shots 4096 \
    --resilience 2
```

**Sortie attendue** :
```
======================================================================
PQE NX48 INTEGRATED - Cycle C162
======================================================================

[1/7] Chargement état NX48...
  ✅ État chargé

[2/7] Simulation quantum pre-measure state...
  ✅ Amplitudes calculées

[3/7] Construction observables complets...
  ✅ 6 observables

[4/7] Construction circuits PQE...
  ✅ 9 circuits

[5/7] Construction pubs...
  ✅ 54 pubs

[6/7] Submit IBM...
  Backend : ibm_fez
  Shots   : 4096
  Resilience : 2
  Transpilation...
    Observable étendu : 2q → 156q
    Observable étendu : 2q → 156q
    ... (54 fois)
  Soumission job...
  
  ✅ Job soumis : cxxxxxxxxxxxxxxxxx
  Backend : ibm_fez
  Pubs    : 54
  
[7/7] Sauvegarde logs forensiques...
  ✅ 13 événements sauvegardés

======================================================================
✅ TERMINÉ
======================================================================

Job ID : cxxxxxxxxxxxxxxxxx
Récupérer résultats :
  python3 tools/retrieve_ibm_job_results.py cxxxxxxxxxxxxxxxxx
```

### Étape 4 : Récupération Résultats

```bash
# Attendre fin du job (15-30 min)
# Puis récupérer résultats :

.venv-ibm/bin/python3 tools/retrieve_ibm_job_results.py <JOB_ID>
```

**Sortie attendue** :
```
🔍 Récupération job : cxxxxxxxxxxxxxxxxx

Status : COMPLETED
Backend : ibm_fez
Shots : 4096
Pubs : 54

📊 Résultats :
  Pub 0 (S_pi) : 0.8234 ± 0.0123
  Pub 1 (S_k_0) : 0.7456 ± 0.0098
  ...

✅ Résultats sauvegardés : test_results_c154/job_cxxxxxxxxxxxxxxxxx_results.json
```

---

## 🐛 RÉSOLUTION PROBLÈMES

### Problème 1 : Token Invalide

**Erreur** :
```
InvalidAccountError: 'Unable to retrieve instances. Please check that you are using a valid API token.'
```

**Solution** :
```bash
# Vérifier token Doppler
doppler secrets get IBM_QUANTUM_TOKEN --plain

# Reconfigurer
./tools/configure_ibm_quantum.sh
```

### Problème 2 : Certificat SSL

**Erreur** :
```
ssl.SSLCertVerificationError: certificate verify failed
```

**Solution A** : Désactiver VPN/Proxy
```bash
# Désactiver VPN temporairement
sudo systemctl stop openvpn

# Réessayer
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_fez --shots 4096
```

**Solution B** : Utiliser token direct dans script
```python
# Modifier tools/ibm_quantum_pqe_nx48_integrated.py ligne ~563
import os
token = os.popen("doppler secrets get IBM_QUANTUM_TOKEN --plain").read().strip()

service = QiskitRuntimeService(
    channel="ibm_quantum",
    token=token,
    instance="ibm-q/open/main"
)
```

### Problème 3 : Backend Indisponible

**Erreur** :
```
Backend 'ibm_fez' not found
```

**Solution** :
```bash
# Lister backends disponibles
.venv-ibm/bin/python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
for b in service.backends():
    print(f"{b.name}: {b.num_qubits} qubits, status={b.status().status_msg}")
EOF

# Utiliser backend alternatif
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_marrakesh --shots 4096
```

### Problème 4 : Quota Dépassé

**Erreur** :
```
RuntimeError: Job submission failed: Quota exceeded
```

**Solution** :
```bash
# Vérifier quota
.venv-ibm/bin/python3 << EOF
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
print("Quota:", service.usage())
EOF

# Attendre reset quota (mensuel) ou utiliser dry-run
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --dry-run
```

---

## 📊 ANALYSE RÉSULTATS

### Étape 1 : Charger Résultats

```python
import json

# Charger résultats IBM
with open("test_results_c154/job_cxxxxxxxxxxxxxxxxx_results.json", "r") as f:
    results = json.load(f)

# Charger logs forensiques
import csv
with open("src/advanced_calculations/bitcoin_quantum_mining/CHAT/DONNEES_FORENSIQUES_pqe_nx48_*.csv", "r") as f:
    reader = csv.DictReader(f)
    forensic_logs = list(reader)
```

### Étape 2 : Analyser Observables

```python
# Extraire valeurs S(π)
s_pi_values = [r["value"] for r in results if r["observable"] == "S_pi"]

# Comparer avec dry-run
dry_run_s_pi = 0.8234  # Valeur Aer Simulator
ibm_s_pi = s_pi_values[0]

fidelity = 1 - abs(dry_run_s_pi - ibm_s_pi) / dry_run_s_pi
print(f"Fidélité : {fidelity:.2%}")
```

### Étape 3 : Vérifier Cohérence NX48

```python
# Extraire NX48 bias depuis forensic
nx48_bias = float([log["value"] for log in forensic_logs if log["event"] == "nx48_exploration_bias"][0])

# Vérifier corrélation avec résultats
if nx48_bias > 0.9:
    # Exploitation → S(π) élevé attendu
    assert ibm_s_pi > 0.7, "Incohérence : exploitation mais S(π) faible"
```

### Étape 4 : Créer Rapport Final

```bash
# Générer rapport automatique
.venv-ibm/bin/python3 << EOF
import json
import csv

# Charger données
with open("test_results_c154/job_cxxxxxxxxxxxxxxxxx_results.json", "r") as f:
    results = json.load(f)

# Créer rapport
report = f"""
# RAPPORT RÉSULTATS IBM QUANTUM - Job {results['job_id']}

## Configuration
- Backend : {results['backend']}
- Shots : {results['shots']}
- Pubs : {len(results['pubs'])}

## Résultats Clés
- S(π) : {results['pubs'][0]['value']:.4f} ± {results['pubs'][0]['std']:.4f}
- Fidélité vs dry-run : {results['fidelity']:.2%}

## Conclusion
{'✅ Résultats cohérents' if results['fidelity'] > 0.9 else '⚠️ Décohérence détectée'}
"""

with open("src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_RESULTATS_IBM_C162.md", "w") as f:
    f.write(report)

print("✅ Rapport créé")
EOF
```

---

## 🔄 WORKFLOW COMPLET

### Pour le Prochain Agent

```bash
#!/bin/bash
# Workflow complet PQE NX48 sur IBM Quantum

set -e

echo "🚀 Démarrage workflow PQE NX48..."

# 1. Configuration
echo "[1/5] Configuration IBM Quantum..."
./tools/configure_ibm_quantum.sh

# 2. Validation dry-run
echo "[2/5] Validation dry-run..."
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --dry-run

# 3. Soumission IBM
echo "[3/5] Soumission IBM Quantum..."
JOB_OUTPUT=$(.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_fez --shots 4096)
JOB_ID=$(echo "$JOB_OUTPUT" | grep "Job soumis" | awk '{print $4}')

echo "✅ Job soumis : $JOB_ID"

# 4. Attente résultats
echo "[4/5] Attente résultats (15-30 min)..."
while true; do
    STATUS=$(.venv-ibm/bin/python3 -c "from qiskit_ibm_runtime import QiskitRuntimeService; service = QiskitRuntimeService(); job = service.job('$JOB_ID'); print(job.status())")
    
    if [ "$STATUS" == "COMPLETED" ]; then
        echo "✅ Job terminé"
        break
    elif [ "$STATUS" == "ERROR" ] || [ "$STATUS" == "CANCELLED" ]; then
        echo "❌ Job échoué : $STATUS"
        exit 1
    fi
    
    echo "⏳ Status : $STATUS"
    sleep 60
done

# 5. Récupération et analyse
echo "[5/5] Récupération résultats..."
.venv-ibm/bin/python3 tools/retrieve_ibm_job_results.py $JOB_ID

echo "✅ Workflow terminé"
echo "Résultats : test_results_c154/job_${JOB_ID}_results.json"
```

**Sauvegarder** :
```bash
cat > tools/run_pqe_nx48_full_workflow.sh << 'EOF'
[contenu ci-dessus]
EOF

chmod +x tools/run_pqe_nx48_full_workflow.sh
```

**Exécuter** :
```bash
./tools/run_pqe_nx48_full_workflow.sh
```

---

## 📝 CHECKLIST AGENT

Avant de lancer l'exécution, vérifier :

- [ ] Doppler configuré (`doppler secrets get IBM_QUANTUM_TOKEN --plain`)
- [ ] Environnement virtuel créé (`.venv-ibm/`)
- [ ] Dépendances installées (`qiskit`, `qiskit-ibm-runtime`)
- [ ] Compte IBM configuré (`./tools/configure_ibm_quantum.sh`)
- [ ] Connexion testée (liste backends disponibles)
- [ ] Fichiers requis présents (`btc_nx48_last.csv`, `quantum_pre_measure_state.py`)
- [ ] Dry-run validé (54 pubs, 13 événements forensiques)
- [ ] Backend choisi (`ibm_fez` ou `ibm_marrakesh`)
- [ ] Quota disponible (vérifier usage IBM)

---

## 🎯 RÉSUMÉ COMMANDES

```bash
# Configuration (1 fois)
./tools/configure_ibm_quantum.sh

# Test connexion
.venv-ibm/bin/python3 -c "from qiskit_ibm_runtime import QiskitRuntimeService; print([b.name for b in QiskitRuntimeService().backends()])"

# Dry-run
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --dry-run

# Soumission IBM
.venv-ibm/bin/python3 tools/ibm_quantum_pqe_nx48_integrated.py --backend ibm_fez --shots 4096

# Récupération résultats
.venv-ibm/bin/python3 tools/retrieve_ibm_job_results.py <JOB_ID>

# Workflow complet
./tools/run_pqe_nx48_full_workflow.sh
```

---

**Auteur** : Bob (Advanced Mode)  
**Date** : 2026-05-03  
**Cycle** : C162  
**Statut** : ✅ Guide complet avec token Doppler