"""
MAGEN - Hierarchical Transformation System
Hiérarchie transformationnelle multi-niveaux (IDENTIFIÉE PAR ANALYSE EXPERTE)

PROBLÈME IDENTIFIÉ:
- Transformations actuelles = flat (pas de hiérarchie)
- Pas de distinction object-level vs scene-level vs global-level
- Manque composition multi-niveaux
- Pas de propagation transformations entre niveaux

SOLUTION:
- Object-level: Transformations sur objets individuels
- Scene-level: Transformations sur groupes/relations
- Global-level: Transformations sur grille entière
- Propagation automatique entre niveaux

Architecture:
1. ObjectLevelTransform: Move, recolor, scale objets
2. SceneLevelTransform: Group, align, distribute objets
3. GlobalLevelTransform: Rotate, flip, crop grille
4. HierarchicalComposer: Composition multi-niveaux
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional, Set
from dataclasses import dataclass, field
from enum import Enum
import copy

# Import modules MAGEN
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic import Transform, TransformType, ComposedTransform
from objects import Object, SceneGraph, Relation, ShapeType, RelationType
from core.safe_operations import SafeGridOperations, safe_access, safe_write


class TransformLevel(Enum):
    """Niveaux hiérarchiques de transformations"""
    OBJECT = "object"      # Transformations sur objets individuels
    SCENE = "scene"        # Transformations sur groupes/relations
    GLOBAL = "global"      # Transformations sur grille entière


@dataclass
class HierarchicalTransform:
    """
    Transformation hiérarchique avec niveau explicite
    
    Permet composition multi-niveaux:
    - Object-level: move(obj1), recolor(obj2)
    - Scene-level: align(group1), distribute(group2)
    - Global-level: rotate_90(grid), flip_h(grid)
    """
    level: TransformLevel
    transform_type: str
    parameters: Dict[str, Any] = field(default_factory=dict)
    target_objects: List[int] = field(default_factory=list)  # IDs objets ciblés
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __repr__(self):
        return f"HierarchicalTransform({self.level.value}:{self.transform_type})"


class ObjectLevelTransforms:
    """
    Transformations niveau objet
    
    Opèrent sur objets individuels:
    - Move: Déplacer objet
    - Recolor: Changer couleur
    - Scale: Redimensionner
    - Rotate: Rotation locale
    - Duplicate: Dupliquer objet
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.transforms_applied = 0
        self.safe_ops = SafeGridOperations()
    
    def move_object(self, obj: Object, dx: int, dy: int, grid_shape: Tuple[int, int]) -> Object:
        """
        Déplacer objet de (dx, dy)
        
        Args:
            obj: Objet à déplacer
            dx, dy: Déplacement horizontal/vertical
            grid_shape: Taille grille pour validation
        
        Returns:
            Nouvel objet déplacé
        """
        new_pixels = []
        for y, x in obj.pixels:
            new_y, new_x = y + dy, x + dx
            # Vérifier limites avec safe_ops
            if self.safe_ops.is_valid_position(grid_shape, new_y, new_x):
                new_pixels.append((new_y, new_x))
        
        if not new_pixels:
            return obj  # Déplacement invalide
        
        # Créer nouvel objet
        new_obj = copy.deepcopy(obj)
        new_obj.pixels = new_pixels
        
        # Recalculer bounding box
        ys = [p[0] for p in new_pixels]
        xs = [p[1] for p in new_pixels]
        new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        self.transforms_applied += 1
        return new_obj
    
    def recolor_object(self, obj: Object, new_color: int) -> Object:
        """
        Changer couleur objet
        
        Args:
            obj: Objet à recolorer
            new_color: Nouvelle couleur (0-9)
        
        Returns:
            Nouvel objet recoloré
        """
        new_obj = copy.deepcopy(obj)
        new_obj.color = new_color
        
        self.transforms_applied += 1
        return new_obj
    
    def scale_object(self, obj: Object, scale_factor: float, grid_shape: Tuple[int, int]) -> Object:
        """
        Redimensionner objet
        
        Args:
            obj: Objet à redimensionner
            scale_factor: Facteur échelle (>1 = agrandir, <1 = réduire)
            grid_shape: Taille grille
        
        Returns:
            Nouvel objet redimensionné
        """
        if scale_factor <= 0:
            return obj
        
        # Centre de l'objet
        center_y = sum(p[0] for p in obj.pixels) / len(obj.pixels)
        center_x = sum(p[1] for p in obj.pixels) / len(obj.pixels)
        
        # Redimensionner pixels
        new_pixels = []
        for y, x in obj.pixels:
            # Coordonnées relatives au centre
            rel_y, rel_x = y - center_y, x - center_x
            # Appliquer échelle
            new_rel_y = int(rel_y * scale_factor)
            new_rel_x = int(rel_x * scale_factor)
            # Coordonnées absolues
            new_y = int(center_y + new_rel_y)
            new_x = int(center_x + new_rel_x)
            
            # Vérifier limites avec safe_ops
            if self.safe_ops.is_valid_position(grid_shape, new_y, new_x):
                new_pixels.append((new_y, new_x))
        
        if not new_pixels:
            return obj
        
        # Créer nouvel objet
        new_obj = copy.deepcopy(obj)
        new_obj.pixels = list(set(new_pixels))  # Éliminer duplicatas
        
        # Recalculer bounding box
        ys = [p[0] for p in new_obj.pixels]
        xs = [p[1] for p in new_obj.pixels]
        new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        self.transforms_applied += 1
        return new_obj
    
    def rotate_object(self, obj: Object, angle: int, grid_shape: Tuple[int, int]) -> Object:
        """
        Rotation locale objet (90°, 180°, 270°)
        
        Args:
            obj: Objet à tourner
            angle: Angle rotation (90, 180, 270)
            grid_shape: Taille grille
        
        Returns:
            Nouvel objet tourné
        """
        if angle not in [90, 180, 270]:
            return obj
        
        # Centre de l'objet
        center_y = sum(p[0] for p in obj.pixels) / len(obj.pixels)
        center_x = sum(p[1] for p in obj.pixels) / len(obj.pixels)
        
        # Rotation pixels
        new_pixels = []
        for y, x in obj.pixels:
            # Coordonnées relatives
            rel_y, rel_x = y - center_y, x - center_x
            
            # Rotation
            if angle == 90:
                new_rel_y, new_rel_x = -rel_x, rel_y
            elif angle == 180:
                new_rel_y, new_rel_x = -rel_y, -rel_x
            else:  # 270
                new_rel_y, new_rel_x = rel_x, -rel_y
            
            # Coordonnées absolues
            new_y = int(center_y + new_rel_y)
            new_x = int(center_x + new_rel_x)
            
            # Vérifier limites avec safe_ops
            if self.safe_ops.is_valid_position(grid_shape, new_y, new_x):
                new_pixels.append((new_y, new_x))
        
        if not new_pixels:
            return obj
        
        # Créer nouvel objet
        new_obj = copy.deepcopy(obj)
        new_obj.pixels = new_pixels
        
        # Recalculer bounding box
        ys = [p[0] for p in new_obj.pixels]
        xs = [p[1] for p in new_obj.pixels]
        new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        self.transforms_applied += 1
        return new_obj
    
    def duplicate_object(self, obj: Object, offset_x: int, offset_y: int, 
                        grid_shape: Tuple[int, int], new_id: int) -> Optional[Object]:
        """
        Dupliquer objet avec offset
        
        Args:
            obj: Objet à dupliquer
            offset_x, offset_y: Offset duplication
            grid_shape: Taille grille
            new_id: ID nouvel objet
        
        Returns:
            Nouvel objet dupliqué ou None si invalide
        """
        # Déplacer copie
        duplicated = self.move_object(obj, offset_x, offset_y, grid_shape)
        
        if len(duplicated.pixels) == 0:
            return None
        
        # Assigner nouvel ID
        duplicated.id = new_id
        
        return duplicated
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'transforms_applied': self.transforms_applied
        }


class SceneLevelTransforms:
    """
    Transformations niveau scène
    
    Opèrent sur groupes d'objets et relations:
    - Group: Grouper objets similaires
    - Align: Aligner objets
    - Distribute: Distribuer objets uniformément
    - Mirror: Créer symétrie
    - Pattern: Répéter pattern
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.transforms_applied = 0
        self.safe_ops = SafeGridOperations()
    
    def group_objects(self, objects: List[Object], criterion: str) -> List[List[Object]]:
        """
        Grouper objets selon critère
        
        Args:
            objects: Liste objets
            criterion: 'color', 'shape', 'size'
        
        Returns:
            Liste de groupes
        """
        groups = {}
        
        for obj in objects:
            if criterion == 'color':
                key = obj.color
            elif criterion == 'shape':
                key = obj.shape_type.value
            elif criterion == 'size':
                key = len(obj.pixels)
            else:
                key = 'default'
            
            if key not in groups:
                groups[key] = []
            groups[key].append(obj)
        
        self.transforms_applied += 1
        return list(groups.values())
    
    def align_objects(self, objects: List[Object], alignment: str, 
                     grid_shape: Tuple[int, int]) -> List[Object]:
        """
        Aligner objets
        
        Args:
            objects: Objets à aligner
            alignment: 'horizontal', 'vertical', 'grid'
            grid_shape: Taille grille
        
        Returns:
            Objets alignés
        """
        if not objects:
            return objects
        
        aligned = []
        
        if alignment == 'horizontal':
            # Aligner sur même ligne (y moyen)
            avg_y = int(np.mean([obj.bounding_box[0] for obj in objects]))
            
            for obj in objects:
                current_y = obj.bounding_box[0]
                dy = avg_y - current_y
                
                # Déplacer objet
                new_pixels = [(y + dy, x) for y, x in obj.pixels]
                new_obj = copy.deepcopy(obj)
                new_obj.pixels = new_pixels
                
                # Recalculer bounding box
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                
                aligned.append(new_obj)
        
        elif alignment == 'vertical':
            # Aligner sur même colonne (x moyen)
            avg_x = int(np.mean([obj.bounding_box[1] for obj in objects]))
            
            for obj in objects:
                current_x = obj.bounding_box[1]
                dx = avg_x - current_x
                
                # Déplacer objet
                new_pixels = [(y, x + dx) for y, x in obj.pixels]
                new_obj = copy.deepcopy(obj)
                new_obj.pixels = new_pixels
                
                # Recalculer bounding box
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                
                aligned.append(new_obj)
        
        else:  # grid
            # Aligner en grille
            aligned = objects  # TODO: Implémenter grille
        
        self.transforms_applied += 1
        return aligned
    
    def distribute_objects(self, objects: List[Object], direction: str,
                          grid_shape: Tuple[int, int]) -> List[Object]:
        """
        Distribuer objets uniformément
        
        Args:
            objects: Objets à distribuer
            direction: 'horizontal', 'vertical'
            grid_shape: Taille grille
        
        Returns:
            Objets distribués
        """
        if len(objects) < 2:
            return objects
        
        distributed = []
        
        if direction == 'horizontal':
            # Distribuer horizontalement
            spacing = grid_shape[1] // (len(objects) + 1)
            
            for i, obj in enumerate(objects):
                target_x = spacing * (i + 1)
                current_x = obj.bounding_box[1]
                dx = target_x - current_x
                
                # Déplacer objet
                new_pixels = [(y, x + dx) for y, x in obj.pixels]
                new_obj = copy.deepcopy(obj)
                new_obj.pixels = new_pixels
                
                # Recalculer bounding box
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                
                distributed.append(new_obj)
        
        elif direction == 'vertical':
            # Distribuer verticalement
            spacing = grid_shape[0] // (len(objects) + 1)
            
            for i, obj in enumerate(objects):
                target_y = spacing * (i + 1)
                current_y = obj.bounding_box[0]
                dy = target_y - current_y
                
                # Déplacer objet
                new_pixels = [(y + dy, x) for y, x in obj.pixels]
                new_obj = copy.deepcopy(obj)
                new_obj.pixels = new_pixels
                
                # Recalculer bounding box
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                
                distributed.append(new_obj)
        
        self.transforms_applied += 1
        return distributed
    
    def create_symmetry(self, objects: List[Object], axis: str,
                       grid_shape: Tuple[int, int]) -> List[Object]:
        """
        Créer symétrie d'objets
        
        Args:
            objects: Objets source
            axis: 'horizontal', 'vertical'
            grid_shape: Taille grille
        
        Returns:
            Objets + objets symétriques
        """
        mirrored = list(objects)  # Garder originaux
        
        for obj in objects:
            if axis == 'horizontal':
                # Symétrie horizontale (axe vertical au centre)
                center_x = grid_shape[1] // 2
                new_pixels = []
                
                for y, x in obj.pixels:
                    # Distance au centre
                    dist = x - center_x
                    # Position symétrique
                    new_x = center_x - dist
                    
                    if self.safe_ops.is_valid_position(grid_shape, y, new_x):
                        new_pixels.append((y, new_x))
                
                if new_pixels:
                    new_obj = copy.deepcopy(obj)
                    new_obj.id = len(mirrored) + 1
                    new_obj.pixels = new_pixels
                    
                    # Recalculer bounding box
                    ys = [p[0] for p in new_pixels]
                    xs = [p[1] for p in new_pixels]
                    new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                    
                    mirrored.append(new_obj)
            
            elif axis == 'vertical':
                # Symétrie verticale (axe horizontal au centre)
                center_y = grid_shape[0] // 2
                new_pixels = []
                
                for y, x in obj.pixels:
                    # Distance au centre
                    dist = y - center_y
                    # Position symétrique
                    new_y = center_y - dist
                    
                    if self.safe_ops.is_valid_position(grid_shape, new_y, x):
                        new_pixels.append((new_y, x))
                
                if new_pixels:
                    new_obj = copy.deepcopy(obj)
                    new_obj.id = len(mirrored) + 1
                    new_obj.pixels = new_pixels
                    
                    # Recalculer bounding box
                    ys = [p[0] for p in new_pixels]
                    xs = [p[1] for p in new_pixels]
                    new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                    
                    mirrored.append(new_obj)
        
        self.transforms_applied += 1
        return mirrored
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'transforms_applied': self.transforms_applied
        }


class GlobalLevelTransforms:
    """
    Transformations niveau global
    
    Opèrent sur grille entière:
    - Rotate: Rotation 90°/180°/270°
    - Flip: Miroir horizontal/vertical
    - Crop: Découper région
    - Expand: Agrandir grille
    - Tile: Répéter grille
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.transforms_applied = 0
        self.safe_ops = SafeGridOperations()
    
    def rotate_grid(self, grid: np.ndarray, angle: int) -> np.ndarray:
        """
        Rotation grille
        
        Args:
            grid: Grille à tourner
            angle: 90, 180, 270
        
        Returns:
            Grille tournée
        """
        if angle == 90:
            rotated = np.rot90(grid, k=1)
        elif angle == 180:
            rotated = np.rot90(grid, k=2)
        elif angle == 270:
            rotated = np.rot90(grid, k=3)
        else:
            rotated = grid
        
        self.transforms_applied += 1
        return rotated
    
    def flip_grid(self, grid: np.ndarray, axis: str) -> np.ndarray:
        """
        Miroir grille
        
        Args:
            grid: Grille à miroiter
            axis: 'horizontal', 'vertical'
        
        Returns:
            Grille miroitée
        """
        if axis == 'horizontal':
            flipped = np.fliplr(grid)
        elif axis == 'vertical':
            flipped = np.flipud(grid)
        else:
            flipped = grid
        
        self.transforms_applied += 1
        return flipped
    
    def crop_grid(self, grid: np.ndarray, y1: int, x1: int, y2: int, x2: int) -> np.ndarray:
        """
        Découper région grille
        
        Args:
            grid: Grille source
            y1, x1, y2, x2: Coordonnées région
        
        Returns:
            Grille découpée
        """
        # Utiliser safe_slice pour crop sécurisé
        cropped = self.safe_ops.safe_slice(grid, y1, y2+1, x1, x2+1)
        
        self.transforms_applied += 1
        return cropped
    
    def expand_grid(self, grid: np.ndarray, new_height: int, new_width: int,
                   fill_value: int = 0) -> np.ndarray:
        """
        Agrandir grille
        
        Args:
            grid: Grille source
            new_height, new_width: Nouvelles dimensions
            fill_value: Valeur remplissage
        
        Returns:
            Grille agrandie
        """
        if new_height < grid.shape[0] or new_width < grid.shape[1]:
            return grid
        
        expanded = np.full((new_height, new_width), fill_value, dtype=grid.dtype)
        expanded[:grid.shape[0], :grid.shape[1]] = grid
        
        self.transforms_applied += 1
        return expanded
    
    def tile_grid(self, grid: np.ndarray, repeat_y: int, repeat_x: int) -> np.ndarray:
        """
        Répéter grille (tiling)
        
        Args:
            grid: Grille source
            repeat_y, repeat_x: Répétitions vertical/horizontal
        
        Returns:
            Grille répétée
        """
        tiled = np.tile(grid, (repeat_y, repeat_x))
        
        self.transforms_applied += 1
        return tiled
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'transforms_applied': self.transforms_applied
        }


# Made with Bob - Expert en Hierarchical Systems Design, Multi-Level Reasoning, Transformation Theory