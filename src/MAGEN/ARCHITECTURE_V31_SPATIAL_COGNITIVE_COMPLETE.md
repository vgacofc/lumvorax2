# ARCHITECTURE MAGEN V31 - SPATIAL-COGNITIVE COMPLETE

**Date** : 2026-06-15  
**Session** : 73 - Jour 7 (Synthèse Finale)  
**Contexte** : Architecture complète post-découvertes majeures  
**Statut** : 🎯 SPÉCIFICATION ARCHITECTURALE COMPLÈTE

---

## RÉSUMÉ EXÉCUTIF

Ce document spécifie l'architecture complète MAGEN V31 basée sur trois découvertes majeures :

1. **Inertie cognitive computationnelle** (498K steps sans changement)
2. **Absence de désinvestissement cognitif** (pas de notion de "NON")
3. **Absence de modèle spatial-identitaire persistant** (pas de carte mentale)

**La solution nécessite une architecture en 3 couches** :

```
COUCHE 3: Métacognitive Négative (Désinvestissement)
    ↑
COUCHE 2: Spatiale-Identitaire (Cartographie)
    ↑
COUCHE 1: Décisionnelle (Existante)
```

---

## 1. LA CHAÎNE CAUSALE FONDAMENTALE

### Ce Qui Manque Actuellement

```
MAGEN fait:
perception locale → réaction locale
```

### Ce Qui Est Nécessaire

```
identité → localisation → cartographie → stratégie → désinvestissement
```

**C'est probablement l'ordre réel.**

### Pourquoi Cet Ordre

**Avant de pouvoir dire** :
```
"cette trajectoire est mauvaise"
```

**Il faut déjà pouvoir dire** :
```
"je suis dans cette région"
```

**Et avant ça** :
```
"cette entité, c'est moi"
```

---

## 2. DIFFÉRENCE FONDAMENTALE

### Mémoriser des Transitions vs Construire un Monde Interne

**MAGEN actuellement fait** :
```python
(state, action, next_state)
```

**Un vrai agent spatial fait** :
```
"je suis ici"
"cette zone existe"
"elle est reliée à cette autre"
"ce corridor mène à une impasse"
"cette région est stérile"
"cet endroit est nouveau"
"je suis déjà passé ici"
```

**Cette différence est fondamentale.**

---

## 3. WORLD MODEL UTILE ≠ PRÉDICTEUR DE TRANSITIONS

### Ce Que N'Est PAS un World Model Utile

❌ Juste un prédicteur de transitions
❌ Juste des statistiques d'actions
❌ Juste des micro-prédictions court terme

### Ce Qu'EST un World Model Utile

✅ **Compression structurelle du monde**
✅ **Géographie interne**
✅ **Mémoire des régions**
✅ **Mémoire de viabilité**
✅ **Localisation de soi dans cette structure**

---

## 4. ARCHITECTURE COMPLÈTE V31

### Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────────┐
│  COUCHE 3: MÉTACOGNITIVE NÉGATIVE                           │
│  (Désinvestissement, Notion de "NON")                       │
├─────────────────────────────────────────────────────────────┤
│  - Policy Disengagement Controller                          │
│  - Cognitive Cost Tracker                                   │
│  - Meta-Reward Generator                                    │
│  - Trajectory Viability Monitor                             │
└─────────────────────────────────────────────────────────────┘
                            ↑
┌─────────────────────────────────────────────────────────────┐
│  COUCHE 2: SPATIALE-IDENTITAIRE                             │
│  (Cartographie, Localisation, Territoire Mental)            │
├─────────────────────────────────────────────────────────────┤
│  - Spatial World Model (carte topologique)                  │
│  - Region Memory (valeur régions)                           │
│  - Trajectory Memory (trajectoires complètes)               │
│  - Self-Localization (position + identité)                  │
└─────────────────────────────────────────────────────────────┘
                            ↑
┌─────────────────────────────────────────────────────────────┐
│  COUCHE 1: DÉCISIONNELLE (EXISTANTE)                        │
│  (Perception, Action, Mémoire Court Terme)                  │
├─────────────────────────────────────────────────────────────┤
│  - World Model Minimal (transitions locales)                │
│  - MAGEN Memory (expériences, patterns)                     │
│  - Decision Kernel (sélection actions)                      │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. COUCHE 2 : SPATIALE-IDENTITAIRE (NOUVEAU)

### 5.1 Spatial World Model

**Responsabilité** : Construire et maintenir une carte topologique cumulative du monde

```python
class SpatialWorldModel:
    """
    Modèle spatial cumulatif du monde
    
    Principe:
    - Construit progressivement une carte topologique
    - Identifie régions, corridors, impasses
    - Mémorise structures spatiales persistantes
    - Permet navigation stratégique
    """
    
    def __init__(self):
        # Carte topologique
        self.regions: Dict[str, Region] = {}
        self.connections: List[Tuple[str, str]] = []
        
        # Structures identifiées
        self.landmarks: List[Landmark] = []
        self.corridors: List[Corridor] = []
        self.dead_ends: List[DeadEnd] = []
        
        # Position actuelle
        self.current_region_id: Optional[str] = None
        
        # Historique spatial
        self.visited_regions: Set[str] = set()
        self.exploration_frontier: Set[str] = set()
        
        # Statistiques
        self.total_regions_discovered = 0
        self.world_coverage = 0.0  # 0.0-1.0
    
    def update_from_observation(
        self,
        agent_pos: Tuple[int, int],
        observation: np.ndarray
    ) -> None:
        """
        Met à jour la carte à partir d'une observation
        
        Processus:
        1. Identifier région actuelle (ou créer nouvelle)
        2. Mettre à jour structures de la région
        3. Détecter connexions avec régions adjacentes
        4. Identifier landmarks
        5. Mettre à jour frontière d'exploration
        """
        pass
    
    def get_region_value(self, region_id: str) -> float:
        """
        Obtenir valeur estimée d'une région
        
        Returns:
            float: 0.0 (stérile) à 1.0 (très productive)
        """
        pass
    
    def find_path_to_region(
        self,
        target_region_id: str
    ) -> List[str]:
        """
        Trouver chemin vers région cible
        
        Returns:
            Liste d'IDs de régions formant le chemin
        """
        pass
    
    def identify_unexplored_regions(self) -> List[str]:
        """
        Identifier régions non encore explorées
        
        Returns:
            Liste d'IDs de régions à la frontière
        """
        pass
```

### 5.2 Region

**Responsabilité** : Représenter une région du monde avec son historique et sa valeur

```python
@dataclass
class Region:
    """
    Une région du monde
    
    Propriétés:
    - Géométrie (bounds, center)
    - Historique (visites, temps passé)
    - Valeur (rewards, success_rate, sterility_score)
    - Connexions (régions adjacentes)
    """
    
    region_id: str
    bounds: Tuple[int, int, int, int]  # (x_min, y_min, x_max, y_max)
    center: Tuple[int, int]
    
    # Historique
    visit_count: int = 0
    last_visited: Optional[float] = None
    total_time_spent: float = 0.0
    
    # Valeur
    rewards_obtained: List[float] = field(default_factory=list)
    success_rate: float = 0.0
    sterility_score: float = 0.0  # 0=productive, 1=stérile
    
    # Connexions
    connected_regions: List[str] = field(default_factory=list)
    access_points: List[Tuple[int, int]] = field(default_factory=list)
    
    # Structures
    landmarks: List[str] = field(default_factory=list)
    obstacles: List[Tuple[int, int]] = field(default_factory=list)
    
    def update_value(self, reward: float) -> None:
        """Met à jour la valeur de la région"""
        self.rewards_obtained.append(reward)
        
        # Calculer success_rate
        if self.rewards_obtained:
            self.success_rate = sum(
                1 for r in self.rewards_obtained if r > 0
            ) / len(self.rewards_obtained)
        
        # Calculer sterility_score
        # Plus de visites sans reward → plus stérile
        if self.visit_count > 10 and self.success_rate < 0.1:
            self.sterility_score = min(1.0, self.visit_count / 50.0)
    
    def is_sterile(self, threshold: float = 0.7) -> bool:
        """Détermine si région est stérile"""
        return self.sterility_score >= threshold
```

### 5.3 Trajectory Memory

**Responsabilité** : Mémoriser trajectoires spatiales complètes et identifier trajectoires stériles

```python
@dataclass
class Trajectory:
    """
    Une trajectoire spatiale complète
    
    Propriétés:
    - Séquence de régions visitées
    - Actions prises
    - Rewards obtenus
    - Durée totale
    - Outcome final
    """
    
    trajectory_id: str
    regions_visited: List[str]
    actions_taken: List[str]
    rewards_obtained: List[float]
    total_duration: float
    outcome: str  # "success", "failure", "abandoned"
    
    def is_sterile(self) -> bool:
        """Détermine si trajectoire est stérile"""
        # Stérile si: longue durée, peu de rewards, échec
        return (
            self.total_duration > 1000 and
            sum(self.rewards_obtained) < 1.0 and
            self.outcome == "failure"
        )
    
    def similarity(self, other: 'Trajectory') -> float:
        """Calcule similarité avec autre trajectoire"""
        # Similarité basée sur régions visitées
        common_regions = set(self.regions_visited) & set(other.regions_visited)
        total_regions = set(self.regions_visited) | set(other.regions_visited)
        
        if not total_regions:
            return 0.0
        
        return len(common_regions) / len(total_regions)


class TrajectoryMemory:
    """
    Mémoire des trajectoires spatiales
    
    Fonctionnalités:
    - Mémoriser trajectoires complètes
    - Identifier trajectoires stériles
    - Détecter patterns de trajectoires
    - Éviter répétition trajectoires stériles
    """
    
    def __init__(self):
        # Trajectoires complètes
        self.trajectories: List[Trajectory] = []
        
        # Trajectoires stériles identifiées
        self.sterile_trajectories: Set[str] = set()
        
        # Trajectoires productives
        self.productive_trajectories: List[Trajectory] = []
        
        # Trajectoire actuelle en cours
        self.current_trajectory: Optional[Trajectory] = None
    
    def start_trajectory(self) -> None:
        """Démarre une nouvelle trajectoire"""
        self.current_trajectory = Trajectory(
            trajectory_id=f"traj_{time.time()}",
            regions_visited=[],
            actions_taken=[],
            rewards_obtained=[],
            total_duration=0.0,
            outcome="in_progress"
        )
    
    def update_trajectory(
        self,
        region_id: str,
        action: str,
        reward: float
    ) -> None:
        """Met à jour trajectoire actuelle"""
        if self.current_trajectory is None:
            self.start_trajectory()
        
        self.current_trajectory.regions_visited.append(region_id)
        self.current_trajectory.actions_taken.append(action)
        self.current_trajectory.rewards_obtained.append(reward)
    
    def end_trajectory(self, outcome: str) -> None:
        """Termine trajectoire actuelle"""
        if self.current_trajectory is None:
            return
        
        self.current_trajectory.outcome = outcome
        
        # Ajouter à historique
        self.trajectories.append(self.current_trajectory)
        
        # Classifier
        if self.current_trajectory.is_sterile():
            self.sterile_trajectories.add(self.current_trajectory.trajectory_id)
        elif outcome == "success":
            self.productive_trajectories.append(self.current_trajectory)
        
        # Reset
        self.current_trajectory = None
    
    def is_similar_to_sterile(
        self,
        current_regions: List[str],
        threshold: float = 0.7
    ) -> bool:
        """
        Vérifie si trajectoire actuelle ressemble à trajectoire stérile
        
        Returns:
            True si similarité > threshold avec trajectoire stérile
        """
        if not current_regions:
            return False
        
        # Créer trajectoire temporaire
        temp_traj = Trajectory(
            trajectory_id="temp",
            regions_visited=current_regions,
            actions_taken=[],
            rewards_obtained=[],
            total_duration=0.0,
            outcome="in_progress"
        )
        
        # Comparer avec trajectoires stériles
        for traj in self.trajectories:
            if traj.trajectory_id in self.sterile_trajectories:
                similarity = temp_traj.similarity(traj)
                if similarity >= threshold:
                    return True
        
        return False
```

### 5.4 Self-Localization

**Responsabilité** : Localiser l'agent dans le monde et maintenir ancrage identitaire

```python
class SelfLocalization:
    """
    Localisation de l'agent dans le monde
    
    Fonctionnalités:
    - Identifier avatar contrôlé (ancrage identitaire)
    - Tracker position actuelle
    - Maintenir historique positions
    - Estimer confiance dans localisation
    """
    
    def __init__(self):
        # Ancrage identitaire
        self.avatar_entity_id: Optional[int] = None
        self.control_confidence: float = 0.0
        
        # Position actuelle
        self.current_position: Optional[Tuple[int, int]] = None
        self.current_region_id: Optional[str] = None
        
        # Historique positions
        self.position_history: List[Tuple[float, Tuple[int, int]]] = []
        
        # Statistiques
        self.total_movements = 0
        self.successful_localizations = 0
    
    def identify_avatar(
        self,
        entities: List[Dict[str, Any]],
        action: str,
        prev_entities: Optional[List[Dict[str, Any]]] = None
    ) -> Optional[int]:
        """
        Identifie l'avatar contrôlé (heuristique premier mouvement)
        
        Principe:
        - Exécute action
        - Observe quelle entité bouge
        - Cette entité = avatar
        
        Returns:
            ID de l'entité avatar ou None
        """
        if prev_entities is None:
            return None
        
        # Comparer positions avant/après
        for i, entity in enumerate(entities):
            if i >= len(prev_entities):
                continue
            
            prev_entity = prev_entities[i]
            
            # Détecter mouvement
            if entity['pos'] != prev_entity['pos']:
                # Cette entité a bougé → probablement l'avatar
                self.avatar_entity_id = i
                self.control_confidence = min(1.0, self.control_confidence + 0.2)
                return i
        
        return None
    
    def update_position(
        self,
        entities: List[Dict[str, Any]],
        spatial_model: 'SpatialWorldModel'
    ) -> None:
        """
        Met à jour position actuelle
        
        Args:
            entities: Liste entités observées
            spatial_model: Modèle spatial pour identifier région
        """
        if self.avatar_entity_id is None:
            return
        
        if self.avatar_entity_id >= len(entities):
            return
        
        # Obtenir position avatar
        avatar = entities[self.avatar_entity_id]
        self.current_position = tuple(avatar['pos'])
        
        # Identifier région actuelle
        self.current_region_id = spatial_model.get_region_at_position(
            self.current_position
        )
        
        # Ajouter à historique
        self.position_history.append((time.time(), self.current_position))
        
        # Limiter historique
        if len(self.position_history) > 1000:
            self.position_history = self.position_history[-1000:]
        
        # Statistiques
        self.total_movements += 1
        if self.current_region_id is not None:
            self.successful_localizations += 1
    
    def get_localization_confidence(self) -> float:
        """
        Confiance dans la localisation actuelle
        
        Returns:
            float: 0.0-1.0
        """
        if self.total_movements == 0:
            return 0.0
        
        return self.successful_localizations / self.total_movements
```

---

## 6. COUCHE 3 : MÉTACOGNITIVE NÉGATIVE (NOUVEAU)

### 6.1 Policy Disengagement Controller

**Responsabilité** : Décider quand abandonner une politique/trajectoire

```python
class PolicyDisengagementController:
    """
    Contrôleur de désinvestissement cognitif
    
    Fonctionnalités:
    - Monitorer viabilité trajectoire actuelle
    - Détecter stagnation cognitive
    - Décider quand abandonner
    - Forcer rupture stratégique
    - Réallouer ressources cognitives
    """
    
    def __init__(
        self,
        stagnation_threshold: int = 1000,
        sterility_threshold: float = 0.7
    ):
        self.stagnation_threshold = stagnation_threshold
        self.sterility_threshold = sterility_threshold
        
        # État actuel
        self.steps_without_progress = 0
        self.current_policy_confidence = 1.0
        
        # Historique
        self.disengagements: List[Dict[str, Any]] = []
    
    def should_disengage(
        self,
        trajectory_memory: TrajectoryMemory,
        spatial_model: SpatialWorldModel,
        current_region_id: str
    ) -> Tuple[bool, str]:
        """
        Détermine si doit abandonner politique actuelle
        
        Returns:
            (should_disengage, reason)
        """
        # Raison 1: Stagnation temporelle
        if self.steps_without_progress >= self.stagnation_threshold:
            return True, "temporal_stagnation"
        
        # Raison 2: Région stérile
        region = spatial_model.regions.get(current_region_id)
        if region and region.is_sterile(self.sterility_threshold):
            return True, "sterile_region"
        
        # Raison 3: Trajectoire similaire à trajectoire stérile
        if trajectory_memory.current_trajectory:
            regions = trajectory_memory.current_trajectory.regions_visited
            if trajectory_memory.is_similar_to_sterile(regions):
                return True, "sterile_trajectory_pattern"
        
        # Raison 4: Confiance politique trop basse
        if self.current_policy_confidence < 0.1:
            return True, "low_policy_confidence"
        
        return False, ""
    
    def execute_disengagement(
        self,
        reason: str,
        spatial_model: SpatialWorldModel
    ) -> Dict[str, Any]:
        """
        Exécute désinvestissement et réallocation
        
        Returns:
            Nouvelle stratégie à adopter
        """
        # Logger désinvestissement
        disengagement = {
            'timestamp': time.time(),
            'reason': reason,
            'steps_invested': self.steps_without_progress
        }
        self.disengagements.append(disengagement)
        
        # Reset compteurs
        self.steps_without_progress = 0
        self.current_policy_confidence = 1.0
        
        # Identifier nouvelle stratégie
        unexplored = spatial_model.identify_unexplored_regions()
        
        if unexplored:
            # Stratégie: Explorer région non visitée
            target_region = unexplored[0]
            return {
                'strategy': 'explore_new_region',
                'target_region': target_region
            }
        else:
            # Stratégie: Revisiter région productive
            productive_regions = [
                r for r in spatial_model.regions.values()
                if r.success_rate > 0.5 and not r.is_sterile()
            ]
            
            if productive_regions:
                target_region = max(productive_regions, key=lambda r: r.success_rate)
                return {
                    'strategy': 'revisit_productive_region',
                    'target_region': target_region.region_id
                }
            else:
                # Stratégie: Reset complet
                return {
                    'strategy': 'full_reset'
                }
```

### 6.2 Cognitive Cost Tracker

**Responsabilité** : Tracker coût cognitif accumulé et détecter gaspillage

```python
class CognitiveCostTracker:
    """
    Tracker de coût cognitif
    
    Fonctionnalités:
    - Comptabiliser steps investis
    - Calculer coût/bénéfice
    - Détecter gaspillage computationnel
    - Estimer ROI cognitif
    """
    
    def __init__(self):
        # Coûts accumulés
        self.total_steps = 0
        self.steps_per_region: Dict[str, int] = {}
        self.steps_per_trajectory: Dict[str, int] = {}
        
        # Bénéfices obtenus
        self.total_rewards = 0.0
        self.rewards_per_region: Dict[str, float] = {}
    
    def update(
        self,
        region_id: str,
        trajectory_id: str,
        reward: float
    ) -> None:
        """Met à jour coûts et bénéfices"""
        self.total_steps += 1
        
        self.steps_per_region[region_id] = \
            self.steps_per_region.get(region_id, 0) + 1
        
        self.steps_per_trajectory[trajectory_id] = \
            self.steps_per_trajectory.get(trajectory_id, 0) + 1
        
        self.total_rewards += reward
        self.rewards_per_region[region_id] = \
            self.rewards_per_region.get(region_id, 0.0) + reward
    
    def get_region_roi(self, region_id: str) -> float:
        """
        Calcule ROI d'une région
        
        Returns:
            float: rewards / steps (0.0 = gaspillage total)
        """
        steps = self.steps_per_region.get(region_id, 0)
        rewards = self.rewards_per_region.get(region_id, 0.0)
        
        if steps == 0:
            return 0.0
        
        return rewards / steps
    
    def identify_wasteful_regions(
        self,
        threshold: float = 0.01
    ) -> List[str]:
        """
        Identifie régions avec ROI < threshold
        
        Returns:
            Liste d'IDs de régions gaspilleuses
        """
        wasteful = []
        
        for region_id in self.steps_per_region:
            roi = self.get_region_roi(region_id)
            if roi < threshold:
                wasteful.append(region_id)
        
        return wasteful
```

---

## 7. INTÉGRATION COMPLÈTE

### 7.1 MAGEN Agent V31

```python
class MAGENAgentV31:
    """
    Agent MAGEN V31 - Architecture Spatial-Cognitive Complete
    
    Intègre:
    - Couche 1: Décisionnelle (existante)
    - Couche 2: Spatiale-Identitaire (nouveau)
    - Couche 3: Métacognitive Négative (nouveau)
    """
    
    def __init__(self):
        # Couche 1: Décisionnelle
        self.world_model = WorldModelMinimal()
        self.memory = MAGENMemory()
        self.decision_kernel = DecisionKernelMinimal()
        
        # Couche 2: Spatiale-Identitaire
        self.spatial_model = SpatialWorldModel()
        self.trajectory_memory = TrajectoryMemory()
        self.self_localization = SelfLocalization()
        
        # Couche 3: Métacognitive Négative
        self.disengagement_controller = PolicyDisengagementController()
        self.cost_tracker = CognitiveCostTracker()
    
    def step(
        self,
        observation: Dict[str, Any]
    ) -> str:
        """
        Step complet avec toutes les couches
        
        Processus:
        1. Perception (Couche 1)
        2. Localisation (Couche 2)
        3. Mise à jour carte (Couche 2)
        4. Vérification désinvestissement (Couche 3)
        5. Décision action (Couche 1)
        6. Mise à jour coûts (Couche 3)
        """
        # 1. Perception
        state = self._process_observation(observation)
        
        # 2. Localisation
        self.self_localization.update_position(
            state['entities'],
            self.spatial_model
        )
        
        # 3. Mise à jour carte
        if self.self_localization.current_position:
            self.spatial_model.update_from_observation(
                self.self_localization.current_position,
                state['grid']
            )
        
        # 4. Vérification désinvestissement
        should_disengage, reason = self.disengagement_controller.should_disengage(
            self.trajectory_memory,
            self.spatial_model,
            self.self_localization.current_region_id
        )
        
        if should_disengage:
            # Exécuter désinvestissement
            new_strategy = self.disengagement_controller.execute_disengagement(
                reason,
                self.spatial_model
            )
            
            # Terminer trajectoire actuelle
            self.trajectory_memory.end_trajectory("abandoned")
            
            # Démarrer nouvelle trajectoire
            self.trajectory_memory.start_trajectory()
            
            # Adapter décision selon nouvelle stratégie
            action = self._execute_strategy(new_strategy)
        else:
            # Décision normale
            action = self.decision_kernel.select_action(state)
        
        # 6. Mise à jour coûts
        self.cost_tracker.update(
            self.self_localization.current_region_id,
            self.trajectory_memory.current_trajectory.trajectory_id,
            state.get('reward', 0.0)
        )
        
        return action
```

---

## 8. MÉTRIQUES DE SUCCÈS V31

### 8.1 Couche Spatiale-Identitaire

- ✅ Identifier avatar en < 10 steps
- ✅ Construire carte en < 500 steps
- ✅ Identifier 3+ régions distinctes
- ✅ Localiser agent avec confiance > 0.8
- ✅ Mémoriser zones stériles
- ✅ Éviter revisite zones mortes

### 8.2 Couche Métacognitive Négative

- ✅ Détecter stagnation en < 1000 steps
- ✅ Abandonner politique stérile
- ✅ Réallouer vers nouvelle stratégie
- ✅ Éviter boucles infinies
- ✅ Dire "NON" explicitement

### 8.3 Performance Globale

- ✅ Résoudre > 50% puzzles (vs 42.9% V28)
- ✅ Pas de boucles > 5000 steps
- ✅ Couverture monde > 80%
- ✅ ROI cognitif > 0.1

---

## 9. PLAN D'IMPLÉMENTATION

### Phase 1 : Couche Spatiale-Identitaire (3 jours)

**Jour 1** :
- Implémenter `SpatialWorldModel`
- Implémenter `Region`
- Tests unitaires

**Jour 2** :
- Implémenter `TrajectoryMemory`
- Implémenter `SelfLocalization`
- Tests unitaires

**Jour 3** :
- Intégration Couche 2
- Tests intégration
- Validation sur environnements synthétiques

### Phase 2 : Couche Métacognitive Négative (2 jours)

**Jour 4** :
- Implémenter `PolicyDisengagementController`
- Implémenter `CognitiveCostTracker`
- Tests unitaires

**Jour 5** :
- Intégration Couche 3
- Tests intégration
- Validation désinvestissement

### Phase 3 : Intégration Complète (2 jours)

**Jour 6** :
- Implémenter `MAGENAgentV31`
- Intégration 3 couches
- Tests complets

**Jour 7** :
- Validation ARC Arcade
- Analyse forensique
- Rapport final

---

## 10. CONCLUSION

### Découvertes Majeures

1. **Inertie cognitive** : 498K steps sans changement
2. **Absence de "NON"** : Pas de désinvestissement
3. **Absence de carte** : Pas de modèle spatial persistant

### Architecture Solution

```
monde → structure → régions → trajectoires → stratégie → désinvestissement
```

### Impact Attendu

**Avant V31** :
- 498K steps → 0 reward → boucle infinie

**Après V31** :
- 500 steps → carte construite
- 1000 steps → détection stagnation
- 1500 steps → abandon explicite
- 2000 steps → nouvelle stratégie
- **OU** : succès avant abandon

### Citation Finale

> "Un world model utile n'est pas juste un prédicteur de transitions. C'est une compression structurelle du monde, une géographie interne, une mémoire des régions, une mémoire de viabilité, et une localisation de soi dans cette structure."

---

**Document généré** : 2026-06-15  
**Auteur** : Bob (Assistant IA)  
**Statut** : SPÉCIFICATION ARCHITECTURALE COMPLÈTE V31  
**Priorité** : CRITIQUE - PRÊT POUR IMPLÉMENTATION

**FIN DU DOCUMENT**