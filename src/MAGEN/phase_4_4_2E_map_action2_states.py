#!/usr/bin/env python3
"""
PHASE 4.4.2E - MAPPING 50 ÉTATS ACTION2
========================================

OBJECTIF CRITIQUE:
Résoudre l'anomalie ACTION2 (50 états uniques mais aucun mouvement 9#3)

HYPOTHÈSES À TESTER:
H1: ACTION2 = reconfiguration système (change mobilité composantes)
H2: ACTION2 = changement de rôle (9#3 devient fixe, autre devient mobile)
H3: ACTION2 = NOP avec états internes (compteur sans effet visible)

MÉTHODOLOGIE FORTE:
Pour chaque état ACTION2 (n=0 à 50):
1. Exécuter ACTION2 x n fois
2. Tester ACTION1 (LEFT)
3. Tester ACTION3 (UP)  
4. Tester ACTION4 (RIGHT/DOWN?)
5. Mesurer quelle composante bouge
6. Construire table: État → Ensemble mouvements possibles

CRITÈRE DE SUCCÈS:
- Si table constante → H3 validée (NOP)
- Si table change → H1/H2 validées (reconfiguration)

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
    """
    Extrait TOUTES les composantes connexes
    
    Returns:
        dict: {value: [(comp_id, coords, centroid, pixel_count), ...]}
    """
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
    """
    Exécute une séquence d'actions
    
    Args:
        game: Instance ls20
        actions: Liste de GameAction
        
    Returns:
        state final
    """
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


def detect_movement(comps_before, comps_after):
    """
    Détecte quelles composantes ont bougé
    
    Returns:
        list: [(value, comp_id, distance), ...]
    """
    movements = []
    
    for value in comps_before.keys():
        if value not in comps_after:
            continue
            
        comps_b = comps_before[value]
        comps_a = comps_after[value]
        
        for i, (comp_id_b, coords_b, centroid_b, pixels_b) in enumerate(comps_b):
            if i < len(comps_a):
                comp_id_a, coords_a, centroid_a, pixels_a = comps_a[i]
                
                dist = np.linalg.norm(centroid_b - centroid_a)
                if dist > 0.1:
                    movements.append({
                        "value": int(value),
                        "comp_id": int(comp_id_b),
                        "distance": float(dist),
                        "centroid_before": [float(centroid_b[0]), float(centroid_b[1])],
                        "centroid_after": [float(centroid_a[0]), float(centroid_a[1])]
                    })
    
    return movements


def test_mobility_after_action2(n_action2):
    """
    Teste la mobilité après n exécutions d'ACTION2
    
    Args:
        n_action2: Nombre d'ACTION2 à exécuter
        
    Returns:
        dict: Résultats mobilité pour ACTION1/3/4
    """
    results = {
        "n_action2": n_action2,
        "tests": {}
    }
    
    # Test ACTION1 (LEFT)
    game = ls20.Ls20()
    game.set_level(0)
    
    # Exécuter ACTION2 x n
    for _ in range(n_action2):
        execute_action_sequence(game, [GameAction.ACTION2])
    
    # État avant ACTION1
    state_before = game.camera.render(game.current_level._sprites).copy()
    comps_before = extract_all_components(state_before)
    
    # Exécuter ACTION1
    state_after = execute_action_sequence(game, [GameAction.ACTION1])
    comps_after = extract_all_components(state_after)
    
    # Détecter mouvements
    movements_action1 = detect_movement(comps_before, comps_after)
    results["tests"]["ACTION1"] = {
        "movements": movements_action1,
        "mobile_components": len(movements_action1)
    }
    
    # Test ACTION3 (UP)
    game = ls20.Ls20()
    game.set_level(0)
    
    for _ in range(n_action2):
        execute_action_sequence(game, [GameAction.ACTION2])
    
    state_before = game.camera.render(game.current_level._sprites).copy()
    comps_before = extract_all_components(state_before)
    
    state_after = execute_action_sequence(game, [GameAction.ACTION3])
    comps_after = extract_all_components(state_after)
    
    movements_action3 = detect_movement(comps_before, comps_after)
    results["tests"]["ACTION3"] = {
        "movements": movements_action3,
        "mobile_components": len(movements_action3)
    }
    
    # Test ACTION4 (RIGHT/DOWN?)
    game = ls20.Ls20()
    game.set_level(0)
    
    for _ in range(n_action2):
        execute_action_sequence(game, [GameAction.ACTION2])
    
    state_before = game.camera.render(game.current_level._sprites).copy()
    comps_before = extract_all_components(state_before)
    
    state_after = execute_action_sequence(game, [GameAction.ACTION4])
    comps_after = extract_all_components(state_after)
    
    movements_action4 = detect_movement(comps_before, comps_after)
    results["tests"]["ACTION4"] = {
        "movements": movements_action4,
        "mobile_components": len(movements_action4)
    }
    
    return results


def map_action2_states():
    """
    MAPPING COMPLET DES 50 ÉTATS ACTION2
    """
    print("=" * 80)
    print("PHASE 4.4.2E - MAPPING 50 ÉTATS ACTION2")
    print("=" * 80)
    print()
    print("OBJECTIF: Résoudre anomalie ACTION2 (50 états uniques)")
    print("MÉTHODE: Tester mobilité après chaque état ACTION2")
    print()
    
    # Résultats
    results = {
        "metadata": {
            "phase": "4.4.2E",
            "objective": "Mapping 50 états ACTION2 - Test reconfiguration système",
            "timestamp": datetime.now().isoformat(),
            "game": "ls20-9607627b",
            "level": 1
        },
        "states": []
    }
    
    # Tester états ACTION2 (échantillonnage intelligent)
    # États clés: 0, 1, 2, 5, 10, 20, 30, 40, 49, 50
    test_states = [0, 1, 2, 5, 10, 20, 30, 40, 49, 50]
    
    print(f"Test de {len(test_states)} états ACTION2 clés...")
    print()
    
    for n in test_states:
        print(f"État ACTION2 x {n}:")
        print("-" * 40)
        
        try:
            state_results = test_mobility_after_action2(n)
            results["states"].append(state_results)
            
            # Afficher résumé
            for action_name, action_data in state_results["tests"].items():
                mobile = action_data["mobile_components"]
                print(f"  {action_name}: {mobile} composantes mobiles")
                
                if action_data["movements"]:
                    for mov in action_data["movements"]:
                        print(f"    - Valeur {mov['value']} comp#{mov['comp_id']}: {mov['distance']:.2f} pixels")
            
            print()
            
        except Exception as e:
            print(f"  ❌ ERREUR: {e}")
            print()
    
    # Analyse comparative
    print("=" * 80)
    print("ANALYSE COMPARATIVE")
    print("=" * 80)
    print()
    
    # Construire table mobilité
    mobility_table = {}
    
    for state_data in results["states"]:
        n = state_data["n_action2"]
        
        # Signature mobilité: quelles composantes bougent
        signature = set()
        for action_name, action_data in state_data["tests"].items():
            for mov in action_data["movements"]:
                signature.add((mov["value"], mov["comp_id"]))
        
        mobility_table[n] = {
            "signature": list(signature),
            "count": len(signature)
        }
    
    print("Table de mobilité:")
    print()
    print("| État ACTION2 | Composantes mobiles | Signature |")
    print("|--------------|---------------------|-----------|")
    
    for n in sorted(mobility_table.keys()):
        data = mobility_table[n]
        sig_str = ", ".join([f"{v}#{c}" for v, c in data["signature"]])
        print(f"| {n:12} | {data['count']:19} | {sig_str:9} |")
    
    print()
    
    # Détection changement
    signatures = [tuple(sorted(mobility_table[n]["signature"])) for n in sorted(mobility_table.keys())]
    unique_signatures = set(signatures)
    
    print(f"Signatures uniques: {len(unique_signatures)}")
    print()
    
    if len(unique_signatures) == 1:
        verdict = "NOP_AVEC_ETATS_INTERNES"
        confidence = "HAUTE"
        hypothesis = "H3 validée - ACTION2 = NOP (compteur sans effet visible)"
    else:
        verdict = "RECONFIGURATION_SYSTEME"
        confidence = "HAUTE"
        hypothesis = "H1/H2 validées - ACTION2 modifie mobilité composantes"
    
    print("=" * 80)
    print("VERDICT")
    print("=" * 80)
    print(f"Type: {verdict}")
    print(f"Confiance: {confidence}")
    print(f"Hypothèse: {hypothesis}")
    print()
    
    results["analysis"] = {
        "mobility_table": mobility_table,
        "unique_signatures": len(unique_signatures),
        "verdict": verdict,
        "confidence": confidence,
        "hypothesis": hypothesis
    }
    
    # Sauvegarder
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"phase_4_4_2E_action2_states_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"✓ Résultats sauvegardés: {output_file}")
    print()
    
    return results


if __name__ == "__main__":
    results = map_action2_states()
    
    print("=" * 80)
    print("PHASE 4.4.2E COMPLÉTÉE")
    print("=" * 80)
    print()
    print("Prochaine étape: Rapport MD basé sur logs forensiques")

# Made with Bob
