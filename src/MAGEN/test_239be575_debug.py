#!/usr/bin/env python3
"""
Test isolé pour débugger 239be575
"""

import json
import numpy as np
from pathlib import Path

# Charger le puzzle
puzzle_path = Path("arc-agi_training_challenges.json")
with open(puzzle_path) as f:
    data = json.load(f)

puzzle = data["239be575"]
print("=" * 80)
print("PUZZLE 239be575 - DEBUG COMPLET")
print("=" * 80)

# Analyser train pairs
print("\n📊 ANALYSE TRAIN PAIRS:")
for i, pair in enumerate(puzzle["train"]):
    input_grid = np.array(pair["input"])
    output_grid = np.array(pair["output"])
    
    print(f"\nTrain {i+1}:")
    print(f"  Input shape: {input_grid.shape}")
    print(f"  Output shape: {output_grid.shape}")
    print(f"  Output value: {output_grid[0, 0]}")
    
    # Analyser couleurs input
    unique, counts = np.unique(input_grid, return_counts=True)
    print(f"  Input colors: {dict(zip(unique.tolist(), counts.tolist()))}")
    
    # Tester heuristiques
    print(f"\n  🔍 HEURISTIQUES:")
    
    # 1. Dominant global
    dominant_global = unique[np.argmax(counts)]
    print(f"    1. dominant_global = {dominant_global} (match: {dominant_global == output_grid[0, 0]})")
    
    # 2. Dominant non-zero
    non_zero_mask = input_grid != 0
    if np.any(non_zero_mask):
        non_zero_values = input_grid[non_zero_mask]
        unique_nz, counts_nz = np.unique(non_zero_values, return_counts=True)
        dominant_nonzero = unique_nz[np.argmax(counts_nz)]
        print(f"    2. dominant_nonzero = {dominant_nonzero} (match: {dominant_nonzero == output_grid[0, 0]})")
    
    # 3. Center pixel
    center_h, center_w = input_grid.shape[0] // 2, input_grid.shape[1] // 2
    center_pixel = input_grid[center_h, center_w]
    print(f"    3. center_pixel = {center_pixel} (match: {center_pixel == output_grid[0, 0]})")
    
    # 4. Most frequent non-zero (même que 2)
    print(f"    4. most_frequent_nonzero = {dominant_nonzero} (match: {dominant_nonzero == output_grid[0, 0]})")
    
    # 5. First non-zero
    non_zero_indices = np.argwhere(input_grid != 0)
    if len(non_zero_indices) > 0:
        first_nz_pos = non_zero_indices[0]
        first_nonzero = input_grid[first_nz_pos[0], first_nz_pos[1]]
        print(f"    5. first_nonzero = {first_nonzero} (match: {first_nonzero == output_grid[0, 0]})")

# Calculer scores
print("\n" + "=" * 80)
print("📈 SCORING HEURISTIQUES SUR TRAIN PAIRS:")
print("=" * 80)

scores = {
    'dominant_global': 0,
    'dominant_nonzero': 0,
    'center_pixel': 0,
    'most_frequent_nonzero': 0,
    'first_nonzero': 0
}

for pair in puzzle["train"]:
    input_grid = np.array(pair["input"])
    output_grid = np.array(pair["output"])
    expected = output_grid[0, 0]
    
    # Calculer chaque heuristique
    unique, counts = np.unique(input_grid, return_counts=True)
    dominant_global = unique[np.argmax(counts)]
    
    non_zero_mask = input_grid != 0
    if np.any(non_zero_mask):
        non_zero_values = input_grid[non_zero_mask]
        unique_nz, counts_nz = np.unique(non_zero_values, return_counts=True)
        dominant_nonzero = unique_nz[np.argmax(counts_nz)]
    else:
        dominant_nonzero = 0
    
    center_h, center_w = input_grid.shape[0] // 2, input_grid.shape[1] // 2
    center_pixel = input_grid[center_h, center_w]
    
    non_zero_indices = np.argwhere(input_grid != 0)
    if len(non_zero_indices) > 0:
        first_nz_pos = non_zero_indices[0]
        first_nonzero = input_grid[first_nz_pos[0], first_nz_pos[1]]
    else:
        first_nonzero = 0
    
    # Scorer
    if dominant_global == expected:
        scores['dominant_global'] += 1
    if dominant_nonzero == expected:
        scores['dominant_nonzero'] += 1
    if center_pixel == expected:
        scores['center_pixel'] += 1
    if dominant_nonzero == expected:  # same as dominant_nonzero
        scores['most_frequent_nonzero'] += 1
    if first_nonzero == expected:
        scores['first_nonzero'] += 1

print("\nScores:")
for name, score in scores.items():
    print(f"  {name}: {score}/{len(puzzle['train'])}")

# Meilleure heuristique
best_heuristic = max(scores.keys(), key=lambda k: scores[k])
print(f"\n🏆 MEILLEURE HEURISTIQUE: {best_heuristic} (score: {scores[best_heuristic]}/{len(puzzle['train'])})")

# Test sur test input
print("\n" + "=" * 80)
print("🧪 TEST SUR TEST INPUT:")
print("=" * 80)

test_input = np.array(puzzle["test"][0]["input"])
test_output = np.array(puzzle["test"][0]["output"])

print(f"\nTest input shape: {test_input.shape}")
print(f"Test output shape: {test_output.shape}")
print(f"Expected output: {test_output[0, 0]}")

# Appliquer meilleure heuristique
unique, counts = np.unique(test_input, return_counts=True)
dominant_global = unique[np.argmax(counts)]

non_zero_mask = test_input != 0
if np.any(non_zero_mask):
    non_zero_values = test_input[non_zero_mask]
    unique_nz, counts_nz = np.unique(non_zero_values, return_counts=True)
    dominant_nonzero = unique_nz[np.argmax(counts_nz)]
else:
    dominant_nonzero = 0

center_h, center_w = test_input.shape[0] // 2, test_input.shape[1] // 2
center_pixel = test_input[center_h, center_w]

non_zero_indices = np.argwhere(test_input != 0)
if len(non_zero_indices) > 0:
    first_nz_pos = non_zero_indices[0]
    first_nonzero = test_input[first_nz_pos[0], first_nz_pos[1]]
else:
    first_nonzero = 0

heuristics = {
    'dominant_global': dominant_global,
    'dominant_nonzero': dominant_nonzero,
    'center_pixel': center_pixel,
    'most_frequent_nonzero': dominant_nonzero,
    'first_nonzero': first_nonzero
}

print(f"\nHeuristiques sur test:")
for name, value in heuristics.items():
    match = "✓" if value == test_output[0, 0] else "✗"
    print(f"  {name}: {value} {match}")

print(f"\n🎯 PRÉDICTION AVEC {best_heuristic}: {heuristics[best_heuristic]}")
print(f"   Expected: {test_output[0, 0]}")
print(f"   Match: {heuristics[best_heuristic] == test_output[0, 0]}")

print("\n" + "=" * 80)

# Made with Bob
