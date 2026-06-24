"""
MAGEN V34 - DisengagementEngine
Moteur de Désengagement Stratégique

Mission: Permettre à MAGEN de dire "ça ne marche pas" et d'abandonner intelligemment
Architecture: Composant critique du MetaCognitiveController

NOUVEAUTÉ V34: Capacité d'abandon stratégique des trajectoires stériles

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V34.0.0
"""

import time
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from enum import Enum


class DisengagementReason(Enum):
    """Raisons de désengagement"""
    STERILE_REGION = "sterile_region"
    GLOBAL_STAGNATION = "global_stagnation"
    LOW_FUTURE_VALUE = "low_future_value"
    RESOURCE_EXHAUSTION = "resource_exhaustion"
    COLLAPSE_DETECTED = "collapse_detected"
    MANUAL_OVERRIDE = "manual_override"


@dataclass
class DisengagementDecision:
    """Décision de désengagement"""
    should_disengage: bool
    reason: DisengagementReason
    confidence: float  # 0.0 à 1.0
    alternative_action: Optional[str] = None
    explanation: str = ""
    
    def to_dict(self) -> Dict:
        return {
            "should_disengage": self.should_disengage,
            "reason": self.reason.value,
            "confidence": self.confidence,
            "alternative_action": self.alternative_action,
            "explanation": self.explanation
        }


@dataclass
class TrajectoryState:
    """État d'une trajectoire"""
    current_step: int
    current_score: float
    steps_in_trajectory: int
    total_cost: float
    sterility_score: float
    stagnation_severity: float
    
    def to_dict(self) -> Dict:
        return {
            "current_step": self.current_step,
            "current_score": self.current_score,
            "steps_in_trajectory": self.steps_in_trajectory,
            "total_cost": self.total_cost,
            "sterility_score": self.sterility_score,
            "stagnation_severity": self.stagnation_severity
        }


class DisengagementEngine:
    """
    Moteur de désengagement stratégique.
    
    Principe fondamental:
        Permettre à MAGEN de dire "ça ne marche pas" et d'abandonner
        intelligemment les trajectoires stériles.
    
    Critères de désengagement:
        1. Stérilité élevée (région/action sans reward)
        2. Stagnation globale persistante
        3. Valeur future prédite très faible
        4. Coût > bénéfice attendu
        5. Collapse détecté (attracteur pathologique)
    
    CRITIQUE: Évite exploration infinie de zones mortes
    RISQUE: Abandon prématuré de trajectoires à long terme
    """
    
    def __init__(
        self,
        forensic_logger=None,
        sterility_threshold: float = 0.7,
        stagnation_threshold: float = 0.6,
        cost_benefit_ratio: float = 2.0,
        min_steps_before_disengage: int = 20
    ):
        """
        Initialise le moteur de désengagement
        
        Args:
            forensic_logger: Logger forensic
            sterility_threshold: Seuil stérilité pour désengagement
            stagnation_threshold: Seuil stagnation pour désengagement
            cost_benefit_ratio: Ratio coût/bénéfice max acceptable
            min_steps_before_disengage: Steps min avant désengagement possible
        """
        self.forensic_logger = forensic_logger
        
        # Configuration
        self.sterility_threshold = sterility_threshold
        self.stagnation_threshold = stagnation_threshold
        self.cost_benefit_ratio = cost_benefit_ratio
        self.min_steps_before_disengage = min_steps_before_disengage
        
        # État trajectoire actuelle
        self.trajectory_start_step: int = 0
        self.trajectory_start_score: float = 0.0
        self.trajectory_cost: float = 0.0
        
        # Historique désengagements
        self.disengagement_history: List[DisengagementDecision] = []
        
        # Statistiques
        self.total_evaluations: int = 0
        self.total_disengagements: int = 0
        self.disengagement_by_reason: Dict[DisengagementReason, int] = {
            reason: 0 for reason in DisengagementReason
        }
        
        # Forensic
        self.creation_time = time.time_ns()
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_initialized",
                "DisengagementEngine",
                {
                    "timestamp_ns": self.creation_time,
                    "version": "v34.0.0",
                    "sterility_threshold": self.sterility_threshold,
                    "stagnation_threshold": self.stagnation_threshold,
                    "cost_benefit_ratio": self.cost_benefit_ratio,
                    "min_steps_before_disengage": self.min_steps_before_disengage
                }
            )
    
    def start_trajectory(self, step: int, score: float) -> None:
        """
        Démarre une nouvelle trajectoire
        
        Args:
            step: Step de démarrage
            score: Score initial
        """
        self.trajectory_start_step = step
        self.trajectory_start_score = score
        self.trajectory_cost = 0.0
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "trajectory_started",
                "DisengagementEngine",
                {
                    "step": step,
                    "initial_score": score
                }
            )
    
    def update_trajectory_cost(self, cost_increment: float) -> None:
        """
        Met à jour le coût de la trajectoire actuelle
        
        Args:
            cost_increment: Incrément de coût
        """
        self.trajectory_cost += cost_increment
    
    def evaluate_disengagement(
        self,
        current_step: int,
        current_score: float,
        sterility_score: float,
        stagnation_severity: float,
        future_value_prediction: float = 0.0
    ) -> DisengagementDecision:
        """
        Évalue si désengagement nécessaire
        
        Args:
            current_step: Step actuel
            current_score: Score actuel
            sterility_score: Score stérilité (0.0-1.0)
            stagnation_severity: Sévérité stagnation (0.0-1.0)
            future_value_prediction: Valeur future prédite (0.0-1.0)
            
        Returns:
            DisengagementDecision
        """
        self.total_evaluations += 1
        
        # Créer état trajectoire
        trajectory_state = TrajectoryState(
            current_step=current_step,
            current_score=current_score,
            steps_in_trajectory=current_step - self.trajectory_start_step,
            total_cost=self.trajectory_cost,
            sterility_score=sterility_score,
            stagnation_severity=stagnation_severity
        )
        
        # Vérifier si assez de steps
        if trajectory_state.steps_in_trajectory < self.min_steps_before_disengage:
            return DisengagementDecision(
                should_disengage=False,
                reason=DisengagementReason.MANUAL_OVERRIDE,
                confidence=0.0,
                explanation="Pas assez de steps pour évaluer"
            )
        
        # Critère 1: Stérilité élevée
        if sterility_score >= self.sterility_threshold:
            decision = self._create_disengagement(
                DisengagementReason.STERILE_REGION,
                confidence=sterility_score,
                explanation=f"Région stérile (score={sterility_score:.2f})",
                trajectory_state=trajectory_state
            )
            if decision.should_disengage:
                return decision
        
        # Critère 2: Stagnation globale
        if stagnation_severity >= self.stagnation_threshold:
            decision = self._create_disengagement(
                DisengagementReason.GLOBAL_STAGNATION,
                confidence=stagnation_severity,
                explanation=f"Stagnation globale (sévérité={stagnation_severity:.2f})",
                trajectory_state=trajectory_state
            )
            if decision.should_disengage:
                return decision
        
        # Critère 3: Valeur future faible
        if future_value_prediction < 0.2:
            decision = self._create_disengagement(
                DisengagementReason.LOW_FUTURE_VALUE,
                confidence=0.8,
                explanation=f"Valeur future faible ({future_value_prediction:.2f})",
                trajectory_state=trajectory_state
            )
            if decision.should_disengage:
                return decision
        
        # Critère 4: Coût/bénéfice défavorable
        score_gain = current_score - self.trajectory_start_score
        if self.trajectory_cost > 0 and score_gain > 0:
            ratio = self.trajectory_cost / score_gain
            if ratio > self.cost_benefit_ratio:
                decision = self._create_disengagement(
                    DisengagementReason.RESOURCE_EXHAUSTION,
                    confidence=0.7,
                    explanation=f"Ratio coût/bénéfice trop élevé ({ratio:.2f})",
                    trajectory_state=trajectory_state
                )
                if decision.should_disengage:
                    return decision
        
        # Critère 5: Collapse (combinaison stérilité + stagnation)
        if sterility_score > 0.5 and stagnation_severity > 0.5:
            combined_severity = (sterility_score + stagnation_severity) / 2
            decision = self._create_disengagement(
                DisengagementReason.COLLAPSE_DETECTED,
                confidence=combined_severity,
                explanation=f"Collapse détecté (stérilité={sterility_score:.2f}, stagnation={stagnation_severity:.2f})",
                trajectory_state=trajectory_state
            )
            if decision.should_disengage:
                return decision
        
        # Pas de désengagement
        return DisengagementDecision(
            should_disengage=False,
            reason=DisengagementReason.MANUAL_OVERRIDE,
            confidence=0.0,
            explanation="Trajectoire viable"
        )
    
    def _create_disengagement(
        self,
        reason: DisengagementReason,
        confidence: float,
        explanation: str,
        trajectory_state: TrajectoryState
    ) -> DisengagementDecision:
        """
        Crée décision de désengagement
        
        Args:
            reason: Raison du désengagement
            confidence: Confiance (0.0-1.0)
            explanation: Explication
            trajectory_state: État trajectoire
            
        Returns:
            DisengagementDecision
        """
        # Suggérer action alternative
        alternative = self._suggest_alternative(reason, trajectory_state)
        
        decision = DisengagementDecision(
            should_disengage=True,
            reason=reason,
            confidence=confidence,
            alternative_action=alternative,
            explanation=explanation
        )
        
        # Enregistrer
        self.disengagement_history.append(decision)
        self.total_disengagements += 1
        self.disengagement_by_reason[reason] += 1
        
        # Log forensic
        if self.forensic_logger:
            self.forensic_logger.log(
                "disengagement_decided",
                "DisengagementEngine",
                {
                    "step": trajectory_state.current_step,
                    "decision": decision.to_dict(),
                    "trajectory_state": trajectory_state.to_dict()
                }
            )
        
        return decision
    
    def _suggest_alternative(
        self,
        reason: DisengagementReason,
        trajectory_state: TrajectoryState
    ) -> str:
        """
        Suggère action alternative après désengagement
        
        Args:
            reason: Raison du désengagement
            trajectory_state: État trajectoire
            
        Returns:
            Action alternative suggérée
        """
        if reason == DisengagementReason.STERILE_REGION:
            return "explore_new_region"
        elif reason == DisengagementReason.GLOBAL_STAGNATION:
            return "reset_and_restart"
        elif reason == DisengagementReason.LOW_FUTURE_VALUE:
            return "try_different_approach"
        elif reason == DisengagementReason.RESOURCE_EXHAUSTION:
            return "optimize_and_retry"
        elif reason == DisengagementReason.COLLAPSE_DETECTED:
            return "emergency_reset"
        else:
            return "continue_with_caution"
    
    def should_disengage_now(self) -> bool:
        """
        Vérifie si désengagement immédiat nécessaire
        
        Returns:
            True si désengagement immédiat, False sinon
        """
        if not self.disengagement_history:
            return False
        
        last_decision = self.disengagement_history[-1]
        return last_decision.should_disengage and last_decision.confidence > 0.8
    
    def get_last_decision(self) -> Optional[DisengagementDecision]:
        """Retourne dernière décision"""
        if self.disengagement_history:
            return self.disengagement_history[-1]
        return None
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques"""
        return {
            "total_evaluations": self.total_evaluations,
            "total_disengagements": self.total_disengagements,
            "disengagement_rate": (
                self.total_disengagements / self.total_evaluations
                if self.total_evaluations > 0 else 0.0
            ),
            "disengagement_by_reason": {
                reason.value: count
                for reason, count in self.disengagement_by_reason.items()
            },
            "current_trajectory_cost": self.trajectory_cost,
            "current_trajectory_steps": 0  # Sera calculé par appelant
        }
    
    def get_state(self) -> Dict:
        """Retourne état complet pour forensic"""
        last_decision = self.get_last_decision()
        
        return {
            "statistics": self.get_statistics(),
            "last_decision": last_decision.to_dict() if last_decision else None,
            "should_disengage_now": self.should_disengage_now(),
            "trajectory_state": {
                "start_step": self.trajectory_start_step,
                "start_score": self.trajectory_start_score,
                "total_cost": self.trajectory_cost
            }
        }
    
    def reset(self) -> None:
        """Reset complet"""
        self.trajectory_start_step = 0
        self.trajectory_start_score = 0.0
        self.trajectory_cost = 0.0
        self.disengagement_history.clear()
        self.total_evaluations = 0
        self.total_disengagements = 0
        self.disengagement_by_reason = {
            reason: 0 for reason in DisengagementReason
        }
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "engine_reset",
                "DisengagementEngine",
                {
                    "timestamp_ns": time.time_ns(),
                    "reason": "manual_reset"
                }
            )
    
    def close(self) -> None:
        """Ferme le module"""
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_closing",
                "DisengagementEngine",
                {
                    "timestamp_ns": time.time_ns(),
                    "final_statistics": self.get_statistics(),
                    "final_state": self.get_state()
                }
            )


# Export
__all__ = [
    'DisengagementEngine',
    'DisengagementDecision',
    'DisengagementReason',
    'TrajectoryState'
]

# Made with Bob
