"""
Test de diagnostic V2 - Identifier pourquoi aucun programme n'est généré
"""

import json
import numpy as np
from dsl.composer import Composer
from dsl.primitives import *

# Charger un puzzle simple
with open("arc-agi_training_challenges.json", 'r') as f:
    dataset = json.load(f)

puzzle_id = "2dee498d"
puzzle = dataset[puzzle_id]

print(f"=== DEBUG PUZZLE {puzzle_id} ===\n")

# Analyser train pairs
train_pairs = []
for i, pair in enumerate(puzzle['train']):
    inp = np.array(pair['input'])
    out = np.array(pair['output'])
    train_pairs.append({'input': inp, 'output': out})
    
    print(f"Train pair {i+1}:")
    print(f"  Input shape:  {inp.shape}")
    print(f"  Output shape: {out.shape}")
    print(f"  Input:\n{inp}")
    print(f"  Output:\n{out}")
    print()

# Test Composer
print("=== TEST COMPOSER ===\n")
composer = Composer(max_depth=5, max_candidates=100)

first_input = train_pairs[0]['input']
first_output = train_pairs[0]['output']

print(f"Génération programmes single-op...")
single_op = composer.generate_single_op_programs(first_input)
print(f"  Générés: {len(single_op)} programmes")
if single_op:
    print(f"  Premier: {single_op[0]}")

print(f"\nGénération programmes from analysis...")
analysis = composer.generate_programs_from_analysis(first_input, first_output, max_programs=10)
print(f"  Générés: {len(analysis)} programmes")
for i, prog in enumerate(analysis[:3]):
    print(f"  {i+1}. {prog}")

print(f"\nGénération programmes two-op...")
two_op = composer.generate_two_op_programs(first_input, categories=['geometric', 'scale'])
print(f"  Générés: {len(two_op)} programmes")

# Test exécution d'un programme simple
print("\n=== TEST EXÉCUTION PROGRAMME ===\n")
from dsl.program import Program

# Test rotate90
prog = Program()
prog.add_operation('rotate90', rotate90)
print(f"Programme: {prog}")

try:
    result = prog.execute(first_input)
    print(f"✓ Exécution réussie")
    print(f"  Result shape: {result.shape}")
    print(f"  Match output: {np.array_equal(result, first_output)}")
except Exception as e:
    print(f"✗ Erreur: {e}")

# Test scoring
print("\n=== TEST SCORING ===\n")
from synthesis.scorer import ProgramScorer

scorer = ProgramScorer()
try:
    score = scorer.score_program(prog, train_pairs)
    print(f"✓ Score calculé: {score:.3f}")
except Exception as e:
    print(f"✗ Erreur scoring: {e}")
    import traceback
    traceback.print_exc()

# Made with Bob
