#!/usr/bin/env python3
"""
Debug des 3 puzzles échoués: 3aa6fb7a, 5bd6f4ac, 88a62173
"""

import numpy as np
import json
import sys
sys.path.insert(0, '/home/lvx/LVX/lumvorax2/src/MAGEN')

from dsl.shape_detector import ShapeDetector
from dsl.rule_detector import RuleDetector

# Charger dataset
with open('/home/lvx/LVX/lumvorax2/src/MAGEN/arc-agi_training_challenges.json', 'r') as f:
    dataset = json.load(f)

failed_puzzles = ['3aa6fb7a', '5bd6f4ac', '88a62173']

for puzzle_id in failed_puzzles:
    print("=" * 80)
    print(f"DEBUG {puzzle_id}")
    print("=" * 80)
    print()
    
    puzzle = dataset[puzzle_id]
    train_pairs = [(np.array(ex['input']), np.array(ex['output'])) 
                   for ex in puzzle['train']]
    test_input = np.array(puzzle['test'][0]['input'])
    test_output = np.array(puzzle['test'][0]['output'])
    
    print(f"Train pairs: {len(train_pairs)}")
    print(f"Test input shape: {test_input.shape}")
    print(f"Test output shape: {test_output.shape}")
    print()
    
    # Analyser avec ShapeDetector
    shape_detector = ShapeDetector()
    shape_type = shape_detector.detect_transformation_type(train_pairs)
    is_periodic = shape_detector.is_periodic_pattern(train_pairs)
    
    print(f"ShapeDetector:")
    print(f"  Shape type: {shape_type}")
    print(f"  Is periodic: {is_periodic}")
    
    if shape_type == 'downscale':
        factor = shape_detector.calculate_downscale_factor(train_pairs)
        print(f"  Downscale factor: {factor}")
    
    print()
    
    # Analyser avec RuleDetector
    rule_detector = RuleDetector()
    
    print(f"RuleDetector:")
    print(f"  Checking L-shapes...")
    
    # Tester détection L-shapes sur train pairs
    for i, (inp, out) in enumerate(train_pairs[:2]):  # Premiers 2 pairs
        print(f"  Train pair {i+1}:")
        print(f"    Input shape: {inp.shape}")
        print(f"    Output shape: {out.shape}")
        
        # Afficher grilles
        print(f"    Input:")
        print(f"    {inp}")
        print(f"    Output:")
        print(f"    {out}")
        
        # Comparer couleurs
        unique_input = np.unique(inp)
        unique_output = np.unique(out)
        print(f"    Unique colors input: {unique_input.tolist()}")
        print(f"    Unique colors output: {unique_output.tolist()}")
        
        # Vérifier si output a nouvelles couleurs
        new_colors = set(unique_output) - set(unique_input)
        if new_colors:
            print(f"    New colors in output: {new_colors}")
    
    print()
    print()

# Made with Bob
