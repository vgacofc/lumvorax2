#!/usr/bin/env python3
"""
Hazard Detector - Détection menaces mobiles et prédiction trajectoires
Inspiré par observation: "Je vois l'ennemi arriver et j'évite sa trajectoire"
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass

from .entity_classifier import Entity, EntityType, EntityClassifier


@dataclass
class Threat:
    """Menace détectée"""
    entity: Entity
    predicted_positions: List[Tuple[int, int]]  # Positions futures prédites
    collision_risk: float  # 0.0 = sûr, 1.0 = collision certaine
    time_to_collision: Optional[int]  # Steps avant collision (None si pas de collision)


class HazardDetector:
    """
    Détecteur de menaces et prédicteur de trajectoires
    
    Inspiré par comportement humain:
    - "Je vois l'ennemi se déplacer vers moi"
    - "Je calcule où il sera dans 3 mouvements"
    - "J'évite sa trajectoire prédite"
    """
    
    def __init__(self, grid_shape: Tuple[int, int]):
        self.grid_shape = grid_shape
        self.height, self.width = grid_shape
        
        # Classifier entités
        self.classifier = EntityClassifier()
        
        # Menaces actives
        self.active_threats: List[Threat] = []
        
        # Statistiques
        self.total_detections = 0
        self.collisions_avoided = 0
    
    def detect_threats(self,
                      grid: np.ndarray,
                      player_position: Tuple[int, int],
                      prediction_depth: int = 5) -> List[Threat]:
        """
        Détecte toutes les menaces et prédit leurs trajectoires
        
        Args:
            grid: Grille actuelle
            player_position: Position joueur
            prediction_depth: Nombre de steps à prédire
            
        Returns:
            Liste de menaces détectées
        """
        self.total_detections += 1
        
        # Mettre à jour classifier
        self.classifier.update_state(grid)
        entities = self.classifier.classify_grid(grid)
        
        # Détecter menaces
        threats = []
        
        # Récupérer entités dangereuses
        dangerous = self.classifier.get_dangerous_entities(threshold=0.3)
        
        for entity in dangerous:
            # Prédire trajectoire
            predicted_positions = self._predict_trajectory(
                entity, prediction_depth
            )
            
            # Calculer risque collision
            collision_risk, time_to_collision = self._compute_collision_risk(
                player_position, predicted_positions
            )
            
            threat = Threat(
                entity=entity,
                predicted_positions=predicted_positions,
                collision_risk=collision_risk,
                time_to_collision=time_to_collision
            )
            
            threats.append(threat)
        
        self.active_threats = threats
        return threats
    
    def _predict_trajectory(self,
                           entity: Entity,
                           depth: int) -> List[Tuple[int, int]]:
        """
        Prédit trajectoire future d'une entité
        
        Args:
            entity: Entité à prédire
            depth: Nombre de steps
            
        Returns:
            Liste positions futures
        """
        positions = [entity.position]
        
        if not entity.is_moving or entity.velocity is None:
            # Entité statique, reste sur place
            return positions * depth
        
        # Extrapoler mouvement linéaire
        y, x = entity.position
        dy, dx = entity.velocity
        
        for step in range(1, depth + 1):
            next_y = y + dy * step
            next_x = x + dx * step
            
            # Vérifier limites grille
            if 0 <= next_y < self.height and 0 <= next_x < self.width:
                positions.append((next_y, next_x))
            else:
                # Hors limites, arrêter prédiction
                break
        
        return positions
    
    def _compute_collision_risk(self,
                               player_position: Tuple[int, int],
                               predicted_positions: List[Tuple[int, int]]) -> Tuple[float, Optional[int]]:
        """
        Calcule risque de collision avec trajectoire prédite
        
        Returns:
            (collision_risk, time_to_collision)
        """
        py, px = player_position
        
        # Vérifier collision directe
        for step, (ey, ex) in enumerate(predicted_positions):
            if (ey, ex) == (py, px):
                return 1.0, step
        
        # Calculer distance minimale
        min_distance = float('inf')
        closest_step = None
        
        for step, (ey, ex) in enumerate(predicted_positions):
            distance = abs(ey - py) + abs(ex - px)  # Manhattan distance
            if distance < min_distance:
                min_distance = distance
                closest_step = step
        
        # Risque basé sur distance
        if min_distance == 0:
            risk = 1.0
        elif min_distance == 1:
            risk = 0.8  # Adjacent = très dangereux
        elif min_distance == 2:
            risk = 0.5  # Proche
        elif min_distance <= 3:
            risk = 0.3  # Modéré
        else:
            risk = 0.1  # Loin
        
        return risk, closest_step if risk > 0.5 else None
    
    def is_position_safe(self,
                        position: Tuple[int, int],
                        time_offset: int = 0) -> Tuple[bool, Optional[str]]:
        """
        Vérifie si une position est sûre (pas de collision prédite)
        
        Args:
            position: Position à vérifier
            time_offset: Offset temporel (0 = maintenant, 1 = prochain step, etc.)
            
        Returns:
            (is_safe, reason)
        """
        y, x = position
        
        # Vérifier limites
        if y < 0 or y >= self.height or x < 0 or x >= self.width:
            return False, "out_of_bounds"
        
        # Vérifier collision avec trajectoires prédites
        for threat in self.active_threats:
            if time_offset < len(threat.predicted_positions):
                pred_y, pred_x = threat.predicted_positions[time_offset]
                
                if (pred_y, pred_x) == (y, x):
                    return False, f"collision_with_threat_at_step_{time_offset}"
                
                # Vérifier adjacence (danger)
                if abs(pred_y - y) + abs(pred_x - x) == 1:
                    if threat.entity.danger_level > 0.5:
                        return False, f"adjacent_to_threat_at_step_{time_offset}"
        
        return True, None
    
    def get_safe_moves(self,
                      current_position: Tuple[int, int],
                      possible_moves: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
        """
        Filtre mouvements pour ne garder que les sûrs
        
        Args:
            current_position: Position actuelle
            possible_moves: Liste mouvements possibles (offsets)
            
        Returns:
            Liste mouvements sûrs
        """
        cy, cx = current_position
        safe_moves = []
        
        for dy, dx in possible_moves:
            new_pos = (cy + dy, cx + dx)
            is_safe, _ = self.is_position_safe(new_pos, time_offset=1)
            
            if is_safe:
                safe_moves.append((dy, dx))
        
        return safe_moves
    
    def get_escape_direction(self,
                            current_position: Tuple[int, int]) -> Optional[Tuple[int, int]]:
        """
        Trouve direction optimale pour échapper aux menaces
        
        Returns:
            (dy, dx) direction ou None si aucune échappatoire
        """
        if not self.active_threats:
            return None
        
        cy, cx = current_position
        
        # Calculer vecteur moyen des menaces
        threat_vector_y = 0.0
        threat_vector_x = 0.0
        
        for threat in self.active_threats:
            ty, tx = threat.entity.position
            
            # Vecteur menace -> joueur
            dy = cy - ty
            dx = cx - tx
            
            # Pondérer par danger
            weight = threat.entity.danger_level
            threat_vector_y += dy * weight
            threat_vector_x += dx * weight
        
        # Normaliser et inverser (fuir dans direction opposée)
        if threat_vector_y == 0 and threat_vector_x == 0:
            return None
        
        # Direction de fuite (opposée aux menaces)
        escape_y = 1 if threat_vector_y > 0 else (-1 if threat_vector_y < 0 else 0)
        escape_x = 1 if threat_vector_x > 0 else (-1 if threat_vector_x < 0 else 0)
        
        return (escape_y, escape_x)
    
    def get_critical_threats(self, threshold: float = 0.7) -> List[Threat]:
        """Retourne menaces critiques (collision imminente)"""
        return [t for t in self.active_threats if t.collision_risk >= threshold]
    
    def should_abort_mission(self, player_position: Tuple[int, int]) -> Tuple[bool, str]:
        """
        Détermine si mission doit être abandonnée (trop dangereux)
        
        Returns:
            (should_abort, reason)
        """
        critical = self.get_critical_threats(threshold=0.8)
        
        if len(critical) >= 3:
            return True, "surrounded_by_threats"
        
        # Vérifier si aucun mouvement sûr
        possible_moves = [(0, 1), (0, -1), (1, 0), (-1, 0)]
        safe_moves = self.get_safe_moves(player_position, possible_moves)
        
        if len(safe_moves) == 0:
            return True, "no_safe_moves"
        
        return False, ""
    
    def get_stats(self) -> Dict:
        """Statistiques détection"""
        return {
            'total_detections': self.total_detections,
            'active_threats': len(self.active_threats),
            'critical_threats': len(self.get_critical_threats()),
            'collisions_avoided': self.collisions_avoided,
            'entity_stats': self.classifier.get_stats()
        }


# Exemple d'utilisation
if __name__ == "__main__":
    # Simuler grille avec ennemi mobile
    grid_shape = (10, 10)
    detector = HazardDetector(grid_shape)
    
    # État 1: Ennemi loin
    grid1 = np.zeros(grid_shape, dtype=int)
    grid1[2, 2] = 2  # Ennemi
    grid1[8, 8] = 1  # Joueur
    
    # État 2: Ennemi se rapproche
    grid2 = np.zeros(grid_shape, dtype=int)
    grid2[3, 3] = 2  # Ennemi bougé
    grid2[8, 8] = 1  # Joueur
    
    # Détecter menaces
    threats = detector.detect_threats(grid2, player_position=(8, 8), prediction_depth=5)
    
    print(f"Menaces détectées: {len(threats)}")
    for threat in threats:
        print(f"  Entité {threat.entity.entity_type.value} à {threat.entity.position}")
        print(f"  Risque collision: {threat.collision_risk:.2f}")
        print(f"  Trajectoire prédite: {threat.predicted_positions[:3]}")
    
    # Vérifier sécurité position
    is_safe, reason = detector.is_position_safe((7, 7), time_offset=1)
    print(f"\nPosition (7,7) sûre? {is_safe} ({reason})")
    
    # Direction fuite
    escape = detector.get_escape_direction((8, 8))
    print(f"Direction fuite: {escape}")
    
    print(f"\nStats: {detector.get_stats()}")

# Made with Bob
