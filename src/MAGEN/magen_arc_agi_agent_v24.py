"""
MAGEN ARC-AGI Agent V24 - Grounding Sémantique + Goal-Oriented Learning
Architecture complète: V23 + StateEncoder + GoalGrounder + SemanticActionLibrary

PROBLÈME V23: Score 0/3 (0%) - Actions découvertes (rotate, mirror) inadaptées jeux arcade
SOLUTION V24: Grounding sémantique actions → objectifs environnementaux

Insights Rapport Technique ARC-AGI-3 (Avril 2026):
- Métrique RHAE: (actions_humain / actions_AI)² par niveau
- 4 Piliers: Exploration, Modeling, Goal-Setting, Planning
- Turn-based: Priorité raisonnement offline vs réflexes temps réel
- Core Knowledge Priors Only: Objectness, géométrie, physique, agentness
- Humains 100% vs Frontier AI <1%

Architecture V24:
1. StateEncoder: Embeddings causaux (128D) vs pixels bruts
2. GoalGrounder: Goal Progress vs Information Gain
3. SemanticActionLibrary: Primitives interaction vs transformations géométriques
4. Logs forensiques profonds: Traçabilité bit-level + nanoseconde

Forensic: Protocole CLAUDE_PILOT + LUMVORAX activé
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

# Modules V24 (NOUVEAUX)
from core.state_encoder import StateEncoder, StateEmbedding
from core.goal_grounder import GoalGrounder, GoalProgress, ActionPriority
from core.semantic_action_library import SemanticActionLibrary, ActionSemantics


@dataclass
class GridObservation:
    """Observation enrichie avec embeddings causaux"""
    grid: np.ndarray
    embedding: StateEmbedding  # NOUVEAU V24
    patterns: List[str]
    objects: List[Dict]
    metadata: Dict


@dataclass
class ActionDecision:
    """Décision d'action avec grounding sémantique"""
    action: GameAction
    action_name: str
    semantic_name: str  # NOUVEAU V24: Nom sémantique (move_right, collect, etc.)
    score: float
    strategy: str
    budget: float
    justification: str
    predicted_outcome: Optional[np.ndarray] = None
    predicted_reward: float = 0.0
    goal_progress_expected: float = 0.0  # NOUVEAU V24


@dataclass
class ForensicLog:
    """Log forensique bit-level avec timestamps nanoseconde"""
    timestamp_ns: int
    event_type: str  # perception, decision, action, learning, goal_progress
    component: str  # state_encoder, goal_grounder, semantic_library, agent
    data: Dict
    
    def to_json(self) -> str:
        """Sérialisation JSON avec conversion types numpy"""
        def convert_numpy(obj):
            """Convertit récursivement types numpy en types Python"""
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


class MAGENArcAgiAgentV24:
    """
    Agent MAGEN V24 pour ARC-AGI-3
    
    Architecture:
    - V22: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17 + C18
    - V23: ActionDiscovery + WorldModel + InformationGainReward
    - V24: StateEncoder + GoalGrounder + SemanticActionLibrary + Forensic Profond
    
    Workflow V24:
    1. Phase Exploration (50 actions): Découvrir primitives interaction + construire world model
    2. Phase Exploitation (150 actions): Goal-oriented planning avec prédictions
    
    Différences clés V23→V24:
    - Comparaison états: Pixels bruts → Embeddings causaux (128D)
    - Reward: Information Gain → Goal Progress
    - Actions: Transformations géométriques → Primitives interaction
    - Logs: Standard → Forensic profond (bit-level + nanoseconde)
    """
    
    def __init__(self, forensic_log_path: Optional[str] = None):
        # ===== MODULES V22 (hérités) =====
        self.tle = TransformationLearningEngine()
        self.pattern_matcher = PatternMatcher()
        self.object_extractor = ObjectExtractor()
        self.meta_arbiter = MetaArbiter()
        self.reputation_system = ActionReputationSystem()
        self.budget_manager = ExplorationBudgetManager()
        
        # ===== MODULES V23 (hérités) =====
        self.action_discovery = ActionDiscoveryEngine(exploration_budget=50)
        self.world_model = WorldModel(max_transitions=1000)
        self.info_gain_reward = InformationGainReward(
            knowledge_weight=0.1,
            causal_weight=0.05,
            novelty_weight=0.1
        )
        
        # ===== MODULES V24 (NOUVEAUX) =====
        self.state_encoder = StateEncoder(embedding_dim=128, motion_threshold=0.1)
        self.goal_grounder = GoalGrounder(
            abandonment_threshold=20,
            promising_threshold=0.1,
            history_size=100
        )
        self.semantic_library = SemanticActionLibrary()
        
        # ===== FORENSIC LOGGING =====
        self.forensic_log_path = forensic_log_path
        self.forensic_logs: List[ForensicLog] = []
        self.forensic_enabled = True
        
        # ===== ÉTAT AGENT =====
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
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
        
        # Statistiques
        self.stats = {
            'total_actions': 0,
            'exploration_actions': 0,
            'exploitation_actions': 0,
            'successful_actions': 0,
            'patterns_detected': 0,
            'objects_extracted': 0,
            'strategy_switches': 0,
            'learning_updates': 0,
            'actions_discovered': 0,
            'world_model_accuracy': 0.0,
            'avg_info_gain_reward': 0.0,
            'avg_goal_progress': 0.0,  # NOUVEAU V24
            'actions_abandoned': 0,  # NOUVEAU V24
            'actions_promising': 0,  # NOUVEAU V24
            'semantic_mappings_learned': 0,  # NOUVEAU V24
            'forensic_logs_generated': 0  # NOUVEAU V24
        }
        
        # Log forensique initialisation
        self._log_forensic('initialization', 'agent', {
            'version': 'V24',
            'modules_v22': ['TLE', 'PatternMatcher', 'ObjectExtractor', 'MetaArbiter', 'C17', 'C18'],
            'modules_v23': ['ActionDiscovery', 'WorldModel', 'InformationGainReward'],
            'modules_v24': ['StateEncoder', 'GoalGrounder', 'SemanticActionLibrary'],
            'forensic_enabled': self.forensic_enabled,
            'forensic_log_path': self.forensic_log_path
        })
        
        print("[MAGEN V24] Agent initialisé avec architecture complète")
        print("[MAGEN V24] V22: TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18")
        print("[MAGEN V24] V23: ActionDiscovery+WorldModel+InformationGainReward")
        print("[MAGEN V24] V24: StateEncoder+GoalGrounder+SemanticActionLibrary+ForensicProfond")
        print(f"[MAGEN V24] Forensic logging: {'ENABLED' if self.forensic_enabled else 'DISABLED'}")
    
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
        PHASE 1: PERCEPTION
        Analyse observation et encode en embedding causal
        
        V24: Utilise StateEncoder pour embeddings vs pixels bruts V23
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
        
        # NOUVEAU V24: Encoder état en embedding causal
        embedding = self.state_encoder.encode(grid)
        
        # DÉSACTIVÉ pour jeux interactifs ARC-AGI-3
        # PatternMatcher et ObjectExtractor conçus pour puzzles ARC statiques
        patterns = []
        objects = []
        
        self.stats['patterns_detected'] += len(patterns)
        self.stats['objects_extracted'] += len(objects)
        
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
            justification=f"Exploration {best_semantic}",
            predicted_outcome=predicted_outcome,
            predicted_reward=predicted_reward,
            goal_progress_expected=0.0
        )
    
    def decide_exploitation(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        PHASE EXPLOITATION: Goal-oriented planning
        
        V24: Utilise GoalGrounder pour sélectionner actions maximisant progression
        """
        start_ns = time.perf_counter_ns()
        
        action_scores = {}
        action_predictions = {}
        
        for env_action in available_actions:
            semantic_name = self.semantic_library.get_semantic_name(env_action)
            
            # Vérifier si action abandonnée
            if env_action in self.goal_grounder.action_priorities:
                priority = self.goal_grounder.action_priorities[env_action]
                if priority.status == "ABANDONED":
                    continue  # Skip actions abandonnées
            
            # Prédire outcome
            predicted_state = self.world_model.predict_outcome(obs.embedding.vector, env_action)
            predicted_reward = self.world_model.predict_reward(obs.embedding.vector, env_action)
            
            # Score = predicted_reward × priority × reputation
            if env_action in self.goal_grounder.action_priorities:
                priority_score = self.goal_grounder.action_priorities[env_action].priority_score
            else:
                priority_score = 0.5
            
            reputation = self.reputation_system.get_action_priority(
                action_name=env_action,
                context={'grid_size': obs.grid.shape, 'color_count': len(np.unique(obs.grid))}
            )
            
            score = predicted_reward * priority_score * reputation
            
            action_scores[env_action] = score
            action_predictions[env_action] = (predicted_state, predicted_reward, priority_score)
        
        # Choisir meilleure action
        if action_scores:
            best_env_action = max(action_scores.items(), key=lambda x: x[1])[0]
            best_score = action_scores[best_env_action]
            best_semantic = self.semantic_library.get_semantic_name(best_env_action)
            predicted_state, predicted_reward, priority_score = action_predictions[best_env_action]
            
            decision_time_ns = time.perf_counter_ns() - start_ns
            
            # Log forensique
            self._log_forensic('decision', 'agent', {
                'phase': 'exploitation',
                'env_action': best_env_action,
                'semantic_action': best_semantic,
                'score': float(best_score),
                'predicted_reward': float(predicted_reward),
                'priority_score': float(priority_score),
                'decision_time_ns': decision_time_ns,
                'actions_evaluated': len(action_scores)
            })
            
            print(f"[EXPLOITATION] Action: {best_env_action} ({best_semantic}) score={best_score:.3f}")
            
            return ActionDecision(
                action=self.env_action_mapping[best_env_action],
                action_name=best_env_action,
                semantic_name=best_semantic,
                score=best_score,
                strategy='exploitation',
                budget=10.0,
                justification=f"Goal-oriented: {best_semantic}",
                predicted_outcome=predicted_state,
                predicted_reward=predicted_reward,
                goal_progress_expected=predicted_reward
            )
        else:
            # Fallback: action aléatoire
            env_action = np.random.choice(available_actions)
            semantic_name = self.semantic_library.get_semantic_name(env_action)
            
            return ActionDecision(
                action=self.env_action_mapping[env_action],
                action_name=env_action,
                semantic_name=semantic_name,
                score=0.0,
                strategy='random_fallback',
                budget=5.0,
                justification="No valid predictions",
                predicted_outcome=None,
                predicted_reward=0.0,
                goal_progress_expected=0.0
            )
    
    def decide(self, obs: GridObservation, available_actions: List[str]) -> ActionDecision:
        """
        PHASE 2: COGNITION
        Décide action (exploration ou exploitation)
        """
        # Choisir phase
        if self.exploration_phase_active and self.exploration_actions_taken < 50:
            decision = self.decide_exploration(obs, available_actions)
            self.exploration_actions_taken += 1
        else:
            if self.exploration_phase_active:
                print(f"\n[MAGEN V24] ===== FIN PHASE EXPLORATION =====")
                print(f"[MAGEN V24] Actions découvertes: {len(self.action_discovery.action_models)}")
                print(f"[MAGEN V24] World model: {len(self.world_model.transitions)} transitions")
                print(f"[MAGEN V24] Semantic mappings: {len(self.semantic_library.env_to_semantic)}")
                print(f"[MAGEN V24] ===== DÉBUT PHASE EXPLOITATION =====\n")
                self.exploration_phase_active = False
                
                # Log forensique transition phase
                self._log_forensic('phase_transition', 'agent', {
                    'from_phase': 'exploration',
                    'to_phase': 'exploitation',
                    'exploration_actions': self.exploration_actions_taken,
                    'actions_discovered': len(self.action_discovery.action_models),
                    'world_model_transitions': len(self.world_model.transitions)
                })
            
            decision = self.decide_exploitation(obs, available_actions)
            self.exploitation_actions_taken += 1
        
        self.total_actions += 1
        self.stats['total_actions'] += 1
        
        if self.exploration_phase_active:
            self.stats['exploration_actions'] += 1
        else:
            self.stats['exploitation_actions'] += 1
        
        return decision
    
    def learn(self, obs_before: GridObservation, decision: ActionDecision, obs_after: GridObservation):
        """
        PHASE 3: APPRENTISSAGE
        Met à jour tous modèles après observation résultat
        
        V24: Utilise GoalGrounder pour tracking progression objectif
        """
        start_ns = time.perf_counter_ns()
        
        # Calculer reward
        reward_before = float(obs_before.metadata['levels_completed'])
        reward_after = float(obs_after.metadata['levels_completed'])
        reward_delta = reward_after - reward_before
        
        # NOUVEAU V24: Enregistrer progression objectif
        progress = self.goal_grounder.record_progress(
            action=decision.action_name,
            reward_before=reward_before,
            reward_after=reward_after,
            levels_before=int(obs_before.metadata['levels_completed']),
            levels_after=int(obs_after.metadata['levels_completed'])
        )
        
        # Créer transitions
        transition_wm = WMStateTransition(
            state_before=obs_before.embedding.vector,  # V24: Utilise embeddings
            action=decision.action_name,
            state_after=obs_after.embedding.vector,
            reward=reward_delta,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        
        transition_ig = IGStateTransition(
            state_before=obs_before.grid,
            action=decision.action_name,
            state_after=obs_after.grid,
            reward=reward_delta,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        
        # 1. Enregistrer dans world model
        self.world_model.record_transition(transition_wm)
        
        # 2. Calculer reward total (avec information gain)
        total_reward = self.info_gain_reward.compute_reward(transition_ig)
        
        # 3. Mettre à jour semantic library
        success = progress.success
        semantic_action = self.semantic_library.action_library.get(decision.semantic_name)
        if semantic_action:
            semantic_action.update_from_feedback(success)
        
        # Mise à jour stats semantic library (compte TOUTES actions utilisées, pas seulement learned_from_feedback)
        self.stats['semantic_actions_used'] = sum(
            1 for a in self.semantic_library.action_library.values() if a.usage_count > 0
        )
        self.stats['semantic_mappings_learned'] = len(self.semantic_library.env_to_semantic)
        
        # 4. Mettre à jour C17 (reputation)
        self.reputation_system.update(
            action_name=decision.action_name,
            success=success,
            error=1.0 - total_reward if total_reward >= 0 else 1.0,
            context={'grid_size': obs_after.grid.shape, 'color_count': len(np.unique(obs_after.grid))}
        )
        
        # 5. Statistiques
        if success:
            self.stats['successful_actions'] += 1
        self.stats['learning_updates'] += 1
        self.stats['world_model_accuracy'] = self.world_model.stats.get('prediction_accuracy', 0.0)
        self.stats['avg_info_gain_reward'] = self.info_gain_reward.stats.get('avg_total_reward', 0.0)
        self.stats['avg_goal_progress'] = self.goal_grounder.stats.get('total_reward_gained', 0.0) / max(1, self.goal_grounder.stats.get('total_progressions', 1))
        self.stats['actions_abandoned'] = self.goal_grounder.stats.get('actions_abandoned', 0)
        self.stats['actions_promising'] = self.goal_grounder.stats.get('actions_promising', 0)
        
        learning_time_ns = time.perf_counter_ns() - start_ns
        
        # Log forensique
        self._log_forensic('learning', 'agent', {
            'action': decision.action_name,
            'semantic_action': decision.semantic_name,
            'reward_delta': float(reward_delta),
            'total_reward': float(total_reward),
            'success': success,
            'goal_progress': float(progress.reward_delta),
            'levels_delta': progress.levels_delta,
            'learning_time_ns': learning_time_ns,
            'world_model_transitions': len(self.world_model.transitions),
            'world_model_accuracy': self.stats['world_model_accuracy']
        })
        
        # Logging périodique avec compteurs V24 exposés
        if self.total_actions % 10 == 0:
            embeddings_count = self.state_encoder.stats.get('total_encodings', 0)
            semantic_used = sum(1 for a in self.semantic_library.action_library.values() if a.usage_count > 0)
            
            print(f"[MAGEN V24] Actions: {self.total_actions} | Phase: {'EXPLORATION' if self.exploration_phase_active else 'EXPLOITATION'}")
            print(f"  State Encoder: {embeddings_count} embeddings generated")
            print(f"  World Model: {len(self.world_model.transitions)} transitions, accuracy={self.stats['world_model_accuracy']:.2f}")
            print(f"  Goal Progress: avg={self.stats['avg_goal_progress']:.3f}, abandoned={self.stats['actions_abandoned']}, promising={self.stats['actions_promising']}")
            print(f"  Semantic: {semantic_used}/{len(self.semantic_library.action_library)} actions used, {self.stats['semantic_mappings_learned']} env mappings")
            print(f"  Forensic: {self.stats['forensic_logs_generated']} logs generated")
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques complètes avec compteurs V24 exposés"""
        return {
            **self.stats,
            'current_phase': 'EXPLORATION' if self.exploration_phase_active else 'EXPLOITATION',
            # V24: Compteurs spécifiques exposés
            'embeddings_generated': self.state_encoder.stats.get('total_encodings', 0),
            'priority_actions_count': len([p for p in self.goal_grounder.action_priorities.values() if p.status == "PROMISING"]),
            'abandoned_actions_count': len([p for p in self.goal_grounder.action_priorities.values() if p.status == "ABANDONED"]),
            'semantic_actions_discovered': self.semantic_library.stats.get('primitives_discovered', 0),
            # Stats modules complets
            'state_encoder_stats': self.state_encoder.stats,
            'goal_grounder_stats': self.goal_grounder.stats,
            'semantic_library_stats': self.semantic_library.stats,
            'action_discovery_stats': self.action_discovery.get_statistics(),
            'world_model_stats': self.world_model.get_statistics(),
            'info_gain_stats': self.info_gain_reward.get_statistics()
        }
    
    def reset_for_new_game(self):
        """Réinitialise agent pour nouveau jeu"""
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # Réinitialiser modules V24
        self.state_encoder.previous_state = None
        self.state_encoder.previous_embedding = None
        self.goal_grounder.progress_history.clear()
        self.goal_grounder.current_sequence.clear()
        
        # Réinitialiser info gain reward (nouveau jeu = nouveaux états)
        self.info_gain_reward.reset()
        
        # Log forensique reset
        self._log_forensic('reset', 'agent', {
            'reason': 'new_game',
            'total_actions_previous_game': self.stats['total_actions'],
            'forensic_logs_previous_game': self.stats['forensic_logs_generated']
        })
        
        print("[MAGEN V24] Agent réinitialisé pour nouveau jeu")
    
    def save_forensic_logs(self, path: str):
        """Sauvegarde logs forensiques dans fichier JSON"""
        with open(path, 'w') as f:
            for log in self.forensic_logs:
                f.write(log.to_json() + '\n')
        
        print(f"[MAGEN V24] {len(self.forensic_logs)} logs forensiques sauvegardés: {path}")

# Made with Bob
