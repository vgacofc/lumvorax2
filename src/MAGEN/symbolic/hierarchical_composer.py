"""
MAGEN - Hierarchical Composer
Composition multi-niveaux de transformations hiérarchiques

Permet:
- Composer transformations de différents niveaux
- Propagation automatique entre niveaux
- Validation cohérence multi-niveaux
- Optimisation compositions
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional
from dataclasses import dataclass, field
import copy

# Import modules MAGEN
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic.hierarchical_transforms import (
    TransformLevel,
    HierarchicalTransform,
    ObjectLevelTransforms,
    SceneLevelTransforms,
    GlobalLevelTransforms
)
from objects import Object, SceneGraph


@dataclass
class HierarchicalComposition:
    """
    Composition hiérarchique de transformations
    
    Représente séquence transformations multi-niveaux:
    - Object-level: [move(obj1), recolor(obj2)]
    - Scene-level: [align(group1)]
    - Global-level: [rotate_90(grid)]
    """
    transforms: List[HierarchicalTransform] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def add_transform(self, transform: HierarchicalTransform):
        """Ajouter transformation"""
        self.transforms.append(transform)
    
    def get_by_level(self, level: TransformLevel) -> List[HierarchicalTransform]:
        """Récupérer transformations d'un niveau"""
        return [t for t in self.transforms if t.level == level]
    
    def __repr__(self):
        counts = {
            TransformLevel.OBJECT: len(self.get_by_level(TransformLevel.OBJECT)),
            TransformLevel.SCENE: len(self.get_by_level(TransformLevel.SCENE)),
            TransformLevel.GLOBAL: len(self.get_by_level(TransformLevel.GLOBAL))
        }
        return f"HierarchicalComposition(O:{counts[TransformLevel.OBJECT]}, S:{counts[TransformLevel.SCENE]}, G:{counts[TransformLevel.GLOBAL]})"


class HierarchicalComposer:
    """
    Compositeur hiérarchique
    
    Fonctionnalités:
    - Composer transformations multi-niveaux
    - Appliquer composition sur grille + scene graph
    - Propager transformations entre niveaux
    - Valider cohérence
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        
        # Transformateurs par niveau
        self.object_transforms = ObjectLevelTransforms(verbose)
        self.scene_transforms = SceneLevelTransforms(verbose)
        self.global_transforms = GlobalLevelTransforms(verbose)
        
        # Statistiques
        self.compositions_applied = 0
        self.propagations = 0
    
    def apply_composition(self, composition: HierarchicalComposition,
                         grid: np.ndarray, scene_graph: SceneGraph) -> Tuple[np.ndarray, SceneGraph]:
        """
        Appliquer composition hiérarchique
        
        Stratégie:
        1. Appliquer transformations object-level
        2. Appliquer transformations scene-level
        3. Appliquer transformations global-level
        4. Propager changements entre niveaux
        
        Args:
            composition: Composition à appliquer
            grid: Grille input
            scene_graph: Scene graph input
        
        Returns:
            (grille transformée, scene graph transformé)
        """
        current_grid = grid.copy()
        current_sg = copy.deepcopy(scene_graph)
        
        # 1. Object-level transforms
        object_transforms = composition.get_by_level(TransformLevel.OBJECT)
        for transform in object_transforms:
            current_grid, current_sg = self._apply_object_transform(
                transform, current_grid, current_sg
            )
        
        # 2. Scene-level transforms
        scene_transforms = composition.get_by_level(TransformLevel.SCENE)
        for transform in scene_transforms:
            current_grid, current_sg = self._apply_scene_transform(
                transform, current_grid, current_sg
            )
        
        # 3. Global-level transforms
        global_transforms = composition.get_by_level(TransformLevel.GLOBAL)
        for transform in global_transforms:
            current_grid, current_sg = self._apply_global_transform(
                transform, current_grid, current_sg
            )
        
        self.compositions_applied += 1
        return current_grid, current_sg
    
    def _apply_object_transform(self, transform: HierarchicalTransform,
                               grid: np.ndarray, scene_graph: SceneGraph) -> Tuple[np.ndarray, SceneGraph]:
        """Appliquer transformation object-level"""
        new_grid = grid.copy()
        new_sg = copy.deepcopy(scene_graph)
        
        # Récupérer objets ciblés
        target_objects = []
        for obj_id in transform.target_objects:
            obj = new_sg.get_object(obj_id)
            if obj:
                target_objects.append(obj)
        
        if not target_objects:
            return new_grid, new_sg
        
        # Appliquer transformation selon type
        transform_type = transform.transform_type
        params = transform.parameters
        
        for obj in target_objects:
            if transform_type == 'move':
                new_obj = self.object_transforms.move_object(
                    obj, params.get('dx', 0), params.get('dy', 0), (grid.shape[0], grid.shape[1])
                )
            elif transform_type == 'recolor':
                new_obj = self.object_transforms.recolor_object(
                    obj, params.get('new_color', obj.color)
                )
            elif transform_type == 'scale':
                new_obj = self.object_transforms.scale_object(
                    obj, params.get('scale_factor', 1.0), (grid.shape[0], grid.shape[1])
                )
            elif transform_type == 'rotate':
                new_obj = self.object_transforms.rotate_object(
                    obj, params.get('angle', 90), (grid.shape[0], grid.shape[1])
                )
            else:
                new_obj = obj
            
            # Mettre à jour scene graph
            idx = new_sg.objects.index(obj)
            new_sg.objects[idx] = new_obj
            
            # Mettre à jour grille
            new_grid = self._update_grid_from_object(new_grid, obj, new_obj)
        
        return new_grid, new_sg
    
    def _apply_scene_transform(self, transform: HierarchicalTransform,
                              grid: np.ndarray, scene_graph: SceneGraph) -> Tuple[np.ndarray, SceneGraph]:
        """Appliquer transformation scene-level"""
        new_grid = grid.copy()
        new_sg = copy.deepcopy(scene_graph)
        
        transform_type = transform.transform_type
        params = transform.parameters
        
        if transform_type == 'group':
            # Grouper objets (pas de modification grille)
            groups = self.scene_transforms.group_objects(
                new_sg.objects, params.get('criterion', 'color')
            )
            # Stocker groupes dans metadata
            new_sg.properties['groups'] = groups
        
        elif transform_type == 'align':
            # Aligner objets
            aligned = self.scene_transforms.align_objects(
                new_sg.objects, params.get('alignment', 'horizontal'), (grid.shape[0], grid.shape[1])
            )
            # Mettre à jour scene graph et grille
            for old_obj, new_obj in zip(new_sg.objects, aligned):
                new_grid = self._update_grid_from_object(new_grid, old_obj, new_obj)
            new_sg.objects = aligned
        
        elif transform_type == 'distribute':
            # Distribuer objets
            distributed = self.scene_transforms.distribute_objects(
                new_sg.objects, params.get('direction', 'horizontal'), (grid.shape[0], grid.shape[1])
            )
            # Mettre à jour scene graph et grille
            for old_obj, new_obj in zip(new_sg.objects, distributed):
                new_grid = self._update_grid_from_object(new_grid, old_obj, new_obj)
            new_sg.objects = distributed
        
        elif transform_type == 'symmetry':
            # Créer symétrie
            mirrored = self.scene_transforms.create_symmetry(
                new_sg.objects, params.get('axis', 'horizontal'), (grid.shape[0], grid.shape[1])
            )
            # Ajouter objets symétriques
            new_objects = mirrored[len(new_sg.objects):]
            for obj in new_objects:
                new_grid = self._add_object_to_grid(new_grid, obj)
            new_sg.objects = mirrored
        
        return new_grid, new_sg
    
    def _apply_global_transform(self, transform: HierarchicalTransform,
                               grid: np.ndarray, scene_graph: SceneGraph) -> Tuple[np.ndarray, SceneGraph]:
        """Appliquer transformation global-level"""
        new_grid = grid.copy()
        new_sg = copy.deepcopy(scene_graph)
        
        transform_type = transform.transform_type
        params = transform.parameters
        
        if transform_type == 'rotate':
            # Rotation grille
            new_grid = self.global_transforms.rotate_grid(
                new_grid, params.get('angle', 90)
            )
            # Propager aux objets
            new_sg = self._propagate_rotation_to_objects(new_sg, params.get('angle', 90), (grid.shape[0], grid.shape[1]))
        
        elif transform_type == 'flip':
            # Flip grille
            new_grid = self.global_transforms.flip_grid(
                new_grid, params.get('axis', 'horizontal')
            )
            # Propager aux objets
            new_sg = self._propagate_flip_to_objects(new_sg, params.get('axis', 'horizontal'), (grid.shape[0], grid.shape[1]))
        
        elif transform_type == 'crop':
            # Crop grille
            new_grid = self.global_transforms.crop_grid(
                new_grid,
                params.get('y1', 0),
                params.get('x1', 0),
                params.get('y2', grid.shape[0]-1),
                params.get('x2', grid.shape[1]-1)
            )
            # Propager aux objets
            new_sg = self._propagate_crop_to_objects(
                new_sg,
                params.get('y1', 0),
                params.get('x1', 0),
                (new_grid.shape[0], new_grid.shape[1])
            )
        
        elif transform_type == 'expand':
            # Expand grille
            new_grid = self.global_transforms.expand_grid(
                new_grid,
                params.get('new_height', grid.shape[0]),
                params.get('new_width', grid.shape[1]),
                params.get('fill_value', 0)
            )
            # Pas de propagation nécessaire (objets restent en place)
            new_sg.grid_shape = (new_grid.shape[0], new_grid.shape[1])
        
        elif transform_type == 'tile':
            # Tile grille
            new_grid = self.global_transforms.tile_grid(
                new_grid,
                params.get('repeat_y', 1),
                params.get('repeat_x', 1)
            )
            # Propager aux objets (dupliquer)
            new_sg = self._propagate_tile_to_objects(
                new_sg,
                params.get('repeat_y', 1),
                params.get('repeat_x', 1),
                (grid.shape[0], grid.shape[1])
            )
        
        return new_grid, new_sg
    
    def _update_grid_from_object(self, grid: np.ndarray, old_obj: Object, new_obj: Object) -> np.ndarray:
        """Mettre à jour grille après modification objet"""
        new_grid = grid.copy()
        
        # Effacer ancien objet
        for y, x in old_obj.pixels:
            if 0 <= y < grid.shape[0] and 0 <= x < grid.shape[1]:
                new_grid[y, x] = 0  # Background
        
        # Dessiner nouvel objet
        for y, x in new_obj.pixels:
            if 0 <= y < new_grid.shape[0] and 0 <= x < new_grid.shape[1]:
                new_grid[y, x] = new_obj.color
        
        return new_grid
    
    def _add_object_to_grid(self, grid: np.ndarray, obj: Object) -> np.ndarray:
        """Ajouter objet à grille"""
        new_grid = grid.copy()
        
        for y, x in obj.pixels:
            if 0 <= y < new_grid.shape[0] and 0 <= x < new_grid.shape[1]:
                new_grid[y, x] = obj.color
        
        return new_grid
    
    def _propagate_rotation_to_objects(self, scene_graph: SceneGraph, angle: int,
                                      original_shape: Tuple[int, int]) -> SceneGraph:
        """Propager rotation grille aux objets"""
        new_sg = copy.deepcopy(scene_graph)
        
        for i, obj in enumerate(new_sg.objects):
            # Rotation pixels
            new_pixels = []
            for y, x in obj.pixels:
                if angle == 90:
                    new_y, new_x = x, original_shape[0] - 1 - y
                elif angle == 180:
                    new_y, new_x = original_shape[0] - 1 - y, original_shape[1] - 1 - x
                elif angle == 270:
                    new_y, new_x = original_shape[1] - 1 - x, y
                else:
                    new_y, new_x = y, x
                
                new_pixels.append((new_y, new_x))
            
            obj.pixels = new_pixels
            
            # Recalculer bounding box
            if new_pixels:
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        # Mettre à jour grid_shape
        if angle in [90, 270]:
            new_sg.grid_shape = (original_shape[1], original_shape[0])
        
        self.propagations += 1
        return new_sg
    
    def _propagate_flip_to_objects(self, scene_graph: SceneGraph, axis: str,
                                   grid_shape: Tuple[int, int]) -> SceneGraph:
        """Propager flip grille aux objets"""
        new_sg = copy.deepcopy(scene_graph)
        
        for obj in new_sg.objects:
            new_pixels = []
            for y, x in obj.pixels:
                if axis == 'horizontal':
                    new_y, new_x = y, grid_shape[1] - 1 - x
                elif axis == 'vertical':
                    new_y, new_x = grid_shape[0] - 1 - y, x
                else:
                    new_y, new_x = y, x
                
                new_pixels.append((new_y, new_x))
            
            obj.pixels = new_pixels
            
            # Recalculer bounding box
            if new_pixels:
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        self.propagations += 1
        return new_sg
    
    def _propagate_crop_to_objects(self, scene_graph: SceneGraph, y1: int, x1: int,
                                   new_shape: Tuple[int, int]) -> SceneGraph:
        """Propager crop grille aux objets"""
        new_sg = copy.deepcopy(scene_graph)
        
        # Filtrer objets hors crop
        valid_objects = []
        for obj in new_sg.objects:
            new_pixels = []
            for y, x in obj.pixels:
                new_y, new_x = y - y1, x - x1
                if 0 <= new_y < new_shape[0] and 0 <= new_x < new_shape[1]:
                    new_pixels.append((new_y, new_x))
            
            if new_pixels:
                obj.pixels = new_pixels
                # Recalculer bounding box
                ys = [p[0] for p in new_pixels]
                xs = [p[1] for p in new_pixels]
                obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                valid_objects.append(obj)
        
        new_sg.objects = valid_objects
        new_sg.grid_shape = new_shape
        
        self.propagations += 1
        return new_sg
    
    def _propagate_tile_to_objects(self, scene_graph: SceneGraph, repeat_y: int, repeat_x: int,
                                   original_shape: Tuple[int, int]) -> SceneGraph:
        """Propager tile grille aux objets"""
        new_sg = copy.deepcopy(scene_graph)
        
        original_objects = list(new_sg.objects)
        new_objects = []
        
        for ry in range(repeat_y):
            for rx in range(repeat_x):
                if ry == 0 and rx == 0:
                    continue  # Garder originaux
                
                offset_y = ry * original_shape[0]
                offset_x = rx * original_shape[1]
                
                for obj in original_objects:
                    # Dupliquer objet avec offset
                    new_obj = copy.deepcopy(obj)
                    new_obj.id = len(new_sg.objects) + len(new_objects) + 1
                    new_obj.pixels = [(y + offset_y, x + offset_x) for y, x in obj.pixels]
                    
                    # Recalculer bounding box
                    ys = [p[0] for p in new_obj.pixels]
                    xs = [p[1] for p in new_obj.pixels]
                    new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
                    
                    new_objects.append(new_obj)
        
        new_sg.objects.extend(new_objects)
        new_sg.grid_shape = (original_shape[0] * repeat_y, original_shape[1] * repeat_x)
        
        self.propagations += 1
        return new_sg
    
    def get_statistics(self) -> Dict[str, Any]:
        """Statistiques complètes"""
        return {
            'compositions_applied': self.compositions_applied,
            'propagations': self.propagations,
            'object_transforms': self.object_transforms.get_statistics(),
            'scene_transforms': self.scene_transforms.get_statistics(),
            'global_transforms': self.global_transforms.get_statistics()
        }


# Made with Bob - Expert en Hierarchical Systems Design, Multi-Level Reasoning, Transformation Theory