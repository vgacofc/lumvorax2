"""
MAGEN V32 - Module C4: CognitiveStabilizer
Spatial Cognitive System - Stabilisation Cognitive

Mission: Empêcher la fragmentation cognitive et maintenir la cohérence stratégique
Règle critique: Policy breaks < 10% des steps
"""

import time
from typing import Dict, List, Optional
from dataclasses import dataclass
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class StrategyState:
    """État d'une stratégie."""
    strategy_id: str
    start_step: int
    current_step: int
    duration: int = 0
    success_indicators: Optional[List[float]] = None
    failure_indicators: Optional[List[float]] = None
    confidence: float = 0.5
    
    def __post_init__(self):
        if self.success_indicators is None:
            self.success_indicators = []
        if self.failure_indicators is None:
            self.failure_indicators = []
        self.duration = self.current_step - self.start_step


class CognitiveStabilizer:
    """
    Stabilisateur Cognitif.
    
    Empêche les changements de stratégie trop fréquents et maintient
    la cohérence cognitive du système.
    
    Contrainte absolue: Policy breaks < 10% des steps
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Configuration
        self.max_break_frequency = 0.10  # 10% maximum
        self.min_strategy_duration = 10  # Minimum 10 steps par stratégie
        self.break_cooldown = 5  # Minimum 5 steps entre breaks
        
        # État actuel
        self.current_strategy: Optional[StrategyState] = None
        self.last_break_step: int = -100
        self.total_steps: int = 0
        self.total_breaks: int = 0
        
        # Historique
        self.strategy_history: List[StrategyState] = []
        self.break_history: List[Dict] = []
        
        # Inertie cognitive
        self.cognitive_inertia: float = 0.7  # Résistance au changement
        self.confidence_threshold: float = 0.8  # Seuil pour changement
        
        # Statistiques
        self.strategies_maintained: int = 0
        self.strategies_changed: int = 0
        self.breaks_prevented: int = 0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/cognitive_stabilizer.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "CognitiveStabilizer")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "CognitiveStabilizer",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "max_break_frequency": self.max_break_frequency,
                "min_strategy_duration": self.min_strategy_duration,
                "break_cooldown": self.break_cooldown,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "cognitive_stabilizer.initialized",
                "cognitive_stabilizer",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0",
                    "max_break_frequency": self.max_break_frequency,
                    "min_strategy_duration": self.min_strategy_duration,
                    "break_cooldown": self.break_cooldown
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_steps': self.total_steps,
            'total_breaks': self.total_breaks,
            'break_frequency': self.total_breaks / max(1, self.total_steps),
            'breaks_prevented': self.breaks_prevented,
            'strategies_maintained': self.strategies_maintained,
            'strategies_changed': self.strategies_changed,
            'current_strategy_id': self.current_strategy.strategy_id if self.current_strategy else None
        }
    
    def should_change_strategy(self,
                              current_evidence: Dict,
                              step: int) -> bool:
        """
        Décider si un changement de stratégie est justifié.
        
        Args:
            current_evidence: Preuves pour/contre changement
            step: Step actuel
            
        Returns:
            True si changement autorisé, False sinon
        """
        start_ns = time.time_ns()
        self.total_steps = step
        
        # Log début décision
        self.forensic.forensic_logger.log(
            "decision_start",
            "CognitiveStabilizer",
            {
                "step": step,
                "evidence": current_evidence,
                "state": self._forensic_state
            }
        )
        
        # 1. Vérifier contrainte de fréquence globale
        if not self._check_frequency_constraint():
            self.breaks_prevented += 1
            if self.logger:
                self.logger.log_event(
                    "cognitive_stabilizer.break_prevented",
                    "cognitive_stabilizer",
                    {
                        "step": step,
                        "reason": "frequency_constraint",
                        "current_frequency": self.total_breaks / max(1, self.total_steps)
                    }
                )
            return False
        
        # 2. Vérifier cooldown
        if not self._check_cooldown(step):
            self.breaks_prevented += 1
            if self.logger:
                self.logger.log_event(
                    "cognitive_stabilizer.break_prevented",
                    "cognitive_stabilizer",
                    {
                        "step": step,
                        "reason": "cooldown",
                        "steps_since_last": step - self.last_break_step
                    }
                )
            return False
        
        # 3. Vérifier durée minimale de stratégie
        if self.current_strategy and not self._check_min_duration(step):
            self.breaks_prevented += 1
            if self.logger:
                self.logger.log_event(
                    "cognitive_stabilizer.break_prevented",
                    "cognitive_stabilizer",
                    {
                        "step": step,
                        "reason": "min_duration",
                        "current_duration": step - self.current_strategy.start_step
                    }
                )
            return False
        
        # 4. Évaluer la force des preuves
        evidence_strength = self._evaluate_evidence_strength(current_evidence)
        
        # 5. Appliquer inertie cognitive
        change_threshold = self.confidence_threshold * self.cognitive_inertia
        
        if evidence_strength < change_threshold:
            self.breaks_prevented += 1
            if self.logger:
                self.logger.log_event(
                    "cognitive_stabilizer.break_prevented",
                    "cognitive_stabilizer",
                    {
                        "step": step,
                        "reason": "insufficient_evidence",
                        "evidence_strength": evidence_strength,
                        "threshold": change_threshold
                    }
                )
            return False
        
        # 6. Changement autorisé
        self._record_strategy_change(step, evidence_strength, current_evidence)
        
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "decision_complete",
            "CognitiveStabilizer",
            {
                "step": step,
                "decision": "change_authorized",
                "evidence_strength": evidence_strength,
                "execution_time_ns": end_ns - start_ns,
                "state": self._forensic_state
            }
        )
        
        return True
    
    def _check_frequency_constraint(self) -> bool:
        """Vérifier la contrainte de fréquence globale."""
        if self.total_steps == 0:
            return True
        
        current_frequency = self.total_breaks / self.total_steps
        return current_frequency < self.max_break_frequency
    
    def _check_cooldown(self, step: int) -> bool:
        """Vérifier le cooldown entre breaks."""
        steps_since_last = step - self.last_break_step
        return steps_since_last >= self.break_cooldown
    
    def _check_min_duration(self, step: int) -> bool:
        """Vérifier la durée minimale de la stratégie actuelle."""
        if not self.current_strategy:
            return True
        
        duration = step - self.current_strategy.start_step
        return duration >= self.min_strategy_duration
    
    def _evaluate_evidence_strength(self, evidence: Dict) -> float:
        """Évaluer la force des preuves pour un changement."""
        # Preuves attendues:
        # - collapse_detected: bool
        # - stagnation_detected: bool
        # - sterility_pressure: float (0-1)
        # - viability: float (0-1)
        # - contradiction_strength: float (0-1)
        
        strength = 0.0
        weight_sum = 0.0
        
        # Collapse (poids: 0.3)
        if evidence.get('collapse_detected', False):
            strength += 0.3 * 1.0
        weight_sum += 0.3
        
        # Stagnation (poids: 0.25)
        if evidence.get('stagnation_detected', False):
            strength += 0.25 * 1.0
        weight_sum += 0.25
        
        # Sterility pressure (poids: 0.2)
        sterility = evidence.get('sterility_pressure', 0.0)
        if sterility > 0.85:  # Seuil plus tolérant que V31 (0.7)
            strength += 0.2 * min(1.0, (sterility - 0.85) / 0.15)
        weight_sum += 0.2
        
        # Viability (poids: 0.15)
        viability = evidence.get('viability', 0.5)
        if viability < 0.1:  # Seuil plus tolérant que V31 (0.2)
            strength += 0.15 * (1.0 - viability / 0.1)
        weight_sum += 0.15
        
        # Contradiction (poids: 0.1)
        contradiction = evidence.get('contradiction_strength', 0.0)
        if contradiction > 0.8:
            strength += 0.1 * contradiction
        weight_sum += 0.1
        
        # Normaliser
        if weight_sum > 0:
            strength = strength / weight_sum
        
        return strength
    
    def _record_strategy_change(self, 
                                step: int,
                                evidence_strength: float,
                                evidence: Dict) -> None:
        """Enregistrer un changement de stratégie."""
        # Finaliser stratégie actuelle
        if self.current_strategy:
            self.current_strategy.current_step = step
            self.current_strategy.duration = step - self.current_strategy.start_step
            self.strategy_history.append(self.current_strategy)
            self.strategies_maintained += 1
        
        # Créer nouvelle stratégie
        new_strategy_id = f"strategy_{len(self.strategy_history)}"
        self.current_strategy = StrategyState(
            strategy_id=new_strategy_id,
            start_step=step,
            current_step=step
        )
        
        # Enregistrer le break
        self.total_breaks += 1
        self.last_break_step = step
        self.strategies_changed += 1
        
        break_record = {
            'step': step,
            'evidence_strength': evidence_strength,
            'evidence': evidence,
            'new_strategy_id': new_strategy_id
        }
        self.break_history.append(break_record)
        
        # Forensic logging
        if self.logger:
            self.logger.log_event(
                "cognitive_stabilizer.strategy_changed",
                "cognitive_stabilizer",
                {
                    'step': step,
                    'new_strategy_id': new_strategy_id,
                    'evidence_strength': evidence_strength,
                    'total_breaks': self.total_breaks,
                    'break_frequency': self.total_breaks / max(1, self.total_steps)
                }
            )
    
    def update_strategy_indicators(self,
                                   success_indicator: Optional[float] = None,
                                   failure_indicator: Optional[float] = None) -> None:
        """Mettre à jour les indicateurs de la stratégie actuelle."""
        if not self.current_strategy:
            return
        
        if success_indicator is not None and self.current_strategy.success_indicators is not None:
            self.current_strategy.success_indicators.append(success_indicator)
        
        if failure_indicator is not None and self.current_strategy.failure_indicators is not None:
            self.current_strategy.failure_indicators.append(failure_indicator)
        
        # Recalculer confiance
        self._update_strategy_confidence()
    
    def _update_strategy_confidence(self) -> None:
        """Mettre à jour la confiance dans la stratégie actuelle."""
        if not self.current_strategy:
            return
        
        success = self.current_strategy.success_indicators or []
        failure = self.current_strategy.failure_indicators or []
        
        if not success and not failure:
            self.current_strategy.confidence = 0.5
            return
        
        # Calculer ratio succès/échec
        total = len(success) + len(failure)
        if total == 0:
            self.current_strategy.confidence = 0.5
            return
        
        success_rate = len(success) / total
        
        # Pondérer par la durée (plus longue = plus de confiance)
        duration_factor = min(1.0, self.current_strategy.duration / 20.0)
        
        confidence = 0.7 * success_rate + 0.3 * duration_factor
        self.current_strategy.confidence = confidence
    
    def get_current_strategy_state(self) -> Optional[Dict]:
        """Obtenir l'état de la stratégie actuelle."""
        if not self.current_strategy:
            return None
        
        success_indicators = self.current_strategy.success_indicators or []
        failure_indicators = self.current_strategy.failure_indicators or []
        
        return {
            'strategy_id': self.current_strategy.strategy_id,
            'duration': self.current_strategy.duration,
            'confidence': self.current_strategy.confidence,
            'success_count': len(success_indicators),
            'failure_count': len(failure_indicators)
        }
    
    def get_stability_metrics(self) -> Dict:
        """Obtenir les métriques de stabilité cognitive."""
        if self.total_steps == 0:
            break_frequency = 0.0
            avg_strategy_duration = 0.0
        else:
            break_frequency = self.total_breaks / self.total_steps
            if self.strategy_history:
                avg_strategy_duration = sum(s.duration for s in self.strategy_history) / len(self.strategy_history)
            else:
                avg_strategy_duration = 0.0
        
        return {
            'total_steps': self.total_steps,
            'total_breaks': self.total_breaks,
            'break_frequency': break_frequency,
            'breaks_prevented': self.breaks_prevented,
            'avg_strategy_duration': avg_strategy_duration,
            'strategies_maintained': self.strategies_maintained,
            'strategies_changed': self.strategies_changed,
            'current_strategy': self.get_current_strategy_state(),
            'cognitive_inertia': self.cognitive_inertia,
            'within_constraint': break_frequency < self.max_break_frequency
        }
    
    def adjust_inertia(self, new_inertia: float) -> None:
        """Ajuster l'inertie cognitive."""
        self.cognitive_inertia = max(0.0, min(1.0, new_inertia))
        
        if self.logger:
            self.logger.log_event(
                "cognitive_stabilizer.inertia_adjusted",
                "cognitive_stabilizer",
                {
                    'new_inertia': self.cognitive_inertia
                }
            )
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter état du stabilisateur en JSON."""
        data = {
            "metrics": self.get_stability_metrics(),
            "strategy_history": [
                {
                    'strategy_id': s.strategy_id,
                    'start_step': s.start_step,
                    'duration': s.duration,
                    'confidence': s.confidence
                }
                for s in self.strategy_history
            ],
            "break_history": self.break_history[-50:]  # 50 derniers
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_stability_metrics()
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "CognitiveStabilizer",
            {
                "final_state": self._forensic_state,
                "total_steps": self.total_steps,
                "total_breaks": self.total_breaks,
                "break_frequency": self.total_breaks / max(1, self.total_steps),
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()


# Made with Bob
