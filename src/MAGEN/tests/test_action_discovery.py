"""
Tests pour ActionDiscoveryEngine
=================================

Teste la découverte automatique d'actions plausibles à partir des train examples.

Date: 2026-06-15
Version: V38.2
"""

import numpy as np
import sys
from pathlib import Path

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.action_discovery_engine import ActionDiscoveryEngine, DiscoveredAction


def test_geometric_discovery():
    """Test découverte transformations géométriques."""
    print("\n=== TEST DÉCOUVERTE GÉOMÉTRIQUE ===")
    
    # Train example: rotation 90°
    input_grid = np.array([
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
    ])
    
    output_grid = np.array([
        [7, 4, 1],
        [8, 5, 2],
        [9, 6, 3]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
    
    # Vérifier qu'on a trouvé rotate_90
    assert any(a.transform_name == 'rotate_90' for a in actions)
    assert actions[0].plausibility == 1.0  # Match parfait
    
    print("✅ Découverte géométrique OK")


def test_spatial_discovery_crop():
    """Test découverte crop."""
    print("\n=== TEST DÉCOUVERTE CROP ===")
    
    # Train example: crop
    input_grid = np.array([
        [0, 0, 0, 0, 0],
        [0, 1, 2, 3, 0],
        [0, 4, 5, 6, 0],
        [0, 0, 0, 0, 0]
    ])
    
    output_grid = np.array([
        [1, 2, 3],
        [4, 5, 6]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
    
    # Vérifier qu'on a trouvé crop_nonzero
    assert any(a.transform_name == 'crop_nonzero' for a in actions)
    
    print("✅ Découverte crop OK")


def test_spatial_discovery_scale():
    """Test découverte scale up."""
    print("\n=== TEST DÉCOUVERTE SCALE UP ===")
    
    # Train example: scale up 2x
    input_grid = np.array([
        [1, 2],
        [3, 4]
    ])
    
    output_grid = np.array([
        [1, 1, 2, 2],
        [1, 1, 2, 2],
        [3, 3, 4, 4],
        [3, 3, 4, 4]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
        if action.parameters:
            print(f"    Paramètres: {action.parameters}")
    
    # Vérifier qu'on a trouvé scale_up
    scale_actions = [a for a in actions if a.transform_name == 'scale_up']
    assert len(scale_actions) > 0
    assert scale_actions[0].parameters.get('factor') == 2
    
    print("✅ Découverte scale up OK")


def test_spatial_discovery_tile():
    """Test découverte tile."""
    print("\n=== TEST DÉCOUVERTE TILE ===")
    
    # Train example: tile 2x2
    input_grid = np.array([
        [1, 2],
        [3, 4]
    ])
    
    output_grid = np.array([
        [1, 2, 1, 2],
        [3, 4, 3, 4],
        [1, 2, 1, 2],
        [3, 4, 3, 4]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
        if action.parameters:
            print(f"    Paramètres: {action.parameters}")
    
    # Vérifier qu'on a trouvé tile
    tile_actions = [a for a in actions if a.transform_name == 'tile']
    assert len(tile_actions) > 0
    assert tile_actions[0].parameters.get('rows') == 2
    assert tile_actions[0].parameters.get('cols') == 2
    
    print("✅ Découverte tile OK")


def test_color_discovery_invert():
    """Test découverte inversion couleur."""
    print("\n=== TEST DÉCOUVERTE INVERSION COULEUR ===")
    
    # Train example: invert colors
    input_grid = np.array([
        [0, 1, 2],
        [3, 4, 5],
        [6, 7, 8]
    ])
    
    output_grid = np.array([
        [9, 8, 7],
        [6, 5, 4],
        [3, 2, 1]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
    
    # Vérifier qu'on a trouvé invert_colors
    assert any(a.transform_name == 'invert_colors' for a in actions)
    
    print("✅ Découverte inversion couleur OK")


def test_color_discovery_mapping():
    """Test découverte mapping couleur."""
    print("\n=== TEST DÉCOUVERTE MAPPING COULEUR ===")
    
    # Train example: color mapping
    input_grid = np.array([
        [1, 2, 3],
        [1, 2, 3],
        [1, 2, 3]
    ])
    
    output_grid = np.array([
        [9, 8, 7],
        [9, 8, 7],
        [9, 8, 7]
    ])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
        if action.parameters:
            print(f"    Paramètres: {action.parameters}")
    
    # Vérifier qu'on a trouvé map_colors
    map_actions = [a for a in actions if a.transform_name == 'map_colors']
    assert len(map_actions) > 0
    assert 'color_map' in map_actions[0].parameters
    
    print("✅ Découverte mapping couleur OK")


def test_multiple_examples():
    """Test avec plusieurs train examples."""
    print("\n=== TEST PLUSIEURS EXAMPLES ===")
    
    # 3 examples avec rotation 90°
    train_examples = []
    
    for i in range(3):
        input_grid = np.random.randint(0, 10, (3, 3))
        output_grid = np.rot90(input_grid, k=-1)
        train_examples.append({'input': input_grid, 'output': output_grid})
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes: {len(actions)}")
    for action in actions:
        print(f"  - {action.transform_name}: {action.plausibility:.2f}")
        print(f"    Evidence: {action.evidence}")
    
    # Vérifier qu'on a trouvé rotate_90 avec plausibilité 1.0
    rotate_actions = [a for a in actions if a.transform_name == 'rotate_90']
    assert len(rotate_actions) > 0
    assert rotate_actions[0].plausibility == 1.0
    assert rotate_actions[0].evidence['matches'] == 3
    
    print("✅ Plusieurs examples OK")


def test_filtering():
    """Test filtrage par plausibilité."""
    print("\n=== TEST FILTRAGE ===")
    
    # Example qui ne matche aucune transformation simple
    input_grid = np.array([[1, 2], [3, 4]])
    output_grid = np.array([[5, 6], [7, 8]])
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    # Avec seuil élevé
    engine = ActionDiscoveryEngine(min_plausibility=0.9, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes (seuil 0.9): {len(actions)}")
    
    # Avec seuil bas
    engine = ActionDiscoveryEngine(min_plausibility=0.1, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    print(f"Actions découvertes (seuil 0.1): {len(actions)}")
    
    print("✅ Filtrage OK")


def test_stats():
    """Test statistiques."""
    print("\n=== TEST STATISTIQUES ===")
    
    # Train example avec rotation
    input_grid = np.array([[1, 2], [3, 4]])
    output_grid = np.rot90(input_grid, k=-1)
    
    train_examples = [
        {'input': input_grid, 'output': output_grid}
    ]
    
    engine = ActionDiscoveryEngine(min_plausibility=0.5, max_actions=10)
    actions = engine.discover_actions(train_examples)
    
    stats = engine.get_stats()
    print(f"Statistiques:")
    print(f"  Total découvertes: {stats['total_discoveries']}")
    print(f"  Géométriques: {stats['geometric_found']}")
    print(f"  Spatiales: {stats['spatial_found']}")
    print(f"  Couleur: {stats['color_found']}")
    
    assert stats['total_discoveries'] > 0
    
    print("✅ Statistiques OK")


if __name__ == '__main__':
    print("=" * 60)
    print("TESTS ACTION DISCOVERY ENGINE V38.2")
    print("=" * 60)
    
    try:
        test_geometric_discovery()
        test_spatial_discovery_crop()
        test_spatial_discovery_scale()
        test_spatial_discovery_tile()
        test_color_discovery_invert()
        test_color_discovery_mapping()
        test_multiple_examples()
        test_filtering()
        test_stats()
        
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
