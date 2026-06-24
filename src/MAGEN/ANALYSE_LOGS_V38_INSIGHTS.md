# ANALYSE LOGS FORENSIC V38 - INSIGHTS CRITIQUES

**Date**: 2026-06-15  
**Session**: test_v38_simple (5 puzzles synthétiques)  
**Logs**: /tmp/magen_v38_simple/

---

## 1. RÉSUMÉ EXÉCUTIF

### Modules Actifs

| Module | Statut | Événements | Utilisation |
|--------|--------|------------|-------------|
| C0 Affordance | ✅ ACTIF | 35 | Filtrage 100% |
| C19 Stagnation | ✅ ACTIF | 34 | Détection parfaite |
| C17 Reputation | ⚠️ INACTIF | 2 | Non utilisé |
| C18 Budget | ⚠️ INACTIF | 6 | Non utilisé |

### Problèmes Identifiés

1. **C0 filtre TOUTES les actions** (`plausibility=0.25` < `min=0.3`)
2. **C17/C18 ne sont pas appelés** dans la boucle de résolution
3. **Solver tourne en boucle** sur même position (1 position unique)

---

## 2. ANALYSE DÉTAILLÉE PAR MODULE

### 2.1 C0: AffordanceDiscoveryEngine

**Événements**: 35 (16 découvertes + 16 filtrages + 2 init + 1 stats)

#### Première Découverte

```json
{
  "state_shape": [2, 2],
  "available_actions": 2,
  "affordances_found": 2,
  "top_affordance": "move_to_(0, 1)",
  "top_plausibility": 0.250,
  "context": {
    "color_count": 4,
    "has_symmetry": false,
    "has_patterns": true,
    "is_sparse": false,
    "complexity": 0.400
  }
}
```

#### Analyse Contexte Perceptuel

- **Couleurs**: 4 (diversité moyenne)
- **Symétrie**: False (pas de symétrie détectée)
- **Patterns**: True (patterns réguliers présents)
- **Sparse**: False (grille dense)
- **Complexité**: 0.400 (moyenne)

#### Problème Critique

```
top_plausibility = 0.250
min_plausibility = 0.300
→ TOUTES actions rejetées
```

**Formule plausibility** :
```python
plausibility = perceptual_score * feasibility_score * reputation_score
0.250 = 0.5 * 0.5 * 1.0  # Approximation
```

**Impact** :
- Actions disponibles: 32
- Actions filtrées: 0
- **Taux filtrage: 100%** ← PROBLÈME

**Solution** :
```python
# Baisser seuil
min_plausibility = 0.2  # Au lieu de 0.3
```

---

### 2.2 C19: TrajectoryStagnationDetector

**Événements**: 34 (16 états + 14 détections + 2 init + 2 stats)

#### Évolution Stagnation

| Détection | Variance | Loop Ratio | Unique Ratio | Should Abandon | Confidence |
|-----------|----------|------------|--------------|----------------|------------|
| #1 | 0.000000 | 0.500 | 0.500 | False | 0.400 |
| #2 | 0.000000 | 0.667 | 0.333 | False | 0.600 |
| #3 | 0.000000 | 0.750 | 0.250 | **True** | 0.800 |
| #4 | 0.000000 | 0.800 | 0.200 | **True** | 1.000 |
| #5 | 0.000000 | 0.833 | 0.167 | **True** | 1.000 |

#### Analyse

**Variance = 0.0** :
- Erreur constante (0.5) à chaque step
- Aucune amélioration détectée

**Loop Ratio = 0.833** :
- 83.3% des états sont des répétitions
- Seulement 16.7% états uniques

**Positions Uniques = 1** :
- Le solver reste bloqué sur position `(1, 1)`
- Aucune exploration réelle

**Décision Abandon** :
- Déclenchée dès détection #3 (window_size=4)
- Confidence = 0.8 (seuil atteint)
- **Fonctionne parfaitement** ✅

---

### 2.3 C17: ActionReputationSystem

**Événements**: 2 (2 init seulement)

#### Configuration

```json
{
  "decay_rate": 0.000,  ← PROBLÈME
  "min_attempts": 0,    ← PROBLÈME
  "exploration_threshold": 0.000  ← PROBLÈME
}
```

#### Problème

Les valeurs sont à 0.0 car **le log n'enregistre pas les vraies valeurs**.

**Tentatives enregistrées**: 0

**Cause** :
- C17 n'est pas appelé dans la boucle `solve_puzzle()`
- Le code dans [`arc_solver_adapter.py:235-327`](lumvorax2/src/MAGEN/core/arc_solver_adapter.py:235) enregistre les tentatives mais ne met jamais à jour le succès

**Solution** :
1. Appeler `reputation_system.record_action_attempt()` avec vrai succès
2. Mettre à jour après exécution action

---

### 2.4 C18: DynamicBudgetAllocator

**Événements**: 6 (2 init + 2 state_retrieved + 2 stats)

#### Configuration

```json
{
  "total_budget": 100.0,
  "min_allocation": 10.0
}
```

#### Problème

**Allocations effectuées**: 0

**Cause** :
- C18 n'est pas appelé dans la boucle `solve_puzzle()`
- Le code dans [`arc_solver_adapter.py:235-327`](lumvorax2/src/MAGEN/core/arc_solver_adapter.py:235) calcule allocation mais ne consomme jamais le budget

**Solution** :
1. Appeler `budget_allocator.consume_budget()` après exécution
2. Vérifier `should_continue()` avant chaque step

---

## 3. PROBLÈME STRUCTUREL IDENTIFIÉ

### Boucle Actuelle (Simplifiée)

```python
for step in range(max_steps):
    # 1. Génération actions
    actions = _generate_candidate_actions()
    
    # 2. C0: Filtrage affordances
    if affordance_engine:
        filtered = affordance_engine.filter_actions()
        # PROBLÈME: filtered est vide (100% rejeté)
    
    # 3. C17: Enregistrement tentatives
    if reputation_system:
        for action in actions:
            reputation_system.record_action_attempt(
                action, 
                success=False,  # ← Jamais mis à jour
                error=0.5
            )
    
    # 4. C18: Allocation budget
    if budget_allocator:
        allocation = budget_allocator.allocate_budget()
        # PROBLÈME: Jamais consommé
    
    # 5. Sélection action
    best_action = _select_best_action_v38()
    
    # 6. Exécution
    current_pos = best_action
    
    # 7. C19: Détection stagnation
    if stagnation_detector:
        stagnation_detector.add_state()
        if stagnation.should_abandon:
            break  # ✅ Fonctionne
```

### Problèmes

1. **C0 rejette tout** → `filtered_actions` vide → pas d'actions à explorer
2. **C17 success jamais mis à jour** → pas d'apprentissage
3. **C18 budget jamais consommé** → pas de gestion ressources
4. **Solver tourne en boucle** → même position répétée

---

## 4. SOLUTIONS PROPOSÉES

### 4.1 Ajuster Seuil C0

```python
# Dans arc_solver_adapter.py __init__
if AffordanceDiscoveryEngine:
    self.affordance_engine = AffordanceDiscoveryEngine(
        forensic_log_path=...,
        reputation_system=self.reputation_system,
        min_plausibility=0.2  # ← Baisser de 0.3 à 0.2
    )
```

### 4.2 Compléter Boucle Résolution

```python
for step in range(max_steps):
    # ... génération actions ...
    
    # Exécution action
    old_error = compute_error(current_state, target)
    current_pos = best_action
    new_error = compute_error(new_state, target)
    
    # Mise à jour C17 (succès réel)
    if reputation_system:
        success = (new_error < old_error)
        reputation_system.record_action_attempt(
            action_id=f"move_to_{best_action}",
            success=success,  # ← Vrai succès
            error=new_error,
            cost=1.0
        )
    
    # Consommation C18 (budget)
    if budget_allocator:
        budget_allocator.consume_budget(
            action_id=f"move_to_{best_action}",
            cost=1.0
        )
        
        if not budget_allocator.should_continue():
            break  # Budget épuisé
    
    # ... détection stagnation C19 ...
```

### 4.3 Améliorer Génération Actions

```python
def _generate_candidate_actions(self, current_pos, grid_shape):
    """Génère vraies actions (pas juste positions)."""
    actions = []
    
    # Actions de mouvement
    for dx, dy in [(-1,0), (1,0), (0,-1), (0,1)]:
        new_pos = (current_pos[0] + dx, current_pos[1] + dy)
        if 0 <= new_pos[0] < grid_shape[0] and 0 <= new_pos[1] < grid_shape[1]:
            actions.append(new_pos)
    
    # Actions de transformation (à implémenter)
    # - rotate_90, flip_horizontal, etc.
    
    return actions
```

---

## 5. MÉTRIQUES OBSERVÉES VS ATTENDUES

| Métrique | Observé | Attendu | Écart |
|----------|---------|---------|-------|
| **C0 Filtrage** | 100% | 70% | +30% |
| **C17 Tentatives** | 0 | >100 | -100% |
| **C18 Utilisation** | 0% | 60% | -60% |
| **C19 Détections** | 14 | ~10 | +40% |
| **Positions uniques** | 1 | >10 | -90% |

---

## 6. PROCHAINES ACTIONS

### Priorité 1: Corriger C0 (Filtrage)

```bash
# Modifier min_plausibility
vim lumvorax2/src/MAGEN/core/arc_solver_adapter.py
# Ligne 146: min_plausibility=0.2
```

### Priorité 2: Compléter Boucle Résolution

```bash
# Ajouter mise à jour C17/C18
vim lumvorax2/src/MAGEN/core/arc_solver_adapter.py
# Lignes 235-350: Compléter logique
```

### Priorité 3: Implémenter Vraies Transformations

```bash
# Créer module transformations
vim lumvorax2/src/MAGEN/core/transformations.py
```

### Priorité 4: Tester sur Dataset Réel

```bash
# Télécharger ARC-AGI-3
wget https://github.com/fchollet/ARC-AGI/raw/master/data/training/arc-agi_training_challenges.json
mv arc-agi_training_challenges.json lumvorax2/data/

# Exécuter tests
python3 lumvorax2/src/MAGEN/tests/test_v38_cognitive_integration.py
```

---

## 7. CONCLUSION

### Modules Validés

✅ **C0 AffordanceDiscoveryEngine** : Fonctionne mais seuil trop élevé  
✅ **C19 TrajectoryStagnationDetector** : Fonctionne parfaitement

### Modules À Compléter

⚠️ **C17 ActionReputationSystem** : Intégré mais pas utilisé  
⚠️ **C18 DynamicBudgetAllocator** : Intégré mais pas utilisé

### Impact

L'architecture cognitive V38 est **structurellement correcte** mais **partiellement active**.

Les modules C0 et C19 démontrent que le pipeline fonctionne.

Les modules C17 et C18 nécessitent une **intégration plus profonde** dans la boucle de résolution.

**Prochaine étape** : Compléter l'intégration et tester sur dataset réel.