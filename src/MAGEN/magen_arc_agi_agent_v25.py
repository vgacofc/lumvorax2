"""
MAGEN ARC-AGI Agent V25 - Causal Transition Graph + Planning
Architecture complète: V24 + CausalTransitionGraph + BFS Planning

PROBLÈME V24: Score 0/3 - Grounding sémantique SANS compréhension objectif
SOLUTION V25: Graphe transitions causales + planification dirigée objectif

Insights Session 69:
- V24 a primitives interaction MAIS pas modèle mental objectifs
- Agent explore aveuglément sans mémoire trajectoire
- Besoin planification multi-step vers objectifs

Architecture V25:
1. V24: StateEncoder + GoalGrounder + SemanticActionLibrary
2. V25: CausalTransitionGraph + BFS Planning + Cycle Detection
3. Forensic: Logs bit-level + nanoseconde

Forensic: Protocole CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN v2.0
"""

import numpy as np
import time
import json
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass, asdict
from arcengine import GameAction, GameState  # type: ignore
from collections import defaultdict, deque

# Modules V22 (hérités)
from core.transformation_learning_engine import TransformationLearningEngine
from core.pattern_matcher import PatternMatcher
from core.object_transformation_engine import ObjectExtractor
from core.meta_arbiter import MetaArbiter
from core.action_reputation_system import ActionReputationSystem
from core.exploration_budget_manager import ExplorationBudgetManager

# Modules V23 (hérités)
from core.action_discovery_engine import ActionDiscoveryEngine, ActionModel, StateTransition as ADStateTransition
from core.world_model import WorldModel, StateTransition as WMStateTransition
from core.information_gain_reward import InformationGainReward, StateTransition as IGStateTransition

# Modules V24 (hérités)
from core.state_encoder import StateEncoder, StateEmbedding
from core.goal_grounder import GoalGrounder, GoalProgress, ActionPriority
from core.semantic_action_library import SemanticActionLibrary, ActionSemantics

# Modules V25 (NOUVEAUX)
from core.causal_transition_graph import CausalTransitionGraph, Path, NodeType


@dataclass
class GridObservation:
    """Observation enrichie avec embeddings causaux"""
    grid: np.ndarray
    embedding: StateEmbedding  # V24
    patterns: List[str]
    objects: List[Dict]
    metadata: Dict


@dataclass
class ActionDecision:
    """Décision d'action avec planification"""
    action: GameAction
    action_name: str
    semantic_name: str
    score: float
    strategy: str  # 'exploration', 'exploitation', 'planning'
    budget: float
    justification: str
    predicted_outcome: Optional[np.ndarray] = None
    predicted_reward: float = 0.0
    goal_progress_expected: float = 0.0
    from_plan: bool = False  # NOUVEAU V25: Si action vient d'un plan


@dataclass
class ForensicLog:
    """Log forensique bit-level avec timestamps nanoseconde"""
    timestamp_ns: int
    event_type: str  # perception, decision, action, learning, goal_progress, planning
    component: str
    data: Dict
    
    def to_json(self) -> str:
        """Sérialisation JSON avec conversion types numpy"""
        def convert_numpy(obj):
            if isinstance(obj, (np.integer, np.floating)):
                return obj.item()
            elif isinstance(obj, np.ndarray):
                return obj.tolist()
            elif isinstance(obj, dict):
                return {k: convert_numpy(v) for k, v in obj.items()}
            elif isinstance(obj, (list, tuple)):
                return [convert_numpy(item) for item in obj]
            return obj
        
        return json.dumps({
            'timestamp_ns': int(self.timestamp_ns),
            'event_type': self.event_type,
            'component': self.component,
            'data': convert_numpy(self.data)
        })


class MAGENArcAgiAgentV25:
    """
    Agent MAGEN V25 avec Causal Transition Graph + Planning
    
    Évolution:
    V22: TLE + PatternMatcher + ObjectExtractor + MetaArbiter
    V23: ActionDiscovery + WorldModel + InformationGainReward
    V24: StateEncoder + GoalGrounder + SemanticActionLibrary
    V25: CausalTransitionGraph + BFS Planning + Cycle Detection
    """
    
    def __init__(self, forensic_log_path: Optional[str] = None):
        # V22: Modules base
        self.tle = TransformationLearningEngine()
        self.pattern_matcher = PatternMatcher()
        self.object_extractor = ObjectExtractor()
        self.meta_arbiter = MetaArbiter()
        self.reputation_system = ActionReputationSystem()
        self.budget_manager = ExplorationBudgetManager()
        
        # V23: Modules découverte
        self.action_discovery = ActionDiscoveryEngine()
        self.world_model = WorldModel()
        self.info_gain_reward = InformationGainReward()
        
        # V24: Modules grounding sémantique
        self.state_encoder = StateEncoder(embedding_dim=128)
        self.goal_grounder = GoalGrounder()
        self.semantic_library = SemanticActionLibrary()
        
        # V25: Modules planification (NOUVEAUX)
        self.causal_graph = CausalTransitionGraph(
            similarity_threshold=0.85,
            max_nodes=10000,
            max_edges_per_node=50
        )
        self.current_plan: Optional[Path] = None
        self.plan_step: int = 0
        
        # Mapping actions environnement → GameAction
        self.env_action_mapping = {
            'ACTION1': GameAction.ACTION1,
            'ACTION2': GameAction.ACTION2,
            'ACTION3': GameAction.ACTION3,
            'ACTION4': GameAction.ACTION4,
            'ACTION5': GameAction.ACTION5,
            'ACTION6': GameAction.ACTION6,
            'ACTION7': GameAction.ACTION7,
            'RESET': GameAction.RESET
        }
        
        # Initialiser mapping sémantique (découvert dynamiquement)
        self._initialize_semantic_mapping()
        
        # Phases exploration/exploitation
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # Logs forensiques
        self.forensic_logs: List[ForensicLog] = []
        self.forensic_log_path = forensic_log_path
        
        # Statistiques
        self.stats = {
            'total_actions': 0,
            'exploration_actions': 0,
            'exploitation_actions': 0,
            'planning_actions': 0,  # NOUVEAU V25
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
            'avg_plan_length': 0.0
        }
        
        print("[MAGEN V25] Agent initialisé avec architecture complète")
        print("[MAGEN V25] V22: TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18")
        print("[MAGEN V25] V23: ActionDiscovery+WorldModel+InformationGainReward")
        print("[MAGEN V25] V24: StateEncoder+GoalGrounder+SemanticActionLibrary")
        print("[MAGEN V25] V25: CausalTransitionGraph+BFSPlanning+CycleDetection")
        print("[MAGEN V25] Forensic logging: ENABLED")
        
        if forensic_log_path:
            print(f"[MAGEN V25] Forensic log path: {forensic_log_path}")
    
    def _initialize_semantic_mapping(self):
        """Initialise mapping actions environnement → sémantique (hypothèses initiales)"""
        # Hypothèses initiales (à affiner via apprentissage)
        self.semantic_library.map_env_action_to_semantic('ACTION1', 'move_up')
        self.semantic_library.map_env_action_to_semantic('ACTION2', 'move_down')
        self.semantic_library.map_env_action_to_semantic('ACTION3', 'move_left')
        self.semantic_library.map_env_action_to_semantic('ACTION4', 'move_right')
        self.semantic_library.map_env_action_to_semantic('ACTION5', 'interact')
        self.semantic_library.map_env_action_to_semantic('ACTION6', 'collect')
        self.semantic_library.map_env_action_to_semantic('ACTION7', 'undo')
        self.semantic_library.map_env_action_to_semantic('RESET', 'reset')
    
    def _log_forensic(self, event_type: str, component: str, data: Dict):
        """Enregistre événement forensique"""
        log = ForensicLog(
            timestamp_ns=time.perf_counter_ns(),
            event_type=event_type,
            component=component,
            data=data
        )
        self.forensic_logs.append(log)
        self.stats['forensic_logs_generated'] += 1
    
    def perceive(self, obs) -> GridObservation:
        """
        PHASE 1: PERCEPTION
        Encode observation environnement
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
        
        # V24: Encoder état en embedding causal
        embedding = self.state_encoder.encode(grid)
        
        # DÉSACTIVÉ pour jeux interactifs ARC-AGI-3
        # PatternMatcher et ObjectExtractor conçus pour puzzles ARC statiques
        patterns = []
        objects = []
        
        # Métadonnées
        metadata = {
            'grid_size': grid.shape,
            'color_count': len(np.unique(grid)),
            'levels_completed': obs.levels_completed if hasattr(obs, 'levels_completed') else 0,
            'embedding_objects': embedding.objects_detected,
            'embedding_motion': embedding.motion_magnitude,
            'embedding_regions': len(embedding.active_regions)
        }
        
        perception_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('perception', 'agent', {
            'grid_shape': list(grid.shape),
            'embedding_dim': embedding.vector.shape[0],
            'objects_detected': embedding.objects_detected,
            'motion_magnitude': float(embedding.motion_magnitude),
            'active_regions_count': len(embedding.active_regions),
            'perception_time_ns': perception_time_ns,
            'levels_completed': metadata['levels_completed']
        })
        
        return GridObservation(
            grid=grid,
            embedding=embedding,
            patterns=patterns,
            objects=objects,
            metadata=metadata
        )
    
    def decide_planning(self, obs: GridObservation, available_actions: List[str]) -> Optional[ActionDecision]:
        """
        NOUVEAU V25: Décision basée planification
        
        Utilise CausalTransitionGraph pour planifier vers objectif
        """
        start_ns = time.perf_counter_ns()
        
        # Vérifier si plan actif
        if self.current_plan and self.plan_step < len(self.current_plan.actions):
            # Continuer plan existant
            action_name = self.current_plan.actions[self.plan_step]
            semantic_name = self.semantic_library.get_semantic_name(action_name)
            
            self.plan_step += 1
            
            # Log forensique
            self._log_forensic('decision', 'planning', {
                'strategy': 'follow_plan',
                'action': action_name,
                'semantic_action': semantic_name,
                'plan_step': self.plan_step,
                'plan_length': len(self.current_plan.actions),
                'plan_progress': self.plan_step / len(self.current_plan.actions)
            })
            
            print(f"[PLANNING] Suivre plan: {action_name} ({semantic_name}) [{self.plan_step}/{len(self.current_plan.actions)}]")
            
            return ActionDecision(
                action=self.env_action_mapping[action_name],
                action_name=action_name,
                semantic_name=semantic_name,
                score=1.0,
                strategy='planning',
                budget=10.0,
                justification=f"Plan step {self.plan_step}/{len(self.current_plan.actions)}",
                from_plan=True
            )
        
        # Générer nouveau plan si nécessaire
        path = self.causal_graph.find_path_bfs(
            start_embedding=obs.embedding.vector,
            goal_type=NodeType.GOAL,
            max_depth=20
        )
        
        if path and len(path.actions) > 0:
            self.current_plan = path
            self.plan_step = 0
            self.stats['plans_generated'] += 1
            
            # Exécuter première action du plan
            action_name = path.actions[0]
            semantic_name = self.semantic_library.get_semantic_name(action_name)
            
            self.plan_step = 1
            
            planning_time_ns = time.perf_counter_ns() - start_ns
            
            # Log forensique
            self._log_forensic('planning', 'causal_graph', {
                'plan_found': True,
                'plan_length': len(path.actions),
                'plan_reward': path.total_reward,
                'plan_success_prob': path.success_probability,
                'planning_time_ns': planning_time_ns
            })
            
            print(f"[PLANNING] Nouveau plan: {len(path.actions)} actions, reward={path.total_reward:.2f}")
            
            return ActionDecision(
                action=self.env_action_mapping[action_name],
                action_name=action_name,
                semantic_name=semantic_name,
                score=path.success_probability,
                strategy='planning',
                budget=10.0,
                justification=f"New plan: {len(path.actions)} steps",
                predicted_reward=path.total_reward,
                from_plan=True
            )
        
        # Aucun plan trouvé
        return None
    
    def decide_exploration(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        PHASE EXPLORATION: Découvrir primitives interaction
        
        V24: Utilise GoalGrounder pour prioriser actions prometteuses
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
        
        # Choisir meilleure action
        best_env_action = max(action_scores.items(), key=lambda x: x[1])[0]
        best_score = action_scores[best_env_action]
        best_semantic = self.semantic_library.get_semantic_name(best_env_action)
        
        # Prédire outcome si world model disponible
        predicted_outcome = None
        predicted_reward = 0.0
        if len(self.world_model.transitions) > 10:
            predicted_outcome = self.world_model.predict_outcome(obs.embedding.vector, best_env_action)
            predicted_reward = self.world_model.predict_reward(obs.embedding.vector, best_env_action)
        
        decision_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('decision', 'agent', {
            'phase': 'exploration',
            'env_action': best_env_action,
            'semantic_action': best_semantic,
            'score': float(best_score),
            'predicted_reward': float(predicted_reward),
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
            justification=f"GoalGrounder priority + exploration bonus",
            predicted_reward=predicted_reward
        )
    
    def decide_exploitation(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """Phase exploitation (V24 hérité)"""
        # Utilise goal grounder + world model
        prioritized = self.goal_grounder.get_prioritized_actions(available_actions)
        
        if prioritized:
            best_env_action, priority_score = prioritized[0]
            semantic_name = self.semantic_library.get_semantic_name(best_env_action)
            
            return ActionDecision(
                action=self.env_action_mapping[best_env_action],
                action_name=best_env_action,
                semantic_name=semantic_name,
                score=priority_score,
                strategy='exploitation',
                budget=10.0,
                justification="Goal-oriented exploitation"
            )
        
        # Fallback
        env_action = np.random.choice(available_actions)
        semantic_name = self.semantic_library.get_semantic_name(env_action)
        
        return ActionDecision(
            action=self.env_action_mapping[env_action],
            action_name=env_action,
            semantic_name=semantic_name,
            score=0.0,
            strategy='random_fallback',
            budget=5.0,
            justification="No valid predictions"
        )
    
    def decide(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        PHASE 2: COGNITION
        Décide action (planning > exploitation > exploration)
        """
        # V25: Priorité 1 - Planning si graphe suffisant
        if self.causal_graph.stats['total_nodes'] > 10:
            decision = self.decide_planning(obs, available_actions)
            if decision:
                self.stats['planning_actions'] += 1
                self.total_actions += 1
                return decision
        
        # Priorité 2 - Exploration ou exploitation
        if self.exploration_phase_active and self.exploration_actions_taken < 50:
            decision = self.decide_exploration(obs, available_actions)
            self.exploration_actions_taken += 1
            self.stats['exploration_actions'] += 1
        else:
            if self.exploration_phase_active:
                print(f"\n[MAGEN V25] ===== FIN PHASE EXPLORATION =====")
                print(f"[MAGEN V25] Graphe: {self.causal_graph.stats['total_nodes']} nodes, {self.causal_graph.stats['total_edges']} edges")
                print(f"[MAGEN V25] ===== DÉBUT PHASE EXPLOITATION + PLANNING =====\n")
                self.exploration_phase_active = False
            
            decision = self.decide_exploitation(obs, available_actions)
            self.exploitation_actions_taken += 1
            self.stats['exploitation_actions'] += 1
        
        self.total_actions += 1
        self.stats['total_actions'] += 1
        
        return decision
    
    def learn(self, obs_before: GridObservation, decision: ActionDecision, obs_after: GridObservation):
        """
        PHASE 3: APPRENTISSAGE
        Met à jour tous modèles + graphe causal
        """
        start_ns = time.perf_counter_ns()
        
        # Calculer reward
        reward_before = float(obs_before.metadata['levels_completed'])
        reward_after = float(obs_after.metadata['levels_completed'])
        reward_delta = reward_after - reward_before
        
        # V25: Ajouter transition au graphe causal
        self.causal_graph.add_transition(
            from_embedding=obs_before.embedding.vector,
            action=decision.action_name,
            semantic_action=decision.semantic_name,
            to_embedding=obs_after.embedding.vector,
            reward=reward_delta,
            success=reward_delta > 0,
            metadata=obs_after.metadata
        )
        
        # V25: Détecter cycles
        cycles = self.causal_graph.detect_cycles(window=10)
        if cycles:
            self.stats['cycles_detected'] += len(cycles)
            print(f"[CAUSAL GRAPH] ⚠️ Cycle détecté: period={cycles[0]['period']}")
        
        # V24: Goal grounder
        progress = self.goal_grounder.record_progress(
            action=decision.action_name,
            reward_before=reward_before,
            reward_after=reward_after,
            levels_before=int(obs_before.metadata['levels_completed']),
            levels_after=int(obs_after.metadata['levels_completed'])
        )
        
        # V23: World model
        transition_wm = WMStateTransition(
            state_before=obs_before.embedding.vector,
            action=decision.action_name,
            state_after=obs_after.embedding.vector,
            reward=reward_delta,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        self.world_model.record_transition(transition_wm)
        
        # Mettre à jour stats
        if progress.success:
            self.stats['successful_actions'] += 1
        
        self.stats['learning_updates'] += 1
        self.stats['semantic_actions_used'] = sum(
            1 for a in self.semantic_library.action_library.values() if a.usage_count > 0
        )
        self.stats['semantic_mappings_learned'] = len(self.semantic_library.env_to_semantic)
        
        learning_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('learning', 'agent', {
            'action': decision.action_name,
            'reward_delta': float(reward_delta),
            'success': progress.success,
            'graph_nodes': self.causal_graph.stats['total_nodes'],
            'graph_edges': self.causal_graph.stats['total_edges'],
            'cycles_detected': len(cycles),
            'learning_time_ns': learning_time_ns
        })
        
        # Logging périodique
        if self.total_actions % 10 == 0:
            embeddings_count = self.state_encoder.stats.get('total_encodings', 0)
            semantic_used = sum(1 for a in self.semantic_library.action_library.values() if a.usage_count > 0)
            
            print(f"[MAGEN V25] Actions: {self.total_actions} | Phase: {'EXPLORATION' if self.exploration_phase_active else 'EXPLOITATION+PLANNING'}")
            print(f"  State Encoder: {embeddings_count} embeddings")
            print(f"  Causal Graph: {self.causal_graph.stats['total_nodes']} nodes, {self.causal_graph.stats['total_edges']} edges")
            print(f"  Plans: {self.stats['plans_generated']} generated, {self.stats['planning_actions']} actions from plans")
            print(f"  Semantic: {semantic_used}/{len(self.semantic_library.action_library)} actions used")
            print(f"  Forensic: {self.stats['forensic_logs_generated']} logs")
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques complètes"""
        return {
            **self.stats,
            'current_phase': 'EXPLORATION' if self.exploration_phase_active else 'EXPLOITATION+PLANNING',
            'embeddings_generated': self.state_encoder.stats.get('total_encodings', 0),
            'causal_graph_stats': self.causal_graph.get_stats(),
            'state_encoder_stats': self.state_encoder.stats,
            'goal_grounder_stats': self.goal_grounder.stats,
            'semantic_library_stats': self.semantic_library.stats
        }
    
    def reset_for_new_game(self):
        """Réinitialise agent pour nouveau jeu"""
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # Réinitialiser plan
        self.current_plan = None
        self.plan_step = 0
        
        # Réinitialiser graphe causal (garder connaissances)
        self.causal_graph.reset_trajectory()
        
        # Réinitialiser modules V24
        self.state_encoder.previous_state = None
        self.state_encoder.previous_embedding = None
        self.goal_grounder.progress_history.clear()
        self.goal_grounder.current_sequence.clear()
        
        # Réinitialiser info gain reward
        self.info_gain_reward.reset()
        
        # Log forensique
        self._log_forensic('reset', 'agent', {
            'reason': 'new_game',
            'graph_nodes_retained': self.causal_graph.stats['total_nodes'],
            'graph_edges_retained': self.causal_graph.stats['total_edges']
        })
        
        print("[MAGEN V25] Agent réinitialisé pour nouveau jeu")
    
    def save_forensic_logs(self, path: str):
        """Sauvegarde logs forensiques"""
        with open(path, 'w') as f:
            for log in self.forensic_logs:
                f.write(log.to_json() + '\n')
        
        print(f"[MAGEN V25] {len(self.forensic_logs)} logs forensiques sauvegardés: {path}")

# Made with Bob