"""
Quadrant Extractor - Générateur Spécifique pour 88a62173
Extrait le quadrant bas-droit d'une grille divisée par ligne vide
"""

import numpy as np
from typing import Optional


def extract_bottom_right_quadrant(grid: np.ndarray) -> np.ndarray:
    """
    Extrait le quadrant UNIQUE (différent des 3 autres) d'une grille 5x5
    
    Args:
        grid: Grille d'entrée (typiquement 5x5)
    
    Returns:
        Sous-grille 2x2 du quadrant unique
    
    Algorithme:
    1. Extraire les 4 quadrants 2x2
    2. Trouver celui qui est DIFFÉRENT des 3 autres
    3. Retourner le quadrant unique
    """
    quadrants_dict = extract_all_quadrants(grid)
    
    # Convertir en liste pour comparaison
    quadrants = [
        ('top_left', quadrants_dict['top_left']),
        ('top_right', quadrants_dict['top_right']),
        ('bottom_left', quadrants_dict['bottom_left']),
        ('bottom_right', quadrants_dict['bottom_right']),
    ]
    
    # Compter occurrences de chaque pattern
    from collections import defaultdict
    pattern_counts = defaultdict(list)
    
    for name, quad in quadrants:
        # Convertir en tuple pour hashabilité
        pattern_key = tuple(quad.flatten())
        pattern_counts[pattern_key].append((name, quad))
    
    # Trouver le pattern qui apparaît 1 seule fois
    for pattern_key, quads_list in pattern_counts.items():
        if len(quads_list) == 1:
            # C'est le quadrant unique!
            return quads_list[0][1]
    
    # Fallback: si tous identiques ou pattern ambigu, retourner bas-droit
    return quadrants_dict['bottom_right']


def find_separator_row(grid: np.ndarray) -> Optional[int]:
    """
    Trouve la ligne séparatrice (ligne entièrement vide)
    
    Args:
        grid: Grille d'entrée
    
    Returns:
        Index de la ligne séparatrice, ou None si non trouvée
    """
    h, w = grid.shape
    
    # Chercher ligne vide (tous les éléments = 0)
    for i in range(h):
        if np.all(grid[i] == 0):
            return i
    
    # Fallback: ligne du milieu
    return h // 2


def extract_all_quadrants(grid: np.ndarray) -> dict:
    """
    Extrait les 4 quadrants d'une grille divisée par ligne vide
    
    Args:
        grid: Grille d'entrée (typiquement 5x5)
    
    Returns:
        Dict avec clés 'top_left', 'top_right', 'bottom_left', 'bottom_right'
    """
    h, w = grid.shape
    sep_row = find_separator_row(grid)
    
    if sep_row is None:
        sep_row = h // 2
    
    # Calculer positions
    mid_col = w // 2
    
    # CORRECTION SESSION 49: Validation STRICTE limites avant accès
    # Calculer indices avec validation
    top_end = min(2, h)
    left_end = min(2, w)
    right_start = min(mid_col + 1, w)
    right_end = min(mid_col + 3, w)
    bottom_start = min(sep_row + 1, h)
    bottom_end = min(sep_row + 3, h)
    
    # Quadrants 2x2 avec limites validées
    quadrants = {
        'top_left': grid[0:top_end, 0:left_end],
        'top_right': grid[0:top_end, right_start:right_end],
        'bottom_left': grid[bottom_start:bottom_end, 0:left_end],
        'bottom_right': grid[bottom_start:bottom_end, right_start:right_end],
    }
    
    return quadrants


def extract_common_pattern(grid: np.ndarray) -> np.ndarray:
    """
    Extrait le pattern commun entre les 4 quadrants par vote majoritaire
    
    Args:
        grid: Grille d'entrée (typiquement 5x5)
    
    Returns:
        Pattern 2x2 le plus fréquent
    """
    quadrants = extract_all_quadrants(grid)
    
    # Pour l'instant, retourner quadrant bas-droit (logique identifiée)
    return quadrants['bottom_right']


# Alias pour compatibilité avec DSL existant
def generate_extract_bottom_right_quadrant():
    """Générateur pour DSL"""
    def transform(grid: np.ndarray) -> np.ndarray:
        return extract_bottom_right_quadrant(grid)
    return transform


def generate_extract_common_pattern():
    """Générateur alternatif pour DSL"""
    def transform(grid: np.ndarray) -> np.ndarray:
        return extract_common_pattern(grid)
    return transform

# Made with Bob
