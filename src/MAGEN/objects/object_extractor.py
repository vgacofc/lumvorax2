"""
MAGEN - Object Extractor
Extraction d'objets et construction de scene graph

PRIORITÉ 2 identifiée par analyse experte:
- Object-centric reasoning (pas feature-centric)
- Scene graph avec objets, relations, propriétés
- Détection de shapes, patterns, structures

Architecture:
- Extraction d'objets connectés (flood fill)
- Détection de shapes (rectangle, line, L-shape, etc.)
- Construction de scene graph
- Détection de relations spatiales
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Set, Optional
from dataclasses import dataclass, field
from enum import Enum
import copy


class ShapeType(Enum):
    """Types de shapes détectables"""
    UNKNOWN = "unknown"
    POINT = "point"
    LINE_HORIZONTAL = "line_horizontal"
    LINE_VERTICAL = "line_vertical"
    LINE_DIAGONAL = "line_diagonal"
    RECTANGLE = "rectangle"
    SQUARE = "square"
    L_SHAPE = "l_shape"
    T_SHAPE = "t_shape"
    CROSS = "cross"
    HOLLOW_RECTANGLE = "hollow_rectangle"
    PATTERN = "pattern"
    BLOB = "blob"


class RelationType(Enum):
    """Types de relations spatiales"""
    ABOVE = "above"
    BELOW = "below"
    LEFT_OF = "left_of"
    RIGHT_OF = "right_of"
    INSIDE = "inside"
    CONTAINS = "contains"
    ADJACENT = "adjacent"
    OVERLAPS = "overlaps"
    SAME_ROW = "same_row"
    SAME_COLUMN = "same_column"
    ALIGNED_HORIZONTAL = "aligned_horizontal"
    ALIGNED_VERTICAL = "aligned_vertical"
    SYMMETRIC_TO = "symmetric_to"


@dataclass
class Object:
    """
    Objet extrait d'une grille ARC
    
    Représentation object-centric (pas feature-centric)
    """
    id: int
    color: int
    pixels: List[Tuple[int, int]]  # Liste de (y, x)
    bounding_box: Tuple[int, int, int, int]  # (y_min, x_min, y_max, x_max)
    shape_type: ShapeType = ShapeType.UNKNOWN
    properties: Dict[str, Any] = field(default_factory=dict)
    
    def __post_init__(self):
        """Calculer propriétés dérivées"""
        if not self.pixels:
            return
        
        # Calculer bounding box si pas fourni
        if self.bounding_box == (0, 0, 0, 0):
            ys = [p[0] for p in self.pixels]
            xs = [p[1] for p in self.pixels]
            self.bounding_box = (min(ys), min(xs), max(ys), max(xs))
        
        # Propriétés de base
        self.properties['size'] = len(self.pixels)
        self.properties['width'] = self.bounding_box[3] - self.bounding_box[1] + 1
        self.properties['height'] = self.bounding_box[2] - self.bounding_box[0] + 1
        self.properties['area'] = self.properties['width'] * self.properties['height']
        self.properties['density'] = self.properties['size'] / self.properties['area'] if self.properties['area'] > 0 else 0
        
        # Centre de masse
        center_y = sum(p[0] for p in self.pixels) / len(self.pixels)
        center_x = sum(p[1] for p in self.pixels) / len(self.pixels)
        self.properties['center'] = (center_y, center_x)
    
    def get_mask(self, grid_shape: Tuple[int, int]) -> np.ndarray:
        """Obtenir masque binaire de l'objet"""
        mask = np.zeros(grid_shape, dtype=bool)
        for y, x in self.pixels:
            if 0 <= y < grid_shape[0] and 0 <= x < grid_shape[1]:
                mask[y, x] = True
        return mask
    
    def __repr__(self):
        return f"Object(id={self.id}, color={self.color}, shape={self.shape_type.value}, size={self.properties.get('size', 0)})"


@dataclass
class Relation:
    """Relation spatiale entre deux objets"""
    type: RelationType
    object1_id: int
    object2_id: int
    confidence: float = 1.0
    properties: Dict[str, Any] = field(default_factory=dict)
    
    def __repr__(self):
        return f"Relation({self.object1_id} {self.type.value} {self.object2_id})"


@dataclass
class SceneGraph:
    """
    Scene graph: représentation structurée d'une grille ARC
    
    Contient:
    - Objets extraits
    - Relations spatiales
    - Propriétés globales
    """
    objects: List[Object]
    relations: List[Relation]
    grid_shape: Tuple[int, int]
    background_color: int = 0
    properties: Dict[str, Any] = field(default_factory=dict)
    
    def get_object(self, obj_id: int) -> Optional[Object]:
        """Récupérer objet par ID"""
        for obj in self.objects:
            if obj.id == obj_id:
                return obj
        return None
    
    def get_relations(self, obj_id: int, relation_type: Optional[RelationType] = None) -> List[Relation]:
        """Récupérer relations d'un objet"""
        relations = []
        for rel in self.relations:
            if rel.object1_id == obj_id or rel.object2_id == obj_id:
                if relation_type is None or rel.type == relation_type:
                    relations.append(rel)
        return relations
    
    def __repr__(self):
        return f"SceneGraph({len(self.objects)} objects, {len(self.relations)} relations)"


class ObjectExtractor:
    """
    Extracteur d'objets pour ARC-AGI-3
    
    Fonctionnalités:
    - Extraction d'objets connectés (4-connectivity et 8-connectivity)
    - Détection de shapes
    - Construction de scene graph
    - Détection de relations spatiales
    """
    
    def __init__(self, connectivity: int = 4, min_object_size: int = 1, verbose: bool = False):
        """
        Args:
            connectivity: 4 ou 8 (connectivité pour flood fill)
            min_object_size: Taille minimale d'un objet (en pixels)
            verbose: Mode verbeux
        """
        self.connectivity = connectivity
        self.min_object_size = min_object_size
        self.verbose = verbose
        self.objects_extracted = 0
    
    def extract_objects(self, grid: np.ndarray, background: int = 0) -> List[Object]:
        """
        Extraire tous les objets d'une grille
        
        Args:
            grid: Grille ARC (H, W)
            background: Couleur de fond à ignorer
        
        Returns:
            Liste d'objets extraits
        """
        objects = []
        visited = np.zeros_like(grid, dtype=bool)
        object_id = 0
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if not visited[i, j] and grid[i, j] != background:
                    # Nouveau objet
                    pixels = self._flood_fill(grid, i, j, visited)
                    
                    if len(pixels) >= self.min_object_size:
                        obj = Object(
                            id=object_id,
                            color=int(grid[i, j]),
                            pixels=pixels,
                            bounding_box=(0, 0, 0, 0)  # Sera calculé dans __post_init__
                        )
                        
                        # Détecter shape
                        obj.shape_type = self._detect_shape(obj, grid.shape)
                        
                        objects.append(obj)
                        object_id += 1
                        self.objects_extracted += 1
        
        return objects
    
    def _flood_fill(self, grid: np.ndarray, start_i: int, start_j: int,
                   visited: np.ndarray) -> List[Tuple[int, int]]:
        """Flood fill pour extraire pixels d'un objet"""
        color = grid[start_i, start_j]
        pixels = []
        stack = [(start_i, start_j)]
        
        while stack:
            i, j = stack.pop()
            if i < 0 or i >= grid.shape[0] or j < 0 or j >= grid.shape[1]:
                continue
            if visited[i, j] or grid[i, j] != color:
                continue
            
            visited[i, j] = True
            pixels.append((i, j))
            
            # Connectivité
            if self.connectivity == 4:
                neighbors = [(i-1, j), (i+1, j), (i, j-1), (i, j+1)]
            else:  # 8-connectivity
                neighbors = [
                    (i-1, j-1), (i-1, j), (i-1, j+1),
                    (i, j-1),             (i, j+1),
                    (i+1, j-1), (i+1, j), (i+1, j+1)
                ]
            
            stack.extend(neighbors)
        
        return pixels
    
    def _detect_shape(self, obj: Object, grid_shape: Tuple[int, int]) -> ShapeType:
        """Détecter le type de shape d'un objet"""
        size = obj.properties['size']
        width = obj.properties['width']
        height = obj.properties['height']
        density = obj.properties['density']
        
        # Point
        if size == 1:
            return ShapeType.POINT
        
        # Ligne horizontale
        if height == 1 and width > 1:
            return ShapeType.LINE_HORIZONTAL
        
        # Ligne verticale
        if width == 1 and height > 1:
            return ShapeType.LINE_VERTICAL
        
        # Ligne diagonale
        if width == height and size == width:
            # Vérifier si pixels forment diagonale
            pixels_set = set(obj.pixels)
            y_min, x_min = obj.bounding_box[0], obj.bounding_box[1]
            diagonal_pixels = {(y_min + i, x_min + i) for i in range(width)}
            if pixels_set == diagonal_pixels:
                return ShapeType.LINE_DIAGONAL
        
        # Rectangle plein
        if density > 0.95:  # Presque tous les pixels du bounding box
            if width == height:
                return ShapeType.SQUARE
            else:
                return ShapeType.RECTANGLE
        
        # Rectangle creux
        if self._is_hollow_rectangle(obj):
            return ShapeType.HOLLOW_RECTANGLE
        
        # Croix
        if self._is_cross(obj):
            return ShapeType.CROSS
        
        # T-shape
        if self._is_t_shape(obj):
            return ShapeType.T_SHAPE
        
        # L-shape
        if self._is_l_shape(obj):
            return ShapeType.L_SHAPE
        
        # Pattern (densité moyenne)
        if 0.3 < density < 0.7:
            return ShapeType.PATTERN
        
        # Blob par défaut
        return ShapeType.BLOB
    
    def _is_hollow_rectangle(self, obj: Object) -> bool:
        """Vérifier si objet est rectangle creux"""
        y_min, x_min, y_max, x_max = obj.bounding_box
        pixels_set = set(obj.pixels)
        
        # Vérifier bordures
        border_pixels = set()
        for y in range(y_min, y_max + 1):
            border_pixels.add((y, x_min))
            border_pixels.add((y, x_max))
        for x in range(x_min, x_max + 1):
            border_pixels.add((y_min, x))
            border_pixels.add((y_max, x))
        
        # Si pixels = bordures, c'est un rectangle creux
        return pixels_set == border_pixels
    
    def _is_cross(self, obj: Object) -> bool:
        """Vérifier si objet est une croix"""
        y_min, x_min, y_max, x_max = obj.bounding_box
        center_y = (y_min + y_max) // 2
        center_x = (x_min + x_max) // 2
        
        pixels_set = set(obj.pixels)
        
        # Croix = ligne verticale + ligne horizontale au centre
        cross_pixels = set()
        for y in range(y_min, y_max + 1):
            cross_pixels.add((y, center_x))
        for x in range(x_min, x_max + 1):
            cross_pixels.add((center_y, x))
        
        # Tolérance: au moins 80% des pixels attendus
        overlap = len(pixels_set & cross_pixels)
        expected = len(cross_pixels)
        return overlap >= 0.8 * expected if expected > 0 else False
    
    def _is_t_shape(self, obj: Object) -> bool:
        """Vérifier si objet est un T"""
        # Simplifié: T = ligne horizontale en haut + ligne verticale au centre
        y_min, x_min, y_max, x_max = obj.bounding_box
        center_x = (x_min + x_max) // 2
        
        pixels_set = set(obj.pixels)
        
        # T-shape pixels
        t_pixels = set()
        # Ligne horizontale en haut
        for x in range(x_min, x_max + 1):
            t_pixels.add((y_min, x))
        # Ligne verticale au centre
        for y in range(y_min, y_max + 1):
            t_pixels.add((y, center_x))
        
        overlap = len(pixels_set & t_pixels)
        expected = len(t_pixels)
        return overlap >= 0.8 * expected if expected > 0 else False
    
    def _is_l_shape(self, obj: Object) -> bool:
        """Vérifier si objet est un L"""
        # Simplifié: L = ligne verticale à gauche + ligne horizontale en bas
        y_min, x_min, y_max, x_max = obj.bounding_box
        
        pixels_set = set(obj.pixels)
        
        # L-shape pixels
        l_pixels = set()
        # Ligne verticale à gauche
        for y in range(y_min, y_max + 1):
            l_pixels.add((y, x_min))
        # Ligne horizontale en bas
        for x in range(x_min, x_max + 1):
            l_pixels.add((y_max, x))
        
        overlap = len(pixels_set & l_pixels)
        expected = len(l_pixels)
        return overlap >= 0.8 * expected if expected > 0 else False
    
    def build_scene_graph(self, grid: np.ndarray, background: int = 0) -> SceneGraph:
        """
        Construire scene graph complet
        
        Args:
            grid: Grille ARC
            background: Couleur de fond
        
        Returns:
            SceneGraph avec objets et relations
        """
        # Extraire objets
        objects = self.extract_objects(grid, background)
        
        # Détecter relations
        relations = self._detect_relations(objects, grid.shape)
        
        # Propriétés globales
        properties = {
            'num_objects': len(objects),
            'num_colors': len(set(obj.color for obj in objects)),
            'colors': sorted(set(obj.color for obj in objects)),
            'shapes': [obj.shape_type.value for obj in objects],
        }
        
        return SceneGraph(
            objects=objects,
            relations=relations,
            grid_shape=grid.shape,
            background_color=background,
            properties=properties
        )
    
    def _detect_relations(self, objects: List[Object], grid_shape: Tuple[int, int]) -> List[Relation]:
        """Détecter relations spatiales entre objets"""
        relations = []
        
        for i, obj1 in enumerate(objects):
            for j, obj2 in enumerate(objects):
                if i >= j:  # Éviter doublons et auto-relations
                    continue
                
                # Détecter relations
                detected = self._detect_pairwise_relations(obj1, obj2, grid_shape)
                relations.extend(detected)
        
        return relations
    
    def _detect_pairwise_relations(self, obj1: Object, obj2: Object,
                                   grid_shape: Tuple[int, int]) -> List[Relation]:
        """Détecter relations entre deux objets"""
        relations = []
        
        bb1 = obj1.bounding_box
        bb2 = obj2.bounding_box
        
        # Above/Below
        if bb1[2] < bb2[0]:  # obj1 au-dessus de obj2
            relations.append(Relation(RelationType.ABOVE, obj1.id, obj2.id))
            relations.append(Relation(RelationType.BELOW, obj2.id, obj1.id))
        
        # Left/Right
        if bb1[3] < bb2[1]:  # obj1 à gauche de obj2
            relations.append(Relation(RelationType.LEFT_OF, obj1.id, obj2.id))
            relations.append(Relation(RelationType.RIGHT_OF, obj2.id, obj1.id))
        
        # Same row/column
        if bb1[0] == bb2[0] and bb1[2] == bb2[2]:
            relations.append(Relation(RelationType.SAME_ROW, obj1.id, obj2.id))
        
        if bb1[1] == bb2[1] and bb1[3] == bb2[3]:
            relations.append(Relation(RelationType.SAME_COLUMN, obj1.id, obj2.id))
        
        # Adjacent (bounding boxes se touchent)
        if self._are_adjacent(bb1, bb2):
            relations.append(Relation(RelationType.ADJACENT, obj1.id, obj2.id))
        
        # Overlaps (bounding boxes se chevauchent)
        if self._do_overlap(bb1, bb2):
            relations.append(Relation(RelationType.OVERLAPS, obj1.id, obj2.id))
        
        return relations
    
    def _are_adjacent(self, bb1: Tuple[int, int, int, int],
                     bb2: Tuple[int, int, int, int]) -> bool:
        """Vérifier si deux bounding boxes sont adjacentes"""
        # Horizontalement adjacentes
        if bb1[3] + 1 == bb2[1] or bb2[3] + 1 == bb1[1]:
            # Vérifier chevauchement vertical
            if not (bb1[2] < bb2[0] or bb2[2] < bb1[0]):
                return True
        
        # Verticalement adjacentes
        if bb1[2] + 1 == bb2[0] or bb2[2] + 1 == bb1[0]:
            # Vérifier chevauchement horizontal
            if not (bb1[3] < bb2[1] or bb2[3] < bb1[1]):
                return True
        
        return False
    
    def _do_overlap(self, bb1: Tuple[int, int, int, int],
                   bb2: Tuple[int, int, int, int]) -> bool:
        """Vérifier si deux bounding boxes se chevauchent"""
        return not (bb1[2] < bb2[0] or bb2[2] < bb1[0] or
                   bb1[3] < bb2[1] or bb2[3] < bb1[1])
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'objects_extracted': self.objects_extracted
        }


# Made with Bob - Expert en Computer Vision, Object Detection, Scene Understanding