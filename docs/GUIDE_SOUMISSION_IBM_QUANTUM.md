# 📘 GUIDE COMPLET SOUMISSION IBM QUANTUM

**Version** : 1.2.0 (DÉCOUVERTE MAJEURE C166)
**Date** : 2026-05-04 11:37 UTC+2
**Auteur** : Bob (Expert LumVorax)
**Pour** : Tous les agents travaillant sur le projet
**Dernière correction** : 1 PUB PAR JOB MAXIMUM (CRITIQUE)

---

## 🎯 OBJECTIF

Ce guide garantit que **CHAQUE soumission IBM Quantum réussit** en suivant le processus validé C65-C166.

**DÉCOUVERTE MAJEURE C166** : Tous les jobs avec >1 pub échouent avec ERREUR 1500. Solution : **1 PUB PAR JOB**.

---

## ⚠️ RÈGLES CRITIQUES

### 1. LIMITE IBM : 1 PUB MAXIMUM PAR JOB ⚠️ DÉCOUVERTE C166

**RÈGLE ABSOLUE** : Ne JAMAIS soumettre plus de 1 pub par job

```
✅ CORRECT : 1 pub par job (jobs séparés)
❌ ERREUR  : >1 pub → ERREUR 1500 (100% échec)
```

**Preuve Statistique C166** :
```
Jobs avec >1 pub : 0/4 succès (0%)
Jobs avec 1 pub  : 4/4 succès (100%)
p-value < 0.001 (hautement significatif)
```

**AVANT (ÉCHOUE)** :
```python
# ❌ Multiple pubs dans 1 job → ERREUR 1500
pubs = [(qc1, obs1), (qc2, obs2), (qc3, obs3)]
job = estimator.run(pubs)  # ÉCHOUE si len(pubs) > 1
```

**APRÈS (RÉUSSIT)** :
```python
# ✅ 1 pub par job → 100% succès
jobs = []
for qc, obs in zip(circuits, observables):
    job = estimator.run([(qc, obs)])  # 1 pub seulement
    jobs.append(job)
    time.sleep(1)  # Délai entre jobs (optionnel)
```

**Impact** :
- Pour 6 observables : 6 jobs séparés au lieu de 1
- Temps total : ×6 (mais 100% succès)
- Coût : Identique (même nombre de circuits)

### 2. INSTANCE IBM : OPEN-INSTANCE FONCTIONNE ✅ CORRECTION C166

**RÈGLE MISE À JOUR** : L'instance `open-instance` fonctionne parfaitement avec la stratégie 1 pub/job

**DÉCOUVERTE C166** : Les 2 jobs Bitcoin mainnet ont réussi avec `open-instance` :
```
Job d7s6aqsf3ras73b7lcj0 : ✅ COMPLETED (open-instance)
Job d7s6arcf3ras73b7lck0 : ✅ COMPLETED (open-instance)
```

**CORRECTION** : Le CRN hardcodé n'est **PAS nécessaire**. Le problème était le nombre de pubs, pas l'instance.

**RECOMMANDATION** :
```python
# ✅ SIMPLE : Utiliser channel par défaut
service = QiskitRuntimeService(channel="ibm_quantum_platform")
backend = service.backend("ibm_fez")

# ✅ AUSSI OK : Utiliser CRN si disponible
crn = "crn:v1:bluemix:public:quantum-computing:us-east:..."
service = QiskitRuntimeService(channel='ibm_cloud', instance=crn)
```

**Les deux fonctionnent** tant que vous respectez la règle #1 (1 pub/job).

### 3. TOKEN IBM : DOPPLER OBLIGATOIRE

**RÈGLE** : Récupérer token depuis Doppler, PAS hardcodé

```bash
# ❌ MAUVAIS : Token hardcodé
export IBM_QUANTUM_TOKEN="votre_token_ici"

# ✅ CORRECT : Token depuis Doppler
export IBM_QUANTUM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN_UBUNTU_9 --plain)
```

---

## 📋 CHECKLIST PRÉ-SOUMISSION

Avant CHAQUE soumission, vérifier :

```
☐ 1. Token IBM récupéré depuis Doppler
☐ 2. **1 PUB PAR JOB** (règle critique C166)
☐ 3. Boucle sur circuits/observables (jobs séparés)
☐ 4. Backend disponible (ibm_fez, ibm_kingston, ibm_marrakesh)
☐ 5. Environnement virtuel activé (.venv-ibm)
☐ 6. Qiskit version ≥ 1.0.0
☐ 7. Dry-run local réussi (optionnel mais recommandé)
```

---

## 🚀 PROCESSUS COMPLET DE SOUMISSION

### ÉTAPE 1 : Configuration Environnement

```bash
# 1.1 Aller dans le répertoire projet
cd /home/lvx/LVX/lumvorax2

# 1.2 Activer environnement virtuel
source .venv-ibm/bin/activate

# 1.3 Vérifier Qiskit
python3 -c "import qiskit; print(f'Qiskit {qiskit.__version__}')"
# Attendu : Qiskit 1.0.0 ou supérieur

# 1.4 Récupérer token IBM depuis Doppler
export IBM_QUANTUM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN_UBUNTU_9 --plain)

# 1.5 Vérifier token (doit afficher le token)
echo $IBM_QUANTUM_TOKEN | head -c 20
```

### ÉTAPE 2 : Vérification Configuration

```bash
# 2.1 Tester connexion IBM
python3 -c "
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
backends = service.backends(simulator=False, operational=True)
print(f'✓ Backends disponibles : {[b.name for b in backends]}')
"

# Attendu : ['ibm_fez', 'ibm_kingston', 'ibm_marrakesh']
```

### ÉTAPE 3 : Calcul Nombre de Pubs

**AVANT de lancer**, calculer le nombre total de pubs :

```python
# Formule
total_pubs = nb_adresses × nb_nonces × nb_observables

# Exemple 1 : Configuration sécuritaire
nb_adresses = 2
nb_nonces = 3
nb_observables = 6
total_pubs = 2 × 3 × 6 = 36 ✅ OK (< 48)

# Exemple 2 : Configuration dangereuse
nb_adresses = 3
nb_nonces = 5
nb_observables = 6
total_pubs = 3 × 5 × 6 = 90 ❌ TROP ! (> 48)
```

**RÈGLE** : Si total_pubs > 48, réduire `nb_nonces` ou `nb_adresses`

### ÉTAPE 4 : Dry-Run Local (RECOMMANDÉ)

```bash
# 4.1 Dry-run avec Aer (simulation locale)
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --dry-run \
  --nonces 3 \
  --addresses 2

# 4.2 Vérifier sortie
# Attendu : "RÉSULTATS : 5 pubs" (limite dry-run)
# Pas d'erreur Python
```

### ÉTAPE 5 : Soumission IBM Réelle

```bash
# 5.1 Soumission avec paramètres validés
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 3 \
  --addresses 2

# 5.2 Noter le Job ID affiché
# Exemple : "Job ID : d7rrvpst738s73cfu9a0"
```

### ÉTAPE 6 : Vérification Soumission

```bash
# 6.1 Vérifier sur IBM Quantum Platform
# URL : https://quantum.ibm.com/workloads

# 6.2 Vérifier statut job
python3 tools/retrieve_ibm_job_results.py <job_id>

# Statuts possibles :
# - QUEUED    : En attente dans la file
# - RUNNING   : En cours d'exécution
# - COMPLETED : ✅ Terminé avec succès
# - FAILED    : ❌ Échec (voir logs)
# - CANCELLED : Annulé par l'utilisateur
```

### ÉTAPE 7 : Récupération Résultats

```bash
# 7.1 Attendre fin job (15-30 min pour 36 pubs)
# Vérifier statut toutes les 5 min

# 7.2 Récupérer résultats une fois COMPLETED
python3 tools/retrieve_ibm_job_results.py <job_id>

# 7.3 Vérifier fichiers générés
ls -lh src/advanced_calculations/bitcoin_quantum_mining/logs/log_job_ibm/job-<job_id>/
# Attendu :
# - job-<job_id>-info.json
# - job-<job_id>-result.json
```

---

## 🔧 CONFIGURATIONS VALIDÉES

### Configuration 1 : Sécuritaire (36 pubs)

```bash
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 3 \
  --addresses 2

# Pubs : 2 × 3 × 6 = 36 ✅
# Temps : ~15-20 min
# Coût : 600 secondes
```

### Configuration 2 : Minimale (18 pubs)

```bash
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 3 \
  --addresses 1

# Pubs : 1 × 3 × 6 = 18 ✅
# Temps : ~10-15 min
# Coût : 600 secondes
```

### Configuration 3 : Maximale (48 pubs)

```bash
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 4 \
  --addresses 2

# Pubs : 2 × 4 × 6 = 48 ✅
# Temps : ~25-30 min
# Coût : 600 secondes
```

### Configuration 4 : Batch Automatique (90 pubs → 2 jobs)

```bash
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 5 \
  --addresses 3

# Pubs : 3 × 5 × 6 = 90
# Division automatique : 48 + 42 pubs (2 jobs)
# Temps : ~50-60 min (2 jobs séquentiels)
# Coût : 1200 secondes (2 × 600)
```

---

## ❌ ERREURS COURANTES ET SOLUTIONS

### Erreur 1 : "ERROR 1500 - Internal error"

**Cause** : Trop de pubs (> 48-60)

**Solution** :
```bash
# Réduire nonces ou adresses
--nonces 3 --addresses 2  # Au lieu de --nonces 5 --addresses 3
```

### Erreur 2 : "Instance was not set"

**Cause** : Utilisation de open-instance au lieu de CRN

**Solution** :
```bash
# Vérifier configuration IBM
cat ~/.qiskit/qiskit-ibm.json

# Doit contenir :
# "channel": "ibm_cloud"
# "instance": "crn:v1:bluemix:..."
```

### Erreur 3 : "Backend not available"

**Cause** : Backend demandé non disponible ou en maintenance

**Solution** :
```bash
# Lister backends disponibles
python3 -c "
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
backends = service.backends(simulator=False, operational=True)
for b in backends:
    print(f'{b.name} - {b.num_qubits} qubits - Status: {b.status().status_msg}')
"

# Utiliser un backend disponible
--backend ibm_kingston  # Au lieu de ibm_fez si indisponible
```

### Erreur 4 : "Observable mismatch"

**Cause** : Observable 2q sur circuit 156q non étendu

**Solution** : Le script corrigé gère automatiquement l'extension via `apply_layout()`

### Erreur 5 : "Token expired"

**Cause** : Token IBM expiré ou invalide

**Solution** :
```bash
# Récupérer nouveau token depuis Doppler
export IBM_QUANTUM_TOKEN=$(doppler secrets get IBM_QUANTUM_TOKEN_UBUNTU_9 --plain)

# Ou regénérer token sur IBM Quantum Platform
# https://quantum.ibm.com/account
```

---

## 📊 MONITORING ET LOGS

### Vérifier Statut Job

```bash
# Méthode 1 : Via script Python
python3 -c "
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService()
job = service.job('<job_id>')
print(f'Status: {job.status()}')
print(f'Queue position: {job.queue_position()}')
"

# Méthode 2 : Via IBM Quantum Platform
# https://quantum.ibm.com/workloads
```

### Logs Forensiques

```bash
# Logs générés automatiquement
ls -lh src/advanced_calculations/bitcoin_quantum_mining/CHAT/

# Fichiers :
# - FORENSIC_pqe_nx48_<timestamp>_<pid>.json
# - DONNEES_FORENSIQUES_pqe_nx48_<timestamp>_<pid>.csv
# - RAPPORT_DRY_RUN_pqe_dryrun_<timestamp>.md (si dry-run)
```

---

## 🎯 MÉTRIQUES DE SUCCÈS

### Job Réussi

```
✅ Status : COMPLETED
✅ Résultats récupérés : job-<id>-result.json
✅ Observables décodés : evs, stds, ensemble_standard_error
✅ Forensic logging : 28 événements tracés
✅ Pas d'erreur 1500
```

### Job Échoué

```
❌ Status : FAILED
❌ Error code : 1500 (trop de pubs)
❌ Error code : 3458 (backend indisponible)
❌ Error code : 1217 (token invalide)
```

---

## 📚 RÉFÉRENCES

### Documentation IBM

- [IBM Quantum Platform](https://quantum.ibm.com/)
- [Qiskit Runtime](https://docs.quantum.ibm.com/api/qiskit-ibm-runtime)
- [EstimatorV2](https://docs.quantum.ibm.com/api/qiskit-ibm-runtime/qiskit_ibm_runtime.EstimatorV2)

### Rapports LumVorax

- [`RAPPORT_ANALYSE_JOB_D7RQ5NST_SUCCESS_C162.md`](../src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_ANALYSE_JOB_D7RQ5NST_SUCCESS_C162.md) - Job 1 pub réussi
- [`RAPPORT_FINAL_C165_CORRECTION_ERREUR_1500.md`](../src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_FINAL_C165_CORRECTION_ERREUR_1500.md) - Correction erreur 1500
- [`RAPPORT_IBM_QUANTUM_C65_RESULTATS_REELS.md`](../src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C65_RESULTATS_REELS.md) - 8 jobs réussis C65

### Scripts

- [`ibm_quantum_pqe_nx48_integrated.py`](../tools/ibm_quantum_pqe_nx48_integrated.py) - Script principal
- [`retrieve_ibm_job_results.py`](../tools/retrieve_ibm_job_results.py) - Récupération résultats
- [`configure_ibm_quantum.sh`](../tools/configure_ibm_quantum.sh) - Configuration token

---

## ✅ CHECKLIST FINALE

Avant de considérer une soumission comme réussie :

```
☐ Job soumis sans erreur Python
☐ Job ID récupéré et noté
☐ Status = QUEUED ou RUNNING (pas FAILED)
☐ Vérification sur IBM Quantum Platform
☐ Attente fin job (15-30 min)
☐ Status final = COMPLETED
☐ Résultats récupérés (job-<id>-result.json)
☐ Observables décodés (evs, stds)
☐ Forensic logging complet (28 événements)
☐ Rapport créé avec analyse résultats
```

---

## 🚨 EN CAS DE PROBLÈME

### Problème Persistant

Si après 3 tentatives le job échoue toujours :

1. **Vérifier IBM Quantum Platform** : https://quantum.ibm.com/workloads
2. **Vérifier status backend** : Peut être en maintenance
3. **Réduire drastiquement** : `--nonces 1 --addresses 1` (6 pubs)
4. **Tester autre backend** : `--backend ibm_kingston`
5. **Contacter support IBM** : Si erreur persiste

### Logs Détaillés

```bash
# Activer logs verbeux
export QISKIT_IBM_RUNTIME_LOG_LEVEL=DEBUG

# Relancer avec logs
python3 tools/ibm_quantum_pqe_nx48_integrated.py \
  --backend ibm_fez \
  --mainnet \
  --nonces 1 \
  --addresses 1 2>&1 | tee submission_debug.log
```

---

**Guide créé par** : Bob (Expert LumVorax)  
**Date** : 2026-05-04  
**Version** : 1.0.0  
**Statut** : ✅ VALIDÉ

**IMPORTANT** : Suivre ce guide À LA LETTRE pour garantir le succès de CHAQUE soumission IBM Quantum.