#!/usr/bin/env python3
"""
Test rapide de la nouvelle primitive extract_periodic_columns sur 2dee498d
"""

import numpy as np
import sys
sys.path.insert(0, '/home/lvx/LVX/lumvorax2/src/MAGEN')

from dsl.primitives_advanced import extract_periodic_columns

# Train Pair 1 de 2dee498d
input_grid = np.array([
    [4, 5, 1, 1, 5, 4, 4, 5, 1],
    [5, 5, 5, 5, 5, 5, 5, 5, 5],
    [1, 5, 4, 4, 5, 1, 1, 5, 4]
])

expected_output = np.array([
    [4, 5, 1],
    [5, 5, 5],
    [1, 5, 4]
])

print("=" * 80)
print("TEST extract_periodic_columns sur 2dee498d Train Pair 1")
print("=" * 80)
print()
print("Input shape:", input_grid.shape)
print("Input:")
print(input_grid)
print()
print("Expected output shape:", expected_output.shape)
print("Expected output:")
print(expected_output)
print()

# Test avec period=3
result = extract_periodic_columns(input_grid, period=3)

print("Result with period=3:")
print("  Shape:", result.shape)
print("  Content:")
print(result)
print()
print("  Match expected:", np.array_equal(result, expected_output))
print()

if np.array_equal(result, expected_output):
    print("✅ SUCCESS: extract_periodic_columns résout 2dee498d Train Pair 1!")
else:
    print("❌ FAIL: Output ne match pas")
    print()
    print("Différences:")
    diff = result != expected_output
    if diff.any():
        print("  Positions différentes:", np.argwhere(diff))

# Made with Bob
