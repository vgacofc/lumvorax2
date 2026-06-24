# MAGEN V2 - Système d'Optimisation des Hyperparamètres

## 📋 Vue d'Ensemble

Système complet d'optimisation des hyperparamètres pour MAGEN V2, implémentant:
- **Grid Search**: Recherche exhaustive dans une grille de paramètres
- **Bayesian Optimization**: Recherche guidée par modèle probabiliste
- **Environnement de test**: Labyrinthe simplifié pour validation rapide

**Objectif**: Améliorer le taux de victoire de 2% (baseline) à 20%+

## 🎯 Résultats Baseline

**Validation Phase 3.4** (ls20-9607627b):
- **Victoires**: 1/50 (2.0%)
- **Reward moyen**: 28.50
- **Steps moyen**: 89
- **Temps**: 0.001s/épisode

## 🚀 Utilisation

### Test Rapide (50 épisodes)

```bash
cd lumvorax2/src/MAGEN
python3 optimization/run_hyperparameter_optimization.py --method test --episodes 50
```

**Sortie attendue**:
```
============================================================
MAGEN V2 - Quick Test
============================================================
Game: ls20-9607627b
Episodes: 50
============================================================

Victoires: X/50 (X.XX%)
Reward Moyen: XX.XX
Steps Moyen: XXX.X
Temps d'Entraînement: X.XXs
```

### Grid Search (recherche exhaustive)

```bash
python3 optimization/run_hyperparameter_optimization.py \
    --method grid \
    --episodes 100 \
    --game ls20-9607627b \
    --save-dir results/grid_search
```

**Paramètres testés**:
- `buffer_capacity`: [5000, 10000]
- `batch_size`: [16, 32]
- `learning_rate`: [0.001, 0.01]

**Total configurations**: 2 × 2 × 2 = 8

**Temps estimé**: ~5-10 minutes (8 configs × 100 épisodes)

### Bayesian Optimization (recherche guidée)

```bash
python3 optimization/run_hyperparameter_optimization.py \
    --method bayesian \
    --iterations 20 \
    --episodes 50 \
    --game ls20-9607627b \
    --save-dir results/bayesian
```

**Paramètres optimisés**:
- `buffer_capacity`: [1000, 20000]
- `batch_size`: [8, 64]
- `learning_rate`: [0.0001, 0.01]

**Phases**:
1. **Exploration** (5 itérations): Échantillonnage aléatoire
2. **Exploitation** (15 itérations): Recherche guidée par GP

**Temps estimé**: ~10-15 minutes (20 configs × 50 épisodes)

## 📊 Résultats

Les résultats sont sauvegardés automatiquement en JSON:

```
results/
├── grid_search/
│   └── grid_search_ls20-9607627b_1234567890.json
└── bayesian/
    └── bayesian_ls20-9607627b_1234567890.json
```

**Format JSON**:
```json
{
  "results": [
    {
      "config": {
        "buffer_capacity": 10000,
        "batch_size": 32,
        "learning_rate": 0.001,
        ...
      },
      "win_rate": 0.02,
      "avg_reward": 28.5,
      "avg_steps": 89.0,
      "episodes": 50,
      "training_time": 3.11
    }
  ],
  "best_config": {...},
  "timestamp": 1234567890
}
```

## 🔧 Configuration Avancée

### Personnaliser la grille de paramètres

Éditer `hyperparameter_tuner.py`:

```python
def create_default_param_grid() -> Dict[str, List[Any]]:
    return {
        'learning_rate': [0.0001, 0.001, 0.01, 0.1],
        'batch_size': [8, 16, 32, 64, 128],
        'gamma': [0.9, 0.95, 0.99, 0.995],
        'epsilon_decay': [0.99, 0.995, 0.999, 0.9999],
        'buffer_capacity': [1000, 5000, 10000, 20000, 50000]
    }
```

### Personnaliser les bornes Bayesian

```python
def create_default_param_bounds() -> Dict[str, Tuple[float, float]]:
    return {
        'learning_rate': (0.00001, 0.1),
        'batch_size': (4, 256),
        'gamma': (0.8, 0.9999),
        'epsilon_decay': (0.9, 0.99999),
        'buffer_capacity': (100, 100000),
        'tau': (0.0001, 0.1),
        'reward_scale': (0.01, 100.0)
    }
```

## 📈 Métriques Suivies

### Par Configuration
- **Win Rate**: Taux de victoire (0-1)
- **Avg Reward**: Récompense moyenne par épisode
- **Avg Steps**: Nombre moyen de steps par épisode
- **Training Time**: Temps d'entraînement total
- **Convergence Episode**: Premier épisode avec victoire

### Globales
- **Best Config**: Meilleure configuration trouvée
- **Best Win Rate**: Meilleur taux de victoire
- **Total Configs Tested**: Nombre de configurations testées
- **Total Time**: Temps total d'optimisation

## 🧪 Tests Unitaires

```bash
# Test du système d'optimisation
cd lumvorax2/src/MAGEN
python3 -m pytest tests/test_hyperparameter_optimization.py -v

# Test des composants individuels
python3 optimization/hyperparameter_tuner.py
```

**Sortie attendue**:
```
MAGEN V2 - Hyperparameter Optimization System
============================================================

Grid Search: 4 configurations générées
Bayesian Optimization: Configuration aléatoire générée
Learning Rate: 0.00XX
Batch Size: XX

✅ Tests réussis!
```

## 🎓 Références Scientifiques

### Grid Search
- **Bergstra & Bengio (2012)**: "Random Search for Hyper-Parameter Optimization"
  - Journal of Machine Learning Research
  - Montre que random search > grid search pour haute dimensionnalité

### Bayesian Optimization
- **Snoek et al. (2012)**: "Practical Bayesian Optimization of Machine Learning Algorithms"
  - NIPS 2012
  - Utilise Gaussian Process pour modéliser la fonction objectif

- **Shahriari et al. (2016)**: "Taking the Human Out of the Loop: A Review of Bayesian Optimization"
  - Proceedings of the IEEE
  - Revue complète des méthodes d'optimisation Bayésienne

### Acquisition Functions
- **Expected Improvement (EI)**: Balance exploration/exploitation
- **Upper Confidence Bound (UCB)**: Favorise l'exploration
- **Probability of Improvement (POI)**: Favorise l'exploitation

## 🔬 Architecture Technique

### Composants

1. **HyperparameterConfig**: Dataclass pour configuration
2. **OptimizationResult**: Dataclass pour résultats
3. **GridSearchOptimizer**: Recherche exhaustive
4. **BayesianOptimizer**: Recherche guidée par GP
5. **SimpleMazeEnvironment**: Environnement de test

### Pipeline d'Optimisation

```
┌─────────────────────────────────────────────────────────┐
│                  Hyperparameter Tuner                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐      ┌──────────────┐               │
│  │ Grid Search  │      │  Bayesian    │               │
│  │              │      │ Optimization │               │
│  └──────┬───────┘      └──────┬───────┘               │
│         │                     │                        │
│         └──────────┬──────────┘                        │
│                    │                                   │
│         ┌──────────▼──────────┐                        │
│         │  evaluate_config()  │                        │
│         └──────────┬──────────┘                        │
│                    │                                   │
│         ┌──────────▼──────────┐                        │
│         │  MAGEN Agent +      │                        │
│         │  Learning Cycle     │                        │
│         └──────────┬──────────┘                        │
│                    │                                   │
│         ┌──────────▼──────────┐                        │
│         │ SimpleMazeEnvironment│                       │
│         └──────────┬──────────┘                        │
│                    │                                   │
│         ┌──────────▼──────────┐                        │
│         │ OptimizationResult  │                        │
│         └─────────────────────┘                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## 📝 Notes Importantes

### Limitations Actuelles

1. **Environnement Simplifié**: Utilise `SimpleMazeEnvironment` au lieu du vrai jeu ARC
2. **Apprentissage Placeholder**: Le vrai apprentissage n'est pas encore implémenté
3. **Métriques Simulées**: Les victoires sont basées sur distance, pas sur résolution réelle

### Prochaines Étapes (Phase 4.2+)

1. **Intégration ARC Réel**: Utiliser le vrai loader ARC
2. **Apprentissage Complet**: Implémenter le vrai cycle d'apprentissage
3. **Multi-Jeux**: Tester sur 5+ jeux différents
4. **Transfer Learning**: Réutiliser connaissances entre jeux
5. **Architecture Search**: Optimiser la structure des layers

## 🎯 Objectifs Phase 4

| Phase | Objectif | Statut |
|-------|----------|--------|
| 4.1 | Hyperparameter Optimization | ✅ 95% |
| 4.2 | Entraînement Intensif 1000+ épisodes | ⏳ 0% |
| 4.3 | Transfer Learning Multi-Jeux | ⏳ 0% |
| 4.4 | Architecture Search NAS | ⏳ 0% |
| 4.5 | Validation Étendue 10+ jeux | ⏳ 0% |
| 4.6 | Benchmark vs Baselines | ⏳ 0% |

**Objectif Global**: 2% → 20%+ taux de victoire

## 📞 Support

Pour questions ou problèmes:
- **Auteur**: Bob (Mode Code)
- **Date**: 2026-06-19
- **Protocole**: Claude Pilot + LumVorax
- **Version**: MAGEN V2 - Phase 4.1

---

**Status**: ✅ Phase 4.1 Complétée (95%)
**Prochaine Étape**: Phase 4.2 - Entraînement Intensif