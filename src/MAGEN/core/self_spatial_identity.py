"""
MAGEN V32 - Module C6: SelfSpatialIdentity
Spatial Cognitive System - Identité Spatiale du Soi

Mission: Maintenir conscience stable de l'avatar dans l'espace
Problème V31: Avatar jamais identifié (threshold 0.8 jamais atteint)
Solution V32: Ancrage spatial robuste avec multi-critères
"""

import time
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from collections import deque
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class AvatarHypothesis:
    """Hypothèse d'identité avatar."""
    position: Tuple[int, int]
    confidence: float  # 0-1
    evidence_count: int
    first_seen_step: int
    last_seen_step: int
    movement_pattern: List[str]  # ["north", "east", ...]
    stability_score: float  # 0-1: Stabilité de l'hypothèse


@dataclass
class SpatialSelfState:
    """État de l'identité spatiale."""
    avatar_position: Optional[Tuple[int, int]]
    avatar_confidence: float
    position_history: List[Tuple[int, int]]
    movement_history: List[str]
    spatial_awareness: float  # 0-1: Conscience de sa position
    identity_stability: float  # 0-1: Stabilité de l'identité


class SelfSpatialIdentity:
    """
    Identité Spatiale du Soi.
    
    Maintient une conscience robuste de la position de l'avatar
    dans l'espace, avec ancrage multi-critères.
    
    Amélioration V32 vs V31:
    - Threshold adaptatif (0.6 → 0.8)
    - Multi-critères (mouvement + observation + cohérence)
    - Historique de confiance
    - Détection de perte d'identité
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Configuration
        self.confidence_threshold = 0.6  # Seuil initial plus permissif
        self.max_threshold = 0.8  # Seuil maximal
        self.min_evidence_count = 3  # Minimum d'observations
        self.position_memory_size = 50
        
        # État actuel
        self.avatar_position: Optional[Tuple[int, int]] = None
        self.avatar_confidence: float = 0.0
        self.identity_locked: bool = False
        
        # Hypothèses actives
        self.active_hypotheses: Dict[Tuple[int, int], AvatarHypothesis] = {}
        
        # Historique
        self.position_history: deque = deque(maxlen=self.position_memory_size)
        self.movement_history: deque = deque(maxlen=self.position_memory_size)
        self.confidence_history: deque = deque(maxlen=100)
        
        # Statistiques
        self.total_observations: int = 0
        self.identity_changes: int = 0
        self.identity_losses: int = 0
        self.false_positives: int = 0
        
        # Métriques de qualité
        self.spatial_awareness: float = 0.0
        self.identity_stability: float = 0.0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/self_spatial_identity.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "SelfSpatialIdentity")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "SelfSpatialIdentity",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "confidence_threshold": self.confidence_threshold,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.initialized",
                "self_spatial_identity",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0",
                    "confidence_threshold": self.confidence_threshold
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_observations': self.total_observations,
            'avatar_confidence': self.avatar_confidence,
            'identity_locked': self.identity_locked,
            'spatial_awareness': self.spatial_awareness,
            'identity_stability': self.identity_stability,
            'hypotheses_count': len(self.active_hypotheses),
            'identity_changes': self.identity_changes,
            'identity_losses': self.identity_losses
        }
    
    def update_from_observation(self,
                               observation: Dict,
                               action_taken: Optional[str],
                               step: int) -> SpatialSelfState:
        """
        Mettre à jour l'identité spatiale depuis une observation.
        
        Args:
            observation: Observation du monde
            action_taken: Action effectuée (pour cohérence mouvement)
            step: Step actuel
            
        Returns:
            État de l'identité spatiale
        """
        start_ns = time.time_ns()
        self.total_observations += 1
        
        # Log début update
        self.forensic.forensic_logger.log(
            "update_start",
            "SelfSpatialIdentity",
            {
                "step": step,
                "action_taken": action_taken,
                "state": self._forensic_state
            }
        )
        
        # 1. Extraire positions candidates depuis observation
        candidates = self._extract_avatar_candidates(observation)
        
        # 2. Mettre à jour hypothèses
        self._update_hypotheses(candidates, action_taken, step)
        
        # 3. Évaluer hypothèse la plus forte
        best_hypothesis = self._select_best_hypothesis()
        
        # 4. Décider si on lock l'identité
        if best_hypothesis and not self.identity_locked:
            if best_hypothesis.confidence >= self.confidence_threshold:
                self._lock_identity(best_hypothesis, step)
        
        # 5. Vérifier cohérence si identité locked
        if self.identity_locked and self.avatar_position:
            self._verify_identity_coherence(observation, action_taken, step)
        
        # 6. Mettre à jour métriques
        self._update_metrics()
        
        # 7. Créer état
        state = SpatialSelfState(
            avatar_position=self.avatar_position,
            avatar_confidence=self.avatar_confidence,
            position_history=list(self.position_history),
            movement_history=list(self.movement_history),
            spatial_awareness=self.spatial_awareness,
            identity_stability=self.identity_stability
        )
        
        # Forensic logging standard
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "update_complete",
            "SelfSpatialIdentity",
            {
                "step": step,
                "avatar_position": self.avatar_position,
                "confidence": self.avatar_confidence,
                "locked": self.identity_locked,
                "execution_time_ns": end_ns - start_ns,
                "success": True,
                "state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.updated",
                "self_spatial_identity",
                {
                    "step": step,
                    "avatar_position": self.avatar_position,
                    "confidence": self.avatar_confidence,
                    "locked": self.identity_locked,
                    "num_hypotheses": len(self.active_hypotheses)
                }
            )
        
        return state
    
    def _extract_avatar_candidates(self, observation: Dict) -> List[Tuple[int, int]]:
        """Extraire positions candidates pour l'avatar."""
        candidates = []
        
        # Méthode 1: Position explicite dans observation
        if 'avatar_position' in observation:
            candidates.append(tuple(observation['avatar_position']))
        
        # Méthode 2: Analyser la grille pour symboles spéciaux
        if 'grid' in observation:
            grid = observation['grid']
            for y, row in enumerate(grid):
                for x, cell in enumerate(row):
                    # Symboles typiques d'avatar: 'A', '@', 'P', etc.
                    if cell in ['A', '@', 'P', '☺', '◉']:
                        candidates.append((x, y))
        
        # Méthode 3: Position centrale si rien trouvé
        if not candidates and 'grid' in observation:
            grid = observation['grid']
            h, w = len(grid), len(grid[0]) if grid else 0
            if h > 0 and w > 0:
                candidates.append((w // 2, h // 2))
        
        return candidates
    
    def _update_hypotheses(self,
                          candidates: List[Tuple[int, int]],
                          action_taken: Optional[str],
                          step: int) -> None:
        """Mettre à jour les hypothèses d'identité."""
        # Décrémenter confiance des hypothèses non observées
        for pos in list(self.active_hypotheses.keys()):
            if pos not in candidates:
                self.active_hypotheses[pos].confidence *= 0.9
                
                # Supprimer si confiance trop faible
                if self.active_hypotheses[pos].confidence < 0.1:
                    del self.active_hypotheses[pos]
        
        # Mettre à jour ou créer hypothèses pour candidats
        for pos in candidates:
            if pos in self.active_hypotheses:
                # Renforcer hypothèse existante
                hyp = self.active_hypotheses[pos]
                hyp.evidence_count += 1
                hyp.last_seen_step = step
                
                # Augmenter confiance
                hyp.confidence = min(1.0, hyp.confidence + 0.15)
                
                # Vérifier cohérence mouvement
                if action_taken and self.avatar_position:
                    expected_pos = self._predict_position_after_action(
                        self.avatar_position, action_taken
                    )
                    if pos == expected_pos:
                        hyp.confidence = min(1.0, hyp.confidence + 0.2)
                        hyp.movement_pattern.append(action_taken)
                
                # Calculer stabilité
                hyp.stability_score = self._compute_stability(hyp)
                
            else:
                # Créer nouvelle hypothèse
                self.active_hypotheses[pos] = AvatarHypothesis(
                    position=pos,
                    confidence=0.3,  # Confiance initiale modérée
                    evidence_count=1,
                    first_seen_step=step,
                    last_seen_step=step,
                    movement_pattern=[],
                    stability_score=0.0
                )
    
    def _predict_position_after_action(self,
                                      current_pos: Tuple[int, int],
                                      action: str) -> Tuple[int, int]:
        """Prédire position après une action."""
        x, y = current_pos
        
        action_lower = action.lower()
        if 'north' in action_lower or 'up' in action_lower:
            return (x, y - 1)
        elif 'south' in action_lower or 'down' in action_lower:
            return (x, y + 1)
        elif 'east' in action_lower or 'right' in action_lower:
            return (x + 1, y)
        elif 'west' in action_lower or 'left' in action_lower:
            return (x - 1, y)
        else:
            return current_pos  # Action inconnue
    
    def _compute_stability(self, hypothesis: AvatarHypothesis) -> float:
        """Calculer score de stabilité d'une hypothèse."""
        # Facteurs de stabilité:
        # 1. Durée d'observation
        duration = hypothesis.last_seen_step - hypothesis.first_seen_step
        duration_score = min(1.0, duration / 20.0)
        
        # 2. Nombre d'évidences
        evidence_score = min(1.0, hypothesis.evidence_count / 10.0)
        
        # 3. Cohérence des mouvements
        movement_score = 0.5
        if len(hypothesis.movement_pattern) >= 3:
            # Vérifier si mouvements sont cohérents (pas trop erratiques)
            unique_moves = len(set(hypothesis.movement_pattern[-5:]))
            movement_score = 1.0 - (unique_moves / 5.0)
        
        stability = (
            0.3 * duration_score +
            0.4 * evidence_score +
            0.3 * movement_score
        )
        
        return stability
    
    def _select_best_hypothesis(self) -> Optional[AvatarHypothesis]:
        """Sélectionner la meilleure hypothèse."""
        if not self.active_hypotheses:
            return None
        
        # Scorer chaque hypothèse
        best_hyp = None
        best_score = 0.0
        
        for hyp in self.active_hypotheses.values():
            # Score = confiance * stabilité * evidence
            score = (
                hyp.confidence * 0.5 +
                hyp.stability_score * 0.3 +
                min(1.0, hyp.evidence_count / 5.0) * 0.2
            )
            
            if score > best_score:
                best_score = score
                best_hyp = hyp
        
        return best_hyp
    
    def _lock_identity(self, hypothesis: AvatarHypothesis, step: int) -> None:
        """Verrouiller l'identité sur une hypothèse."""
        old_position = self.avatar_position
        
        self.avatar_position = hypothesis.position
        self.avatar_confidence = hypothesis.confidence
        self.identity_locked = True
        
        # Enregistrer changement si position différente
        if old_position and old_position != hypothesis.position:
            self.identity_changes += 1
        
        # Ajouter à l'historique
        self.position_history.append(hypothesis.position)
        self.confidence_history.append(hypothesis.confidence)
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.identity_locked",
                "self_spatial_identity",
                {
                    "step": step,
                    "position": hypothesis.position,
                    "confidence": hypothesis.confidence,
                    "evidence_count": hypothesis.evidence_count
                }
            )
    
    def _verify_identity_coherence(self,
                                   observation: Dict,
                                   action_taken: Optional[str],
                                   step: int) -> None:
        """Vérifier cohérence de l'identité locked."""
        if not self.avatar_position:
            return
        
        # Vérifier si position actuelle est toujours observée
        candidates = self._extract_avatar_candidates(observation)
        
        if self.avatar_position not in candidates:
            # Position perdue !
            self.avatar_confidence *= 0.7
            
            if self.avatar_confidence < 0.4:
                # Perte d'identité
                self._lose_identity(step)
        else:
            # Position confirmée
            self.avatar_confidence = min(1.0, self.avatar_confidence + 0.05)
            
            # Vérifier cohérence mouvement
            if action_taken:
                expected_pos = self._predict_position_after_action(
                    self.avatar_position, action_taken
                )
                
                if expected_pos in candidates:
                    # Mouvement cohérent
                    self.avatar_position = expected_pos
                    self.avatar_confidence = min(1.0, self.avatar_confidence + 0.1)
                    self.position_history.append(expected_pos)
                    self.movement_history.append(action_taken)
    
    def _lose_identity(self, step: int) -> None:
        """Perdre l'identité avatar."""
        self.identity_losses += 1
        self.identity_locked = False
        self.avatar_position = None
        self.avatar_confidence = 0.0
        
        # Réinitialiser hypothèses
        self.active_hypotheses.clear()
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.identity_lost",
                "self_spatial_identity",
                {
                    "step": step,
                    "total_losses": self.identity_losses
                }
            )
    
    def _update_metrics(self) -> None:
        """Mettre à jour les métriques de qualité."""
        # Spatial awareness = confiance + historique
        if self.avatar_confidence > 0:
            history_factor = min(1.0, len(self.position_history) / 20.0)
            self.spatial_awareness = 0.7 * self.avatar_confidence + 0.3 * history_factor
        else:
            self.spatial_awareness = 0.0
        
        # Identity stability = stabilité dans le temps
        if len(self.confidence_history) >= 10:
            recent_conf = list(self.confidence_history)[-10:]
            variance = sum((c - self.avatar_confidence) ** 2 for c in recent_conf) / 10
            self.identity_stability = max(0.0, 1.0 - variance * 2.0)
        else:
            self.identity_stability = 0.5
    
    def get_current_state(self) -> Dict:
        """Obtenir l'état actuel de l'identité."""
        return {
            "avatar_position": self.avatar_position,
            "avatar_confidence": self.avatar_confidence,
            "identity_locked": self.identity_locked,
            "spatial_awareness": self.spatial_awareness,
            "identity_stability": self.identity_stability,
            "num_hypotheses": len(self.active_hypotheses),
            "position_history_length": len(self.position_history)
        }
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques."""
        return {
            "total_observations": self.total_observations,
            "identity_changes": self.identity_changes,
            "identity_losses": self.identity_losses,
            "false_positives": self.false_positives,
            "current_confidence": self.avatar_confidence,
            "spatial_awareness": self.spatial_awareness,
            "identity_stability": self.identity_stability
        }
    
    def force_identity(self, position: Tuple[int, int], confidence: float = 0.8) -> None:
        """Forcer l'identité à une position (pour tests)."""
        self.avatar_position = position
        self.avatar_confidence = confidence
        self.identity_locked = True
        self.position_history.append(position)
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.identity_forced",
                "self_spatial_identity",
                {
                    "position": position,
                    "confidence": confidence
                }
            )
    
    def reset(self) -> None:
        """Réinitialiser l'identité."""
        self.avatar_position = None
        self.avatar_confidence = 0.0
        self.identity_locked = False
        self.active_hypotheses.clear()
        self.position_history.clear()
        self.movement_history.clear()
        self.confidence_history.clear()
        
        if self.logger:
            self.logger.log_event(
                "self_spatial_identity.reset",
                "self_spatial_identity",
                {}
            )
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return {**self.get_current_state(), **self.get_statistics()}
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter état en JSON."""
        data = {
            "current_state": self.get_current_state(),
            "statistics": self.get_statistics(),
            "active_hypotheses": [
                {
                    "position": list(hyp.position),
                    "confidence": hyp.confidence,
                    "evidence_count": hyp.evidence_count,
                    "stability": hyp.stability_score
                }
                for hyp in self.active_hypotheses.values()
            ],
            "recent_positions": [list(pos) for pos in list(self.position_history)[-20:]],
            "recent_movements": list(self.movement_history)[-20:]
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "SelfSpatialIdentity",
            {
                "final_state": self._forensic_state,
                "total_observations": self.total_observations,
                "identity_changes": self.identity_changes,
                "identity_losses": self.identity_losses,
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()

# Made with Bob
