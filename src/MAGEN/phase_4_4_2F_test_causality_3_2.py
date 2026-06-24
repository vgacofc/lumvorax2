#!/usr/bin/env python3
"""
PHASE 4.4.2F - TESTS CAUSALITÉ COMPOSANTE 3#2
==============================================

OBJECTIF CRITIQUE:
Discriminer entre 4 hypothèses sur nature de 3#2

HYPOTHÈSES À TESTER:
Cas A: 3#2 = entité mobile indépendante
Cas B: 3#2 = porte (états ouverture/fermeture)
Cas C: 3#2 = convoyeur (mécanisme périodique)
Cas D: 3#2 = curseur temporel (horloge visualisée)

TESTS CAUSAUX:
Test 1: Réversibilité (état50 = état0 ?)
Test 2: Collision Avatar ↔ 3#2
Test 3: Goals ↔ 3#2

CRITÈRE DE SUCCÈS:
- Si état50 = état0 → Cycle fermé (Cas C/D)
- Si collision change score → Interaction physique (Cas A/B)
- Si 3#2 atteint goals → 3#2 = objet puzzle (Cas A)

Date: 2026-06-19
Auteur: Bob (MAGEN Project)
"""

import sys
import os
import json
import numpy as np
from datetime import datetime
from scipy import ndimage
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

# Import ls20 API
ls20_path = Path(__file__).parent.parent.parent.parent / "environment_files" / "ls20" / "9607627b"
sys.path.insert(0, str(ls20_path))

import ls20
from ls20 import GameAction


def extract_all_components(pixels):
    """Extrait TOUTES les composantes connexes"""
    unique_values = np.unique(pixels)
    unique_values = unique_values[unique_values != 0]
    
    components = {}
    
    for value in unique_values:
        mask = (pixels == value).astype(int)
        labeled, num_features = ndimage.label(mask)
        
        value_components = []
        for comp_id in range(1, num_features + 1):
            comp_mask = (labeled == comp_id)
            coords = np.argwhere(comp_mask)
            
            if len(coords) > 0:
                centroid = coords.mean(axis=0)
                pixel_count = len(coords)
                value_components.append((comp_id, coords, centroid, pixel_count))
        
        if value_components:
            components[value] = value_components
    
    return components


def execute_action_sequence(game, actions):
    """Exécute une séquence d'actions"""
    class ActionInput:
        def __init__(self, action_id):
            self.id = action_id
    
    for action in actions:
        action_input = ActionInput(action)
        game._set_action(action_input)
        
        steps = 0
        max_steps = 100
        while not game.is_action_complete() and steps < max_steps:
            game.step()
            steps += 1
    
    return game.camera.render(game.current_level._sprites).copy()


def get_component_3_2_position(components):
    """Extrait position composante 3#2"""
    if 3 not in components:
        return None
    
    if len(components[3]) < 2:
        return None
    
    # Composante #2 = index 1 (0-based)
    _, coords, centroid, pixels = components[3][1]
    return {
        "centroid": [float(centroid[0]), float(centroid[1])],
        "pixels": int(pixels),
        "coords": coords.tolist()
    }


def get_component_9_3_position(components):
    """Extrait position avatar (9#3)"""
    if 9 not in components:
        return None
    
    if len(components[9]) < 3:
        return None
    
    # Composante #3 = index 2 (0-based)
    _, coords, centroid, pixels = components[9][2]
    return {
        "centroid": [float(centroid[0]), float(centroid[1])],
        "pixels": int(pixels),
        "coords": coords.tolist()
    }


def get_goals_positions(components):
    """Extrait positions goals (valeur 8)"""
    if 8 not in components:
        return []
    
    goals = []
    for comp_id, coords, centroid, pixels in components[8]:
        goals.append({
            "comp_id": int(comp_id),
            "centroid": [float(centroid[0]), float(centroid[1])],
            "pixels": int(pixels)
        })
    
    return goals


def test_reversibility():
    """
    TEST 1: RÉVERSIBILITÉ
    État50 = État0 ?
    """
    print("=" * 80)
    print("TEST 1: RÉVERSIBILITÉ (état50 = état0 ?)")
    print("=" * 80)
    print()
    
    # État initial
    game = ls20.Ls20()
    game.set_level(0)
    
    state_0 = game.camera.render(game.current_level._sprites).copy()
    comps_0 = extract_all_components(state_0)
    pos_3_2_state_0 = get_component_3_2_position(comps_0)
    
    print(f"État 0:")
    print(f"  Composante 3#2: {pos_3_2_state_0}")
    print()
    
    # Exécuter ACTION2 x 50
    print("Exécution ACTION2 x 50...")
    for i in range(50):
        execute_action_sequence(game, [GameAction.ACTION2])
    
    state_50 = game.camera.render(game.current_level._sprites).copy()
    comps_50 = extract_all_components(state_50)
    pos_3_2_state_50 = get_component_3_2_position(comps_50)
    
    print(f"État 50:")
    print(f"  Composante 3#2: {pos_3_2_state_50}")
    print()
    
    # Comparaison
    if pos_3_2_state_0 is None and pos_3_2_state_50 is None:
        verdict = "IDENTIQUE (3#2 absente dans les deux)"
        reversible = True
    elif pos_3_2_state_0 is None or pos_3_2_state_50 is None:
        verdict = "DIFFÉRENT (3#2 présente dans un seul état)"
        reversible = False
    else:
        dist = np.linalg.norm(
            np.array(pos_3_2_state_0["centroid"]) - 
            np.array(pos_3_2_state_50["centroid"])
        )
        if dist < 0.1:
            verdict = f"IDENTIQUE (distance={dist:.4f})"
            reversible = True
        else:
            verdict = f"DIFFÉRENT (distance={dist:.4f})"
            reversible = False
    
    print(f"Verdict: {verdict}")
    print(f"Réversible: {reversible}")
    print()
    
    return {
        "test": "reversibility",
        "state_0": pos_3_2_state_0,
        "state_50": pos_3_2_state_50,
        "verdict": verdict,
        "reversible": reversible
    }


def test_collision_avatar_3_2():
    """
    TEST 2: COLLISION AVATAR ↔ 3#2
    Forcer contact et observer changements
    """
    print("=" * 80)
    print("TEST 2: COLLISION AVATAR ↔ 3#2")
    print("=" * 80)
    print()
    
    # Activer 3#2
    game = ls20.Ls20()
    game.set_level(0)
    
    execute_action_sequence(game, [GameAction.ACTION2])
    
    # Positions initiales
    state_before = game.camera.render(game.current_level._sprites).copy()
    comps_before = extract_all_components(state_before)
    
    pos_avatar_before = get_component_9_3_position(comps_before)
    pos_3_2_before = get_component_3_2_position(comps_before)
    
    print("Avant collision:")
    print(f"  Avatar (9#3): {pos_avatar_before}")
    print(f"  Composante 3#2: {pos_3_2_before}")
    
    if pos_avatar_before and pos_3_2_before:
        dist_before = np.linalg.norm(
            np.array(pos_avatar_before["centroid"]) - 
            np.array(pos_3_2_before["centroid"])
        )
        print(f"  Distance: {dist_before:.2f} pixels")
    print()
    
    # Déplacer avatar vers 3#2
    # Stratégie: UP puis RIGHT pour approcher
    print("Déplacement avatar vers 3#2...")
    
    for _ in range(10):
        execute_action_sequence(game, [GameAction.ACTION3])  # UP
    
    for _ in range(5):
        execute_action_sequence(game, [GameAction.ACTION1])  # LEFT (vers droite écran)
    
    # Positions après
    state_after = game.camera.render(game.current_level._sprites).copy()
    comps_after = extract_all_components(state_after)
    
    pos_avatar_after = get_component_9_3_position(comps_after)
    pos_3_2_after = get_component_3_2_position(comps_after)
    
    print("Après déplacement:")
    print(f"  Avatar (9#3): {pos_avatar_after}")
    print(f"  Composante 3#2: {pos_3_2_after}")
    
    if pos_avatar_after and pos_3_2_after:
        dist_after = np.linalg.norm(
            np.array(pos_avatar_after["centroid"]) - 
            np.array(pos_3_2_after["centroid"])
        )
        print(f"  Distance: {dist_after:.2f} pixels")
        
        if dist_after < 5.0:
            collision = True
            print("  ✓ COLLISION DÉTECTÉE")
        else:
            collision = False
            print("  ✗ Pas de collision")
    else:
        collision = False
        print("  ✗ Composante manquante")
    
    print()
    
    # Observer changements
    pixel_diff = np.sum(state_before != state_after)
    print(f"Différence pixels: {pixel_diff}")
    print()
    
    return {
        "test": "collision_avatar_3_2",
        "before": {
            "avatar": pos_avatar_before,
            "3_2": pos_3_2_before
        },
        "after": {
            "avatar": pos_avatar_after,
            "3_2": pos_3_2_after
        },
        "collision": collision,
        "pixel_diff": int(pixel_diff)
    }


def test_goals_3_2():
    """
    TEST 3: GOALS ↔ 3#2
    Tester si 3#2 peut atteindre goals
    """
    print("=" * 80)
    print("TEST 3: GOALS ↔ 3#2")
    print("=" * 80)
    print()
    
    # Activer 3#2
    game = ls20.Ls20()
    game.set_level(0)
    
    execute_action_sequence(game, [GameAction.ACTION2])
    
    # Positions initiales
    state_initial = game.camera.render(game.current_level._sprites).copy()
    comps_initial = extract_all_components(state_initial)
    
    pos_3_2 = get_component_3_2_position(comps_initial)
    goals = get_goals_positions(comps_initial)
    
    print("Positions initiales:")
    print(f"  Composante 3#2: {pos_3_2}")
    print(f"  Goals (valeur 8): {len(goals)} détectés")
    for i, goal in enumerate(goals):
        print(f"    Goal #{i+1}: {goal}")
    print()
    
    # Calculer distances
    if pos_3_2 and goals:
        print("Distances 3#2 → Goals:")
        for i, goal in enumerate(goals):
            dist = np.linalg.norm(
                np.array(pos_3_2["centroid"]) - 
                np.array(goal["centroid"])
            )
            print(f"  Goal #{i+1}: {dist:.2f} pixels")
        print()
    
    # Faire évoluer 3#2 (ACTION2 x 25 pour cycle complet)
    print("Évolution 3#2 (ACTION2 x 25)...")
    for _ in range(25):
        execute_action_sequence(game, [GameAction.ACTION2])
    
    state_evolved = game.camera.render(game.current_level._sprites).copy()
    comps_evolved = extract_all_components(state_evolved)
    
    pos_3_2_evolved = get_component_3_2_position(comps_evolved)
    goals_evolved = get_goals_positions(comps_evolved)
    
    print("Après évolution:")
    print(f"  Composante 3#2: {pos_3_2_evolved}")
    print(f"  Goals: {len(goals_evolved)} détectés")
    print()
    
    # Vérifier overlap
    overlap_detected = False
    if pos_3_2_evolved and goals_evolved:
        print("Distances 3#2 → Goals (après évolution):")
        for i, goal in enumerate(goals_evolved):
            dist = np.linalg.norm(
                np.array(pos_3_2_evolved["centroid"]) - 
                np.array(goal["centroid"])
            )
            print(f"  Goal #{i+1}: {dist:.2f} pixels")
            
            if dist < 2.0:
                overlap_detected = True
                print(f"    ✓ OVERLAP DÉTECTÉ avec Goal #{i+1}")
        print()
    
    return {
        "test": "goals_3_2",
        "initial": {
            "3_2": pos_3_2,
            "goals": goals
        },
        "evolved": {
            "3_2": pos_3_2_evolved,
            "goals": goals_evolved
        },
        "overlap_detected": overlap_detected
    }


def run_causality_tests():
    """
    TESTS CAUSALITÉ COMPLETS
    """
    print("=" * 80)
    print("PHASE 4.4.2F - TESTS CAUSALITÉ COMPOSANTE 3#2")
    print("=" * 80)
    print()
    
    results = {
        "metadata": {
            "phase": "4.4.2F",
            "objective": "Tests causalité 3#2 - Discriminer 4 hypothèses",
            "timestamp": datetime.now().isoformat(),
            "game": "ls20-9607627b",
            "level": 1
        },
        "tests": []
    }
    
    # Test 1: Réversibilité
    result_1 = test_reversibility()
    results["tests"].append(result_1)
    
    # Test 2: Collision
    result_2 = test_collision_avatar_3_2()
    results["tests"].append(result_2)
    
    # Test 3: Goals
    result_3 = test_goals_3_2()
    results["tests"].append(result_3)
    
    # Analyse globale
    print("=" * 80)
    print("ANALYSE GLOBALE")
    print("=" * 80)
    print()
    
    # Discrimination hypothèses
    print("Discrimination hypothèses:")
    print()
    
    if result_1["reversible"]:
        print("✓ Réversible → Cycle fermé (Cas C/D plausibles)")
    else:
        print("✗ Non réversible → Mémoire cachée (Cas A/B plausibles)")
    
    if result_2["collision"]:
        print("✓ Collision détectée → Interaction physique (Cas A/B plausibles)")
    else:
        print("✗ Pas de collision → Pas d'interaction directe (Cas C/D plausibles)")
    
    if result_3["overlap_detected"]:
        print("✓ Overlap avec goals → 3#2 = objet puzzle (Cas A plausible)")
    else:
        print("✗ Pas d'overlap → 3#2 ≠ objet puzzle (Cas B/C/D plausibles)")
    
    print()
    
    # Sauvegarder
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"phase_4_4_2F_causality_3_2_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"✓ Résultats sauvegardés: {output_file}")
    print()
    
    return results


if __name__ == "__main__":
    results = run_causality_tests()
    
    print("=" * 80)
    print("PHASE 4.4.2F COMPLÉTÉE")
    print("=" * 80)
    print()
    print("Prochaine étape: Rapport MD basé sur logs forensiques")

# Made with Bob
