# PLAN OPTIMISATIONS V38.3 - BASÉ SUR ANALYSE FORENSIC RÉELLE

**Date**: 2026-06-15  
**Base**: Analyse logs réels V38.2  
**Objectif**: Passer de puzzles synthétiques triviaux à 43.5% sur ARC-AGI réel

---

## 🎯 OBJECTIFS MESURABLES

### Métriques Cibles V38.3

| Métrique | V38.2 (Synthétique) | V38.3 (Cible Réelle) | Écart |
|----------|---------------------|----------------------|-------|
| Taux succès ARC-AGI | 100% (10 puzzles) | 43.5% (100 puzzles) | -56.5% |
| Réduction affordance | 0% | 30-50% | +30-50% |
| Diversité actions | 1 action/puzzle | 3-5 actions/puzzle | +200-400% |
| Épuisements budget | 0% | 5-10% | +5-10% |
| Abandons stagnation | 0 | 10-15 | +10-15 |
| Régions world model | 0 | 5-10/puzzle | +5-10 |

---

## 📋 PHASES D'OPTIMISATION

### PHASE 1: Infrastructure Tests Réels (PRIORITÉ CRITIQUE)

**Objectif**: Remplacer puzzles synthétiques par dataset ARC-AGI officiel

**Actions**:

1. **Télécharger dataset ARC-AGI officiel**
   ```bash
   cd lumvorax2/data/
   wget https://github.com/fchollet/ARC-AGI/archive/refs/heads/master.zip
   unzip master.zip
   mv ARC-AGI-master/data/ arc_agi_official/
   ```

2. **Créer loader dataset officiel**
   ```python
   # lumvorax2/src/MAGEN/utils/arc_dataset_loader.py
   import json
   from pathlib import Path
   from typing import List, Dict, Tuple
   
   class ARCDatasetLoader:
       def __init__(self, data_dir: str = "data/arc_agi_official"):
           self.data_dir = Path(data_dir)
           self.training_dir = self.data_dir / "training"
           self.evaluation_dir = self.data_dir / "evaluation"
       
       def load_training_puzzles(self, limit: int = 100) -> List[Dict]:
           """Charge puzzles training du dataset officiel"""
           puzzles = []
           for json_file in self.training_dir.glob("*.json"):
               with open(json_file) as f:
                   puzzle = json.load(f)
                   puzzle['id'] = json_file.stem
                   puzzles.append(puzzle)
               if len(puzzles) >= limit:
                   break
           return puzzles
       
       def load_evaluation_puzzles(self, limit: int = 100) -> List[Dict]:
           """Charge puzzles evaluation du dataset officiel"""
           puzzles = []
           for json_file in self.evaluation_dir.glob("*.json"):
               with open(json_file) as f:
                   puzzle = json.load(f)
                   puzzle['id'] = json_file.stem
                   puzzles.append(puzzle)
               if len(puzzles) >= limit:
                   break
           return puzzles
   ```

3. **Modifier test_unified_arc_arcade.py**
   ```python
   # Remplacer _generate_arc_puzzle() par:
   def _run_arc_puzzles(self):
       from MAGEN.utils.arc_dataset_loader import ARCDatasetLoader
       
       loader = ARCDatasetLoader()
       puzzles = loader.load_training_puzzles(limit=100)
       
       for puzzle in puzzles:
           result = self.solver.solve(
               train_examples=puzzle['train'],
               test_input=puzzle['test'][0]['input']
           )
           # Vérifier contre test[0]['output']
   ```

**Livrables**:
- [ ] `arc_dataset_loader.py` (150 lignes)
- [ ] `test_unified_arc_arcade.py` modifié
- [ ] Tests sur 100 puzzles ARC-AGI réels
- [ ] Rapport métriques baseline réelles

**Durée estimée**: 2h

---

### PHASE 2: Recalibrage Affordance Discovery

**Objectif**: Obtenir 30-50% de réduction d'espace d'exploration

**Problème identifié**: Réduction 0% sur tous les filtrages (seuils trop permissifs)

**Actions**:

1. **Ajuster seuils de plausibilité**
   ```python
   # Dans affordance_discovery_engine.py
   class AffordanceDiscoveryEngine:
       def __init__(self):
           # AVANT: self.min_plausibility = 0.1
           self.min_plausibility = 0.3  # Plus strict
           
           # AVANT: self.min_coherence = 0.3
           self.min_coherence = 0.5     # Plus strict
           
           # NOUVEAU: Seuil de diversité
           self.min_diversity_score = 0.4
   ```

2. **Ajouter filtrage par diversité**
   ```python
   def _compute_diversity_score(self, action: str, recent_actions: List[str]) -> float:
       """Pénalise actions trop fréquentes récemment"""
       if not recent_actions:
           return 1.0
       
       count = recent_actions[-10:].count(action)
       return 1.0 / (1.0 + count)
   
   def filter_candidates(self, candidates: List[Action]) -> List[Action]:
       filtered = []
       for action in candidates:
           diversity = self._compute_diversity_score(action.name, self.recent_actions)
           
           if (action.plausibility >= self.min_plausibility and
               action.coherence >= self.min_coherence and
               diversity >= self.min_diversity_score):
               filtered.append(action)
       
       return filtered
   ```

3. **Tests de validation**
   ```python
   def test_affordance_filtering_effectiveness():
       engine = AffordanceDiscoveryEngine()
       
       # Générer 100 candidats
       candidates = generate_diverse_candidates(100)
       
       # Filtrer
       filtered = engine.filter_candidates(candidates)
       
       # Vérifier réduction 30-50%
       reduction = 1.0 - (len(filtered) / len(candidates))
       assert 0.3 <= reduction <= 0.5, f"Réduction {reduction:.1%} hors cible"
   ```

**Livrables**:
- [ ] `affordance_discovery_engine.py` modifié
- [ ] Tests unitaires filtrage
- [ ] Métriques réduction sur 100 puzzles réels

**Durée estimée**: 1h

---

### PHASE 3: Correction Stagnation Detector

**Objectif**: Réduire faux positifs et activer abandons

**Problème identifié**: 96% détections, 0 abandons (hypersensible)

**Actions**:

1. **Recalibrer seuils de variance**
   ```python
   # Dans trajectory_stagnation_detector.py
   class TrajectoryStagnationDetector:
       def __init__(self):
           # AVANT: self.variance_threshold = 0.01
           self.variance_threshold = 0.001  # 10x plus strict
           
           # AVANT: self.loop_ratio_threshold = 0.7
           self.loop_ratio_threshold = 0.85  # Plus tolérant
           
           # NOUVEAU: Activer abandons
           self.enable_abandonment = True
           self.max_stagnation_steps = 5
   ```

2. **Implémenter mécanisme d'abandon**
   ```python
   def should_abandon(self, trajectory_id: str) -> bool:
       """Décide si une trajectoire doit être abandonnée"""
       if not self.enable_abandonment:
           return False
       
       stagnation_count = self.stagnation_counts.get(trajectory_id, 0)
       
       if stagnation_count >= self.max_stagnation_steps:
           self._log_abandonment(trajectory_id, stagnation_count)
           return True
       
       return False
   
   def _log_abandonment(self, trajectory_id: str, count: int):
       self.logger.log_event(
           event="trajectory_abandoned",
           component="stagnation_detector",
           data={
               "trajectory_id": trajectory_id,
               "stagnation_steps": count,
               "reason": "max_stagnation_exceeded"
           }
       )
   ```

3. **Intégrer dans solver**
   ```python
   # Dans arc_solver_adapter.py
   def _exploration_step(self, state):
       # Vérifier stagnation
       is_stagnant = self.stagnation_detector.detect_stagnation(
           trajectory_id=state.trajectory_id,
           state=state
       )
       
       # Abandonner si nécessaire
       if self.stagnation_detector.should_abandon(state.trajectory_id):
           return None  # Signale abandon
       
       # Continuer exploration
       return self._select_next_action(state)
   ```

**Livrables**:
- [ ] `trajectory_stagnation_detector.py` modifié
- [ ] Intégration abandons dans solver
- [ ] Tests validation abandons (10-15 attendus sur 100 puzzles)

**Durée estimée**: 1.5h

---

### PHASE 4: Activation World Model

**Objectif**: Construire carte interne avec 5-10 régions par puzzle

**Problème identifié**: 0 régions, cohérence 0.000 (non fonctionnel)

**Actions**:

1. **Implémenter construction de régions**
   ```python
   # Dans world_model_minimal.py
   class WorldModelMinimal:
       def __init__(self):
           self.regions = {}  # region_id -> observation
           self.region_map = {}  # position -> region_id
           self.region_counter = 0
       
       def _identify_region(self, observation: np.ndarray) -> str:
           """Identifie ou crée une région basée sur l'observation"""
           # Comparer avec régions existantes
           for region_id, region_obs in self.regions.items():
               similarity = self._compute_similarity(observation, region_obs)
               if similarity > 0.8:  # Même région
                   return region_id
           
           # Nouvelle région
           region_id = f"region_{self.region_counter}"
           self.region_counter += 1
           self.regions[region_id] = observation
           return region_id
       
       def update_region_map(self, position: Tuple[int, int], observation: np.ndarray):
           """Met à jour la carte des régions"""
           region_id = self._identify_region(observation)
           self.region_map[position] = region_id
           
           self.logger.log_event(
               event="region_updated",
               component="world_model",
               data={
                   "position": position,
                   "region_id": region_id,
                   "total_regions": len(self.regions)
               }
           )
   ```

2. **Intégrer dans solver**
   ```python
   # Dans arc_solver_adapter.py
   def _update_world_model(self, state):
       # Extraire observation actuelle
       observation = self._extract_observation(state)
       
       # Mettre à jour world model
       self.world_model.update_region_map(
           position=state.position,
           observation=observation
       )
       
       # Utiliser pour sélection d'actions
       region_id = self.world_model.get_region(state.position)
       region_history = self.world_model.get_region_history(region_id)
       
       # Adapter stratégie selon région
       if region_history.get('visited_count', 0) > 3:
           # Région déjà explorée, favoriser nouvelles régions
           action_bias = "explore_new"
       else:
           action_bias = "exploit_current"
   ```

**Livrables**:
- [ ] `world_model_minimal.py` modifié (construction régions)
- [ ] Intégration dans solver
- [ ] Tests validation 5-10 régions/puzzle

**Durée estimée**: 2h

---

### PHASE 5: Optimisation Budget Dynamique

**Objectif**: Forcer utilisation optimale du budget (épuisements 5-10%)

**Problème identifié**: 22% budget utilisé, jamais épuisé

**Actions**:

1. **Réduire budget initial**
   ```python
   # Dans dynamic_budget_allocator.py
   class DynamicBudgetAllocator:
       def __init__(self):
           # AVANT: self.total_budget = 100
           self.total_budget = 50  # Réduit de 50%
           
           # NOUVEAU: Budget adaptatif selon complexité
           self.adaptive_budget = True
   ```

2. **Budget adaptatif selon complexité puzzle**
   ```python
   def estimate_puzzle_complexity(self, puzzle: Dict) -> float:
       """Estime complexité d'un puzzle pour ajuster budget"""
       train_examples = puzzle['train']
       
       # Facteurs de complexité
       avg_size = np.mean([len(ex['input']) * len(ex['input'][0]) 
                          for ex in train_examples])
       num_colors = len(set(np.array(train_examples[0]['input']).flatten()))
       num_examples = len(train_examples)
       
       # Score complexité (0-1)
       complexity = (
           0.3 * min(avg_size / 100, 1.0) +
           0.3 * min(num_colors / 10, 1.0) +
           0.4 * min(num_examples / 5, 1.0)
       )
       
       return complexity
   
   def allocate_initial_budget(self, puzzle: Dict) -> int:
       """Alloue budget initial selon complexité"""
       if not self.adaptive_budget:
           return self.total_budget
       
       complexity = self.estimate_puzzle_complexity(puzzle)
       
       # Budget entre 30 et 70 selon complexité
       budget = int(30 + complexity * 40)
       
       self.logger.log_event(
           event="budget_allocated",
           component="budget_allocator",
           data={
               "complexity": complexity,
               "budget": budget
           }
       )
       
       return budget
   ```

**Livrables**:
- [ ] `dynamic_budget_allocator.py` modifié
- [ ] Budget adaptatif implémenté
- [ ] Tests validation épuisements 5-10%

**Durée estimée**: 1h

---

### PHASE 6: Diversification Action Discovery

**Objectif**: Découvrir 3-5 actions par puzzle (vs 1 actuellement)

**Problème identifié**: Tous les puzzles découvrent uniquement `rotate_270`

**Actions**:

1. **Améliorer détection patterns**
   ```python
   # Dans action_discovery_engine.py
   def discover_plausible_actions(self, train_examples: List[Dict]) -> List[Action]:
       """Découvre TOUTES les transformations plausibles"""
       plausible_actions = []
       
       # Tester TOUTES les transformations disponibles
       for transform_name in self.available_transforms:
           plausibility = self._compute_plausibility(
               transform_name, 
               train_examples
           )
           
           # Seuil plus permissif pour découverte
           if plausibility >= 0.2:  # Au lieu de 0.3
               plausible_actions.append(Action(
                   name=transform_name,
                   plausibility=plausibility
               ))
       
       # Trier par plausibilité
       plausible_actions.sort(key=lambda a: a.plausibility, reverse=True)
       
       # Retourner top 5
       return plausible_actions[:5]
   ```

2. **Ajouter détection patterns composés**
   ```python
   def _detect_composite_patterns(self, train_examples: List[Dict]) -> List[str]:
       """Détecte patterns nécessitant compositions de transformations"""
       patterns = []
       
       for example in train_examples:
           input_grid = np.array(example['input'])
           output_grid = np.array(example['output'])
           
           # Tester compositions 2-transformations
           for t1 in self.available_transforms:
               intermediate = self._apply_transform(t1, input_grid)
               
               for t2 in self.available_transforms:
                   final = self._apply_transform(t2, intermediate)
                   
                   if np.array_equal(final, output_grid):
                       patterns.append(f"{t1}+{t2}")
       
       return patterns
   ```

**Livrables**:
- [ ] `action_discovery_engine.py` modifié
- [ ] Détection patterns composés
- [ ] Tests validation 3-5 actions/puzzle

**Durée estimée**: 2h

---

## 📊 MÉTRIQUES DE VALIDATION

### Tests de Régression

Après chaque phase, exécuter:

```bash
cd lumvorax2/src/MAGEN/tests
python test_unified_arc_arcade.py --puzzles 100 --log-dir /tmp/magen_v38.3/
python analyze_forensic_logs.py --log-dir /tmp/magen_v38.3/
```

### Métriques à Tracker

| Métrique | Baseline V38.2 | Cible V38.3 | Actuel |
|----------|----------------|-------------|--------|
| Taux succès | 100% (synth) | 43.5% (réel) | ? |
| Réduction affordance | 0% | 30-50% | ? |
| Diversité actions | 1/puzzle | 3-5/puzzle | ? |
| Épuisements budget | 0% | 5-10% | ? |
| Abandons stagnation | 0 | 10-15 | ? |
| Régions world model | 0 | 5-10/puzzle | ? |

---

## 🎯 CRITÈRES DE SUCCÈS V38.3

### Critères Obligatoires

✅ Tests sur 100 puzzles ARC-AGI réels du dataset officiel  
✅ Taux succès >= 40% (proche de 43.5% baseline)  
✅ Réduction affordance >= 30%  
✅ Diversité actions >= 3/puzzle  
✅ Épuisements budget >= 5%  

### Critères Optionnels

⭐ Taux succès >= 45% (amélioration vs baseline)  
⭐ Abandons stagnation 10-15  
⭐ Régions world model 5-10/puzzle  
⭐ Durée moyenne < 1s/puzzle  

---

## 📅 PLANNING

### Semaine 1: Infrastructure + Recalibrage

- **Jour 1-2**: PHASE 1 (Tests réels)
- **Jour 3**: PHASE 2 (Affordance)
- **Jour 4**: PHASE 3 (Stagnation)
- **Jour 5**: Tests validation intermédiaires

### Semaine 2: Optimisations Avancées

- **Jour 6-7**: PHASE 4 (World Model)
- **Jour 8**: PHASE 5 (Budget)
- **Jour 9**: PHASE 6 (Diversification)
- **Jour 10**: Tests finaux + Rapport

**Durée totale estimée**: 10 jours (9.5h développement)

---

## 🚨 RISQUES IDENTIFIÉS

### Risque 1: Biais d'Exploitation Prématurée

**Description**: Actions populaires écrasent branches rares nécessaires

**Mitigation**:
- Bonus diversité dans réputation
- Exploration forcée de nouvelles actions
- Tracking branches sous-explorées

### Risque 2: Overfitting sur Training Set

**Description**: Optimisations trop spécifiques au training set

**Mitigation**:
- Validation croisée sur evaluation set
- Tests sur puzzles inconnus
- Métriques de généralisation

### Risque 3: Dégradation Performance

**Description**: Optimisations ralentissent le solver

**Mitigation**:
- Profiling avant/après chaque phase
- Seuils de performance (< 1s/puzzle)
- Rollback si dégradation > 50%

---

## 📝 LIVRABLES FINAUX

### Code

- [ ] `arc_dataset_loader.py` (150 lignes)
- [ ] `affordance_discovery_engine.py` modifié
- [ ] `trajectory_stagnation_detector.py` modifié
- [ ] `world_model_minimal.py` modifié
- [ ] `dynamic_budget_allocator.py` modifié
- [ ] `action_discovery_engine.py` modifié
- [ ] `test_unified_arc_arcade.py` modifié

### Documentation

- [ ] `RAPPORT_VALIDATION_V38.3.md` (métriques sur 100 puzzles réels)
- [ ] `ANALYSE_COMPARATIVE_V38.2_V38.3.md`
- [ ] `GUIDE_OPTIMISATIONS_HYPERPARAMETRES.md`

### Logs

- [ ] Logs forensic 100 puzzles réels (6 fichiers × 100 puzzles)
- [ ] Métriques agrégées par catégorie de puzzle
- [ ] Analyse patterns échecs/succès

---

**Fin du Plan Optimisations V38.3**

*Basé sur analyse forensic réelle des logs V38.2*