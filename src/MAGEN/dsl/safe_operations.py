"""
MAGEN DSL - Safe Operations Module
Fonctions sécurisées pour éviter IndexError dans les transformations DSL

CORRECTION V20 SESSION 57:
- 16 IndexError identifiés en V19
- Cause: Accès hors limites dans grilles numpy
- Solution: Wrapper sécurisé avec validation bounds
"""

import numpy as np
from typing import Any, Optional, Tuple


def validate_bounds(grid: np.ndarray, row: int, col: int) -> bool:
    """
    Valide si les coordonnées sont dans les limites de la grille
    
    Args:
        grid: Grille numpy
        row: Index ligne
        col: Index colonne
        
    Returns:
        True si coordonnées valides, False sinon
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> validate_bounds(grid, 0, 0)  # True
        >>> validate_bounds(grid, 5, 5)  # False
        >>> validate_bounds(grid, -1, 0)  # False
    """
    if grid.size == 0:
        return False
    
    h, w = grid.shape
    return 0 <= row < h and 0 <= col < w


def safe_get_cell(grid: np.ndarray, row: int, col: int, default: Any = 0) -> Any:
    """
    Accès sécurisé à une cellule de grille
    
    Args:
        grid: Grille numpy
        row: Index ligne
        col: Index colonne
        default: Valeur par défaut si hors limites
        
    Returns:
        Valeur de la cellule ou default si hors limites
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> safe_get_cell(grid, 0, 0)  # 1
        >>> safe_get_cell(grid, 5, 5, default=-1)  # -1
    """
    if validate_bounds(grid, row, col):
        return grid[row, col]
    return default


def safe_set_cell(grid: np.ndarray, row: int, col: int, value: Any) -> bool:
    """
    Modification sécurisée d'une cellule de grille
    
    Args:
        grid: Grille numpy (modifiée in-place)
        row: Index ligne
        col: Index colonne
        value: Nouvelle valeur
        
    Returns:
        True si modification réussie, False si hors limites
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> safe_set_cell(grid, 0, 0, 9)  # True, grid[0,0] = 9
        >>> safe_set_cell(grid, 5, 5, 9)  # False, pas de modification
    """
    if validate_bounds(grid, row, col):
        grid[row, col] = value
        return True
    return False


def safe_get_region(grid: np.ndarray, 
                    row_start: int, row_end: int,
                    col_start: int, col_end: int,
                    default: Any = 0) -> np.ndarray:
    """
    Extraction sécurisée d'une région de grille
    
    Args:
        grid: Grille numpy
        row_start: Ligne début (inclusive)
        row_end: Ligne fin (exclusive)
        col_start: Colonne début (inclusive)
        col_end: Colonne fin (exclusive)
        default: Valeur par défaut pour zones hors limites
        
    Returns:
        Région extraite (avec padding si nécessaire)
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> safe_get_region(grid, 0, 2, 0, 2)  # [[1, 2], [3, 4]]
        >>> safe_get_region(grid, 0, 3, 0, 3, default=0)  # [[1, 2, 0], [3, 4, 0], [0, 0, 0]]
    """
    h, w = grid.shape
    
    # Calculer intersections avec limites grille
    actual_row_start = max(0, row_start)
    actual_row_end = min(h, row_end)
    actual_col_start = max(0, col_start)
    actual_col_end = min(w, col_end)
    
    # Dimensions région demandée
    target_h = row_end - row_start
    target_w = col_end - col_start
    
    # Créer région avec valeur par défaut
    result = np.full((target_h, target_w), default, dtype=grid.dtype)
    
    # Copier partie valide
    if actual_row_end > actual_row_start and actual_col_end > actual_col_start:
        # Calculer offsets dans résultat
        result_row_offset = actual_row_start - row_start
        result_col_offset = actual_col_start - col_start
        
        result[
            result_row_offset:result_row_offset + (actual_row_end - actual_row_start),
            result_col_offset:result_col_offset + (actual_col_end - actual_col_start)
        ] = grid[actual_row_start:actual_row_end, actual_col_start:actual_col_end]
    
    return result


def safe_slice(grid: np.ndarray, 
               row_slice: Optional[slice] = None,
               col_slice: Optional[slice] = None,
               default: Any = 0) -> np.ndarray:
    """
    Slicing sécurisé avec gestion hors limites
    
    Args:
        grid: Grille numpy
        row_slice: Slice pour lignes (None = toutes)
        col_slice: Slice pour colonnes (None = toutes)
        default: Valeur par défaut pour zones hors limites
        
    Returns:
        Région slicée (avec padding si nécessaire)
        
    Exemple:
        >>> grid = np.array([[1, 2, 3], [4, 5, 6]])
        >>> safe_slice(grid, slice(0, 2), slice(0, 2))  # [[1, 2], [4, 5]]
        >>> safe_slice(grid, slice(0, 5), slice(0, 5), default=0)  # Avec padding
    """
    h, w = grid.shape
    
    # Gérer slices None
    if row_slice is None:
        row_slice = slice(0, h)
    if col_slice is None:
        col_slice = slice(0, w)
    
    # Extraire indices start/stop
    row_start = row_slice.start if row_slice.start is not None else 0
    row_stop = row_slice.stop if row_slice.stop is not None else h
    col_start = col_slice.start if col_slice.start is not None else 0
    col_stop = col_slice.stop if col_slice.stop is not None else w
    
    return safe_get_region(grid, row_start, row_stop, col_start, col_stop, default)


def safe_index_array(grid: np.ndarray, 
                     row_indices: np.ndarray,
                     col_indices: np.ndarray,
                     default: Any = 0) -> np.ndarray:
    """
    Indexation avancée sécurisée
    
    Args:
        grid: Grille numpy
        row_indices: Array d'indices lignes
        col_indices: Array d'indices colonnes
        default: Valeur par défaut pour indices hors limites
        
    Returns:
        Array de valeurs extraites
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> safe_index_array(grid, np.array([0, 1, 5]), np.array([0, 1, 5]), default=-1)
        array([1, 4, -1])
    """
    h, w = grid.shape
    result = np.full(row_indices.shape, default, dtype=grid.dtype)
    
    # Masque pour indices valides
    valid_mask = (
        (row_indices >= 0) & (row_indices < h) &
        (col_indices >= 0) & (col_indices < w)
    )
    
    # Extraire valeurs valides
    if np.any(valid_mask):
        result[valid_mask] = grid[row_indices[valid_mask], col_indices[valid_mask]]
    
    return result


def safe_reshape(grid: np.ndarray, new_shape: Tuple[int, int], 
                 pad_value: Any = 0) -> np.ndarray:
    """
    Reshape sécurisé avec padding si nécessaire
    
    Args:
        grid: Grille numpy
        new_shape: Nouvelle forme (height, width)
        pad_value: Valeur pour padding
        
    Returns:
        Grille reshapée (avec padding/crop si nécessaire)
        
    Exemple:
        >>> grid = np.array([[1, 2], [3, 4]])
        >>> safe_reshape(grid, (3, 3), pad_value=0)
        array([[1, 2, 0],
               [3, 4, 0],
               [0, 0, 0]])
    """
    h, w = grid.shape
    new_h, new_w = new_shape
    
    # Créer nouvelle grille
    result = np.full(new_shape, pad_value, dtype=grid.dtype)
    
    # Copier données existantes (crop si nécessaire)
    copy_h = min(h, new_h)
    copy_w = min(w, new_w)
    result[:copy_h, :copy_w] = grid[:copy_h, :copy_w]
    
    return result


def safe_concatenate(grids: list, axis: int = 0, pad_value: Any = 0) -> np.ndarray:
    """
    Concaténation sécurisée avec padding automatique
    
    Args:
        grids: Liste de grilles numpy
        axis: Axe de concaténation (0=vertical, 1=horizontal)
        pad_value: Valeur pour padding si tailles différentes
        
    Returns:
        Grille concaténée
        
    Exemple:
        >>> g1 = np.array([[1, 2]])
        >>> g2 = np.array([[3, 4, 5]])
        >>> safe_concatenate([g1, g2], axis=0, pad_value=0)
        array([[1, 2, 0],
               [3, 4, 5]])
    """
    if not grids:
        return np.array([[]], dtype=np.int32)
    
    if len(grids) == 1:
        return grids[0].copy()
    
    # Trouver dimensions maximales
    if axis == 0:
        # Concaténation verticale: aligner largeurs
        max_width = max(g.shape[1] for g in grids)
        padded = [safe_reshape(g, (g.shape[0], max_width), pad_value) for g in grids]
    else:
        # Concaténation horizontale: aligner hauteurs
        max_height = max(g.shape[0] for g in grids)
        padded = [safe_reshape(g, (max_height, g.shape[1]), pad_value) for g in grids]
    
    return np.concatenate(padded, axis=axis)


# Export des fonctions principales
__all__ = [
    'validate_bounds',
    'safe_get_cell',
    'safe_set_cell',
    'safe_get_region',
    'safe_slice',
    'safe_index_array',
    'safe_reshape',
    'safe_concatenate'
]

# Made with Bob
