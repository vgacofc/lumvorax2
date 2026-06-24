"""
Behavioral Entity Detector - MAGEN V27.1
=========================================

Principe AGI-Compatible:
    "Le joueur ne doit plus être reconnu.
     Il doit être inféré."

Méthode:
    Inférence causale basée sur corrélation action→changement
    
    "L'objet qui change systématiquement après mes actions
     est probablement moi"

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14

FORENSIC: Intégré via ForensicMiddleware (PROTOCOLE_MAGEN V3.0)
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass, field
from collections import defaultdict
import time
import json

from .forensic_middleware import ForensicMiddleware, CausalIDManager


@dataclass
class Entity:
    """Représentation d'une entité dans le monde"""
    entity_id: int
    positions: List[Tuple[int, int]]  # Historique positions
    values: List[int]  # Historique valeurs
    first_seen: int  # Timestamp première observation
    last_seen: int  # Timestamp dernière observation
    
    # Scores comportementaux
    mobility_score: float = 0.0  # Fréquence mouvement
    causality_score: float = 0.0  # Corrélation avec actions
    persistence_score: float = 0.0  # Stabilité temporelle
    interaction_score: float = 0.0  # Modifications environnement
    
    # Métadonnées
    is_stable: bool = False  # Entité statique (décor)
    is_mobile: bool = False  # Entité mobile
    is_reactive: bool = False  # Réagit aux actions
    is_agent_candidate: bool = False  # Candidat joueur


@dataclass
class CausalEvent:
    """Événement causal action→changement"""
    timestamp: int
    action: str
    entity_id: int
    position_before: Tuple[int, int]
    position_after: Tuple[int, int]
    value_before: int
    value_after: int
    distance_moved: float
    causality_strength: float  # 0.0-1.0


class BehavioralEntityDetector:
    """
    Détecteur d'entités par comportement et inférence causale.
    
    Capacités:
    1. Tracking entités temporel
    2. Détection mobilité
    3. Inférence causale action→changement
    4. Identification agent par causalité
    5. Classification entités (stable/mobile/reactive)
    """
    
    def __init__(
        self,
        causality_threshold: float = 0.6,
        mobility_threshold: float = 0.3,
        persistence_threshold: int = 3,
        max_history: int = 100,
        forensic_log_path: Optional[str] = None,
        id_manager: Optional[CausalIDManager] = None
    ):
        """
        Args:
            causality_threshold: Seuil score causal pour agent
            mobility_threshold: Seuil mobilité pour entité mobile
            persistence_threshold: Observations min pour entité stable
            max_history: Taille max historique
            forensic_log_path: Chemin fichier log forensic (optionnel)
            id_manager: Manager IDs causaux partagé (optionnel)
        """
        self.causality_threshold = causality_threshold
        self.mobility_threshold = mobility_threshold
        self.persistence_threshold = persistence_threshold
        self.max_history = max_history
        
        # État interne
        self.entities: Dict[int, Entity] = {}
        self.causal_events: List[CausalEvent] = []
        self.action_history: List[Tuple[int, str]] = []  # (timestamp, action)
        self.grid_history: List[Tuple[int, np.ndarray]] = []  # (timestamp, grid)
        
        # Statistiques
        self.total_observations = 0
        self.agent_entity_id: Optional[int] = None
        self.agent_confidence: float = 0.0
        
        # FORENSIC: Middleware standardisé (PROTOCOLE_MAGEN V3.0)
        self.forensic: Optional[ForensicMiddleware] = None
        if forensic_log_path:
            self.forensic = ForensicMiddleware(
                forensic_log_path,
                "BehavioralEntityDetector",
                id_manager
            )
    
    def observe(self, grid: np.ndarray, action: Optional[str] = None) -> Dict:
        """
        Observe nouvelle grille et met à jour modèle monde.
        
        Args:
            grid: Grille observation (H, W) ou (C, H, W)
            action: Action exécutée (optionnel)
        
        Returns:
            observation_result: Dict avec entités détectées
        """
        timestamp_start = int(time.time_ns())
        timestamp = timestamp_start
        
        # FORENSIC: Log observation start
        if self.forensic:
            self.forensic.log_event('observation_start', {
                'timestamp_ns': timestamp,
                'grid_shape': list(grid.shape),
                'action': action,
                'entities_count_before': len(self.entities),
                'total_observations': self.total_observations
            })
        
        # Normaliser grille
        if len(grid.shape) == 3:
            grid = grid[0]  # Prendre premier channel
        
        # Enregistrer action
        if action:
            self.action_history.append((timestamp, action))
        
        # Enregistrer grille
        self.grid_history.append((timestamp, grid.copy()))
        
        # Limiter historique
        if len(self.grid_history) > self.max_history:
            self.grid_history.pop(0)
        if len(self.action_history) > self.max_history:
            self.action_history.pop(0)
        
        # Extraire entités
        entities_detected = self._extract_entities(grid, timestamp)
        
        # Tracking temporel
        if len(self.grid_history) >= 2:
            self._update_temporal_tracking()
        
        # Inférence causale
        if len(self.action_history) >= 1 and len(self.grid_history) >= 2:
            self._infer_causality()
        
        # Identifier agent
        if len(self.causal_events) >= 3:
            self._identify_agent()
        
        self.total_observations += 1
        
        # FORENSIC: Log observation complete
        timestamp_end = int(time.time_ns())
        if self.forensic:
            self.forensic.log_event('observation_complete', {
                'timestamp_ns': timestamp_end,
                'duration_ns': timestamp_end - timestamp_start,
                'entities_detected': len(entities_detected),
                'agent_detected': self.agent_entity_id is not None,
                'agent_confidence': float(self.agent_confidence),
                'causal_events_count': len(self.causal_events)
            })
        
        return {
            'entities_count': len(self.entities),
            'agent_detected': self.agent_entity_id is not None,
            'agent_id': self.agent_entity_id,
            'agent_confidence': self.agent_confidence,
            'agent_position': self._get_agent_position(),
            'mobile_entities': self._count_mobile_entities(),
            'stable_entities': self._count_stable_entities(),
            'reactive_entities': self._count_reactive_entities()
        }
    
    def _extract_entities(self, grid: np.ndarray, timestamp: int) -> List[Entity]:
        """Extrait entités uniques de la grille"""
        entities_found = []
        unique_values = np.unique(grid)
        
        for value in unique_values:
            if value == 0:  # Skip background
                continue
            
            # Protection contre grids 0D/1D
            if grid.ndim < 2:
                continue
            
            # Trouver positions
            positions = list(zip(*np.where(grid == value)))
            
            if not positions:
                continue
            
            # Créer ou mettre à jour entité
            entity_id = int(value)  # Simple: valeur = ID
            
            if entity_id not in self.entities:
                # Nouvelle entité
                entity = Entity(
                    entity_id=entity_id,
                    positions=[positions[0]],  # Position initiale
                    values=[value],
                    first_seen=timestamp,
                    last_seen=timestamp
                )
                self.entities[entity_id] = entity
            else:
                # Mettre à jour entité existante
                entity = self.entities[entity_id]
                entity.positions.append(positions[0])
                entity.values.append(value)
                entity.last_seen = timestamp
                
                # Limiter historique
                if len(entity.positions) > self.max_history:
                    entity.positions.pop(0)
                    entity.values.pop(0)
            
            entities_found.append(self.entities[entity_id])
        
        return entities_found
    
    def _update_temporal_tracking(self):
        """Met à jour scores mobilité et persistence"""
        for entity in self.entities.values():
            if len(entity.positions) < 2:
                continue
            
            # Calculer mobilité
            movements = 0
            total_distance = 0.0
            
            for i in range(1, len(entity.positions)):
                pos_prev = entity.positions[i-1]
                pos_curr = entity.positions[i]
                
                distance = np.sqrt(
                    (pos_curr[0] - pos_prev[0])**2 +
                    (pos_curr[1] - pos_prev[1])**2
                )
                
                if distance > 0:
                    movements += 1
                    total_distance += distance
            
            # Score mobilité
            entity.mobility_score = movements / len(entity.positions)
            
            # Score persistence
            observations = len(entity.positions)
            entity.persistence_score = min(1.0, observations / self.persistence_threshold)
            
            # Classification
            entity.is_mobile = entity.mobility_score > self.mobility_threshold
            entity.is_stable = (
                entity.persistence_score >= 1.0 and
                entity.mobility_score < self.mobility_threshold
            )
    
    def _infer_causality(self):
        """Infère causalité action→changement pour chaque entité"""
        if len(self.grid_history) < 2 or len(self.action_history) < 1:
            return
        
        # Dernière action et grilles
        last_action_time, last_action = self.action_history[-1]
        
        # Trouver grilles avant/après action
        grid_before = None
        grid_after = None
        
        for i in range(len(self.grid_history) - 1, -1, -1):
            ts, grid = self.grid_history[i]
            if ts <= last_action_time and grid_before is None:
                grid_before = grid
            if ts > last_action_time and grid_after is None:
                grid_after = grid
                break
        
        if grid_before is None or grid_after is None:
            return
        
        # Détecter changements pour chaque entité
        for entity in self.entities.values():
            # Vérifier historique suffisant
            if len(entity.positions) < 1 or len(entity.values) < 1:
                continue
            
            # Positions avant/après (sécurisé)
            if len(entity.positions) >= 2:
                pos_before = entity.positions[-2]
                pos_after = entity.positions[-1]
            else:
                # Pas assez d'historique, skip
                continue
            
            # Valeurs avant/après (sécurisé)
            if len(entity.values) >= 2:
                val_before = entity.values[-2]
                val_after = entity.values[-1]
            else:
                val_before = entity.values[-1]
                val_after = entity.values[-1]
            
            # Distance mouvement
            distance = np.sqrt(
                (pos_after[0] - pos_before[0])**2 +
                (pos_after[1] - pos_before[1])**2
            )
            
            # Changement détecté ?
            if distance > 0 or val_before != val_after:
                # Calculer force causale
                # Plus le changement est proche temporellement de l'action, plus la causalité est forte
                time_delta = abs(self.grid_history[-1][0] - last_action_time)
                causality_strength = 1.0 / (1.0 + time_delta / 1e9)  # Décroissance temporelle
                
                # Créer événement causal
                event = CausalEvent(
                    timestamp=last_action_time,
                    action=last_action,
                    entity_id=entity.entity_id,
                    position_before=pos_before,
                    position_after=pos_after,
                    value_before=val_before,
                    value_after=val_after,
                    distance_moved=distance,
                    causality_strength=causality_strength
                )
                
                self.causal_events.append(event)
                
                # Limiter historique
                if len(self.causal_events) > self.max_history:
                    self.causal_events.pop(0)
                
                # Mettre à jour score causalité entité
                entity_events = [e for e in self.causal_events if e.entity_id == entity.entity_id]
                if entity_events:
                    entity.causality_score = float(np.mean([e.causality_strength for e in entity_events]))
                    entity.is_reactive = entity.causality_score > 0.3
                    
                    # FORENSIC: Log causal event
                    if self.forensic:
                        self.forensic.log_event('causal_event_detected', {
                            'timestamp_ns': int(event.timestamp),
                            'entity_id': int(entity.entity_id),
                            'action': event.action,
                            'distance_moved': float(event.distance_moved),
                            'causality_strength': float(event.causality_strength),
                            'causality_score_updated': float(entity.causality_score),
                            'is_reactive': bool(entity.is_reactive)
                        })
    
    def _identify_agent(self):
        """Identifie agent par score causal maximal"""
        if not self.entities:
            return
        
        # Trouver entité avec score causal maximal
        best_entity = None
        best_score = 0.0
        
        for entity in self.entities.values():
            # Score composite
            score = (
                entity.causality_score * 0.6 +  # Causalité prioritaire
                entity.mobility_score * 0.3 +   # Mobilité importante
                entity.interaction_score * 0.1  # Interaction bonus
            )
            
            if score > best_score:
                best_score = score
                best_entity = entity
        
        # Vérifier seuil
        if best_entity and best_score >= self.causality_threshold:
            self.agent_entity_id = best_entity.entity_id
            self.agent_confidence = best_score
            best_entity.is_agent_candidate = True
            
            # FORENSIC: Log agent identification
            if self.forensic:
                detection_data = {
                    'timestamp_ns': int(time.time_ns()),
                    'agent_id': int(self.agent_entity_id),
                    'confidence': float(self.agent_confidence),
                    'causality_score': float(best_entity.causality_score),
                    'mobility_score': float(best_entity.mobility_score),
                    'causal_events_count': len([e for e in self.causal_events if e.entity_id == self.agent_entity_id]),
                    'composite_score': float(best_score),
                    'threshold': float(self.causality_threshold),
                    'position': list(best_entity.positions[-1]) if best_entity.positions else None
                }
                self.forensic.log_event('agent_identified', detection_data)
    
    def _get_agent_position(self) -> Optional[Tuple[int, int]]:
        """Retourne position actuelle de l'agent"""
        if self.agent_entity_id is None:
            return None
        
        entity = self.entities.get(self.agent_entity_id)
        if entity and entity.positions:
            return entity.positions[-1]
        
        return None
    
    def _count_mobile_entities(self) -> int:
        """Compte entités mobiles"""
        return sum(1 for e in self.entities.values() if e.is_mobile)
    
    def _count_stable_entities(self) -> int:
        """Compte entités stables"""
        return sum(1 for e in self.entities.values() if e.is_stable)
    
    def _count_reactive_entities(self) -> int:
        """Compte entités réactives"""
        return sum(1 for e in self.entities.values() if e.is_reactive)
    
    def get_agent_info(self) -> Dict:
        """Retourne informations complètes sur l'agent détecté"""
        if self.agent_entity_id is None:
            return {
                'detected': False,
                'confidence': 0.0
            }
        
        entity = self.entities.get(self.agent_entity_id)
        if not entity:
            return {
                'detected': False,
                'confidence': 0.0
            }
        
        return {
            'detected': True,
            'entity_id': entity.entity_id,
            'confidence': self.agent_confidence,
            'current_position': entity.positions[-1] if entity.positions else None,
            'trajectory': entity.positions[-10:],  # 10 dernières positions
            'causality_score': entity.causality_score,
            'mobility_score': entity.mobility_score,
            'persistence_score': entity.persistence_score,
            'causal_events_count': len([e for e in self.causal_events if e.entity_id == self.agent_entity_id]),
            'first_seen': entity.first_seen,
            'last_seen': entity.last_seen
        }
    
    def get_world_model(self) -> Dict:
        """Retourne modèle complet du monde"""
        return {
            'entities': {
                eid: {
                    'id': e.entity_id,
                    'position': e.positions[-1] if e.positions else None,
                    'is_agent': eid == self.agent_entity_id,
                    'is_mobile': e.is_mobile,
                    'is_stable': e.is_stable,
                    'is_reactive': e.is_reactive,
                    'causality_score': e.causality_score,
                    'mobility_score': e.mobility_score
                }
                for eid, e in self.entities.items()
            },
            'agent_detected': self.agent_entity_id is not None,
            'agent_id': self.agent_entity_id,
            'agent_confidence': self.agent_confidence,
            'total_observations': self.total_observations,
            'causal_events_count': len(self.causal_events)
        }
    
    def reset(self):
        """Réinitialise détecteur pour nouveau jeu"""
        # FORENSIC: Log reset
        if self.forensic:
            self.forensic.log_event('detector_reset', {
                'timestamp_ns': int(time.time_ns()),
                'entities_before': len(self.entities),
                'causal_events_before': len(self.causal_events),
                'total_observations_before': self.total_observations
            })
        
        self.entities.clear()
        self.causal_events.clear()
        self.action_history.clear()
        self.grid_history.clear()
        self.agent_entity_id = None
        self.agent_confidence = 0.0
        self.total_observations = 0

# Made with Bob
