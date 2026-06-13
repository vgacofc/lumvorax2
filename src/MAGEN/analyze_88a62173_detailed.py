#!/usr/bin/env python3
"""
Analyse détaillée 88a62173 - Tous les quadrants
"""

import json
import numpy as np
from pathlib import Path

# Charger puzzle
dataset_path = Path("arc-agi_training_challenges.json")
with open(dataset_path) as f:
    dataset = json.load(f)

puzzle = dataset["88a62173"]

print("="*80)
print("ANALYSE DÉTAILLÉE: 88a62173 - TOUS LES QUADRANTS")
print("="*80)

for idx, pair in enumerate(puzzle['train'], 1):
    input_grid = np.array(pair['input'])
    expected_output = np.array(pair['output'])
    
    print(f"\n{'='*80}")
    print(f"TRAIN PAIR {idx}/3")
    print(f"{'='*80}")
    
    # Trouver séparateur
    sep_row = None
    for i in range(input_grid.shape[0]):
        if np.all(input_grid[i] == 0):
            sep_row = i
            break
    
    print(f"\nInput 5x5 (séparateur ligne {sep_row}):")
    for i, row in enumerate(input_grid):
        marker = " ← SEP" if i == sep_row else ""
        print(f"  {i}: " + "".join(str(c) for c in row) + marker)
    
    # Extraire les 4 quadrants
    h, w = input_grid.shape
    mid_col = w // 2
    
    # Haut
    top_left = input_grid[0:2, 0:2]
    top_right = input_grid[0:2, mid_col+1:mid_col+3]
    
    # Bas (après séparateur)
    bottom_left = input_grid[sep_row+1:sep_row+3, 0:2]
    bottom_right = input_grid[sep_row+1:sep_row+3, mid_col+1:mid_col+3]
    
    print(f"\nQUADRANTS 2x2:")
    print(f"\n  Haut-Gauche (lignes 0-1, cols 0-1):")
    for row in top_left:
        print(f"    " + "".join(str(c) for c in row))
    
    print(f"\n  Haut-Droit (lignes 0-1, cols 3-4):")
    for row in top_right:
        print(f"    " + "".join(str(c) for c in row))
    
    print(f"\n  Bas-Gauche (lignes 3-4, cols 0-1):")
    for row in bottom_left:
        print(f"    " + "".join(str(c) for c in row))
    
    print(f"\n  Bas-Droit (lignes 3-4, cols 3-4):")
    for row in bottom_right:
        print(f"    " + "".join(str(c) for c in row))
    
    print(f"\nExpected Output:")
    for row in expected_output:
        print(f"    " + "".join(str(c) for c in row))
    
    # Comparer avec chaque quadrant
    print(f"\nCOMPARAISONS:")
    print(f"  Haut-Gauche  == Output: {np.array_equal(top_left, expected_output)}")
    print(f"  Haut-Droit   == Output: {np.array_equal(top_right, expected_output)}")
    print(f"  Bas-Gauche   == Output: {np.array_equal(bottom_left, expected_output)}")
    print(f"  Bas-Droit    == Output: {np.array_equal(bottom_right, expected_output)}")
    
    # Vote majoritaire position par position
    print(f"\nVOTE MAJORITAIRE (position par position):")
    vote_result = np.zeros((2, 2), dtype=input_grid.dtype)
    for i in range(2):
        for j in range(2):
            values = [
                top_left[i,j],
                top_right[i,j],
                bottom_left[i,j],
                bottom_right[i,j]
            ]
            # Compter occurrences
            from collections import Counter
            counts = Counter(values)
            most_common_value = counts.most_common(1)[0][0]
            vote_result[i,j] = most_common_value
            
            print(f"  Position ({i},{j}): {values} → {most_common_value}")
    
    print(f"\nRésultat vote majoritaire:")
    for row in vote_result:
        print(f"    " + "".join(str(c) for c in row))
    
    print(f"\nVote == Output: {np.array_equal(vote_result, expected_output)}")

# Made with Bob
