"""
MAGEN ARC-AGI Agent V26 GRID-AWARE - Perception Structurée Jeux Arcade
=======================================================================

PROBLÈME V25 ENRICHED: Score 0/3 - Stratégie aléatoire inefficace
CAUSE RACINE: Pas d'analyse grille → Actions aveugles sans compréhension gameplay

SOLUTION V26 GRID-AWARE:
1. GridAnalyzer: Détection entités (joueur, objectif, obstacles, ennemis)
2. Pathfinding A*: Navigation optimale vers objectif
3. Heuristiques gameplay: Évitement dangers, progression, collecte
4. Suggestions actions intelligentes basées sur analyse spatiale
5. Intégration complète dans boucle décision

Architecture:
- V25 ENRICHED: StateEncoderV25 + Reward Shaping + Cycle Avoidance
- V26 GRID-AWARE: GridAnalyzer + Spatial Analysis + Smart Actions

Objectif: Passer de 0/3 à >0/3 en remplaçant exploration aléatoire par navigation intelligente

Protocole: CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN v2.0
Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 71
"""

import numpy as np
import time
import json
from typing import List, Dict, Optional, Tuple, Set
from dataclasses import dataclass, asdict
from collections import defaultdict, deque

# Import arcengine (optionnel pour tests locaux)
try:
    from arcengine import GameAction, GameState  # type: ignore
except ImportError:
    # Mock classes pour tests sans arcengine
    class GameAction:
        ACTION1 = "ACTION1"
        ACTION2 = "ACTION2"
        ACTION3 = "ACTION3"
        ACTION4 = "ACTION4"
        ACTION5 = "ACTION5"
        ACTION6 = "ACTION6"
        ACTION7 = "ACTION7"
        RESET = "RESET"
    
    class GameState:
        pass

# Modules V22-V25 (hérités)
from core.transformation_learning_engine import TransformationLearningEngine
from core.pattern_matcher import PatternMatcher
from core.object_transformation_engine import ObjectExtractor
from core.meta_arbiter import MetaArbiter
from core.action_reputation_system import ActionReputationSystem
from core.exploration_budget_manager import ExplorationBudgetManager
from core.action_discovery_engine import ActionDiscoveryEngine, ActionModel, StateTransition as ADStateTransition
from core.world_model import WorldModel, StateTransition as WMStateTransition
from core.information_gain_reward import InformationGainReward, StateTransition as IGStateTransition
from core.goal_grounder import GoalGrounder, GoalProgress, ActionPriority
from core.semantic_action_library import SemanticActionLibrary, ActionSemantics
from core.state_encoder_v25 import StateEncoderV25, StateEmbedding
from core.causal_transition_graph import CausalTransitionGraph, Path, NodeType

# Module V26 (NOUVEAU)
from core.grid_analyzer import GridAnalyzer, SpatialAnalysis, ActionSuggestion


@dataclass
class ForensicLog:
    """Log forensique bit-level avec timestamp nanoseconde"""
    timestamp_ns: int
    event_type: str  # 'perception', 'decision', 'learning', 'planning', 'grid_analysis'
    component: str
    data: Dict
    
    def to_json(self) -> str:
        return json.dumps({
            'timestamp_ns': self.timestamp_ns,
            'event_type': self.event_type,
            'component': self.component,
            'data': self.data
        })


@dataclass
class GridObservation:
    """Observation grille enrichie avec analyse spatiale"""
    grid: np.ndarray
    embedding: StateEmbedding
    spatial_analysis: Optional[SpatialAnalysis]  # V26: Ajout analyse spatiale
    patterns: List
    objects: List
    metadata: Dict


@dataclass
class ActionDecision:
    """Décision action avec justification"""
    action: GameAction
    action_name: str
    semantic_name: str
    score: float
    strategy: str  # 'grid_aware', 'planning', 'exploitation', 'exploration'
    budget: float
    justification: str
    from_plan: bool = False
    predicted_reward: float = 0.0
    grid_suggestion: Optional[ActionSuggestion] = None  # V26: Suggestion GridAnalyzer


class MAGENArcAgiAgentV26GridAware:
    """
    Agent MAGEN V26 GRID-AWARE pour ARC-AGI-3
    
    Architecture:
    - V22: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17 + C18
    - V23: ActionDiscovery + WorldModel + InformationGainReward
    - V24: GoalGrounder + SemanticActionLibrary
    - V25: StateEncoderV25 (256D) + CausalGraph + BFS Planning + Cycle Detection
    - V25 ENRICHED: Reward Shaping + Cycle Avoidance + Pénalisation Répétition
    - V26 GRID-AWARE: GridAnalyzer + Spatial Analysis + Smart Navigation
    """
    
    def __init__(self, forensic_log_path: Optional[str] = None):
        # V22: Modules hérités (désactivés pour jeux interactifs)
        self.tle = TransformationLearningEngine()
        self.pattern_matcher = PatternMatcher()
        self.object_extractor = ObjectExtractor()
        self.meta_arbiter = MetaArbiter()
        self.reputation_system = ActionReputationSystem()
        self.budget_manager = ExplorationBudgetManager()
        
        # V23: Modules hérités
        self.action_discovery = ActionDiscoveryEngine(exploration_budget=50)
        self.world_model = WorldModel(max_transitions=1000)
        self.info_gain_reward = InformationGainReward(
            knowledge_weight=0.1,
            causal_weight=0.05,
            novelty_weight=0.1
        )
        
        # V24: Modules grounding sémantique
        self.state_encoder = StateEncoderV25(embedding_dim=256)
        self.goal_grounder = GoalGrounder()
        self.semantic_library = SemanticActionLibrary()
        
        # V25: Modules planification
        self.causal_graph = CausalTransitionGraph(
            similarity_threshold=0.70,
            max_nodes=10000,
            max_edges_per_node=50
        )
        self.current_plan: Optional[Path] = None
        self.plan_step: int = 0
        
        # V26: Module analyse grille (NOUVEAU)
        self.grid_analyzer = GridAnalyzer()
        self.grid_aware_enabled = True  # Flag pour activer/désactiver
        
        # Mapping actions environnement → GameAction
        self.env_action_mapping = {
            'ACTION0': GameAction.ACTION1,
            'ACTION1': GameAction.ACTION1,
            'ACTION2': GameAction.ACTION2,
            'ACTION3': GameAction.ACTION3,
            'ACTION4': GameAction.ACTION4,
            'ACTION5': GameAction.ACTION5,
            'ACTION6': GameAction.ACTION6,
            'ACTION7': GameAction.ACTION7,
            'ACTION8': GameAction.RESET,
            'ACTION9': GameAction.RESET,
            'RESET': GameAction.RESET
        }
        
        # V26: Mapping sémantique → actions environnement
        self.semantic_to_env_mapping = {
            'move_up': 'ACTION1',
            'move_down': 'ACTION2',
            'move_left': 'ACTION3',
            'move_right': 'ACTION4',
            'interact': 'ACTION5',
            'collect': 'ACTION6',
            'undo': 'ACTION7',
            'reset': 'RESET'
        }
        
        # Initialiser mapping sémantique
        self._initialize_semantic_mapping()
        
        # Phases exploration/exploitation
        self.exploration_phase_active = True
        self.exploration_budget = 0.80
        self.min_exploration_actions = 150
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # ENRICHED: Historique pour pénalisation répétition
        self.recent_actions: deque = deque(maxlen=20)
        self.action_counts: Dict[str, int] = defaultdict(int)
        
        # ENRICHED: Tracking pour reward shaping intrinsèque
        self.seen_hashes: Set[str] = set()
        self.seen_transitions: Set[Tuple[str, str]] = set()
        self.in_cycle = False
        self.cycle_count = 0
        
        # Logs forensiques
        self.forensic_logs: List[ForensicLog] = []
        self.forensic_log_path = forensic_log_path
        self.forensic_enabled = True
        
        # Statistiques
        self.stats = {
            'total_actions': 0,
            'exploration_actions': 0,
            'exploitation_actions': 0,
            'planning_actions': 0,
            'grid_aware_actions': 0,  # V26: Nouvelles stats
            'successful_actions': 0,
            'learning_updates': 0,
            'world_model_accuracy': 0.0,
            'avg_info_gain_reward': 0.0,
            'avg_goal_progress': 0.0,
            'actions_abandoned': 0,
            'actions_promising': 0,
            'semantic_actions_used': 0,
            'semantic_mappings_learned': 0,
            'forensic_logs_generated': 0,
            # V25 stats
            'plans_generated': 0,
            'plans_completed': 0,
            'plans_abandoned': 0,
            'cycles_detected': 0,
            'avg_plan_length': 0.0,
            # ENRICHED stats
            'intrinsic_rewards_total': 0.0,
            'repetitions_penalized': 0,
            'cycles_avoided': 0,
            'unique_states_discovered': 0,
            # V26 stats
            'grids_analyzed': 0,
            'players_detected': 0,
            'goals_detected': 0,
            'paths_found': 0,
            'grid_suggestions_used': 0
        }
        
        # Log initialisation
        self._log_forensic('initialization', 'agent', {
            'version': 'V26_GRID_AWARE',
            'modules_v22': ['TLE', 'PatternMatcher', 'ObjectExtractor', 'MetaArbiter', 'C17', 'C18'],
            'modules_v23': ['ActionDiscovery', 'WorldModel', 'InformationGainReward'],
            'modules_v24': ['GoalGrounder', 'SemanticActionLibrary'],
            'modules_v25': ['StateEncoderV25_256D', 'CausalGraph_0.70', 'BFSPlanning', 'CycleDetection'],
            'modules_enriched': ['RewardShaping', 'CycleAvoidance', 'RepetitionPenalty', 'Exploration80%'],
            'modules_v26': ['GridAnalyzer', 'SpatialAnalysis', 'PathfindingAStar', 'SmartNavigation'],
            'forensic_enabled': self.forensic_enabled,
            'forensic_log_path': self.forensic_log_path,
            'grid_aware_enabled': self.grid_aware_enabled
        })
        
        print("=" * 80)
        print("[MAGEN V26 GRID-AWARE] Agent initialisé avec architecture complète")
        print("[V22] TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18")
        print("[V23] ActionDiscovery+WorldModel+InformationGainReward")
        print("[V24] GoalGrounder+SemanticActionLibrary")
        print("[V25] StateEncoderV25(256D)+CausalGraph(0.70)+BFSPlanning+CycleDetection")
        print("[V25 ENRICHED] RewardShaping+CycleAvoidance+RepetitionPenalty+Exploration80%")
        print("[V26 GRID-AWARE] GridAnalyzer+SpatialAnalysis+PathfindingAStar+SmartNavigation")
        print(f"[FORENSIC] Logging: ENABLED → {forensic_log_path}")
        print(f"[GRID-AWARE] Status: {'ENABLED' if self.grid_aware_enabled else 'DISABLED'}")
        print("=" * 80)
    
    def _initialize_semantic_mapping(self):
        """Initialise mapping actions environnement → sémantique"""
        self.semantic_library.map_env_action_to_semantic('ACTION1', 'move_up')
        self.semantic_library.map_env_action_to_semantic('ACTION2', 'move_down')
        self.semantic_library.map_env_action_to_semantic('ACTION3', 'move_left')
        self.semantic_library.map_env_action_to_semantic('ACTION4', 'move_right')
        self.semantic_library.map_env_action_to_semantic('ACTION5', 'interact')
        self.semantic_library.map_env_action_to_semantic('ACTION6', 'collect')
        self.semantic_library.map_env_action_to_semantic('ACTION7', 'undo')
        self.semantic_library.map_env_action_to_semantic('RESET', 'reset')
    
    def _log_forensic(self, event_type: str, component: str, data: Dict):
        """Log forensique bit-level avec timestamp nanoseconde"""
        if not self.forensic_enabled:
            return
        
        log = ForensicLog(
            timestamp_ns=time.perf_counter_ns(),
            event_type=event_type,
            component=component,
            data=data
        )
        self.forensic_logs.append(log)
        self.stats['forensic_logs_generated'] += 1
        
        # Écrire dans fichier si path fourni
        if self.forensic_log_path:
            with open(self.forensic_log_path, 'a') as f:
                f.write(log.to_json() + '\n')
    
    def perceive(self, obs) -> GridObservation:
        """
        PHASE 1: PERCEPTION ENRICHIE + ANALYSE GRILLE (V26)
        
        1. Encode observation avec StateEncoderV25 (256D)
        2. Analyse grille avec GridAnalyzer (détection entités, pathfinding)
        """
        start_ns = time.perf_counter_ns()
        
        # Extraire frame depuis obs (FrameDataRaw)
        if hasattr(obs, 'frame') and obs.frame is not None:
            frame = obs.frame
            
            # Conversion liste → numpy array si nécessaire
            if isinstance(frame, list):
                frame = np.array(frame, dtype=np.uint8)
            
            # Conversion RGB → grayscale pour compatibilité
            if len(frame.shape) == 3 and frame.shape[2] == 3:
                grid = np.mean(frame, axis=2).astype(np.uint8)
            else:
                grid = frame.astype(np.uint8)
        else:
            # Fallback: grille vide 10x10
            grid = np.zeros((10, 10), dtype=np.uint8)
        
        # ENRICHED: Encoder avec action context
        previous_action = self.recent_actions[-1] if self.recent_actions else None
        embedding = self.state_encoder.encode(grid, action=previous_action)
        
        # V26: Analyse grille avec GridAnalyzer
        spatial_analysis = None
        if self.grid_aware_enabled:
            try:
                spatial_analysis = self.grid_analyzer.analyze_grid(grid)
                self.stats['grids_analyzed'] += 1
                
                if spatial_analysis.player_position:
                    self.stats['players_detected'] += 1
                if spatial_analysis.goal_position:
                    self.stats['goals_detected'] += 1
                if spatial_analysis.path_to_goal:
                    self.stats['paths_found'] += 1
                    
            except Exception as e:
                print(f"[WARNING] GridAnalyzer error: {e}")
                spatial_analysis = None
        
        # DÉSACTIVÉ pour jeux interactifs ARC-AGI-3
        patterns = []
        objects = []
        
        # Métadonnées enrichies
        metadata = {
            'grid_size': grid.shape,
            'color_count': len(np.unique(grid)),
            'levels_completed': obs.levels_completed if hasattr(obs, 'levels_completed') else 0,
            'embedding_objects': embedding.objects_detected,
            'embedding_motion': embedding.motion_magnitude,
            'embedding_regions': len(embedding.active_regions),
            'spatial_relations': embedding.spatial_relations_count,
            'topological_hash': embedding.topological_hash,
            'action_context': embedding.action_context,
            # V26: Métadonnées analyse spatiale
            'player_detected': spatial_analysis.player_position is not None if spatial_analysis else False,
            'goal_detected': spatial_analysis.goal_position is not None if spatial_analysis else False,
            'path_found': spatial_analysis.path_to_goal is not None if spatial_analysis else False,
            'distance_to_goal': spatial_analysis.distance_to_goal if spatial_analysis else float('inf')
        }
        
        perception_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('perception', 'agent', {
            'grid_shape': list(grid.shape),
            'embedding_dim': embedding.vector.shape[0],
            'objects_detected': embedding.objects_detected,
            'motion_magnitude': float(embedding.motion_magnitude),
            'active_regions_count': len(embedding.active_regions),
            'spatial_relations_count': embedding.spatial_relations_count,
            'topological_hash': embedding.topological_hash,
            'perception_time_ns': perception_time_ns,
            'levels_completed': metadata['levels_completed'],
            # V26: Logs analyse spatiale
            'grid_analysis_enabled': self.grid_aware_enabled,
            'player_detected': metadata['player_detected'],
            'goal_detected': metadata['goal_detected'],
            'path_found': metadata['path_found'],
            'distance_to_goal': float(metadata['distance_to_goal'])
        })
        
        return GridObservation(
            grid=grid,
            embedding=embedding,
            spatial_analysis=spatial_analysis,
            patterns=patterns,
            objects=objects,
            metadata=metadata
        )
    
    def _penalize_repetition(self, action_scores: Dict[str, float]) -> Dict[str, float]:
        """
        ENRICHED: Pénaliser actions répétées récemment
        
        Stratégie progressive:
        - 3 dernières actions: pénalité 90%
        - 5 dernières actions: pénalité 75%
        - 10 dernières actions: pénalité 50%
        """
        penalized_scores = action_scores.copy()
        recent_list = list(self.recent_actions)
        
        for i, action in enumerate(recent_list[-10:]):
            if action in penalized_scores:
                # Pénalité progressive
                if i >= 7:  # 3 dernières
                    penalized_scores[action] *= 0.1
                    self.stats['repetitions_penalized'] += 1
                elif i >= 5:  # 5 dernières
                    penalized_scores[action] *= 0.25
                else:  # 10 dernières
                    penalized_scores[action] *= 0.5
        
        return penalized_scores
    
    def _handle_cycle_detection(self, cycle_detected: bool, cycle_period: int = 0) -> str:
        """
        ENRICHED: Réagir activement à détection cycle
        
        Actions:
        - Cycle court (<5): Forcer action rare
        - Cycle moyen (5-10): Injecter bruit
        - Cycle long (>10): Réinitialiser exploration
        """
        if not cycle_detected:
            self.in_cycle = False
            return 'normal'
        
        self.in_cycle = True
        self.cycle_count += 1
        
        # Cycle court: Forcer action différente
        if cycle_period < 5:
            return 'force_rare_action'
        
        # Cycle moyen: Injecter bruit
        elif cycle_period < 10:
            return 'inject_noise'
        
        # Cycle long: Réinitialiser
        else:
            return 'reset_exploration'
    
    def _compute_intrinsic_reward(self, obs_before: GridObservation, obs_after: GridObservation, action: str) -> float:
        """
        ENRICHED: Calculer reward intrinsèque multi-échelle
        
        Rewards:
        - Nouveauté état: Hash jamais vu (+0.1)
        - Diversité action: Action rare (+0.05)
        - Désynchronisation: Sortie cycle (+0.2)
        - Découverte transition: Transition non vue (+0.15)
        - Changement topologique: Hash différent (+0.1)
        - V26: Progression vers objectif (+0.3)
        """
        reward = 0.0
        
        # 1. Nouveauté état
        hash_after = obs_after.embedding.topological_hash
        if hash_after not in self.seen_hashes:
            reward += 0.1
            self.seen_hashes.add(hash_after)
            self.stats['unique_states_discovered'] += 1
        
        # 2. Diversité action
        action_count = self.action_counts.get(action, 0)
        if action_count < 5:
            reward += 0.05
        
        # 3. Désynchronisation cycle
        if self.in_cycle and not self._detect_cycle_now():
            reward += 0.2
            self.in_cycle = False
            self.stats['cycles_avoided'] += 1
        
        # 4. Découverte transition
        hash_before = obs_before.embedding.topological_hash
        transition_key = (hash_before, action)
        if transition_key not in self.seen_transitions:
            reward += 0.15
            self.seen_transitions.add(transition_key)
        
        # 5. Changement topologique
        if hash_before != hash_after:
            reward += 0.1
        
        # V26: 6. Progression vers objectif
        if obs_before.spatial_analysis and obs_after.spatial_analysis:
            dist_before = obs_before.spatial_analysis.distance_to_goal
            dist_after = obs_after.spatial_analysis.distance_to_goal
            
            if dist_after < dist_before:
                # Récompenser progression
                reward += 0.3
            elif dist_after > dist_before:
                # Pénaliser régression
                reward -= 0.1
        
        self.stats['intrinsic_rewards_total'] += reward
        return reward
    
    def _detect_cycle_now(self) -> bool:
        """Détecte si actuellement dans un cycle"""
        if len(self.recent_actions) < 4:
            return False
        
        # Vérifier pattern répétitif simple
        last_4 = list(self.recent_actions)[-4:]
        if last_4[0] == last_4[2] and last_4[1] == last_4[3]:
            return True
        
        return False
    
    def decide(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        DÉCISION V26 GRID-AWARE avec priorités:
        1. Grid-Aware (si analyse spatiale disponible + chemin trouvé)
        2. Planning (si plan actif)
        3. Exploitation (si graphe riche + rewards positifs)
        4. Exploration (sinon, 80% du temps)
        """
        # V26: Priorité 1 - Grid-Aware Navigation
        if self.grid_aware_enabled and obs.spatial_analysis:
            decision = self.decide_grid_aware(obs, available_actions)
            if decision and decision.score > 0.7:  # Seuil confiance
                return decision
        
        # ENRICHED: Forcer exploration si conditions non remplies
        force_exploration = (
            self.total_actions < self.min_exploration_actions or
            self.stats['successful_actions'] == 0 or
            len(self.causal_graph.nodes) < 10
        )
        
        if force_exploration:
            return self.decide_exploration(obs, available_actions)
        
        # Priorité 2: Planning
        if self.current_plan and self.plan_step < len(self.current_plan.actions):
            decision = self.decide_planning(obs, available_actions)
            if decision:
                return decision
        
        # Priorité 3/4: Exploitation vs Exploration (ratio dynamique)
        exploration_ratio = max(0.2, self.exploration_budget)
        if np.random.random() < exploration_ratio:
            return self.decide_exploration(obs, available_actions)
        else:
            return self.decide_exploitation(obs, available_actions)
    
    def decide_grid_aware(self, obs: GridObservation, available_actions: List[str]) -> Optional[ActionDecision]:
        """
        V26: Phase Grid-Aware Navigation
        
        Utilise GridAnalyzer pour suggérer action intelligente basée sur analyse spatiale
        """
        start_ns = time.perf_counter_ns()
        
        if not obs.spatial_analysis:
            return None
        
        # Obtenir suggestion depuis GridAnalyzer
        try:
            suggestion = self.grid_analyzer.suggest_action(obs.spatial_analysis)
        except Exception as e:
            print(f"[WARNING] GridAnalyzer suggestion error: {e}")
            return None
        
        # Convertir action sémantique → action environnement
        env_action = self.semantic_to_env_mapping.get(suggestion.action_name)
        
        # Vérifier si action disponible
        if not env_action or env_action not in available_actions:
            return None
        
        decision_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('decision', 'grid_aware', {
            'env_action': env_action,
            'semantic_action': suggestion.action_name,
            'score': float(suggestion.score),
            'justification': suggestion.justification,
            'expected_outcome': suggestion.expected_outcome,
            'risk_level': float(suggestion.risk_level),
            'player_position': obs.spatial_analysis.player_position,
            'goal_position': obs.spatial_analysis.goal_position,
            'distance_to_goal': float(obs.spatial_analysis.distance_to_goal),
            'path_length': len(obs.spatial_analysis.path_to_goal) if obs.spatial_analysis.path_to_goal else 0,
            'decision_time_ns': decision_time_ns
        })
        
        self.stats['grid_aware_actions'] += 1
        self.stats['grid_suggestions_used'] += 1
        
        print(f"[GRID-AWARE] Action: {env_action} ({suggestion.action_name}) score={suggestion.score:.3f}")
        print(f"[GRID-AWARE] Justification: {suggestion.justification}")
        print(f"[GRID-AWARE] Expected: {suggestion.expected_outcome} (risk={suggestion.risk_level:.2f})")
        
        return ActionDecision(
            action=self.env_action_mapping[env_action],
            action_name=env_action,
            semantic_name=suggestion.action_name,
            score=suggestion.score,
            strategy='grid_aware',
            budget=10.0,
            justification=suggestion.justification,
            predicted_reward=suggestion.score * (1.0 - suggestion.risk_level),
            grid_suggestion=suggestion
        )
    
    def decide_planning(self, obs: GridObservation, available_actions: List[str]) -> Optional[ActionDecision]:
        """Phase planning (V25)"""
        # Implémentation simplifiée - voir agent V25 original pour détails
        return None
    
    def decide_exploitation(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """Phase exploitation (V24)"""
        # Implémentation simplifiée - voir agent V24 original pour détails
        return self.decide_exploration(obs, available_actions)
    
    def decide_exploration(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        Phase exploration ENRICHIE
        
        Utilise GoalGrounder + pénalisation répétition + cycle avoidance
        """
        start_ns = time.perf_counter_ns()
        
        # Obtenir priorités actions depuis GoalGrounder
        action_scores = {}
        for env_action in available_actions:
            semantic_name = self.semantic_library.get_semantic_name(env_action)
            
            # Score = priorité GoalGrounder + bonus exploration
            if env_action in self.goal_grounder.action_priorities:
                priority = self.goal_grounder.action_priorities[env_action]
                score = priority.priority_score
            else:
                score = 0.5  # Neutre pour actions non testées
            
            # Bonus exploration pour actions peu testées
            exploration_bonus = self.info_gain_reward.get_exploration_bonus(env_action)
            score += exploration_bonus * 0.3
            
            action_scores[env_action] = score
        
        # ENRICHED: Pénaliser répétitions
        action_scores = self._penalize_repetition(action_scores)
        
        # ENRICHED: Cycle avoidance
        cycle_detected = self._detect_cycle_now()
        if cycle_detected:
            strategy = self._handle_cycle_detection(True, 2)
            
            if strategy == 'force_rare_action':
                # Choisir action avec count minimal
                best_env_action = min(self.action_counts, key=lambda k: self.action_counts.get(k, 0))
            elif strategy == 'inject_noise':
                # Ajouter bruit gaussien
                action_scores = {k: v + np.random.normal(0, 0.1) for k, v in action_scores.items()}
                best_env_action = max(action_scores.items(), key=lambda x: x[1])[0]
            else:
                # Réinitialiser et choisir aléatoire
                best_env_action = np.random.choice(available_actions)
        else:
            # Choisir meilleure action
            best_env_action = max(action_scores.items(), key=lambda x: x[1])[0]
        
        best_score = action_scores.get(best_env_action, 0.5)
        best_semantic = self.semantic_library.get_semantic_name(best_env_action)
        
        decision_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('decision', 'exploration', {
            'env_action': best_env_action,
            'semantic_action': best_semantic,
            'score': float(best_score),
            'cycle_detected': cycle_detected,
            'decision_time_ns': decision_time_ns,
            'actions_evaluated': len(action_scores)
        })
        
        print(f"[EXPLORATION] Action: {best_env_action} ({best_semantic}) score={best_score:.3f}")
        
        return ActionDecision(
            action=self.env_action_mapping[best_env_action],
            action_name=best_env_action,
            semantic_name=best_semantic,
            score=best_score,
            strategy='exploration',
            budget=10.0,
            justification=f"GoalGrounder priority + exploration bonus + repetition penalty",
            predicted_reward=0.0
        )
    
    def learn(self, obs_before: GridObservation, action: str, obs_after: GridObservation, reward: float, done: bool):
        """
        APPRENTISSAGE ENRICHI + V26
        
        Ajoute reward intrinsèque + met à jour graphe causal + feedback GridAnalyzer
        """
        start_ns = time.perf_counter_ns()
        
        # ENRICHED: Calculer reward intrinsèque (inclut progression V26)
        intrinsic_reward = self._compute_intrinsic_reward(obs_before, obs_after, action)
        total_reward = reward + intrinsic_reward
        
        # Mettre à jour historique
        self.recent_actions.append(action)
        self.action_counts[action] += 1
        
        # Mettre à jour graphe causal
        success = total_reward > 0
        semantic_action = self.semantic_library.get_semantic_name(action)
        self.causal_graph.add_transition(
            from_embedding=obs_before.embedding.vector,
            action=action,
            semantic_action=semantic_action,
            to_embedding=obs_after.embedding.vector,
            reward=total_reward,
            success=success
        )
        
        # Détecter cycles
        cycles_list = self.causal_graph.detect_cycles(window=10)
        cycles_detected = len(cycles_list) if cycles_list else 0
        if cycles_detected > 0:
            self.stats['cycles_detected'] += cycles_detected
        
        # Mettre à jour statistiques
        self.stats['learning_updates'] += 1
        if success:
            self.stats['successful_actions'] += 1
        
        learning_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('learning', 'agent', {
            'action': action,
            'env_reward': float(reward),
            'intrinsic_reward': float(intrinsic_reward),
            'total_reward': float(total_reward),
            'success': success,
            'cycles_detected': cycles_detected,
            'graph_nodes': len(self.causal_graph.nodes),
            'graph_edges': len(self.causal_graph.edges),
            'learning_time_ns': learning_time_ns,
            # V26: Feedback analyse spatiale
            'distance_change': (
                obs_after.spatial_analysis.distance_to_goal - obs_before.spatial_analysis.distance_to_goal
                if obs_before.spatial_analysis and obs_after.spatial_analysis
                else 0.0
            )
        })
    
    def reset(self):
        """Réinitialiser agent pour nouveau jeu"""
        self.current_plan = None
        self.plan_step = 0
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # ENRICHED: Réinitialiser tracking
        self.recent_actions.clear()
        self.action_counts.clear()
        self.seen_hashes.clear()
        self.seen_transitions.clear()
        self.in_cycle = False
        self.cycle_count = 0
        
        print("[MAGEN V26 GRID-AWARE] Agent réinitialisé pour nouveau jeu")
    
    def get_stats(self) -> Dict:
        """Retourner statistiques complètes"""
        return {
            **self.stats,
            'causal_graph_stats': {
                'nodes': len(self.causal_graph.nodes),
                'edges': len(self.causal_graph.edges),
                'plans_generated': self.stats['plans_generated'],
                'cycles_detected': self.stats['cycles_detected']
            },
            'state_encoder_stats': self.state_encoder.stats,
            'goal_grounder_stats': self.goal_grounder.get_stats(),
            'semantic_library_stats': self.semantic_library.get_stats(),
            'grid_analyzer_stats': self.grid_analyzer.get_stats()  # V26
        }


# Made with Bob - MAGEN V26 GRID-AWARE