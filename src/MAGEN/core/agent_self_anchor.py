"""
MAGEN V31 - Agent Self-Anchor Module
====================================

**Objectif**: Identifier l'avatar contrôlé par l'agent via analyse causale motion↔action

**Principe**: L'avatar est l'entité dont le mouvement est causalement corrélé aux actions

**Méthode**:
1. Observer état avant action
2. Exécuter action  
3. Observer entités ayant bougé
4. Accumuler corrélation motion ↔ action
5. Confirmer après 3-5 actions (confidence > 0.8)

**Formulation**:
    agent = argmax_entity(correlation(entity.motion, action_history))

**Session**: 73 - Jour 8 - Phase 1
**Date**: 2026-06-15
**Version**: 1.0.0
"""

import time
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from collections import defaultdict
import numpy as np


@dataclass
class MotionSignature:
    """Signature de mouvement d'une entité"""
    entity_id: int
    positions: List[Tuple[int, int]] = field(default_factory=list)
    actions_correlated: List[str] = field(default_factory=list)
    motion_count: int = 0
    correlation_score: float = 0.0
    last_seen: int = 0  # Step number


@dataclass
class AnchorEvent:
    """Événement forensique d'ancrage"""
    timestamp_ns: int
    event_type: str  # motion_observed, avatar_identified, confidence_updated
    entity_id: Optional[int]
    position: Optional[Tuple[int, int]]
    action: Optional[str]
    confidence: float
    data: Dict


class AgentSelfAnchor:
    """
    Module d'identification de l'avatar contrôlé
    
    Identifie l'entité dont les mouvements sont causalement corrélés
    aux actions exécutées par l'agent.
    
    **Métriques de succès**:
    - Identification < 10 steps
    - Confidence > 0.8
    - Robustesse multi-entités
    """
    
    def __init__(self, forensic_logger=None):
        """
        Initialise le module d'ancrage
        
        Args:
            forensic_logger: Logger forensique pour traçabilité complète
        """
        # État identification
        self.agent_id: Optional[int] = None
        self.confidence: float = 0.0
        self.identification_step: Optional[int] = None
        
        # Historique trajectoire avatar
        self.trajectory_history: List[Tuple[int, int]] = []
        
        # Signatures mouvement par entité
        self.motion_signatures: Dict[int, MotionSignature] = {}
        
        # Historique actions
        self.action_history: List[str] = []
        
        # Compteur steps
        self.step_count: int = 0
        
        # Forensic logging
        self.forensic_logger = forensic_logger
        self.events: List[AnchorEvent] = []
        
        # Seuils
        self.confidence_threshold: float = 0.8
        self.min_observations: int = 3
        
        self._log_event("anchor_initialized", None, None, None, {
            "confidence_threshold": self.confidence_threshold,
            "min_observations": self.min_observations
        })
    
    def observe_motion(
        self,
        prev_entities: List[Dict],
        curr_entities: List[Dict],
        action: str
    ) -> None:
        """
        Observe mouvement des entités après action
        
        Args:
            prev_entities: Liste entités avant action [{id, x, y, type}, ...]
            curr_entities: Liste entités après action
            action: Action exécutée (ACTION1, ACTION2, etc.)
        """
        start_ns = time.perf_counter_ns()
        self.step_count += 1
        self.action_history.append(action)
        
        # Créer mapping id → position
        prev_positions = {e['id']: (e['x'], e['y']) for e in prev_entities if 'id' in e and 'x' in e and 'y' in e}
        curr_positions = {e['id']: (e['x'], e['y']) for e in curr_entities if 'id' in e and 'x' in e and 'y' in e}
        
        # Détecter entités ayant bougé
        moved_entities = []
        for entity_id in prev_positions:
            if entity_id in curr_positions:
                prev_pos = prev_positions[entity_id]
                curr_pos = curr_positions[entity_id]
                
                if prev_pos != curr_pos:
                    # Entité a bougé
                    moved_entities.append({
                        'id': entity_id,
                        'prev_pos': prev_pos,
                        'curr_pos': curr_pos,
                        'delta': (curr_pos[0] - prev_pos[0], curr_pos[1] - prev_pos[1])
                    })
                    
                    # Mettre à jour signature
                    if entity_id not in self.motion_signatures:
                        self.motion_signatures[entity_id] = MotionSignature(
                            entity_id=entity_id
                        )
                    
                    sig = self.motion_signatures[entity_id]
                    sig.positions.append(curr_pos)
                    sig.actions_correlated.append(action)
                    sig.motion_count += 1
                    sig.last_seen = self.step_count
        
        # Log événement
        self._log_event("motion_observed", None, None, action, {
            "step": self.step_count,
            "action": action,
            "entities_moved": len(moved_entities),
            "moved_ids": [e['id'] for e in moved_entities],
            "total_entities": len(curr_entities),
            "observation_time_ns": time.perf_counter_ns() - start_ns
        })
        
        # Tenter identification si pas encore fait
        if self.agent_id is None and self.step_count >= self.min_observations:
            self._attempt_identification()
    
    def _attempt_identification(self) -> None:
        """
        Tente d'identifier l'avatar via corrélation causale
        
        Calcule score de corrélation pour chaque entité:
        - Fréquence mouvement (motion_count / total_steps)
        - Cohérence temporelle (mouvements récents)
        - Diversité actions (nombre actions différentes corrélées)
        """
        start_ns = time.perf_counter_ns()
        
        if not self.motion_signatures:
            return
        
        # Calculer scores corrélation
        scores = {}
        for entity_id, sig in self.motion_signatures.items():
            # Fréquence mouvement
            motion_frequency = sig.motion_count / self.step_count
            
            # Cohérence temporelle (bonus si mouvements récents)
            recency_factor = 1.0 if sig.last_seen == self.step_count else 0.5
            
            # Diversité actions
            unique_actions = len(set(sig.actions_correlated))
            action_diversity = unique_actions / len(self.action_history) if self.action_history else 0.0
            
            # Score final (pondéré)
            score = (
                0.5 * motion_frequency +
                0.3 * recency_factor +
                0.2 * action_diversity
            )
            
            scores[entity_id] = score
        
        # Identifier entité avec score maximal
        if scores:
            best_entity = max(scores.items(), key=lambda x: x[1])
            best_id, best_score = best_entity
            
            # Vérifier seuil confidence
            if best_score >= self.confidence_threshold:
                self.agent_id = best_id
                self.confidence = best_score
                self.identification_step = self.step_count
                
                # Initialiser trajectoire
                sig = self.motion_signatures[best_id]
                self.trajectory_history = sig.positions.copy()
                
                self._log_event("avatar_identified", best_id, 
                               sig.positions[-1] if sig.positions else None,
                               None, {
                    "entity_id": best_id,
                    "confidence": best_score,
                    "step": self.step_count,
                    "motion_count": sig.motion_count,
                    "unique_actions": len(set(sig.actions_correlated)),
                    "identification_time_ns": time.perf_counter_ns() - start_ns
                })
            else:
                # Confidence insuffisante, continuer observation
                self._log_event("confidence_insufficient", best_id, None, None, {
                    "best_entity": best_id,
                    "best_score": best_score,
                    "threshold": self.confidence_threshold,
                    "step": self.step_count
                })
    
    def identify_avatar(self) -> Optional[int]:
        """
        Retourne ID avatar identifié
        
        Returns:
            ID avatar si identifié avec confidence > threshold, None sinon
        """
        return self.agent_id if self.confidence >= self.confidence_threshold else None
    
    def get_avatar_position(
        self,
        entities: List[Dict]
    ) -> Optional[Tuple[int, int]]:
        """
        Retourne position actuelle de l'avatar
        
        Args:
            entities: Liste entités actuelles
            
        Returns:
            (x, y) position avatar, ou None si non identifié
        """
        if self.agent_id is None:
            return None
        
        for entity in entities:
            if entity.get('id') == self.agent_id:
                x = entity.get('x')
                y = entity.get('y')
                if x is not None and y is not None:
                    return (int(x), int(y))
        
        return None
    
    def get_trajectory(self) -> List[Tuple[int, int]]:
        """
        Retourne trajectoire complète de l'avatar
        
        Returns:
            Liste positions [(x, y), ...] dans l'ordre chronologique
        """
        return self.trajectory_history.copy()
    
    def get_statistics(self) -> Dict:
        """
        Retourne statistiques d'identification
        
        Returns:
            Dict avec métriques identification
        """
        return {
            "agent_id": self.agent_id,
            "confidence": self.confidence,
            "identification_step": self.identification_step,
            "current_step": self.step_count,
            "trajectory_length": len(self.trajectory_history),
            "entities_tracked": len(self.motion_signatures),
            "actions_observed": len(self.action_history),
            "identified": self.agent_id is not None
        }
    
    def _log_event(
        self,
        event_type: str,
        entity_id: Optional[int],
        position: Optional[Tuple[int, int]],
        action: Optional[str],
        data: Dict
    ) -> None:
        """
        Log événement forensique
        
        Args:
            event_type: Type événement
            entity_id: ID entité concernée
            position: Position entité
            action: Action exécutée
            data: Données additionnelles
        """
        event = AnchorEvent(
            timestamp_ns=time.perf_counter_ns(),
            event_type=event_type,
            entity_id=entity_id,
            position=position,
            action=action,
            confidence=self.confidence,
            data=data
        )
        
        self.events.append(event)
        
        # Log vers forensic logger externe si disponible
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type=f"agent_self_anchor.{event_type}",
                component="agent_self_anchor",
                data={
                    "entity_id": entity_id,
                    "position": position,
                    "action": action,
                    "confidence": self.confidence,
                    **data
                }
            )
    
    def reset(self) -> None:
        """Reset complet du module (nouveau jeu)"""
        self.agent_id = None
        self.confidence = 0.0
        self.identification_step = None
        self.trajectory_history.clear()
        self.motion_signatures.clear()
        self.action_history.clear()
        self.step_count = 0
        self.events.clear()
        
        self._log_event("anchor_reset", None, None, None, {
            "reason": "manual_reset"
        })

# Made with Bob
