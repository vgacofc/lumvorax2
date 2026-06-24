# RAPPORT DIAGNOSTIC COMPLET - MAGEN V31 C1+C2+C3+C4
## Échec Total sur Tests Arcade Réels

**Date**: 2026-06-15  
**Version**: V31 Phase 1 (C1+C2+C3+C4)  
**Résultat**: 0/3 victoires (0.0%)  
**Statut**: ÉCHEC CRITIQUE

---

## 1. RÉSUMÉ EXÉCUTIF

### Résultats Globaux
```
Jeux testés: ls20, cn04, cn05
Victoires: 0/3 (0.0%)
Steps moyens: 200.0 (limite atteinte)
Reward moyen: -1.387 (négatif)
Positions uniques moyennes: 37.0
Loops détectés: 50 total
Dead-ends détectés: 0 total (ANOMALIE)
Policy breaks: 200 total (~50% des steps)
```

### Problèmes Critiques Identifiés

1. **Avatar Non Identifié (cn04)**: Échec complet d'identification
2. **Policy Breaks Excessifs**: 99-101 breaks en 200 steps
3. **Dead-End Detection Cassée**: 0 détections malgré loops
4. **Comportement Circulaire**: 22-28 loops malgré interventions
5. **Absence de Goal-Seeking**: Rewards négatifs, pas d'approche du but
6. **Decision Logic Aléatoire**: Pas de biais vers l'objectif

---

## 2. ANALYSE FORENSIQUE DÉTAILLÉE

### 2.1 Game ls20 - Analyse Ligne par Ligne

#### Phase d'Identification (Steps 1-3)
```json
Step 1: action="up", entities_moved=1, moved_ids=["avatar"]
Step 2: action="down", entities_moved=1, moved_ids=["avatar"]  
Step 3: action="up", entities_moved=1, moved_ids=["avatar"]
→ Avatar identifié step 3, confidence=0.933 ✓
```

**Succès**: Identification rapide et correcte.

#### Phase d'Exploration Initiale (Steps 4-9)
```json
Step 4-5: Mouvement vers [5,3] puis [5,2] puis [5,1] puis [5,0]
→ Exploration linéaire vers le haut
→ Viability=1.0, cognitive_cost=7.0, sterility=0.0
```

**Observation**: Exploration unidirectionnelle sans stratégie.

#### Premier Blocage (Steps 6-10)
```json
Step 6: [5,0] → action="up" → [5,0] (mur)
Step 7-9: Répétition action="up" à [5,0]
→ Visit_count passe de 1 à 5
→ Viability chute: 1.0 → 0.45 → 0.4 → 0.364 → 0.337
```

**Problème**: Aucune détection de dead-end malgré 5 visites.

#### Premier Policy Break (Step 10)
```json
Step 10: Loop détecté (pattern_length=3)
→ Collapse détecté (dominant_action="up", ratio=1.0)
→ Sterility_pressure=1.0
→ Policy break forcé (break_0)
```

**Analyse**: 
- Détection correcte du collapse
- MAIS: Trop tardive (après 5 tentatives)
- MAIS: Pas de détection de dead-end topologique

#### Comportement Post-Break (Steps 11-23)
```json
Step 12: action="left" → [4,0] (nouvelle position)
Step 13: action="up" → [4,0] (mur)
Step 14-15: Policy breaks successifs
→ Sterility_pressure reste à 0.85-1.0
→ Viability oscille 0.3-0.45
```

**Pattern Récurrent**:
1. Mouvement vers nouvelle position
2. Tentative dans direction bloquée
3. Policy break immédiat
4. Répétition du cycle

#### Phase de Stagnation (Steps 24-40)
```json
Steps 24-32: Exploration [6,0] → [6,1] → ... → [6,9]
→ Série de mouvements "down" réussis
→ Viability monte: 0.36 → 1.0
→ Sterility_pressure baisse: 0.85 → 0.45
```

**Observation Positive**: Quand l'agent trouve une direction libre, il progresse.

#### Retour au Blocage (Steps 33-40)
```json
Step 33-37: À [6,9], répétition action="down" (mur)
→ Visit_count: 1 → 2 → 3 → 4 → 5
→ Loops détectés (loop_2, loop_3)
→ Viability chute: 1.0 → 0.165
→ Policy breaks: break_14, break_15, break_16
```

**Problème Majeur**: 
- Même pattern qu'au début
- Aucun apprentissage
- Dead-end toujours pas détecté

#### Fin de Partie (Steps 100-200)
```json
Steps 100-112: Exploration [1,0] → [1,1] → ... → [1,8]
→ Découverte de nouvelles positions
→ Viability remonte temporairement
Steps 107-112: Blocage à [1,0]
→ Loop_5, loop_6 détectés
→ Policy breaks continus
→ Fin à step 200 sans victoire
```

**Bilan**: L'agent explore mais sans stratégie goal-directed.

---

### 2.2 Game cn04 - Échec Catastrophique

#### Problème d'Identification
```json
Step 1-200: TOUS les steps montrent:
"confidence_insufficient"
"best_entity": "avatar"
"best_score": 0.55-0.64 (jamais > 0.8)
"threshold": 0.8
```

**Cause Racine**: 
```python
# Dans observe_motion():
if delta == expected_delta:
    self.motion_correlation[entity_id] += 1.0
```

**Problème**: Le jeu cn04 a probablement:
- Plusieurs entités qui bougent
- Ou des mouvements non-déterministes
- Ou une structure d'entités différente

**Conséquence**: 
- `avatar_pos = None` pendant TOUS les 200 steps
- World model jamais initialisé
- Topology jamais construite
- Metacognitive controller sans données

#### Logs Typiques
```json
Step 50: 
"action": "noop"
"entities_moved": 0
"confidence": 0.0
"best_score": 0.436

Step 100:
"action": "down"  
"entities_moved": 1
"confidence": 0.0
"best_score": 0.618
```

**Pattern**: Score monte quand mouvement, baisse avec noop, mais jamais assez.

---

### 2.3 Game cn05 - Comportement Similaire à ls20

#### Identification Réussie
```json
Step 2: Avatar identifié
confidence=0.9333
position=[5,4]
```

#### Même Cycle de Blocage
```json
Steps 1-20: Exploration initiale
Steps 21-40: Premier cycle blocage/break
Steps 41-60: Deuxième cycle
...
Steps 180-200: Énième cycle
```

**Statistiques**:
- 47 positions uniques (vs 64 pour ls20)
- 28 loops détectés
- 101 policy breaks
- 0 dead-ends détectés

---

## 3. ANALYSE DES CAUSES RACINES

### 3.1 Avatar Identification Fragile

**Problème**: Seuil de confiance trop élevé (0.8)

```python
# Code actuel:
if self.confidence >= self.confidence_threshold:
    self.avatar_identified = True
```

**Impact cn04**:
- Best score plafonne à 0.64
- Jamais d'identification
- Système complètement paralysé

**Solution Proposée**:
```python
# Seuil adaptatif:
if step > 50 and best_score > 0.6:
    # Accepter le meilleur candidat
    self.avatar_id = best_entity
    self.confidence = best_score
```

---

### 3.2 Dead-End Detection Cassée

**Code Actuel**:
```python
def identify_dead_ends(self):
    for pos, data in self.nodes.items():
        connectivity = len(data['neighbors'])
        visit_count = self.world_model.position_history.get(pos, {}).get('visit_count', 0)
        
        if connectivity <= 1 and visit_count >= 2:
            self.dead_ends.add(pos)
```

**Problème 1**: Condition trop strictive
- `connectivity <= 1` signifie position avec 0 ou 1 voisin
- Mais dans une grille, même un coin a 2 voisins (diagonales exclues)
- Un mur a 3 voisins (les 3 directions libres)

**Problème 2**: Pas de détection de "cul-de-sac fonctionnel"
- Position [5,0] dans ls20:
  - 3 voisins théoriques: [4,0], [6,0], [5,1]
  - Mais [5,-1] est un mur (action "up" échoue)
  - Donc c'est un dead-end fonctionnel
  - MAIS connectivity=3, donc pas détecté

**Solution Proposée**:
```python
def identify_dead_ends(self):
    for pos in self.position_sequence[-50:]:  # Fenêtre récente
        visit_count = self.world_model.position_history.get(pos, {}).get('visit_count', 0)
        
        # Dead-end si:
        # 1. Visité plusieurs fois
        # 2. ET actions échouent souvent
        if visit_count >= 3:
            failed_actions = self._count_failed_actions(pos)
            if failed_actions >= 2:  # 2+ directions bloquées
                self.dead_ends.add(pos)
```

---

### 3.3 Policy Breaks Trop Agressifs

**Statistiques**:
- ls20: 99 breaks / 200 steps = 49.5%
- cn05: 101 breaks / 200 steps = 50.5%

**Causes**:
1. **Sterility Pressure Trop Sensible**:
```python
# Déclenche à 0.7:
if sterility_pressure > 0.7:
    return True
```

2. **Viability Threshold Trop Élevé**:
```python
# Déclenche à 0.2:
if viability < 0.2:
    return True
```

3. **Cumul des Conditions**:
```python
return (
    is_collapse or           # Déclenche souvent
    is_stagnant or          # Déclenche souvent  
    sterility_pressure > 0.7 or  # Déclenche souvent
    viability < 0.2         # Déclenche souvent
)
```

**Résultat**: Presque chaque step déclenche un break.

**Solution Proposée**:
```python
# Seuils plus tolérants:
sterility_threshold = 0.85  # au lieu de 0.7
viability_threshold = 0.1   # au lieu de 0.2

# ET condition de cooldown:
if self.steps_since_last_break < 5:
    return False  # Pas de break si trop récent
```

---

### 3.4 Decision Logic Sans Goal-Seeking

**Code Actuel**:
```python
# Base scores aléatoires:
action_scores = {a: random.random() for a in actions}

# Ajustements:
action_scores[action] += 0.3 * region_value  # World model
action_scores[action] -= 0.5  # Dead-end penalty
action_scores[action] += 0.3  # Frontier bonus
```

**Problèmes**:
1. **Base aléatoire**: Pas de biais vers le goal
2. **Pas de goal proximity**: Aucune notion de distance au but
3. **Pas de path planning**: Aucune planification

**Résultat**: 
- Exploration aléatoire
- Rewards négatifs (-1.387 moyen)
- Pas d'approche du goal

**Solution Proposée**:
```python
# Ajouter goal proximity:
if goal_pos and avatar_pos:
    for action in actions:
        next_pos = simulate_action(avatar_pos, action)
        current_dist = manhattan_distance(avatar_pos, goal_pos)
        next_dist = manhattan_distance(next_pos, goal_pos)
        
        if next_dist < current_dist:
            action_scores[action] += 1.0  # Fort bonus
        else:
            action_scores[action] -= 0.3  # Pénalité
```

---

## 4. CORRECTIONS PRIORITAIRES

### Correction 1: Avatar Identification Robuste

**Fichier**: `agent_self_anchor.py`

```python
def identify_avatar(self, entities: Dict[str, Tuple[int, int]]) -> Optional[str]:
    """Identification avec fallback adaptatif."""
    if len(self.motion_correlation) < self.min_observations:
        return None
    
    # Calcul des scores
    scores = {}
    for entity_id, correlation in self.motion_correlation.items():
        total_observations = len([a for a in self.action_history if a != 'noop'])
        if total_observations > 0:
            scores[entity_id] = correlation / total_observations
    
    if not scores:
        return None
    
    best_entity = max(scores.items(), key=lambda x: x[1])
    best_score = best_entity[1]
    
    # Seuil adaptatif
    threshold = self.confidence_threshold
    if len(self.action_history) > 50:
        # Après 50 steps, accepter le meilleur si > 0.6
        threshold = 0.6
    
    if best_score >= threshold:
        self.avatar_id = best_entity[0]
        self.confidence = best_score
        self.avatar_identified = True
        return self.avatar_id
    
    return None
```

---

### Correction 2: Dead-End Detection Fonctionnelle

**Fichier**: `topological_memory.py`

```python
def identify_dead_ends(self) -> Set[Tuple[int, int]]:
    """Détection de dead-ends fonctionnels."""
    dead_ends = set()
    
    # Analyser les positions récentes
    recent_positions = self.position_sequence[-100:]
    position_counts = {}
    
    for pos in recent_positions:
        position_counts[pos] = position_counts.get(pos, 0) + 1
    
    # Dead-end si visité 3+ fois
    for pos, count in position_counts.items():
        if count >= 3:
            # Vérifier les actions échouées
            failed_actions = self._count_failed_actions_at(pos)
            
            # Dead-end si 2+ directions bloquées
            if failed_actions >= 2:
                dead_ends.add(pos)
                
                # Log forensique
                if self.logger:
                    self.logger.log_event(
                        "topology.dead_end_detected",
                        "topological_memory",
                        {
                            "position": list(pos),
                            "visit_count": count,
                            "failed_actions": failed_actions,
                            "connectivity": len(self.nodes.get(pos, {}).get('neighbors', []))
                        }
                    )
    
    self.dead_ends = dead_ends
    return dead_ends

def _count_failed_actions_at(self, position: Tuple[int, int]) -> int:
    """Compte les actions qui ont échoué à cette position."""
    failed = 0
    transitions = self.world_model.transitions.get(position, {})
    
    for action, data in transitions.items():
        # Action échouée si from_pos == to_pos
        if data['to'] == position:
            failed += 1
    
    return failed
```

---

### Correction 3: Policy Breaks Modérés

**Fichier**: `meta_cognitive_controller.py`

```python
def __init__(self):
    # ... existing code ...
    self.last_break_step = -10  # Cooldown tracking
    self.break_cooldown = 5     # Minimum steps entre breaks

def should_disengage(self, ...) -> bool:
    """Décision de désengagement avec cooldown."""
    
    # Cooldown: pas de break si trop récent
    if self.step - self.last_break_step < self.break_cooldown:
        return False
    
    # Seuils plus tolérants
    is_collapse = self.detect_collapse()
    is_stagnant = self.detect_cognitive_stagnation()
    high_sterility = sterility_pressure > 0.85  # au lieu de 0.7
    low_viability = viability < 0.1  # au lieu de 0.2
    
    # Conditions plus strictes (AND au lieu de OR pour certaines)
    critical_state = is_collapse and high_sterility
    hopeless_state = low_viability and is_stagnant
    
    should_break = critical_state or hopeless_state
    
    if should_break:
        self.last_break_step = self.step
    
    return should_break
```

---

### Correction 4: Goal-Directed Decision Logic

**Fichier**: `test_v31_c1_c2_c3_c4_arcade_real.py`

```python
def make_decision(game, anchor, world_model, topology, meta):
    """Logique de décision goal-directed."""
    
    # 1. Identifier avatar et goal
    entities = game.get_entities()
    avatar_pos = anchor.get_avatar_position(entities)
    goal_pos = game.get_goal_position()
    
    if not avatar_pos:
        return random.choice(game.get_actions())
    
    actions = game.get_actions()
    action_scores = {}
    
    # 2. Base: Goal proximity (PRIORITAIRE)
    for action in actions:
        next_pos = simulate_action(avatar_pos, action, game)
        
        if goal_pos:
            current_dist = manhattan_distance(avatar_pos, goal_pos)
            next_dist = manhattan_distance(next_pos, goal_pos)
            
            # Fort bonus si rapproche du goal
            if next_dist < current_dist:
                action_scores[action] = 2.0
            elif next_dist == current_dist:
                action_scores[action] = 0.5
            else:
                action_scores[action] = 0.0
        else:
            action_scores[action] = random.random()
    
    # 3. Ajustements secondaires
    for action in actions:
        next_pos = simulate_action(avatar_pos, action, game)
        
        # Pénalité dead-end
        if topology.is_dead_end(next_pos):
            action_scores[action] -= 1.0
        
        # Pénalité positions visitées
        visit_count = world_model.position_history.get(next_pos, {}).get('visit_count', 0)
        action_scores[action] -= 0.1 * visit_count
        
        # Bonus frontières
        if next_pos in world_model.identify_frontiers():
            action_scores[action] += 0.3
    
    # 4. Policy breaks si nécessaire
    if meta.should_disengage(...):
        action_scores = meta.force_policy_break(action_scores)
    
    # 5. Sélection
    best_action = max(action_scores.items(), key=lambda x: x[1])[0]
    return best_action

def manhattan_distance(pos1, pos2):
    return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1])

def simulate_action(pos, action, game):
    """Simule l'action pour prédire la prochaine position."""
    x, y = pos
    if action == 'up': return (x, y-1)
    elif action == 'down': return (x, y+1)
    elif action == 'left': return (x-1, y)
    elif action == 'right': return (x+1, y)
    else: return pos
```

---

## 5. PLAN D'ACTION IMMÉDIAT

### Phase 1: Corrections Critiques (2h)
1. ✅ Implémenter avatar identification robuste
2. ✅ Implémenter dead-end detection fonctionnelle
3. ✅ Ajuster seuils policy breaks
4. ✅ Implémenter goal-directed decision logic

### Phase 2: Tests de Validation (1h)
1. Re-tester sur ls20, cn04, cn05
2. Vérifier:
   - Avatar identifié dans les 3 jeux
   - Dead-ends détectés
   - Policy breaks < 30% des steps
   - Rewards positifs
   - Au moins 1 victoire

### Phase 3: Analyse Post-Correction (30min)
1. Comparer logs avant/après
2. Mesurer améliorations quantitatives
3. Identifier problèmes résiduels

---

## 6. MÉTRIQUES DE SUCCÈS

### Critères Minimaux
- ✅ Avatar identifié: 3/3 jeux
- ✅ Dead-ends détectés: > 0
- ✅ Policy breaks: < 60/200 steps (30%)
- ✅ Victoires: ≥ 1/3 jeux

### Critères Optimaux
- ✅ Avatar identifié: < 10 steps
- ✅ Dead-ends détectés: > 5 par jeu
- ✅ Policy breaks: < 40/200 steps (20%)
- ✅ Victoires: ≥ 2/3 jeux
- ✅ Reward moyen: > 0

---

## 7. CONCLUSION

### Diagnostic Final

MAGEN V31 C1+C2+C3+C4 souffre de **4 défauts architecturaux majeurs**:

1. **Avatar Identification Fragile**: Seuil trop strict, échec sur cn04
2. **Dead-End Detection Cassée**: Condition topologique inadaptée
3. **Policy Breaks Excessifs**: Seuils trop sensibles, 50% des steps
4. **Absence de Goal-Seeking**: Exploration aléatoire sans stratégie

### Impact

Ces défauts créent un **cercle vicieux**:
```
Pas d'avatar → Pas de world model → Pas de topology → Pas de métacognition
                                                              ↓
                                                    Décisions aléatoires
                                                              ↓
                                                      Échec garanti
```

### Prochaines Étapes

Les corrections proposées sont **chirurgicales et testables**:
- Chaque correction adresse un défaut spécifique
- Chaque correction est mesurable
- L'ensemble devrait débloquer au moins 1 victoire

**Priorité Absolue**: Implémenter les 4 corrections et re-tester immédiatement.

---

**Rapport généré le**: 2026-06-15 15:45:00 UTC+2  
**Analyste**: Bob (Mode Advanced)  
**Statut**: CORRECTIONS REQUISES IMMÉDIATEMENT