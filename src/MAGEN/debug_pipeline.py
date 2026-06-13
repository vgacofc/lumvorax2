"""
Debug Pipeline - Analyse Détaillée Exécution
=============================================

Script debug pour comprendre pourquoi les règles ne sont pas appliquées.

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot
"""

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))

import json
import numpy as np

from core.advanced_pipeline import AdvancedPipeline, ContextualRuleDetector
from objects.object_extractor import ObjectExtractor


def debug_puzzle_007bbfb7():
    """Debug puzzle 007bbfb7 en détail"""
    print("\n" + "="*80)
    print("DEBUG PUZZLE 007bbfb7")
    print("="*80)
    
    # Charger puzzle
    with open("arc-agi_training_challenges.json", 'r') as f:
        data = json.load(f)
    
    puzzle = data['007bbfb7']
    
    # Premier exemple training
    train_input = np.array(puzzle['train'][0]['input'], dtype=int)
    train_output = np.array(puzzle['train'][0]['output'], dtype=int)
    
    print("\nTrain Input:")
    print(train_input)
    print("\nTrain Output:")
    print(train_output)
    
    # Extraire objets
    extractor = ObjectExtractor(verbose=True)
    objects_in = extractor.extract_objects(train_input)
    objects_out = extractor.extract_objects(train_output)
    
    print(f"\nObjets input: {len(objects_in)}")
    for obj in objects_in:
        print(f"  - Objet {obj.id}: {obj.shape_type.value}, couleur={obj.color}, bbox={obj.bounding_box}")
        print(f"    Pixels: {obj.pixels[:5]}...")  # Premiers 5 pixels
    
    print(f"\nObjets output: {len(objects_out)}")
    for obj in objects_out:
        print(f"  - Objet {obj.id}: {obj.shape_type.value}, couleur={obj.color}, bbox={obj.bounding_box}")
    
    # Détecter règles
    rule_detector = ContextualRuleDetector(verbose=True)
    
    pos_rules = rule_detector.detect_positional_rules(
        train_input, train_output, objects_in, objects_out
    )
    print(f"\nRègles positionnelles: {len(pos_rules)}")
    for rule in pos_rules:
        print(f"  - {rule}")
    
    rel_rules = rule_detector.detect_relational_rules(objects_in, objects_out)
    print(f"\nRègles relationnelles: {len(rel_rules)}")
    for rule in rel_rules:
        print(f"  - {rule}")
    
    cond_rules = rule_detector.detect_conditional_rules(
        train_input, train_output, objects_in
    )
    print(f"\nRègles conditionnelles: {len(cond_rules)}")
    for rule in cond_rules:
        print(f"  - {rule}")
    
    # Test input
    test_input = np.array(puzzle['test'][0]['input'], dtype=int)
    test_output = np.array(puzzle['test'][0]['output'], dtype=int)
    
    print(f"\n{'='*80}")
    print("TEST APPLICATION RÈGLES")
    print(f"{'='*80}")
    
    print("\nTest Input:")
    print(test_input)
    
    print("\nTest Output (attendu):")
    print(test_output)
    
    # Extraire objets test
    test_objects = extractor.extract_objects(test_input)
    print(f"\nObjets test input: {len(test_objects)}")
    for obj in test_objects:
        print(f"  - Objet {obj.id}: {obj.shape_type.value}, couleur={obj.color}, bbox={obj.bounding_box}")
        print(f"    Pixels: {obj.pixels}")
    
    # Appliquer première règle positionnelle
    if pos_rules:
        rule = pos_rules[0]
        print(f"\nApplication règle: {rule}")
        
        # Simuler application manuelle
        result = test_input.copy()
        
        for obj in test_objects:
            print(f"\n  Traitement objet {obj.id} ({obj.shape_type.value}):")
            
            # CORRECTION: Filtrer par nombre de pixels au lieu de shape_type
            if len(obj.pixels) != 3:
                print(f"    ✗ Pas 3 pixels ({len(obj.pixels)} pixels), skip")
                continue
            
            print(f"    ✓ 3 pixels détectés, traitement...")
            
            y_min, x_min, y_max, x_max = obj.bounding_box
            print(f"    Bounding box: ({y_min}, {x_min}) → ({y_max}, {x_max})")
            
            # Tester chaque coin
            corners = [
                (y_min, x_min, "top_left"),
                (y_min, x_max, "top_right"),
                (y_max, x_min, "bottom_left"),
                (y_max, x_max, "bottom_right")
            ]
            
            for y, x, corner_name in corners:
                if not (0 <= y < result.shape[0] and 0 <= x < result.shape[1]):
                    continue
                
                if result[y, x] != 0:
                    continue
                
                # Compter adjacents
                adjacent_count = 0
                for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    ny, nx = y + dy, x + dx
                    if (0 <= ny < result.shape[0] and 
                        0 <= nx < result.shape[1] and
                        (ny, nx) in obj.pixels):
                        adjacent_count += 1
                
                print(f"    Coin {corner_name} ({y}, {x}): {adjacent_count} adjacents")
                
                if adjacent_count >= 2:
                    print(f"      ✓ COIN INTÉRIEUR TROUVÉ!")
                    result[y, x] = rule['color']
                    break
        
        print("\nRésultat après application:")
        print(result)
        
        print("\nComparaison:")
        match = np.array_equal(result, test_output)
        print(f"Match exact: {match}")
        
        if not match:
            print("\nDifférences:")
            diff = result != test_output
            diff_positions = np.argwhere(diff)
            for pos in diff_positions:
                y, x = pos
                print(f"  Position ({y}, {x}): obtenu={result[y,x]}, attendu={test_output[y,x]}")
    
    print(f"\n{'='*80}")
    print("✓ DEBUG COMPLÉTÉ")
    print(f"{'='*80}\n")


if __name__ == "__main__":
    debug_puzzle_007bbfb7()

# Made with Bob
