"""
MAGEN - Object Transformer
Transformations sur objets et scene graphs

Fonctionnalités:
- Transformations d'objets individuels
- Transformations de scene graphs
- Détection de patterns de transformation
- Génération d'hypothèses object-centric
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional
from dataclasses import dataclass
import copy
import sys
from pathlib import Path

# Import absolu pour éviter erreur relative
sys.path.insert(0, str(Path(__file__).parent.parent))

from objects.object_extractor import Object, Relation, SceneGraph, ShapeType, RelationType
from core.safe_operations import SafeGridOperations, safe_access, safe_write


@dataclass
class ObjectTransformation:
    """Transformation appliquée à un objet"""
    object_id: int
    transform_type: str
    parameters: Dict[str, Any]
    description: str = ""
    
    def __repr__(self):
        return f"ObjectTransform({self.object_id}: {self.transform_type})"


class ObjectTransformer:
    """
    Transformateur d'objets pour ARC-AGI-3
    
    Opérations:
    - Move: Déplacer objet
    - Recolor: Changer couleur
    - Scale: Agrandir/rétrécir
    - Rotate: Rotation
    - Duplicate: Dupliquer
    - Delete: Supprimer
    - Merge: Fusionner objets
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.transformations_applied = 0
        self.safe_ops = SafeGridOperations()
    
    def move_object(self, obj: Object, dx: int, dy: int) -> Object:
        """Déplacer un objet"""
        new_obj = copy.deepcopy(obj)
        new_obj.pixels = [(y + dy, x + dx) for y, x in obj.pixels]
        
        # Recalculer bounding box
        ys = [p[0] for p in new_obj.pixels]
        xs = [p[1] for p in new_obj.pixels]
        new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        # Recalculer centre
        center_y = sum(p[0] for p in new_obj.pixels) / len(new_obj.pixels)
        center_x = sum(p[1] for p in new_obj.pixels) / len(new_obj.pixels)
        new_obj.properties['center'] = (center_y, center_x)
        
        self.transformations_applied += 1
        return new_obj
    
    def recolor_object(self, obj: Object, new_color: int) -> Object:
        """Changer couleur d'un objet"""
        new_obj = copy.deepcopy(obj)
        new_obj.color = new_color
        self.transformations_applied += 1
        return new_obj
    
    def scale_object(self, obj: Object, factor: int) -> Object:
        """Agrandir objet (répétition pixels)"""
        new_obj = copy.deepcopy(obj)
        
        # Calculer nouveau centre
        y_min, x_min = obj.bounding_box[0], obj.bounding_box[1]
        
        # Nouveaux pixels
        new_pixels = []
        for y, x in obj.pixels:
            # Position relative
            rel_y = y - y_min
            rel_x = x - x_min
            
            # Répéter
            for dy in range(factor):
                for dx in range(factor):
                    new_y = y_min + rel_y * factor + dy
                    new_x = x_min + rel_x * factor + dx
                    new_pixels.append((new_y, new_x))
        
        new_obj.pixels = new_pixels
        
        # Recalculer propriétés
        ys = [p[0] for p in new_pixels]
        xs = [p[1] for p in new_pixels]
        new_obj.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        new_obj.properties['size'] = len(new_pixels)
        new_obj.properties['width'] = new_obj.bounding_box[3] - new_obj.bounding_box[1] + 1
        new_obj.properties['height'] = new_obj.bounding_box[2] - new_obj.bounding_box[0] + 1
        
        self.transformations_applied += 1
        return new_obj
    
    def duplicate_object(self, obj: Object, new_id: int, dx: int = 0, dy: int = 0) -> Object:
        """Dupliquer objet avec déplacement optionnel"""
        new_obj = copy.deepcopy(obj)
        new_obj.id = new_id
        
        if dx != 0 or dy != 0:
            new_obj = self.move_object(new_obj, dx, dy)
        
        self.transformations_applied += 1
        return new_obj
    
    def apply_to_scene_graph(self, scene_graph: SceneGraph,
                            transformations: List[ObjectTransformation]) -> SceneGraph:
        """Appliquer transformations à un scene graph"""
        new_sg = copy.deepcopy(scene_graph)
        
        for transform in transformations:
            obj = new_sg.get_object(transform.object_id)
            if obj is None:
                continue
            
            if transform.transform_type == 'move':
                dx = transform.parameters.get('dx', 0)
                dy = transform.parameters.get('dy', 0)
                new_obj = self.move_object(obj, dx, dy)
                # Remplacer dans liste
                for i, o in enumerate(new_sg.objects):
                    if o.id == obj.id:
                        new_sg.objects[i] = new_obj
                        break
            
            elif transform.transform_type == 'recolor':
                new_color = transform.parameters.get('new_color', obj.color)
                new_obj = self.recolor_object(obj, new_color)
                for i, o in enumerate(new_sg.objects):
                    if o.id == obj.id:
                        new_sg.objects[i] = new_obj
                        break
            
            elif transform.transform_type == 'scale':
                factor = transform.parameters.get('factor', 2)
                new_obj = self.scale_object(obj, factor)
                for i, o in enumerate(new_sg.objects):
                    if o.id == obj.id:
                        new_sg.objects[i] = new_obj
                        break
        
        return new_sg
    
    def scene_graph_to_grid(self, scene_graph: SceneGraph) -> np.ndarray:
        """Reconstruire grille depuis scene graph"""
        grid = np.full(scene_graph.grid_shape, scene_graph.background_color, dtype=int)
        
        for obj in scene_graph.objects:
            for y, x in obj.pixels:
                if self.safe_ops.is_valid_position(grid.shape, y, x):
                    safe_write(grid, y, x, obj.color)
        
        return grid
    
    def detect_object_transformation_pattern(self, sg_input: SceneGraph,
                                            sg_output: SceneGraph) -> List[ObjectTransformation]:
        """
        Détecter pattern de transformation entre deux scene graphs
        
        Stratégie:
        - Matcher objets par couleur et shape
        - Détecter déplacements
        - Détecter changements de couleur
        - Détecter duplications
        """
        transformations = []
        
        # Matcher objets input→output par couleur et shape
        matched = set()
        
        for obj_in in sg_input.objects:
            best_match = None
            best_score = 0
            
            for obj_out in sg_output.objects:
                if obj_out.id in matched:
                    continue
                
                # Score de similarité
                score = 0
                if obj_in.color == obj_out.color:
                    score += 2
                if obj_in.shape_type == obj_out.shape_type:
                    score += 2
                if abs(obj_in.properties['size'] - obj_out.properties['size']) < 5:
                    score += 1
                
                if score > best_score:
                    best_score = score
                    best_match = obj_out
            
            if best_match and best_score >= 3:
                matched.add(best_match.id)
                
                # Détecter transformation
                center_in = obj_in.properties['center']
                center_out = best_match.properties['center']
                
                dx = int(center_out[1] - center_in[1])
                dy = int(center_out[0] - center_in[0])
                
                if dx != 0 or dy != 0:
                    transformations.append(ObjectTransformation(
                        object_id=obj_in.id,
                        transform_type='move',
                        parameters={'dx': dx, 'dy': dy},
                        description=f"Move object {obj_in.id} by ({dx}, {dy})"
                    ))
                
                if obj_in.color != best_match.color:
                    transformations.append(ObjectTransformation(
                        object_id=obj_in.id,
                        transform_type='recolor',
                        parameters={'new_color': best_match.color},
                        description=f"Recolor object {obj_in.id} to {best_match.color}"
                    ))
        
        return transformations
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'transformations_applied': self.transformations_applied
        }


# Made with Bob - Expert en Object Manipulation, Scene Graph Transformation