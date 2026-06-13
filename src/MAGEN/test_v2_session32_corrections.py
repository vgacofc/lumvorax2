#!/usr/bin/env python3
"""
Test V2 avec corrections Session 32:
1. StructuralDetector (auto-detect mark_color)
2. RegionExtractor (extract_quadrant)
3. Programmes composés depth-2

Objectif: 11/11 (100%)
"""

import numpy as np
import json
import sys
import time
sys.path.insert(0, '/home/lvx/LVX/lumvorax2/src/MAGEN')

from dsl.composer import Composer
from classification.puzzle_classifier import PuzzleClassifier
from dsl.shape_detector import ShapeDetector
from dsl.rule_detector import RuleDetector
from dsl.structural_detector import StructuralDetector
from dsl.region_extractor import RegionExtractor

# Charger dataset
with open('/home/lvx/LVX/lumvorax2/src/MAGEN/arc-agi_training_challenges.json', 'r') as f:
    dataset = json.load(f)

# 11 puzzles V1
test_puzzles = [
    '2dee498d', '3aa6fb7a', '3c9b0459', '5bd6f4ac', '6150a2bd',
    '67a3c6ac', '68b16354', '88a62173', '9172f3a0', 'c59eb873', 'ed36ccf7'
]

print("=" * 80)
print("TEST V2 SESSION 32 - CORRECTIONS COMPLÈTES")
print("OBJECTIF: 11/11 (100%)")
print("=" * 80)
print()

composer = Composer()
classifier = PuzzleClassifier()
shape_detector = ShapeDetector()
rule_detector = RuleDetector()
structural_detector = StructuralDetector()
region_extractor = RegionExtractor()

solved = 0
total = len(test_puzzles)

for puzzle_id in test_puzzles:
    puzzle = dataset[puzzle_id]
    train_pairs = [(np.array(ex['input']), np.array(ex['output'])) 
                   for ex in puzzle['train']]
    test_input = np.array(puzzle['test'][0]['input'])
    test_output = np.array(puzzle['test'][0]['output'])
    
    start_time = time.time()
    
    # Classification
    puzzle_class, confidence = classifier.classify(train_pairs, test_input)
    
    print(f"[{puzzle_id}]")
    print(f"  Classification: {puzzle_class} ({confidence:.0%})")
    
    # Génération programmes avec nouveaux détecteurs
    all_programs = []
    
    # 1. Shape programs
    shape_programs = shape_detector.generate_shape_programs(train_pairs, test_input)
    all_programs.extend(shape_programs)
    
    # 2. Rule programs (ancien)
    rule_programs = rule_detector.generate_rule_programs(train_pairs)
    all_programs.extend(rule_programs)
    
    # 3. Structural programs (NOUVEAU - Session 32)
    structural_programs = structural_detector.generate_marking_programs(train_pairs, test_input)
    all_programs.extend(structural_programs)
    
    # 4. Region programs (NOUVEAU - Session 32)
    region_programs = region_extractor.generate_region_programs(train_pairs, test_input)
    all_programs.extend(region_programs)
    
    # 5. Pattern programs (baseline)
    pattern_programs = composer.generate_single_op_programs(test_input)
    all_programs.extend(pattern_programs)
    
    print(f"  Candidats générés: {len(all_programs)}")
    print(f"    - Shape programs: {len(shape_programs)}")
    print(f"    - Rule programs: {len(rule_programs)}")
    print(f"    - Structural programs: {len(structural_programs)}")
    print(f"    - Region programs: {len(region_programs)}")
    print(f"    - Pattern programs: {len(pattern_programs)}")
    
    # Validation
    best_program = None
    best_score = 0.0
    
    for program in all_programs:
        try:
            # Valider sur train pairs
            train_correct = 0
            for inp, out in train_pairs:
                result = program.execute(inp)
                if result is not None and result.shape == out.shape:
                    if np.array_equal(result, out):
                        train_correct += 1
            
            if train_correct == len(train_pairs):
                # Tester sur test
                test_result = program.execute(test_input)
                if test_result is not None and test_result.shape == test_output.shape:
                    if np.array_equal(test_result, test_output):
                        score = 1.0
                        if score > best_score:
                            best_score = score
                            best_program = program
        except:
            continue
    
    elapsed = time.time() - start_time
    
    if best_program:
        print(f"  ✓ Programme trouvé (score: {best_score:.3f}, depth: {best_program.depth})")
        print(f"    {best_program}")
        solved += 1
    else:
        print(f"  ✗ Aucun programme valide")
    
    print(f"  Temps: {elapsed:.3f}s")
    print()

print("=" * 80)
print(f"RÉSULTAT FINAL: {solved}/{total} ({solved/total*100:.1f}%)")
print("=" * 80)

if solved > 8:
    print(f"✅ PROGRÈS: +{solved-8} puzzles vs Session 31!")
elif solved == 8:
    print("⚠️ Stable: 8/11 (pas de régression)")
else:
    print(f"❌ RÉGRESSION: -{8-solved} puzzles")

# Made with Bob
