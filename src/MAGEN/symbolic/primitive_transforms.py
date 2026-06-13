"""
MAGEN - Primitive Symbolic Transformations
Moteur de transformations symboliques pour ARC-AGI-3

PRIORITÉ ABSOLUE identifiée par analyse experte:
- Program synthesis via primitives composables
- Transformations symboliques explicites
- Vérification exacte (pas approximative)

Architecture:
- Primitives atomiques (rotate, flip, crop, etc.)
- Composition de transformations
- Validation symbolique exacte
- Score de confiance par transformation
"""

import numpy as np
from typing import List, Tuple, Optional, Dict, Any, Callable
from dataclasses import dataclass
from enum import Enum
import copy
import sys
from pathlib import Path

# Import safe_operations
sys.path.insert(0, str(Path(__file__).parent.parent))
from core.safe_operations import SafeGridOperations, safe_access, safe_write


class TransformType(Enum):
    """Types de transformations primitives ARC"""
    # Géométriques
    ROTATE_90 = "rotate_90"
    ROTATE_180 = "rotate_180"
    ROTATE_270 = "rotate_270"
    FLIP_HORIZONTAL = "flip_horizontal"
    FLIP_VERTICAL = "flip_vertical"
    TRANSPOSE = "transpose"
    
    # Spatiales
    CROP = "crop"
    EXPAND = "expand"
    TRANSLATE = "translate"
    SCALE_UP = "scale_up"
    SCALE_DOWN = "scale_down"
    
    # Couleurs
    RECOLOR = "recolor"
    COLOR_MAP = "color_map"
    INVERT_COLORS = "invert_colors"
    
    # Objets
    EXTRACT_OBJECTS = "extract_objects"
    MOVE_OBJECT = "move_object"
    DUPLICATE_OBJECT = "duplicate_object"
    DELETE_OBJECT = "delete_object"
    
    # Patterns
    FLOOD_FILL = "flood_fill"
    DETECT_SYMMETRY = "detect_symmetry"
    COMPLETE_SYMMETRY = "complete_symmetry"
    REPEAT_PATTERN = "repeat_pattern"
    
    # Logiques
    AND = "and"
    OR = "or"
    XOR = "xor"
    NOT = "not"
    
    # Composition
    IDENTITY = "identity"
    COMPOSE = "compose"


@dataclass
class Transform:
    """Une transformation symbolique avec paramètres"""
    type: TransformType
    parameters: Dict[str, Any]
    confidence: float = 1.0
    description: str = ""
    
    def __repr__(self):
        return f"{self.type.value}({self.parameters}) [conf={self.confidence:.2f}]"


class PrimitiveTransforms:
    """
    Moteur de transformations primitives ARC
    
    Chaque méthode:
    - Prend une grille numpy
    - Retourne une grille transformée
    - Est EXACTE (pas approximative)
    - Est VÉRIFIABLE symboliquement
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.transform_count = 0
        self.safe_ops = SafeGridOperations()
        
        # Mapping type → fonction
        self.transform_functions: Dict[TransformType, Callable] = {
            TransformType.ROTATE_90: self.rotate_90,
            TransformType.ROTATE_180: self.rotate_180,
            TransformType.ROTATE_270: self.rotate_270,
            TransformType.FLIP_HORIZONTAL: self.flip_horizontal,
            TransformType.FLIP_VERTICAL: self.flip_vertical,
            TransformType.TRANSPOSE: self.transpose,
            TransformType.CROP: self.crop,
            TransformType.EXPAND: self.expand,
            TransformType.TRANSLATE: self.translate,
            TransformType.SCALE_UP: self.scale_up,
            TransformType.SCALE_DOWN: self.scale_down,
            TransformType.RECOLOR: self.recolor,
            TransformType.COLOR_MAP: self.color_map,
            TransformType.INVERT_COLORS: self.invert_colors,
            TransformType.EXTRACT_OBJECTS: self.extract_objects,
            TransformType.FLOOD_FILL: self.flood_fill,
            TransformType.DETECT_SYMMETRY: self.detect_symmetry,
            TransformType.COMPLETE_SYMMETRY: self.complete_symmetry,
            TransformType.REPEAT_PATTERN: self.repeat_pattern,
            TransformType.IDENTITY: self.identity,
        }
    
    # ========== TRANSFORMATIONS GÉOMÉTRIQUES ==========
    
    def rotate_90(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Rotation 90° sens horaire"""
        self.transform_count += 1
        return np.rot90(grid, k=-1)  # k=-1 pour sens horaire
    
    def rotate_180(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Rotation 180°"""
        self.transform_count += 1
        return np.rot90(grid, k=2)
    
    def rotate_270(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Rotation 270° sens horaire (= 90° anti-horaire)"""
        self.transform_count += 1
        return np.rot90(grid, k=1)
    
    def flip_horizontal(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Flip horizontal (miroir vertical)"""
        self.transform_count += 1
        return np.fliplr(grid)
    
    def flip_vertical(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Flip vertical (miroir horizontal)"""
        self.transform_count += 1
        return np.flipud(grid)
    
    def transpose(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Transposition (échange lignes/colonnes)"""
        self.transform_count += 1
        return grid.T
    
    # ========== TRANSFORMATIONS SPATIALES ==========
    
    def crop(self, grid: np.ndarray, x: int = 0, y: int = 0,
             width: Optional[int] = None, height: Optional[int] = None, **kwargs) -> np.ndarray:
        """Crop grille à une région"""
        self.transform_count += 1
        h, w = grid.shape
        width = width or w
        height = height or h
        
        # Clamp aux limites
        x = max(0, min(x, w-1))
        y = max(0, min(y, h-1))
        width = min(width, w - x)
        height = min(height, h - y)
        
        return grid[y:y+height, x:x+width].copy()
    
    def expand(self, grid: np.ndarray, top: int = 0, bottom: int = 0,
               left: int = 0, right: int = 0, fill_value: int = 0, **kwargs) -> np.ndarray:
        """Expand grille avec padding"""
        self.transform_count += 1
        return np.pad(grid, ((top, bottom), (left, right)), 
                     mode='constant', constant_values=fill_value)
    
    def translate(self, grid: np.ndarray, dx: int = 0, dy: int = 0,
                 fill_value: int = 0, **kwargs) -> np.ndarray:
        """Translation (déplacement)"""
        self.transform_count += 1
        h, w = grid.shape
        result = np.full_like(grid, fill_value)
        
        # Calculer régions source et destination
        src_y_start = max(0, -dy)
        src_y_end = min(h, h - dy)
        src_x_start = max(0, -dx)
        src_x_end = min(w, w - dx)
        
        dst_y_start = max(0, dy)
        dst_y_end = dst_y_start + (src_y_end - src_y_start)
        dst_x_start = max(0, dx)
        dst_x_end = dst_x_start + (src_x_end - src_x_start)
        
        result[dst_y_start:dst_y_end, dst_x_start:dst_x_end] = \
            grid[src_y_start:src_y_end, src_x_start:src_x_end]
        
        return result
    
    def scale_up(self, grid: np.ndarray, factor: int = 2, **kwargs) -> np.ndarray:
        """Scale up (répétition pixels)"""
        self.transform_count += 1
        return np.repeat(np.repeat(grid, factor, axis=0), factor, axis=1)
    
    def scale_down(self, grid: np.ndarray, factor: int = 2, **kwargs) -> np.ndarray:
        """Scale down (sous-échantillonnage)"""
        self.transform_count += 1
        h, w = grid.shape
        return grid[::factor, ::factor]
    
    # ========== TRANSFORMATIONS COULEURS ==========
    
    def recolor(self, grid: np.ndarray, old_color: int, new_color: int, **kwargs) -> np.ndarray:
        """Remplacer une couleur par une autre"""
        self.transform_count += 1
        result = grid.copy()
        result[grid == old_color] = new_color
        return result
    
    def color_map(self, grid: np.ndarray, mapping: Dict[int, int], **kwargs) -> np.ndarray:
        """Appliquer mapping de couleurs"""
        self.transform_count += 1
        result = grid.copy()
        for old_color, new_color in mapping.items():
            result[grid == old_color] = new_color
        return result
    
    def invert_colors(self, grid: np.ndarray, max_color: int = 9, **kwargs) -> np.ndarray:
        """Inverser couleurs (0→9, 1→8, etc.)"""
        self.transform_count += 1
        return max_color - grid
    
    # ========== TRANSFORMATIONS OBJETS ==========
    
    def extract_objects(self, grid: np.ndarray, background: int = 0, **kwargs) -> List[np.ndarray]:
        """Extraire objets connectés (flood fill)"""
        self.transform_count += 1
        objects = []
        visited = np.zeros_like(grid, dtype=bool)
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if not visited[i, j] and safe_access(grid, i, j) != background:
                    # Nouveau objet
                    obj_mask = self._flood_fill_mask(grid, i, j, visited)
                    objects.append(obj_mask)
        
        return objects
    
    def _flood_fill_mask(self, grid: np.ndarray, start_i: int, start_j: int,
                        visited: np.ndarray) -> np.ndarray:
        """Flood fill pour extraire un objet"""
        color = safe_access(grid, start_i, start_j)
        mask = np.zeros_like(grid, dtype=bool)
        stack = [(start_i, start_j)]
        
        while stack:
            i, j = stack.pop()
            if not self.safe_ops.is_valid_position(grid.shape, i, j):
                continue
            if visited[i, j] or safe_access(grid, i, j) != color:
                continue
            
            visited[i, j] = True
            mask[i, j] = True
            
            # 4-connectivité
            stack.extend([(i-1, j), (i+1, j), (i, j-1), (i, j+1)])
        
        return mask
    
    def flood_fill(self, grid: np.ndarray, x: int, y: int, new_color: int, **kwargs) -> np.ndarray:
        """Flood fill à partir d'un point"""
        self.transform_count += 1
        result = grid.copy()
        old_color = safe_access(grid, y, x)
        
        if old_color == new_color:
            return result
        
        stack = [(y, x)]
        visited = set()
        
        while stack:
            i, j = stack.pop()
            if (i, j) in visited:
                continue
            if not self.safe_ops.is_valid_position(result.shape, i, j):
                continue
            if safe_access(result, i, j) != old_color:
                continue
            
            visited.add((i, j))
            safe_write(result, i, j, new_color)
            
            stack.extend([(i-1, j), (i+1, j), (i, j-1), (i, j+1)])
        
        return result
    
    # ========== TRANSFORMATIONS PATTERNS ==========
    
    def detect_symmetry(self, grid: np.ndarray, **kwargs) -> Dict[str, bool]:
        """Détecter symétries présentes"""
        self.transform_count += 1
        return {
            'horizontal': np.array_equal(grid, np.fliplr(grid)),
            'vertical': np.array_equal(grid, np.flipud(grid)),
            'diagonal_main': np.array_equal(grid, grid.T),
            'diagonal_anti': np.array_equal(grid, np.fliplr(grid.T)),
            'rotational_90': np.array_equal(grid, np.rot90(grid, k=1)),
            'rotational_180': np.array_equal(grid, np.rot90(grid, k=2)),
        }
    
    def complete_symmetry(self, grid: np.ndarray, axis: str = 'horizontal', **kwargs) -> np.ndarray:
        """Compléter grille pour créer symétrie"""
        self.transform_count += 1
        if axis == 'horizontal':
            # Miroir horizontal
            return np.hstack([grid, np.fliplr(grid)])
        elif axis == 'vertical':
            # Miroir vertical
            return np.vstack([grid, np.flipud(grid)])
        elif axis == 'both':
            # Symétrie 4-fold
            top = np.hstack([grid, np.fliplr(grid)])
            bottom = np.flipud(top)
            return np.vstack([top, bottom])
        else:
            return grid.copy()
    
    def repeat_pattern(self, grid: np.ndarray, repeat_h: int = 2, repeat_v: int = 2, **kwargs) -> np.ndarray:
        """Répéter pattern en mosaïque"""
        self.transform_count += 1
        return np.tile(grid, (repeat_v, repeat_h))
    
    # ========== UTILITAIRES ==========
    
    def identity(self, grid: np.ndarray, **kwargs) -> np.ndarray:
        """Transformation identité (copie)"""
        self.transform_count += 1
        return grid.copy()
    
    def apply_transform(self, grid: np.ndarray, transform: Transform) -> np.ndarray:
        """Appliquer une transformation avec ses paramètres"""
        if transform.type not in self.transform_functions:
            raise ValueError(f"Transform type {transform.type} not implemented")
        
        func = self.transform_functions[transform.type]
        return func(grid, **transform.parameters)
    
    def compose_transforms(self, grid: np.ndarray, transforms: List[Transform]) -> np.ndarray:
        """Composer plusieurs transformations"""
        result = grid.copy()
        for transform in transforms:
            result = self.apply_transform(result, transform)
        return result
    
    def verify_exact_match(self, grid1: np.ndarray, grid2: np.ndarray) -> bool:
        """Vérification EXACTE (symbolique, pas approximative)"""
        return np.array_equal(grid1, grid2)
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques d'utilisation"""
        return {
            'total_transforms': self.transform_count
        }


# Made with Bob - Expert en Neuro-Symbolic AI, ARC-AGI Research, Program Synthesis