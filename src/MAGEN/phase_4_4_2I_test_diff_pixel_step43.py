#!/usr/bin/env python3
"""
PHASE 4.4.2I - Test B: Diff Pixel-par-Pixel Steps 42→43
========================================================

OBJECTIF:
Identifier les transformations exactes lors de l'événement step 43
en comparant pixel par pixel les états des grilles.

CONTEXTE (Phase 4.4.2H):
- Cycle découvert: 43 steps (pas 42)
- Step 43: Événement de téléportation (Door 3#2: Y=33.5→62.5, 84px→4px)
- Step 44: Reset (Door 3#2: Y=62.5→13.0, 4px→2px)

PROTOCOLE:
1. Exécuter ACTION2 × 42 (juste avant événement)
2. Capturer état complet step 42 (grille + entités)
3. Exécuter ACTION2 × 1 (déclenchement événement)
4. Capturer état complet step 43 (grille + entités)
5. Comparer pixel-par-pixel:
   - Grille RGB (210×160 pixels)
   - Positions entités (19 composantes)
   - Pixels modifiés (coordonnées + valeurs avant/après)

HYPOTHÈSES À TESTER:
H1: Seule Door 3#2 change (téléportation isolée)
H2: Autres entités affectées (cascade)
H3: Pixels timer modifiés (réapparition)
H4: Transformation couleur (pas seulement position)

Date: 2026-06-19
Auteur: Bob (MAGEN Project)
"""

import sys
import os
import json
import numpy as np
from datetime import datetime
from pathlib import Path

# Setup paths
MAGEN_ROOT = Path(__file__).parent
WORKSPACE_ROOT = MAGEN_ROOT.parent.parent.parent
sys.path.insert(0, str(WORKSPACE_ROOT))  # Pour accéder à environment_files

# Import du jeu RÉEL ls20
ls20_path = WORKSPACE_ROOT / "environment_files" / "ls20" / "9607627b"
sys.path.insert(0, str(ls20_path))

import ls20
from arcengine import GameAction
import scipy.ndimage as ndimage


def get_grid_from_game(game):
    """Obtient la grille RGB du jeu."""
    frame = game.camera.render(game.current_level._sprites)
    if isinstance(frame, np.ndarray):
        return frame
    return np.array(frame)


def extract_connected_components(grid):
    """Extrait les composantes connexes de la grille."""
    # La grille est déjà en valeurs (pas RGB)
    if len(grid.shape) == 3:
        # Si RGB, convertir en valeurs
        height, width, _ = grid.shape
        pixels = np.zeros((height, width), dtype=int)
        unique_colors = {}
        color_id = 0
        
        for y in range(height):
            for x in range(width):
                color = tuple(grid[y, x])
                if color not in unique_colors:
                    unique_colors[color] = color_id
                    color_id += 1
                pixels[y, x] = unique_colors[color]
    else:
        # Déjà en valeurs
        pixels = grid
        height, width = grid.shape
    
    # Extraire composantes connexes pour chaque valeur
    entities = []
    entity_id = 0
    
    for value in np.unique(pixels):
        mask = (pixels == value).astype(int)
        labeled, num_features = ndimage.label(mask)
        
        for comp_id in range(1, num_features + 1):
            comp_mask = (labeled == comp_id)
            coords = np.argwhere(comp_mask)
            
            if len(coords) > 0:
                centroid = coords.mean(axis=0)
                
                # Récupérer la valeur
                y, x = coords[0]
                pixel_value = int(pixels[y, x])
                
                entity = {
                    'id': f"{value}#{comp_id}",
                    'global_id': entity_id,
                    'value': pixel_value,
                    'pixels': len(coords),
                    'centroid': centroid.tolist()
                }
                entities.append(entity)
                entity_id += 1
    
    return entities


def capture_complete_state(game):
    """
    Capture l'état complet du jeu.
    
    Returns:
        dict: État complet avec grille, entités, métadonnées
    """
    # Grille de valeurs
    grid = get_grid_from_game(game)
    
    # Composantes connexes
    entities = extract_connected_components(grid)
    
    # Hash de la grille pour vérification
    grid_hash = hash(grid.tobytes())
    
    return {
        'grid': grid,
        'entities': entities,
        'grid_hash': grid_hash,
        'grid_shape': grid.shape
    }


def compare_grids_pixel_by_pixel(state_42, state_43):
    """
    Compare deux états pixel par pixel.
    
    Returns:
        dict: Analyse des différences
    """
    grid_42 = state_42['grid']
    grid_43 = state_43['grid']
    
    # Différence absolue
    diff = np.abs(grid_43.astype(int) - grid_42.astype(int))
    
    # Pixels modifiés
    modified_mask = (diff > 0)
    modified_coords = np.argwhere(modified_mask)
    
    # Détails des modifications
    modifications = []
    for y, x in modified_coords:
        value_before = int(grid_42[y, x])
        value_after = int(grid_43[y, x])
        value_diff = int(diff[y, x])
        
        modifications.append({
            'position': [int(x), int(y)],
            'value_before': value_before,
            'value_after': value_after,
            'value_diff': value_diff
        })
    
    # Statistiques
    total_pixels = grid_42.shape[0] * grid_42.shape[1]
    modified_pixels = len(modifications)
    percentage = (modified_pixels / total_pixels) * 100
    
    return {
        'total_pixels': total_pixels,
        'modified_pixels': modified_pixels,
        'percentage_modified': percentage,
        'modifications': modifications,
        'grid_shape': grid_42.shape
    }


def compare_entities(state_42, state_43):
    """
    Compare les entités entre deux états.
    
    Returns:
        dict: Analyse des changements d'entités
    """
    entities_42 = {e['id']: e for e in state_42['entities']}
    entities_43 = {e['id']: e for e in state_43['entities']}
    
    changes = []
    
    # Comparer chaque entité
    for entity_id in sorted(set(entities_42.keys()) | set(entities_43.keys())):
        if entity_id not in entities_42:
            changes.append({
                'entity_id': entity_id,
                'change_type': 'CREATED',
                'state_43': entities_43[entity_id]
            })
        elif entity_id not in entities_43:
            changes.append({
                'entity_id': entity_id,
                'change_type': 'DELETED',
                'state_42': entities_42[entity_id]
            })
        else:
            # Comparer positions et pixels
            e42 = entities_42[entity_id]
            e43 = entities_43[entity_id]
            
            centroid_42 = e42['centroid']
            centroid_43 = e43['centroid']
            delta_x = centroid_43[0] - centroid_42[0]
            delta_y = centroid_43[1] - centroid_42[1]
            
            pixels_42 = e42['pixels']
            pixels_43 = e43['pixels']
            
            if delta_x != 0 or delta_y != 0 or pixels_42 != pixels_43:
                changes.append({
                    'entity_id': entity_id,
                    'change_type': 'MODIFIED',
                    'centroid_42': centroid_42,
                    'centroid_43': centroid_43,
                    'delta': [delta_x, delta_y],
                    'pixels_42': pixels_42,
                    'pixels_43': pixels_43,
                    'value_42': e42['value'],
                    'value_43': e43['value']
                })
    
    return {
        'total_entities_42': len(entities_42),
        'total_entities_43': len(entities_43),
        'changes': changes,
        'num_changes': len(changes)
    }


def analyze_pixel_regions(modifications):
    """
    Analyse les régions de pixels modifiés.
    
    Returns:
        dict: Régions identifiées
    """
    if not modifications:
        return {'regions': [], 'num_regions': 0}
    
    # Grouper par proximité (région = pixels adjacents)
    positions = np.array([m['position'] for m in modifications])
    
    # Clustering simple par distance
    regions = []
    used = set()
    
    for i, pos in enumerate(positions):
        if i in used:
            continue
        
        # Nouvelle région
        region = [i]
        used.add(i)
        
        # Trouver pixels adjacents (distance <= 2)
        for j, other_pos in enumerate(positions):
            if j in used:
                continue
            dist = np.linalg.norm(pos - other_pos)
            if dist <= 2.0:
                region.append(j)
                used.add(j)
        
        # Calculer bounding box
        region_positions = positions[region]
        min_x, min_y = region_positions.min(axis=0)
        max_x, max_y = region_positions.max(axis=0)
        
        regions.append({
            'pixel_indices': region,
            'num_pixels': len(region),
            'bounding_box': {
                'min': [int(min_x), int(min_y)],
                'max': [int(max_x), int(max_y)],
                'width': int(max_x - min_x + 1),
                'height': int(max_y - min_y + 1)
            }
        })
    
    return {
        'regions': regions,
        'num_regions': len(regions)
    }


def execute_action(game, action_name):
    """
    Exécute une action dans le jeu.
    
    Args:
        game: Instance du jeu ls20
        action_name: Nom de l'action (ex: 'ACTION2')
    """
    # Créer l'objet action
    class ActionInput:
        def __init__(self, action_id):
            self.id = action_id
    
    # Obtenir l'enum GameAction correspondant
    game_action = getattr(GameAction, action_name)
    action_input = ActionInput(game_action)
    
    # Exécuter l'action
    game._set_action(action_input)
    
    # Attendre que l'action soit complète
    max_steps = 100
    steps = 0
    while not game.is_action_complete() and steps < max_steps:
        game.step()
        steps += 1


def test_diff_pixel_step43():
    """
    Test principal: Diff pixel-par-pixel steps 42→43.
    """
    print("=" * 80)
    print("PHASE 4.4.2I - Test B: Diff Pixel-par-Pixel Steps 42→43")
    print("=" * 80)
    print()
    
    # Initialiser le jeu
    print("Initialisation du jeu ls20-9607627b...")
    game = ls20.Ls20()
    game.set_level(0)  # Niveau 1
    grid = get_grid_from_game(game)
    print(f"✓ Jeu initialisé (grille {grid.shape})")
    print()
    
    # Exécuter ACTION2 × 42 (juste avant événement)
    print("Exécution ACTION2 × 42 (approche événement step 43)...")
    for step in range(1, 43):
        execute_action(game, 'ACTION2')
        if step % 10 == 0:
            print(f"  Step {step}/42...")
    print("✓ Step 42 atteint")
    print()
    
    # Capturer état step 42
    print("Capture état complet step 42...")
    state_42 = capture_complete_state(game)
    print(f"✓ État capturé:")
    print(f"  - Grille: {state_42['grid_shape']}")
    print(f"  - Entités: {len(state_42['entities'])}")
    print(f"  - Hash: {hex(state_42['grid_hash'])}")
    print()
    
    # Exécuter ACTION2 × 1 (déclenchement événement step 43)
    print("Exécution ACTION2 × 1 (déclenchement événement step 43)...")
    execute_action(game, 'ACTION2')
    print("✓ Step 43 atteint")
    print()
    
    # Capturer état step 43
    print("Capture état complet step 43...")
    state_43 = capture_complete_state(game)
    print(f"✓ État capturé:")
    print(f"  - Grille: {state_43['grid_shape']}")
    print(f"  - Entités: {len(state_43['entities'])}")
    print(f"  - Hash: {hex(state_43['grid_hash'])}")
    print()
    
    # Comparer grilles pixel par pixel
    print("Analyse diff pixel-par-pixel...")
    pixel_diff = compare_grids_pixel_by_pixel(state_42, state_43)
    print(f"✓ Pixels modifiés: {pixel_diff['modified_pixels']}/{pixel_diff['total_pixels']}")
    print(f"  Pourcentage: {pixel_diff['percentage_modified']:.2f}%")
    print()
    
    # Analyser régions modifiées
    print("Analyse régions modifiées...")
    regions = analyze_pixel_regions(pixel_diff['modifications'])
    print(f"✓ Régions identifiées: {regions['num_regions']}")
    for i, region in enumerate(regions['regions'], 1):
        bbox = region['bounding_box']
        print(f"  Région {i}: {region['num_pixels']} pixels")
        print(f"    BBox: ({bbox['min'][0]},{bbox['min'][1]}) → ({bbox['max'][0]},{bbox['max'][1]})")
        print(f"    Taille: {bbox['width']}×{bbox['height']}")
    print()
    
    # Comparer entités
    print("Analyse changements entités...")
    entity_diff = compare_entities(state_42, state_43)
    print(f"✓ Entités step 42: {entity_diff['total_entities_42']}")
    print(f"✓ Entités step 43: {entity_diff['total_entities_43']}")
    print(f"✓ Changements: {entity_diff['num_changes']}")
    print()
    
    if entity_diff['changes']:
        print("Détails changements entités:")
        for change in entity_diff['changes']:
            entity_id = change['entity_id']
            change_type = change['change_type']
            print(f"  • Entité {entity_id}: {change_type}")
            
            if change_type == 'MODIFIED':
                delta = change['delta']
                print(f"    Δposition: ({delta[0]:.1f}, {delta[1]:.1f})")
                print(f"    Pixels: {change['pixels_42']} → {change['pixels_43']}")
    print()
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"phase_4_4_2I_diff_pixel_step43_{timestamp}.json"
    
    results = {
        'metadata': {
            'phase': '4.4.2I',
            'test': 'Diff Pixel-par-Pixel Steps 42→43',
            'timestamp': timestamp,
            'game': 'ls20-9607627b'
        },
        'protocol': {
            'steps_before_event': 42,
            'event_step': 43,
            'action_used': 'ACTION2'
        },
        'state_42': {
            'grid_hash': hex(state_42['grid_hash']),
            'num_entities': len(state_42['entities']),
            'entities': state_42['entities']
        },
        'state_43': {
            'grid_hash': hex(state_43['grid_hash']),
            'num_entities': len(state_43['entities']),
            'entities': state_43['entities']
        },
        'pixel_diff': pixel_diff,
        'regions': regions,
        'entity_diff': entity_diff
    }
    
    print(f"Sauvegarde résultats dans {output_file}...")
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"✓ Résultats sauvegardés ({os.path.getsize(output_file)} bytes)")
    print()
    
    # Résumé final
    print("=" * 80)
    print("RÉSUMÉ PHASE 4.4.2I")
    print("=" * 80)
    print(f"Pixels modifiés: {pixel_diff['modified_pixels']} ({pixel_diff['percentage_modified']:.2f}%)")
    print(f"Régions identifiées: {regions['num_regions']}")
    print(f"Entités changées: {entity_diff['num_changes']}/{entity_diff['total_entities_42']}")
    print()
    
    # Validation hypothèses
    print("VALIDATION HYPOTHÈSES:")
    print(f"H1 (Seule Door 3#2 change): {'✓' if entity_diff['num_changes'] == 1 else '✗'}")
    print(f"H2 (Cascade entités): {'✓' if entity_diff['num_changes'] > 1 else '✗'}")
    print(f"H3 (Timer modifié): {'✓' if regions['num_regions'] > 1 else '?'}")
    print(f"H4 (Transformation couleur): {'?' if pixel_diff['modified_pixels'] > 0 else '✗'}")
    print()
    
    return results


if __name__ == "__main__":
    try:
        results = test_diff_pixel_step43()
        print("✓ Test PHASE 4.4.2I terminé avec succès")
        sys.exit(0)
    except Exception as e:
        print(f"✗ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
