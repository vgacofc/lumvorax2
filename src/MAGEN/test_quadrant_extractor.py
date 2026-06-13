#!/usr/bin/env python3
"""
Test manuel du générateur extract_bottom_right_quadrant sur 88a62173
"""

import json
import numpy as np
from pathlib import Path
from dsl.quadrant_extractor import extract_bottom_right_quadrant, find_separator_row

# Charger puzzle
dataset_path = Path("arc-agi_training_challenges.json")
with open(dataset_path) as f:
    dataset = json.load(f)

puzzle = dataset["88a62173"]

print("="*80)
print("TEST MANUEL: extract_bottom_right_quadrant sur 88a62173")
print("="*80)

for idx, pair in enumerate(puzzle['train'], 1):
    input_grid = np.array(pair['input'])
    expected_output = np.array(pair['output'])
    
    print(f"\n{'='*80}")
    print(f"TRAIN PAIR {idx}/3")
    print(f"{'='*80}")
    
    print(f"\nInput ({input_grid.shape}):")
    for row in input_grid:
        print("  " + "".join(str(c) for c in row))
    
    print(f"\nExpected Output ({expected_output.shape}):")
    for row in expected_output:
        print("  " + "".join(str(c) for c in row))
    
    # Trouver séparateur
    sep_row = find_separator_row(input_grid)
    print(f"\nSéparateur trouvé: ligne {sep_row}")
    
    # Extraire quadrant
    result = extract_bottom_right_quadrant(input_grid)
    
    print(f"\nRésultat extrait ({result.shape}):")
    for row in result:
        print("  " + "".join(str(c) for c in row))
    
    # Comparer
    match = np.array_equal(result, expected_output)
    print(f"\n{'✓' if match else '✗'} Match: {match}")
    
    if not match:
        print("\nDIFFÉRENCES:")
        print(f"  Result shape: {result.shape}")
        print(f"  Expected shape: {expected_output.shape}")
        if result.shape == expected_output.shape:
            diff = (result != expected_output)
            print(f"  Cellules différentes: {diff.sum()}/{result.size}")
            if diff.sum() <= 10:
                positions = np.argwhere(diff)
                for pos in positions:
                    i, j = pos
                    print(f"    ({i},{j}): {result[i,j]} vs {expected_output[i,j]}")

# Made with Bob
