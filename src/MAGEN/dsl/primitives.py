"""
MAGEN DSL Primitives - Universal Transformation Operators for ARC-AGI

Ces primitives sont les opérateurs fondamentaux composables pour résoudre les puzzles ARC.
Chaque primitive est:
- Pure (pas d'effets de bord)
- Composable (peut être combinée avec d'autres)
- Testable (comportement déterministe)
- Documentée (avec exemples)
"""

import numpy as np
from typing import Tuple, List, Optional, Callable, Any
from scipy import ndimage
from dataclasses import dataclass


@dataclass(frozen=True)
class BoundingBox:
    """Représente une boîte englobante"""
    x: int
    y: int
    width: int
    height: int
    
    def to_slice(self) -> Tuple[slice, slice]:
        """Convertit en slices numpy"""
        return (
            slice(self.y, self.y + self.height),
            slice(self.x, self.x + self.width)
        )


# ============================================================================
# PRIMITIVES GÉOMÉTRIQUES
# ============================================================================

def rotate90(grid: np.ndarray) -> np.ndarray:
    """
    Rotation 90° sens horaire
    
    Exemple:
        Input:  [[1, 2],     Output: [[3, 1],
                 [3, 4]]              [4, 2]]
    """
    return np.rot90(grid, k=-1)


def rotate180(grid: np.ndarray) -> np.ndarray:
    """
    Rotation 180°
    
    Exemple:
        Input:  [[1, 2],     Output: [[4, 3],
                 [3, 4]]              [2, 1]]
    """
    return np.rot90(grid, k=2)


def rotate270(grid: np.ndarray) -> np.ndarray:
    """
    Rotation 270° sens horaire (= 90° anti-horaire)
    
    Exemple:
        Input:  [[1, 2],     Output: [[2, 4],
                 [3, 4]]              [1, 3]]
    """
    return np.rot90(grid, k=1)


def mirror_horizontal(grid: np.ndarray) -> np.ndarray:
    """
    Miroir horizontal (flip gauche-droite)
    
    Exemple:
        Input:  [[1, 2, 3],     Output: [[3, 2, 1],
                 [4, 5, 6]]              [6, 5, 4]]
    """
    return np.fliplr(grid)


def mirror_vertical(grid: np.ndarray) -> np.ndarray:
    """
    Miroir vertical (flip haut-bas)
    
    Exemple:
        Input:  [[1, 2, 3],     Output: [[4, 5, 6],
                 [4, 5, 6]]              [1, 2, 3]]
    """
    return np.flipud(grid)


def transpose(grid: np.ndarray) -> np.ndarray:
    """
    Transposition (échange lignes/colonnes)
    
    Exemple:
        Input:  [[1, 2],     Output: [[1, 3],
                 [3, 4]]              [2, 4]]
    """
    return grid.T


def scale(grid: np.ndarray, factor: int) -> np.ndarray:
    """
    Mise à l'échelle par répétition de pixels
    
    Args:
        grid: Grille d'entrée
        factor: Facteur de mise à l'échelle (>= 1)
    
    Exemple (factor=2):
        Input:  [[1, 2],     Output: [[1, 1, 2, 2],
                 [3, 4]]              [1, 1, 2, 2],
                                      [3, 3, 4, 4],
                                      [3, 3, 4, 4]]
    """
    if factor <= 0:
        raise ValueError(f"Scale factor must be positive, got {factor}")
    if factor == 1:
        return grid.copy()
    
    h, w = grid.shape
    result = np.zeros((h * factor, w * factor), dtype=grid.dtype)
    
    for i in range(h):
        for j in range(w):
            result[i*factor:(i+1)*factor, j*factor:(j+1)*factor] = grid[i, j]
    
    return result


def downscale(grid: np.ndarray, factor: int) -> np.ndarray:
    """
    Réduction par échantillonnage
    
    Args:
        grid: Grille d'entrée
        factor: Facteur de réduction (>= 1)
    
    Exemple (factor=2):
        Input:  [[1, 1, 2, 2],     Output: [[1, 2],
                 [1, 1, 2, 2],              [3, 4]]
                 [3, 3, 4, 4],
                 [3, 3, 4, 4]]
    """
    if factor <= 0:
        raise ValueError(f"Downscale factor must be positive, got {factor}")
    if factor == 1:
        return grid.copy()
    
    h, w = grid.shape
    if h % factor != 0 or w % factor != 0:
        raise ValueError(f"Grid dimensions ({h}x{w}) not divisible by factor {factor}")
    
    return grid[::factor, ::factor]


# ============================================================================
# PRIMITIVES SPATIALES
# ============================================================================

def crop(grid: np.ndarray, bbox: BoundingBox) -> np.ndarray:
    """
    Extraction d'une région rectangulaire
    
    Args:
        grid: Grille d'entrée
        bbox: Boîte englobante à extraire
    
    Exemple:
        Input:  [[1, 2, 3],     bbox=(1,1,2,1)
                 [4, 5, 6],     Output: [[5, 6]]
                 [7, 8, 9]]
    """
    slices = bbox.to_slice()
    return grid[slices].copy()


def translate(grid: np.ndarray, dx: int, dy: int, fill_value: int = 0) -> np.ndarray:
    """
    Translation avec remplissage
    
    Args:
        grid: Grille d'entrée
        dx: Déplacement horizontal (positif = droite)
        dy: Déplacement vertical (positif = bas)
        fill_value: Valeur de remplissage pour les zones vides
    
    Exemple (dx=1, dy=1):
        Input:  [[1, 2],     Output: [[0, 0, 0],
                 [3, 4]]              [0, 1, 2],
                                      [0, 3, 4]]
    """
    h, w = grid.shape
    result = np.full((h + abs(dy), w + abs(dx)), fill_value, dtype=grid.dtype)
    
    src_y = max(0, -dy)
    src_x = max(0, -dx)
    dst_y = max(0, dy)
    dst_x = max(0, dx)
    
    # Copie avec vérification des limites
    for i in range(h):
        for j in range(w):
            if 0 <= src_y + i < grid.shape[0] and 0 <= src_x + j < grid.shape[1]:
                result[dst_y + i, dst_x + j] = grid[src_y + i, src_x + j]
            else:
                result[dst_y + i, dst_x + j] = fill_value
    
    return result


def pad(grid: np.ndarray, top: int, bottom: int, left: int, right: int, 
        fill_value: int = 0) -> np.ndarray:
    """
    Ajout de bordures
    
    Args:
        grid: Grille d'entrée
        top, bottom, left, right: Tailles des bordures
        fill_value: Valeur de remplissage
    
    Exemple (top=1, left=1, fill_value=9):
        Input:  [[1, 2],     Output: [[9, 9, 9],
                 [3, 4]]              [9, 1, 2],
                                      [9, 3, 4]]
    """
    return np.pad(grid, ((top, bottom), (left, right)), 
                  mode='constant', constant_values=fill_value)


# ============================================================================
# PRIMITIVES COULEURS
# ============================================================================

def recolor(grid: np.ndarray, from_color: int, to_color: int) -> np.ndarray:
    """
    Remplacement de couleur
    
    Args:
        grid: Grille d'entrée
        from_color: Couleur à remplacer
        to_color: Nouvelle couleur
    
    Exemple (from_color=1, to_color=9):
        Input:  [[1, 2, 1],     Output: [[9, 2, 9],
                 [3, 1, 4]]              [3, 9, 4]]
    """
    result = grid.copy()
    result[grid == from_color] = to_color
    return result


def floodfill(grid: np.ndarray, start_pos: Tuple[int, int],
              new_color: int) -> np.ndarray:
    """
    Remplissage par propagation (flood fill)
    
    Args:
        grid: Grille d'entrée
        start_pos: Position de départ (y, x)
        new_color: Couleur de remplissage
    
    Exemple (start_pos=(0,0), new_color=9):
        Input:  [[1, 1, 2],     Output: [[9, 9, 2],
                 [1, 2, 2],              [9, 2, 2],
                 [3, 3, 3]]              [3, 3, 3]]
    """
    if not (0 <= start_pos[0] < grid.shape[0] and 0 <= start_pos[1] < grid.shape[1]):
        return grid.copy()
    
    old_color = grid[start_pos[0], start_pos[1]]
    if old_color == new_color:
        return grid.copy()
    
    result = grid.copy()
    stack = [start_pos]
    
    while stack:
        y, x = stack.pop()
        if not (0 <= y < grid.shape[0] and 0 <= x < grid.shape[1]):
            continue
        if result[y, x] != old_color:
            continue
            
        result[y, x] = new_color
        stack.extend([(y+1, x), (y-1, x), (y, x+1), (y, x-1)])
    
    return result


def recolor_mask(grid: np.ndarray, mask: np.ndarray, new_color: int) -> np.ndarray:
    """
    Recoloration selon un masque booléen
    
    Args:
        grid: Grille d'entrée
        mask: Masque booléen (True = recolorer)
        new_color: Nouvelle couleur
    
    Exemple:
        Input:  [[1, 2, 3],     mask: [[T, F, T],
                 [4, 5, 6]]             [F, T, F]]
        Output: [[9, 2, 9],     (new_color=9)
                 [4, 9, 6]]
    """
    result = grid.copy()
    result[mask] = new_color
    return result


# ============================================================================
# PRIMITIVES OBJETS
# ============================================================================

def extract_objects(grid: np.ndarray, background: int = 0, 
                   connectivity: int = 2) -> List[Tuple[np.ndarray, BoundingBox]]:
    """
    Extraction d'objets connectés
    
    Args:
        grid: Grille d'entrée
        background: Couleur de fond à ignorer
        connectivity: 1 (4-connectivité) ou 2 (8-connectivité)
    
    Returns:
        Liste de (objet_masqué, bounding_box)
    
    Exemple:
        Input:  [[1, 0, 2],     background=0
                 [1, 0, 2],     
                 [0, 0, 0]]
        Output: 2 objets détectés
    """
    # Créer masque binaire (non-background)
    binary = (grid != background).astype(int)
    
    # Labelliser composantes connectées
    labeled, num_features = ndimage.label(binary, 
                                         structure=ndimage.generate_binary_structure(2, connectivity))
    
    objects = []
    for label in range(1, num_features + 1):
        mask = (labeled == label)
        
        # Trouver bounding box
        rows, cols = np.where(mask)
        if len(rows) == 0:
            continue
        
        bbox = BoundingBox(
            x=int(cols.min()),
            y=int(rows.min()),
            width=int(cols.max() - cols.min() + 1),
            height=int(rows.max() - rows.min() + 1)
        )
        
        # Extraire objet avec masque
        obj_grid = grid.copy()
        obj_grid[~mask] = background
        
        objects.append((obj_grid, bbox))
    
    return objects


def filter_objects_by_size(objects: List[Tuple[np.ndarray, BoundingBox]], 
                          min_size: int = 1, 
                          max_size: Optional[int] = None) -> List[Tuple[np.ndarray, BoundingBox]]:
    """
    Filtrage d'objets par taille
    
    Args:
        objects: Liste d'objets (grid, bbox)
        min_size: Taille minimale (en pixels)
        max_size: Taille maximale (None = pas de limite)
    
    Returns:
        Objets filtrés
    """
    filtered = []
    for obj_grid, bbox in objects:
        size = bbox.width * bbox.height
        if size >= min_size and (max_size is None or size <= max_size):
            filtered.append((obj_grid, bbox))
    return filtered


# ============================================================================
# PRIMITIVES PATTERNS
# ============================================================================

def repeat_pattern(pattern: np.ndarray, rows: int, cols: int) -> np.ndarray:
    """
    Répétition d'un pattern en grille
    
    Args:
        pattern: Pattern à répéter
        rows: Nombre de répétitions verticales
        cols: Nombre de répétitions horizontales
    
    Exemple (rows=2, cols=3):
        Input:  [[1, 2]]     Output: [[1, 2, 1, 2, 1, 2],
                                      [1, 2, 1, 2, 1, 2]]
    """
    return np.tile(pattern, (rows, cols))


def tile_pattern(pattern: np.ndarray, target_shape: Tuple[int, int]) -> np.ndarray:
    """
    Pavage d'un pattern pour remplir une forme cible
    
    Args:
        pattern: Pattern à paver
        target_shape: Forme cible (height, width)
    
    Returns:
        Grille pavée (peut être tronquée si nécessaire)
    """
    ph, pw = pattern.shape
    th, tw = target_shape
    
    # Calculer répétitions nécessaires
    rows = (th + ph - 1) // ph
    cols = (tw + pw - 1) // pw
    
    # Répéter et tronquer
    tiled = repeat_pattern(pattern, rows, cols)
    return tiled[:th, :tw]


def detect_symmetry(grid: np.ndarray) -> dict:
    """
    Détection de symétries
    
    Returns:
        Dict avec clés: 'horizontal', 'vertical', 'diagonal', 'rotational'
    """
    return {
        'horizontal': np.array_equal(grid, mirror_horizontal(grid)),
        'vertical': np.array_equal(grid, mirror_vertical(grid)),
        'diagonal': np.array_equal(grid, transpose(grid)),
        'rotational_90': np.array_equal(grid, rotate90(grid)),
        'rotational_180': np.array_equal(grid, rotate180(grid))
    }


# ============================================================================
# PRIMITIVES LOGIQUES
# ============================================================================

def compose(*operations: Callable) -> Callable:
    """
    Composition de fonctions (droite vers gauche)
    
    Exemple:
        f = compose(rotate90, mirror_h)
        result = f(grid)  # Équivalent à rotate90(mirror_h(grid))
    """
    def composed(grid: np.ndarray) -> np.ndarray:
        result = grid
        for op in reversed(operations):
            result = op(result)
        return result
    return composed


def apply_if(condition: Callable[[np.ndarray], bool], 
            true_op: Callable, 
            false_op: Optional[Callable] = None) -> Callable:
    """
    Application conditionnelle d'opération
    
    Args:
        condition: Fonction testant la grille
        true_op: Opération si condition vraie
        false_op: Opération si condition fausse (None = identité)
    """
    def conditional(grid: np.ndarray) -> np.ndarray:
        if condition(grid):
            return true_op(grid)
        elif false_op is not None:
            return false_op(grid)
        else:
            return grid.copy()
    return conditional


# ============================================================================
# UTILITAIRES
# ============================================================================

def get_bounding_box(grid: np.ndarray, background: int = 0) -> Optional[BoundingBox]:
    """
    Calcul de la bounding box du contenu non-background
    
    Returns:
        BoundingBox ou None si grille vide
    """
    mask = (grid != background)
    rows, cols = np.where(mask)
    
    if len(rows) == 0:
        return None
    
    return BoundingBox(
        x=int(cols.min()),
        y=int(rows.min()),
        width=int(cols.max() - cols.min() + 1),
        height=int(rows.max() - rows.min() + 1)
    )


def count_colors(grid: np.ndarray) -> dict:
    """
    Comptage des couleurs présentes
    
    Returns:
        Dict {couleur: nombre_pixels}
    """
    unique, counts = np.unique(grid, return_counts=True)
    return dict(zip(unique.tolist(), counts.tolist()))


def most_common_color(grid: np.ndarray, exclude_background: bool = True, 
                     background: int = 0) -> int:
    """
    Couleur la plus fréquente
    
    Args:
        grid: Grille d'entrée
        exclude_background: Ignorer la couleur de fond
        background: Couleur de fond à ignorer
    """
    colors = count_colors(grid)
    
    if exclude_background and background in colors:
        del colors[background]
    
    if not colors:
        return background
    
    return max(colors.items(), key=lambda x: x[1])[0]

def extract_repeated_pattern(grid: List[List[int]]) -> List[List[int]]:
    """
    Extrait le pattern de base d'une grille répétée.
    
    Détecte si la grille est composée d'un pattern répété horizontalement
    et/ou verticalement, et retourne le pattern de base.
    
    Exemple: [[1,2,1,2,1,2]] → [[1,2]]
    Exemple: [[1,2],[3,4],[1,2],[3,4]] → [[1,2],[3,4]]
    """
    if not grid or not grid[0]:
        return grid
    
    h, w = len(grid), len(grid[0])
    
    # Tester répétitions horizontales (diviseurs de w)
    for pattern_w in range(1, w // 2 + 1):
        if w % pattern_w == 0:
            repetitions = w // pattern_w
            # Vérifier si pattern se répète (avec safe_get_cell)
            is_repeated = True
            for i in range(h):
                for rep in range(1, repetitions):
                    for j in range(pattern_w):
                        val1 = grid[i][j] if i < len(grid) and j < len(grid[i]) else None
                        idx2 = rep * pattern_w + j
                        val2 = grid[i][idx2] if i < len(grid) and idx2 < len(grid[i]) else None
                        if val1 != val2:
                            is_repeated = False
                            break
                    if not is_repeated:
                        break
                if not is_repeated:
                    break
            
            if is_repeated:
                # Extraire pattern horizontal
                pattern = [row[:pattern_w] for row in grid]
                
                # Tester répétitions verticales sur le pattern
                pattern_h = len(pattern)
                for test_h in range(1, pattern_h // 2 + 1):
                    if pattern_h % test_h == 0:
                        v_reps = pattern_h // test_h
                        is_v_repeated = True
                        for rep in range(1, v_reps):
                            for i in range(test_h):
                                if pattern[i] != pattern[rep * test_h + i]:
                                    is_v_repeated = False
                                    break
                            if not is_v_repeated:
                                break
                        
                        if is_v_repeated:
                            return pattern[:test_h]
                
                return pattern
    
    # Tester répétitions verticales uniquement
    for pattern_h in range(1, h // 2 + 1):
        if h % pattern_h == 0:
            repetitions = h // pattern_h
            is_repeated = True
            for rep in range(1, repetitions):
                for i in range(pattern_h):
                    if grid[i] != grid[rep * pattern_h + i]:
                        is_repeated = False
                        break
                if not is_repeated:
                    break
            
            if is_repeated:
                return grid[:pattern_h]
    
    # Pas de répétition détectée
    return grid


# ============================================================================
# EXPORT
# ============================================================================

__all__ = [
    # Géométriques
    'rotate90', 'rotate180', 'rotate270',
    'mirror_horizontal', 'mirror_vertical',
    'transpose', 'scale', 'downscale',
    
    # Spatiales
    'crop', 'translate', 'pad',
    
    # Couleurs
    'recolor', 'floodfill', 'recolor_mask',
    
    # Objets
    'extract_objects', 'filter_objects_by_size',
    
    # Patterns
    'repeat_pattern', 'tile_pattern', 'detect_symmetry', 'extract_repeated_pattern',
    
    # Logiques
    'compose', 'apply_if',
    
    # Utilitaires
    'BoundingBox', 'get_bounding_box', 'count_colors', 'most_common_color'
]

# Made with Bob
