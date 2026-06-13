#!/usr/bin/env python3
"""
Analyse simple des 4 échecs V2
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

def analyze_puzzle(puzzle_id: str):
    """Analyse un puzzle"""
    print(f"\n{'='*80}")
    print(f"PUZZLE {puzzle_id}")
    print(f"{'='*80}")
    
    puzzle = load_puzzle(puzzle_id)
    
    # Analyser train pairs
    for i, pair in enumerate(puzzle['train']):
        inp = np.array(pair['input'])
        out = np.array(pair['output'])
        
        print(f"\nTrain pair {i+1}:")
        print(f"  Input:  {inp.shape} - colors: {sorted(set(inp.flatten()))}")
        print(f"  Output: {out.shape} - colors: {sorted(set(out.flatten()))}")
        
        # Afficher grilles si petites
        if inp.shape[0] <= 10 and inp.shape[1] <= 10:
            print(f"\n  Input grid:")
            for row in inp:
                print(f"    {row.tolist()}")
            print(f"\n  Output grid:")
            for row in out:
                print(f"    {row.tolist()}")
        
        # Vérifier transformations simples
        if inp.shape == out.shape:
            if np.array_equal(np.rot90(inp, 2), out):
                print(f"  ✓ Transformation: rotate180")
            elif np.array_equal(np.fliplr(inp), out):
                print(f"  ✓ Transformation: mirror_horizontal")
            elif np.array_equal(np.flipud(inp), out):
                print(f"  ✓ Transformation: mirror_vertical")
            elif np.array_equal(inp, out):
                print(f"  ⚠️  Identity (no change)")
            else:
                print(f"  ⚠️  Complex transformation (same shape)")
        else:
            h_ratio = out.shape[0] / inp.shape[0]
            w_ratio = out.shape[1] / inp.shape[1]
            if h_ratio == w_ratio:
                if h_ratio > 1:
                    print(f"  ✓ Transformation: scale({int(h_ratio)})")
                else:
                    print(f"  ✓ Transformation: downscale({int(1/h_ratio)})")
            else:
                print(f"  ⚠️  Complex transformation (shape change)")
                print(f"     Ratio H: {h_ratio:.2f}, W: {w_ratio:.2f}")

def main():
    """Analyse les 4 échecs"""
    failures = ["2dee498d", "3aa6fb7a", "5bd6f4ac", "88a62173"]
    
    print("="*80)
    print("ANALYSE DÉTAILLÉE - 4 ÉCHECS V2")
    print("="*80)
    
    for puzzle_id in failures:
        analyze_puzzle(puzzle_id)
        print()

if __name__ == "__main__":
    main()

# Made with Bob
