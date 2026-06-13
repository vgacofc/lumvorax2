"""
Shape Corner Marker - Générateur Spécifique pour 3aa6fb7a
Marque les coins des formes en L avec une couleur spécifiée
"""

import numpy as np
from typing import List, Tuple, Set, Optional
from scipy.ndimage import label


def mark_shape_corners(grid: np.ndarray, target_color: int = 8, mark_color: int = 1) -> np.ndarray:
    """
    Marque les coins des formes en L avec couleur spécifiée
    
    Args:
        grid: Grille d'entrée
        target_color: Couleur des formes à analyser (défaut: 8)
        mark_color: Couleur pour marquer les coins (défaut: 1)
    
    Returns:
        Grille avec coins marqués
    
    Algorithme:
    1. Détecter formes (connected components)
    2. Pour chaque forme, identifier si L ou angle
    3. Calculer position coin (angle 90°)
    4. Marquer coin avec mark_color
    """
    output = grid.copy()
    h, w = grid.shape
    
    # Créer masque binaire pour target_color
    binary_mask = (grid == target_color).astype(int)
    
    # Détecter connected components
    labeled_array, num_features = label(binary_mask)
    
    # Pour chaque forme détectée
    for shape_id in range(1, num_features + 1):
        shape_mask = (labeled_array == shape_id)
        shape_positions = np.argwhere(shape_mask)
        
        if len(shape_positions) < 2:
            continue  # Forme trop petite
        
        # Trouver coin de la forme
        corner = find_L_corner(shape_positions, (h, w))
        
        if corner is not None:
            i, j = corner
            # Marquer seulement si position vide avec safe_set_cell
            if validate_bounds(output, i, j) and output[i, j] == 0:
                safe_set_cell(output, i, j, mark_color)
    
    return output


def find_L_corner(positions: np.ndarray, grid_shape: Tuple[int, int]) -> Optional[Tuple[int, int]]:
    """
    Trouve le coin d'une forme en L
    
    Le coin est la position vide adjacente à 2 cellules formant angle 90°
    
    Args:
        positions: Positions (i,j) des cellules de la forme
        grid_shape: Dimensions de la grille
    
    Returns:
        Position (i,j) du coin, ou None si non trouvé
    """
    h, w = grid_shape
    positions_set = set(map(tuple, positions))
    
    # Pour chaque position de la forme
    for pos in positions:
        i, j = pos
        
        # Vérifier les 4 positions adjacentes diagonales (coins potentiels)
        diagonal_neighbors = [
            (i-1, j-1),  # Haut-gauche
            (i-1, j+1),  # Haut-droit
            (i+1, j-1),  # Bas-gauche
            (i+1, j+1),  # Bas-droit
        ]
        
        for corner_i, corner_j in diagonal_neighbors:
            if not (0 <= corner_i < h and 0 <= corner_j < w):
                continue
            
            if (corner_i, corner_j) in positions_set:
                continue  # Déjà occupé par la forme
            
            # Vérifier si cette position forme un angle avec 2 cellules de la forme
            # Les 2 cellules doivent être orthogonales (pas diagonales)
            orthogonal_neighbors = [
                (corner_i, j),      # Même colonne que pos original
                (i, corner_j),      # Même ligne que pos original
            ]
            
            # Compter combien de voisins orthogonaux sont dans la forme
            count = sum(1 for n in orthogonal_neighbors if n in positions_set)
            
            if count == 2:
                # C'est un coin valide!
                return (corner_i, corner_j)
    
    # Fallback: Trouver position vide la plus proche du centre de masse
    center_i = np.mean(positions[:, 0])
    center_j = np.mean(positions[:, 1])
    
    # Chercher dans un rayon de 2 autour du centre
    for di in [-1, 0, 1]:
        for dj in [-1, 0, 1]:
            if di == 0 and dj == 0:
                continue
            
            corner_i = int(center_i + di)
            corner_j = int(center_j + dj)
            
            if not (0 <= corner_i < h and 0 <= corner_j < w):
                continue
            
            if (corner_i, corner_j) not in positions_set:
                # Vérifier si adjacent à au moins 2 cellules de la forme
                adjacent_count = 0
                for ni, nj in [(corner_i-1, corner_j), (corner_i+1, corner_j),
                               (corner_i, corner_j-1), (corner_i, corner_j+1)]:
                    if (ni, nj) in positions_set:
                        adjacent_count += 1
                
                if adjacent_count >= 2:
                    return (corner_i, corner_j)
    
    return None


def is_L_shape(positions: np.ndarray) -> bool:
    """
    Vérifie si une forme ressemble à un L
    
    Critères:
    - Au moins 3 cellules
    - Forme non-linéaire (pas alignée)
    - Forme connexe
    """
    if len(positions) < 3:
        return False
    
    # Vérifier si aligné (tous sur même ligne ou colonne)
    rows = positions[:, 0]
    cols = positions[:, 1]
    
    if len(set(rows)) == 1 or len(set(cols)) == 1:
        return False  # Aligné = pas un L
    
    return True


# Alias pour compatibilité avec DSL existant
def generate_mark_shape_corners(target_color: int = 8, mark_color: int = 1):
    """Générateur pour DSL"""
    def transform(grid: np.ndarray) -> np.ndarray:
        return mark_shape_corners(grid, target_color, mark_color)
    return transform

# Made with Bob
