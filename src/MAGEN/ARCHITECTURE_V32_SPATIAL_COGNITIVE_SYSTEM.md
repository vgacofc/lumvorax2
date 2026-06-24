# ARCHITECTURE V32 - SPATIAL COGNITIVE SYSTEM
## Refonte Architecturale Cognitive Topologique

**Date**: 2026-06-15  
**Version**: V32.0.0  
**Statut**: CONCEPTION INITIALE

---

## 1. VISION ARCHITECTURALE

### Transformation Fondamentale

```
V28-V31: Reactive Exploration Agent
           ↓
V32: Spatial Cognitive System
```

### Objectif Central

```
Construire une représentation cognitive exploitable du monde
au lieu de simplement explorer réactivement
```

---

## 2. PRINCIPES DIRECTEURS

### Principe 1: Structuration Cognitive
```
Priorité #1: STRUCTURATION COGNITIVE
Priorité #2: COHÉRENCE STRATÉGIQUE  
Priorité #3: APPRENTISSAGE TOPOLOGIQUE
Priorité #4: PERFORMANCE
```

### Principe 2: Conservation V28
```
✓ V28.2.2 reste fonctionnel
✓ Tous les tests V28 passent
✓ Comparaison V28 vs V32 possible
✗ Pas de modification destructive
```

### Principe 3: Forensic Total
```
Chaque module produit des logs détaillés
Traçabilité complète de la cognition
Métriques cognitives mesurables
```

### Principe 4: Modularité Stricte
```
✓ Un module = une capacité cognitive
✓ Interfaces claires
✓ Pas de dépendances circulaires
✗ Pas de monolithe
```

---

## 3. ARCHITECTURE GLOBALE

### Stack Cognitif Hiérarchique

```
┌─────────────────────────────────────────────┐
│  NIVEAU 4: MÉTACOGNITION                    │
│  - MetaCognitiveMonitor                     │
│  - CognitiveStabilizer                      │
│  - GoalHypothesisEngine                     │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  NIVEAU 3: STRATÉGIE GLOBALE                │
│  - RegionalStrategist                       │
│  - TrajectoryMeaningSystem                  │
│  - SelfSpatialIdentity                      │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  NIVEAU 2: STRUCTURATION SPATIALE           │
│  - MentalMapBuilder                         │
│  - SpatialCompressor                        │
│  - SpatialKnowledgeGraph                    │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  NIVEAU 1: PERCEPTION                       │
│  - AgentSelfAnchor (V31)                    │
│  - PersistentWorldModel (V31)               │
│  - TopologicalMemory (V31)                  │
└─────────────────────────────────────────────┘
```

---

## 4. MODULES COGNITIFS V32

### Module C1: MentalMapBuilder

**Mission**: Construire une représentation persistante du monde

**Capacités**:
- Spatial Persistence (positions, transitions, walls, loops)
- Region Formation (clustering automatique)
- Spatial Compression (trajectoires → structures)

**Input**:
```python
positions = [(5,0), (6,0), (7,0), (8,0)]
actions = ['right', 'right', 'right', 'right']
rewards = [-0.1, -0.1, -0.1, -0.1]
```

**Output**:
```python
{
    "region_type": "linear_corridor",
    "orientation": "east",
    "termination": "wall",
    "strategic_value": 0.0,
    "confidence": 0.92,
    "should_avoid": True
}
```

---

### Module C2: SpatialCompressor

**Mission**: Transformer expérience brute en structures cognitives

**Capacités**:
- Pattern Recognition (corridors, loops, clusters)
- Trajectory Compression (séquences → types)
- Semantic Labeling (noms significatifs)

**Exemple**:
```python
# Input
trajectory = [(5,0), (6,0), (7,0), (8,0), (8,0), (8,0)]

# Output
compressed = {
    "pattern": "east_corridor_dead_end",
    "length": 4,
    "termination_type": "wall",
    "revisits": 2,
    "cognitive_label": "sterile_probe"
}
```

---

### Module C3: RegionalStrategist

**Mission**: Évaluer utilité stratégique des régions

**Capacités**:
- Strategic Value Calculation
- Learning Potential Assessment
- Exploration Priority Ranking

**Métriques par Région**:
```python
{
    "strategic_value": 0.0-1.0,
    "learning_potential": 0.0-1.0,
    "novelty": 0.0-1.0,
    "reward_probability": 0.0-1.0,
    "loop_pressure": 0.0-1.0,
    "exploration_priority": 0.0-1.0
}
```

**Règle Critique**:
```python
# Une région peut être "cognitivement stérile"
# même avec beaucoup d'espace et plusieurs voisins
if learning_potential < 0.1 and reward_probability < 0.1:
    region.status = "cognitively_sterile"
```

---

### Module C4: CognitiveStabilizer

**Mission**: Empêcher fragmentation cognitive

**Problème V31**:
```
101 policy breaks / 200 steps = 50.5%
→ Chaos cognitif
```

**Solution V32**:
```python
# Règle stricte
policy_breaks < 10% des steps

# Sauf cas extrêmes:
- stagnation_extreme
- contradiction_forte  
- collapse_attractif_majeur
```

**Mécanismes**:
- Strategy Persistence Score
- Cognitive Confidence
- Policy Change Cost
- Inertie Cognitive Utile

---

### Module C5: MetaCognitiveMonitor

**Mission**: Réflexion sur la propre compréhension

**Métriques Cognitives**:
```python
{
    "map_informativeness": 0.0-1.0,
    "strategy_coherence": 0.0-1.0,
    "cognitive_growth_rate": 0.0-1.0,
    "hypothesis_consistency": 0.0-1.0,
    "information_gain_per_step": 0.0-1.0
}
```

**Questions Métacognitives**:
- "Ma carte devient-elle plus informative?"
- "Ma stratégie est-elle cohérente?"
- "Suis-je en train d'apprendre?"
- "Mes hypothèses sont-elles stables?"

---

### Module C6: SelfSpatialIdentity

**Mission**: Avatar comme centre référentiel

**Transformation**:
```
V31: avatar = moving entity
V32: avatar = CENTER OF COGNITIVE WORLD MODEL
```

**Représentation**:
```python
self_model = {
    "current_position": (5, 3),
    "current_region": "eastern_corridor",
    "active_strategy": "explore_southeast",
    "goal_hypothesis": "southeast_quadrant",
    "world_confidence": 0.7,
    "trajectory_meaning": "systematic_exploration"
}
```

---

### Module C7: GoalHypothesisEngine

**Mission**: Former hypothèses spatiales sur le goal

**Capacités**:
- Goal Location Hypothesis
- Region Promising Assessment
- Zone Abandonment Decision

**Output**:
```python
goal_hypotheses = [
    {
        "region": "southeast",
        "confidence": 0.74,
        "reasoning": [
            "high_frontier_density",
            "low_revisit_pressure",
            "open_connectivity"
        ],
        "priority": "high"
    },
    {
        "region": "northwest",
        "confidence": 0.12,
        "reasoning": [
            "multiple_loops",
            "no_progress",
            "high_sterility"
        ],
        "priority": "avoid"
    }
]
```

---

### Module C8: TrajectoryMeaningSystem

**Mission**: Transformer séquences en compréhension

**Transformation**:
```python
# Input (événementiel)
trajectory = [(5,0), (6,0), (7,0)]

# Output (cognitif)
meaning = {
    "trajectory_type": "failed_linear_probe",
    "information_gained": 0.6,
    "region_status": "dead_end_confirmed",
    "future_policy": "avoid",
    "cognitive_value": "high"  # Même échec = apprentissage
}
```

---

### Module C9: SpatialKnowledgeGraph

**Mission**: Représenter le monde comme graphe cognitif

**Structure**:
```python
graph = {
    "nodes": {
        "region_A": {
            "type": "corridor",
            "value": 0.3,
            "confidence": 0.9
        },
        "region_B": {
            "type": "frontier",
            "value": 0.8,
            "confidence": 0.6
        }
    },
    "edges": {
        ("region_A", "region_B"): {
            "type": "corridor",
            "traversals": 5,
            "cost": 0.2
        }
    }
}
```

---

## 5. DECISION KERNEL V32

### DecisionKernelV32Spatial

**Architecture**:
```python
class DecisionKernelV32Spatial:
    def __init__(self):
        # Niveau 1: Perception
        self.anchor = AgentSelfAnchor()
        self.world_model = PersistentWorldModel()
        self.topology = TopologicalMemory()
        
        # Niveau 2: Structuration
        self.mental_map = MentalMapBuilder()
        self.compressor = SpatialCompressor()
        self.knowledge_graph = SpatialKnowledgeGraph()
        
        # Niveau 3: Stratégie
        self.strategist = RegionalStrategist()
        self.trajectory_meaning = TrajectoryMeaningSystem()
        self.self_identity = SelfSpatialIdentity()
        
        # Niveau 4: Métacognition
        self.metacognitive = MetaCognitiveMonitor()
        self.stabilizer = CognitiveStabilizer()
        self.goal_engine = GoalHypothesisEngine()
    
    def decide(self, observation, step):
        # 1. Perception
        entities = observation['entities']
        avatar_pos = self.anchor.identify_avatar(entities)
        
        # 2. Structuration
        self.mental_map.update(avatar_pos, observation)
        regions = self.mental_map.get_regions()
        
        # 3. Compression
        trajectory = self.world_model.get_recent_trajectory()
        compressed = self.compressor.compress(trajectory)
        
        # 4. Évaluation stratégique
        region_values = self.strategist.evaluate_regions(regions)
        
        # 5. Hypothèses goal
        goal_hypotheses = self.goal_engine.generate_hypotheses(
            regions, region_values
        )
        
        # 6. Identité spatiale
        self.self_identity.update(
            avatar_pos, regions, goal_hypotheses
        )
        
        # 7. Métacognition
        cognitive_state = self.metacognitive.assess(
            self.mental_map,
            self.strategist,
            step
        )
        
        # 8. Stabilisation
        should_change = self.stabilizer.should_change_strategy(
            cognitive_state
        )
        
        # 9. Décision
        action = self._select_action(
            avatar_pos,
            regions,
            region_values,
            goal_hypotheses,
            cognitive_state,
            should_change
        )
        
        return action
```

---

## 6. MÉTRIQUES COGNITIVES

### Nouvelles Métriques V32

```python
cognitive_metrics = {
    # Compréhension
    "map_informativeness": 0.0-1.0,
    "world_model_stability": 0.0-1.0,
    "region_understanding": 0.0-1.0,
    
    # Cohérence
    "strategy_coherence": 0.0-1.0,
    "trajectory_meaningfulness": 0.0-1.0,
    "hypothesis_quality": 0.0-1.0,
    
    # Apprentissage
    "cognitive_growth": 0.0-1.0,
    "structural_compression_ratio": 0.0-1.0,
    "information_gain_rate": 0.0-1.0,
    
    # Stabilité
    "policy_change_frequency": 0.0-1.0,
    "cognitive_confidence": 0.0-1.0,
    "model_consistency": 0.0-1.0
}
```

---

## 7. FORENSIC LOGGING V32

### Structure des Logs

```
logs_v32_cognitive/
├── cognitive_map.json
├── region_analysis.json
├── strategic_evolution.json
├── hypothesis_tracking.json
├── spatial_compression.json
├── metacognitive_state.json
└── trajectory_meanings.json
```

### Contenu Minimal

```python
{
    "episode_id": "ls20_v32_001",
    "mental_map": {
        "regions": [...],
        "compression_events": [...],
        "structural_evolution": [...]
    },
    "strategic_analysis": {
        "region_values": [...],
        "goal_hypotheses": [...],
        "policy_changes": [...]
    },
    "cognitive_growth": {
        "map_informativeness": [...],
        "learning_events": [...],
        "understanding_milestones": [...]
    },
    "metacognitive_trace": {
        "self_assessments": [...],
        "strategy_reflections": [...],
        "confidence_evolution": [...]
    }
}
```

---

## 8. TESTS OBLIGATOIRES

### test_v32_spatial_cognition.py

**Tests Minimaux**:

1. **TEST_REGION_FORMATION**: Système crée des régions
2. **TEST_TRAJECTORY_COMPRESSION**: Système compresse trajectoires
3. **TEST_STERILE_DETECTION**: Système identifie région stérile
4. **TEST_STRATEGY_STABILITY**: Stratégie stable > 20 steps
5. **TEST_GOAL_HYPOTHESIS**: Système génère hypothèse goal
6. **TEST_POLICY_BREAKS**: Breaks < 10% des steps
7. **TEST_MAP_GROWTH**: map_informativeness augmente

---

## 9. CRITÈRES DE SUCCÈS

### Succès Technique

```
✓ Tous les tests passent
✓ V28.2.2 reste fonctionnel
✓ Logs forensic complets
✓ Métriques cognitives mesurables
```

### Succès Cognitif

```
✓ Carte mentale stable émerge
✓ Régions identifiées correctement
✓ Stratégie cohérente maintenue
✓ Hypothèses goal formées
✓ Compression spatiale effective
```

### Succès Comparatif

```
V32 vs V31:
✓ Moins de policy breaks
✓ Plus de compréhension
✓ Plus de stabilité
✓ Meilleure performance (optionnel)
```

---

## 10. PLAN D'IMPLÉMENTATION

### Phase 1: Fondations (Jour 8)
- Module C1: MentalMapBuilder
- Module C2: SpatialCompressor
- Tests unitaires

### Phase 2: Stratégie (Jour 9)
- Module C3: RegionalStrategist
- Module C4: CognitiveStabilizer
- Tests intégration

### Phase 3: Métacognition (Jour 10)
- Module C5: MetaCognitiveMonitor
- Module C6: SelfSpatialIdentity
- Module C7: GoalHypothesisEngine

### Phase 4: Intégration (Jour 11)
- Module C8: TrajectoryMeaningSystem
- Module C9: SpatialKnowledgeGraph
- DecisionKernelV32Spatial

### Phase 5: Validation (Jour 12)
- Tests complets
- Comparaison V28/V31/V32
- Rapports forensic

---

## 11. CONTRAINTES ABSOLUES

### INTERDICTIONS

```
✗ Casser V28.2.2
✗ Monolithe géant
✗ Dépendances circulaires
✗ Policy breaks > 10%
✗ Oublier la carte mentale
✗ Fonctionner uniquement par reward externe
```

### OBLIGATIONS

```
✓ Modularité stricte
✓ Forensic total
✓ Métriques cognitives
✓ Stabilité cognitive
✓ Accumulation de compréhension
✓ Organisation spatiale
✓ Formation d'hypothèses
✓ Cohérence interne
```

---

## 12. CONCLUSION

### Vision V32

```
MAGEN V32 ne doit plus seulement EXPLORER
Il doit COMPRENDRE LE MONDE
```

### Transformation Fondamentale

```
Reactive Agent → Spatial Cognitive System
```

### Objectif Final

```
Voir émerger une carte mentale stable,
cohérente et exploitable
```

---

**Document d'Architecture V32**  
**Statut**: PRÊT POUR IMPLÉMENTATION  
**Prochaine Étape**: Implémenter Module C1 - MentalMapBuilder