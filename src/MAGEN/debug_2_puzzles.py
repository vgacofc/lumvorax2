#!/usr/bin/env python3
"""
Debug détaillé des 2 puzzles restants
"""

import json
import numpy as np
from pathlib import Path

def analyze_puzzle_00d62c1b():
    """Analyser puzzle 00d62c1b - Duplication lignes adjacentes"""
    print("\n" + "="*80)
    print("PUZZLE 00d62c1b - DUPLICATION LIGNES ADJACENTES")
    print("="*80)
    
    with open('arc-agi_training_challenges.json', 'r') as f:
        data = json.load(f)
    
    puzzle = data['00d62c1b']
    train = puzzle['train'][0]
    
    inp = np.array(train['input'])
    out = np.array(train['output'])
    
    print("\nINPUT:")
    for i, row in enumerate(inp):
        print(f"Row {i}: {row}")
    
    print("\nOUTPUT:")
    for i, row in enumerate(out):
        print(f"Row {i}: {row}")
    
    print("\nANALYSE:")
    
    # Trouver lignes avec pattern
    pattern_rows = []
    for i, row in enumerate(inp):
        if np.any(row != 0):
            pattern_rows.append(i)
            print(f"  - Ligne {i} contient pattern: {row}")
    
    print(f"\nLignes avec pattern: {pattern_rows}")
    
    # Analyser output
    print("\nDans OUTPUT:")
    for i in pattern_rows:
        if i > 0:
            print(f"  - Ligne {i-1} (au-dessus de {i}): {out[i-1]}")
        print(f"  - Ligne {i} (pattern original): {out[i]}")
        if i < len(out) - 1:
            print(f"  - Ligne {i+1} (en-dessous de {i}): {out[i+1]}")
    
    print("\nRÈGLE DÉTECTÉE:")
    print("  Pour chaque ligne avec pattern non-nul:")
    print("  1. Copier ligne au-dessus avec couleur 4 (jaune)")
    print("  2. Garder ligne originale")
    print("  3. Copier ligne en-dessous avec couleur 4 (jaune)")

def analyze_puzzle_025d127b():
    """Analyser puzzle 025d127b - Rectangle englobant"""
    print("\n" + "="*80)
    print("PUZZLE 025d127b - RECTANGLE ENGLOBANT")
    print("="*80)
    
    with open('arc-agi_training_challenges.json', 'r') as f:
        data = json.load(f)
    
    puzzle = data['025d127b']
    train = puzzle['train'][0]
    
    inp = np.array(train['input'])
    out = np.array(train['output'])
    
    print("\nINPUT:")
    for i, row in enumerate(inp):
        print(f"Row {i}: {row}")
    
    print("\nOUTPUT:")
    for i, row in enumerate(out):
        print(f"Row {i}: {row}")
    
    print("\nANALYSE:")
    
    # Trouver pixels non-nuls
    pixels = []
    for i in range(inp.shape[0]):
        for j in range(inp.shape[1]):
            if inp[i, j] != 0:
                pixels.append((i, j, inp[i, j]))
                print(f"  - Pixel ({i},{j}) = couleur {inp[i, j]}")
    
    if len(pixels) >= 2:
        # Calculer bounding box
        rows = [p[0] for p in pixels]
        cols = [p[1] for p in pixels]
        
        min_row, max_row = min(rows), max(rows)
        min_col, max_col = min(cols), max(cols)
        
        print(f"\nBounding box: rows [{min_row}, {max_row}], cols [{min_col}, {max_col}]")
        
        # Couleur du rectangle
        rect_color = pixels[0][2]
        print(f"Couleur rectangle: {rect_color}")
        
        print("\nRÈGLE DÉTECTÉE:")
        print("  1. Trouver tous pixels non-nuls")
        print("  2. Calculer bounding box englobant")
        print("  3. Dessiner rectangle CREUX avec couleur du premier pixel")
        print("  4. Garder pixels originaux à l'intérieur")

def test_current_pipeline():
    """Tester pipeline actuel sur ces 2 puzzles"""
    print("\n" + "="*80)
    print("TEST PIPELINE ACTUEL")
    print("="*80)
    
    from core.advanced_pipeline import AdvancedPipeline
    
    with open('arc-agi_training_challenges.json', 'r') as f:
        data = json.load(f)
    
    pipeline = AdvancedPipeline()
    
    for puzzle_id in ['00d62c1b', '025d127b']:
        print(f"\n--- Testing {puzzle_id} ---")
        
        puzzle = data[puzzle_id]
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        try:
            result = pipeline.solve_puzzle(train_pairs, test_input)
            
            if result is not None:
                print(f"✓ Solution générée: shape {result.shape}")
                print("Output:")
                for row in result:
                    print(' '.join(str(x) for x in row))
            else:
                print("✗ Aucune solution générée")
                
        except Exception as e:
            print(f"✗ Erreur: {e}")

if __name__ == '__main__':
    analyze_puzzle_00d62c1b()
    analyze_puzzle_025d127b()
    test_current_pipeline()

# Made with Bob
