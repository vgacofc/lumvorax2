# SOLUTION A RÉVISÉE - MODIFICATIONS V25

**Date**: 2026-06-14  
**Basé sur**: Analyse utilisateur Session 70  
**Objectif**: Enrichir perception avant d'optimiser planification

---

## PRIORITÉS ARCHITECTURALES CORRIGÉES

### Analyse Utilisateur (Citation)
> "Le problème réel est: Le moteur perceptif ne produit pas assez d'entropie structurelle pour créer des transitions causales exploitables."

> "Le système n'est plus 'cassé'. Il est maintenant 'cognitivement aveugle'."

### Priorités Révisées
1. ✅ **Enrichir embeddings** (StateEncoder V25 créé - 485 lignes)
2. ⏳ **Pénalisation répétition active**
3. ⏳ **Cycle avoidance actif**
4. ⏳ **Reward shaping intrinsèque**
5. ⏳ **Threshold tuning** (0.85 → 0.70)
6. ⏳ **Ratio exploration/exploitation** (28.5% → 80%)

---

## MODIFICATION #1: StateEncoder V25 Enrichi ✅

**Fichier créé**: `core/state_encoder_v25.py` (485 lignes)

### Nouveautés V25
```python
# Embedding: 128D → 256D (doublement)
# Nouvelles features:
- Spatial Relations: distances, alignements objets↔objets
- Topological Hash: structure locale invariante
- Action Context: embedding(state_t, action_t-1)
- Object Positions: centroids normalisés
```

### Architecture Embedding 256D
```
64D: Spatial features (downsampled 8x8)
32D: Object features (positions normalisées)
32D: Spatial relations (distances, alignements) [NOUVEAU]
32D: Topological features (hash MD5 structure) [NOUVEAU]
32D: Motion features (magnitude temporelle)
32D: Attention features (régions actives)
32D: Action context (one-hot action précédente) [NOUVEAU]
```

### Méthodes Nouvelles
```python
_detect_objects_with_positions() → (count, [(x,y), ...])
_compute_spatial_relations() → [{obj1, obj2, distance, alignment, quadrant}, ...]
_compute_topological_hash() → "md5_hash_8chars"
_compress_to_embedding_v25() → np.ndarray(256,)
```

### Impact Attendu
- **Diversité états**: Hash topologique + relations spatiales → États visuellement similaires deviennent distincts
- **Causalité**: Action context → Même état atteint différemment = embeddings différents
- **Graphe causal**: Plus de nodes (>10 attendus vs 1 actuel)

---

## MODIFICATION #2: Pénalisation Répétition Active ⏳

**Fichier**: `magen_arc_agi_agent_v25.py`

### Problème Actuel
```python
# Logs montrent:
[EXPLORATION] Action: ACTION1 (move_up) score=0.300
[CAUSAL GRAPH] ⚠️ Cycle détecté: period=2
[EXPLORATION] Action: ACTION1 (move_up) score=0.300  # Répétition
```

### Solution
```python
def _penalize_repetition(self, action_scores: Dict[str, float], recent_actions: List[str]) -> Dict[str, float]:
    """
    Pénaliser actions répétées récemment
    
    Stratégie:
    - 10 dernières actions: pénalité 50%
    - 5 dernières actions: pénalité 75%
    - 3 dernières actions: pénalité 90%
    """
    penalized_scores = action_scores.copy()
    
    for i, action in enumerate(recent_actions[-10:]):
        if action in penalized_scores:
            # Pénalité progressive
            if i >= 7:  # 3 dernières
                penalized_scores[action] *= 0.1
            elif i >= 5:  # 5 dernières
                penalized_scores[action] *= 0.25
            else:  # 10 dernières
                penalized_scores[action] *= 0.5
    
    return penalized_scores
```

### Intégration
```python
# Dans decide_exploration() ligne ~380
action_scores = self._penalize_repetition(action_scores, self.recent_actions[-10:])
```

---

## MODIFICATION #3: Cycle Avoidance Actif ⏳

**Fichier**: `magen_arc_agi_agent_v25.py`

### Problème Actuel
```python
# 570 cycles détectés MAIS aucune action corrective
self.stats['cycles_detected'] = 570  # Passif
```

### Solution
```python
def _handle_cycle_detection(self, cycle_detected: bool, cycle_period: int) -> str:
    """
    Réagir activement à détection cycle
    
    Actions:
    - Forcer action rare (jamais testée)
    - Injecter bruit contrôlé
    - Augmenter exploration locale
    - Réinitialiser si cycle persistant
    """
    if not cycle_detected:
        return 'normal'
    
    # Cycle court (period < 5): Forcer action différente
    if cycle_period < 5:
        return 'force_rare_action'
    
    # Cycle moyen (5-10): Injecter bruit
    elif cycle_period < 10:
        return 'inject_noise'
    
    # Cycle long (>10): Réinitialiser
    else:
        return 'reset_exploration'
```

### Intégration
```python
# Dans learn() après cycle detection
if cycles_detected:
    strategy = self._handle_cycle_detection(True, cycle_period)
    if strategy == 'force_rare_action':
        # Choisir action avec count minimal
        action = min(self.action_counts, key=self.action_counts.get)
    elif strategy == 'inject_noise':
        # Ajouter bruit gaussien aux scores
        action_scores = {k: v + np.random.normal(0, 0.1) for k, v in action_scores.items()}
    elif strategy == 'reset_exploration':
        # Réinitialiser priorités
        self.goal_grounder.reset_priorities()
```

---

## MODIFICATION #4: Reward Shaping Intrinsèque ⏳

**Fichier**: `magen_arc_agi_agent_v25.py`

### Problème Actuel
```python
# Aucun reward positif observé
avg_goal_progress: 0.0
actions_promising: 0
```

### Solution: Rewards Multi-Échelle
```python
def _compute_intrinsic_reward(self, obs_before, obs_after, action) -> float:
    """
    Calculer reward intrinsèque multi-échelle
    
    Rewards:
    - Nouveauté: État jamais vu (+0.1)
    - Diversité: Action rare (+0.05)
    - Désynchronisation: Sortie cycle (+0.2)
    - Découverte: Transition non vue (+0.15)
    - Changement topologique: Hash différent (+0.1)
    """
    reward = 0.0
    
    # 1. Nouveauté état
    if obs_after.embedding.topological_hash not in self.seen_hashes:
        reward += 0.1
        self.seen_hashes.add(obs_after.embedding.topological_hash)
    
    # 2. Diversité action
    action_count = self.action_counts.get(action, 0)
    if action_count < 5:
        reward += 0.05
    
    # 3. Désynchronisation cycle
    if self.in_cycle and not self._detect_cycle_now():
        reward += 0.2
        self.in_cycle = False
    
    # 4. Découverte transition
    transition_key = (obs_before.embedding.topological_hash, action)
    if transition_key not in self.seen_transitions:
        reward += 0.15
        self.seen_transitions.add(transition_key)
    
    # 5. Changement topologique
    if obs_before.embedding.topological_hash != obs_after.embedding.topological_hash:
        reward += 0.1
    
    return reward
```

### Intégration
```python
# Dans learn() après transition
intrinsic_reward = self._compute_intrinsic_reward(obs_before, obs_after, action)
total_reward = env_reward + intrinsic_reward
```

---

## MODIFICATION #5: Threshold Tuning ⏳

**Fichier**: `magen_arc_agi_agent_v25.py` ligne 137

### Changement Simple
```python
# AVANT
self.causal_graph = CausalTransitionGraph(
    similarity_threshold=0.85,  # Trop strict
    ...
)

# APRÈS
self.causal_graph = CausalTransitionGraph(
    similarity_threshold=0.70,  # Plus permissif
    ...
)
```

### Impact Attendu
- Plus de nodes créés (>10 vs 1)
- Graphe causal plus riche
- Planification BFS activée

---

## MODIFICATION #6: Ratio Exploration/Exploitation ⏳

**Fichier**: `magen_arc_agi_agent_v25.py` ligne 148-152

### Problème Actuel
```python
# Métriques montrent:
exploration: 28.5%
exploitation: 71.5%

# MAIS:
aucun reward utile
aucun goal trouvé
aucun plan
aucun état différencié
```

### Solution
```python
# AVANT
self.exploration_phase_active = True
self.exploration_actions_taken = 0
self.exploitation_actions_taken = 0
self.total_actions = 0

# APRÈS
self.exploration_phase_active = True
self.exploration_budget = 0.80  # 80% exploration
self.min_exploration_actions = 150  # Minimum avant exploitation
```

### Logique Décision Modifiée
```python
def decide(self, obs, available_actions):
    # Forcer exploration si:
    # 1. Moins de 150 actions
    # 2. Aucun reward positif observé
    # 3. Graphe causal < 10 nodes
    
    force_exploration = (
        self.total_actions < self.min_exploration_actions or
        self.stats['successful_actions'] == 0 or
        len(self.causal_graph.nodes) < 10
    )
    
    if force_exploration:
        return self.decide_exploration(obs, available_actions)
    else:
        # Ratio dynamique basé sur performance
        exploration_ratio = max(0.2, 1.0 - (self.stats['successful_actions'] / 100))
        if np.random.random() < exploration_ratio:
            return self.decide_exploration(obs, available_actions)
        else:
            return self.decide_exploitation(obs, available_actions)
```

---

## ORDRE D'IMPLÉMENTATION

### Phase 1: Perception Enrichie (FAIT ✅)
1. ✅ StateEncoder V25 créé (485 lignes)

### Phase 2: Comportement Adaptatif (À FAIRE)
2. ⏳ Pénalisation répétition
3. ⏳ Cycle avoidance actif
4. ⏳ Reward shaping intrinsèque

### Phase 3: Paramètres (À FAIRE)
5. ⏳ Threshold tuning
6. ⏳ Ratio exploration/exploitation

### Phase 4: Intégration (À FAIRE)
7. ⏳ Remplacer StateEncoder par StateEncoderV25 dans agent
8. ⏳ Ajouter méthodes comportement adaptatif
9. ⏳ Modifier logique décision

### Phase 5: Test (À FAIRE)
10. ⏳ Test V25 itération #10
11. ⏳ Analyser logs forensiques
12. ⏳ Vérifier métriques:
    - Graphe: >10 nodes, >20 edges
    - Plans: >1 plan généré
    - Score: >0/3
    - Cycles: <100 (réduction 80%)

---

## MÉTRIQUES SUCCÈS

### Avant (Test #9)
```
Graphe: 1 node, 1 edge
Plans: 0 générés
Score: 0/3
Cycles: 570
Hashes uniques: ~1
Exploration: 28.5%
```

### Après (Cible Test #10)
```
Graphe: >10 nodes, >20 edges
Plans: >1 généré
Score: >0/3 (au moins 1 niveau)
Cycles: <100 (réduction 80%)
Hashes uniques: >50
Exploration: 80%
```

---

## JUSTIFICATION PRIORITÉS

### Pourquoi Perception D'abord?
> "Tu as ajouté une couche cognitive V25 très avancée, mais elle repose sur une perception V24 trop compressée, donc tout l'édifice cognitif travaille sur un monde quasi statique."

### Pourquoi Pas Juste Threshold?
> "Même à 0.70: si tes embeddings restent trop homogènes, ton graphe va juste devenir un faux graphe bruité, pas un graphe causal réellement informatif."

### Pourquoi Reward Intrinsèque?
> "Dans des environnements difficiles, il faut récompenser: nouveauté, diversité, désynchronisation cycles, découverte état rare, transition non vue, réduction incertitude, changement topologique."

### Pourquoi Cycle Avoidance Actif?
> "Tes 570 cycles détectés sont une mine d'or. La plupart des systèmes considèrent les cycles comme des erreurs. Mais chez toi ils peuvent devenir: des signaux d'apprentissage intrinsèque."

---

**Statut**: Phase 1 complète (StateEncoder V25), Phase 2-5 en attente implémentation
**Prochaine action**: Implémenter modifications #2-6 puis test itération #10