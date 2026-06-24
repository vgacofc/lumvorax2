"""
MAGEN V35 - GlobalRegretDetector
Détecteur de Regret Global

Mission: Détecter "on perd notre temps" au niveau système
Architecture: V34 + Signal de Regret Global

NOUVEAUTÉ V35: CAUSALITÉ ÉVALUATIVE
- Détecte gaspillage computationnel global
- Signal "nous sommes dans une mauvaise direction"
- Apprend patterns de trajectoires improductives
- Déclenche réinitialisation stratégique

CRITIQUE: Implémente conscience de l'inefficacité - "meta-meta-cognition"

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


class RegretLevel(Enum):
    """Niveau de regret"""
    NONE = "none"
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    CRITICAL = "critical"


@dataclass
class RegretSignal:
    """Signal de regret global"""
    level: RegretLevel
    regret_score: float  # 0.0 = pas de regret, 1.0 = regret maximum
    wasted_computation: float  # Proportion computation gaspillée
    reasons: List[str]
    recommended_action: str
    
    def to_dict(self) -> Dict:
        return {
            "level": self.level.value,
            "regret_score": self.regret_score,
            "wasted_computation": self.wasted_computation,
            "reasons": self.reasons,
            "recommended_action": self.recommended_action
        }


class GlobalRegretDetector:
    """
    Détecteur de regret global.
    
    Architecture V35:
        ┌─────────────────────────────────────┐
        │   GlobalRegretDetector              │
        │                                     │
        │  ┌──────────────────────────────┐  │
        │  │   Signaux Regret             │  │
        │  │   - Loops répétés            │  │
        │  │   - Stagnation prolongée     │  │
        │  │   - Coût sans progrès        │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │  ┌──────────────────────────────┐  │
        │  │   Agrégation                 │  │
        │  │   Regret Score = Σ(signaux)  │  │
        │  └──────────────────────────────┘  │
        │              ↓                      │
        │  ┌──────────────────────────────┐  │
        │  │   Décision Stratégique       │  │
        │  │   - Continue                 │  │
        │  │   - Reset partiel            │  │
        │  │   - Reset complet            │  │
        │  └──────────────────────────────┘  │
        └─────────────────────────────────────┘
    
    Principe:
        1. Collecter signaux inefficacité
        2. Agréger en score regret global
        3. Déclencher action corrective si nécessaire
        4. Apprendre patterns échec
    
    CRITIQUE: Première implémentation "global regret" dans MAGEN
    """
    
    def __init__(
        self,
        window_size: int = 100,
        loop_threshold: float = 0.7,
        stagnation_threshold: float = 0.8,
        waste_threshold: float = 0.6,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise le détecteur
        
        Args:
            window_size: Taille fenêtre analyse
            loop_threshold: Seuil détection boucles
            stagnation_threshold: Seuil stagnation
            waste_threshold: Seuil gaspillage
            forensic_log_path: Chemin log forensic (LEÇON-73.1)
        """
        self.window_size = window_size
        self.loop_threshold = loop_threshold
        self.stagnation_threshold = stagnation_threshold
        self.waste_threshold = waste_threshold
        
        # Historique observations
        self.loop_history: List[float] = []
        self.stagnation_history: List[float] = []
        self.utility_history: List[float] = []
        self.cost_history: List[float] = []
        
        # Statistiques
        self.total_detections: int = 0
        self.regret_signals_emitted: int = 0
        self.resets_triggered: int = 0
        
        # V35: Forensic standard LumVorax (LEÇON-73.1)
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v35/global_regret_detector.jsonl"
        
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
            'total_detections': self.total_detections,
            'regret_signals_emitted': self.regret_signals_emitted,
            'resets_triggered': self.resets_triggered,
            'regret_rate': (
                self.regret_signals_emitted / self.total_detections
                if self.total_detections > 0 else 0.0
            ),
            'window_size': self.window_size
        }
    
    def update(
        self,
        loop_score: float,
        stagnation_score: float,
        trajectory_utility: float,
        step_cost: float
    ) -> None:
        """
        Met à jour historique observations
        
        Args:
            loop_score: Score boucles (0-1, 1=boucle infinie)
            stagnation_score: Score stagnation (0-1, 1=stagnation totale)
            trajectory_utility: Utilité trajectoire (0-1)
            step_cost: Coût step actuel
        """
        self.loop_history.append(loop_score)
        self.stagnation_history.append(stagnation_score)
        self.utility_history.append(trajectory_utility)
        self.cost_history.append(step_cost)
        
        # Limiter taille historique
        if len(self.loop_history) > self.window_size:
            self.loop_history.pop(0)
            self.stagnation_history.pop(0)
            self.utility_history.pop(0)
            self.cost_history.pop(0)
    
    def detect_regret(self) -> RegretSignal:
        """
        Détecte regret global
        
        Returns:
            RegretSignal avec niveau et recommandations
        """
        start_ns = time.time_ns()
        self.total_detections += 1
        
        # Vérifier historique suffisant
        if len(self.loop_history) < 10:
            return RegretSignal(
                level=RegretLevel.NONE,
                regret_score=0.0,
                wasted_computation=0.0,
                reasons=["Insufficient history"],
                recommended_action="continue"
            )
        
        # Calculer signaux regret
        reasons = []
        regret_components = []
        
        # Signal 1: Boucles répétées
        avg_loop = float(np.mean(self.loop_history[-20:]))
        if avg_loop > self.loop_threshold:
            reasons.append(f"High loop score: {avg_loop:.2f}")
            regret_components.append(avg_loop)
        
        # Signal 2: Stagnation prolongée
        avg_stagnation = float(np.mean(self.stagnation_history[-20:]))
        if avg_stagnation > self.stagnation_threshold:
            reasons.append(f"Prolonged stagnation: {avg_stagnation:.2f}")
            regret_components.append(avg_stagnation)
        
        # Signal 3: Utilité faible
        avg_utility = float(np.mean(self.utility_history[-20:]))
        if avg_utility < 0.3:
            reasons.append(f"Low trajectory utility: {avg_utility:.2f}")
            regret_components.append(1.0 - avg_utility)
        
        # Signal 4: Coût élevé sans progrès
        total_cost = sum(self.cost_history[-20:])
        total_utility = sum(self.utility_history[-20:])
        if total_cost > 0 and total_utility / total_cost < 0.1:
            reasons.append(f"High cost/utility ratio: {total_cost/total_utility:.1f}")
            regret_components.append(0.8)
        
        # Calculer score regret global
        if regret_components:
            regret_score = float(np.mean(regret_components))
        else:
            regret_score = 0.0
        
        # Calculer gaspillage computation
        wasted_computation = self._compute_waste()
        
        # Déterminer niveau et action
        level, action = self._determine_level_and_action(
            regret_score,
            wasted_computation,
            len(reasons)
        )
        
        signal = RegretSignal(
            level=level,
            regret_score=regret_score,
            wasted_computation=wasted_computation,
            reasons=reasons if reasons else ["No regret detected"],
            recommended_action=action
        )
        
        # Statistiques
        if level != RegretLevel.NONE:
            self.regret_signals_emitted += 1
        
        if "reset" in action:
            self.resets_triggered += 1
        
        # Log forensic
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "regret_detected",
            self.__class__.__name__,
            {
                "signal": signal.to_dict(),
                "execution_time_ns": end_ns - start_ns,
                "timestamp_ns": end_ns
            }
        )
        
        return signal
    
    def _compute_waste(self) -> float:
        """
        Calcule proportion computation gaspillée
        
        Returns:
            Proportion 0-1
        """
        if len(self.utility_history) < 10:
            return 0.0
        
        # Computation gaspillée = steps avec utilité < seuil
        recent_utilities = self.utility_history[-20:]
        wasted_steps = sum(1 for u in recent_utilities if u < 0.2)
        
        waste_ratio = wasted_steps / len(recent_utilities)
        
        return waste_ratio
    
    def _determine_level_and_action(
        self,
        regret_score: float,
        wasted_computation: float,
        num_reasons: int
    ) -> Tuple[RegretLevel, str]:
        """
        Détermine niveau regret et action recommandée
        
        Returns:
            (level, action)
        """
        # Niveau CRITICAL
        if regret_score > 0.8 and wasted_computation > 0.7:
            return RegretLevel.CRITICAL, "reset_complete"
        
        # Niveau HIGH
        if regret_score > 0.6 or wasted_computation > 0.6:
            return RegretLevel.HIGH, "reset_partial"
        
        # Niveau MEDIUM
        if regret_score > 0.4 or num_reasons >= 3:
            return RegretLevel.MEDIUM, "change_strategy"
        
        # Niveau LOW
        if regret_score > 0.2 or num_reasons >= 2:
            return RegretLevel.LOW, "adjust_parameters"
        
        # Niveau NONE
        return RegretLevel.NONE, "continue"
    
    def should_reset(self) -> Tuple[bool, str]:
        """
        Décide si reset nécessaire
        
        Returns:
            (should_reset, reset_type)
        """
        signal = self.detect_regret()
        
        if signal.level in [RegretLevel.CRITICAL, RegretLevel.HIGH]:
            return True, signal.recommended_action
        
        return False, "no_reset"
    
    def reset(self) -> None:
        """Reset historique"""
        self.loop_history.clear()
        self.stagnation_history.clear()
        self.utility_history.clear()
        self.cost_history.clear()
        
        # Log forensic
        self.forensic.forensic_logger.log(
            "detector_reset",
            self.__class__.__name__,
            {
                "timestamp_ns": time.time_ns(),
                "reason": "manual_reset"
            }
        )
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques complètes"""
        return {
            'total_detections': self.total_detections,
            'regret_signals_emitted': self.regret_signals_emitted,
            'resets_triggered': self.resets_triggered,
            'regret_rate': (
                self.regret_signals_emitted / self.total_detections
                if self.total_detections > 0 else 0.0
            ),
            'current_metrics': {
                'avg_loop': float(np.mean(self.loop_history)) if self.loop_history else 0.0,
                'avg_stagnation': float(np.mean(self.stagnation_history)) if self.stagnation_history else 0.0,
                'avg_utility': float(np.mean(self.utility_history)) if self.utility_history else 0.0,
                'total_cost': sum(self.cost_history)
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
__all__ = ['GlobalRegretDetector', 'RegretSignal', 'RegretLevel']

# Made with Bob