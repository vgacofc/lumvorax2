"""
MAGEN V32 - Module C8: TrajectoryMeaningSystem
Spatial Cognitive System - Système de Signification des Trajectoires

Mission: Donner sens sémantique aux mouvements et trajectoires
Transformer: Séquences d'actions → Intentions → Comportements → Stratégies
"""

import time
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass
from collections import deque
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class TrajectorySegment:
    """Segment de trajectoire avec signification."""
    segment_id: str
    positions: List[Tuple[int, int]]
    actions: List[str]
    start_step: int
    end_step: int
    semantic_label: str  # "exploration", "goal_pursuit", "escape", "patrol", etc.
    intention: str  # "discover", "reach", "avoid", "optimize"
    confidence: float  # 0-1
    success: bool
    reward_delta: float


@dataclass
class BehavioralPattern:
    """Pattern comportemental identifié."""
    pattern_id: str
    pattern_type: str  # "systematic_search", "greedy_approach", "cautious_exploration"
    frequency: int
    success_rate: float
    contexts: List[str]  # Contextes où le pattern apparaît
    effectiveness: float  # 0-1


@dataclass
class StrategicIntent:
    """Intention stratégique inférée."""
    intent_id: str
    intent_type: str  # "maximize_coverage", "minimize_risk", "optimize_path"
    evidence: List[str]
    confidence: float
    alignment_score: float  # Alignement avec objectif supposé


class TrajectoryMeaningSystem:
    """
    Système de Signification des Trajectoires.
    
    Analyse sémantique des mouvements pour extraire:
    - Intentions (pourquoi ces actions?)
    - Comportements (quels patterns?)
    - Stratégies (quelle approche globale?)
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Configuration
        self.segment_min_length = 3  # Minimum d'actions pour un segment
        self.segment_max_length = 15  # Maximum d'actions par segment
        self.pattern_min_frequency = 3  # Minimum d'occurrences pour pattern
        
        # Historique des mouvements
        self.movement_history: deque = deque(maxlen=200)
        self.position_history: deque = deque(maxlen=200)
        self.reward_history: deque = deque(maxlen=200)
        
        # Segments identifiés
        self.trajectory_segments: List[TrajectorySegment] = []
        self.current_segment: Optional[TrajectorySegment] = None
        self.segment_counter: int = 0
        
        # Patterns comportementaux
        self.behavioral_patterns: Dict[str, BehavioralPattern] = {}
        self.pattern_counter: int = 0
        
        # Intentions stratégiques
        self.strategic_intents: List[StrategicIntent] = []
        self.current_intent: Optional[StrategicIntent] = None
        
        # Statistiques
        self.total_segments_created: int = 0
        self.total_patterns_identified: int = 0
        self.total_intents_inferred: int = 0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/trajectory_meaning_system.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "TrajectoryMeaningSystem")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "TrajectoryMeaningSystem",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "trajectory_meaning_system.initialized",
                "trajectory_meaning_system",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_segments': self.total_segments_created,
            'total_patterns': self.total_patterns_identified,
            'total_intents': self.total_intents_inferred,
            'active_patterns': len(self.behavioral_patterns),
            'current_segment_label': self.current_segment.semantic_label if self.current_segment else None,
            'avg_intention_confidence': self.current_intent.confidence if self.current_intent else 0.0
        }
    
    def update(self,
               action: str,
               position: Tuple[int, int],
               reward: float,
               compressed_pattern: Optional[Dict],
               step: int) -> Dict:
        """
        Mettre à jour l'analyse sémantique.
        
        Args:
            action: Action effectuée
            position: Position résultante
            reward: Récompense reçue
            compressed_pattern: Pattern compressé (depuis C2)
            step: Step actuel
            
        Returns:
            État sémantique actuel
        """
        # 1. Enregistrer mouvement
        self.movement_history.append(action)
        self.position_history.append(position)
        self.reward_history.append(reward)
        
        # 2. Mettre à jour segment actuel
        self._update_current_segment(action, position, reward, step)
        
        # 3. Détecter fin de segment si nécessaire
        if self._should_segment(compressed_pattern, reward):
            self._finalize_current_segment(step)
            self._start_new_segment(action, position, reward, step)
        
        # 4. Identifier patterns comportementaux
        if len(self.trajectory_segments) >= 5:
            self._identify_behavioral_patterns()
        
        # 5. Inférer intentions stratégiques
        if len(self.trajectory_segments) >= 10:
            self._infer_strategic_intents()
        
        # 6. Créer état
        state = self._create_semantic_state()
        
        # Forensic logging
        if self.logger and step % 10 == 0:  # Log tous les 10 steps
            self.logger.log_event(
                "trajectory_meaning_system.updated",
                "trajectory_meaning_system",
                {
                    "step": step,
                    "current_segment": self.current_segment.semantic_label if self.current_segment else None,
                    "num_patterns": len(self.behavioral_patterns),
                    "current_intent": self.current_intent.intent_type if self.current_intent else None
                }
            )
        
        return state
    
    def _update_current_segment(self,
                                action: str,
                                position: Tuple[int, int],
                                reward: float,
                                step: int) -> None:
        """Mettre à jour le segment actuel."""
        if not self.current_segment:
            self._start_new_segment(action, position, reward, step)
            return
        
        # Ajouter au segment actuel
        self.current_segment.positions.append(position)
        self.current_segment.actions.append(action)
        self.current_segment.end_step = step
        self.current_segment.reward_delta += reward
        
        # Mettre à jour label sémantique
        self._update_semantic_label(self.current_segment)
    
    def _should_segment(self, compressed_pattern: Optional[Dict], reward: float) -> bool:
        """Décider si on doit créer un nouveau segment."""
        if not self.current_segment:
            return False
        
        segment_length = len(self.current_segment.actions)
        
        # Critère 1: Longueur maximale atteinte
        if segment_length >= self.segment_max_length:
            return True
        
        # Critère 2: Changement de pattern (depuis C2)
        if compressed_pattern and segment_length >= self.segment_min_length:
            current_pattern = compressed_pattern.get('pattern_type', '')
            if current_pattern and current_pattern != self.current_segment.semantic_label:
                return True
        
        # Critère 3: Changement drastique de récompense
        if segment_length >= self.segment_min_length:
            avg_reward = self.current_segment.reward_delta / segment_length
            if abs(reward - avg_reward) > 0.5:
                return True
        
        return False
    
    def _start_new_segment(self,
                          action: str,
                          position: Tuple[int, int],
                          reward: float,
                          step: int) -> None:
        """Démarrer un nouveau segment."""
        self.segment_counter += 1
        self.current_segment = TrajectorySegment(
            segment_id=f"seg_{self.segment_counter}",
            positions=[position],
            actions=[action],
            start_step=step,
            end_step=step,
            semantic_label="unknown",
            intention="explore",
            confidence=0.5,
            success=False,
            reward_delta=reward
        )
    
    def _finalize_current_segment(self, step: int) -> None:
        """Finaliser le segment actuel."""
        if not self.current_segment:
            return
        
        # Déterminer succès
        self.current_segment.success = self.current_segment.reward_delta > 0
        
        # Mettre à jour confiance
        self._update_segment_confidence(self.current_segment)
        
        # Ajouter à l'historique
        self.trajectory_segments.append(self.current_segment)
        self.total_segments_created += 1
        
        # Limiter historique
        if len(self.trajectory_segments) > 100:
            self.trajectory_segments = self.trajectory_segments[-100:]
    
    def _update_semantic_label(self, segment: TrajectorySegment) -> None:
        """Mettre à jour le label sémantique d'un segment."""
        if len(segment.actions) < 2:
            segment.semantic_label = "initialization"
            return
        
        # Analyser pattern de mouvement
        unique_actions = len(set(segment.actions))
        total_actions = len(segment.actions)
        
        # Analyser positions
        positions = segment.positions
        unique_positions = len(set(positions))
        
        # Analyser récompenses
        avg_reward = segment.reward_delta / max(1, total_actions)
        
        # Classification sémantique
        if unique_positions == 1:
            segment.semantic_label = "stationary"
            segment.intention = "wait"
        elif unique_actions == 1:
            segment.semantic_label = "linear_probe"
            segment.intention = "discover"
        elif unique_positions < total_actions * 0.5:
            segment.semantic_label = "loop"
            segment.intention = "search"
        elif avg_reward > 0.3:
            segment.semantic_label = "goal_pursuit"
            segment.intention = "reach"
        elif avg_reward < -0.3:
            segment.semantic_label = "escape"
            segment.intention = "avoid"
        elif unique_actions / total_actions > 0.7:
            segment.semantic_label = "exploration"
            segment.intention = "discover"
        else:
            segment.semantic_label = "patrol"
            segment.intention = "optimize"
    
    def _update_segment_confidence(self, segment: TrajectorySegment) -> None:
        """Mettre à jour la confiance dans le label sémantique."""
        # Facteurs de confiance:
        # 1. Longueur du segment
        length_factor = min(1.0, len(segment.actions) / 10.0)
        
        # 2. Cohérence des actions
        unique_ratio = len(set(segment.actions)) / len(segment.actions)
        coherence_factor = 1.0 - unique_ratio
        
        # 3. Clarté de la récompense
        reward_clarity = min(1.0, abs(segment.reward_delta) / 2.0)
        
        confidence = (
            0.4 * length_factor +
            0.3 * coherence_factor +
            0.3 * reward_clarity
        )
        
        segment.confidence = confidence
    
    def _identify_behavioral_patterns(self) -> None:
        """Identifier patterns comportementaux récurrents."""
        if len(self.trajectory_segments) < 5:
            return
        
        # Analyser les 20 derniers segments
        recent_segments = self.trajectory_segments[-20:]
        
        # Compter fréquences des labels sémantiques
        label_counts: Dict[str, int] = {}
        label_successes: Dict[str, int] = {}
        
        for seg in recent_segments:
            label = seg.semantic_label
            label_counts[label] = label_counts.get(label, 0) + 1
            if seg.success:
                label_successes[label] = label_successes.get(label, 0) + 1
        
        # Créer ou mettre à jour patterns
        for label, count in label_counts.items():
            if count >= self.pattern_min_frequency:
                pattern_id = f"pattern_{label}"
                
                success_rate = label_successes.get(label, 0) / count
                
                if pattern_id in self.behavioral_patterns:
                    # Mettre à jour pattern existant
                    pattern = self.behavioral_patterns[pattern_id]
                    pattern.frequency = count
                    pattern.success_rate = success_rate
                else:
                    # Créer nouveau pattern
                    self.pattern_counter += 1
                    pattern = BehavioralPattern(
                        pattern_id=pattern_id,
                        pattern_type=label,
                        frequency=count,
                        success_rate=success_rate,
                        contexts=[],
                        effectiveness=success_rate
                    )
                    self.behavioral_patterns[pattern_id] = pattern
                    self.total_patterns_identified += 1
    
    def _infer_strategic_intents(self) -> None:
        """Inférer intentions stratégiques globales."""
        if len(self.trajectory_segments) < 10:
            return
        
        recent_segments = self.trajectory_segments[-30:]
        
        # Analyser distribution des intentions
        intention_counts: Dict[str, int] = {}
        for seg in recent_segments:
            intention_counts[seg.intention] = intention_counts.get(seg.intention, 0) + 1
        
        # Identifier intention dominante
        if intention_counts:
            dominant_intention = max(intention_counts.items(), key=lambda x: x[1])
            intent_type, count = dominant_intention
            
            confidence = count / len(recent_segments)
            
            # Créer ou mettre à jour intention stratégique
            if confidence > 0.4:  # Seuil de signification
                intent_id = f"intent_{intent_type}"
                
                # Calculer alignement avec succès
                successful_segments = [s for s in recent_segments if s.success and s.intention == intent_type]
                alignment = len(successful_segments) / max(1, count)
                
                intent = StrategicIntent(
                    intent_id=intent_id,
                    intent_type=intent_type,
                    evidence=[f"{count}/{len(recent_segments)} segments"],
                    confidence=confidence,
                    alignment_score=alignment
                )
                
                self.current_intent = intent
                
                # Ajouter si nouveau
                if not any(i.intent_type == intent_type for i in self.strategic_intents):
                    self.strategic_intents.append(intent)
                    self.total_intents_inferred += 1
    
    def _create_semantic_state(self) -> Dict:
        """Créer état sémantique actuel."""
        return {
            "current_segment": {
                "label": self.current_segment.semantic_label if self.current_segment else None,
                "intention": self.current_segment.intention if self.current_segment else None,
                "confidence": self.current_segment.confidence if self.current_segment else 0.0,
                "length": len(self.current_segment.actions) if self.current_segment else 0
            },
            "behavioral_patterns": {
                pattern_id: {
                    "type": p.pattern_type,
                    "frequency": p.frequency,
                    "success_rate": p.success_rate,
                    "effectiveness": p.effectiveness
                }
                for pattern_id, p in self.behavioral_patterns.items()
            },
            "strategic_intent": {
                "type": self.current_intent.intent_type if self.current_intent else None,
                "confidence": self.current_intent.confidence if self.current_intent else 0.0,
                "alignment": self.current_intent.alignment_score if self.current_intent else 0.0
            } if self.current_intent else None,
            "statistics": {
                "total_segments": self.total_segments_created,
                "total_patterns": self.total_patterns_identified,
                "total_intents": self.total_intents_inferred
            }
        }
    
    def get_dominant_behavior(self) -> Optional[str]:
        """Obtenir le comportement dominant."""
        if not self.behavioral_patterns:
            return None
        
        dominant = max(
            self.behavioral_patterns.values(),
            key=lambda p: p.frequency * p.effectiveness
        )
        return dominant.pattern_type
    
    def get_current_intention(self) -> Optional[str]:
        """Obtenir l'intention actuelle."""
        if self.current_segment:
            return self.current_segment.intention
        return None
    
    def get_strategic_direction(self) -> Optional[str]:
        """Obtenir la direction stratégique."""
        if self.current_intent:
            return self.current_intent.intent_type
        return None
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques."""
        return {
            "total_segments": self.total_segments_created,
            "total_patterns": self.total_patterns_identified,
            "total_intents": self.total_intents_inferred,
            "active_patterns": len(self.behavioral_patterns),
            "current_segment_label": self.current_segment.semantic_label if self.current_segment else None,
            "dominant_behavior": self.get_dominant_behavior(),
            "strategic_direction": self.get_strategic_direction()
        }
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter état en JSON."""
        data = {
            "statistics": self.get_statistics(),
            "current_state": self._create_semantic_state(),
            "recent_segments": [
                {
                    "id": seg.segment_id,
                    "label": seg.semantic_label,
                    "intention": seg.intention,
                    "length": len(seg.actions),
                    "success": seg.success,
                    "confidence": seg.confidence
                }
                for seg in self.trajectory_segments[-10:]
            ],
            "behavioral_patterns": [
                {
                    "id": p.pattern_id,
                    "type": p.pattern_type,
                    "frequency": p.frequency,
                    "success_rate": p.success_rate,
                    "effectiveness": p.effectiveness
                }
                for p in self.behavioral_patterns.values()
            ],
            "strategic_intents": [
                {
                    "id": i.intent_id,
                    "type": i.intent_type,
                    "confidence": i.confidence,
                    "alignment": i.alignment_score
                }
                for i in self.strategic_intents
            ]
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "TrajectoryMeaningSystem",
            {
                "final_state": self._forensic_state,
                "total_segments": self.total_segments_created,
                "total_patterns": self.total_patterns_identified,
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()


# Made with Bob
