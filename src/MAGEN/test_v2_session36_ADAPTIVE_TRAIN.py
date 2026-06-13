#!/usr/bin/env python3
"""
Test V2 SESSION 36 - ADAPTIVE STRATEGY + TRAIN VALIDATION
Intègre AdaptiveStrategy dans boucle d'apprentissage Session 33
OBJECTIF: Récupérer 3aa6fb7a et 88a62173 → 11/11 (100%)
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


def generate_programs_with_fallback(train_pairs, test_input, classification, 
                                    memory: LearningMemory, puzzle_id: str):
    """Génère programmes avec fallback multi-classe"""
    programs = []
    
    # 1. Famille primaire
    programs.extend(_generate_for_classification(train_pairs, test_input, classification))
    
    # 2. Fallback: Puzzles similaires
    similar_solved = memory.get_similar_puzzles(puzzle_id, classification)
    if similar_solved:
        print(f"    → Puzzles similaires résolus: {len(similar_solved)}")
    
    # 3. Fallback: Stratégie alternative
    should_retry, suggested_strategy = memory.should_retry_puzzle(puzzle_id)
    if should_retry and suggested_strategy:
        print(f"    → Stratégie alternative suggérée par mémoire")
    
    return programs


def _generate_for_classification(train_pairs, test_input, classification):
    """Génère programmes pour une classification donnée"""
    programs = []
    
    # Shape programs
    shape_detector = ShapeDetector()
    shape_programs = shape_detector.generate_shape_programs(train_pairs, test_input)
    programs.extend(shape_programs)
    
    # Rule programs
    rule_detector = RuleDetector()
    rule_programs = rule_detector.generate_rule_programs(train_pairs)
    programs.extend(rule_programs)
    
    # Structural programs
    structural_detector = StructuralDetector()
    structural_programs = structural_detector.generate_marking_programs(train_pairs, test_input)
    programs.extend(structural_programs)
    
    # Region extraction programs
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


def synthesize_with_adaptive(train_pairs, test_input, puzzle_id, 
                             memory: LearningMemory, 
                             adaptive: AdaptiveStrategy,
                             max_retries: int = 3):
    """
    Synthèse avec AdaptiveStrategy + Apprentissage
    
    NOUVEAUTÉ SESSION 36:
    - Intégrer AdaptiveStrategy pour programmes score > 0.55
    - Valider sur TRAIN au lieu de TEST
    - Récupérer échecs 3aa6fb7a et 88a62173
    """
    # Classifier
    classifier = PuzzleClassifier()
    classification_result = classifier.classify(train_pairs)
    classification = classification_result.primary_class.value
    confidence = classification_result.confidence
    
    print(f"  Classification: {classification} ({confidence:.0%})")
    
    # Convertir train_pairs
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    train_hash = compute_train_hash(train_pairs)
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    # Statistiques adaptatives
    adaptive_attempts = 0
    adaptive_successes = 0
    
    # BOUCLE D'APPRENTISSAGE
    for attempt in range(max_retries):
        if attempt > 0:
            print(f"  → Tentative {attempt + 1}/{max_retries} (apprentissage)")
        
        # Générer programmes
        candidates = generate_programs_with_fallback(
            train_pairs, test_input, classification, memory, puzzle_id
        )
        
        print(f"  Candidats générés: {len(candidates)}")
        
        best_program = None
        best_score = -1.0
        
        for program in candidates:
            try:
                score = scorer.score_program(program, train_dicts)
                
                # Validation stricte
                is_valid, accuracy = validator.validate_on_train(program, train_dicts)
                
                if is_valid:
                    if score > best_score:
                        best_score = score
                        best_program = program
                
                # NOUVEAUTÉ SESSION 36: AdaptiveStrategy pour scores > 0.55
                elif score > 0.55:
                    print(f"    Programme score={score:.2f}, validation échouée → Adaptive retry")
                    adaptive_attempts += 1
                    
                    # Valider sur TRAIN (premier exemple)
                    train_input = train_pairs[0][0]
                    train_output = train_pairs[0][1]
                    
                    # Fonctions pour AdaptiveStrategy
                    def execute_fn(prog, inp):
                        try:
                            return prog.execute(inp)
                        except:
                            return None
                    
                    def validate_fn(result, expected):
                        if result is None or expected is None:
                            return False, 0.0
                        if not isinstance(result, np.ndarray):
                            result = np.array(result)
                        if not isinstance(expected, np.ndarray):
                            expected = np.array(expected)
                        if result.shape != expected.shape:
                            return False, 0.0
                        matches = np.sum(result == expected)
                        total = expected.size
                        score = matches / total if total > 0 else 0.0
                        return score >= 0.95, score
                    
                    # Retry adaptatif
                    success, adapted_program, history = adaptive.adapt_and_retry(
                        puzzle_id=puzzle_id,
                        initial_program=program,
                        execute_fn=execute_fn,
                        validate_fn=validate_fn,
                        input_grid=train_input,
                        expected_output=train_output
                    )
                    
                    if success:
                        adaptive_successes += 1
                        # Valider sur TOUS les train pairs
                        is_valid_all, _ = validator.validate_on_train(adapted_program, train_dicts)
                        if is_valid_all:
                            print(f"    ✓ Récupération adaptative réussie!")
                            best_program = adapted_program
                            best_score = 1.0
                            break
            
            except Exception as e:
                pass
        
        if best_program:
            # SUCCÈS
            depth = len(best_program.operations)
            print(f"  ✓ Programme trouvé (score: {best_score:.3f}, depth: {depth})")
            print(f"    {best_program}")
            
            if adaptive_successes > 0:
                print(f"  🔄 Récupération adaptative: {adaptive_successes}/{adaptive_attempts}")
            
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
            
            return True, adaptive_attempts, adaptive_successes
        
        # ÉCHEC
        failure_reason = _diagnose_failure(train_pairs, test_input, classification, candidates)
        print(f"  ✗ Échec tentative {attempt + 1}: {failure_reason}")
        
        if adaptive_attempts > 0:
            print(f"  🔄 Tentatives adaptatives: {adaptive_attempts}, succès: {adaptive_successes}")
        
        experience = PuzzleExperience(
            puzzle_id=puzzle_id,
            timestamp=datetime.now().isoformat(),
            success=False,
            classification=classification,
            confidence=confidence,
            programs_tried=len(candidates),
            best_score=best_score,
            solution_program=None,
            failure_reason=failure_reason,
            train_pairs_hash=train_hash
        )
        memory.store_experience(experience)
        
        if attempt < max_retries - 1:
            should_retry, suggested = memory.should_retry_puzzle(puzzle_id)
            if not should_retry:
                break
    
    print(f"  ✗ Aucun programme valide après {max_retries} tentatives")
    return False, adaptive_attempts, adaptive_successes


def _diagnose_failure(train_pairs, test_input, classification, candidates):
    """Diagnostique raison de l'échec"""
    if len(candidates) == 0:
        return "no_candidates_generated"
    
    if classification == "object_relational":
        return "relational_marking_failed"
    elif classification == "semantic_region":
        return "region_extraction_failed"
    elif "scale" in classification or "sample" in classification:
        return "non_integer_sampling_failed"
    else:
        return "validation_failed"


def main():
    """Test Session 36 avec AdaptiveStrategy"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 SESSION 36 - ADAPTIVE STRATEGY + TRAIN VALIDATION")
    print("Baseline Session 33 (9/11) + AdaptiveStrategy intégrée")
    print("OBJECTIF: Récupérer 3aa6fb7a et 88a62173 → 11/11 (100%)")
    print("="*80)
    
    # Initialiser mémoire et adaptive
    memory = LearningMemory()
    adaptive = AdaptiveStrategy(max_attempts=3)
    
    # Stats mémoire
    stats = memory.get_learning_stats()
    if stats['total_experiences'] > 0:
        print(f"\n📚 Mémoire chargée:")
        print(f"  - Expériences totales: {stats['total_experiences']}")
        print(f"  - Taux de succès: {stats['success_rate']:.1%}")
        print(f"  - Patterns d'échecs: {stats['failure_patterns']}")
    
    successes = 0
    baseline_solved = {
        '2dee498d', '3c9b0459', '5bd6f4ac', '6150a2bd', '67a3c6ac',
        '68b16354', '9172f3a0', 'c59eb873', 'ed36ccf7'
    }
    current_solved = set()
    
    total_adaptive_attempts = 0
    total_adaptive_successes = 0
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        start = time.time()
        success, attempts, recoveries = synthesize_with_adaptive(
            train_pairs, test_input, puzzle_id, memory, adaptive, max_retries=3
        )
        elapsed = time.time() - start
        
        if success:
            successes += 1
            current_solved.add(puzzle_id)
        
        total_adaptive_attempts += attempts
        total_adaptive_successes += recoveries
        
        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    # Anti-regression
    regression = baseline_solved - current_solved
    improvement = current_solved - baseline_solved
    
    if regression:
        print(f"⚠️ RÉGRESSION: {len(regression)} puzzles perdus")
        print(f"   Puzzles perdus: {regression}")
    
    if improvement:
        print(f"✅ AMÉLIORATION: +{len(improvement)} puzzles récupérés!")
        print(f"   Puzzles récupérés: {improvement}")
    
    # Stats AdaptiveStrategy
    print(f"\n🔄 STATISTIQUES ADAPTIVE STRATEGY:")
    print(f"  - Tentatives adaptatives totales: {total_adaptive_attempts}")
    print(f"  - Récupérations réussies: {total_adaptive_successes}")
    if total_adaptive_attempts > 0:
        print(f"  - Taux succès adaptation: {100*total_adaptive_successes/total_adaptive_attempts:.1f}%")
    print(f"  - Contribution au score: {total_adaptive_successes}/{successes} succès")
    
    # Export knowledge base
    kb_path = Path("memory/knowledge_base_session36.json")
    with open(kb_path, 'w') as f:
        json.dump(memory.export_knowledge_base(), f, indent=2)
    print(f"\n💾 Knowledge base exportée: {kb_path}")
    
    if successes == 11:
        print("\n🎯 OBJECTIF ATTEINT: 11/11 (100%)")
        print("✅ Prêt pour test dataset complet (400 puzzles)")
    elif successes >= 10:
        print(f"\n✓ PROGRESSION MAJEURE: {successes}/11")
    elif successes == 9:
        print("\n✓ Baseline maintenue: 9/11")
    else:
        print(f"\n⚠️ Score: {successes}/11")
    
    print(f"\nGain vs Session 35 (9/11): {successes-9:+d} puzzles")


if __name__ == "__main__":
    main()

# Made with Bob - Session 36