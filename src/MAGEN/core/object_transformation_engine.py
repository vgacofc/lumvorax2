#!/usr/bin/env python3
"""
MAGEN - Object Transformation Engine
=====================================

SESSION 57 - MOTEUR TRANSFORMATIONNEL STRUCTUREL RÉEL

Insight critique utilisateur:
"ARC attend OBJET ENTIER pas PIXEL"

Ce module implémente:
1. Extraction objets complète (bbox, mask, structure)
2. Duplication structurelle (préservation forme)
3. Placement spatial intelligent (symétrie, alignement)

Protocole: CLAUDE_PILOT + LUMVORAX
Auteur: Bob (Mode Advanced)
Date: 2026-06-13
"""

import numpy as np
from typing import List, Tuple, Dict, Optional, Set
from dataclasses import dataclass
from scipy.ndimage import label, center_of_mass
from collections import defaultdict


@dataclass
class StructuredObject:
    """
    Représentation structurelle complète d'un objet ARC
    
    Contrairement à un simple pixel, capture:
    - Forme complète (tous pixels)
    - Structure spatiale (bbox, centroid)
    - Signature géométrique (pour matching)
    - Propriétés topologiques (connexité, densité)
    """
    object_id: int
    pixels: np.ndarray  # Coordonnées (N, 2) des pixels
    color: int
    bbox: Tuple[int, int, int, int]  # (min_y, min_x, max_y, max_x)
    centroid: Tuple[float, float]  # (cy, cx)
    area: int  # Nombre pixels
    density: float  # area / bbox_area
    shape_signature: str  # Hash forme pour matching
    
    def get_relative_coords(self) -> np.ndarray:
        """Coordonnées relatives au centroid"""
        cy, cx = self.centroid
        return self.pixels - np.array([cy, cx])
    
    def get_width(self) -> int:
        """Largeur objet"""
        return self.bbox[3] - self.bbox[1] + 1
    
    def get_height(self) -> int:
        """Hauteur objet"""
        return self.bbox[2] - self.bbox[0] + 1


class ObjectExtractor:
    """
    Extracteur d'objets structurels depuis grille ARC
    
    Amélioration critique vs version naïve:
    - Extraction COMPLÈTE (pas juste 1 pixel)
    - Préservation structure spatiale
    - Calcul propriétés géométriques
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def extract_objects(self, grid: np.ndarray) -> List[StructuredObject]:
        """
        Extrait TOUS les objets structurés de la grille
        
        Returns:
            Liste objets avec structure complète
        """
        if grid.size == 0:
            return []
        
        # Masque binaire (non-zéro = objet)
        binary = (grid > 0).astype(int)
        
        # Connected components labeling
        labeled, num_objects = label(binary)
        
        objects = []
        for obj_id in range(1, num_objects + 1):
            obj = self._extract_single_object(grid, labeled, obj_id)
            if obj:
                objects.append(obj)
        
        if self.verbose:
            print(f"[ObjectExtractor] Extracted {len(objects)} structured objects")
        
        return objects
    
    def _extract_single_object(
        self,
        grid: np.ndarray,
        labeled: np.ndarray,
        obj_id: int
    ) -> Optional[StructuredObject]:
        """Extrait UN objet avec structure complète"""
        
        # Masque objet
        mask = (labeled == obj_id)
        
        # Pixels objet
        pixels = np.argwhere(mask)
        
        if len(pixels) == 0:
            return None
        
        # Couleur (prendre couleur dominante)
        colors = grid[mask]
        color = int(np.bincount(colors).argmax())
        
        # Bounding box
        min_y, min_x = pixels.min(axis=0)
        max_y, max_x = pixels.max(axis=0)
        bbox = (int(min_y), int(min_x), int(max_y), int(max_x))
        
        # Centroid
        centroid = center_of_mass(mask)
        
        # Propriétés géométriques
        area = len(pixels)
        bbox_area = (max_y - min_y + 1) * (max_x - min_x + 1)
        density = area / max(1, bbox_area)
        
        # Signature forme (hash simple basé sur forme relative)
        shape_sig = self._compute_shape_signature(pixels, centroid)
        
        return StructuredObject(
            object_id=obj_id,
            pixels=pixels,
            color=color,
            bbox=bbox,
            centroid=centroid,
            area=area,
            density=density,
            shape_signature=shape_sig
        )
    
    def _compute_shape_signature(
        self,
        pixels: np.ndarray,
        centroid: Tuple[float, float]
    ) -> str:
        """
        Calcule signature géométrique pour matching
        
        Basé sur distribution angulaire pixels autour centroid
        """
        if len(pixels) == 0:
            return "empty"
        
        cy, cx = centroid
        
        # Coordonnées relatives
        rel_y = pixels[:, 0] - cy
        rel_x = pixels[:, 1] - cx
        
        # Angles et distances
        angles = np.arctan2(rel_y, rel_x)
        distances = np.sqrt(rel_y**2 + rel_x**2)
        
        # Histogramme angulaire (8 bins)
        hist, _ = np.histogram(angles, bins=8, range=(-np.pi, np.pi))
        
        # Signature = pattern histogram
        sig = ''.join(str(int(h > 0)) for h in hist)
        
        return sig


class SpatialPlacer:
    """
    Placement spatial intelligent d'objets
    
    Amélioration critique vs placement aléatoire:
    - Respect symétrie existante
    - Alignement avec objets existants
    - Maximisation espace vide
    - Répétition spatiale régulière
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def find_placement_positions(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        count: int,
        strategy: str = "auto"
    ) -> List[Tuple[int, int]]:
        """
        Trouve positions optimales pour placer N copies d'un objet
        
        Args:
            grid: Grille actuelle
            obj: Objet à placer
            count: Nombre copies à placer
            strategy: "auto", "symmetric", "aligned", "grid", "maxspace"
        
        Returns:
            Liste positions (y, x) pour centroids
        """
        h, w = grid.shape
        
        # Détecter stratégie automatiquement si "auto"
        if strategy == "auto":
            strategy = self._detect_placement_strategy(grid, obj)
        
        if strategy == "symmetric":
            return self._symmetric_placement(grid, obj, count)
        elif strategy == "aligned":
            return self._aligned_placement(grid, obj, count)
        elif strategy == "grid":
            return self._grid_placement(grid, obj, count)
        elif strategy == "maxspace":
            return self._maxspace_placement(grid, obj, count)
        else:
            # Fallback: maxspace
            return self._maxspace_placement(grid, obj, count)
    
    def _detect_placement_strategy(
        self,
        grid: np.ndarray,
        obj: StructuredObject
    ) -> str:
        """Détecte stratégie placement optimale"""
        
        # Extraire objets existants
        extractor = ObjectExtractor()
        existing = extractor.extract_objects(grid)
        
        if len(existing) == 0:
            return "maxspace"
        
        # Vérifier symétrie
        if self._has_symmetry(existing):
            return "symmetric"
        
        # Vérifier alignement
        if self._has_alignment(existing):
            return "aligned"
        
        # Vérifier grille régulière
        if self._has_grid_pattern(existing):
            return "grid"
        
        # Défaut
        return "maxspace"
    
    def _has_symmetry(self, objects: List[StructuredObject]) -> bool:
        """Vérifie si objets ont symétrie"""
        if len(objects) < 2:
            return False
        
        # Vérifier symétrie verticale
        centroids = np.array([obj.centroid for obj in objects])
        mean_x = centroids[:, 1].mean()
        
        # Compter objets de chaque côté
        left = np.sum(centroids[:, 1] < mean_x)
        right = np.sum(centroids[:, 1] > mean_x)
        
        return abs(left - right) <= 1
    
    def _has_alignment(self, objects: List[StructuredObject]) -> bool:
        """Vérifie si objets sont alignés"""
        if len(objects) < 2:
            return False
        
        centroids = np.array([obj.centroid for obj in objects])
        
        # Vérifier alignement horizontal
        y_std = centroids[:, 0].std()
        if y_std < 2.0:
            return True
        
        # Vérifier alignement vertical
        x_std = centroids[:, 1].std()
        if x_std < 2.0:
            return True
        
        return False
    
    def _has_grid_pattern(self, objects: List[StructuredObject]) -> bool:
        """Vérifie si objets forment grille régulière"""
        if len(objects) < 4:
            return False
        
        centroids = np.array([obj.centroid for obj in objects])
        
        # Calculer distances entre objets
        from scipy.spatial.distance import pdist
        distances = pdist(centroids)
        
        # Vérifier si distances sont régulières
        unique_dists = np.unique(np.round(distances, 1))
        
        # Grille régulière = peu de distances uniques
        return len(unique_dists) <= 3
    
    def _symmetric_placement(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        count: int
    ) -> List[Tuple[int, int]]:
        """Placement symétrique"""
        h, w = grid.shape
        center_x = w // 2
        
        positions = []
        spacing = w // (count + 1)
        
        for i in range(count):
            # Alterner gauche/droite
            if i % 2 == 0:
                x = center_x + (i // 2 + 1) * spacing
            else:
                x = center_x - (i // 2 + 1) * spacing
            
            y = h // 2
            
            if 0 <= x < w and 0 <= y < h:
                positions.append((y, x))
        
        return positions[:count]
    
    def _aligned_placement(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        count: int
    ) -> List[Tuple[int, int]]:
        """Placement aligné"""
        h, w = grid.shape
        
        # Trouver ligne/colonne libre
        occupied = (grid > 0)
        
        # Chercher ligne avec le plus d'espace
        row_space = (~occupied).sum(axis=1)
        best_row = int(row_space.argmax())
        
        positions = []
        spacing = w // (count + 1)
        
        for i in range(count):
            x = (i + 1) * spacing
            if 0 <= x < w:
                positions.append((best_row, x))
        
        return positions[:count]
    
    def _grid_placement(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        count: int
    ) -> List[Tuple[int, int]]:
        """Placement grille régulière"""
        h, w = grid.shape
        
        # Calculer grille optimale
        cols = int(np.ceil(np.sqrt(count)))
        rows = int(np.ceil(count / cols))
        
        spacing_y = h // (rows + 1)
        spacing_x = w // (cols + 1)
        
        positions = []
        for i in range(count):
            row = i // cols
            col = i % cols
            
            y = (row + 1) * spacing_y
            x = (col + 1) * spacing_x
            
            if 0 <= y < h and 0 <= x < w:
                positions.append((y, x))
        
        return positions[:count]
    
    def _maxspace_placement(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        count: int
    ) -> List[Tuple[int, int]]:
        """Placement maximisant espace vide"""
        h, w = grid.shape
        occupied = (grid > 0)
        
        positions = []
        obj_h, obj_w = obj.get_height(), obj.get_width()
        
        # Trouver zones vides suffisamment grandes
        for _ in range(count):
            best_pos = None
            max_space = 0
            
            for y in range(0, h - obj_h + 1, 2):
                for x in range(0, w - obj_w + 1, 2):
                    # Vérifier si zone libre
                    zone = occupied[y:y+obj_h, x:x+obj_w]
                    if not zone.any():
                        # Calculer espace autour
                        space = self._compute_surrounding_space(occupied, y, x, obj_h, obj_w)
                        if space > max_space:
                            max_space = space
                            best_pos = (y + obj_h // 2, x + obj_w // 2)
            
            if best_pos:
                positions.append(best_pos)
                # Marquer zone occupée
                y, x = best_pos
                y1 = max(0, y - obj_h // 2)
                x1 = max(0, x - obj_w // 2)
                y2 = min(h, y1 + obj_h)
                x2 = min(w, x1 + obj_w)
                occupied[y1:y2, x1:x2] = True
        
        return positions
    
    def _compute_surrounding_space(
        self,
        occupied: np.ndarray,
        y: int,
        x: int,
        h: int,
        w: int
    ) -> int:
        """Calcule espace vide autour d'une zone"""
        grid_h, grid_w = occupied.shape
        
        # Zone étendue
        margin = 2
        y1 = max(0, y - margin)
        x1 = max(0, x - margin)
        y2 = min(grid_h, y + h + margin)
        x2 = min(grid_w, x + w + margin)
        
        zone = occupied[y1:y2, x1:x2]
        return int((~zone).sum())


class ObjectDuplicator:
    """
    Duplication structurelle d'objets
    
    Amélioration critique vs copie pixel:
    - Préserve FORME COMPLÈTE
    - Maintient structure spatiale
    - Conserve propriétés géométriques
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.placer = SpatialPlacer(verbose=verbose)
    
    def duplicate_objects(
        self,
        grid: np.ndarray,
        objects: List[StructuredObject],
        target_count: int
    ) -> np.ndarray:
        """
        Duplique objets pour atteindre target_count
        
        Args:
            grid: Grille actuelle
            objects: Objets à dupliquer
            target_count: Nombre total objets souhaité
        
        Returns:
            Nouvelle grille avec objets dupliqués
        """
        if len(objects) == 0 or target_count <= len(objects):
            return grid.copy()
        
        result = grid.copy()
        to_add = target_count - len(objects)
        
        # Choisir objet à dupliquer (le plus fréquent)
        obj_to_duplicate = self._select_object_to_duplicate(objects)
        
        # Trouver positions placement
        positions = self.placer.find_placement_positions(
            result,
            obj_to_duplicate,
            to_add,
            strategy="auto"
        )
        
        # Placer copies
        for pos in positions:
            result = self._place_object_copy(result, obj_to_duplicate, pos)
        
        if self.verbose:
            print(f"[ObjectDuplicator] Added {len(positions)} object copies")
        
        return result
    
    def _select_object_to_duplicate(
        self,
        objects: List[StructuredObject]
    ) -> StructuredObject:
        """Sélectionne objet à dupliquer (le plus représentatif)"""
        if len(objects) == 1:
            return objects[0]
        
        # Grouper par signature forme
        by_shape = defaultdict(list)
        for obj in objects:
            by_shape[obj.shape_signature].append(obj)
        
        # Prendre forme la plus fréquente
        most_common_shape = max(by_shape.keys(), key=lambda k: len(by_shape[k]))
        candidates = by_shape[most_common_shape]
        
        # Prendre objet médian en taille
        candidates.sort(key=lambda o: o.area)
        return candidates[len(candidates) // 2]
    
    def _select_object_to_duplicate(
        self,
        objects: List[StructuredObject]
    ) -> StructuredObject:
        """Sélectionne objet à dupliquer (le plus représentatif)"""
        if len(objects) == 1:
            return objects[0]
        
        # Grouper par signature forme
        by_shape = defaultdict(list)
        for obj in objects:
            by_shape[obj.shape_signature].append(obj)
        
        # Prendre forme la plus fréquente
        most_common_shape = max(by_shape.keys(), key=lambda k: len(by_shape[k]))
        candidates = by_shape[most_common_shape]
        
        # Prendre objet médian en taille
        candidates.sort(key=lambda o: o.area)
        return candidates[len(candidates) // 2]
    
    def _place_object_copy(
        self,
        grid: np.ndarray,
        obj: StructuredObject,
        position: Tuple[int, int]
    ) -> np.ndarray:
        """Place copie d'objet à position donnée"""
        result = grid.copy()
        
        target_y, target_x = position
        cy, cx = obj.centroid
        
        # Calculer offset
        offset_y = target_y - cy
        offset_x = target_x - cx
        
        # Placer tous pixels
        for py, px in obj.pixels:
            new_y = int(py + offset_y)
            new_x = int(px + offset_x)
            
            # Vérifier bounds
            if 0 <= new_y < grid.shape[0] and 0 <= new_x < grid.shape[1]:
                # Ne pas écraser objets existants
                if result[new_y, new_x] == 0:
                    result[new_y, new_x] = obj.color
        
        return result


# Made with Bob - Expert Forensique LumVorax
# Protocole: CLAUDE_PILOT activé
# Session 57 - Object Transformation Engine RÉEL