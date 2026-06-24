"""
MAGEN V34 - NonProgressDetector
Détecteur de Non-Progrès Global

Mission: Détecter quand MAGEN stagne globalement malgré l'activité locale
Architecture: Composant du MetaCognitiveController

NOUVEAUTÉ V34: Détection de stagnation avec prédiction de valeur future

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V34.0.0
"""

import time
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from collections import deque
import numpy as np


@dataclass
class ProgressWindow:
    """Fenêtre d'observation du progrès"""
    start_step: int
    end_step: int
    start_score: float
    end_score: float
    actions_count: int
    unique_actions: int
    avg_health: float
    
    @property
    def delta_score(self) -> float:
        return self.end_score - self.start_score
    
    @property
    def duration(self) -> int:
        return self.end_step - self.start_step
    
    @property
    def action_diversity(self) -> float:
        if self.actions_count == 0:
            return 0.0
        return self.unique_actions / self.actions_count


@dataclass
class StagnationEvent:
    """Événement de stagnation détecté"""
    detection_step: int
    window_size: int
    score_delta: float
    action_diversity: float
    avg_health: float
    severity: float  # 0.0 à 1.0
    reason: str
    
    def to_dict(self) -> Dict:
        return {
            "detection_step": self.detection_step,
            "window_size": self.window_size,
            "score_delta": self.score_delta,
            "action_diversity": self.action_diversity,
            "avg_health": self.avg_health,
            "severity": self.severity,
            "reason": self.reason
        }


class NonProgressDetector:
    """
    Détecteur de non-progrès global.
    
    Principe:
        Surveille le progrès sur des fenêtres temporelles.
        Détecte quand score stagne malgré activité.
        Prédit valeur future pour éviter faux positifs.
    
    Critères de stagnation:
        1. Score gelé sur N steps
        2. Diversité d'actions faible
        3. Santé décroissante
        4. Valeur future prédite faible
    
    CRITIQUE: Distingue stagnation réelle de plateau temporaire utile
    """
    
    def __init__(
        self,
        forensic_logger=None,
        window_size: int = 50,
        stagnation_threshold: float = 0.01,
        diversity_threshold: float = 0.3,
        health_threshold: float = 0.4
    ):
        """
        Initialise le détecteur
        
        Args:
            forensic_logger: Logger forensic
            window_size: Taille fenêtre observation
            stagnation_threshold: Seuil delta score pour stagnation
            diversity_threshold: Seuil diversité actions
            health_threshold: Seuil santé minimum
        """
        self.forensic_logger = forensic_logger
        
        # Configuration
        self.window_size = window_size
        self.stagnation_threshold = stagnation_threshold
        self.diversity_threshold = diversity_threshold
        self.health_threshold = health_threshold
        
        # Historique
        self.score_history: deque = deque(maxlen=window_size * 2)
        self.action_history: deque = deque(maxlen=window_size * 2)
        self.health_history: deque = deque(maxlen=window_size * 2)
        self.step_history: deque = deque(maxlen=window_size * 2)
        
        # Fenêtres d'observation
        self.windows: List[ProgressWindow] = []
        
        # Événements de stagnation
        self.stagnation_events: List[StagnationEvent] = []
        
        # Statistiques
        self.total_checks: int = 0
        self.stagnation_count: int = 0
        
        # Forensic
        self.creation_time = time.time_ns()
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_initialized",
                "NonProgressDetector",
                {
                    "timestamp_ns": self.creation_time,
                    "version": "v34.0.0",
                    "window_size": self.window_size,
                    "stagnation_threshold": self.stagnation_threshold,
                    "diversity_threshold": self.diversity_threshold,
                    "health_threshold": self.health_threshold
                }
            )
    
    def update(
        self,
        current_step: int,
        current_score: float,
        current_action: str,
        current_health: float
    ) -> None:
        """
        Met à jour l'historique
        
        Args:
            current_step: Step actuel
            current_score: Score actuel
            current_action: Action exécutée
            current_health: Santé actuelle
        """
        self.step_history.append(current_step)
        self.score_history.append(current_score)
        self.action_history.append(current_action)
        self.health_history.append(current_health)
    
    def check_stagnation(self, current_step: int) -> Optional[StagnationEvent]:
        """
        Vérifie si stagnation détectée
        
        Args:
            current_step: Step actuel
            
        Returns:
            StagnationEvent si stagnation, None sinon
        """
        self.total_checks += 1
        
        # Besoin d'assez d'historique
        if len(self.score_history) < self.window_size:
            return None
        
        # Créer fenêtre d'observation
        window = self._create_window(current_step)
        self.windows.append(window)
        
        # Vérifier critères de stagnation
        stagnation = self._detect_stagnation(window, current_step)
        
        if stagnation:
            self.stagnation_count += 1
            self.stagnation_events.append(stagnation)
            
            # Log forensic
            if self.forensic_logger:
                self.forensic_logger.log(
                    "stagnation_detected",
                    "NonProgressDetector",
                    {
                        "step": current_step,
                        "event": stagnation.to_dict(),
                        "window": {
                            "start_step": window.start_step,
                            "end_step": window.end_step,
                            "delta_score": window.delta_score,
                            "action_diversity": window.action_diversity
                        }
                    }
                )
        
        return stagnation
    
    def _create_window(self, current_step: int) -> ProgressWindow:
        """Crée fenêtre d'observation"""
        # Prendre derniers N éléments
        recent_steps = list(self.step_history)[-self.window_size:]
        recent_scores = list(self.score_history)[-self.window_size:]
        recent_actions = list(self.action_history)[-self.window_size:]
        recent_health = list(self.health_history)[-self.window_size:]
        
        return ProgressWindow(
            start_step=recent_steps[0],
            end_step=recent_steps[-1],
            start_score=recent_scores[0],
            end_score=recent_scores[-1],
            actions_count=len(recent_actions),
            unique_actions=len(set(recent_actions)),
            avg_health=float(np.mean(recent_health))
        )
    
    def _detect_stagnation(
        self,
        window: ProgressWindow,
        current_step: int
    ) -> Optional[StagnationEvent]:
        """
        Détecte stagnation dans fenêtre
        
        Returns:
            StagnationEvent si stagnation, None sinon
        """
        reasons = []
        severity_factors = []
        
        # Critère 1: Score gelé
        if abs(window.delta_score) < self.stagnation_threshold:
            reasons.append("score_frozen")
            severity_factors.append(0.5)
        
        # Critère 2: Diversité faible
        if window.action_diversity < self.diversity_threshold:
            reasons.append("low_diversity")
            severity_factors.append(0.4)
        
        # Critère 3: Santé décroissante
        if window.avg_health < self.health_threshold:
            reasons.append("low_health")
            severity_factors.append(0.3)
        
        # Critère 4: Valeur future prédite faible
        future_value = self._predict_future_value(window)
        if future_value < 0.1:
            reasons.append("low_future_value")
            severity_factors.append(0.3)
        
        # Stagnation si au moins 2 critères
        if len(reasons) >= 2:
            severity = sum(severity_factors) / len(severity_factors)
            
            return StagnationEvent(
                detection_step=current_step,
                window_size=window.duration,
                score_delta=window.delta_score,
                action_diversity=window.action_diversity,
                avg_health=window.avg_health,
                severity=severity,
                reason=" + ".join(reasons)
            )
        
        return None
    
    def _predict_future_value(self, window: ProgressWindow) -> float:
        """
        Prédit valeur future basée sur tendance
        
        CRITIQUE: Évite faux positifs sur plateaux temporaires utiles
        
        Args:
            window: Fenêtre d'observation
            
        Returns:
            Valeur prédite entre 0.0 et 1.0
        """
        # Si score augmente, valeur future positive
        if window.delta_score > 0:
            return min(1.0, 0.5 + window.delta_score)
        
        # Si diversité élevée, exploration peut être utile
        if window.action_diversity > 0.5:
            return 0.4
        
        # Si santé élevée, capacité de récupération
        if window.avg_health > 0.6:
            return 0.3
        
        # Sinon, valeur future faible
        return 0.1
    
    def is_stagnating(self) -> bool:
        """
        Vérifie si actuellement en stagnation
        
        Returns:
            True si stagnation récente, False sinon
        """
        if not self.stagnation_events:
            return False
        
        # Vérifier si dernier événement récent
        last_event = self.stagnation_events[-1]
        
        if len(self.step_history) == 0:
            return False
        
        current_step = self.step_history[-1]
        steps_since_event = current_step - last_event.detection_step
        
        # Stagnation si événement dans derniers 20 steps
        return steps_since_event < 20
    
    def get_stagnation_severity(self) -> float:
        """
        Retourne sévérité de stagnation actuelle
        
        Returns:
            Sévérité entre 0.0 et 1.0
        """
        if not self.is_stagnating():
            return 0.0
        
        return self.stagnation_events[-1].severity
    
    def get_recent_events(self, n: int = 5) -> List[StagnationEvent]:
        """Retourne N derniers événements"""
        return self.stagnation_events[-n:]
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques"""
        return {
            "total_checks": self.total_checks,
            "stagnation_count": self.stagnation_count,
            "stagnation_rate": (
                self.stagnation_count / self.total_checks
                if self.total_checks > 0 else 0.0
            ),
            "is_stagnating": self.is_stagnating(),
            "current_severity": self.get_stagnation_severity(),
            "window_size": self.window_size,
            "history_length": len(self.score_history)
        }
    
    def get_state(self) -> Dict:
        """Retourne état complet pour forensic"""
        return {
            "statistics": self.get_statistics(),
            "recent_events": [
                event.to_dict()
                for event in self.get_recent_events(3)
            ],
            "is_stagnating": self.is_stagnating()
        }
    
    def reset(self) -> None:
        """Reset complet"""
        self.score_history.clear()
        self.action_history.clear()
        self.health_history.clear()
        self.step_history.clear()
        self.windows.clear()
        self.stagnation_events.clear()
        self.total_checks = 0
        self.stagnation_count = 0
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "detector_reset",
                "NonProgressDetector",
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
                "NonProgressDetector",
                {
                    "timestamp_ns": time.time_ns(),
                    "final_statistics": self.get_statistics(),
                    "final_state": self.get_state()
                }
            )


# Export
__all__ = ['NonProgressDetector', 'StagnationEvent', 'ProgressWindow']

# Made with Bob
