#!/usr/bin/env python3
"""
ANALYSE MANUELLE PUZZLE - Session 45
Charger et visualiser puzzles pour comprendre logique RÉELLE
"""

import json
import numpy as np
from pathlib import Path
import sys

def load_puzzle(puzzle_id: str):
    """Charger puzzle depuis dataset"""
    dataset_path = Path("arc-agi_training_challenges.json")
    
    if not dataset_path.exists():
        print(f"❌ Dataset introuvable: {dataset_path}")
        return None
    
    with open(dataset_path, 'r') as f:
        dataset = json.load(f)
    
    if puzzle_id not in dataset:
        print(f"❌ Puzzle {puzzle_id} introuvable dans dataset")
        return None
    
    return dataset[puzzle_id]

def visualize_grid(grid, title="Grid"):
    """Visualiser grille avec caractères"""
    print(f"\n{title}:")
    print(f"  Shape: {len(grid)}x{len(grid[0])}")
    
    # Mapping couleurs → caractères
    color_map = {
        0: '.',  # noir
        1: '1',  # bleu
        2: '2',  # rouge
        3: '3',  # vert
        4: '4',  # jaune
        5: '5',  # gris
        6: '6',  # magenta
        7: '7',  # orange
        8: '8',  # cyan
        9: '9',  # marron
    }
    
    for row in grid:
        print("  " + "".join(color_map.get(c, '?') for c in row))

def analyze_transformation(input_grid, output_grid):
    """Analyser transformation input → output"""
    input_arr = np.array(input_grid)
    output_arr = np.array(output_grid)
    
    print(f"\n🔍 ANALYSE TRANSFORMATION:")
    print(f"  Input shape:  {input_arr.shape}")
    print(f"  Output shape: {output_arr.shape}")
    print(f"  Shape preserved: {input_arr.shape == output_arr.shape}")
    
    # Couleurs
    input_colors = set(input_arr.flatten())
    output_colors = set(output_arr.flatten())
    new_colors = output_colors - input_colors
    removed_colors = input_colors - output_colors
    
    print(f"  Input colors:  {sorted(input_colors)}")
    print(f"  Output colors: {sorted(output_colors)}")
    if new_colors:
        print(f"  New colors:    {sorted(new_colors)}")
    if removed_colors:
        print(f"  Removed colors: {sorted(removed_colors)}")
    
    # Différences
    if input_arr.shape == output_arr.shape:
        diff = (input_arr != output_arr)
        num_changes = diff.sum()
        total_cells = input_arr.size
        print(f"  Cells changed: {num_changes}/{total_cells} ({100*num_changes/total_cells:.1f}%)")
        
        # Positions changées
        if num_changes > 0 and num_changes <= 20:
            changed_positions = np.argwhere(diff)
            print(f"  Changed positions:")
            for pos in changed_positions:
                i, j = pos
                print(f"    ({i},{j}): {input_arr[i,j]} → {output_arr[i,j]}")

def analyze_puzzle(puzzle_id: str):
    """Analyser puzzle complet"""
    print("="*80)
    print(f"ANALYSE MANUELLE PUZZLE: {puzzle_id}")
    print("="*80)
    
    puzzle = load_puzzle(puzzle_id)
    if not puzzle:
        return
    
    # Analyser train pairs
    print(f"\n📚 TRAIN PAIRS: {len(puzzle['train'])}")
    
    for idx, pair in enumerate(puzzle['train'], 1):
        print(f"\n{'='*80}")
        print(f"TRAIN PAIR {idx}/{len(puzzle['train'])}")
        print(f"{'='*80}")
        
        visualize_grid(pair['input'], f"Input {idx}")
        visualize_grid(pair['output'], f"Output {idx}")
        analyze_transformation(pair['input'], pair['output'])
    
    # Test pair
    print(f"\n{'='*80}")
    print(f"TEST PAIR")
    print(f"{'='*80}")
    
    for test in puzzle['test']:
        visualize_grid(test['input'], "Test Input")
        if 'output' in test:
            visualize_grid(test['output'], "Test Output (solution)")
            analyze_transformation(test['input'], test['output'])

def compare_train_pairs(puzzle_id: str):
    """Comparer logiques entre train pairs"""
    print("\n" + "="*80)
    print("COMPARAISON LOGIQUES TRAIN PAIRS")
    print("="*80)
    
    puzzle = load_puzzle(puzzle_id)
    if not puzzle or len(puzzle['train']) < 2:
        return
    
    # Extraire patterns de chaque train pair
    for idx, pair in enumerate(puzzle['train'], 1):
        input_arr = np.array(pair['input'])
        output_arr = np.array(pair['output'])
        
        print(f"\nTrain Pair {idx}:")
        
        # Pattern 1: Ratio dimensions
        if input_arr.shape != output_arr.shape:
            ratio_h = output_arr.shape[0] / input_arr.shape[0]
            ratio_w = output_arr.shape[1] / input_arr.shape[1]
            print(f"  Ratio dimensions: {ratio_h:.2f}x{ratio_w:.2f}")
        
        # Pattern 2: Couleurs ajoutées
        input_colors = set(input_arr.flatten())
        output_colors = set(output_arr.flatten())
        new_colors = output_colors - input_colors
        if new_colors:
            print(f"  Couleurs ajoutées: {sorted(new_colors)}")
        
        # Pattern 3: Positions changées
        if input_arr.shape == output_arr.shape:
            diff = (input_arr != output_arr)
            changed_positions = np.argwhere(diff)
            if len(changed_positions) > 0:
                print(f"  Positions changées: {len(changed_positions)}")
                
                # Analyser pattern spatial
                if len(changed_positions) <= 10:
                    for pos in changed_positions:
                        i, j = pos
                        print(f"    ({i},{j}): {input_arr[i,j]} → {output_arr[i,j]}")

if __name__ == "__main__":
    # Analyser 3aa6fb7a (object_relational)
    print("\n" + "🎯"*40)
    print("PUZZLE 1: 3aa6fb7a (object_relational)")
    print("🎯"*40)
    analyze_puzzle("3aa6fb7a")
    compare_train_pairs("3aa6fb7a")
    
    print("\n\n" + "🎯"*40)
    print("PUZZLE 2: 88a62173 (semantic_region)")
    print("🎯"*40)
    analyze_puzzle("88a62173")
    compare_train_pairs("88a62173")

# Made with Bob
