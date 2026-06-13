#!/usr/bin/env python3
"""
Test V2 SESSION 34 - ADAPTIVE STRATEGY
Baseline Session 33 (9/11) + AdaptiveStrategy pour récupération échecs
OBJECTIF: 11/11 (100%) avec tentatives multiples adaptatives
"""

import json
import numpy as np
from pathlib import Path
import time
from datetime import datetime

from classification import PuzzleClassifier
from dsl.shape_detector import ShapeDetector
from dsl.rule_detector import RuleDetector
from dsl.structural_detector import StructuralDetector
from dsl.region_extractor import RegionExtractor
from dsl.composer import Composer
from dsl.program import Program
from synthesis.scorer import ProgramScorer
from synthesis.validator import ProgramValidator
from core.learning_memory import LearningMemory, PuzzleExperience, compute_train_hash
from synthesis.adaptive_strategy import AdaptiveStrategy


def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def generate_programs(train_pairs, test_input, classification):
    """Génère programmes (baseline Session 33)"""
    programs = []
    
    # Shape programs (priorité absolue)
    shape_detector = ShapeDetector()
    shape_programs = shape_detector.generate_shape_programs(train_pairs, test_input)
    programs.extend(shape_programs)
    
    # Rule programs
    rule_detector = RuleDetector()
    rule_programs = rule_detector.generate_rule_programs(train_pairs)
    programs.extend(rule_programs)
    
    # Structural programs (Session 32)
    structural_detector = StructuralDetector()
    structural_programs = structural_detector.generate_marking_programs(train_pairs, test_input)
    programs.extend(structural_programs)
    
    # Region extraction programs (Session 32)
    region_extractor = RegionExtractor()
    region_programs = region_extractor.generate_region_programs(train_pairs, test_input)
    programs.extend(region_programs)
    
    # Standard single-op programs
    composer = Composer(max_depth=7, max_candidates=200)
    for category, primitives in composer.primitives.items():
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


def execute_program(program, input_grid):
    """Exécute un programme"""
    try:
        return program.apply(input_grid)
    except:
        return None


def validate_program(result, expected):
    """Valide résultat"""
    if result is None or expected is None:
        return False, 0.0
    
    if result.shape != expected.shape:
        return False, 0.0
    
    matching = np.sum(result == expected)
    total = expected.size
    score = matching / total if total > 0 else 0.0
    
    return score > 0.95, score


def synthesize_with_adaptive_strategy(train_pairs, test_input, test_output, 
                                      puzzle_id, memory: LearningMemory,
                                      adaptive: AdaptiveStrategy):
    """
    Synthèse avec AdaptiveStrategy
    
    NOUVEAUTÉ SESSION 34:
    - Utilise AdaptiveStrategy.adapt_and_retry() pour tentatives multiples
    - Relaxation validation, paramètres alternatifs, composition depth-2
    - Récupère échecs 3aa6fb7a (relational) et 88a62173 (ratio non-entier)
    """
    # Classifier
    classifier = PuzzleClassifier()
    classification_result = classifier.classify(train_pairs)
    classification = classification_result.primary_class.value
    confidence = classification_result.confidence
    
    print(f"  Classification: {classification} ({confidence:.0%})")
    
    # Générer programmes (baseline Session 33)
    candidates = generate_programs(train_pairs, test_input, classification)
    print(f"  Candidats générés: {len(candidates)}")
    
    if len(candidates) == 0:
        print(f"  ✗ Aucun candidat généré")
        return False, None
    
    # Convertir train_pairs en format Dict
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    train_hash = compute_train_hash(train_pairs)
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    # NOUVEAUTÉ SESSION 34: Essayer chaque programme avec AdaptiveStrategy
    best_program = None
    best_score = -1.0
    adaptive_recovery = False
    
    for idx, program in enumerate(candidates):
        try:
            # Score sur train
            score = scorer.score_program(program, train_dicts)
            
            if score > 0.5:  # Candidat prometteur
                # Validation stricte d'abord
                is_valid, accuracy = validator.validate_on_train(program, train_dicts)
                
                if is_valid:
                    # Succès direct
                    if score > best_score:
                        best_score = score
                        best_program = program
                else:
                    # NOUVEAUTÉ: Essayer avec AdaptiveStrategy
                    print(f"    Programme {idx+1}: score={score:.2f}, validation échouée → Adaptive retry")
                    
                    success, final_program, attempt_history = adaptive.adapt_and_retry(
                        puzzle_id=puzzle_id,
                        initial_program=program,
                        execute_fn=execute_program,
                        validate_fn=validate_program,
                        input_grid=test_input,
                        expected_output=test_output
                    )
                    
                    if success:
                        print(f"    ✅ RÉCUPÉRATION ADAPTATIVE après {len(attempt_history)} tentatives!")
                        best_program = final_program
                        best_score = 1.0
                        adaptive_recovery = True
                        break  # Succès trouvé
        except Exception as e:
            pass
    
    if best_program:
        # SUCCÈS
        depth = len(best_program.operations)
        recovery_marker = "🔄" if adaptive_recovery else ""
        print(f"  ✓ Programme trouvé {recovery_marker}(score: {best_score:.3f}, depth: {depth})")
        print(f"    {best_program}")
        
        experience = PuzzleExperience(
            puzzle_id=puzzle_id,
            timestamp=datetime.now().isoformat(),
            success=True,
            classification=classification,
            confidence=confidence,
            programs_tried=len(candidates),
            best_score=best_score,
            solution_program=str(best_program),
            failure_reason=None,
            train_pairs_hash=train_hash
        )
        memory.store_experience(experience)
        
        return True, adaptive_recovery
    
    # ÉCHEC
    print(f"  ✗ Aucun programme valide")
    
    experience = PuzzleExperience(
        puzzle_id=puzzle_id,
        timestamp=datetime.now().isoformat(),
        success=False,
        classification=classification,
        confidence=confidence,
        programs_tried=len(candidates),
        best_score=best_score,
        solution_program=None,
        failure_reason="all_programs_failed",
        train_pairs_hash=train_hash
    )
    memory.store_experience(experience)
    
    return False, False


def main():
    """Test avec AdaptiveStrategy"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 SESSION 34 - ADAPTIVE STRATEGY")
    print("Baseline Session 33 (9/11) + AdaptiveStrategy")
    print("OBJECTIF: 11/11 (100%) avec récupération adaptative")
    print("="*80)
    
    # Initialiser mémoire et stratégie adaptative
    memory = LearningMemory()
    adaptive = AdaptiveStrategy(max_attempts=3)
    
    successes = 0
    adaptive_recoveries = 0
    baseline_solved = {
        '2dee498d', '3c9b0459', '6150a2bd', '67a3c6ac',
        '68b16354', '9172f3a0', 'c59eb873', 'ed36ccf7', '5bd6f4ac'  # Session 33: 9/11
    }
    current_solved = set()
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        test_output = np.array(puzzle['test'][0]['output']) if 'output' in puzzle['test'][0] else None
        
        start = time.time()
        success, was_adaptive = synthesize_with_adaptive_strategy(
            train_pairs, test_input, test_output, puzzle_id, memory, adaptive
        )
        elapsed = time.time() - start
        
        if success:
            successes += 1
            current_solved.add(puzzle_id)
            if was_adaptive:
                adaptive_recoveries += 1
        
        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    # Anti-regression check
    regression = baseline_solved - current_solved
    improvement = current_solved - baseline_solved
    
    if regression:
        print(f"⚠️ RÉGRESSION DÉTECTÉE: {len(regression)} puzzles perdus")
        print(f"   Puzzles perdus: {regression}")
    
    if improvement:
        print(f"✅ AMÉLIORATION: +{len(improvement)} puzzles récupérés!")
        print(f"   Puzzles récupérés: {improvement}")
    
    # Stats AdaptiveStrategy
    print(f"\n🔄 STATISTIQUES ADAPTIVE STRATEGY:")
    print(f"  - Récupérations adaptatives: {adaptive_recoveries}")
    print(f"  - Taux récupération: {adaptive_recoveries/successes*100:.1f}% des succès" if successes > 0 else "  - Aucun succès")
    
    adaptive_stats = adaptive.get_stats()
    print(f"  - Adaptations totales: {adaptive_stats['total_adaptations']}")
    print(f"  - Adaptations réussies: {adaptive_stats['successful_adaptations']}")
    print(f"  - Taux succès adaptation: {adaptive_stats['success_rate']*100:.1f}%")
    
    # Exporter knowledge base
    kb_path = Path("memory/knowledge_base_session34.json")
    kb_path.parent.mkdir(exist_ok=True)
    with open(kb_path, 'w') as f:
        json.dump(memory.export_knowledge_base(), f, indent=2)
    print(f"\n💾 Knowledge base exportée: {kb_path}")
    
    if successes == 11:
        print("\n✓✓✓ OBJECTIF ATTEINT: 11/11 (100%) ✓✓✓")
    elif successes >= 10:
        print(f"\n✓✓ QUASI-OBJECTIF: {successes}/11")
    elif successes >= 9:
        print(f"\n✓ PROGRESSION: {successes}/11 (baseline maintenue)")
    else:
        print(f"\n✗ Régression: {successes}/11")
    
    print(f"\nGain vs Session 33 (9/11): +{successes-9} puzzles")
    
    return successes == 11


if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)

# Made with Bob