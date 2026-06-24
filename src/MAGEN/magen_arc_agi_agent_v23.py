"""
MAGEN ARC-AGI Agent V23 - Action Discovery & World Model
Intégration complète: V22 (TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18) + V23 (ActionDiscovery+WorldModel+InfoGainReward)
"""

import numpy as np
import time
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass
from arcengine import GameAction, GameState

# Modules V22
from core.transformation_learning_engine import TransformationLearningEngine
from core.pattern_matcher import PatternMatcher
from core.object_transformation_engine import ObjectExtractor
from core.meta_arbiter import MetaArbiter
from core.action_reputation_system import ActionReputationSystem
from core.exploration_budget_manager import ExplorationBudgetManager

# Modules V23 (NOUVEAUX)
from core.action_discovery_engine import ActionDiscoveryEngine, ActionModel, StateTransition as ADStateTransition
from core.world_model import WorldModel, StateTransition as WMStateTransition
from core.information_gain_reward import InformationGainReward, StateTransition as IGStateTransition


@dataclass
class GridObservation:
    """Observation enrichie d'une grille"""
    grid: np.ndarray
    patterns: List[str]
    objects: List[Dict]
    metadata: Dict


@dataclass
class ActionDecision:
    """Décision d'action avec justification"""
    action: GameAction
    action_name: str
    score: float
    strategy: str
    budget: float
    justification: str
    predicted_outcome: Optional[np.ndarray] = None
    predicted_reward: float = 0.0


class MAGENArcAgiAgentV23:
    """
    Agent MAGEN V23 pour ARC-AGI-3
    
    Architecture:
    - V22: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17 + C18
    - V23: ActionDiscovery + WorldModel + InformationGainReward
    
    Workflow:
    1. Phase Exploration (50 actions): Découvrir actions et construire world model
    2. Phase Exploitation (150 actions): Utiliser prédictions pour actions informées
    """
    
    def __init__(self):
        # ===== MODULES V22 =====
        self.tle = TransformationLearningEngine()
        self.pattern_matcher = PatternMatcher()
        self.object_extractor = ObjectExtractor()
        self.meta_arbiter = MetaArbiter()
        self.reputation_system = ActionReputationSystem()
        self.budget_manager = ExplorationBudgetManager()
        
        # ===== MODULES V23 (NOUVEAUX) =====
        self.action_discovery = ActionDiscoveryEngine(exploration_budget=50)
        self.world_model = WorldModel(max_transitions=1000)
        self.info_gain_reward = InformationGainReward(
            knowledge_weight=0.1,
            causal_weight=0.05,
            novelty_weight=0.1
        )
        
        # ===== ÉTAT AGENT =====
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # Mapping actions
        self.action_mapping = {
            'rotate_90': GameAction.ACTION1,
            'rotate_180': GameAction.ACTION2,
            'mirror_horizontal': GameAction.ACTION3,
            'mirror_vertical': GameAction.ACTION4,
            'identity': GameAction.ACTION5,
            'action_6': GameAction.ACTION6,  # À découvrir
            'undo': GameAction.ACTION7
        }
        
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
            'avg_info_gain_reward': 0.0
        }
        
        print("[MAGEN V23] Agent initialisé avec architecture complète")
        print("[MAGEN V23] V22: TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18")
        print("[MAGEN V23] V23: ActionDiscovery+WorldModel+InformationGainReward")
    
    def perceive(self, obs) -> GridObservation:
        """
        PHASE 1: PERCEPTION
        Analyse l'observation et extrait patterns + objets
        """
        # Extraire frame depuis obs (FrameDataRaw)
        # obs.frame peut être une liste Python ou numpy array
        if hasattr(obs, 'frame') and obs.frame is not None:
            frame = obs.frame
            
            # Conversion liste → numpy array si nécessaire
            if isinstance(frame, list):
                frame = np.array(frame, dtype=np.uint8)
            
            # Conversion RGB → grayscale pour compatibilité avec GridObservation
            # Utiliser moyenne des canaux RGB
            if len(frame.shape) == 3 and frame.shape[2] == 3:
                grid = np.mean(frame, axis=2).astype(np.uint8)
            else:
                grid = frame.astype(np.uint8)
        else:
            # Fallback: grille vide 10x10
            grid = np.zeros((10, 10), dtype=np.uint8)
        
        # DÉSACTIVÉ pour jeux interactifs ARC-AGI-3
        # PatternMatcher et ObjectExtractor sont conçus pour puzzles ARC statiques
        # V23 utilise Action Discovery à la place
        patterns = []
        objects = []
        
        # TODO V24: Adapter PatternMatcher pour images RGB jeux interactifs
        # TODO V24: Adapter ObjectExtractor pour détection objets dans frames
        
        self.stats['patterns_detected'] += len(patterns)
        self.stats['objects_extracted'] += len(objects)
        
        return GridObservation(
            grid=grid,
            patterns=patterns,
            objects=objects,
            metadata={
                'grid_size': grid.shape,
                'color_count': len(np.unique(grid)),
                'levels_completed': obs.levels_completed if hasattr(obs, 'levels_completed') else 0
            }
        )
    
    def decide_exploration(self, obs: GridObservation, available_actions: List[GameAction]) -> ActionDecision:
        """
        PHASE EXPLORATION: Découvrir les actions
        
        Stratégie:
        - Tester chaque action plusieurs fois
        - Observer effets causaux
        - Construire world model
        """
        # Choisir action à explorer
        action_names = list(self.action_mapping.keys())
        
        # Prioriser actions peu observées
        unexplored_actions = [name for name in action_names if self.action_discovery.should_explore(name)]
        
        if unexplored_actions:
            # Choisir action avec bonus exploration maximal
            best_action_name = max(unexplored_actions, 
                                  key=lambda name: self.info_gain_reward.get_exploration_bonus(name))
        else:
            # Toutes actions explorées, choisir aléatoirement
            best_action_name = np.random.choice(action_names)
        
        action = self.action_mapping[best_action_name]
        
        # Prédire outcome si world model disponible
        predicted_outcome = None
        predicted_reward = 0.0
        if len(self.world_model.transitions) > 10:
            predicted_outcome = self.world_model.predict_outcome(obs.grid, best_action_name)
            predicted_reward = self.world_model.predict_reward(obs.grid, best_action_name)
        
        print(f"[EXPLORATION] Action: {best_action_name} (bonus={self.info_gain_reward.get_exploration_bonus(best_action_name):.2f})")
        
        return ActionDecision(
            action=action,
            action_name=best_action_name,
            score=self.info_gain_reward.get_exploration_bonus(best_action_name),
            strategy='exploration',
            budget=10.0,  # Budget fixe en exploration
            justification=f"Exploration action {best_action_name}",
            predicted_outcome=predicted_outcome,
            predicted_reward=predicted_reward
        )
    
    def decide_exploitation(self, obs: GridObservation, available_actions: List[GameAction]) -> ActionDecision:
        """
        PHASE EXPLOITATION: Utiliser world model pour actions informées
        
        Stratégie:
        - Prédire outcome de chaque action
        - Choisir action avec meilleur outcome prédit
        - Utiliser C17+C18 pour scoring
        """
        action_scores = {}
        action_predictions = {}
        
        for action_name, action in self.action_mapping.items():
            if action not in available_actions:
                continue
            
            # Prédire outcome
            predicted_state = self.world_model.predict_outcome(obs.grid, action_name)
            predicted_reward = self.world_model.predict_reward(obs.grid, action_name)
            
            # Score = predicted_reward × reputation × budget
            reputation = self.reputation_system.get_action_priority(
                action_name=action_name,
                context={'grid_size': obs.grid.shape, 'color_count': len(np.unique(obs.grid))}
            )
            
            budget_alloc = self.budget_manager.allocate_budget(
                action_name=action_name,
                reputation=reputation,
                grid_size=obs.grid.shape[0] * obs.grid.shape[1],
                color_count=len(np.unique(obs.grid))
            )
            
            score = predicted_reward * reputation * (budget_alloc.allocated_budget / 10.0)
            
            action_scores[action_name] = score
            action_predictions[action_name] = (predicted_state, predicted_reward, budget_alloc.allocated_budget)
        
        # Choisir meilleure action
        if action_scores:
            best_action_name = max(action_scores.items(), key=lambda x: x[1])[0]
            best_score = action_scores[best_action_name]
            predicted_state, predicted_reward, budget = action_predictions[best_action_name]
            
            print(f"[EXPLOITATION] Action: {best_action_name} (score={best_score:.3f}, pred_reward={predicted_reward:.3f})")
            
            return ActionDecision(
                action=self.action_mapping[best_action_name],
                action_name=best_action_name,
                score=best_score,
                strategy='exploitation',
                budget=budget,
                justification=f"Predicted reward: {predicted_reward:.3f}",
                predicted_outcome=predicted_state,
                predicted_reward=predicted_reward
            )
        else:
            # Fallback: action aléatoire
            action_name = np.random.choice(list(self.action_mapping.keys()))
            return ActionDecision(
                action=self.action_mapping[action_name],
                action_name=action_name,
                score=0.0,
                strategy='random_fallback',
                budget=5.0,
                justification="No valid predictions",
                predicted_outcome=None,
                predicted_reward=0.0
            )
    
    def decide(self, obs: GridObservation, available_actions: List[GameAction]) -> ActionDecision:
        """
        PHASE 2: COGNITION
        Décide de l'action à prendre (exploration ou exploitation)
        """
        # Choisir phase
        if self.exploration_phase_active and self.exploration_actions_taken < 50:
            decision = self.decide_exploration(obs, available_actions)
            self.exploration_actions_taken += 1
        else:
            if self.exploration_phase_active:
                print(f"\n[MAGEN V23] ===== FIN PHASE EXPLORATION =====")
                print(f"[MAGEN V23] Actions découvertes: {len(self.action_discovery.action_models)}")
                print(f"[MAGEN V23] World model: {len(self.world_model.transitions)} transitions")
                print(f"[MAGEN V23] ===== DÉBUT PHASE EXPLOITATION =====\n")
                self.exploration_phase_active = False
            
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
        Met à jour tous les modèles après observation du résultat
        """
        # Calculer reward
        reward_base = obs_after.metadata['levels_completed'] - obs_before.metadata['levels_completed']
        
        # Créer transition
        transition_ad = ADStateTransition(
            state_before=obs_before.grid,
            action=decision.action_name,
            state_after=obs_after.grid,
            reward=reward_base,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        
        transition_wm = WMStateTransition(
            state_before=obs_before.grid,
            action=decision.action_name,
            state_after=obs_after.grid,
            reward=reward_base,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        
        transition_ig = IGStateTransition(
            state_before=obs_before.grid,
            action=decision.action_name,
            state_after=obs_after.grid,
            reward=reward_base,
            context=obs_after.metadata,
            timestamp=time.time()
        )
        
        # 1. Enregistrer dans world model
        self.world_model.record_transition(transition_wm)
        
        # 2. Calculer reward total (avec information gain)
        total_reward = self.info_gain_reward.compute_reward(transition_ig)
        
        # 3. Mettre à jour action discovery
        if self.exploration_phase_active:
            # En exploration: accumuler transitions pour découverte
            self.action_discovery.all_transitions.append(transition_ad)
            
            # Découvrir action périodiquement
            if len(self.action_discovery.all_transitions) % 10 == 0:
                action_transitions = [t for t in self.action_discovery.all_transitions if t.action == decision.action_name]
                if len(action_transitions) >= 3:
                    self.action_discovery.discover_action(decision.action_name, action_transitions)
                    self.stats['actions_discovered'] = len(self.action_discovery.action_models)
        
        # 4. Mettre à jour C17 (reputation)
        success = reward_base > 0
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
        
        # Logging
        if self.total_actions % 10 == 0:
            print(f"[MAGEN V23] Actions: {self.total_actions} | Phase: {'EXPLORATION' if self.exploration_phase_active else 'EXPLOITATION'}")
            print(f"  World Model: {len(self.world_model.transitions)} transitions, accuracy={self.stats['world_model_accuracy']:.2f}")
            print(f"  Info Gain: avg_reward={self.stats['avg_info_gain_reward']:.3f}")
            print(f"  Actions découvertes: {self.stats['actions_discovered']}")
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques complètes"""
        return {
            **self.stats,
            'action_discovery_stats': self.action_discovery.get_statistics(),
            'world_model_stats': self.world_model.get_statistics(),
            'info_gain_stats': self.info_gain_reward.get_statistics()
        }
    
    def reset_for_new_game(self):
        """Réinitialise l'agent pour un nouveau jeu"""
        self.exploration_phase_active = True
        self.exploration_actions_taken = 0
        self.exploitation_actions_taken = 0
        self.total_actions = 0
        
        # Réinitialiser info gain reward (nouveau jeu = nouveaux états)
        self.info_gain_reward.reset()
        
        print("[MAGEN V23] Agent réinitialisé pour nouveau jeu")

# Made with Bob
