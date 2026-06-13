"""
MAGEN V26 - Semantic Transformation Engine
Primitives sémantiques orientées objets/règles/patterns

OBJECTIF: Compléter l'espace transformationnel avec primitives sémantiques
au lieu de uniquement géométriques (rotate, mirror, flip)
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional, Callable
from dataclasses import dataclass
from scipy import ndimage
from collections import defaultdict


@dataclass
class SemanticContext:
    """Contexte sémantique d'une grille"""
    objects: List[np.ndarray]  # Objets connectés détectés
    colors: List[int]  # Couleurs présentes
    patterns: List[np.ndarray]  # Patterns répétés détectés
    symmetries: List[str]  # Symétries détectées
    grid_properties: Dict[str, Any]  # Propriétés globales


class SemanticTransformationEngine:
    """
    Moteur de transformations sémantiques
    Complète TransformationLearningEngine avec primitives orientées objets/règles
    """
    
    def __init__(self, verbose: bool = False, forensic_logger=None):
        self.verbose = verbose
        self.forensic_logger = forensic_logger
        
        # Registres primitives par catégorie
        self.object_primitives: Dict[str, Dict[str, Any]] = {}
        self.pattern_primitives: Dict[str, Dict[str, Any]] = {}
        self.rule_primitives: Dict[str, Dict[str, Any]] = {}
        self.spatial_primitives: Dict[str, Dict[str, Any]] = {}
        
        # Initialiser primitives
        self._initialize_semantic_primitives()
    
    def _initialize_semantic_primitives(self):
        """Initialiser toutes les primitives sémantiques"""
        
        # PRIORITÉ HIGH - Object-Based
        self._register_primitive('remove_object_by_color', 
                                 self._remove_object_by_color, 
                                 'object', priority='HIGH')
        
        self._register_primitive('keep_largest_object',
                                 self._keep_largest_object,
                                 'object', priority='HIGH')
        
        self._register_primitive('extract_objects',
                                 self._extract_objects,
                                 'object', priority='HIGH')
        
        # PRIORITÉ HIGH - Pattern-Based
        self._register_primitive('replicate_pattern',
                                 self._replicate_pattern,
                                 'pattern', priority='HIGH')
        
        self._register_primitive('tile_pattern',
                                 self._tile_pattern,
                                 'pattern', priority='MEDIUM')
        
        # PRIORITÉ HIGH - Color-Based  
        self._register_primitive('propagate_color',
                                 self._propagate_color,
                                 'rule', priority='HIGH')
        
        self._register_primitive('flood_fill',
                                 self._flood_fill,
                                 'rule', priority='MEDIUM')
        
        # PRIORITÉ HIGH - Spatial-Based
        self._register_primitive('expand_until_boundary',
                                 self._expand_until_boundary,
                                 'spatial', priority='HIGH')
        
        self._register_primitive('connect_components',
                                 self._connect_components,
                                 'spatial', priority='MEDIUM')
        
        # PRIORITÉ HIGH - Rule-Based
        self._register_primitive('conditional_replace',
                                 self._conditional_replace,
                                 'rule', priority='HIGH')
        
        self._register_primitive('count_and_generate',
                                 self._count_and_generate,
                                 'rule', priority='MEDIUM')
        
        if self.verbose:
            print(f"[SEMANTIC] {len(self.object_primitives)} object primitives")
            print(f"[SEMANTIC] {len(self.pattern_primitives)} pattern primitives")
            print(f"[SEMANTIC] {len(self.rule_primitives)} rule primitives")
            print(f"[SEMANTIC] {len(self.spatial_primitives)} spatial primitives")
    
    def _register_primitive(self, name: str, func: Callable, category: str, priority: str = 'MEDIUM'):
        """Enregistrer une primitive sémantique"""
        if category == 'object':
            self.object_primitives[name] = {'func': func, 'priority': priority}
        elif category == 'pattern':
            self.pattern_primitives[name] = {'func': func, 'priority': priority}
        elif category == 'rule':
            self.rule_primitives[name] = {'func': func, 'priority': priority}
        elif category == 'spatial':
            self.spatial_primitives[name] = {'func': func, 'priority': priority}
    
    # ========== OBJECT-BASED PRIMITIVES ==========
    
    def _remove_object_by_color(self, grid: np.ndarray, color: int) -> np.ndarray:
        """Supprimer tous les objets d'une couleur donnée"""
        result = grid.copy()
        result[result == color] = 0
        return result
    
    def _keep_largest_object(self, grid: np.ndarray) -> np.ndarray:
        """Garder uniquement le plus grand objet connecté"""
        if grid.size == 0:
            return grid.copy()
        
        # Détecter objets connectés pour chaque couleur non-nulle
        result = np.zeros_like(grid)
        colors = np.unique(grid)
        colors = colors[colors != 0]
        
        largest_size = 0
        largest_object = None
        
        for color in colors:
            mask = (grid == color)
            labeled, num_features = ndimage.label(mask)
            
            for i in range(1, num_features + 1):
                obj_mask = (labeled == i)
                obj_size = np.sum(obj_mask)
                
                if obj_size > largest_size:
                    largest_size = obj_size
                    largest_object = obj_mask
        
        if largest_object is not None:
            result[largest_object] = grid[largest_object]
        
        return result
    
    def _extract_objects(self, grid: np.ndarray) -> List[np.ndarray]:
        """Extraire tous les objets connectés"""
        objects = []
        colors = np.unique(grid)
        colors = colors[colors != 0]
        
        for color in colors:
            mask = (grid == color)
            labeled, num_features = ndimage.label(mask)
            
            for i in range(1, num_features + 1):
                obj_mask = (labeled == i)
                obj = np.zeros_like(grid)
                obj[obj_mask] = color
                objects.append(obj)
        
        return objects
    
    # ========== PATTERN-BASED PRIMITIVES ==========
    
    def _replicate_pattern(self, grid: np.ndarray, pattern: Optional[np.ndarray] = None) -> np.ndarray:
        """Répliquer un pattern détecté"""
        if pattern is None:
            # Détecter pattern automatiquement (pattern = plus petit objet non-nul)
            objects = self._extract_objects(grid)
            if not objects:
                return grid.copy()
            
            # Trouver plus petit objet
            pattern = min(objects, key=lambda obj: np.sum(obj != 0))
        
        # Répliquer pattern sur toute la grille
        result = grid.copy()
        pattern_h, pattern_w = pattern.shape
        
        for i in range(0, grid.shape[0], pattern_h):
            for j in range(0, grid.shape[1], pattern_w):
                h_end = min(i + pattern_h, grid.shape[0])
                w_end = min(j + pattern_w, grid.shape[1])
                
                result[i:h_end, j:w_end] = pattern[:h_end-i, :w_end-j]
        
        return result
    
    def _tile_pattern(self, grid: np.ndarray, pattern: Optional[np.ndarray] = None) -> np.ndarray:
        """Paver la grille avec un pattern"""
        # Similaire à replicate_pattern mais avec alignement strict
        return self._replicate_pattern(grid, pattern)
    
    # ========== COLOR-BASED PRIMITIVES ==========
    
    def _propagate_color(self, grid: np.ndarray, source_color: int, target_color: int) -> np.ndarray:
        """Propager une couleur selon règle de voisinage"""
        result = grid.copy()
        
        # Trouver cellules source
        source_mask = (grid == source_color)
        
        # Propager aux voisins de target_color
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if grid[i, j] == target_color:
                    # Vérifier voisins
                    neighbors = []
                    if i > 0: neighbors.append(grid[i-1, j])
                    if i < grid.shape[0]-1: neighbors.append(grid[i+1, j])
                    if j > 0: neighbors.append(grid[i, j-1])
                    if j < grid.shape[1]-1: neighbors.append(grid[i, j+1])
                    
                    if source_color in neighbors:
                        result[i, j] = source_color
        
        return result
    
    def _flood_fill(self, grid: np.ndarray, start_pos: Tuple[int, int], new_color: int) -> np.ndarray:
        """Remplissage par diffusion depuis une position"""
        result = grid.copy()
        
        if start_pos[0] < 0 or start_pos[0] >= grid.shape[0]:
            return result
        if start_pos[1] < 0 or start_pos[1] >= grid.shape[1]:
            return result
        
        old_color = grid[start_pos]
        if old_color == new_color:
            return result
        
        # Flood fill récursif
        stack = [start_pos]
        visited = set()
        
        while stack:
            i, j = stack.pop()
            
            if (i, j) in visited:
                continue
            if i < 0 or i >= grid.shape[0] or j < 0 or j >= grid.shape[1]:
                continue
            if result[i, j] != old_color:
                continue
            
            visited.add((i, j))
            result[i, j] = new_color
            
            # Ajouter voisins
            stack.extend([(i-1, j), (i+1, j), (i, j-1), (i, j+1)])
        
        return result
    
    # ========== SPATIAL-BASED PRIMITIVES ==========
    
    def _expand_until_boundary(self, grid: np.ndarray, color: int) -> np.ndarray:
        """Étendre une couleur jusqu'aux frontières"""
        result = grid.copy()
        
        # Trouver toutes les cellules de cette couleur
        mask = (grid == color)
        
        # Étendre dans toutes les directions jusqu'à rencontrer autre couleur
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if mask[i, j]:
                    # Étendre horizontalement
                    left = j
                    while left > 0 and result[i, left-1] == 0:
                        result[i, left-1] = color
                        left -= 1
                    
                    right = j
                    while right < grid.shape[1]-1 and result[i, right+1] == 0:
                        result[i, right+1] = color
                        right += 1
                    
                    # Étendre verticalement
                    up = i
                    while up > 0 and result[up-1, j] == 0:
                        result[up-1, j] = color
                        up -= 1
                    
                    down = i
                    while down < grid.shape[0]-1 and result[down+1, j] == 0:
                        result[down+1, j] = color
                        down += 1
        
        return result
    
    def _connect_components(self, grid: np.ndarray, color: int) -> np.ndarray:
        """Connecter tous les composants d'une couleur"""
        result = grid.copy()
        
        # Trouver tous les composants
        mask = (grid == color)
        labeled, num_features = ndimage.label(mask)
        
        if num_features <= 1:
            return result
        
        # Trouver centres de masse de chaque composant
        centers = ndimage.center_of_mass(mask, labeled, range(1, num_features+1))
        
        # Connecter chaque paire de centres
        for i in range(len(centers)):
            for j in range(i+1, len(centers)):
                y1, x1 = int(centers[i][0]), int(centers[i][1])
                y2, x2 = int(centers[j][0]), int(centers[j][1])
                
                # Ligne de Bresenham simplifiée
                dx = abs(x2 - x1)
                dy = abs(y2 - y1)
                sx = 1 if x1 < x2 else -1
                sy = 1 if y1 < y2 else -1
                err = dx - dy
                
                x, y = x1, y1
                while True:
                    if 0 <= y < grid.shape[0] and 0 <= x < grid.shape[1]:
                        result[y, x] = color
                    
                    if x == x2 and y == y2:
                        break
                    
                    e2 = 2 * err
                    if e2 > -dy:
                        err -= dy
                        x += sx
                    if e2 < dx:
                        err += dx
                        y += sy
        
        return result
    
    # ========== RULE-BASED PRIMITIVES ==========
    
    def _conditional_replace(self, grid: np.ndarray, condition_color: int, replacement_color: int) -> np.ndarray:
        """Remplacer une couleur par une autre si condition"""
        result = grid.copy()
        result[result == condition_color] = replacement_color
        return result
    
    def _count_and_generate(self, grid: np.ndarray) -> np.ndarray:
        """Compter objets et générer pattern basé sur compte"""
        objects = self._extract_objects(grid)
        count = len(objects)
        
        # Générer grille avec 'count' objets
        result = np.zeros_like(grid)
        
        if count > 0 and objects:
            # Répliquer premier objet 'count' fois
            pattern = objects[0]
            pattern_h, pattern_w = pattern.shape
            
            for i in range(min(count, grid.shape[0] // pattern_h)):
                y_pos = i * pattern_h
                if y_pos + pattern_h <= grid.shape[0]:
                    result[y_pos:y_pos+pattern_h, :pattern_w] = pattern[:, :pattern_w]
        
        return result
    
    # ========== CONTEXT INFERENCE ==========
    
    def infer_semantic_context(self, grid: np.ndarray) -> SemanticContext:
        """Inférer contexte sémantique d'une grille"""
        # Extraire objets
        objects = self._extract_objects(grid)
        
        # Extraire couleurs
        colors = list(np.unique(grid))
        colors = [c for c in colors if c != 0]
        
        # Détecter patterns (simplifié)
        patterns = []
        
        # Détecter symétries
        symmetries = []
        if np.array_equal(grid, np.fliplr(grid)):
            symmetries.append('horizontal')
        if np.array_equal(grid, np.flipud(grid)):
            symmetries.append('vertical')
        
        # Propriétés globales
        grid_properties = {
            'num_objects': len(objects),
            'num_colors': len(colors),
            'density': np.count_nonzero(grid) / grid.size,
            'shape': grid.shape
        }
        
        return SemanticContext(
            objects=objects,
            colors=colors,
            patterns=patterns,
            symmetries=symmetries,
            grid_properties=grid_properties
        )
    
    def rank_semantic_actions(self, grid: np.ndarray, context: SemanticContext) -> List[Tuple[str, float]]:
        """Ranking causal des actions sémantiques basé sur contexte"""
        rankings = []
        
        # Scoring basé sur contexte
        num_objects = context.grid_properties['num_objects']
        num_colors = context.grid_properties['num_colors']
        density = context.grid_properties['density']
        
        # Object-based actions
        if num_objects > 1:
            rankings.append(('keep_largest_object', 0.8))
            rankings.append(('connect_components', 0.7))
        
        if num_colors > 1:
            rankings.append(('remove_object_by_color', 0.7))
            rankings.append(('propagate_color', 0.6))
        
        # Pattern-based actions
        if num_objects > 0:
            rankings.append(('replicate_pattern', 0.6))
            rankings.append(('tile_pattern', 0.5))
        
        # Spatial actions
        if density < 0.5:
            rankings.append(('expand_until_boundary', 0.7))
            rankings.append(('flood_fill', 0.6))
        
        # Rule-based actions
        rankings.append(('conditional_replace', 0.5))
        rankings.append(('count_and_generate', 0.4))
        
        # Trier par score décroissant
        rankings.sort(key=lambda x: x[1], reverse=True)
        
        return rankings


# Export
__all__ = ['SemanticTransformationEngine', 'SemanticContext']

# Made with Bob - LumVorax Semantic Technology