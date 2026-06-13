#!/usr/bin/env python3
"""
Analyse détaillée des 4 échecs persistants
Pour comprendre les transformations exactes nécessaires
"""

import json
import numpy as np
from pathlib import Path


def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def analyze_transformation(puzzle_id: str):
    """Analyse détaillée d'une transformation"""
    puzzle = load_puzzle(puzzle_id)
    
    print(f"\n{'='*80}")
    print(f"PUZZLE: {puzzle_id}")
    print(f"{'='*80}")
    
    train_pairs = puzzle['train']
    
    for i, pair in enumerate(train_pairs):
        inp = np.array(pair['input'])
        out = np.array(pair['output'])
        
        print(f"\n--- Train Pair {i+1} ---")
        print(f"Input shape:  {inp.shape}")
        print(f"Output shape: {out.shape}")
        print(f"Shape ratio:  {inp.shape[0]/out.shape[0]:.2f}x{inp.shape[1]/out.shape[1]:.2f}")
        
        # Couleurs
        inp_colors = set(inp.flatten())
        out_colors = set(out.flatten())
        print(f"Input colors:  {sorted(inp_colors)}")
        print(f"Output colors: {sorted(out_colors)}")
        print(f"New colors:    {sorted(out_colors - inp_colors)}")
        print(f"Lost colors:   {sorted(inp_colors - out_colors)}")
        
        # Patterns
        print(f"\nInput grid:")
        print(inp)
        print(f"\nOutput grid:")
        print(out)
        
        # Détection patterns spécifiques
        if inp.shape[1] % out.shape[1] == 0 and inp.shape[0] == out.shape[0]:
            ratio = inp.shape[1] // out.shape[1]
            print(f"\n→ PATTERN DÉTECTÉ: Compression horizontale x{ratio}")
            
            # Vérifier si pattern répété
            pattern_width = out.shape[1]
            first_pattern = inp[:, :pattern_width]
            print(f"  Premier pattern ({pattern_width} cols):")
            print(f"  {first_pattern}")
            
            is_repeated = True
            for j in range(1, ratio):
                start = j * pattern_width
                end = start + pattern_width
                current = inp[:, start:end]
                if not np.array_equal(first_pattern, current):
                    is_repeated = False
                    print(f"  Pattern {j+1} DIFFÉRENT")
                    break
            
            if is_repeated:
                print(f"  ✓ Pattern répété {ratio} fois")
                print(f"  → Transformation: compress_periodic_pattern")
        
        # Downscale non-entier
        if inp.shape != out.shape:
            h_ratio = inp.shape[0] / out.shape[0]
            w_ratio = inp.shape[1] / out.shape[1]
            print(f"\n→ DOWNSCALE: {h_ratio:.2f}x{w_ratio:.2f}")
            
            if h_ratio != int(h_ratio) or w_ratio != int(w_ratio):
                print(f"  ⚠️ Ratio NON-ENTIER")
                print(f"  → Transformation: sample_grid_non_integer({out.shape})")
        
        # Détection objets et recolor
        if out_colors != inp_colors:
            print(f"\n→ RECOLORATION détectée")
            print(f"  Nouvelles couleurs: {sorted(out_colors - inp_colors)}")


def main():
    """Analyse les 4 échecs"""
    failures = [
        "2dee498d",  # periodic (67%)
        "3aa6fb7a",  # object_relational (80%)
        "5bd6f4ac",  # semantic_region (100%)
        "88a62173"   # semantic_region (100%)
    ]
    
    for puzzle_id in failures:
        analyze_transformation(puzzle_id)
    
    print(f"\n{'='*80}")
    print("SYNTHÈSE DES TRANSFORMATIONS NÉCESSAIRES")
    print(f"{'='*80}")
    print("\n2dee498d: compress_periodic_pattern (pattern 3x9 répété 3 fois → 3x3)")
    print("3aa6fb7a: Détection formes L + recolor conditionnel (multi-étapes)")
    print("5bd6f4ac: extract_semantic_region + downscale (composition)")
    print("88a62173: sample_grid_non_integer(5x5 → 2x2, ratio 2.5)")


if __name__ == "__main__":
    main()

# Made with Bob
