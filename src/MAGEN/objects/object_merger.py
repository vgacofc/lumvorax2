"""
Object Merger - CORRECTION 1
============================

Fusion d'objets adjacents pour améliorer précision extraction.

Problème identifié: Pixels ajoutés comptés comme objets séparés
Solution: Fusionner objets à distance ≤ max_distance

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot
"""

import numpy as np
from typing import List, Dict, Any, Tuple, Set
from dataclasses import dataclass


@dataclass
class BoundingBox:
    """Boîte englobante d'un objet"""
    min_row: int
    max_row: int
    min_col: int
    max_col: int
    color: int
    pixels: Set[Tuple[int, int]]
    
    def distance_to(self, other: 'BoundingBox') -> int:
        """
        Calculer distance minimale à autre objet
        
        Args:
            other: Autre objet
            
        Returns:
            Distance minimale en pixels
        """
        # Distance horizontale
        if self.max_col < other.min_col:
            h_dist = other.min_col - self.max_col
        elif other.max_col < self.min_col:
            h_dist = self.min_col - other.max_col
        else:
            h_dist = 0
        
        # Distance verticale
        if self.max_row < other.min_row:
            v_dist = other.min_row - self.max_row
        elif other.max_row < self.min_row:
            v_dist = self.min_row - other.max_row
        else:
            v_dist = 0
        
        # Distance euclidienne
        return int(np.sqrt(h_dist**2 + v_dist**2))
    
    def merge_with(self, other: 'BoundingBox') -> 'BoundingBox':
        """
        Fusionner avec autre objet
        
        Args:
            other: Objet à fusionner
            
        Returns:
            Nouvel objet fusionné
        """
        return BoundingBox(
            min_row=min(self.min_row, other.min_row),
            max_row=max(self.max_row, other.max_row),
            min_col=min(self.min_col, other.min_col),
            max_col=max(self.max_col, other.max_col),
            color=self.color,  # Garder couleur premier objet
            pixels=self.pixels | other.pixels
        )


class ObjectMerger:
    """Fusion d'objets adjacents"""
    
    def __init__(self, max_distance: int = 1, verbose: bool = False):
        """
        Initialiser merger
        
        Args:
            max_distance: Distance max pour fusion (pixels)
            verbose: Afficher logs
        """
        self.max_distance = max_distance
        self.verbose = verbose
    
    def extract_objects(self, grid: np.ndarray) -> List[BoundingBox]:
        """
        Extraire objets de la grille
        
        Args:
            grid: Grille input
            
        Returns:
            Liste objets extraits
        """
        objects = []
        visited = np.zeros_like(grid, dtype=bool)
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if grid[i, j] != 0 and not visited[i, j]:
                    # Extraire objet connecté
                    obj = self._extract_connected_object(grid, i, j, visited)
                    objects.append(obj)
        
        if self.verbose:
            print(f"[EXTRACT] {len(objects)} objets extraits")
        
        return objects
    
    def _extract_connected_object(
        self,
        grid: np.ndarray,
        start_i: int,
        start_j: int,
        visited: np.ndarray
    ) -> BoundingBox:
        """
        Extraire objet connecté par flood fill
        
        Args:
            grid: Grille
            start_i: Ligne départ
            start_j: Colonne départ
            visited: Grille pixels visités
            
        Returns:
            Objet extrait
        """
        color = grid[start_i, start_j]
        pixels = set()
        stack = [(start_i, start_j)]
        
        min_row = start_i
        max_row = start_i
        min_col = start_j
        max_col = start_j
        
        while stack:
            i, j = stack.pop()
            
            if i < 0 or i >= grid.shape[0] or j < 0 or j >= grid.shape[1]:
                continue
            
            if visited[i, j] or grid[i, j] != color:
                continue
            
            visited[i, j] = True
            pixels.add((i, j))
            
            # Mettre à jour bounding box
            min_row = min(min_row, i)
            max_row = max(max_row, i)
            min_col = min(min_col, j)
            max_col = max(max_col, j)
            
            # Ajouter voisins (4-connectivité)
            stack.extend([(i-1, j), (i+1, j), (i, j-1), (i, j+1)])
        
        return BoundingBox(
            min_row=min_row,
            max_row=max_row,
            min_col=min_col,
            max_col=max_col,
            color=color,
            pixels=pixels
        )
    
    def merge_adjacent_objects(self, objects: List[BoundingBox]) -> List[BoundingBox]:
        """
        Fusionner objets adjacents
        
        Args:
            objects: Liste objets
            
        Returns:
            Liste objets fusionnés
        """
        if not objects:
            return []
        
        if self.verbose:
            print(f"[MERGE] Fusion {len(objects)} objets (max_distance={self.max_distance})")
        
        # Trier par couleur
        by_color = {}
        for obj in objects:
            if obj.color not in by_color:
                by_color[obj.color] = []
            by_color[obj.color].append(obj)
        
        # Fusionner par couleur
        merged = []
        for color, color_objects in by_color.items():
            merged.extend(self._merge_same_color(color_objects))
        
        if self.verbose:
            print(f"[MERGE] ✓ {len(merged)} objets après fusion")
        
        return merged
    
    def _merge_same_color(self, objects: List[BoundingBox]) -> List[BoundingBox]:
        """
        Fusionner objets de même couleur
        
        Args:
            objects: Objets même couleur
            
        Returns:
            Objets fusionnés
        """
        if len(objects) <= 1:
            return objects
        
        # Union-Find pour grouper objets adjacents
        parent = list(range(len(objects)))
        
        def find(x):
            if parent[x] != x:
                parent[x] = find(parent[x])
            return parent[x]
        
        def union(x, y):
            px, py = find(x), find(y)
            if px != py:
                parent[px] = py
        
        # Trouver paires adjacentes
        for i in range(len(objects)):
            for j in range(i + 1, len(objects)):
                dist = objects[i].distance_to(objects[j])
                if dist <= self.max_distance:
                    union(i, j)
        
        # Grouper par composante
        groups = {}
        for i in range(len(objects)):
            root = find(i)
            if root not in groups:
                groups[root] = []
            groups[root].append(objects[i])
        
        # Fusionner chaque groupe
        merged = []
        for group in groups.values():
            if len(group) == 1:
                merged.append(group[0])
            else:
                # Fusionner tous objets du groupe
                result = group[0]
                for obj in group[1:]:
                    result = result.merge_with(obj)
                merged.append(result)
        
        return merged


def test_object_merger():
    """Test fusion objets"""
    print("\n" + "="*80)
    print("TEST OBJECT MERGER - CORRECTION 1")
    print("="*80)
    
    # Créer grille test: 2 objets adjacents
    grid = np.array([
        [0, 8, 0, 0],
        [0, 8, 8, 0],
        [0, 0, 0, 8],
        [0, 0, 0, 8]
    ])
    
    print("\nGrille test:")
    print(grid)
    
    # Sans fusion
    merger_no_merge = ObjectMerger(max_distance=0, verbose=True)
    objects_no_merge = merger_no_merge.extract_objects(grid)
    print(f"\nSans fusion: {len(objects_no_merge)} objets")
    
    # Avec fusion
    merger_with_merge = ObjectMerger(max_distance=1, verbose=True)
    objects_with_merge = merger_with_merge.extract_objects(grid)
    merged = merger_with_merge.merge_adjacent_objects(objects_with_merge)
    print(f"\nAvec fusion: {len(merged)} objets")
    
    print("\n" + "="*80)
    print("✓ TEST COMPLÉTÉ")
    print("="*80)


if __name__ == "__main__":
    test_object_merger()

# Made with Bob
