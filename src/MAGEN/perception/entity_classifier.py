#!/usr/bin/env python3
"""
Entity Classifier - Classification entités (STATIC/DYNAMIC/HAZARD/TIMER)
Inspiré par observation: "Je fais la différence entre mur (statique) et ennemi (dynamique)"
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from enum import Enum
from dataclasses import dataclass


class EntityType(Enum):
    """Types d'entités"""
    STATIC = "static"        # Murs, obstacles fixes
    DYNAMIC = "dynamic"      # Ennemis mobiles, objets qui bougent
    HAZARD = "hazard"        # Dangers (pièges, zones mortelles)
    TIMER = "timer"          # Contraintes temporelles
    PLAYER = "player"        # Joueur/agent
    GOAL = "goal"            # Objectif
    UNKNOWN = "unknown"      # Non classifié


@dataclass
class Entity:
    """Entité détectée"""
    entity_type: EntityType
    position: Tuple[int, int]
    color: int
    size: int
    is_moving: bool = False
    velocity: Optional[Tuple[int, int]] = None
    danger_level: float = 0.0


class EntityClassifier:
    """
    Classificateur d'entités pour raisonnement structurel
    
    Inspiré par comportement humain:
    - "Le mur reste statique"
    - "L'ennemi bouge quand je fais une action"
    - "Si il me touche je perds"
    """
    
    def __init__(self):
        # Historique états pour détecter mouvement
        self.state_history: List[np.ndarray] = []
        self.max_history = 10
        
        # Entités détectées
        self.entities: Dict[Tuple[int, int], Entity] = {}
        
        # Statistiques
        self.total_classifications = 0
    
    def update_state(self, grid: np.ndarray):
        """
        Met à jour historique états pour détection mouvement
        
        Args:
            grid: Grille actuelle
        """
        self.state_history.append(grid.copy())
        
        # Limiter historique
        if len(self.state_history) > self.max_history:
            self.state_history.pop(0)
    
    def classify_grid(self, grid: np.ndarray) -> Dict[Tuple[int, int], Entity]:
        """
        Classifie toutes les entités dans une grille
        
        Args:
            grid: Grille à analyser
            
        Returns:
            Dict position -> Entity
        """
        self.total_classifications += 1
        entities = {}
        
        # Détecter toutes les positions non-vides
        non_zero = np.argwhere(grid != 0)
        
        for pos in non_zero:
            y, x = int(pos[0]), int(pos[1])
            color = int(grid[y, x])
            
            # Classifier entité
            entity = self._classify_entity(grid, (y, x), color)
            entities[(y, x)] = entity
        
        self.entities = entities
        return entities
    
    def _classify_entity(self, 
                        grid: np.ndarray,
                        position: Tuple[int, int],
                        color: int) -> Entity:
        """
        Classifie une entité individuelle
        
        Args:
            grid: Grille complète
            position: Position entité
            color: Couleur entité
            
        Returns:
            Entity classifiée
        """
        y, x = position
        
        # Détecter si entité bouge
        is_moving, velocity = self._detect_movement(position, color)
        
        # Détecter taille (région connectée)
        size = self._compute_entity_size(grid, position, color)
        
        # Classifier type basé sur caractéristiques
        entity_type = self._infer_entity_type(
            color, size, is_moving, velocity
        )
        
        # Estimer niveau danger
        danger_level = self._estimate_danger_level(
            entity_type, is_moving, velocity
        )
        
        return Entity(
            entity_type=entity_type,
            position=position,
            color=color,
            size=size,
            is_moving=is_moving,
            velocity=velocity,
            danger_level=danger_level
        )
    
    def _detect_movement(self,
                        position: Tuple[int, int],
                        color: int) -> Tuple[bool, Optional[Tuple[int, int]]]:
        """
        Détecte si entité bouge en comparant historique
        
        Returns:
            (is_moving, velocity)
        """
        if len(self.state_history) < 2:
            return False, None
        
        # Comparer 2 derniers états
        prev_grid = self.state_history[-2]
        curr_grid = self.state_history[-1]
        
        y, x = position
        
        # Vérifier si couleur présente dans état précédent
        prev_positions = np.argwhere(prev_grid == color)
        
        if len(prev_positions) == 0:
            # Nouvelle entité
            return False, None
        
        # Trouver position la plus proche dans état précédent
        distances = [abs(py - y) + abs(px - x) for py, px in prev_positions]
        min_idx = np.argmin(distances)
        prev_y, prev_x = prev_positions[min_idx]
        
        # Calculer vélocité
        dy = y - prev_y
        dx = x - prev_x
        
        if dy != 0 or dx != 0:
            return True, (dy, dx)
        
        return False, None
    
    def _compute_entity_size(self,
                            grid: np.ndarray,
                            position: Tuple[int, int],
                            color: int) -> int:
        """
        Calcule taille entité (région connectée)
        
        Returns:
            Nombre de pixels connectés
        """
        y, x = position
        
        # Flood fill simple pour compter pixels connectés
        visited = set()
        stack = [position]
        count = 0
        
        while stack and count < 1000:  # Limite sécurité
            y, x = stack.pop()
            
            if (y, x) in visited:
                continue
            
            if y < 0 or y >= grid.shape[0] or x < 0 or x >= grid.shape[1]:
                continue
            
            if grid[y, x] != color:
                continue
            
            visited.add((y, x))
            count += 1
            
            # Ajouter voisins
            stack.extend([(y+1, x), (y-1, x), (y, x+1), (y, x-1)])
        
        return count
    
        return count
    
    def _infer_entity_type(self,
                          color: int,
                          size: int,
                          is_moving: bool,
                          velocity: Optional[Tuple[int, int]]) -> EntityType:
        """
        Infère type entité basé sur caractéristiques
        
        Heuristiques:
        - Grande taille + statique = STATIC (mur)
        - Petite taille + mobile = DYNAMIC (ennemi)
        - Mobile + rapide = HAZARD (danger)
        """
        if is_moving:
            if velocity and (abs(velocity[0]) > 1 or abs(velocity[1]) > 1):
                return EntityType.HAZARD  # Rapide = dangereux
            else:
                return EntityType.DYNAMIC  # Mobile normal
        else:
            if size > 10:
                return EntityType.STATIC  # Grande structure = mur
            elif size == 1:
                return EntityType.UNKNOWN  # Pixel isolé
            else:
                return EntityType.STATIC  # Petit obstacle
    
    def _estimate_danger_level(self,
                               entity_type: EntityType,
                               is_moving: bool,
                               velocity: Optional[Tuple[int, int]]) -> float:
        """
        Estime niveau de danger (0.0 = sûr, 1.0 = très dangereux)
        """
        if entity_type == EntityType.HAZARD:
            return 1.0
        elif entity_type == EntityType.DYNAMIC:
            if velocity:
                speed = abs(velocity[0]) + abs(velocity[1])
                return min(0.8, 0.3 + speed * 0.1)
            return 0.5
        elif entity_type == EntityType.STATIC:
            return 0.1  # Obstacles évitables
        else:
            return 0.0
    
    def get_entities_by_type(self, entity_type: EntityType) -> List[Entity]:
        """Retourne toutes les entités d'un type donné"""
        return [e for e in self.entities.values() if e.entity_type == entity_type]
    
    def get_dangerous_entities(self, threshold: float = 0.5) -> List[Entity]:
        """Retourne entités dangereuses"""
        return [e for e in self.entities.values() if e.danger_level >= threshold]
    
    def get_static_obstacles(self) -> List[Entity]:
        """Retourne obstacles statiques (murs)"""
        return self.get_entities_by_type(EntityType.STATIC)
    
    def get_dynamic_threats(self) -> List[Entity]:
        """Retourne menaces dynamiques (ennemis)"""
        return self.get_entities_by_type(EntityType.DYNAMIC)
    
    def get_stats(self) -> Dict:
        """Statistiques classification"""
        type_counts = {}
        for entity in self.entities.values():
            type_name = entity.entity_type.value
            type_counts[type_name] = type_counts.get(type_name, 0) + 1
        
        return {
            'total_classifications': self.total_classifications,
            'entities_detected': len(self.entities),
            'type_distribution': type_counts,
            'moving_entities': sum(1 for e in self.entities.values() if e.is_moving),
            'dangerous_entities': len(self.get_dangerous_entities())
        }


# Exemple d'utilisation
if __name__ == "__main__":
    classifier = EntityClassifier()
    
    # Simuler 2 états successifs
    grid1 = np.array([
        [0, 0, 0, 0, 0],
        [1, 1, 1, 1, 1],  # Mur statique
        [0, 0, 2, 0, 0],  # Ennemi position 1
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0]
    ])
    
    grid2 = np.array([
        [0, 0, 0, 0, 0],
        [1, 1, 1, 1, 1],  # Mur statique (même position)
        [0, 0, 0, 2, 0],  # Ennemi position 2 (bougé)
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0]
    ])
    
    # Classifier
    classifier.update_state(grid1)
    entities1 = classifier.classify_grid(grid1)
    
    classifier.update_state(grid2)
    entities2 = classifier.classify_grid(grid2)
    
    print("Entités détectées:")
    for pos, entity in entities2.items():
        print(f"  Position {pos}: {entity.entity_type.value}, "
              f"moving={entity.is_moving}, danger={entity.danger_level:.2f}")
    
    print(f"\nStats: {classifier.get_stats()}")

# Made with Bob
