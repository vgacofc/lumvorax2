"""
Tests pour ARCTransformations
==============================

Teste toutes les transformations géométriques, spatiales et couleur.

Date: 2026-06-15
Version: V38.1
"""

import numpy as np
import sys
from pathlib import Path

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.transformations import ARCTransformations, apply_transformation


def test_geometric_transforms():
    """Test transformations géométriques."""
    print("\n=== TEST TRANSFORMATIONS GÉOMÉTRIQUES ===")
    
    # Grille test 3x3
    grid = np.array([
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
    ])
    
    print(f"Grille originale:\n{grid}\n")
    
    # Test rotate_90
    result = ARCTransformations.rotate_90(grid)
    print(f"rotate_90:\n{result.grid}")
    assert result.success
    assert result.grid.shape == (3, 3)
    assert result.grid[0, 0] == 7  # Coin supérieur gauche après rotation
    
    # Test rotate_180
    result = ARCTransformations.rotate_180(grid)
    print(f"rotate_180:\n{result.grid}")
    assert result.success
    assert result.grid[0, 0] == 9
    
    # Test rotate_270
    result = ARCTransformations.rotate_270(grid)
    print(f"rotate_270:\n{result.grid}")
    assert result.success
    assert result.grid[0, 0] == 3
    
    # Test flip_horizontal
    result = ARCTransformations.flip_horizontal(grid)
    print(f"flip_horizontal:\n{result.grid}")
    assert result.success
    assert result.grid[0, 0] == 3
    assert result.grid[0, 2] == 1
    
    # Test flip_vertical
    result = ARCTransformations.flip_vertical(grid)
    print(f"flip_vertical:\n{result.grid}")
    assert result.success
    assert result.grid[0, 0] == 7
    assert result.grid[2, 0] == 1
    
    # Test transpose
    result = ARCTransformations.transpose(grid)
    print(f"transpose:\n{result.grid}")
    assert result.success
    assert result.grid[0, 1] == 4
    assert result.grid[1, 0] == 2
    
    print("✅ Toutes les transformations géométriques OK")


def test_spatial_transforms():
    """Test transformations spatiales."""
    print("\n=== TEST TRANSFORMATIONS SPATIALES ===")
    
    # Grille avec background
    grid = np.array([
        [0, 0, 0, 0, 0],
        [0, 1, 2, 3, 0],
        [0, 4, 5, 6, 0],
        [0, 0, 0, 0, 0]
    ])
    
    print(f"Grille originale:\n{grid}\n")
    
    # Test crop_nonzero
    result = ARCTransformations.crop_nonzero(grid)
    print(f"crop_nonzero:\n{result.grid}")
    print(f"bbox: {result.metadata['bbox']}")
    assert result.success
    assert result.grid.shape == (2, 3)
    assert result.grid[0, 0] == 1
    
    # Test extend
    small_grid = np.array([[1, 2], [3, 4]])
    result = ARCTransformations.extend(small_grid, factor=2)
    print(f"extend (factor=2):\n{result.grid}")
    assert result.success
    assert result.grid.shape == (4, 4)
    
    # Test tile
    result = ARCTransformations.tile(small_grid, rows=2, cols=3)
    print(f"tile (2x3):\n{result.grid}")
    assert result.success
    assert result.grid.shape == (4, 6)
    
    # Test scale_up
    result = ARCTransformations.scale_up(small_grid, factor=3)
    print(f"scale_up (factor=3):\n{result.grid}")
    assert result.success
    assert result.grid.shape == (6, 6)
    
    print("✅ Toutes les transformations spatiales OK")


def test_color_transforms():
    """Test transformations couleur."""
    print("\n=== TEST TRANSFORMATIONS COULEUR ===")
    
    grid = np.array([
        [0, 1, 2],
        [3, 4, 5],
        [6, 7, 8]
    ])
    
    print(f"Grille originale:\n{grid}\n")
    
    # Test invert_colors
    result = ARCTransformations.invert_colors(grid, max_color=9)
    print(f"invert_colors:\n{result.grid}")
    assert result.success
    assert result.grid[0, 0] == 9
    assert result.grid[2, 2] == 1
    
    # Test map_colors
    color_map = {1: 9, 2: 8, 3: 7}
    result = ARCTransformations.map_colors(grid, color_map)
    print(f"map_colors {color_map}:\n{result.grid}")
    assert result.success
    assert result.grid[0, 1] == 9
    assert result.grid[0, 2] == 8
    
    # Test filter_color
    result = ARCTransformations.filter_color(grid, keep_color=5)
    print(f"filter_color (keep=5):\n{result.grid}")
    assert result.success
    assert result.grid[1, 2] == 5  # Position correcte de 5 dans grille
    assert result.grid[0, 0] == 0
    
    print("✅ Toutes les transformations couleur OK")


def test_apply_transformation():
    """Test fonction apply_transformation."""
    print("\n=== TEST APPLY_TRANSFORMATION ===")
    
    grid = np.array([[1, 2], [3, 4]])
    
    # Test par nom
    result = apply_transformation(grid, 'rotate_90')
    assert result.success
    print(f"rotate_90 via apply_transformation: OK")
    
    result = apply_transformation(grid, 'flip_horizontal')
    assert result.success
    print(f"flip_horizontal via apply_transformation: OK")
    
    # Test transformation inconnue
    result = apply_transformation(grid, 'unknown_transform')
    assert not result.success
    print(f"unknown_transform détecté: OK")
    
    print("✅ apply_transformation OK")


def test_get_all_transforms():
    """Test récupération de toutes les transformations."""
    print("\n=== TEST GET_ALL_TRANSFORMS ===")
    
    grid = np.array([[1, 2], [3, 4]])
    
    # Géométriques
    geo_transforms = ARCTransformations.get_all_geometric_transforms(grid)
    print(f"Transformations géométriques: {len(geo_transforms)}")
    assert len(geo_transforms) == 7
    
    # Spatiales
    spatial_transforms = ARCTransformations.get_all_spatial_transforms(grid)
    print(f"Transformations spatiales: {len(spatial_transforms)}")
    assert len(spatial_transforms) == 4
    
    # Couleur
    color_transforms = ARCTransformations.get_all_color_transforms(grid)
    print(f"Transformations couleur: {len(color_transforms)}")
    assert len(color_transforms) >= 1  # Au moins invert
    
    print("✅ get_all_transforms OK")


def test_real_arc_pattern():
    """Test pattern réel ARC."""
    print("\n=== TEST PATTERN RÉEL ARC ===")
    
    # Pattern: grille avec objet à détecter
    grid = np.array([
        [0, 0, 0, 0, 0],
        [0, 1, 1, 0, 0],
        [0, 1, 1, 0, 0],
        [0, 0, 0, 0, 0]
    ])
    
    print(f"Grille originale:\n{grid}\n")
    
    # Crop l'objet
    result = ARCTransformations.crop_nonzero(grid)
    print(f"Objet croppé:\n{result.grid}")
    
    # Rotate l'objet
    rotated = ARCTransformations.rotate_90(result.grid)
    print(f"Objet rotated:\n{rotated.grid}")
    
    # Scale up
    scaled = ARCTransformations.scale_up(rotated.grid, factor=2)
    print(f"Objet scaled:\n{scaled.grid}")
    
    print("✅ Pattern réel ARC OK")


if __name__ == '__main__':
    print("=" * 60)
    print("TESTS TRANSFORMATIONS ARC V38.1")
    print("=" * 60)
    
    try:
        test_geometric_transforms()
        test_spatial_transforms()
        test_color_transforms()
        test_apply_transformation()
        test_get_all_transforms()
        test_real_arc_pattern()
        
        print("\n" + "=" * 60)
        print("✅ TOUS LES TESTS RÉUSSIS")
        print("=" * 60)
        
    except AssertionError as e:
        print(f"\n❌ ÉCHEC TEST: {e}")
        import traceback
        traceback.print_exc()
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()

# Made with Bob
