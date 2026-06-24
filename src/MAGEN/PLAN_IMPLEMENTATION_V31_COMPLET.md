# PLAN D'IMPLÉMENTATION MAGEN V31 - COMPLET

**Date** : 2026-06-15  
**Session** : 73 - Jour 7 (Plan d'Implémentation)  
**Objectif** : Transformer MAGEN en agent cognitif cartographique  
**Statut** : 📋 PLAN D'EXÉCUTION DÉTAILLÉ

---

## OBJECTIF PRINCIPAL

Transformer MAGEN d'un **moteur réactif** en **agent cognitif cartographique et métacognitif** capable de :

1. ✅ Identifier son avatar réel dans le jeu
2. ✅ Construire une carte persistante du monde
3. ✅ Mémoriser les zones déjà explorées
4. ✅ Mémoriser les zones stériles
5. ✅ Construire une topologie des transitions
6. ✅ Réutiliser les découvertes passées
7. ✅ Détecter les politiques mortes
8. ✅ Abandonner une trajectoire inutile
9. ✅ Réallouer l'exploration stratégiquement

---

## DIAGNOSTIC FONDAMENTAL

### Ce Que MAGEN Fait Actuellement

✅ Explore  
✅ Agit  
✅ Génère des actions  
✅ Calcule des scores  

### Ce Que MAGEN Ne Fait PAS

❌ Représentation persistante du monde  
❌ Mémoire topologique globale  
❌ Identité agent stable  
❌ Mémoire de stérilité  
❌ Coût cognitif  
❌ Mécanisme de désengagement stratégique  

### Conséquences

- Collapse attractif (ACTION1 85%)
- Répétition infinie (498K steps)
- Scores figés (0.5 constant)
- Aucune progression réelle

---

## ARCHITECTURE DES 12 CORRECTIONS

```
┌─────────────────────────────────────────────────────────┐
│  COUCHE MÉTACOGNITIVE                                   │
├─────────────────────────────────────────────────────────┤
│  C5: MetaCognitiveController                            │
│  C6: PolicyBreakSystem                                  │
│  C8: CognitiveCostModel                                 │
│  C9: GlobalTrajectoryManager                            │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE SPATIALE-IDENTITAIRE                            │
├─────────────────────────────────────────────────────────┤
│  C1: AgentSelfAnchor                                    │
│  C2: PersistentWorldModel                               │
│  C3: TopologicalMemory                                  │
│  C4: SterilityMemory                                    │
│  C7: MetaRewardGenerator                                │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE DÉCISIONNELLE (REFONTE)                         │
├─────────────────────────────────────────────────────────┤
│  C10: DecisionKernelV2                                  │
│  C11: ForensicLoggingComplet                            │
└─────────────────────────────────────────────────────────┘
```

---

## CORRECTION 1 : AGENT SELF-ANCHOR

### Fichier
`lumvorax2/src/MAGEN/core/agent_self_anchor.py`

### Objectif
Identifier l'avatar contrôlé par l'agent

### Principe
L'avatar est l'entité dont le mouvement est causalement corrélé aux actions exécutées

### Méthode
1. Observer état avant action
2. Exécuter action
3. Observer entités ayant bougé
4. Accumuler corrélation motion ↔ action
5. Confirmer après 3-5 actions

### Formulation
```python
agent = argmax_entity(
    correlation(entity.motion, action_history)
)
```

### Interface
```python
class AgentSelfAnchor:
    def __init__(self):
        self.agent_id: Optional[int] = None
        self.confidence: float = 0.0
        self.trajectory_history: List[Tuple[int, int]] = []
        self.motion_signature: Dict[str, List[Tuple[int, int]]] = {}
    
    def observe_motion(
        self,
        prev_entities: List[Dict],
        curr_entities: List[Dict],
        action: str
    ) -> None:
        """Observe mouvement après action"""
        pass
    
    def identify_avatar(self) -> Optional[int]:
        """Identifie avatar avec confiance > 0.8"""
        pass
    
    def get_avatar_position(
        self,
        entities: List[Dict]
    ) -> Optional[Tuple[int, int]]:
        """Retourne position avatar actuelle"""
        pass
```

### Tests
- Test identification avatar en 5 steps
- Test confiance > 0.8
- Test robustesse multi-entités

---

## CORRECTION 2 : PERSISTENT WORLD MODEL

### Fichier
`lumvorax2/src/MAGEN/core/persistent_world_model.py`

### Objectif
Construire une carte persistante du monde

### Structure
```python
world_graph = {
    node: {
        'visits': int,
        'reward_history': List[float],
        'sterility_score': float,
        'transitions': Dict[str, node],
        'novelty': float,
        'escape_routes': List[node]
    }
}
```

### Interface
```python
class PersistentWorldModel:
    def __init__(self):
        self.world_graph: Dict[Tuple[int, int], Node] = {}
        self.visited_positions: Set[Tuple[int, int]] = set()
        self.regions: Dict[str, Region] = {}
        self.total_coverage: float = 0.0
    
    def update_from_observation(
        self,
        position: Tuple[int, int],
        observation: np.ndarray,
        action: str,
        reward: float
    ) -> None:
        """Met à jour carte à partir observation"""
        pass
    
    def get_node(
        self,
        position: Tuple[int, int]
    ) -> Optional[Node]:
        """Retourne nœud à position donnée"""
        pass
    
    def identify_regions(self) -> List[Region]:
        """Identifie régions distinctes"""
        pass
    
    def get_unexplored_frontier(self) -> List[Tuple[int, int]]:
        """Retourne positions frontière non explorées"""
        pass
```

### Tests
- Test construction carte progressive
- Test identification régions
- Test mémorisation transitions
- Test couverture monde

---

## CORRECTION 3 : TOPOLOGICAL MEMORY

### Fichier
`lumvorax2/src/MAGEN/core/topological_memory.py`

### Objectif
Modéliser le monde comme graphe causal

### Interface
```python
class TopologicalMemory:
    def __init__(self):
        self.connectivity_graph: nx.DiGraph = nx.DiGraph()
        self.corridors: List[Corridor] = []
        self.dead_ends: List[Tuple[int, int]] = []
        self.cycles: List[List[Tuple[int, int]]] = []
    
    def add_transition(
        self,
        from_pos: Tuple[int, int],
        to_pos: Tuple[int, int],
        action: str,
        success: bool
    ) -> None:
        """Ajoute transition au graphe"""
        pass
    
    def identify_corridors(self) -> List[Corridor]:
        """Identifie corridors (chemins linéaires)"""
        pass
    
    def identify_dead_ends(self) -> List[Tuple[int, int]]:
        """Identifie impasses"""
        pass
    
    def find_path(
        self,
        start: Tuple[int, int],
        goal: Tuple[int, int]
    ) -> Optional[List[Tuple[int, int]]]:
        """Trouve chemin entre deux positions"""
        pass
```

### Tests
- Test construction graphe connectivité
- Test identification corridors
- Test identification impasses
- Test pathfinding

---

## CORRECTION 4 : STERILITY MEMORY

### Fichier
`lumvorax2/src/MAGEN/core/sterility_memory.py`

### Objectif
Mémoriser les zones historiquement inutiles

### Formulation
```python
sterility_score = (
    repeated_visits +
    no_reward_duration +
    entropy_collapse +
    failed_exits
)
```

### Interface
```python
class SterilityMemory:
    def __init__(self):
        self.sterile_regions: Dict[str, float] = {}
        self.sterile_positions: Set[Tuple[int, int]] = set()
        self.sterility_threshold: float = 0.7
    
    def update_sterility(
        self,
        position: Tuple[int, int],
        reward: float,
        novelty: float,
        visits: int
    ) -> None:
        """Met à jour score stérilité"""
        pass
    
    def is_sterile(
        self,
        position: Tuple[int, int]
    ) -> bool:
        """Vérifie si position est stérile"""
        pass
    
    def get_sterile_regions(self) -> List[str]:
        """Retourne régions stériles"""
        pass
    
    def penalize_sterile_action(
        self,
        action_scores: Dict[str, float],
        predicted_positions: Dict[str, Tuple[int, int]]
    ) -> Dict[str, float]:
        """Pénalise actions menant vers zones stériles"""
        pass
```

### Tests
- Test détection stérilité
- Test pénalisation actions
- Test évitement zones mortes

---

## CORRECTION 5 : META-COGNITIVE CONTROLLER

### Fichier
`lumvorax2/src/MAGEN/core/meta_cognitive_controller.py`

### Objectif
Détecter quand la politique actuelle doit être abandonnée

### Conditions de Rupture
```python
variance(scores) < threshold
OR stagnation_steps > threshold
OR sterility > threshold
OR repeated_policy > threshold
```

### Interface
```python
class MetaCognitiveController:
    def __init__(self):
        self.stagnation_threshold: int = 1000
        self.variance_threshold: float = 0.01
        self.steps_without_progress: int = 0
        self.policy_breaks: List[Dict] = []
    
    def should_break_policy(
        self,
        action_scores: Dict[str, float],
        reward_history: List[float],
        sterility_score: float
    ) -> Tuple[bool, str]:
        """Détermine si doit casser politique"""
        pass
    
    def execute_policy_break(
        self,
        reason: str
    ) -> Dict[str, Any]:
        """Exécute rupture politique"""
        pass
    
    def detect_attractive_collapse(
        self,
        action_distribution: Dict[str, float]
    ) -> bool:
        """Détecte collapse attractif (ex: ACTION1 > 80%)"""
        pass
```

### Tests
- Test détection stagnation
- Test détection collapse
- Test rupture politique
- Test réallocation

---

## CORRECTION 6 : POLICY BREAK SYSTEM

### Fichier
`lumvorax2/src/MAGEN/core/policy_break_system.py`

### Objectif
Empêcher les collapses attractifs irréversibles

### Stratégies de Break
1. Reset total réputation
2. Inversion dominance
3. Exploration forcée
4. Blacklist temporaire action dominante

### Interface
```python
class PolicyBreakSystem:
    def __init__(self):
        self.break_strategies: List[str] = [
            'reset_reputation',
            'invert_dominance',
            'force_exploration',
            'blacklist_dominant'
        ]
        self.blacklisted_actions: Set[str] = set()
        self.break_history: List[Dict] = []
    
    def execute_break(
        self,
        strategy: str,
        action_scores: Dict[str, float],
        dominant_action: str
    ) -> Dict[str, float]:
        """Exécute stratégie de break"""
        pass
    
    def reset_reputation(
        self,
        action_scores: Dict[str, float]
    ) -> Dict[str, float]:
        """Reset complet réputation"""
        pass
    
    def invert_dominance(
        self,
        action_scores: Dict[str, float],
        dominant_action: str
    ) -> Dict[str, float]:
        """Inverse préférences"""
        pass
    
    def blacklist_action(
        self,
        action: str,
        duration: int = 100
    ) -> None:
        """Blacklist temporaire action"""
        pass
```

### Tests
- Test reset réputation
- Test inversion dominance
- Test blacklist temporaire
- Test efficacité break

---

## CORRECTION 7 : META-REWARD GENERATOR

### Fichier
`lumvorax2/src/MAGEN/core/meta_reward_generator.py`

### Objectif
Générer des gradients internes quand le monde n'en fournit pas

### Récompenses Internes
```python
# Récompenser
+ découverte nouvelles régions
+ augmentation connectivité
+ réduction incertitude
+ nouvelles transitions
+ sortie région stérile
+ rupture de boucle

# Punir
- répétition
- cycles
- stagnation
- revisites stériles
- collapse attractif
```

### Interface
```python
class MetaRewardGenerator:
    def __init__(self):
        self.novelty_weight: float = 0.5
        self.connectivity_weight: float = 0.3
        self.sterility_penalty: float = -0.5
    
    def generate_meta_reward(
        self,
        position: Tuple[int, int],
        is_novel: bool,
        is_sterile: bool,
        connectivity_increase: int,
        in_cycle: bool
    ) -> float:
        """Génère reward interne"""
        pass
    
    def compute_novelty_reward(
        self,
        position: Tuple[int, int],
        visited_positions: Set[Tuple[int, int]]
    ) -> float:
        """Reward pour nouveauté"""
        pass
    
    def compute_sterility_penalty(
        self,
        sterility_score: float
    ) -> float:
        """Pénalité pour stérilité"""
        pass
```

### Tests
- Test reward nouveauté
- Test pénalité stérilité
- Test reward connectivité
- Test impact sur décision

---

## CORRECTION 8 : COGNITIVE COST MODEL

### Fichier
`lumvorax2/src/MAGEN/core/cognitive_cost_model.py`

### Objectif
Estimer le coût réel d'une trajectoire

### Formulation
```python
trajectory_value = (
    expected_future_gain
    - cognitive_cost
    - sterility_penalty
)
```

### Interface
```python
class CognitiveCostModel:
    def __init__(self):
        self.total_steps: int = 0
        self.steps_per_region: Dict[str, int] = {}
        self.rewards_per_region: Dict[str, float] = {}
    
    def update_cost(
        self,
        region_id: str,
        steps: int,
        reward: float
    ) -> None:
        """Met à jour coût cognitif"""
        pass
    
    def compute_trajectory_value(
        self,
        expected_gain: float,
        steps_invested: int,
        sterility_score: float
    ) -> float:
        """Calcule valeur trajectoire"""
        pass
    
    def get_roi(
        self,
        region_id: str
    ) -> float:
        """Calcule ROI région"""
        pass
    
    def should_abandon_trajectory(
        self,
        trajectory_value: float,
        threshold: float = 0.0
    ) -> bool:
        """Détermine si abandonner trajectoire"""
        pass
```

### Tests
- Test calcul coût
- Test calcul ROI
- Test décision abandon
- Test réallocation

---

## CORRECTION 9 : GLOBAL TRAJECTORY MANAGER

### Fichier
`lumvorax2/src/MAGEN/core/global_trajectory_manager.py`

### Objectif
Gérer plusieurs stratégies concurrentes

### Interface
```python
class GlobalTrajectoryManager:
    def __init__(self):
        self.active_trajectories: List[Trajectory] = []
        self.completed_trajectories: List[Trajectory] = []
        self.budget_allocation: Dict[str, float] = {}
    
    def create_trajectory(
        self,
        strategy: str,
        target_region: str
    ) -> Trajectory:
        """Crée nouvelle trajectoire"""
        pass
    
    def allocate_budget(
        self,
        trajectories: List[Trajectory]
    ) -> Dict[str, float]:
        """Alloue budget entre trajectoires"""
        pass
    
    def select_best_trajectory(
        self,
        trajectories: List[Trajectory]
    ) -> Trajectory:
        """Sélectionne meilleure trajectoire"""
        pass
    
    def abandon_trajectory(
        self,
        trajectory: Trajectory,
        reason: str
    ) -> None:
        """Abandonne trajectoire"""
        pass
```

### Tests
- Test création trajectoires
- Test allocation budget
- Test sélection meilleure
- Test abandon trajectoire

---

## CORRECTION 10 : DECISION KERNEL V2

### Fichier
`lumvorax2/src/MAGEN/core/decision_kernel_v2.py`

### Objectif
Refondre complètement le DecisionKernel

### Formulation
```python
action_score = weighted_sum(
    reward_signal,
    topology_signal,
    novelty_signal,
    sterility_penalty,
    trajectory_value,
    causal_prediction,
    exploration_value
)
```

### Interface
```python
class DecisionKernelV2:
    def __init__(self):
        self.weights = {
            'reward': 0.3,
            'topology': 0.2,
            'novelty': 0.2,
            'sterility': -0.3,
            'trajectory': 0.2,
            'causal': 0.1
        }
    
    def compute_action_scores(
        self,
        state: Dict,
        world_model: PersistentWorldModel,
        sterility_memory: SterilityMemory,
        meta_reward: MetaRewardGenerator,
        cost_model: CognitiveCostModel
    ) -> Dict[str, float]:
        """Calcule scores actions avec tous signaux"""
        pass
    
    def select_action(
        self,
        action_scores: Dict[str, float],
        blacklisted: Set[str]
    ) -> str:
        """Sélectionne action finale"""
        pass
    
    def adapt_weights(
        self,
        performance_history: List[float]
    ) -> None:
        """Adapte poids signaux selon performance"""
        pass
```

### Tests
- Test fusion signaux
- Test sélection action
- Test adaptation poids
- Test robustesse

---

## CORRECTION 11 : FORENSIC LOGGING COMPLET

### Fichier
`lumvorax2/src/MAGEN/core/forensic_logger_v2.py`

### Objectif
Logging temps réel complet

### Métriques Loggées
```python
{
    'agent_identity': {
        'agent_id': int,
        'confidence': float,
        'position': Tuple[int, int]
    },
    'world_state': {
        'regions_visited': Set[str],
        'sterile_regions': Set[str],
        'coverage': float,
        'connectivity': int
    },
    'metacognition': {
        'policy_breaks': int,
        'trajectory_switches': int,
        'collapse_detections': int,
        'stagnation_steps': int
    },
    'performance': {
        'cognitive_cost': float,
        'meta_reward': float,
        'exploration_efficiency': float,
        'roi': float
    }
}
```

### Interface
```python
class ForensicLoggerV2:
    def __init__(self, log_dir: str):
        self.log_dir = Path(log_dir)
        self.current_log: Dict = {}
    
    def log_step(
        self,
        step: int,
        agent_anchor: AgentSelfAnchor,
        world_model: PersistentWorldModel,
        sterility_memory: SterilityMemory,
        meta_controller: MetaCognitiveController,
        cost_model: CognitiveCostModel
    ) -> None:
        """Log step complet"""
        pass
    
    def save_forensic_report(
        self,
        filename: str
    ) -> None:
        """Sauvegarde rapport forensique"""
        pass
```

### Tests
- Test logging complet
- Test sauvegarde
- Test visualisation
- Test analyse causale

---

## CORRECTION 12 : TESTS OBLIGATOIRES

### Structure Tests
```
tests/
├── test_agent_self_anchor.py
├── test_persistent_world_model.py
├── test_topological_memory.py
├── test_sterility_memory.py
├── test_meta_cognitive_controller.py
├── test_policy_break_system.py
├── test_meta_reward_generator.py
├── test_cognitive_cost_model.py
├── test_global_trajectory_manager.py
├── test_decision_kernel_v2.py
├── test_forensic_logger_v2.py
└── test_integration_v31.py
```

### Tests Unitaires (Chaque Module)
- Test fonctionnalités de base
- Test cas limites
- Test robustesse
- Test performance

### Tests Intégration
- Test intégration 2 modules
- Test intégration couche complète
- Test intégration 3 couches
- Test système complet

### Tests Validation
- Test vs V28 baseline
- Test environnements synthétiques
- Test ARC Arcade réels
- Test forensic complet

---

## PLAN D'EXÉCUTION (7 JOURS)

### Jour 8 : Couche Spatiale-Identitaire (Partie 1)

**Matin** :
- [ ] Implémenter `AgentSelfAnchor`
- [ ] Tests unitaires AgentSelfAnchor
- [ ] Implémenter `PersistentWorldModel` (structure base)

**Après-midi** :
- [ ] Compléter `PersistentWorldModel`
- [ ] Tests unitaires PersistentWorldModel
- [ ] Implémenter `TopologicalMemory`

**Soir** :
- [ ] Tests unitaires TopologicalMemory
- [ ] Tests intégration Anchor + WorldModel

### Jour 9 : Couche Spatiale-Identitaire (Partie 2)

**Matin** :
- [ ] Implémenter `SterilityMemory`
- [ ] Tests unitaires SterilityMemory
- [ ] Implémenter `MetaRewardGenerator`

**Après-midi** :
- [ ] Tests unitaires MetaRewardGenerator
- [ ] Tests intégration couche spatiale complète
- [ ] Validation environnements synthétiques

**Soir** :
- [ ] Corrections bugs couche spatiale
- [ ] Optimisations performance
- [ ] Documentation

### Jour 10 : Couche Métacognitive

**Matin** :
- [ ] Implémenter `MetaCognitiveController`
- [ ] Tests unitaires MetaCognitiveController
- [ ] Implémenter `PolicyBreakSystem`

**Après-midi** :
- [ ] Tests unitaires PolicyBreakSystem
- [ ] Implémenter `CognitiveCostModel`
- [ ] Tests unitaires CognitiveCostModel

**Soir** :
- [ ] Implémenter `GlobalTrajectoryManager`
- [ ] Tests unitaires GlobalTrajectoryManager
- [ ] Tests intégration couche métacognitive

### Jour 11 : Couche Décisionnelle Refonte

**Matin** :
- [ ] Implémenter `DecisionKernelV2`
- [ ] Tests unitaires DecisionKernelV2
- [ ] Implémenter `ForensicLoggerV2`

**Après-midi** :
- [ ] Tests unitaires ForensicLoggerV2
- [ ] Intégration 3 couches
- [ ] Tests intégration complète

**Soir** :
- [ ] Corrections bugs intégration
- [ ] Optimisations performance
- [ ] Validation environnements synthétiques

### Jour 12 : Validation et Tests Finaux

**Matin** :
- [ ] Tests comparatifs V28 vs V31
- [ ] Tests 7 environnements synthétiques
- [ ] Analyse forensique comparative

**Après-midi** :
- [ ] Tests 3 ARC Arcade réels
- [ ] Analyse forensique ARC
- [ ] Validation métriques succès

**Soir** :
- [ ] Rapport final V31
- [ ] Documentation complète
- [ ] Préparation déploiement

---

## MÉTRIQUES DE SUCCÈS V31

### Couche Spatiale-Identitaire

- ✅ Identifier avatar en < 10 steps (confiance > 0.8)
- ✅ Construire carte en < 500 steps (coverage > 50%)
- ✅ Identifier 3+ régions distinctes
- ✅ Mémoriser zones stériles (precision > 0.9)
- ✅ Éviter revisite zones mortes (reduction > 80%)

### Couche Métacognitive

- ✅ Détecter stagnation en < 1000 steps
- ✅ Casser collapse attractif (ACTION1 < 50%)
- ✅ Abandonner trajectoire stérile (< 2000 steps)
- ✅ Réallouer exploration (3+ trajectoires)
- ✅ Éviter boucles infinies (max 5000 steps)

### Performance Globale

- ✅ Résoudre > 50% puzzles (vs 42.9% V28)
- ✅ Pas de boucles > 5000 steps
- ✅ Couverture monde > 80%
- ✅ ROI cognitif > 0.1
- ✅ Variance actions > 0.2
- ✅ Scores non figés (variance > 0.1)

---

## TRANSFORMATION ATTENDUE

### AVANT (V28/V30)

```
Moteur réactif local
→ Exploration aveugle
→ Politique figée (ACTION1 85%)
→ Scores figés (0.5)
→ Boucles infinies (498K steps)
→ Aucune carte mentale
→ Aucun désinvestissement
```

### APRÈS (V31)

```
Agent cognitif cartographique
→ Mémoire topologique persistante
→ Exploration stratégique
→ Contrôle exécutif métacognitif
→ Gestion dynamique trajectoires
→ Carte mentale cumulative
→ Désinvestissement explicite
→ Notion de "NON"
```

---

## CONCLUSION

Ce plan transforme MAGEN d'un **moteur de sélection d'actions** en **système cognitif complet** capable de :

1. **Comprendre** où il est (Self-Anchor + WorldModel)
2. **Mémoriser** ce qu'il a découvert (TopologicalMemory)
3. **Apprendre** ce qui ne marche pas (SterilityMemory)
4. **Décider** quand abandonner (MetaCognitiveController)
5. **Réallouer** stratégiquement (TrajectoryManager)
6. **Dire "NON"** explicitement (PolicyBreakSystem)

**Prêt pour implémentation Phase 1 - Jour 8**

---

**Document généré** : 2026-06-15  
**Auteur** : Bob (Assistant IA)  
**Statut** : PLAN D'EXÉCUTION DÉTAILLÉ - PRÊT  
**Priorité** : CRITIQUE - DÉMARRAGE IMMÉDIAT

**FIN DU PLAN**