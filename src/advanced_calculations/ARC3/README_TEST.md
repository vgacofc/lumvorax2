# 🧪 TEST AGENT LUMVORAX - ARC-AGI-3

## 📋 Vue d'ensemble

Ce répertoire contient les tests pour l'agent **LumVorax** sur la compétition ARC-AGI-3.

## 🚀 Installation

### Prérequis

```bash
# Python 3.12+
python --version

# Dépendances ARC-AGI-3
cd arc-prize-2026-arc-agi-3/ARC-AGI-3-Agents
pip install -r requirements.txt

# Vérifier installation
python -c "from agents.templates.lumvorax_agent import LumVoraxAgent; print('✅ OK')"
```

### Variables d'environnement

```bash
# Créer .env dans ARC-AGI-3-Agents/
cd arc-prize-2026-arc-agi-3/ARC-AGI-3-Agents
cp .env.example .env

# Éditer .env avec vos clés API
# OPENAI_API_KEY=sk-...
# ARC_API_KEY=...
```

## 🧪 Exécuter les tests

### Test basique ar25

```bash
cd /home/lvx/LVX/lumvorax2/src/advanced_calculations/ARC3
python test_lumvorax_ar25.py
```

**Sortie attendue** :
```
🌟 TEST AGENT LUMVORAX - JEU AR25 🌟
✅ Imports réussis
✅ Environnement créé
✅ Agent créé
🚀 Lancement test (max 10 actions)...
...
✅ TEST RÉUSSI - Agent LumVorax opérationnel sur ar25
```

### Résultats

Les résultats sont sauvegardés dans `test_results_ar25.json` :

```json
{
  "test_date": "2026-05-04 19:50:00",
  "game_id": "ar25",
  "metrics": {
    "decisions": 10,
    "quantum_recognitions": 10,
    "stdp_spikes": 3,
    "strategy_switches": 2
  },
  "nx48_stats": {...},
  "poh_valid": true,
  "forensic_summary": {...}
}
```

## 📊 Métriques trackées

### NX48 (Contrôleur Adaptatif)
- **Stratégies** : exploration, exploitation, pattern_matching, hypothesis_testing, systematic_scan
- **Scores** : Performance de chaque stratégie
- **Epsilon** : Taux d'exploration (decay automatique)

### NX49 (Neurone Biologique)
- **Spikes** : Nombre d'activations neuronales
- **STDP** : Apprentissage Hebbien (LTP/LTD)
- **Poids synaptiques** : Adaptation dynamique

### Quantum Pattern Recognition
- **Patterns détectés** : Nombre de patterns reconnus
- **Confiance** : Probabilité de reconnaissance
- **Bibliothèque** : Patterns appris

### POH (Proof of History)
- **Séquence** : Nombre de décisions enregistrées
- **Intégrité** : Vérification chaîne SHA-256
- **Hash** : Traçabilité cryptographique

### Forensic Logger
- **Events** : Nombre total d'événements
- **Catégories** : INIT, DECISION, NX48, NX49, QUANTUM, ACTION, ERROR
- **Performance** : Events/sec

## 🔧 Configuration

### Configuration par défaut

```python
config = LumVoraxConfig(
    nx48_learning_rate=0.1,
    nx48_adaptation_threshold=0.3,
    nx49_stdp_tau_plus=20.0,
    nx49_stdp_tau_minus=20.0,
    quantum_superposition_depth=3,
    forensic_log_level="BIT_LEVEL"
)
```

### Configuration personnalisée

Modifier `test_lumvorax_ar25.py` ligne 50-65 :

```python
config = LumVoraxConfig(
    nx48_learning_rate=0.15,        # Plus agressif
    nx49_stdp_tau_plus=15.0,        # STDP plus rapide
    quantum_superposition_depth=5,  # Plus de profondeur
)
```

## 🐛 Debugging

### Logs forensiques

```python
# Accéder aux logs
for log in agent.forensic.logs:
    print(f"[{log['category']}] {log['event']}: {log['data']}")
```

### Vérification POH

```python
# Vérifier intégrité
if not agent.poh.verify_chain():
    print("⚠️ Chaîne POH corrompue!")
```

### Statistiques NX48

```python
stats = agent.nx48.get_strategy_stats()
for strategy, data in stats['strategies'].items():
    print(f"{strategy}: score={data['score']:.3f}")
```

## 📚 Documentation

- **Agent complet** : [`lumvorax_agent.py`](arc-prize-2026-arc-agi-3/ARC-AGI-3-Agents/agents/templates/lumvorax_agent.py)
- **Rapport intégration** : [`RAPPORT_INTEGRATION_LUMVORAX_ARC3.md`](RAPPORTS/RAPPORT_INTEGRATION_LUMVORAX_ARC3.md)
- **Checklist** : [`CHECKLIST_ARC3_C174.md`](CHECKLIST_ARC3_C174.md)

## 🎯 Prochaines étapes

1. ✅ Test basique ar25
2. [ ] Tests sur 5-10 jeux variés
3. [ ] Optimisation hyperparamètres
4. [ ] Benchmarking vs baseline
5. [ ] Soumission compétition

## 🌟 Technologies intégrées

- **NX48** : Adaptation stratégique temps réel
- **NX49** : Apprentissage neuronal biologique (STDP)
- **Quantum** : Reconnaissance patterns en superposition
- **POH** : Traçabilité cryptographique (SHA-256)
- **Forensic** : Logging bit-level complet

---

**Made with ❤️ by LumVorax Team**