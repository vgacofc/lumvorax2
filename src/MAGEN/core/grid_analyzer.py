"""
MAGEN Grid Analyzer - Analyse Intelligente Grilles ARC-AGI-3
=============================================================

OBJECTIF: Transformer perception "cognitivement aveugle" en perception structurée
pour jeux arcade interactifs.

PROBLÈME IDENTIFIÉ (Session 70):
- Agent V25 score 0/3 sur jeux arcade
- Stratégie aléatoire baseline inefficace
- Pas d'analyse grille → Actions aveugles
- 182-250% actions baseline sans compléter niveau 1

SOLUTION: GridAnalyzer avec détection patterns visuels + heuristiques gameplay

Architecture:
1. Détection entités (joueur, objectif, obstacles, ennemis)
2. Analyse spatiale (distances, chemins, zones)
3. Heuristiques gameplay (pathfinding, évitement, progression)
4. Suggestion actions intelligentes

Protocole: CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN v2.0
Auteur: Bob (Mode Code)
Date: 2026-06-14
"""

import numpy as np
from typing import List, Dict, Optional, Tuple, Set
from dataclasses import dataclass
from collections import deque
import heapq


@dataclass
class GridEntity:
    """Entité détectée dans la grille"""
    entity_type: str  # 'player', 'goal', 'obstacle', 'enemy', 'collectible', 'unknown'
    position: Tuple[int, int]  # (row, col)
    color: int
    confidence: float
    metadata: Dict


@dataclass
class SpatialAnalysis:
    """Analyse spatiale de la grille"""
    player_position: Optional[Tuple[int, int]]
    goal_position: Optional[Tuple[int, int]]
    obstacles: List[Tuple[int, int]]
    enemies: List[Tuple[int, int]]
    collectibles: List[Tuple[int, int]]
    distance_to_goal: float
    path_to_goal: Optional[List[Tuple[int, int]]]
    safe_zones: List[Tuple[int, int]]
    danger_zones: List[Tuple[int, int]]


@dataclass
class ActionSuggestion:
    """Suggestion d'action avec justification"""
    action_name: str  # 'move_up', 'move_down', 'move_left', 'move_right', 'interact', etc.
    score: float  # 0.0-1.0
    justification: str
    expected_outcome: str
    risk_level: float  # 0.0-1.0


class GridAnalyzer:
    """
    Analyseur intelligent de grilles pour jeux ARC-AGI-3
    
    Fonctionnalités:
    - Détection automatique entités (joueur, objectif, obstacles)
    - Pathfinding A* pour navigation optimale
    - Heuristiques gameplay (évitement, progression, collecte)
    - Suggestions actions intelligentes
    """
    
    def __init__(self):
        # Patterns de détection (à affiner par apprentissage)
        self.player_colors = {1, 2, 3}  # Couleurs typiques joueur
        self.goal_colors = {4, 5, 6}    # Couleurs typiques objectif
        self.obstacle_colors = {7, 8, 9}  # Couleurs typiques obstacles
        
        # Historique pour détection mouvement
        self.previous_grids: deque = deque(maxlen=5)
        self.detected_player_positions: deque = deque(maxlen=10)
        
        # Statistiques apprentissage
        self.entity_detection_stats = {
            'player_detections': 0,
            'goal_detections': 0,
            'obstacle_detections': 0,
            'false_positives': 0
        }
    
    def analyze_grid(self, grid: np.ndarray) -> SpatialAnalysis:
        """
        Analyse complète de la grille
        
        Args:
            grid: Grille 64x64 RGB ou grille couleurs
            
        Returns:
            SpatialAnalysis avec toutes les informations détectées
        """
        # Normaliser grille (RGB → couleurs si nécessaire)
        if len(grid.shape) == 3:
            grid = self._rgb_to_colors(grid)
        
        # Détecter entités
        entities = self._detect_entities(grid)
        
        # Extraire positions par type
        player_pos = self._find_entity_position(entities, 'player')
        goal_pos = self._find_entity_position(entities, 'goal')
        obstacles = self._find_entity_positions(entities, 'obstacle')
        enemies = self._find_entity_positions(entities, 'enemy')
        collectibles = self._find_entity_positions(entities, 'collectible')
        
        # Calculer distance et chemin vers objectif
        distance_to_goal = float('inf')
        path_to_goal = None
        if player_pos and goal_pos:
            path_to_goal = self._find_path_astar(grid, player_pos, goal_pos, obstacles)
            if path_to_goal:
                distance_to_goal = len(path_to_goal)
        
        # Identifier zones sûres et dangereuses
        safe_zones = self._identify_safe_zones(grid, enemies, obstacles)
        danger_zones = self._identify_danger_zones(grid, enemies)
        
        # Mettre à jour historique
        self.previous_grids.append(grid.copy())
        if player_pos:
            self.detected_player_positions.append(player_pos)
        
        return SpatialAnalysis(
            player_position=player_pos,
            goal_position=goal_pos,
            obstacles=obstacles,
            enemies=enemies,
            collectibles=collectibles,
            distance_to_goal=distance_to_goal,
            path_to_goal=path_to_goal,
            safe_zones=safe_zones,
            danger_zones=danger_zones
        )
    
    def suggest_action(self, spatial_analysis: SpatialAnalysis) -> ActionSuggestion:
        """
        Suggère la meilleure action basée sur l'analyse spatiale
        
        Args:
            spatial_analysis: Résultat de analyze_grid()
            
        Returns:
            ActionSuggestion avec action recommandée
        """
        player_pos = spatial_analysis.player_position
        goal_pos = spatial_analysis.goal_position
        path = spatial_analysis.path_to_goal
        
        # Cas 1: Pas de joueur détecté → Exploration
        if not player_pos:
            return ActionSuggestion(
                action_name='move_right',
                score=0.5,
                justification="Joueur non détecté, exploration par défaut",
                expected_outcome="Découvrir position joueur",
                risk_level=0.3
            )
        
        # Cas 2: Pas d'objectif détecté → Exploration
        if not goal_pos:
            return ActionSuggestion(
                action_name='move_up',
                score=0.5,
                justification="Objectif non détecté, exploration",
                expected_outcome="Découvrir objectif",
                risk_level=0.3
            )
        
        # Cas 3: Chemin vers objectif trouvé → Suivre chemin
        if path and len(path) > 1:
            next_pos = path[1]  # path[0] = position actuelle
            action = self._position_to_action(player_pos, next_pos)
            
            # Vérifier danger sur chemin
            risk = self._calculate_risk(next_pos, spatial_analysis.danger_zones)
            
            return ActionSuggestion(
                action_name=action,
                score=0.9 - risk * 0.3,
                justification=f"Suivre chemin optimal vers objectif ({len(path)} steps)",
                expected_outcome="Progression vers objectif",
                risk_level=risk
            )
        
        # Cas 4: Pas de chemin → Se rapprocher de l'objectif
        if goal_pos:
            action = self._move_towards(player_pos, goal_pos)
            return ActionSuggestion(
                action_name=action,
                score=0.7,
                justification="Pas de chemin direct, se rapprocher de l'objectif",
                expected_outcome="Réduire distance à l'objectif",
                risk_level=0.5
            )
        
        # Cas 5: Fallback → Action aléatoire
        return ActionSuggestion(
            action_name='move_right',
            score=0.3,
            justification="Situation ambiguë, action par défaut",
            expected_outcome="Exploration",
            risk_level=0.5
        )
    
    def _detect_entities(self, grid: np.ndarray) -> List[GridEntity]:
        """Détecte toutes les entités dans la grille"""
        entities = []
        rows, cols = grid.shape
        
        # Compter occurrences de chaque couleur
        color_counts = {}
        for color in np.unique(grid):
            if color == 0:  # Ignorer background
                continue
            color_counts[color] = np.sum(grid == color)
        
        # Heuristique: Joueur = couleur rare (1-5 pixels)
        # Objectif = couleur rare (1-5 pixels)
        # Obstacles = couleur fréquente (>5 pixels)
        
        # Détecter d'abord les couleurs rares (joueur/objectif)
        rare_colors = {c: cnt for c, cnt in color_counts.items() if 1 <= cnt <= 5}
        frequent_colors = {c: cnt for c, cnt in color_counts.items() if cnt > 5}
        
        for row in range(rows):
            for col in range(cols):
                color = grid[row, col]
                if color == 0:
                    continue
                
                count = color_counts.get(color, 0)
                
                # Classification basée sur fréquence
                if color in rare_colors:
                    # Rare → Joueur ou objectif
                    if color in self.player_colors:
                        entity_type = 'player'
                        confidence = 0.7
                    elif color in self.goal_colors:
                        entity_type = 'goal'
                        confidence = 0.7
                    else:
                        # Première couleur rare = joueur, deuxième = objectif
                        sorted_rare = sorted(rare_colors.items(), key=lambda x: x[1])
                        if color == sorted_rare[0][0]:
                            entity_type = 'player'
                            confidence = 0.6
                        elif len(sorted_rare) > 1 and color == sorted_rare[1][0]:
                            entity_type = 'goal'
                            confidence = 0.6
                        else:
                            entity_type = 'collectible'
                            confidence = 0.5
                elif color in frequent_colors:
                    # Fréquent → Obstacle
                    entity_type = 'obstacle'
                    confidence = 0.6
                else:
                    entity_type = 'unknown'
                    confidence = 0.3
                
                entities.append(GridEntity(
                    entity_type=entity_type,
                    position=(row, col),
                    color=int(color),
                    confidence=confidence,
                    metadata={'count': count}
                ))
        
        return entities
    
    def _find_entity_position(self, entities: List[GridEntity], entity_type: str) -> Optional[Tuple[int, int]]:
        """Trouve la position de la première entité du type donné"""
        for entity in entities:
            if entity.entity_type == entity_type:
                return entity.position
        return None
    
    def _find_entity_positions(self, entities: List[GridEntity], entity_type: str) -> List[Tuple[int, int]]:
        """Trouve toutes les positions des entités du type donné"""
        return [entity.position for entity in entities if entity.entity_type == entity_type]
    
    def _find_path_astar(self, grid: np.ndarray, start: Tuple[int, int], goal: Tuple[int, int], 
                         obstacles: List[Tuple[int, int]]) -> Optional[List[Tuple[int, int]]]:
        """
        Pathfinding A* pour trouver chemin optimal
        
        Args:
            grid: Grille
            start: Position départ (row, col)
            goal: Position objectif (row, col)
            obstacles: Liste positions obstacles
            
        Returns:
            Liste positions formant le chemin, ou None si pas de chemin
        """
        rows, cols = grid.shape
        obstacles_set = set(obstacles)
        
        # Heuristique: Distance Manhattan
        def heuristic(pos):
            return abs(pos[0] - goal[0]) + abs(pos[1] - goal[1])
        
        # Priority queue: (f_score, position, path)
        open_set = [(heuristic(start), start, [start])]
        closed_set = set()
        
        while open_set:
            f_score, current, path = heapq.heappop(open_set)
            
            if current == goal:
                return path
            
            if current in closed_set:
                continue
            
            closed_set.add(current)
            
            # Voisins (haut, bas, gauche, droite)
            for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                neighbor = (current[0] + dr, current[1] + dc)
                
                # Vérifier limites
                if not (0 <= neighbor[0] < rows and 0 <= neighbor[1] < cols):
                    continue
                
                # Vérifier obstacles
                if neighbor in obstacles_set:
                    continue
                
                # Vérifier déjà visité
                if neighbor in closed_set:
                    continue
                
                new_path = path + [neighbor]
                new_f_score = len(new_path) + heuristic(neighbor)
                heapq.heappush(open_set, (new_f_score, neighbor, new_path))
        
        return None  # Pas de chemin trouvé
    
    def _identify_safe_zones(self, grid: np.ndarray, enemies: List[Tuple[int, int]], 
                            obstacles: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
        """Identifie les zones sûres (loin des ennemis)"""
        rows, cols = grid.shape
        safe_zones = []
        
        for row in range(rows):
            for col in range(cols):
                pos = (row, col)
                
                # Ignorer obstacles
                if pos in obstacles:
                    continue
                
                # Vérifier distance aux ennemis
                min_distance = float('inf')
                for enemy_pos in enemies:
                    distance = abs(row - enemy_pos[0]) + abs(col - enemy_pos[1])
                    min_distance = min(min_distance, distance)
                
                # Zone sûre si distance > 3
                if min_distance > 3:
                    safe_zones.append(pos)
        
        return safe_zones
    
    def _identify_danger_zones(self, grid: np.ndarray, enemies: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
        """Identifie les zones dangereuses (proches des ennemis)"""
        danger_zones = []
        
        for enemy_pos in enemies:
            # Zone de danger = 3x3 autour de l'ennemi
            for dr in range(-1, 2):
                for dc in range(-1, 2):
                    danger_pos = (enemy_pos[0] + dr, enemy_pos[1] + dc)
                    if 0 <= danger_pos[0] < grid.shape[0] and 0 <= danger_pos[1] < grid.shape[1]:
                        danger_zones.append(danger_pos)
        
        return danger_zones
    
    def _position_to_action(self, current: Tuple[int, int], next_pos: Tuple[int, int]) -> str:
        """Convertit déplacement en action"""
        dr = next_pos[0] - current[0]
        dc = next_pos[1] - current[1]
        
        if dr == -1:
            return 'move_up'
        elif dr == 1:
            return 'move_down'
        elif dc == -1:
            return 'move_left'
        elif dc == 1:
            return 'move_right'
        else:
            return 'interact'  # Même position → interaction
    
    def _move_towards(self, current: Tuple[int, int], goal: Tuple[int, int]) -> str:
        """Détermine action pour se rapprocher de l'objectif"""
        dr = goal[0] - current[0]
        dc = goal[1] - current[1]
        
        # Prioriser axe avec plus grande distance
        if abs(dr) > abs(dc):
            return 'move_down' if dr > 0 else 'move_up'
        else:
            return 'move_right' if dc > 0 else 'move_left'
    
    def _calculate_risk(self, position: Tuple[int, int], danger_zones: List[Tuple[int, int]]) -> float:
        """Calcule niveau de risque d'une position"""
        if position in danger_zones:
            return 1.0
        
        # Risque diminue avec distance aux zones dangereuses
        min_distance = float('inf')
        for danger_pos in danger_zones:
            distance = abs(position[0] - danger_pos[0]) + abs(position[1] - danger_pos[1])
            min_distance = min(min_distance, distance)
        
        if min_distance == float('inf'):
            return 0.0
        
        return max(0.0, 1.0 - min_distance / 5.0)
    
    def _rgb_to_colors(self, grid_rgb: np.ndarray) -> np.ndarray:
        """Convertit grille RGB en grille de couleurs"""
        # Simplification: Utiliser canal rouge comme couleur
        if len(grid_rgb.shape) == 3:
            return grid_rgb[:, :, 0]
        return grid_rgb
    
    def get_stats(self) -> Dict:
        """Retourne statistiques de détection"""
        return {
            'entity_detection_stats': self.entity_detection_stats,
            'grids_analyzed': len(self.previous_grids),
            'player_positions_tracked': len(self.detected_player_positions)
        }

# Made with Bob
