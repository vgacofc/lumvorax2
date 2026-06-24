"""
MAGEN V31 - Meta-Cognitive Controller (C4)
===========================================

**Objectif**: Implémenter le mécanisme de "NON" - savoir quand arrêter une trajectoire

**Principe**: Le système doit savoir "Cette trajectoire ne mérite plus de ressources cognitives"
et casser les attracteurs dominants

**Session**: 73 - Jour 8 - Phase 1
**Date**: 2026-06-15
**Version**: 1.0.0
"""

import time
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
import numpy as np


@dataclass
class TrajectoryMetrics:
    """Métriques viabilité trajectoire"""
    trajectory_id: str
    viability: float = 1.0
    cognitive_cost: float = 0.0
    sterility_pressure: float = 0.0
    exploration_entropy: float = 1.0
    progress_rate: float = 0.0
    collapse_detected: bool = False
    stagnation_detected: bool = False


class MetaCognitiveController:
    """
    Contrôleur métacognitif avec mécanisme de désengagement
    
    Implémente:
    - Évaluation viabilité trajectoire
    - Détection collapse attractif
    - Détection stagnation cognitive
    - Décision désengagement
    - Rupture politique forcée
    """
    
    def __init__(
        self,
        collapse_threshold: float = 0.8,
        stagnation_window: int = 50,
        max_cognitive_cost: float = 1000.0,
        forensic_logger=None
    ):
        """
        Initialise contrôleur métacognitif
        
        Args:
            collapse_threshold: Seuil détection collapse (action dominante)
            stagnation_window: Fenêtre détection stagnation
            max_cognitive_cost: Coût cognitif max avant désengagement
            forensic_logger: Logger forensique
        """
        # Paramètres
        self.collapse_threshold = collapse_threshold
        self.stagnation_window = stagnation_window
        self.max_cognitive_cost = max_cognitive_cost
        
        # Métriques trajectoire courante
        self.current_trajectory: Optional[TrajectoryMetrics] = None
        self.trajectory_history: List[TrajectoryMetrics] = []
        
        # Historique actions
        self.action_history: List[str] = []
        self.action_counts: Dict[str, int] = {}
        
        # Historique progrès
        self.reward_history: List[float] = []
        self.position_history: List[Tuple[int, int]] = []
        self.discovery_history: List[int] = []  # Nouvelles positions par fenêtre
        
        # Détection collapse
        self.dominant_action: Optional[str] = None
        self.dominant_action_ratio: float = 0.0
        self.collapse_count: int = 0
        
        # Détection stagnation
        self.steps_without_progress: int = 0
        self.steps_without_discovery: int = 0
        self.stagnation_count: int = 0
        
        # Contrôle désengagement
        self.should_disengage_flag: bool = False
        self.disengagement_reason: Optional[str] = None
        self.total_disengagements: int = 0
        
        # Policy breaks
        self.policy_breaks: List[Dict] = []
        self.last_break_step: int = 0
        
        # Métriques globales
        self.current_step: int = 0
        self.total_cognitive_cost: float = 0.0
        
        # Forensic
        self.forensic_logger = forensic_logger
        self.events: List[Dict] = []
        
        self._log_event("meta_controller_initialized", {
            "collapse_threshold": collapse_threshold,
            "stagnation_window": stagnation_window,
            "max_cognitive_cost": max_cognitive_cost
        })
    
    def update_trajectory_metrics(
        self,
        action: str,
        position: Tuple[int, int],
        reward: float,
        new_discovery: bool = False
    ) -> TrajectoryMetrics:
        """
        Met à jour métriques trajectoire courante
        
        Args:
            action: Action exécutée
            position: Position atteinte
            reward: Reward obtenu
            new_discovery: Nouvelle position découverte
            
        Returns:
            Métriques trajectoire mises à jour
        """
        self.current_step += 1
        
        # Mettre à jour historiques
        self.action_history.append(action)
        self.action_counts[action] = self.action_counts.get(action, 0) + 1
        self.reward_history.append(reward)
        self.position_history.append(position)
        
        # Créer trajectoire si nécessaire
        if self.current_trajectory is None:
            self.current_trajectory = TrajectoryMetrics(
                trajectory_id=f"traj_{len(self.trajectory_history)}"
            )
        
        traj = self.current_trajectory
        
        # Calculer viabilité
        traj.viability = self._compute_trajectory_viability()
        
        # Calculer coût cognitif
        traj.cognitive_cost = self._compute_cognitive_cost()
        self.total_cognitive_cost += 1.0
        
        # Calculer pression stérilité
        traj.sterility_pressure = self._compute_sterility_pressure()
        
        # Calculer entropie exploration
        traj.exploration_entropy = self._compute_exploration_entropy()
        
        # Calculer taux progrès
        traj.progress_rate = self._compute_progress_rate()
        
        # Détecter collapse
        traj.collapse_detected = self.detect_collapse()
        
        # Détecter stagnation
        traj.stagnation_detected = self.detect_cognitive_stagnation()
        
        # Mettre à jour découvertes
        if new_discovery:
            self.steps_without_discovery = 0
        else:
            self.steps_without_discovery += 1
        
        # Mettre à jour progrès
        if reward > 0:
            self.steps_without_progress = 0
        else:
            self.steps_without_progress += 1
        
        self._log_event("trajectory_metrics_updated", {
            "step": self.current_step,
            "viability": traj.viability,
            "cognitive_cost": traj.cognitive_cost,
            "sterility_pressure": traj.sterility_pressure,
            "exploration_entropy": traj.exploration_entropy,
            "progress_rate": traj.progress_rate,
            "collapse": traj.collapse_detected,
            "stagnation": traj.stagnation_detected
        })
        
        return traj
    
    def detect_collapse(self) -> bool:
        """
        Détecte collapse attractif (action dominante >80%)
        
        Returns:
            True si collapse détecté
        """
        if len(self.action_history) < 10:
            return False
        
        # Analyser dernières actions
        recent_actions = self.action_history[-self.stagnation_window:]
        action_counts = {}
        
        for action in recent_actions:
            action_counts[action] = action_counts.get(action, 0) + 1
        
        # Trouver action dominante
        if action_counts:
            self.dominant_action = max(action_counts.items(), key=lambda x: x[1])[0]
            self.dominant_action_ratio = action_counts[self.dominant_action] / len(recent_actions)
            
            # Collapse si >threshold
            if self.dominant_action_ratio > self.collapse_threshold:
                if not self.current_trajectory or not self.current_trajectory.collapse_detected:
                    self.collapse_count += 1
                    
                    self._log_event("collapse_detected", {
                        "dominant_action": self.dominant_action,
                        "ratio": self.dominant_action_ratio,
                        "threshold": self.collapse_threshold,
                        "collapse_count": self.collapse_count,
                        "step": self.current_step
                    })
                
                return True
        
        return False
    
    def detect_cognitive_stagnation(self) -> bool:
        """
        Détecte stagnation cognitive (absence progrès/découvertes)
        
        Returns:
            True si stagnation détectée
        """
        # Critères stagnation
        no_progress = self.steps_without_progress > self.stagnation_window
        no_discovery = self.steps_without_discovery > self.stagnation_window // 2
        low_entropy = (self.current_trajectory is not None and
                      self.current_trajectory.exploration_entropy < 0.3)
        
        is_stagnant = no_progress or (no_discovery and low_entropy)
        
        if is_stagnant and is_stagnant is not None:
            if not self.current_trajectory or not self.current_trajectory.stagnation_detected:
                self.stagnation_count += 1
                
                self._log_event("stagnation_detected", {
                    "steps_without_progress": self.steps_without_progress,
                    "steps_without_discovery": self.steps_without_discovery,
                    "exploration_entropy": self.current_trajectory.exploration_entropy if self.current_trajectory else 0,
                    "stagnation_count": self.stagnation_count,
                    "step": self.current_step
                })
        
        return is_stagnant
    
    def should_disengage(self) -> Tuple[bool, Optional[str]]:
        """
        Décide si désengagement nécessaire
        
        Returns:
            (should_disengage, reason)
        """
        if self.current_trajectory is None:
            return False, None
        
        traj = self.current_trajectory
        
        # Critères désengagement
        reasons = []
        
        if traj.collapse_detected:
            reasons.append("collapse_attractif")
        
        if traj.stagnation_detected:
            reasons.append("stagnation_cognitive")
        
        if traj.sterility_pressure > 0.8:
            reasons.append("sterility_excessive")
        
        if traj.cognitive_cost > self.max_cognitive_cost:
            reasons.append("cognitive_cost_exceeded")
        
        if traj.viability < 0.2:
            reasons.append("viability_too_low")
        
        should_disengage = len(reasons) > 0
        reason = ", ".join(reasons) if reasons else None
        
        if should_disengage and not self.should_disengage_flag:
            self.should_disengage_flag = True
            self.disengagement_reason = reason
            self.total_disengagements += 1
            
            self._log_event("disengagement_triggered", {
                "reason": reason,
                "viability": traj.viability,
                "cognitive_cost": traj.cognitive_cost,
                "sterility_pressure": traj.sterility_pressure,
                "total_disengagements": self.total_disengagements,
                "step": self.current_step
            })
        
        return should_disengage, reason
    
    def force_policy_break(
        self,
        penalty_duration: int = 100
    ) -> Dict:
        """
        Force rupture politique brutale
        
        Args:
            penalty_duration: Durée pénalité action dominante
            
        Returns:
            Dict avec détails break
        """
        break_info = {
            "break_id": f"break_{len(self.policy_breaks)}",
            "step": self.current_step,
            "dominant_action": self.dominant_action,
            "dominant_ratio": self.dominant_action_ratio,
            "penalty_duration": penalty_duration,
            "reason": self.disengagement_reason or "forced_break"
        }
        
        self.policy_breaks.append(break_info)
        self.last_break_step = self.current_step
        
        # Reset flags
        self.should_disengage_flag = False
        self.disengagement_reason = None
        
        # Archiver trajectoire courante
        if self.current_trajectory:
            self.trajectory_history.append(self.current_trajectory)
            self.current_trajectory = None
        
        self._log_event("policy_break_forced", break_info)
        
        return break_info
    
    def get_meta_statistics(self) -> Dict:
        """
        Retourne statistiques métacognitives
        
        Returns:
            Dict avec métriques
        """
        return {
            "current_step": self.current_step,
            "total_cognitive_cost": self.total_cognitive_cost,
            "collapse_count": self.collapse_count,
            "stagnation_count": self.stagnation_count,
            "total_disengagements": self.total_disengagements,
            "policy_breaks": len(self.policy_breaks),
            "dominant_action": self.dominant_action,
            "dominant_ratio": self.dominant_action_ratio,
            "steps_without_progress": self.steps_without_progress,
            "steps_without_discovery": self.steps_without_discovery,
            "current_viability": self.current_trajectory.viability if self.current_trajectory else 0.0,
            "current_entropy": self.current_trajectory.exploration_entropy if self.current_trajectory else 0.0
        }
    
    def _compute_trajectory_viability(self) -> float:
        """Calcule viabilité trajectoire"""
        if len(self.reward_history) < 5:
            return 1.0
        
        # Composantes viabilité
        recent_rewards = self.reward_history[-20:]
        avg_reward = np.mean(recent_rewards)
        
        # Tendance progrès
        if len(recent_rewards) >= 10:
            first_half = np.mean(recent_rewards[:len(recent_rewards)//2])
            second_half = np.mean(recent_rewards[len(recent_rewards)//2:])
            progress_trend = (second_half - first_half) / (abs(first_half) + 1e-6)
        else:
            progress_trend = 0.0
        
        # Diversité exploration
        recent_positions = self.position_history[-20:]
        unique_positions = len(set(recent_positions))
        diversity = unique_positions / len(recent_positions) if recent_positions else 0.0
        
        # Formule viabilité
        viability = (
            0.4 * max(0.0, avg_reward) +
            0.3 * max(0.0, progress_trend + 0.5) +
            0.3 * diversity
        )
        
        return float(np.clip(viability, 0.0, 1.0))
    
    def _compute_cognitive_cost(self) -> float:
        """Calcule coût cognitif accumulé"""
        # Coût = steps + revisites + stagnation
        revisit_cost = len(self.position_history) - len(set(self.position_history))
        stagnation_cost = self.steps_without_progress * 0.5
        
        return float(len(self.action_history) + revisit_cost + stagnation_cost)
    
    def _compute_sterility_pressure(self) -> float:
        """Calcule pression stérilité"""
        if len(self.reward_history) < 10:
            return 0.0
        
        # Absence reward récent
        recent_rewards = self.reward_history[-20:]
        zero_rewards = sum(1 for r in recent_rewards if r <= 0)
        sterility = zero_rewards / len(recent_rewards)
        
        return float(sterility)
    
    def _compute_exploration_entropy(self) -> float:
        """Calcule entropie exploration"""
        if len(self.action_history) < 5:
            return 1.0
        
        # Entropie Shannon sur actions récentes
        recent_actions = self.action_history[-50:]
        action_counts = {}
        
        for action in recent_actions:
            action_counts[action] = action_counts.get(action, 0) + 1
        
        total = len(recent_actions)
        probs = [count / total for count in action_counts.values()]
        
        entropy = -sum(p * np.log(p + 1e-10) for p in probs)
        max_entropy = np.log(len(action_counts) + 1e-10)
        
        normalized_entropy = entropy / max_entropy if max_entropy > 0 else 0.0
        
        return float(np.clip(normalized_entropy, 0.0, 1.0))
    
    def _compute_progress_rate(self) -> float:
        """Calcule taux progrès"""
        if len(self.reward_history) < 10:
            return 0.0
        
        recent_rewards = self.reward_history[-20:]
        positive_rewards = sum(1 for r in recent_rewards if r > 0)
        
        return float(positive_rewards / len(recent_rewards))
    
    def _log_event(
        self,
        event_type: str,
        data: Dict
    ) -> None:
        """Log événement forensique"""
        event = {
            "timestamp_ns": time.perf_counter_ns(),
            "event_type": f"meta.{event_type}",
            "data": data
        }
        
        self.events.append(event)
        
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type=event["event_type"],
                component="meta_cognitive_controller",
                data=data
            )
    
    def reset(self) -> None:
        """Reset complet contrôleur"""
        self.current_trajectory = None
        self.trajectory_history.clear()
        self.action_history.clear()
        self.action_counts.clear()
        self.reward_history.clear()
        self.position_history.clear()
        self.discovery_history.clear()
        self.dominant_action = None
        self.dominant_action_ratio = 0.0
        self.collapse_count = 0
        self.steps_without_progress = 0
        self.steps_without_discovery = 0
        self.stagnation_count = 0
        self.should_disengage_flag = False
        self.disengagement_reason = None
        self.total_disengagements = 0
        self.policy_breaks.clear()
        self.last_break_step = 0
        self.current_step = 0
        self.total_cognitive_cost = 0.0
        self.events.clear()
        
        self._log_event("meta_controller_reset", {
            "reason": "manual_reset"
        })

# Made with Bob
