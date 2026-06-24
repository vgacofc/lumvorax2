"""
Self Identification - Identification Automatique de l'Avatar (Phase 4.5.1)

Objectif: Répondre à la question fondamentale "Qui suis-je ?"

Méthode:
- Observer corrélation ACTION → MOUVEMENT
- Si ACTION_DROITE et Objet_A bouge à droite → Probabilité(Objet_A = Avatar) += 1
- Après 5-10 actions: identification quasi certaine (>95% confiance)

Auteur: Bob (MAGEN Project)
Date: 2026-06-20
"""

import numpy as np
from collections import defaultdict
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from enum import IntEnum


class GameAction(IntEnum):
    """Actions du jeu (depuis ls20)"""
    ACTION1 = 1  # Mouvement (direction variable)
    ACTION2 = 2  # Horloge temporelle
    ACTION3 = 3  # Mouvement alternatif
    ACTION4 = 4  # Pousser blocs


@dataclass
class MovementObservation:
    """Observation d'un mouvement"""
    entity_id: str
    action: int
    position_before: Tuple[float, float]
    position_after: Tuple[float, float]
    displacement: Tuple[float, float]
    distance: float


class SelfIdentification:
    """
    Identification automatique de l'avatar
    
    Principe: L'avatar est l'entité dont les mouvements corrèlent
    le plus fortement avec les actions effectuées.
    
    Méthode:
    1. Pour chaque action, observer quelles entités bougent
    2. Calculer corrélation action-mouvement pour chaque entité
    3. Entité avec meilleure corrélation = avatar
    4. Confiance augmente avec nombre d'observations
    """
    
    def __init__(self):
        # Candidats avatar
        self.avatar_candidates: List[str] = []
        self.confidence_scores: Dict[str, float] = defaultdict(float)
        
        # Avatar identifié
        self.avatar_id: Optional[str] = None
        self.confidence: float = 0.0
        
        # Historique observations
        self.movement_observations: List[MovementObservation] = []
        self.action_history: List[int] = []
        
        # Statistiques par entité
        self.entity_stats: Dict[str, Dict] = defaultdict(lambda: {
            'total_movements': 0,
            'correlated_movements': 0,
            'total_actions': 0,
            'movement_directions': [],
            'action_directions': []
        })
        
        # Seuils
        self.confidence_threshold = 0.95
        self.min_observations = 5
    
    def update(self, action: int, entities_before: Dict, entities_after: Dict):
        """
        Mettre à jour identification après une action
        
        Args:
            action: Action effectuée
            entities_before: Entités avant action
            entities_after: Entités après action
        """
        self.action_history.append(action)
        
        # Détecter mouvements
        movements = self.detect_movements(entities_before, entities_after)
        
        # Analyser corrélation action-mouvement
        for movement in movements:
            self.movement_observations.append(movement)
            
            # Mettre à jour statistiques
            entity_id = movement.entity_id
            stats = self.entity_stats[entity_id]
            stats['total_movements'] += 1
            stats['total_actions'] += 1
            stats['movement_directions'].append(movement.displacement)
            
            # Vérifier corrélation avec action
            if self.is_correlated_movement(action, movement):
                stats['correlated_movements'] += 1
                self.confidence_scores[entity_id] += 1.0
            
            # Ajouter comme candidat si pas déjà présent
            if entity_id not in self.avatar_candidates:
                self.avatar_candidates.append(entity_id)
        
        # Mettre à jour identification
        self.update_identification()
    
    def detect_movements(self, entities_before: Dict, entities_after: Dict) -> List[MovementObservation]:
        """
        Détecter entités qui ont bougé
        
        Compare positions avant/après pour identifier mouvements
        """
        movements = []
        
        # Matcher entités par ID
        for entity_id in entities_before:
            if entity_id in entities_after:
                entity_before = entities_before[entity_id]
                entity_after = entities_after[entity_id]
                
                # Calculer déplacement
                pos_before = entity_before.centroid
                pos_after = entity_after.centroid
                
                displacement = (
                    pos_after[0] - pos_before[0],
                    pos_after[1] - pos_before[1]
                )
                
                distance = np.sqrt(displacement[0]**2 + displacement[1]**2)
                
                # Si mouvement significatif
                if distance > 0.5:
                    movement = MovementObservation(
                        entity_id=entity_id,
                        action=self.action_history[-1] if self.action_history else 0,
                        position_before=pos_before,
                        position_after=pos_after,
                        displacement=displacement,
                        distance=distance
                    )
                    movements.append(movement)
        
        return movements
    
    def is_correlated_movement(self, action: int, movement: MovementObservation) -> bool:
        """
        Vérifier si mouvement corrèle avec action
        
        Heuristiques:
        - ACTION1/3/4: Mouvements directionnels (haut/bas/gauche/droite)
        - ACTION2: Pas de mouvement attendu (horloge temporelle)
        
        Returns:
            True si mouvement corrèle avec action
        """
        # ACTION2 = horloge temporelle, pas de mouvement avatar attendu
        if action == GameAction.ACTION2:
            return False
        
        # ACTION1/3/4 = mouvements directionnels
        if action in [GameAction.ACTION1, GameAction.ACTION3, GameAction.ACTION4]:
            # Tout mouvement significatif est potentiellement corrélé
            # (on ne connaît pas encore la direction exacte de chaque action)
            return movement.distance > 1.0
        
        return False
    
    def update_identification(self):
        """
        Mettre à jour identification avatar
        
        Calcule confiance basée sur:
        - Nombre d'observations
        - Ratio mouvements corrélés / total mouvements
        - Consistance des mouvements
        """
        if len(self.action_history) < self.min_observations:
            return
        
        if not self.confidence_scores:
            return
        
        # Identifier entité avec meilleur score
        best_entity_id = max(self.confidence_scores.keys(), key=lambda k: self.confidence_scores[k])
        best_score = self.confidence_scores[best_entity_id]
        
        # Calculer confiance
        total_actions = len(self.action_history)
        
        # Confiance = ratio mouvements corrélés / total actions
        # (normalisé pour éviter sur-confiance avec peu d'observations)
        raw_confidence = best_score / total_actions
        
        # Ajuster confiance selon nombre d'observations
        observation_factor = min(1.0, total_actions / 20.0)
        self.confidence = raw_confidence * observation_factor
        
        # Si confiance suffisante, identifier avatar
        if self.confidence >= self.confidence_threshold:
            self.avatar_id = best_entity_id
        else:
            # Sinon, garder meilleur candidat mais pas confirmé
            self.avatar_id = None
    
    def get_avatar_id(self) -> Optional[str]:
        """
        Obtenir ID de l'avatar identifié
        
        Returns:
            ID avatar si identifié avec confiance suffisante, None sinon
        """
        return self.avatar_id
    
    def get_confidence(self) -> float:
        """
        Obtenir niveau de confiance identification
        
        Returns:
            Confiance entre 0.0 et 1.0
        """
        return float(self.confidence)
    
    def get_candidates(self) -> List[Tuple[str, float]]:
        """
        Obtenir liste candidats avec scores
        
        Returns:
            Liste (entity_id, score) triée par score décroissant
        """
        candidates = [(entity_id, score) 
                     for entity_id, score in self.confidence_scores.items()]
        candidates.sort(key=lambda x: x[1], reverse=True)
        return candidates
    
    def get_statistics(self) -> Dict:
        """
        Obtenir statistiques identification
        
        Returns:
            Dict avec statistiques détaillées
        """
        stats = {
            'avatar_id': self.avatar_id,
            'confidence': self.confidence,
            'total_observations': len(self.action_history),
            'total_movements': len(self.movement_observations),
            'candidates_count': len(self.avatar_candidates),
            'candidates': self.get_candidates()[:5],  # Top 5
            'identified': self.avatar_id is not None
        }
        
        # Ajouter stats entité identifiée
        if self.avatar_id:
            entity_stats = self.entity_stats[self.avatar_id]
            stats['avatar_stats'] = {
                'total_movements': entity_stats['total_movements'],
                'correlated_movements': entity_stats['correlated_movements'],
                'correlation_ratio': (entity_stats['correlated_movements'] / 
                                    entity_stats['total_movements'] 
                                    if entity_stats['total_movements'] > 0 else 0.0)
            }
        
        return stats
    
    def reset(self):
        """Réinitialiser identification"""
        self.avatar_candidates.clear()
        self.confidence_scores.clear()
        self.avatar_id = None
        self.confidence = 0.0
        self.movement_observations.clear()
        self.action_history.clear()
        self.entity_stats.clear()
    
    def __repr__(self):
        if self.avatar_id:
            return (f"SelfIdentification(avatar={self.avatar_id}, "
                   f"confidence={self.confidence:.2%}, "
                   f"observations={len(self.action_history)})")
        else:
            candidates = self.get_candidates()
            if candidates:
                best = candidates[0]
                return (f"SelfIdentification(best_candidate={best[0]}, "
                       f"score={best[1]:.1f}, "
                       f"confidence={self.confidence:.2%}, "
                       f"observations={len(self.action_history)})")
            else:
                return f"SelfIdentification(no_candidates, observations={len(self.action_history)})"


class DirectionalSelfIdentification(SelfIdentification):
    """
    Version avancée avec détection directionnelle
    
    Améliore identification en analysant directions précises:
    - ACTION1 + mouvement HAUT → corrélation forte
    - ACTION1 + mouvement BAS → corrélation faible
    - etc.
    
    Nécessite calibration des actions (quelle action = quelle direction)
    """
    
    def __init__(self):
        super().__init__()
        
        # Mapping action → direction (à apprendre)
        self.action_directions: Dict[int, Tuple[float, float]] = {}
        
        # Historique pour calibration
        self.calibration_data: List[Tuple[int, Tuple[float, float]]] = []
    
    def is_correlated_movement(self, action: int, movement: MovementObservation) -> bool:
        """
        Version améliorée avec directions
        
        Si direction action connue, vérifier alignement avec mouvement
        """
        # Utiliser version de base si direction inconnue
        if action not in self.action_directions:
            return super().is_correlated_movement(action, movement)
        
        # Vérifier alignement directionnel
        expected_direction = self.action_directions[action]
        actual_direction = movement.displacement
        
        # Normaliser directions
        expected_norm = np.sqrt(expected_direction[0]**2 + expected_direction[1]**2)
        actual_norm = np.sqrt(actual_direction[0]**2 + actual_direction[1]**2)
        
        if expected_norm < 0.1 or actual_norm < 0.1:
            return False
        
        expected_unit = (expected_direction[0] / expected_norm,
                        expected_direction[1] / expected_norm)
        actual_unit = (actual_direction[0] / actual_norm,
                      actual_direction[1] / actual_norm)
        
        # Produit scalaire (cosinus angle)
        dot_product = (expected_unit[0] * actual_unit[0] + 
                      expected_unit[1] * actual_unit[1])
        
        # Corrélation si angle < 45° (dot_product > 0.707)
        return dot_product > 0.707
    
    def calibrate_action_directions(self):
        """
        Calibrer directions des actions
        
        Analyse historique mouvements pour déduire direction de chaque action
        """
        if len(self.calibration_data) < 10:
            return
        
        # Grouper par action
        movements_by_action = defaultdict(list)
        for action, displacement in self.calibration_data:
            movements_by_action[action].append(displacement)
        
        # Calculer direction moyenne pour chaque action
        for action, displacements in movements_by_action.items():
            if len(displacements) >= 3:
                mean_displacement = (
                    np.mean([d[0] for d in displacements]),
                    np.mean([d[1] for d in displacements])
                )
                self.action_directions[action] = mean_displacement

# Made with Bob
