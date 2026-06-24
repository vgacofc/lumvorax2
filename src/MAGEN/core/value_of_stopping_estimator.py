"""
MAGEN V35 - ValueOfStoppingEstimator
Estimateur de Valeur d'Arrêt

Mission: Décider "continuer ou arrêter" basé sur coût/bénéfice
Architecture: V34 + Fonction de Valeur d'Arrêt (Value of Stopping)

NOUVEAUTÉ V35: DÉCISION STRATÉGIQUE D'ARRÊT
- Estime valeur de continuer vs arrêter
- Compare coût futur attendu vs bénéfice potentiel
- Apprend seuils optimaux par expérience
- Évite gaspillage computationnel

CRITIQUE: Implémente "cortex préfrontal" de MAGEN - capacité de dire "stop"

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V35.0.0
"""

import time
import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from pathlib import Path
from enum import Enum

# V35: Forensic standard LEÇON-73.1
from .forensic_middleware import ForensicMiddleware


class StoppingDecision(Enum):
    """Décision d'arrêt"""
    CONTINUE = "continue"
    STOP_LOW_VALUE = "stop_low_value"
    STOP_HIGH_COST = "stop_high_cost"
    STOP_DIMINISHING_RETURNS = "stop_diminishing_returns"


@dataclass
class StoppingEvaluation:
    """Évaluation décision d'arrêt"""
    decision: StoppingDecision
    value_of_continuing: float  # Valeur estimée si on continue
    value_of_stopping: float  # Valeur estimée si on arrête
    expected_cost: float  # Coût attendu si on continue
    confidence: float  # Confiance dans l'estimation
    reasoning: str
    
    def to_dict(self) -> Dict:
        return {
            "decision": self.decision.value,
            "value_of_continuing": self.value_of_continuing,
            "value_of_stopping": self.value_of_stopping,
            "expected_cost": self.expected_cost,
            "confidence": self.confidence,
            "reasoning": self.reasoning
        }


class ValueOfStoppingEstimator:
    """
    Estimateur de valeur d'arrêt.
    
    Architecture V35:
        ┌─────────────────────────────────────┐
        │   ValueOfStoppingEstimator          │
        │                                     │
        │  ┌──────────────────────────────┐  │
        │  │   Valeur Continuer           │  │
        │  │   V(continue) = E[reward]    │  │
        │  │                - E[cost]     │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │  ┌──────────────────────────────┐  │
        │  │   Valeur Arrêter             │  │
        │  │   V(stop) = current_value    │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │  ┌──────────────────────────────┐  │
        │  │   Décision Optimale          │  │
        │  │   argmax(V(continue), V(stop))│  │
        │  └──────────────────────────────┘  │
        └─────────────────────────────────────┘
    
    Principe:
        1. Estimer valeur future si on continue
        2. Estimer coût futur si on continue
        3. Comparer avec valeur actuelle (arrêt)
        4. Décider action optimale
    
    CRITIQUE: Première implémentation "stop function" dans MAGEN
    """
    
    def __init__(
        self,
        cost_per_step: float = 0.1,
        diminishing_returns_threshold: float = 0.01,
        max_cost_threshold: float = 100.0,
        min_value_threshold: float = -10.0,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise l'estimateur
        
        Args:
            cost_per_step: Coût par step de recherche (0.1 = favorise exploration)
            diminishing_returns_threshold: Seuil rendements décroissants
            max_cost_threshold: Coût maximum acceptable
            min_value_threshold: Valeur minimum pour continuer (-10.0 = très permissif)
            forensic_log_path: Chemin log forensic (LEÇON-73.1)
        """
        self.cost_per_step = cost_per_step
        self.diminishing_returns_threshold = diminishing_returns_threshold
        self.max_cost_threshold = max_cost_threshold
        self.min_value_threshold = min_value_threshold
        
        # Historique décisions
        self.total_evaluations: int = 0
        self.stop_decisions: int = 0
        self.continue_decisions: int = 0
        
        # Apprentissage
        self.historical_rewards: List[float] = []
        self.historical_costs: List[float] = []
        self.max_history: int = 100
        
        # V35: Forensic standard LumVorax (LEÇON-73.1)
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v35/value_of_stopping_estimator.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, self.__class__.__name__)
        
        self.creation_time = time.time_ns()
        
        # Log initialization
        self.forensic.forensic_logger.log(
            "module_initialized",
            self.__class__.__name__,
            {
                "timestamp_ns": self.creation_time,
                "version": "v35.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_evaluations': self.total_evaluations,
            'stop_decisions': self.stop_decisions,
            'continue_decisions': self.continue_decisions,
            'stop_rate': (
                self.stop_decisions / self.total_evaluations
                if self.total_evaluations > 0 else 0.0
            ),
            'cost_per_step': self.cost_per_step,
            'max_cost_threshold': self.max_cost_threshold
        }
    
    def evaluate_stopping(
        self,
        current_value: float,
        current_cost: float,
        trajectory_utility: float,
        convergence_rate: float,
        steps_remaining_estimate: int = 10
    ) -> StoppingEvaluation:
        """
        Évalue si arrêter ou continuer
        
        Args:
            current_value: Valeur actuelle obtenue
            current_cost: Coût déjà payé
            trajectory_utility: Utilité trajectoire (0-1)
            convergence_rate: Vitesse convergence
            steps_remaining_estimate: Steps estimés restants
            
        Returns:
            StoppingEvaluation avec décision et raisonnement
        """
        start_ns = time.time_ns()
        self.total_evaluations += 1
        
        # 1. Estimer valeur de continuer
        expected_future_reward = self._estimate_future_reward(
            trajectory_utility,
            convergence_rate,
            steps_remaining_estimate
        )
        
        expected_future_cost = steps_remaining_estimate * self.cost_per_step
        
        value_of_continuing = expected_future_reward - expected_future_cost
        
        # 2. Valeur d'arrêter = valeur actuelle (pas de coût additionnel)
        value_of_stopping = current_value
        
        # 3. Décision basée sur comparaison
        decision, reasoning = self._make_decision(
            value_of_continuing=value_of_continuing,
            value_of_stopping=value_of_stopping,
            current_cost=current_cost,
            expected_future_cost=expected_future_cost,
            convergence_rate=convergence_rate
        )
        
        # 4. Calculer confiance
        confidence = self._compute_confidence(
            trajectory_utility,
            convergence_rate,
            len(self.historical_rewards)
        )
        
        # Statistiques
        if decision != StoppingDecision.CONTINUE:
            self.stop_decisions += 1
        else:
            self.continue_decisions += 1
        
        # Apprentissage
        self.historical_rewards.append(expected_future_reward)
        self.historical_costs.append(expected_future_cost)
        if len(self.historical_rewards) > self.max_history:
            self.historical_rewards.pop(0)
            self.historical_costs.pop(0)
        
        evaluation = StoppingEvaluation(
            decision=decision,
            value_of_continuing=value_of_continuing,
            value_of_stopping=value_of_stopping,
            expected_cost=expected_future_cost,
            confidence=confidence,
            reasoning=reasoning
        )
        
        # Log forensic
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "stopping_evaluated",
            self.__class__.__name__,
            {
                "evaluation": evaluation.to_dict(),
                "inputs": {
                    "current_value": current_value,
                    "current_cost": current_cost,
                    "trajectory_utility": trajectory_utility,
                    "convergence_rate": convergence_rate
                },
                "execution_time_ns": end_ns - start_ns,
                "timestamp_ns": end_ns
            }
        )
        
        return evaluation
    
    def _estimate_future_reward(
        self,
        trajectory_utility: float,
        convergence_rate: float,
        steps_remaining: int
    ) -> float:
        """
        Estime reward futur si on continue
        
        Formule: reward = utility * convergence * steps * 10
        (facteur multiplicatif pour encourager exploration)
        """
        if convergence_rate <= 0:
            return 0.0
        
        # Facteur multiplicatif pour encourager exploration
        exploration_factor = 10.0
        
        expected_reward = trajectory_utility * convergence_rate * steps_remaining * exploration_factor
        
        return max(0.0, expected_reward)
    
    def _make_decision(
        self,
        value_of_continuing: float,
        value_of_stopping: float,
        current_cost: float,
        expected_future_cost: float,
        convergence_rate: float
    ) -> Tuple[StoppingDecision, str]:
        """
        Prend décision d'arrêt
        
        Returns:
            (decision, reasoning)
        """
        # Règle 1: Coût trop élevé
        if current_cost + expected_future_cost > self.max_cost_threshold:
            return (
                StoppingDecision.STOP_HIGH_COST,
                f"Total cost {current_cost + expected_future_cost:.1f} > threshold {self.max_cost_threshold}"
            )
        
        # Règle 2: Valeur continuer trop faible
        if value_of_continuing < self.min_value_threshold:
            return (
                StoppingDecision.STOP_LOW_VALUE,
                f"Value of continuing {value_of_continuing:.3f} < threshold {self.min_value_threshold}"
            )
        
        # Règle 3: Rendements décroissants
        if convergence_rate < self.diminishing_returns_threshold:
            return (
                StoppingDecision.STOP_DIMINISHING_RETURNS,
                f"Convergence rate {convergence_rate:.4f} < threshold {self.diminishing_returns_threshold}"
            )
        
        # Règle 4: Comparaison directe
        if value_of_stopping > value_of_continuing:
            return (
                StoppingDecision.STOP_LOW_VALUE,
                f"V(stop)={value_of_stopping:.3f} > V(continue)={value_of_continuing:.3f}"
            )
        
        # Continuer
        return (
            StoppingDecision.CONTINUE,
            f"V(continue)={value_of_continuing:.3f} > V(stop)={value_of_stopping:.3f}"
        )
    
    def _compute_confidence(
        self,
        trajectory_utility: float,
        convergence_rate: float,
        history_size: int
    ) -> float:
        """
        Calcule confiance dans l'estimation
        
        Confiance augmente avec:
        - Utilité trajectoire élevée
        - Convergence stable
        - Historique suffisant
        """
        utility_confidence = trajectory_utility
        convergence_confidence = min(1.0, convergence_rate * 10)  # Normaliser
        history_confidence = min(1.0, history_size / 20)  # 20 samples = confiance max
        
        # Moyenne pondérée
        confidence = (
            0.4 * utility_confidence +
            0.4 * convergence_confidence +
            0.2 * history_confidence
        )
        
        return confidence
    
    def update_thresholds(
        self,
        actual_reward: float,
        actual_cost: float
    ) -> None:
        """
        Met à jour seuils basés sur expérience réelle
        
        Args:
            actual_reward: Reward réellement obtenu
            actual_cost: Coût réellement payé
        """
        # Apprentissage simple: ajuster seuils si erreur systématique
        if len(self.historical_rewards) >= 10:
            avg_predicted_reward = np.mean(self.historical_rewards[-10:])
            avg_actual_reward = actual_reward
            
            # Si on sous-estime systématiquement, augmenter seuil
            if avg_actual_reward > avg_predicted_reward * 1.5:
                self.min_value_threshold *= 0.9  # Être plus optimiste
            
            # Si on sur-estime, réduire seuil
            elif avg_actual_reward < avg_predicted_reward * 0.5:
                self.min_value_threshold *= 1.1  # Être plus conservateur
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "thresholds_updated",
            self.__class__.__name__,
            {
                "actual_reward": actual_reward,
                "actual_cost": actual_cost,
                "new_min_value_threshold": self.min_value_threshold,
                "timestamp_ns": time.time_ns()
            }
        )
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques complètes"""
        return {
            'total_evaluations': self.total_evaluations,
            'stop_decisions': self.stop_decisions,
            'continue_decisions': self.continue_decisions,
            'stop_rate': (
                self.stop_decisions / self.total_evaluations
                if self.total_evaluations > 0 else 0.0
            ),
            'avg_predicted_reward': (
                float(np.mean(self.historical_rewards))
                if self.historical_rewards else 0.0
            ),
            'avg_predicted_cost': (
                float(np.mean(self.historical_costs))
                if self.historical_costs else 0.0
            ),
            'current_thresholds': {
                'min_value': self.min_value_threshold,
                'max_cost': self.max_cost_threshold,
                'diminishing_returns': self.diminishing_returns_threshold
            }
        }
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return {
            'statistics': self.get_statistics(),
            'forensic_state': self._forensic_state
        }
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            self.__class__.__name__,
            {
                "final_state": self._forensic_state,
                "final_statistics": self.get_statistics(),
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()


# Export
__all__ = ['ValueOfStoppingEstimator', 'StoppingEvaluation', 'StoppingDecision']

# Made with Bob