"""
MAGEN DSL - Primitives Avancées pour puzzles complexes
Primitives spécialisées pour résoudre les 4 échecs identifiés

CORRECTION V20 SESSION 57:
- Import safe_operations pour éviter IndexError
"""

import numpy as np
from typing import Tuple, List, Optional
from collections import Counter
from .safe_operations import validate_bounds, safe_get_cell, safe_set_cell


def compress_periodic_pattern(grid: np.ndarray) -> np.ndarray:
    """
    Compresse un pattern répété périodiquement
    
    Détecte si la grille contient un pattern répété horizontalement
    et extrait une seule instance du pattern.
    
    Exemple: [A B C | A B C | A B C] → [A B C]
    
    Args:
        grid: Grille avec pattern répété
        
    Returns:
        Pattern compressé (une seule instance)
    """
    h, w = grid.shape
    
    # Tester différentes largeurs de pattern (diviseurs de w)
    for pattern_width in range(1, w + 1):
        if w % pattern_width != 0:
            continue
        
        num_repetitions = w // pattern_width
        if num_repetitions == 1:
            continue
        
        # Extraire le premier pattern
        first_pattern = grid[:, :pattern_width]
        
        # Vérifier si ce pattern se répète
        is_repeated = True
        for i in range(1, num_repetitions):
            start_col = i * pattern_width
            end_col = start_col + pattern_width
            current_pattern = grid[:, start_col:end_col]
            
            if not np.array_equal(first_pattern, current_pattern):
                is_repeated = False
                break
        
        if is_repeated:
            return first_pattern
    
    # Pas de pattern répété détecté, retourner grille originale
    return grid


def extract_periodic_columns(grid: np.ndarray, period: int) -> np.ndarray:
    """
    Extrait colonnes périodiques sans vérifier répétition exacte
    
    Extrait les N premières colonnes où N = largeur / period.
    Utile pour puzzles où le pattern n'est pas exactement répété
    mais la structure périodique est présente.
    
    Exemple: grid (3, 9) avec period=3 → (3, 3) [colonnes 0-2]
    
    Args:
        grid: Grille d'entrée
        period: Période (facteur de division)
        
    Returns:
        Grille avec colonnes extraites
    """
    h, w = grid.shape
    
    if w % period != 0:
        # Si pas divisible, retourner original
        return grid
    
    target_width = w // period
    return grid[:, :target_width]


def detect_and_mark_L_shapes(grid: np.ndarray,
                             target_color: Optional[int] = None,
                             mark_color: int = 5) -> np.ndarray:
    """
    Détecte les formes en L et marque leurs coins
    
    Une forme L est définie comme:
    - 3 cellules formant un angle droit
    - Configurations: ┐ ┌ └ ┘
    
    Args:
        grid: Grille d'entrée
        target_color: Couleur à chercher (None = toutes sauf 0)
        mark_color: Couleur pour marquer les coins
        
    Returns:
        Grille avec coins marqués
    """
    result = grid.copy()
    h, w = grid.shape
    
    # Patterns de formes L (relatif au coin)
    # Format: [(dy1, dx1), (dy2, dx2)] - les 2 autres cellules du L
    l_patterns = [
        [(0, 1), (1, 0)],   # ┌ (coin haut-gauche)
        [(0, -1), (1, 0)],  # ┐ (coin haut-droite)
        [(0, 1), (-1, 0)],  # └ (coin bas-gauche)
        [(0, -1), (-1, 0)]  # ┘ (coin bas-droite)
    ]
    
    for y in range(h):
        for x in range(w):
            # CORRECTION SESSION 52: Utilisation de safe_get_cell
            cell_color = safe_get_cell(grid, y, x, default=0)
            
            # Ignorer fond (0) ou vérifier target_color
            if cell_color == 0:
                continue
            if target_color is not None and cell_color != target_color:
                continue
            
            # Tester chaque pattern de L
            for pattern in l_patterns:
                is_l_shape = True
                
                for dy, dx in pattern:
                    ny, nx = y + dy, x + dx
                    
                    # CORRECTION SESSION 52: Validation avec validate_bounds
                    if not validate_bounds(grid, ny, nx):
                        is_l_shape = False
                        break
                    
                    # Vérifier même couleur avec safe_get_cell
                    neighbor_color = safe_get_cell(grid, ny, nx, default=-1)
                    if neighbor_color != cell_color:
                        is_l_shape = False
                        break
                
                if is_l_shape:
                    # Marquer le coin avec safe_set_cell
                    safe_set_cell(result, y, x, mark_color)
                    break
    
    return result


def extract_semantic_region(grid: np.ndarray,
                            region_type: str = 'largest_object') -> np.ndarray:
    """
    Extrait une région sémantique spécifique de la grille
    
    Args:
        grid: Grille d'entrée
        region_type: Type de région à extraire
            - 'largest_object': Plus grand objet connecté
            - 'top_left_quadrant': Quadrant haut-gauche
            - 'top_right_quadrant': Quadrant haut-droit
            - 'bottom_left_quadrant': Quadrant bas-gauche
            - 'bottom_right_quadrant': Quadrant bas-droit
            - 'center': Région centrale
            
    Returns:
        Région extraite (peut être plus petite que l'entrée)
    """
    h, w = grid.shape
    
    if region_type == 'top_left_quadrant':
        # Extraire quadrant haut-gauche
        mid_h = h // 2
        mid_w = w // 2
        return grid[:mid_h, :mid_w]
    
    elif region_type == 'top_right_quadrant':
        # Extraire quadrant haut-droit
        mid_h = h // 2
        mid_w = w // 2
        return grid[:mid_h, mid_w:]
    
    elif region_type == 'bottom_left_quadrant':
        # Extraire quadrant bas-gauche
        mid_h = h // 2
        mid_w = w // 2
        return grid[mid_h:, :mid_w]
    
    elif region_type == 'bottom_right_quadrant':
        # Extraire quadrant bas-droit
        mid_h = h // 2
        mid_w = w // 2
        return grid[mid_h:, mid_w:]
    
    elif region_type == 'center':
        # Extraire région centrale (50% de chaque dimension)
        start_h = h // 4
        end_h = start_h + (h // 2)
        start_w = w // 4
        end_w = start_w + (w // 2)
        return grid[start_h:end_h, start_w:end_w]
    
    elif region_type == 'largest_object':
        # Trouver le plus grand objet connecté (non-zéro)
        try:
            from scipy import ndimage
            
            # Créer masque binaire (non-zéro = objet)
            binary = (grid != 0).astype(int)
            
            # Labelliser objets connectés
            labeled_result = ndimage.label(binary)
            labeled = labeled_result[0]
            num_objects = labeled_result[1]
        except ImportError:
            # Fallback si scipy pas disponible
            return grid
        
        if num_objects == 0:
            return grid
        
        # Trouver le plus grand objet
        object_sizes = []
        for obj_id in range(1, num_objects + 1):
            size = np.sum(labeled == obj_id)
            object_sizes.append((size, obj_id))
        
        largest_size, largest_id = max(object_sizes)
        
        # Extraire bounding box du plus grand objet
        obj_mask = (labeled == largest_id)
        rows = np.any(obj_mask, axis=1)
        cols = np.any(obj_mask, axis=0)
        
        y_min, y_max = np.where(rows)[0][[0, -1]]
        x_min, x_max = np.where(cols)[0][[0, -1]]
        
        return grid[y_min:y_max+1, x_min:x_max+1]
    
    return grid


def sample_grid_non_integer(grid: np.ndarray, 
                            target_shape: Tuple[int, int]) -> np.ndarray:
    """
    Downscale avec ratio non-entier en échantillonnant
    
    Utilise échantillonnage nearest-neighbor pour ratios non-entiers.
    Exemple: 5x5 → 2x2 (ratio 2.5)
    
    Args:
        grid: Grille d'entrée
        target_shape: (height, width) cible
        
    Returns:
        Grille échantillonnée
    """
    h_in, w_in = grid.shape
    h_out, w_out = target_shape
    
    result = np.zeros(target_shape, dtype=grid.dtype)
    
    # Calculer ratios
    ratio_h = h_in / h_out
    ratio_w = w_in / w_out
    
    # Échantillonner avec safe_get_cell
    for y_out in range(h_out):
        for x_out in range(w_out):
            # Trouver position correspondante dans grille source
            y_in = int(y_out * ratio_h)
            x_in = int(x_out * ratio_w)
            
            # Assurer limites
            y_in = min(y_in, h_in - 1)
            x_in = min(x_in, w_in - 1)
            
            # Utilisation de safe_get_cell
            value = safe_get_cell(grid, y_in, x_in, default=0)
            safe_set_cell(result, y_out, x_out, value)
    
    return result


def downsample_by_majority(grid: np.ndarray, factor: int = 2) -> np.ndarray:
    """
    Downscale en prenant la couleur majoritaire dans chaque bloc
    
    Plus robuste que moyenne pour grilles avec couleurs discrètes.
    
    Args:
        grid: Grille d'entrée
        factor: Facteur de réduction (2 = diviser par 2)
        
    Returns:
        Grille réduite
    """
    h, w = grid.shape
    
    # Calculer nouvelle taille
    new_h = h // factor
    new_w = w // factor
    
    result = np.zeros((new_h, new_w), dtype=grid.dtype)
    
    for y in range(new_h):
        for x in range(new_w):
            # Extraire bloc avec validation
            y_start = y * factor
            y_end = min(y_start + factor, h)
            x_start = x * factor
            x_end = min(x_start + factor, w)
            
            block = grid[y_start:y_end, x_start:x_end]
            
            # Trouver couleur majoritaire
            colors, counts = np.unique(block, return_counts=True)
            majority_idx = np.argmax(counts)
            safe_set_cell(result, y, x, colors[majority_idx])
    
    return result


# Générateurs d'arguments pour les nouvelles primitives

def gen_compress_args(grid: np.ndarray) -> List[tuple]:
    """Pas d'arguments pour compress_periodic_pattern"""
    return [()]


def gen_detect_L_args(grid: np.ndarray) -> List[tuple]:
    """Générer arguments pour detect_and_mark_L_shapes"""
    args_list = []
    
    # Sans target_color (toutes couleurs)
    args_list.append(())
    
    # Avec chaque couleur présente
    unique_colors = np.unique(grid)
    for color in unique_colors:
        if color != 0:  # Ignorer fond
            args_list.append((color,))
    
    return args_list


def gen_extract_region_args(grid: np.ndarray) -> List[tuple]:
    """Générer arguments pour extract_semantic_region"""
    return [
        ('largest_object',),
        ('top_left_quadrant',),
        ('center',)
    ]


def gen_extract_periodic_args(grid: np.ndarray) -> List[tuple]:
    """Générer arguments pour extract_periodic_columns"""
    h, w = grid.shape
    args_list = []
    
    # Tester différentes périodes (diviseurs de w)
    for period in range(2, w + 1):
        if w % period == 0:
            args_list.append((period,))
    
    return args_list


def gen_sample_non_integer_args(grid: np.ndarray) -> List[tuple]:
    """Générer arguments pour sample_grid_non_integer"""
    h, w = grid.shape
    args_list = []
    
    # Tester différentes tailles cibles
    for target_h in range(1, min(h, 10)):
        for target_w in range(1, min(w, 10)):
            # Éviter ratio 1:1
            if target_h != h or target_w != w:
                args_list.append(((target_h, target_w),))
    
    # Retourner au moins une liste vide si aucun argument
    return args_list[:20] if args_list else []


def gen_downsample_majority_args(grid: np.ndarray) -> List[tuple]:
    """Générer arguments pour downsample_by_majority"""
    return [(2,), (3,), (4,), (5,)]

# Made with Bob
