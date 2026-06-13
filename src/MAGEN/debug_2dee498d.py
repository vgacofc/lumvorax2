#!/usr/bin/env python3
"""
Debug 2dee498d - Pourquoi shape programs échouent?
"""

import json
import numpy as np
from pathlib import Path

from dsl.shape_detector import ShapeDetector
from dsl.primitives_advanced import compress_periodic_pattern
from dsl.primitives import downscale


def load_puzzle(puzzle_id: str):
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def main():
    puzzle = load_puzzle("2dee498d")
    train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                   for p in puzzle['train']]
    
    print("="*80)
    print("DEBUG 2dee498d")
    print("="*80)
    
    # Analyser train pairs
    for i, (inp, out) in enumerate(train_pairs):
        print(f"\nTrain Pair {i+1}:")
        print(f"  Input shape:  {inp.shape}")
        print(f"  Output shape: {out.shape}")
        print(f"  Ratio: {inp.shape[0]/out.shape[0]:.2f}x{inp.shape[1]/out.shape[1]:.2f}")
        
        print(f"\n  Input:")
        print(inp)
        print(f"\n  Output:")
        print(out)
        
        # Tester compress_periodic_pattern
        print(f"\n  Test compress_periodic_pattern:")
        try:
            result = compress_periodic_pattern(inp)
            print(f"    Result shape: {result.shape}")
            print(f"    Match output: {np.array_equal(result, out)}")
            if not np.array_equal(result, out):
                print(f"    Result:")
                print(f"    {result}")
        except Exception as e:
            print(f"    ERROR: {e}")
        
        # Tester downscale
        if inp.shape[1] % out.shape[1] == 0:
            factor = inp.shape[1] // out.shape[1]
            print(f"\n  Test downscale({factor}):")
            try:
                result = downscale(inp, factor)
                print(f"    Result shape: {result.shape}")
                print(f"    Match output: {np.array_equal(result, out)}")
                if not np.array_equal(result, out):
                    print(f"    Result:")
                    print(f"    {result}")
            except Exception as e:
                print(f"    ERROR: {e}")
    
    # Tester ShapeDetector
    print(f"\n{'='*80}")
    print("ShapeDetector Analysis:")
    print(f"{'='*80}")
    
    detector = ShapeDetector()
    shape_type = detector.detect_transformation_type(train_pairs)
    print(f"Shape type: {shape_type}")
    
    is_periodic = detector.is_periodic_pattern(train_pairs)
    print(f"Is periodic: {is_periodic}")
    
    factor = detector.calculate_downscale_factor(train_pairs)
    print(f"Downscale factor: {factor}")
    
    # Générer programmes
    test_input = np.array(puzzle['test'][0]['input'])
    programs = detector.generate_shape_programs(train_pairs, test_input)
    print(f"\nPrograms generated: {len(programs)}")
    for prog in programs:
        print(f"  - {prog}")


if __name__ == "__main__":
    main()

# Made with Bob
