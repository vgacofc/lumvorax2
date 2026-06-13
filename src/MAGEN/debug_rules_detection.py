#!/usr/bin/env python3
"""
Debug détaillé de la détection et application des règles
"""

import json
import numpy as np
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent))

from core.advanced_pipeline import AdvancedPipeline, ContextualRuleDetector
from objects.object_extractor import ObjectExtractor

def debug_puzzle(puzzle_id: str):
    """Debug complet d'un puzzle"""
    print(f"\n{'='*80}")
    print(f"DEBUG PUZZLE: {puzzle_id}")
    print('='*80)
    
    with open('arc-agi_training_challenges.json', 'r') as f:
        data = json.load(f)
    
    puzzle = data[puzzle_id]
    train = puzzle['train'][0]
    test = puzzle['test'][0]
    
    inp_train = np.array(train['input'])
    out_train = np.array(train['output'])
    inp_test = np.array(test['input'])
    
    # Extraire objets
    extractor = ObjectExtractor()
    detector = ContextualRuleDetector(verbose=True)
    
    print("\n--- TRAIN INPUT ---")
    objects_train_in = extractor.extract_objects(inp_train)
    print(f"Objets extraits: {len(objects_train_in)}")
    for i, obj in enumerate(objects_train_in):
        print(f"  Objet {i}: couleur={obj.color}, pixels={len(obj.pixels)}, bbox={obj.bounding_box}")
        print(f"    Pixels: {obj.pixels[:5]}{'...' if len(obj.pixels) > 5 else ''}")
    
    print("\n--- TRAIN OUTPUT ---")
    objects_train_out = extractor.extract_objects(out_train)
    print(f"Objets extraits: {len(objects_train_out)}")
    for i, obj in enumerate(objects_train_out):
        print(f"  Objet {i}: couleur={obj.color}, pixels={len(obj.pixels)}, bbox={obj.bounding_box}")
    
    print("\n--- TEST INPUT ---")
    objects_test_in = extractor.extract_objects(inp_test)
    print(f"Objets extraits: {len(objects_test_in)}")
    for i, obj in enumerate(objects_test_in):
        print(f"  Objet {i}: couleur={obj.color}, pixels={len(obj.pixels)}, bbox={obj.bounding_box}")
        print(f"    Pixels: {obj.pixels[:5]}{'...' if len(obj.pixels) > 5 else ''}")
    
    # Détecter règles
    print("\n--- RÈGLES DÉTECTÉES (depuis TRAIN) ---")
    pos_rules = detector.detect_positional_rules(
        inp_train, out_train, objects_train_in, objects_train_out
    )
    rel_rules = detector.detect_relational_rules(
        objects_train_in, objects_train_out
    )
    cond_rules = detector.detect_conditional_rules(
        inp_train, out_train, objects_train_in
    )
    
    all_rules = pos_rules + rel_rules + cond_rules
    
    print(f"\nTotal règles: {len(all_rules)}")
    for i, rule in enumerate(all_rules):
        print(f"\nRègle {i+1}: {rule['type']}")
        print(f"  Confidence: {rule['confidence']}")
        print(f"  Détails: {rule}")
    
    # Tester application sur TEST
    print("\n--- APPLICATION SUR TEST ---")
    pipeline = AdvancedPipeline(verbose=True)
    
    train_pairs = [(inp_train, out_train)]
    result = pipeline.solve_puzzle(train_pairs, inp_test)
    
    print("\nRésultat:")
    for row in result:
        print(' '.join(str(x) for x in row))
    
    print("\nAttendu (si disponible):")
    if 'output' in test:
        expected = np.array(test['output'])
        for row in expected:
            print(' '.join(str(x) for x in row))
        
        # Comparer
        if np.array_equal(result, expected):
            print("\n✓ SUCCÈS - Output correspond!")
        else:
            print("\n✗ ÉCHEC - Output différent")
            diff = (result != expected)
            print(f"Pixels différents: {np.sum(diff)}")

if __name__ == '__main__':
    debug_puzzle('00d62c1b')
    debug_puzzle('025d127b')

# Made with Bob
