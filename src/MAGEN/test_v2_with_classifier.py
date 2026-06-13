#!/usr/bin/env python3
"""
Test V2 avec Puzzle Classifier intégré
Version simplifiée pour validation rapide
"""

import json
import numpy as np
from pathlib import Path
import time

from classification import PuzzleClassifier, TransformationClass
from dsl.composer import Composer
from synthesis.scorer import ProgramScorer
from synthesis.validator import ProgramValidator


def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def synthesize_with_classification(train_pairs, test_input, puzzle_id):
    """
    Synthèse guidée par classification
    
    1. Classifier le puzzle
    2. Générer candidats avec priorités
    3. Scorer et valider
    """
    # Classifier
    classifier = PuzzleClassifier()
    classification = classifier.classify(train_pairs)
    
    print(f"\n  Classification: {classification.primary_class.value} ({classification.confidence:.0%})")
    
    # Composer avec priorités basées sur classe
    composer = Composer(max_depth=7, max_candidates=200)
    
    # Générer candidats (pour l'instant, utiliser méthode standard)
    # TODO: Implémenter génération spécialisée par classe
    candidates = []
    
    # Générer programmes single-op
    for category, primitives in composer.primitives.items():
        for name, func, arg_gen in primitives:
            try:
                args_list = arg_gen(train_pairs[0][0])
                for args in args_list:
                    from dsl.program import Program
                    prog = Program()
                    prog.add_operation(name, func, args)
                    candidates.append(prog)
            except:
                pass
    
    print(f"  Candidats générés: {len(candidates)}")
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    best_program = None
    best_score = -1.0
    
    # Convertir train_pairs en format Dict pour scorer/validator
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    
    for program in candidates[:200]:  # Limiter à 200
        try:
            score = scorer.score_program(program, train_dicts)
            if score > best_score:
                is_valid, accuracy = validator.validate_on_train(program, train_dicts)
                if is_valid:
                    best_score = score
                    best_program = program
        except:
            pass
    
    if best_program:
        print(f"  ✓ Programme trouvé (score: {best_score:.3f})")
        print(f"    {best_program}")
        return True
    else:
        print(f"  ✗ Aucun programme valide")
        return False


def main():
    """Test sur les 11 puzzles"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 AVEC CLASSIFIER - 11 PUZZLES")
    print("="*80)
    
    successes = 0
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        start = time.time()
        success = synthesize_with_classification(train_pairs, test_input, puzzle_id)
        elapsed = time.time() - start
        
        if success:
            successes += 1
        
        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    if successes >= 9:
        print("✓ OBJECTIF ATTEINT (9+/11)")
    elif successes >= 7:
        print("⚠️ Stable (7/11)")
    else:
        print("✗ Régression")


if __name__ == "__main__":
    main()

# Made with Bob
