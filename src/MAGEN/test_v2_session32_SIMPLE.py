#!/usr/bin/env python3
"""
Test V2 FINAL avec Knowledge V1 intégré
ShapeDetector + RuleDetector + Compositions ciblées
OBJECTIF: 11/11 (100%)
"""

import json
import numpy as np
from pathlib import Path
import time

from classification import PuzzleClassifier
from dsl.shape_detector import ShapeDetector
from dsl.rule_detector import RuleDetector
from dsl.composer import Composer
from dsl.program import Program
from synthesis.scorer import ProgramScorer
from synthesis.validator import ProgramValidator


def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def generate_v1_inspired_programs(train_pairs, test_input):
    """
    Génère programmes inspirés de V1
    
    Ordre de priorité (comme V1):
    1. ShapeDetector (shape changes)
    2. RuleDetector (règles contextuelles)
    3. PatternMatcher (patterns exacts - déjà dans composer)
    """
    programs = []
    
    # 1. Shape programs (priorité absolue)
    shape_detector = ShapeDetector()
    shape_programs = shape_detector.generate_shape_programs(train_pairs, test_input)
    programs.extend(shape_programs)
    
    # 2. Rule programs
    rule_detector = RuleDetector()
    rule_programs = rule_detector.generate_rule_programs(train_pairs)
    programs.extend(rule_programs)
    
    # 3. Standard single-op programs (patterns exacts)
    composer = Composer(max_depth=7, max_candidates=200)
    for category, primitives in composer.primitives.items():
        # Prioriser geometric et scale (patterns exacts)
        if category in ['geometric', 'scale']:
            for name, func, arg_gen in primitives:
                try:
                    args_list = arg_gen(train_pairs[0][0])
                    for args in args_list:
                        prog = Program()
                        prog.add_operation(name, func, args)
                        programs.append(prog)
                except:
                    pass
    
    return programs


def synthesize_v1_style(train_pairs, test_input, puzzle_id):
    """
    Synthèse style V1: Analyse puis génération ciblée
    """
    # Classifier (pour info)
    classifier = PuzzleClassifier()
    classification = classifier.classify(train_pairs)
    
    print(f"  Classification: {classification.primary_class.value} ({classification.confidence:.0%})")
    
    # Générer programmes V1-style
    candidates = generate_v1_inspired_programs(train_pairs, test_input)
    
    print(f"  Candidats générés: {len(candidates)}")
    if len(candidates) > 0:
        print(f"    - Shape programs: {sum(1 for p in candidates if any('shape' in op.name.lower() or 'scale' in op.name.lower() or 'compress' in op.name.lower() or 'sample' in op.name.lower() for op in p.operations))}")
        print(f"    - Rule programs: {sum(1 for p in candidates if any('detect' in op.name.lower() or 'mark' in op.name.lower() for op in p.operations))}")
        print(f"    - Pattern programs: {sum(1 for p in candidates if any('rotate' in op.name.lower() or 'mirror' in op.name.lower() for op in p.operations))}")
    
    # Convertir train_pairs en format Dict
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    best_program = None
    best_score = -1.0
    
    for program in candidates:
        try:
            score = scorer.score_program(program, train_dicts)
            if score > best_score:
                is_valid, accuracy = validator.validate_on_train(program, train_dicts)
                if is_valid:
                    best_score = score
                    best_program = program
        except Exception as e:
            pass
    
    if best_program:
        depth = len(best_program.operations)
        print(f"  ✓ Programme trouvé (score: {best_score:.3f}, depth: {depth})")
        print(f"    {best_program}")
        return True
    else:
        print(f"  ✗ Aucun programme valide")
        return False


def main():
    """Test sur les 11 puzzles avec V1 knowledge"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 FINAL - V1 KNOWLEDGE INTÉGRÉ")
    print("OBJECTIF: 11/11 (100%)")
    print("="*80)
    
    successes = 0
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        start = time.time()
        success = synthesize_v1_style(train_pairs, test_input, puzzle_id)
        elapsed = time.time() - start
        
        if success:
            successes += 1
        
        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    if successes == 11:
        print("✓✓✓ OBJECTIF ATTEINT: 11/11 (100%) ✓✓✓")
        print("V2 = V1 sur les 11 puzzles!")
    elif successes >= 9:
        print(f"✓ PROGRESSION: {successes}/11 (+{successes-7} vs baseline)")
    elif successes >= 7:
        print("⚠️ Stable: 7/11 (pas de régression)")
    else:
        print(f"✗ Régression: {successes}/11")
    
    print(f"\nGain vs V2 baseline: +{successes-7} puzzles")


if __name__ == "__main__":
    main()

# Made with Bob
