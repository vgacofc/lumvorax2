## 🧠 MAGEN - Mémoire Artificielle GEN pour ARC-AGI-3

**Système de mémoire artificielle multi-échelle pour la compétition ARC-AGI-3**

[![Status](https://img.shields.io/badge/Status-Phase%20Locale-green)]()
[![Python](https://img.shields.io/badge/Python-3.12-blue)]()
[![Forensic](https://img.shields.io/badge/Forensic-LumVorax-red)]()

---

## 📋 Table des Matières

- [Vue d'ensemble](#vue-densemble)
- [Architecture](#architecture)
- [Installation](#installation)
- [Utilisation](#utilisation)
- [Tests](#tests)
- [Logs Forensiques](#logs-forensiques)
- [Configuration Doppler](#configuration-doppler)
- [Résultats](#résultats)
- [Roadmap](#roadmap)

---

## 🎯 Vue d'ensemble

MAGEN est un système de **mémoire artificielle multi-échelle** conçu pour résoudre les puzzles ARC-AGI-3. Il combine:

- **Mémoire 4 niveaux**: Court terme, moyen terme, long terme, archive profonde
- **Perception avancée**: Features spatiales, patterns, transformations
- **Agent exploration**: Stratégie exploration/exploitation adaptative
- **Logs forensiques**: Traçabilité bit-level nanoseconde avec LumVorax

### Compétition ARC-AGI-3

- **Nom**: `arc-prize-2026-arc-agi-3`
- **Dataset**: 25 jeux publics + 110 jeux privés
- **Prix**: $850K ($150K progress + $700K bonus)
- **Scoring**: `(human_actions/agent_actions)²` par niveau

---

## 🏗️ Architecture

```
MAGEN/
├── core/
│   └── magen_memory.py          # Mémoire multi-échelle (521 lignes)
├── perception/
│   └── arc_perception.py        # Perception grilles (408 lignes)
├── agent/
│   └── arc_agent.py             # Agent exploration (258 lignes)
├── forensic/
│   └── lumvorax_logger.py       # Logs forensiques (458 lignes)
├── adapters/
│   └── arc_agi_adapter.py       # Adaptateur ARC-AGI-3 (310 lignes)
├── tests/
│   ├── test_magen_integration_local.py      # Tests simulés ✅
│   └── test_magen_real_arc_games.py         # Tests réels (520 lignes)
├── scripts/
│   ├── download_arc_dataset.py              # Téléchargement dataset
│   ├── install_arc_dependencies.sh          # Installation wheels
│   ├── run_magen_local_tests.sh             # Exécution tests
│   └── configure_doppler.sh                 # Configuration secrets
├── data/
│   └── arc_agi_3/                           # Dataset officiel (42.3 MB)
│       ├── environment_files/               # 25 jeux
│       ├── ARC-AGI-3-Agents/                # Code référence
│       └── arc_agi_3_wheels/                # Dépendances
└── logs/
    ├── forensic/                            # Logs LumVorax (.lum + .json)
    ├── results/                             # Résultats JSON
    └── magen/                               # Logs mémoire
```

### Composants Clés

#### 1. Mémoire Multi-Échelle (`magen_memory.py`)

```python
class MAGENMemory:
    - short_term: 50 expériences récentes
    - medium_term: 200 patterns extraits
    - long_term: Expériences success > 0.7
    - deep_archive: Toutes expériences
    
    Méthodes:
    - inject_experience()      # Ajouter expérience
    - consolidate_memory()     # Consolider patterns
    - retrieve_similar_pattern()  # Récupérer similaires
    - build_causal_graph()     # Graphe causal
```

#### 2. Perception ARC (`arc_perception.py`)

```python
class ARCPerception:
    Features extraites:
    - Spatiales: densité, symétrie, entropie
    - Patterns: bordures, clusters, rotations
    - Transformations: flips, translations
    
    Méthodes:
    - extract_features()       # Extraction complète
    - compute_similarity()     # Similarité grilles
    - detect_transformation()  # Détection transformations
```

#### 3. Agent Exploration (`arc_agent.py`)

```python
class ARCAgent:
    Stratégie: Exploration/Exploitation
    - exploration_rate: 0.3 (défaut)
    - Actions: RESET, ACTION1-7
    
    Méthodes:
    - solve_level()            # Résoudre niveau
    - choose_action()          # Choisir action
```

#### 4. Logs Forensiques (`lumvorax_logger.py`)

```python
class LumVoraxLogger:
    - Timestamps nanoseconde
    - Signatures HMAC-SHA256
    - Format binaire .lum
    - Export JSON parallèle
    
    Méthodes:
    - log_event()              # Logger événement
    - validate_integrity()     # Valider signatures
    - detect_anomalies()       # Détecter anomalies
```

---

## 🚀 Installation

### Prérequis

- Python 3.12+
- Git
- Doppler CLI (optionnel, pour secrets)

### Étape 1: Cloner et Setup

```bash
cd lumvorax2/src/MAGEN

# Créer environnement virtuel
python3 -m venv venv
source venv/bin/activate

# Installer dépendances de base
pip install -r requirements.txt
```

### Étape 2: Télécharger Dataset ARC-AGI-3

```bash
# Configurer token Kaggle
export KAGGLE_API_TOKEN="KGAT_9002d163e9b18247583c6c6a699d1115"

# Télécharger dataset (42.3 MB)
python scripts/download_arc_dataset.py
```

**Résultat**: Dataset dans `data/arc_agi_3/` avec 25 jeux publics

### Étape 3: Installer Dépendances ARC-AGI-3

```bash
# Installer wheels arc_agi et arcengine
bash scripts/install_arc_dependencies.sh
```

**Vérification**:
```bash
python -c "from arc_agi import EnvironmentWrapper; print('✅ OK')"
```

---

## 💻 Utilisation

### Tests Simulés (Déjà Validés ✅)

```bash
# Test d'intégration complet
python tests/test_magen_integration_local.py

# Test rapide
python tests/test_magen_integration_local.py --quick

# Test perception uniquement
python tests/test_magen_integration_local.py --perception
```

**Résultats**: 6/6 niveaux réussis, 100% success rate

### Tests avec Jeux Réels ARC-AGI-3

```bash
# Test sur 3 jeux (défaut)
bash scripts/run_magen_local_tests.sh

# Test sur 5 jeux
bash scripts/run_magen_local_tests.sh --games 5

# Test jeux spécifiques
bash scripts/run_magen_local_tests.sh --game-ids ar25 bp35 cd82

# Mode forensique complet
bash scripts/run_magen_local_tests.sh --games 5 --forensic-full

# Mode debug
bash scripts/run_magen_local_tests.sh --games 3 --debug
```

### Exécution Directe Python

```bash
# Activer venv
source venv/bin/activate

# Test 3 jeux
python tests/test_magen_real_arc_games.py --games 3

# Test avec forensique complet
python tests/test_magen_real_arc_games.py --games 5 --forensic-full

# Jeux spécifiques
python tests/test_magen_real_arc_games.py --game-ids ar25 bp35 cd82 cn04 dc22
```

---

## 🧪 Tests

### Tests Disponibles

| Test | Description | Statut |
|------|-------------|--------|
| `test_magen_integration_local.py` | Tests simulés complets | ✅ Validé |
| `test_magen_real_arc_games.py` | Tests jeux réels ARC-AGI-3 | 🔄 À exécuter |

### Métriques Collectées

- **Performance**: Actions/niveau, temps, efficacité vs baseline
- **Mémoire**: Usage MB, patterns extraits, liens causaux
- **Forensique**: Événements, signatures, anomalies
- **Score**: `(baseline/actions)²` par niveau

### Résultats Tests Simulés

```
✅ Composants: Tous initialisés
✅ Perception: 3 grilles testées
✅ Résolution: 6/6 niveaux (100%)
   - Actions totales: 38
   - Actions/niveau: 6.3 moyenne
   - Temps: 0.17s

📊 Mémoire:
   - Court terme: 20 expériences
   - Moyen terme: 3 patterns
   - Archive: 56 expériences
   - Usage: 2.71 MB
   - Liens causaux: 35
   - Contradictions: 25

🔐 Forensique:
   - Événements: 6
   - Bytes: 1,666
   - Anomalies: 0
   - Signatures: 100% validées
```

---

## 🔐 Logs Forensiques

### Format LumVorax

Chaque événement est tracé avec:

- **Timestamp nanoseconde**: `time.time_ns()`
- **Signature HMAC-SHA256**: Intégrité cryptographique
- **Format binaire**: `.lum` (magic 0x4D444241)
- **Export JSON**: Parallèle pour analyse

### Structure Logs

```
logs/
├── forensic/
│   ├── magen_arc_ar25_20260612_015000.lum      # Binaire
│   ├── magen_arc_ar25_20260612_015000.json     # JSON
│   ├── magen_arc_bp35_20260612_015100.lum
│   └── magen_arc_bp35_20260612_015100.json
├── results/
│   └── magen_arc_results_20260612_015200.json  # Résultats globaux
└── magen_arc_test.log                          # Logs exécution
```

### Événements Tracés

1. **game_start**: Début jeu
2. **level_start**: Début niveau
3. **perception**: Features extraites
4. **action**: Action choisie
5. **level_end**: Fin niveau
6. **game_end**: Fin jeu

### Analyse Logs

```bash
# Lire logs JSON
cat logs/forensic/magen_arc_ar25_*.json | jq .

# Valider signatures
python -c "
from forensic.lumvorax_logger import LumVoraxLogger
logger = LumVoraxLogger.load_from_file('logs/forensic/magen_arc_ar25_*.lum')
print(f'Signatures valides: {logger.validate_integrity()}')
"

# Détecter anomalies
python -c "
from forensic.lumvorax_logger import LumVoraxLogger
logger = LumVoraxLogger.load_from_file('logs/forensic/magen_arc_ar25_*.lum')
anomalies = logger.detect_anomalies()
print(f'Anomalies: {len(anomalies)}')
"
```

---

## 🔑 Configuration Doppler

### Installation Doppler

```bash
# Debian/Ubuntu
curl -sLf --retry 3 --tlsv1.2 --proto '=https' \
  'https://packages.doppler.com/public/cli/gpg.DE2A7741A397C129.key' | \
  sudo gpg --dearmor -o /usr/share/keyrings/doppler-archive-keyring.gpg

echo 'deb [signed-by=/usr/share/keyrings/doppler-archive-keyring.gpg] \
  https://packages.doppler.com/public/cli/deb/debian any-version main' | \
  sudo tee /etc/apt/sources.list.d/doppler-cli.list

sudo apt-get update && sudo apt-get install doppler

# Authentification
doppler login
```

### Configuration Secrets

```bash
# Configurer projet MAGEN
bash scripts/configure_doppler.sh
```

**Secrets configurés**:
- `KAGGLE_API_TOKEN`: Token Kaggle
- `LUMVORAX_SECRET_KEY`: Clé forensique
- `MAGEN_ENCRYPTION_KEY`: Clé chiffrement

### Utilisation avec Doppler

```bash
# Exécuter avec secrets local
doppler run --project magen --config local -- \
  python tests/test_magen_real_arc_games.py --games 3

# Exécuter avec secrets kaggle
doppler run --project magen --config kaggle -- \
  python submit_to_kaggle.py

# Exporter secrets
doppler secrets download --project magen --config local --format env > .env.local
```

---

## 📊 Résultats

### Format Résultats

```json
{
  "game_id": "ar25",
  "baseline_actions": [32, 50, 75, 37, 89, 159, 233, 73],
  "levels_completed": 8,
  "total_actions": 450,
  "efficiency_ratio": 0.85,
  "score": 0.7225,
  "duration_seconds": 12.5,
  "forensic_events": 156,
  "levels": [
    {
      "level": 0,
      "baseline_actions": 32,
      "actions_taken": 28,
      "success": true
    }
  ]
}
```

### Métriques Clés

- **Efficiency Ratio**: `baseline_total / magen_total`
- **Score**: `(baseline_total / magen_total)²`
- **Success Rate**: `levels_completed / total_levels`

---

## 🗺️ Roadmap

### Phase 1: Implémentation Locale ✅

- [x] Système mémoire multi-échelle
- [x] Perception ARC-AGI-3
- [x] Agent exploration
- [x] Logs forensiques LumVorax
- [x] Tests simulés (100% success)
- [x] Dataset téléchargé (25 jeux)
- [x] Adaptateur ARC-AGI-3
- [x] Scripts installation/exécution

### Phase 2: Tests Réels (En Cours)

- [ ] Installation wheels arc_agi
- [ ] Tests sur 5 jeux publics
- [ ] Analyse logs forensiques complets
- [ ] Optimisation mémoire
- [ ] Configuration Doppler
- [ ] Validation utilisateur

### Phase 3: Optimisation (À Venir)

- [ ] Consolidation adaptative
- [ ] Patterns hiérarchiques
- [ ] Oubli sélectif
- [ ] Cache features
- [ ] Batch processing

### Phase 4: Production (Après Validation)

- [ ] Préparation notebook Kaggle
- [ ] Tests environnement Kaggle
- [ ] Soumission compétition
- [ ] Monitoring performance

---

## 📝 Notes Importantes

### ⚠️ CONTRAINTE CRITIQUE

**TOUT RESTE EN LOCAL** jusqu'à validation explicite de l'utilisateur.

- ❌ Aucune soumission Kaggle automatique
- ❌ Aucun upload sans autorisation
- ✅ Tests locaux uniquement
- ✅ Validation manuelle requise

### 🎯 Objectifs Phase Locale

1. **Valider système MAGEN**: Tests réels sur 5+ jeux
2. **Logs forensiques**: Traçabilité bit-level complète
3. **Optimisation**: Mémoire < 100 MB, consolidation < 1ms
4. **Performance**: > 50% vs baseline humain

### 📞 Support

Pour questions ou problèmes:

1. Consulter logs: `logs/magen_arc_test.log`
2. Vérifier forensique: `logs/forensic/*.json`
3. Analyser résultats: `logs/results/*.json`

---

## 📄 Licence

Projet MAGEN - LumVorax Technology  
© 2026 - Tous droits réservés

---

**Dernière mise à jour**: 2026-06-12 01:55 UTC  
**Version**: 1.0.0-local  
**Statut**: Phase locale - Prêt pour tests réels