#!/usr/bin/env python3
"""
Test V2 avec génération de programmes composés (depth 2-3)
Version avec compositions multi-étapes pour résoudre les 4 échecs
"""

import json
import numpy as np
from pathlib import Path
import time

from classification import PuzzleClassifier, TransformationClass
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


def generate_depth1_programs(composer, train_pairs):
    """Génère programmes depth 1 (single-op)"""
    candidates = []
    
    for category, primitives in composer.primitives.items():
        for name, func, arg_gen in primitives:
            try:
                args_list = arg_gen(train_pairs[0][0])
                for args in args_list:
                    prog = Program()
                    prog.add_operation(name, func, args)
                    candidates.append(prog)
            except:
                pass
    
    return candidates


def generate_depth2_programs(composer, train_pairs, depth1_programs):
    """
    Génère programmes depth 2 (compositions)
    
    Stratégie: Pour chaque programme depth-1, essayer de composer avec
    des primitives compatibles
    """
    candidates = []
    
    # Limiter depth-1 aux meilleurs candidats
    depth1_sample = depth1_programs[:50]  # Top 50
    
    # Catégories compatibles pour composition
    compatible_pairs = [
        ('advanced', 'scale'),      # extract_region + downscale
        ('advanced', 'color'),      # detect_L + recolor
        ('pattern', 'scale'),       # compress + downscale
        ('spatial', 'geometric'),   # crop + rotate
        ('geometric', 'scale'),     # rotate + scale
    ]
    
    for prog1 in depth1_sample:
        # Déterminer catégorie du programme
        if not prog1.operations:
            continue
        
        op1_name = prog1.operations[0].name
        
        # Trouver catégorie
        cat1 = None
        for category, primitives in composer.primitives.items():
            for name, func, arg_gen in primitives:
                if name == op1_name:
                    cat1 = category
                    break
            if cat1:
                break
        
        if not cat1:
            continue
        
        # Composer avec primitives compatibles
        for cat_pair in compatible_pairs:
            if cat1 not in cat_pair:
                continue
            
            # Trouver l'autre catégorie
            cat2 = cat_pair[1] if cat_pair[0] == cat1 else cat_pair[0]
            
            if cat2 not in composer.primitives:
                continue
            
            # Essayer de composer
            for name2, func2, arg_gen2 in composer.primitives[cat2]:
                try:
                    # Exécuter prog1 sur premier train pair
                    intermediate = prog1.execute(train_pairs[0][0])
                    
                    # Générer args pour op2 basés sur intermediate
                    args_list2 = arg_gen2(intermediate)
                    
                    for args2 in args_list2[:3]:  # Limiter à 3 args par primitive
                        # Créer programme composé
                        prog2 = prog1.copy()
                        prog2.add_operation(name2, func2, args2)
                        candidates.append(prog2)
                        
                        # Limiter nombre total
                        if len(candidates) >= 300:
                            return candidates
                except:
                    pass
    
    return candidates


def synthesize_with_compositions(train_pairs, test_input, puzzle_id, classification):
    """
    Synthèse avec compositions multi-étapes
    
    1. Générer depth-1 programs
    2. Générer depth-2 programs (compositions)
    3. Scorer et valider tous
    """
    composer = Composer(max_depth=7, max_candidates=200)
    
    print(f"  Classification: {classification.primary_class.value} ({classification.confidence:.0%})")
    
    # Générer candidats depth-1
    depth1 = generate_depth1_programs(composer, train_pairs)
    print(f"  Candidats depth-1: {len(depth1)}")
    
    # Générer candidats depth-2
    depth2 = generate_depth2_programs(composer, train_pairs, depth1)
    print(f"  Candidats depth-2: {len(depth2)}")
    
    # Combiner tous les candidats
    all_candidates = depth1 + depth2
    print(f"  Total candidats: {len(all_candidates)}")
    
    # Convertir train_pairs en format Dict
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    best_program = None
    best_score = -1.0
    
    for program in all_candidates:
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
        depth = len(best_program.operations)
        print(f"  ✓ Programme trouvé (score: {best_score:.3f}, depth: {depth})")
        print(f"    {best_program}")
        return True
    else:
        print(f"  ✗ Aucun programme valide")
        return False


def main():
    """Test sur les 11 puzzles avec compositions"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 AVEC COMPOSITIONS MULTI-ÉTAPES - 11 PUZZLES")
    print("="*80)
    
    successes = 0
    classifier = PuzzleClassifier()
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        # Classifier
        classification = classifier.classify(train_pairs)
        
        start = time.time()
        success = synthesize_with_compositions(train_pairs, test_input, puzzle_id, classification)
        elapsed = time.time() - start
        
        if success:
            successes += 1
        
        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    if successes >= 11:
        print("✓✓✓ OBJECTIF ATTEINT: 11/11 (100%) ✓✓✓")
    elif successes >= 9:
        print("✓ PROGRESSION: 9+/11")
    elif successes >= 7:
        print("⚠️ Stable: 7/11")
    else:
        print("✗ Régression")


if __name__ == "__main__":
    main()

# Made with Bob
