# LEÇON #79.1 - GESTION CLÉS API VIA DOPPLER (OBLIGATOIRE)

**Date**: 2026-06-16  
**Auteur**: Bob  
**Priorité**: 🔴 CRITIQUE  
**Statut**: RÈGLE OBLIGATOIRE

---

## 🚨 ERREUR CRITIQUE IDENTIFIÉE

### Problème Détecté

**MAUVAISE PRATIQUE ACTUELLE**:
```python
# ❌ INTERDIT - Clé API en dur dans le code
ARC_API_KEY = "0a6e34a4-5c5f-49bb-b03b-73d6537f260c"
os.environ["ARC_API_KEY"] = ARC_API_KEY
```

**CONSÉQUENCES**:
1. ❌ Clés exposées dans le code source
2. ❌ Clés exposées dans les commits Git
3. ❌ Clés exposées dans les logs
4. ❌ Impossible de changer les clés sans modifier le code
5. ❌ Risque de sécurité majeur

---

## ✅ SOLUTION OBLIGATOIRE: DOPPLER

### Principe

**Doppler** est un gestionnaire de secrets centralisé qui:
- ✅ Stocke les clés de manière sécurisée
- ✅ Permet de changer les clés sans modifier le code
- ✅ Synchronise automatiquement les clés entre environnements
- ✅ Fournit un audit trail complet
- ✅ Évite l'exposition des clés dans Git

### Architecture Correcte

```
┌─────────────────────────────────────────────────────────────┐
│                    DOPPLER (Cloud)                          │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Projet: MAGEN                                        │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  ARC_API_KEY = 705f499b-05a3-4e59-a248-...     │  │  │
│  │  │  KAGGLE_API_TOKEN = xxxxx                       │  │  │
│  │  │  IBM_QUANTUM_TOKEN = xxxxx                      │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
                    doppler run --
                            ↓
┌─────────────────────────────────────────────────────────────┐
│              Script Python (train_single_game.py)           │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  # ✅ CORRECT - Lecture depuis environnement          │  │
│  │  ARC_API_KEY = os.getenv("ARC_API_KEY")              │  │
│  │  if not ARC_API_KEY:                                  │  │
│  │      raise ValueError("ARC_API_KEY non configurée")   │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 PROTOCOLE OBLIGATOIRE

### Étape 1: Configuration Doppler (Une seule fois)

```bash
# Installer Doppler CLI
curl -Ls https://cli.doppler.com/install.sh | sh

# Se connecter
doppler login

# Configurer le projet
cd lumvorax2/src/MAGEN
doppler setup
# Sélectionner: Projet = MAGEN, Config = dev
```

### Étape 2: Mise à Jour des Clés dans Doppler

```bash
# Mettre à jour la clé ARC-AGI
doppler secrets set ARC_API_KEY="705f499b-05a3-4e59-a248-5c8717da7d92"

# Mettre à jour le scorecard (optionnel, pour référence)
doppler secrets set ARC_SCORECARD_ID="424a1e84-e63c-498c-a481-70647dcf1f3d"

# Vérifier
doppler secrets get ARC_API_KEY
```

### Étape 3: Modification du Code (OBLIGATOIRE)

**AVANT (❌ INTERDIT)**:
```python
# Configuration API - NOUVEAU COMPTE VALIDÉ ✅
ARC_API_KEY = "0a6e34a4-5c5f-49bb-b03b-73d6537f260c"
os.environ["ARC_API_KEY"] = ARC_API_KEY
```

**APRÈS (✅ CORRECT)**:
```python
# Configuration API - Lecture depuis Doppler
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError(
        "ARC_API_KEY non configurée. "
        "Exécutez: doppler run -- python script.py"
    )
```

### Étape 4: Exécution avec Doppler

```bash
# ✅ CORRECT - Exécution avec Doppler
doppler run -- python3 train_single_game_ls20_realtime.py

# ❌ INTERDIT - Exécution directe
python3 train_single_game_ls20_realtime.py
```

---

## 🔄 WORKFLOW COMPLET

### Scénario: Nouvelle Clé API Reçue

**Nouvelle clé**: `705f499b-05a3-4e59-a248-5c8717da7d92`

**Étapes**:

1. **Mettre à jour Doppler** (30 secondes):
   ```bash
   doppler secrets set ARC_API_KEY="705f499b-05a3-4e59-a248-5c8717da7d92"
   ```

2. **Vérifier la mise à jour**:
   ```bash
   doppler secrets get ARC_API_KEY
   # Output: 705f499b-05a3-4e59-a248-5c8717da7d92
   ```

3. **Relancer le script** (aucune modification de code):
   ```bash
   doppler run -- python3 train_single_game_ls20_realtime.py
   ```

**RÉSULTAT**: Le script utilise automatiquement la nouvelle clé, sans modification de code !

---

## 📊 COMPARAISON AVANT/APRÈS

| Aspect | AVANT (Clé en dur) | APRÈS (Doppler) |
|--------|-------------------|-----------------|
| **Sécurité** | ❌ Clé exposée dans Git | ✅ Clé sécurisée dans Doppler |
| **Changement clé** | ❌ Modifier code + commit | ✅ 1 commande Doppler |
| **Temps changement** | ❌ 5-10 minutes | ✅ 30 secondes |
| **Audit** | ❌ Aucun | ✅ Historique complet |
| **Environnements** | ❌ Clés dupliquées | ✅ Clés centralisées |
| **Risque exposition** | ❌ Élevé | ✅ Minimal |

---

## 🎯 RÈGLES OBLIGATOIRES

### Règle #1: Jamais de Clés en Dur

```python
# ❌ INTERDIT
API_KEY = "abc123..."
TOKEN = "xyz789..."

# ✅ CORRECT
API_KEY = os.getenv("API_KEY")
TOKEN = os.getenv("TOKEN")
```

### Règle #2: Toujours Vérifier la Présence

```python
# ✅ CORRECT - Vérification obligatoire
API_KEY = os.getenv("API_KEY")
if not API_KEY:
    raise ValueError("API_KEY non configurée dans Doppler")
```

### Règle #3: Exécution via Doppler

```bash
# ✅ CORRECT
doppler run -- python3 script.py

# ❌ INTERDIT (sauf si variables déjà exportées)
python3 script.py
```

### Règle #4: Documentation des Variables

```python
"""
Variables d'environnement requises (via Doppler):
- ARC_API_KEY: Clé API ARC-AGI-3
- KAGGLE_API_TOKEN: Token Kaggle (optionnel)
- IBM_QUANTUM_TOKEN: Token IBM Quantum (optionnel)

Exécution:
    doppler run -- python3 script.py
"""
```

---

## 🔧 COMMANDES UTILES

### Gestion des Secrets

```bash
# Lister tous les secrets
doppler secrets

# Obtenir une valeur
doppler secrets get ARC_API_KEY

# Mettre à jour
doppler secrets set ARC_API_KEY="nouvelle-cle"

# Supprimer
doppler secrets delete OLD_KEY

# Télécharger tous les secrets (backup)
doppler secrets download > secrets_backup.json
```

### Gestion des Environnements

```bash
# Lister les configs
doppler configs

# Changer de config
doppler setup --config prod

# Comparer deux configs
doppler secrets diff dev prod
```

### Intégration CI/CD

```bash
# Générer un token de service
doppler configs tokens create ci-token --config dev

# Utiliser dans CI/CD
export DOPPLER_TOKEN="dp.st.xxx"
doppler run -- python3 script.py
```

---

## 📝 CHECKLIST MIGRATION

Pour migrer un script existant vers Doppler:

- [ ] Identifier toutes les clés/tokens en dur
- [ ] Créer les secrets dans Doppler
- [ ] Remplacer les valeurs en dur par `os.getenv()`
- [ ] Ajouter vérifications de présence
- [ ] Tester avec `doppler run --`
- [ ] Supprimer les valeurs en dur du code
- [ ] Ajouter documentation variables requises
- [ ] Mettre à jour README avec instructions Doppler
- [ ] Vérifier que les clés ne sont pas dans Git history

---

## 🚨 CAS D'URGENCE

### Clé Compromise

```bash
# 1. Révoquer immédiatement dans le service (ARC-AGI, etc.)
# 2. Générer nouvelle clé
# 3. Mettre à jour Doppler
doppler secrets set ARC_API_KEY="nouvelle-cle-securisee"
# 4. Relancer les services
doppler run -- python3 script.py
```

**Temps total**: < 2 minutes

### Perte d'Accès Doppler

```bash
# Backup local (à faire régulièrement)
doppler secrets download > backup_$(date +%Y%m%d).json

# Restauration
cat backup_20260616.json | jq -r 'to_entries[] | "\(.key)=\(.value)"' | \
while IFS='=' read -r key value; do
    doppler secrets set "$key"="$value"
done
```

---

## 📚 RÉFÉRENCES

- **Doppler Documentation**: https://docs.doppler.com/
- **Best Practices**: https://docs.doppler.com/docs/best-practices
- **CLI Reference**: https://docs.doppler.com/docs/cli

---

## 🎓 RÉSUMÉ EXÉCUTIF

### Ce qu'il faut retenir

1. **JAMAIS** de clés API en dur dans le code
2. **TOUJOURS** utiliser Doppler pour les secrets
3. **TOUJOURS** exécuter avec `doppler run --`
4. **TOUJOURS** vérifier la présence des variables
5. Mettre à jour Doppler = 30 secondes, pas de modification code

### Impact

- ✅ Sécurité: Clés protégées
- ✅ Agilité: Changement clé en 30s
- ✅ Audit: Historique complet
- ✅ Scalabilité: Multi-environnements
- ✅ Conformité: Best practices respectées

---

**Créé par**: Bob  
**Date**: 2026-06-16  
**Version**: 1.0  
**Statut**: RÈGLE OBLIGATOIRE MAGEN

---

## NOUVELLE CLÉ API REÇUE

**Date**: 2026-06-16  
**Nouvelle clé ARC-AGI**: `705f499b-05a3-4e59-a248-5c8717da7d92`  
**Scorecard associé**: `424a1e84-e63c-498c-a481-70647dcf1f3d`

**Action immédiate**:
```bash
doppler secrets set ARC_API_KEY="705f499b-05a3-4e59-a248-5c8717da7d92"
doppler secrets set ARC_SCORECARD_ID="424a1e84-e63c-498c-a481-70647dcf1f3d"