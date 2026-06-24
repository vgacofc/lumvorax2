"""
PHASE 6 V40.3: Policy Manager avec Consultation World Model FORCÉE
===================================================================

CORRECTION CRITIQUE identifiée par audit complet:
- V39: PolicyManager ne consultait JAMAIS le World Model (32/36 APIs inutilisées)
- V40.3: Consultation FORCÉE de tous les modules dans decide_action()

Changements majeurs:
1. decide_action() accepte maintenant world_graph, agent_loc, causal_mem, learning_sys
2. Consultation OBLIGATOIRE avant chaque décision
3. Traçage forensique complet de TOUTES les consultations
4. Décisions INFLUENCÉES par les données du World Model
5. Correction effondrement puzzle 10+ (forcer exploration si couverture < 10%)

Auteur: Bob (Assistant IA)
Date: 2026-06-16
Version: V40.3 (Correction Audit Complet)
"""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import numpy as np
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
import json

from reputation_system_v39 import ReputationSystem
from budget_allocator_v39 import BudgetAllocator
from trajectory_monitor_v39 import TrajectoryMonitor
# PHASE 2 (P1): Intégration DecisionKernelV34Causal (Session 79)
try:
    from decision_kernel_v34_causal import DecisionKernelV34Causal
except ImportError:
    DecisionKernelV34Causal = None  # Sera géré dans __init__


class ExplorationMode(Enum):
    """Modes d'exploration."""
    EXPLORE = "explore"
    EXPLOIT = "exploit"
    BALANCED = "balanced"
    ADAPTIVE = "adaptive"


class ProblemType(Enum):
    """Types de problèmes."""
    ARC_AGI = "arc_agi"
    ARCADE = "arcade"
    UNKNOWN = "unknown"


@dataclass
class PolicyState:
    """État de la politique courante."""
    mode: ExplorationMode = ExplorationMode.BALANCED
    problem_type: ProblemType = ProblemType.UNKNOWN
    exploration_rate: float = 0.5
    step_count: int = 0
    success_count: int = 0
    failure_count: int = 0
    last_reward: float = 0.0
    avg_reward: float = 0.0
    strategy_changes: int = 0
    
    def update(self, success: bool, reward: float):
        """Met à jour l'état."""
        self.step_count += 1
        self.last_reward = reward
        
        if success:
            self.success_count += 1
        else:
            self.failure_count += 1
        
        alpha = 0.1
        self.avg_reward = (1 - alpha) * self.avg_reward + alpha * reward
    
    def get_success_rate(self) -> float:
        """Retourne le taux de succès."""
        total = self.success_count + self.failure_count
        return self.success_count / total if total > 0 else 0.0


@dataclass
class WorldModelConsultation:
    """Résultat d'une consultation du World Model."""
    world_graph_consulted: bool = False
    agent_loc_consulted: bool = False
    causal_mem_consulted: bool = False
    learning_sys_consulted: bool = False
    
    # Données extraites
    coverage: Optional[float] = None
    unvisited_cells_count: Optional[int] = None
    suggested_position: Optional[Tuple[int, int]] = None
    agent_confidence: Optional[float] = None
    best_transformations: List[str] = field(default_factory=list)
    learned_scores: Dict[str, float] = field(default_factory=lambda: {})
    
    # Impact sur décision
    decision_influenced: bool = False
    influence_reason: str = ""
    
    def __hash__(self) -> int:
        """Rend l'objet hashable pour utilisation comme clé de dictionnaire.
        
        BUG #5 FIX: Implémentation de __hash__() pour éviter 'unhashable type: WorldModelConsultation'
        Utilise un tuple des attributs immuables pour générer le hash.
        """
        # Convertir les listes et dicts en tuples pour le hashing
        transformations_tuple = tuple(self.best_transformations)
        scores_tuple = tuple(sorted(self.learned_scores.items()))
        suggested_pos_tuple = self.suggested_position if self.suggested_position else (None, None)
        
        return hash((
            self.world_graph_consulted,
            self.agent_loc_consulted,
            self.causal_mem_consulted,
            self.learning_sys_consulted,
            self.coverage,
            self.unvisited_cells_count,
            suggested_pos_tuple,
            self.agent_confidence,
            transformations_tuple,
            scores_tuple,
            self.decision_influenced,
            self.influence_reason
        ))
    
    def __eq__(self, other) -> bool:
        """Implémente l'égalité pour cohérence avec __hash__().
        
        BUG #5 FIX: Nécessaire pour utilisation correcte dans dictionnaires/sets.
        """
        if not isinstance(other, WorldModelConsultation):
            return False
        
        return (
            self.world_graph_consulted == other.world_graph_consulted and
            self.agent_loc_consulted == other.agent_loc_consulted and
            self.causal_mem_consulted == other.causal_mem_consulted and
            self.learning_sys_consulted == other.learning_sys_consulted and
            self.coverage == other.coverage and
            self.unvisited_cells_count == other.unvisited_cells_count and
            self.suggested_position == other.suggested_position and
            self.agent_confidence == other.agent_confidence and
            self.best_transformations == other.best_transformations and
            self.learned_scores == other.learned_scores and
            self.decision_influenced == other.decision_influenced and
            self.influence_reason == other.influence_reason
        )
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire pour forensic."""
        return {
            'consulted': {
                'world_graph': self.world_graph_consulted,
                'agent_loc': self.agent_loc_consulted,
                'causal_mem': self.causal_mem_consulted,
                'learning_sys': self.learning_sys_consulted
            },
            'data': {
                'coverage': self.coverage,
                'unvisited_cells': self.unvisited_cells_count,
                'suggested_position': self.suggested_position,
                'agent_confidence': self.agent_confidence,
                'best_transformations': self.best_transformations,
                'learned_scores': self.learned_scores
            },
            'impact': {
                'decision_influenced': self.decision_influenced,
                'reason': self.influence_reason
            }
        }


class PolicyManagerV40_3:
    """
    Gestionnaire de politique V40.3 avec consultation World Model FORCÉE.
    
    CORRECTION CRITIQUE:
    - V39: decide_action() ne consultait JAMAIS le World Model
    - V40.3: Consultation OBLIGATOIRE de tous les modules
    
    Nouvelles fonctionnalités:
    - Consultation world_graph, agent_loc, causal_mem, learning_sys
    - Traçage forensique complet
    - Décisions INFLUENCÉES par World Model
    - Correction effondrement puzzle 10+
    """
    
    def __init__(self,
                 reputation_system: Optional[ReputationSystem] = None,
                 budget_allocator: Optional[BudgetAllocator] = None,
                 trajectory_monitor: Optional[TrajectoryMonitor] = None,
                 initial_mode: ExplorationMode = ExplorationMode.BALANCED,
                 forensic_log_path: Optional[str] = None,
                 enable_decision_kernel_v34: bool = False):
        """
        Initialise le gestionnaire de politique V40.3.
        
        Args:
            reputation_system: Système de réputation (C17)
            budget_allocator: Allocateur de budget (C18)
            trajectory_monitor: Moniteur de trajectoire (C19)
            initial_mode: Mode initial
            forensic_log_path: Chemin log forensic
            enable_decision_kernel_v34: Activer DecisionKernelV34Causal (Phase 2)
        """
        self.reputation = reputation_system or ReputationSystem()
        self.budget = budget_allocator or BudgetAllocator()
        self.trajectory = trajectory_monitor or TrajectoryMonitor()
        
        self.state = PolicyState(mode=initial_mode)
        
        self.decision_history: List[Dict] = []
        self.mode_performance: Dict[ExplorationMode, List[float]] = {
            mode: [] for mode in ExplorationMode
        }
        
        # V40.3: Statistiques consultation World Model
        self.consultation_stats = {
            'total_decisions': 0,
            'world_model_consulted': 0,
            'decisions_influenced': 0,
            'consultation_rate': 0.0,
            'influence_rate': 0.0
        }
        
        # PHASE 2 (P1): DecisionKernelV34Causal optionnel
        self.decision_kernel_v34 = None
        self.enable_decision_kernel_v34 = enable_decision_kernel_v34
        if enable_decision_kernel_v34 and DecisionKernelV34Causal is not None:
            try:
                self.decision_kernel_v34 = DecisionKernelV34Causal(
                    logger=None,
                    forensic_log_path=forensic_log_path,
                    enable_metacognition=True,
                    enable_reputation=False,  # Utiliser reputation_system existant
                    causal_delta_threshold=0.05
                )
                print("[PHASE 2] ✅ DecisionKernelV34Causal activé")
            except Exception as e:
                print(f"[PHASE 2] ⚠️  Erreur activation DecisionKernelV34: {e}")
                print("[PHASE 2] ℹ️  Utilisation stratégie V40.3 standard")
                self.decision_kernel_v34 = None
        elif enable_decision_kernel_v34:
            print("[PHASE 2] ⚠️  DecisionKernelV34Causal non disponible (import failed)")
            print("[PHASE 2] ℹ️  Utilisation stratégie V40.3 standard")
        
        # Forensic
        self.forensic_log_path = forensic_log_path
    
    def set_problem_type(self, problem_type: ProblemType):
        """Configure le type de problème."""
        self.state.problem_type = problem_type
        
        if problem_type == ProblemType.ARC_AGI:
            self.state.exploration_rate = 0.6
            self.state.mode = ExplorationMode.EXPLORE
        elif problem_type == ProblemType.ARCADE:
            self.state.exploration_rate = 0.4
            self.state.mode = ExplorationMode.EXPLOIT
    
    def decide_action(self, 
                     available_actions: List[str],
                     world_graph: Optional[Any] = None,
                     agent_loc: Optional[Any] = None,
                     causal_mem: Optional[Any] = None,
                     learning_sys: Optional[Any] = None) -> Tuple[str, WorldModelConsultation]:
        """
        Décide quelle action prendre avec consultation FORCÉE du World Model.
        
        CHANGEMENT MAJEUR V40.3:
        - Accepte maintenant world_graph, agent_loc, causal_mem, learning_sys
        - Consultation OBLIGATOIRE avant décision
        - Traçage forensique complet
        - Décision INFLUENCÉE par World Model
        
        Args:
            available_actions: Actions disponibles
            world_graph: WorldStateGraph (optionnel mais recommandé)
            agent_loc: AgentLocalizationSystem (optionnel)
            causal_mem: CausalMemorySystem (optionnel)
            learning_sys: MinimalLearningSystem (optionnel)
        
        Returns:
            (action_choisie, consultation_details)
        """
        if not available_actions:
            raise ValueError("Aucune action disponible")
        
        self.consultation_stats['total_decisions'] += 1
        
        # CONSULTATION WORLD MODEL (V40.3)
        consultation = self._consult_world_model(
            available_actions, world_graph, agent_loc, causal_mem, learning_sys
        )
        
        # Filtrer actions blacklistées
        valid_actions = [a for a in available_actions 
                        if not self.reputation.is_blacklisted(a)]
        
        if not valid_actions:
            valid_actions = available_actions
        
        # CORRECTION EFFONDREMENT PUZZLE 10+ (V40.3)
        if consultation.world_graph_consulted and consultation.coverage is not None:
            if consultation.coverage < 0.1:
                # Forcer exploration si couverture < 10%
                self.state.mode = ExplorationMode.EXPLORE
                self.state.exploration_rate = 0.9
                consultation.decision_influenced = True
                consultation.influence_reason = "forced_exploration_low_coverage"
        
        # DÉCISION INFLUENCÉE PAR WORLD MODEL (V40.3)
        action = self._make_informed_decision(
            valid_actions, consultation
        )
        
        # Enregistrer décision
        self._record_decision(action, available_actions, consultation)
        
        # Traçage forensique
        self._log_forensic('decision_made', {
            'action': action,
            'available_actions': available_actions,
            'consultation': consultation.to_dict(),
            'mode': self.state.mode.value,
            'exploration_rate': self.state.exploration_rate
        })
        
        return action, consultation
    
    def _consult_world_model(self,
                            available_actions: List[str],
                            world_graph: Optional[Any],
                            agent_loc: Optional[Any],
                            causal_mem: Optional[Any],
                            learning_sys: Optional[Any]) -> WorldModelConsultation:
        """
        Consulte TOUS les modules du World Model.
        
        NOUVEAU V40.3: Fonction dédiée à la consultation.
        """
        consultation = WorldModelConsultation()
        
        # Consulter WorldStateGraph
        if world_graph:
            consultation.world_graph_consulted = True
            consultation.coverage = world_graph.get_coverage()
            unvisited = world_graph.get_unvisited_cells()
            consultation.unvisited_cells_count = len(unvisited)
            self.consultation_stats['world_model_consulted'] += 1
        
        # Consulter AgentLocalizationSystem
        if agent_loc:
            consultation.agent_loc_consulted = True
            consultation.suggested_position = agent_loc.suggest_next_position()
            consultation.agent_confidence = agent_loc.get_position_confidence()
        
        # Consulter CausalMemorySystem
        if causal_mem:
            consultation.causal_mem_consulted = True
            best_transforms = causal_mem.get_best_transformations(top_k=5)
            consultation.best_transformations = [t[0] for t in best_transforms]
        
        # Consulter MinimalLearningSystem
        if learning_sys:
            consultation.learning_sys_consulted = True
            consultation.learned_scores = {
                action: learning_sys.get_action_score(action)
                for action in available_actions
            }
        
        return consultation
    
    def _make_informed_decision(self,
                               valid_actions: List[str],
                               consultation: WorldModelConsultation) -> str:
        """
        Prend décision INFORMÉE par le World Model.
        
        NOUVEAU V40.3: Utilise les données consultées pour décider.
        """
        # Calculer scores combinés
        action_scores = {}
        
        for action in valid_actions:
            score = 0.0
            
            # Score réputation (baseline)
            reputation_score = self.reputation.get_action_priority(action)
            score += 0.3 * reputation_score
            
            # Score apprentissage (si disponible)
            if consultation.learned_scores and action in consultation.learned_scores:
                learned_score = consultation.learned_scores[action]
                score += 0.3 * learned_score
                consultation.decision_influenced = True
                consultation.influence_reason = "learned_scores_used"
            
            # Bonus transformations causales (si disponible)
            if action in consultation.best_transformations:
                score += 0.2
                consultation.decision_influenced = True
                consultation.influence_reason = "causal_patterns_used"
            
            # Bonus exploration (si couverture faible)
            if consultation.coverage is not None and consultation.coverage < 0.3:
                # Favoriser actions exploratoires
                if 'identity' not in action.lower():
                    score += 0.2
                    consultation.decision_influenced = True
                    consultation.influence_reason = "exploration_bonus_low_coverage"
            
            action_scores[action] = score
        
        # Décision selon mode
        if self.state.mode == ExplorationMode.EXPLORE:
            # Exploration: Ajouter bruit aléatoire
            action = max(action_scores, key=lambda a: action_scores[a] + np.random.random() * 0.5)
        
        elif self.state.mode == ExplorationMode.EXPLOIT:
            # Exploitation: Meilleur score
            action = max(action_scores.items(), key=lambda x: x[1])[0]
        
        else:  # BALANCED ou ADAPTIVE
            # ε-greedy avec scores combinés
            if np.random.random() < self.state.exploration_rate:
                action = np.random.choice(valid_actions)
            else:
                action = max(action_scores.items(), key=lambda x: x[1])[0]
        
        # Mettre à jour stats si influencé
        if consultation.decision_influenced:
            self.consultation_stats['decisions_influenced'] += 1
        
        # Calculer taux
        if self.consultation_stats['total_decisions'] > 0:
            self.consultation_stats['consultation_rate'] = (
                self.consultation_stats['world_model_consulted'] / 
                self.consultation_stats['total_decisions']
            )
            self.consultation_stats['influence_rate'] = (
                self.consultation_stats['decisions_influenced'] / 
                self.consultation_stats['total_decisions']
            )
        
        return action
    
    def update_after_action(self, action_id: str, state_hash: str,
                           success: bool, reward: float, error: float,
                           steps_used: int, time_ms: float, memory_mb: float):
        """Met à jour tous les systèmes après une action."""
        self.state.update(success, reward)
        
        self.reputation.record_action(action_id, success, error)
        self.budget.record_usage(action_id, steps_used, time_ms, memory_mb,
                                success, reward)
        self.trajectory.add_point(self.state.step_count, state_hash, action_id,
                                 reward, error, success)
        
        if self.trajectory.should_change_strategy():
            self._change_strategy()
        
        self.mode_performance[self.state.mode].append(reward)
    
    def _change_strategy(self):
        """Change la stratégie d'exploration."""
        self.state.strategy_changes += 1
        
        recommendation = self.trajectory.get_recommendation()
        
        if recommendation['action'] == 'increase_exploration':
            self.state.exploration_rate = min(1.0, self.state.exploration_rate + 0.1)
            self.state.mode = ExplorationMode.EXPLORE
        
        elif recommendation['action'] == 'backtrack':
            self.trajectory.reset_trajectory()
            self.state.mode = ExplorationMode.BALANCED
        
        elif recommendation['action'] == 'diversify':
            self.state.mode = ExplorationMode.EXPLORE
            self.state.exploration_rate = 0.8
        
        elif recommendation['action'] == 'explore_new_states':
            self.state.exploration_rate = min(1.0, self.state.exploration_rate + 0.2)
        
        self.decision_history.append({
            'timestamp': datetime.now().isoformat(),
            'type': 'strategy_change',
            'reason': recommendation['reason'],
            'new_mode': self.state.mode.value,
            'new_exploration_rate': self.state.exploration_rate
        })
    
    def _record_decision(self, chosen_action: str, available_actions: List[str],
                        consultation: WorldModelConsultation):
        """Enregistre une décision avec détails consultation."""
        self.decision_history.append({
            'timestamp': datetime.now().isoformat(),
            'type': 'action_decision',
            'chosen_action': chosen_action,
            'available_actions': available_actions,
            'mode': self.state.mode.value,
            'exploration_rate': self.state.exploration_rate,
            'consultation': consultation.to_dict()
        })
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensic."""
        if not self.forensic_log_path:
            return
        
        event = {
            'timestamp': datetime.now().isoformat(),
            'module': 'PolicyManagerV40_3',
            'event_type': event_type,
            'data': data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def get_best_mode(self) -> ExplorationMode:
        """Retourne le meilleur mode selon performance."""
        avg_performance = {}
        for mode, rewards in self.mode_performance.items():
            if rewards:
                avg_performance[mode] = np.mean(rewards)
            else:
                avg_performance[mode] = 0.0
        
        return max(avg_performance.items(), key=lambda x: x[1])[0]
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques complètes."""
        reputation_stats = self.reputation.get_stats()
        budget_stats = self.budget.get_stats()
        trajectory_stats = self.trajectory.get_stats()
        
        mode_perf = {}
        for mode, rewards in self.mode_performance.items():
            if rewards:
                mode_perf[mode.value] = {
                    'avg_reward': float(np.mean(rewards)),
                    'samples': len(rewards)
                }
            else:
                mode_perf[mode.value] = {'avg_reward': 0.0, 'samples': 0}
        
        return {
            'policy_state': {
                'mode': self.state.mode.value,
                'problem_type': self.state.problem_type.value,
                'exploration_rate': self.state.exploration_rate,
                'step_count': self.state.step_count,
                'success_rate': self.state.get_success_rate(),
                'avg_reward': self.state.avg_reward,
                'strategy_changes': self.state.strategy_changes
            },
            'world_model_consultation': self.consultation_stats,  # NOUVEAU V40.3
            'reputation': reputation_stats,
            'budget': budget_stats,
            'trajectory': trajectory_stats,
            'mode_performance': mode_perf,
            'best_mode': self.get_best_mode().value,
            'total_decisions': len(self.decision_history)
        }
    
    def export_decisions(self) -> List[Dict]:
        """Exporte l'historique des décisions."""
        return self.decision_history


# Made with Bob - V40.3 World Model Consultation FORCÉE