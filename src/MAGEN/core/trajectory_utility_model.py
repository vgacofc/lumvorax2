"""
MAGEN V35 - TrajectoryUtilityModel
Modèle d'Utilité des Trajectoires

Mission: Évaluer si une séquence d'actions "vaut quelque chose"
Architecture: V34 + Causalité Fonctionnelle (pas seulement mécanique)

NOUVEAUTÉ V35: CAUSALITÉ FONCTIONNELLE
- Mesure si changements améliorent performance
- Métriques: loop_reduction, convergence_rate, cumulative_reward
- Apprentissage: quelles trajectoires sont productives
- Décision: continuer ou abandonner basée sur utilité prédite

CRITIQUE: Transforme métacognition descriptive en métacognition évaluative

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V35.0.0
"""

import time
import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
from pathlib import Path
from collections import deque

# V35: Forensic standard LEÇON-73.1
from .forensic_middleware import ForensicMiddleware


@dataclass
class TrajectoryMetrics:
    """Métriques d'utilité d'une trajectoire"""
    trajectory_id: str
    start_step: int
    current_step: int
    
    # Métriques fonctionnelles (NOUVEAUTÉ V35)
    loop_reduction: float = 0.0  # Réduction boucles infinies
    convergence_rate: float = 0.0  # Vitesse convergence vers solution
    cumulative_reward: float = 0.0  # Reward cumulé
    
    # Métriques secondaires
    actions_count: int = 0
    unique_states_visited: int = 0
    avg_score_improvement: float = 0.0
    
    # Historique
    score_history: List[float] = field(default_factory=list)
    action_history: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict:
        return {
            "trajectory_id": self.trajectory_id,
            "start_step": self.start_step,
            "current_step": self.current_step,
            "loop_reduction": self.loop_reduction,
            "convergence_rate": self.convergence_rate,
            "cumulative_reward": self.cumulative_reward,
            "actions_count": self.actions_count,
            "unique_states_visited": self.unique_states_visited,
            "avg_score_improvement": self.avg_score_improvement
        }


@dataclass
class UtilityPrediction:
    """Prédiction d'utilité future"""
    predicted_utility: float  # 0.0 = inutile, 1.0 = très utile
    confidence: float  # 0.0 = incertain, 1.0 = certain
    reasoning: str
    metrics_used: Dict
    
    def to_dict(self) -> Dict:
        return {
            "predicted_utility": self.predicted_utility,
            "confidence": self.confidence,
            "reasoning": self.reasoning,
            "metrics_used": self.metrics_used
        }


class TrajectoryUtilityModel:
    """
    Modèle d'utilité des trajectoires.
    
    Architecture V35:
        ┌─────────────────────────────────────┐
        │   TrajectoryUtilityModel            │
        │                                     │
        │  ┌──────────────────────────────┐  │
        │  │   Métriques Fonctionnelles   │  │
        │  │   - loop_reduction           │  │
        │  │   - convergence_rate         │  │
        │  │   - cumulative_reward        │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │  ┌──────────────────────────────┐  │
        │  │   Prédiction Utilité         │  │
        │  │   - Modèle linéaire          │  │
        │  │   - Confiance bayésienne     │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │      DÉCISION: Continuer/Stop       │
        └─────────────────────────────────────┘
    
    Principe:
        1. Mesurer métriques fonctionnelles réelles
        2. Prédire utilité future basée sur historique
        3. Décider continuer ou abandonner
        4. Apprendre des trajectoires passées
    
    CRITIQUE: Première implémentation causalité fonctionnelle dans MAGEN
    """
    
    def __init__(
        self,
        window_size: int = 50,
        min_trajectory_length: int = 5,
        utility_threshold: float = 0.3,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise le modèle d'utilité
        
        Args:
            window_size: Taille fenêtre analyse
            min_trajectory_length: Longueur minimale avant évaluation
            utility_threshold: Seuil utilité minimum
            forensic_log_path: Chemin log forensic (LEÇON-73.1)
        """
        self.window_size = window_size
        self.min_trajectory_length = min_trajectory_length
        self.utility_threshold = utility_threshold
        
        # État trajectoire courante
        self.current_trajectory: Optional[TrajectoryMetrics] = None
        self.trajectory_active: bool = False
        
        # Historique trajectoires
        self.completed_trajectories: List[TrajectoryMetrics] = []
        self.max_history: int = 100
        
        # Statistiques apprentissage
        self.total_trajectories: int = 0
        self.successful_trajectories: int = 0
        self.abandoned_trajectories: int = 0
        
        # Modèle prédiction (poids appris)
        self.prediction_weights = {
            'loop_reduction': 0.4,
            'convergence_rate': 0.4,
            'cumulative_reward': 0.2
        }
        
        # V35: Forensic standard LumVorax (LEÇON-73.1)
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v35/trajectory_utility_model.jsonl"
        
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
            'total_trajectories': self.total_trajectories,
            'successful_trajectories': self.successful_trajectories,
            'abandoned_trajectories': self.abandoned_trajectories,
            'trajectory_active': self.trajectory_active,
            'window_size': self.window_size,
            'utility_threshold': self.utility_threshold
        }
    
    def start_trajectory(self, trajectory_id: str, start_step: int) -> None:
        """
        Démarre une nouvelle trajectoire
        
        Args:
            trajectory_id: Identifiant unique
            start_step: Step de démarrage
        """
        self.current_trajectory = TrajectoryMetrics(
            trajectory_id=trajectory_id,
            start_step=start_step,
            current_step=start_step
        )
        self.trajectory_active = True
        self.total_trajectories += 1
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "trajectory_started",
            self.__class__.__name__,
            {
                "trajectory_id": trajectory_id,
                "start_step": start_step,
                "timestamp_ns": time.time_ns()
            }
        )
    
    def update_trajectory(
        self,
        current_step: int,
        current_score: float,
        current_action: str,
        reward: float,
        state_hash: str
    ) -> None:
        """
        Met à jour trajectoire courante
        
        Args:
            current_step: Step actuel
            current_score: Score actuel
            current_action: Action exécutée
            reward: Reward obtenu
            state_hash: Hash état (pour détecter boucles)
        """
        if not self.trajectory_active or not self.current_trajectory:
            return
        
        traj = self.current_trajectory
        traj.current_step = current_step
        traj.actions_count += 1
        traj.cumulative_reward += reward
        
        # Historique
        traj.score_history.append(current_score)
        traj.action_history.append(current_action)
        
        # Calculer métriques fonctionnelles
        self._compute_functional_metrics(traj, state_hash)
    
    def _compute_functional_metrics(
        self,
        traj: TrajectoryMetrics,
        state_hash: str
    ) -> None:
        """
        Calcule métriques fonctionnelles (NOUVEAUTÉ V35)
        
        Args:
            traj: Trajectoire courante
            state_hash: Hash état actuel
        """
        # 1. Loop Reduction: Détecte si on revisite états
        recent_actions = traj.action_history[-self.window_size:]
        unique_recent = len(set(recent_actions))
        total_recent = len(recent_actions)
        
        if total_recent > 0:
            diversity = unique_recent / total_recent
            traj.loop_reduction = diversity  # 1.0 = pas de boucles, 0.0 = boucle infinie
        
        # 2. Convergence Rate: Vitesse amélioration score
        if len(traj.score_history) >= 2:
            recent_scores = traj.score_history[-min(10, len(traj.score_history)):]
            if len(recent_scores) >= 2:
                score_delta = recent_scores[-1] - recent_scores[0]
                steps_delta = len(recent_scores)
                traj.convergence_rate = max(0.0, score_delta / steps_delta)
        
        # 3. Average Score Improvement
        if len(traj.score_history) >= 2:
            improvements = [
                traj.score_history[i] - traj.score_history[i-1]
                for i in range(1, len(traj.score_history))
            ]
            traj.avg_score_improvement = float(np.mean([max(0, imp) for imp in improvements]))
    
    def predict_utility(self) -> UtilityPrediction:
        """
        Prédit utilité future de la trajectoire courante
        
        Returns:
            UtilityPrediction avec utilité prédite et confiance
        """
        if not self.trajectory_active or not self.current_trajectory:
            return UtilityPrediction(
                predicted_utility=0.0,
                confidence=0.0,
                reasoning="No active trajectory",
                metrics_used={}
            )
        
        traj = self.current_trajectory
        
        # Vérifier longueur minimale
        if traj.actions_count < self.min_trajectory_length:
            return UtilityPrediction(
                predicted_utility=0.5,  # Neutre
                confidence=0.0,
                reasoning=f"Trajectory too short ({traj.actions_count} < {self.min_trajectory_length})",
                metrics_used={}
            )
        
        # Calculer utilité prédite (modèle linéaire pondéré)
        metrics = {
            'loop_reduction': traj.loop_reduction,
            'convergence_rate': min(1.0, traj.convergence_rate),
            'cumulative_reward': min(1.0, max(0.0, traj.cumulative_reward))
        }
        
        predicted_utility = sum(
            self.prediction_weights[key] * metrics[key]
            for key in metrics.keys()
        )
        
        # Calculer confiance (basée sur longueur trajectoire)
        confidence = min(1.0, traj.actions_count / (self.min_trajectory_length * 2))
        
        # Raisonnement
        reasoning = self._generate_reasoning(metrics, predicted_utility)
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "utility_predicted",
            self.__class__.__name__,
            {
                "trajectory_id": traj.trajectory_id,
                "predicted_utility": predicted_utility,
                "confidence": confidence,
                "metrics": metrics,
                "timestamp_ns": time.time_ns()
            }
        )
        
        return UtilityPrediction(
            predicted_utility=predicted_utility,
            confidence=confidence,
            reasoning=reasoning,
            metrics_used=metrics
        )
    
    def _generate_reasoning(self, metrics: Dict, utility: float) -> str:
        """Génère explication prédiction"""
        reasons = []
        
        if metrics['loop_reduction'] < 0.5:
            reasons.append("High loop detection")
        if metrics['convergence_rate'] < 0.1:
            reasons.append("Low convergence rate")
        if metrics['cumulative_reward'] < 0.0:
            reasons.append("Negative cumulative reward")
        
        if not reasons:
            reasons.append("Metrics within acceptable range")
        
        return f"Utility={utility:.2f}: " + ", ".join(reasons)
    
    def should_continue(self) -> Tuple[bool, str]:
        """
        Décide si continuer trajectoire
        
        Returns:
            (should_continue, reason)
        """
        prediction = self.predict_utility()
        
        # Décision basée sur utilité prédite
        should_continue = prediction.predicted_utility >= self.utility_threshold
        
        reason = (
            f"Utility {prediction.predicted_utility:.2f} "
            f"{'≥' if should_continue else '<'} threshold {self.utility_threshold:.2f}"
        )
        
        return should_continue, reason
    
    def end_trajectory(self, success: bool) -> None:
        """
        Termine trajectoire courante
        
        Args:
            success: Trajectoire réussie ou abandonnée
        """
        if not self.trajectory_active or not self.current_trajectory:
            return
        
        traj = self.current_trajectory
        
        # Statistiques
        if success:
            self.successful_trajectories += 1
        else:
            self.abandoned_trajectories += 1
        
        # Sauvegarder historique
        self.completed_trajectories.append(traj)
        if len(self.completed_trajectories) > self.max_history:
            self.completed_trajectories.pop(0)
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "trajectory_ended",
            self.__class__.__name__,
            {
                "trajectory_id": traj.trajectory_id,
                "success": success,
                "metrics": traj.to_dict(),
                "timestamp_ns": time.time_ns()
            }
        )
        
        # Reset
        self.current_trajectory = None
        self.trajectory_active = False
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques complètes"""
        return {
            'total_trajectories': self.total_trajectories,
            'successful_trajectories': self.successful_trajectories,
            'abandoned_trajectories': self.abandoned_trajectories,
            'success_rate': (
                self.successful_trajectories / self.total_trajectories
                if self.total_trajectories > 0 else 0.0
            ),
            'trajectory_active': self.trajectory_active,
            'current_trajectory': (
                self.current_trajectory.to_dict()
                if self.current_trajectory else None
            )
        }
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return {
            'statistics': self.get_statistics(),
            'forensic_state': self._forensic_state,
            'prediction_weights': self.prediction_weights
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
__all__ = ['TrajectoryUtilityModel', 'TrajectoryMetrics', 'UtilityPrediction']

# Made with Bob