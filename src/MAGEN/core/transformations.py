"""
ARC Transformations - Transformations Réelles pour Puzzles ARC
===============================================================

Implémente les transformations géométriques, spatiales et de couleur
nécessaires pour résoudre les puzzles ARC-AGI.

Catégories:
- Géométriques: rotate, flip, transpose
- Spatiales: crop, extend, tile, scale
- Couleur: map, invert, filter
- Patterns: detect, extract, apply

Date: 2026-06-15
Version: V38.1
"""

import numpy as np
from typing import Tuple, List, Optional, Dict
from dataclasses import dataclass


@dataclass
class TransformationResult:
    """Résultat d'une transformation."""
    grid: np.ndarray
    transform_name: str
    success: bool
    metadata: Dict


class ARCTransformations:
    """
    Transformations ARC réelles.
    
    Toutes les transformations retournent un TransformationResult.
    """
    
    # ========== TRANSFORMATIONS GÉOMÉTRIQUES ==========
    
    @staticmethod
    def rotate_90(grid: np.ndarray) -> TransformationResult:
        """Rotation 90° sens horaire."""
        result = np.rot90(grid, k=-1)  # k=-1 pour sens horaire
        return TransformationResult(
            grid=result,
            transform_name='rotate_90',
            success=True,
            metadata={'angle': 90, 'direction': 'clockwise'}
        )
    
    @staticmethod
    def rotate_180(grid: np.ndarray) -> TransformationResult:
        """Rotation 180°."""
        result = np.rot90(grid, k=2)
        return TransformationResult(
            grid=result,
            transform_name='rotate_180',
            success=True,
            metadata={'angle': 180}
        )
    
    @staticmethod
    def rotate_270(grid: np.ndarray) -> TransformationResult:
        """Rotation 270° sens horaire (= 90° anti-horaire)."""
        result = np.rot90(grid, k=1)
        return TransformationResult(
            grid=result,
            transform_name='rotate_270',
            success=True,
            metadata={'angle': 270, 'direction': 'clockwise'}
        )
    
    @staticmethod
    def flip_horizontal(grid: np.ndarray) -> TransformationResult:
        """Flip horizontal (miroir vertical)."""
        result = np.fliplr(grid)
        return TransformationResult(
            grid=result,
            transform_name='flip_horizontal',
            success=True,
            metadata={'axis': 'horizontal'}
        )
    
    @staticmethod
    def flip_vertical(grid: np.ndarray) -> TransformationResult:
        """Flip vertical (miroir horizontal)."""
        result = np.flipud(grid)
        return TransformationResult(
            grid=result,
            transform_name='flip_vertical',
            success=True,
            metadata={'axis': 'vertical'}
        )
    
    @staticmethod
    def transpose(grid: np.ndarray) -> TransformationResult:
        """Transposition (échange lignes/colonnes)."""
        result = grid.T
        return TransformationResult(
            grid=result,
            transform_name='transpose',
            success=True,
            metadata={'original_shape': grid.shape, 'new_shape': result.shape}
        )
    
    # ========== TRANSFORMATIONS SPATIALES ==========
    
    @staticmethod
    def crop_nonzero(grid: np.ndarray, background: int = 0) -> TransformationResult:
        """Crop aux éléments non-background."""
        mask = grid != background
        if not mask.any():
            return TransformationResult(
                grid=grid,
                transform_name='crop_nonzero',
                success=False,
                metadata={'reason': 'all_background'}
            )
        
        rows = np.any(mask, axis=1)
        cols = np.any(mask, axis=0)
        ymin, ymax = np.where(rows)[0][[0, -1]]
        xmin, xmax = np.where(cols)[0][[0, -1]]
        
        result = grid[ymin:ymax+1, xmin:xmax+1]
        return TransformationResult(
            grid=result,
            transform_name='crop_nonzero',
            success=True,
            metadata={
                'bbox': (xmin, ymin, xmax+1, ymax+1),
                'original_shape': grid.shape,
                'cropped_shape': result.shape
            }
        )
    
    @staticmethod
    def extend(grid: np.ndarray, factor: int = 2) -> TransformationResult:
        """Étend la grille par un facteur."""
        h, w = grid.shape
        result = np.zeros((h * factor, w * factor), dtype=grid.dtype)
        
        for i in range(factor):
            for j in range(factor):
                result[i*h:(i+1)*h, j*w:(j+1)*w] = grid
        
        return TransformationResult(
            grid=result,
            transform_name='extend',
            success=True,
            metadata={
                'factor': factor,
                'original_shape': grid.shape,
                'extended_shape': result.shape
            }
        )
    
    @staticmethod
    def tile(grid: np.ndarray, rows: int = 2, cols: int = 2) -> TransformationResult:
        """Tile la grille (répétition)."""
        result = np.tile(grid, (rows, cols))
        return TransformationResult(
            grid=result,
            transform_name='tile',
            success=True,
            metadata={
                'rows': rows,
                'cols': cols,
                'original_shape': grid.shape,
                'tiled_shape': result.shape
            }
        )
    
    @staticmethod
    def scale_up(grid: np.ndarray, factor: int = 2) -> TransformationResult:
        """Scale up (chaque pixel devient factor×factor)."""
        h, w = grid.shape
        result = np.repeat(np.repeat(grid, factor, axis=0), factor, axis=1)
        return TransformationResult(
            grid=result,
            transform_name='scale_up',
            success=True,
            metadata={
                'factor': factor,
                'original_shape': grid.shape,
                'scaled_shape': result.shape
            }
        )
    
    # ========== TRANSFORMATIONS COULEUR ==========
    
    @staticmethod
    def invert_colors(grid: np.ndarray, max_color: int = 9) -> TransformationResult:
        """Inverse les couleurs."""
        result = max_color - grid
        return TransformationResult(
            grid=result,
            transform_name='invert_colors',
            success=True,
            metadata={'max_color': max_color}
        )
    
    @staticmethod
    def map_colors(grid: np.ndarray, color_map: Dict[int, int]) -> TransformationResult:
        """Mappe les couleurs selon dictionnaire."""
        result = grid.copy()
        for old_color, new_color in color_map.items():
            result[grid == old_color] = new_color
        
        return TransformationResult(
            grid=result,
            transform_name='map_colors',
            success=True,
            metadata={'color_map': color_map}
        )
    
    @staticmethod
    def filter_color(grid: np.ndarray, keep_color: int, background: int = 0) -> TransformationResult:
        """Garde seulement une couleur, reste = background."""
        result = np.where(grid == keep_color, keep_color, background)
        return TransformationResult(
            grid=result,
            transform_name='filter_color',
            success=True,
            metadata={'keep_color': keep_color, 'background': background}
        )
    
    # ========== TRANSFORMATIONS COMPOSÉES ==========
    
    @staticmethod
    def identity(grid: np.ndarray) -> TransformationResult:
        """Transformation identité (aucun changement)."""
        return TransformationResult(
            grid=grid.copy(),
            transform_name='identity',
            success=True,
            metadata={}
        )
    
    @staticmethod
    def get_all_geometric_transforms(grid: np.ndarray) -> List[TransformationResult]:
        """Retourne toutes les transformations géométriques."""
        return [
            ARCTransformations.identity(grid),
            ARCTransformations.rotate_90(grid),
            ARCTransformations.rotate_180(grid),
            ARCTransformations.rotate_270(grid),
            ARCTransformations.flip_horizontal(grid),
            ARCTransformations.flip_vertical(grid),
            ARCTransformations.transpose(grid),
        ]
    
    @staticmethod
    def get_all_spatial_transforms(grid: np.ndarray) -> List[TransformationResult]:
        """Retourne toutes les transformations spatiales."""
        return [
            ARCTransformations.crop_nonzero(grid),
            ARCTransformations.extend(grid, factor=2),
            ARCTransformations.tile(grid, rows=2, cols=2),
            ARCTransformations.scale_up(grid, factor=2),
        ]
    
    @staticmethod
    def get_all_color_transforms(grid: np.ndarray) -> List[TransformationResult]:
        """Retourne toutes les transformations couleur."""
        transforms = [ARCTransformations.invert_colors(grid)]
        
        # Ajouter filtres pour chaque couleur présente
        unique_colors = np.unique(grid)
        for color in unique_colors:
            if color != 0:  # Skip background
                transforms.append(ARCTransformations.filter_color(grid, color))
        
        return transforms


def apply_transformation(grid: np.ndarray, transform_name: str, **kwargs) -> TransformationResult:
    """
    Applique une transformation par nom.
    
    Args:
        grid: Grille d'entrée
        transform_name: Nom de la transformation
        **kwargs: Paramètres additionnels
    
    Returns:
        TransformationResult
    """
    transforms = {
        'identity': ARCTransformations.identity,
        'rotate_90': ARCTransformations.rotate_90,
        'rotate_180': ARCTransformations.rotate_180,
        'rotate_270': ARCTransformations.rotate_270,
        'flip_horizontal': ARCTransformations.flip_horizontal,
        'flip_vertical': ARCTransformations.flip_vertical,
        'transpose': ARCTransformations.transpose,
        'crop_nonzero': ARCTransformations.crop_nonzero,
        'extend': ARCTransformations.extend,
        'tile': ARCTransformations.tile,
        'scale_up': ARCTransformations.scale_up,
        'invert_colors': ARCTransformations.invert_colors,
        'map_colors': ARCTransformations.map_colors,
        'filter_color': ARCTransformations.filter_color,
    }
    
    if transform_name not in transforms:
        return TransformationResult(
            grid=grid,
            transform_name=transform_name,
            success=False,
            metadata={'error': f'Unknown transform: {transform_name}'}
        )
    
    try:
        return transforms[transform_name](grid, **kwargs)
    except Exception as e:
        return TransformationResult(
            grid=grid,
            transform_name=transform_name,
            success=False,
            metadata={'error': str(e)}
        )

# Made with Bob
