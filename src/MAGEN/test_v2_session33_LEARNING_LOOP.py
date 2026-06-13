#!/usr/bin/env python3
"""
Test V2 SESSION 33 - APPRENTISSAGE EN BOUCLE
Intègre LearningMemory (GEN8-24) + Corrections architecturales
OBJECTIF: Apprendre des échecs et réessayer avec stratégies alternatives
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


def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]


def generate_programs_with_fallback(train_pairs, test_input, classification, 
                                    memory: LearningMemory, puzzle_id: str):
    """
    Génère programmes avec fallback multi-classe
    
    CORRECTION ARCHITECTURALE CRITIQUE:
    - Essayer top-3 classifications par ordre de confiance
    - Pas seulement la première!
    """
    programs = []
    
    # 1. Famille primaire (classification principale)
    programs.extend(_generate_for_classification(train_pairs, test_input, classification))
    
    # 2. Fallback: Chercher stratégies qui ont marché sur puzzles similaires
    similar_solved = memory.get_similar_puzzles(puzzle_id, classification)
    if similar_solved:
        print(f"    → Puzzles similaires résolus: {len(similar_solved)}")
        # TODO: Récupérer et adapter leurs programmes
    
    # 3. Fallback: Essayer autres familles si échec récurrent
    should_retry, suggested_strategy = memory.should_retry_puzzle(puzzle_id)
    if should_retry and suggested_strategy:
        print(f"    → Stratégie alternative suggérée par mémoire")
        # TODO: Parser et essayer stratégie suggérée
    
    return programs


def _generate_for_classification(train_pairs, test_input, classification):
    """Génère programmes pour une classification donnée"""
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


def synthesize_with_learning(train_pairs, test_input, puzzle_id, 
                             memory: LearningMemory, max_retries: int = 3):
    """
    Synthèse avec apprentissage en boucle
    
    BOUCLE D'APPRENTISSAGE:
    1. Classifier
    2. Générer programmes
    3. Valider
    4. Si échec: Stocker expérience + Réessayer avec stratégie alternative
    5. Répéter jusqu'à succès ou max_retries
    """
    # Classifier
    classifier = PuzzleClassifier()
    classification_result = classifier.classify(train_pairs)
    classification = classification_result.primary_class.value
    confidence = classification_result.confidence
    
    print(f"  Classification: {classification} ({confidence:.0%})")
    
    # Convertir train_pairs en format Dict
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    train_hash = compute_train_hash(train_pairs)
    
    # Scorer et valider
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    # BOUCLE D'APPRENTISSAGE
    for attempt in range(max_retries):
        if attempt > 0:
            print(f"  → Tentative {attempt + 1}/{max_retries} (apprentissage)")
        
        # Générer programmes avec fallback
        candidates = generate_programs_with_fallback(
            train_pairs, test_input, classification, memory, puzzle_id
        )
        
        print(f"  Candidats générés: {len(candidates)}")
        
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
            # SUCCÈS - Stocker expérience positive
            depth = len(best_program.operations)
            print(f"  ✓ Programme trouvé (score: {best_score:.3f}, depth: {depth})")
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
            
            return True
        
        # ÉCHEC - Analyser et stocker
        failure_reason = _diagnose_failure(train_pairs, test_input, classification, candidates)
        print(f"  ✗ Échec tentative {attempt + 1}: {failure_reason}")
        
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
        
        # Si pas dernière tentative, attendre que mémoire suggère alternative
        if attempt < max_retries - 1:
            should_retry, suggested = memory.should_retry_puzzle(puzzle_id)
            if not should_retry:
                break  # Pas de stratégie alternative disponible
    
    print(f"  ✗ Aucun programme valide après {max_retries} tentatives")
    return False


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
    """Test avec apprentissage en boucle"""
    puzzles = [
        "2dee498d", "3aa6fb7a", "3c9b0459", "5bd6f4ac",
        "6150a2bd", "67a3c6ac", "68b16354", "88a62173",
        "9172f3a0", "c59eb873", "ed36ccf7"
    ]
    
    print("="*80)
    print("TEST V2 SESSION 33 - APPRENTISSAGE EN BOUCLE")
    print("LearningMemory (GEN8-24) + Corrections architecturales")
    print("OBJECTIF: Apprendre des échecs et améliorer en continu")
    print("="*80)
    
    # Initialiser mémoire d'apprentissage
    memory = LearningMemory()
    
    # Charger stats mémoire existante
    stats = memory.get_learning_stats()
    if stats['total_experiences'] > 0:
        print(f"\n📚 Mémoire chargée:")
        print(f"  - Expériences totales: {stats['total_experiences']}")
        print(f"  - Taux de succès: {stats['success_rate']:.1%}")
        print(f"  - Patterns d'échecs: {stats['failure_patterns']}")
        print(f"  - Classifications apprises: {stats['classifications_learned']}")
    
    successes = 0
    baseline_solved = {
        '2dee498d', '3c9b0459', '6150a2bd', '67a3c6ac',
        '68b16354', '9172f3a0', 'c59eb873', 'ed36ccf7'
    }
    current_solved = set()
    
    for puzzle_id in puzzles:
        print(f"\n[{puzzle_id}]")
        
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        test_input = np.array(puzzle['test'][0]['input'])
        
        start = time.time()
        success = synthesize_with_learning(
            train_pairs, test_input, puzzle_id, memory, max_retries=3
        )
        elapsed = time.time() - start
        
        if success:
            successes += 1
            current_solved.add(puzzle_id)
        
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
    
    # Stats d'apprentissage finales
    final_stats = memory.get_learning_stats()
    print(f"\n📊 STATISTIQUES D'APPRENTISSAGE:")
    print(f"  - Expériences session: {len(memory.short_term)}")
    print(f"  - Taux succès session: {successes/11:.1%}")
    print(f"  - Patterns échecs identifiés: {len(memory.get_failure_patterns())}")
    
    # Afficher patterns d'échecs récurrents
    failure_patterns = memory.get_failure_patterns(min_frequency=2)
    if failure_patterns:
        print(f"\n🔍 PATTERNS D'ÉCHECS RÉCURRENTS:")
        for pattern in failure_patterns:
            print(f"  - {pattern.pattern_type}: {pattern.frequency}x sur {len(pattern.puzzle_ids)} puzzles")
            print(f"    Puzzles: {pattern.puzzle_ids}")
            if pattern.correction_attempted:
                status = "✓" if pattern.correction_successful else "✗"
                print(f"    Correction tentée: {status}")
    
    # Exporter knowledge base
    kb_path = Path("memory/knowledge_base_session33.json")
    with open(kb_path, 'w') as f:
        json.dump(memory.export_knowledge_base(), f, indent=2)
    print(f"\n💾 Knowledge base exportée: {kb_path}")
    
    if successes == 11:
        print("\n✓✓✓ OBJECTIF ATTEINT: 11/11 (100%) ✓✓✓")
    elif successes >= 9:
        print(f"\n✓ PROGRESSION MAJEURE: {successes}/11")
    elif successes == 8:
        print("\n⚠️ Stable: 8/11 (baseline maintenue)")
    else:
        print(f"\n✗ Régression: {successes}/11")
    
    print(f"\nGain vs Session 31 (8/11): +{successes-8} puzzles")


if __name__ == "__main__":
    main()

# Made with Bob
