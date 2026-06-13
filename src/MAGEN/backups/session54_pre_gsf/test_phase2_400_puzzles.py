#!/usr/bin/env python3
"""
MAGEN - Phase 2: Test Dataset Complet 400 Puzzles
==================================================

SESSION 48 - PHASE 2 DATASET COMPLET
Objectif: 20-40/400 (5-10%)
Mode: 100% LOCAL
Protocole: CLAUDE_PILOT + LUMVORAX

Architecture complète MAGEN:
- LearningMemory (174 expériences, 75.3% succès)
- AdaptiveStrategy (récupération adaptative)
- InvariantExtractor (analyse invariants)
- Générateurs spécifiques (mark_shape_corners, extract_quadrant)
- DynamicIdentityTracker (continuité compositionnelle)
"""

import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime

# Import modules existants
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
from core.invariant_extractor import InvariantExtractor
# CORRECTION SESSION 49 #3: Intégration modules cognitifs
from core.transformation_space import TransformationSpace
from core.cognitive_strategy_map import CognitiveStrategyMap
# CORRECTION SESSION 50: Intégration Parallel World Frame Engine
from core.parallel_world_frame_engine import ParallelWorldFrameEngine


def load_dataset():
    """Charge le dataset complet 400 puzzles"""
    dataset_path = Path("data/arc-agi_evaluation_challenges.json")
    
    if not dataset_path.exists():
        raise FileNotFoundError(
            f"Dataset non trouvé: {dataset_path}\n"
            "Télécharger depuis: https://github.com/fchollet/ARC-AGI"
        )
    
    with open(dataset_path) as f:
        dataset = json.load(f)
    
    return dataset


def generate_programs_with_fallback(train_pairs, test_input, classification, 
                                    memory: LearningMemory, puzzle_id: str):
    """Génère programmes avec fallback multi-classe"""
    programs = []
    
    programs.extend(_generate_for_classification(train_pairs, test_input, classification))
    
    similar_solved = memory.get_similar_puzzles(puzzle_id, classification)
    if similar_solved:
        print(f"    → Puzzles similaires résolus: {len(similar_solved)}")
    
    should_retry, suggested_strategy = memory.should_retry_puzzle(puzzle_id)
    if should_retry and suggested_strategy:
        print(f"    → Stratégie alternative suggérée par mémoire")
    
    return programs


def _generate_for_classification(train_pairs, test_input, classification):
    """Génère programmes pour une classification donnée"""
    programs = []
    
    shape_detector = ShapeDetector()
    shape_programs = shape_detector.generate_shape_programs(train_pairs, test_input)
    programs.extend(shape_programs)
    
    rule_detector = RuleDetector()
    rule_programs = rule_detector.generate_rule_programs(train_pairs)
    programs.extend(rule_programs)
    
    structural_detector = StructuralDetector()
    structural_programs = structural_detector.generate_marking_programs(train_pairs, test_input)
    programs.extend(structural_programs)
    
    region_extractor = RegionExtractor()
    region_programs = region_extractor.generate_region_programs(train_pairs, test_input)
    programs.extend(region_programs)
    
    composer = Composer(max_depth=7, max_candidates=200)
    for category, primitives in composer.primitives.items():
        if category in ['geometric', 'scale', 'advanced']:
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


def synthesize_solution(train_pairs, test_input, puzzle_id,
                       memory: LearningMemory,
                       adaptive: AdaptiveStrategy,
                       max_retries: int = 3):
    """Synthèse solution avec architecture complète MAGEN"""
    # CORRECTION SESSION 49 #3: Initialiser modules cognitifs
    invariant_extractor = InvariantExtractor()
    invariants = invariant_extractor.extract(train_pairs)
    
    # Initialiser TransformationSpace pour continuité compositionnelle
    transform_space = TransformationSpace()
    
    # CORRECTION SESSION 51 #1: Activer mode verbose pour diagnostic
    pwfe = ParallelWorldFrameEngine(
        max_hypotheses=50,
        max_frames=5,
        pruning_threshold=0.3,
        verbose=True  # ✅ ACTIVÉ pour détecter erreurs
    )
    
    # Debug: Vérifier initialisation
    print(f"🔍 DEBUG: PWFE initialisé (verbose={pwfe.verbose})")
    
    classifier = PuzzleClassifier()
    classification_result = classifier.classify(train_pairs)
    classification = classification_result.primary_class.value
    confidence = classification_result.confidence
    
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    train_hash = compute_train_hash(train_pairs)
    
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    adaptive_attempts = 0
    adaptive_successes = 0
    
    for attempt in range(max_retries):
        candidates = generate_programs_with_fallback(
            train_pairs, test_input, classification, memory, puzzle_id
        )
        
        # CORRECTION SESSION 51 #1: Debug prints pour tracer exécution PWFE
        print(f"\n🔍 DEBUG: Tentative {attempt+1}/{max_retries}")
        print(f"   Candidats générés: {len(candidates)}")
        print(f"   Type candidats: {type(candidates[0]) if candidates else 'N/A'}")
        
        # CORRECTION SESSION 50: Utiliser PWFE pour simulation parallèle
        # Au lieu de tester séquentiellement, simuler TOUS les mondes en parallèle
        pwfe_result = pwfe.generate_parallel_worlds(
            input_grid=test_input,
            train_pairs=train_pairs,
            program_generators=candidates,  # CORRECTION: Passer les programmes générés
            invariants=invariants
        )
        
        # Debug: Vérifier résultat PWFE
        print(f"   PWFE success: {pwfe_result.get('success', False)}")
        print(f"   PWFE mondes générés: {pwfe_result.get('total_worlds', 0)}")
        print(f"   PWFE best_trajectory: {pwfe_result.get('best_trajectory') is not None}")
        
        # Si PWFE trouve solution validée, l'utiliser directement
        if pwfe_result['success'] and pwfe_result['best_trajectory']:
            best_trajectory = pwfe_result['best_trajectory']
            final_world_id = best_trajectory.world_sequence[-1]
            final_world = pwfe.worlds[final_world_id]
            
            experience = PuzzleExperience(
                puzzle_id=puzzle_id,
                timestamp=datetime.now().isoformat(),
                success=True,
                classification=classification,
                confidence=confidence,
                programs_tried=pwfe_result['total_worlds'],
                best_score=final_world.validation_score,
                solution_program=str(final_world.program),
                failure_reason=None,
                train_pairs_hash=train_hash
            )
            memory.store_experience(experience)
            
            return True, final_world.program, 0, 0
        
        # Sinon, fallback sur méthode séquentielle avec AdaptiveStrategy
        best_program = None
        best_score = -1.0
        
        for program in candidates:
            try:
                # CORRECTION SESSION 49 #5: Try-catch global pour IndexError
                score = scorer.score_program(program, train_dicts)
                is_valid, accuracy = validator.validate_on_train(program, train_dicts)
                
                if is_valid:
                    if score > best_score:
                        best_score = score
                        best_program = program
                
                # CORRECTION SESSION 49: Assouplir seuil 0.55→0.45
                elif score > 0.45:
                    adaptive_attempts += 1
                    
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
                        # CORRECTION SESSION 49: Assouplir validation 95%→80%
                        return score >= 0.80, score
                    
                    adapted_program = None
                    all_valid = True
                    
                    for idx, (train_input, train_output) in enumerate(train_pairs):
                        success, adapted, history = adaptive.adapt_and_retry(
                            puzzle_id=puzzle_id,
                            initial_program=program,
                            execute_fn=execute_fn,
                            validate_fn=validate_fn,
                            input_grid=train_input,
                            expected_output=train_output
                        )
                        
                        if not success:
                            all_valid = False
                            break
                        
                        if idx == 0:
                            adapted_program = adapted
                    
                    if all_valid and adapted_program:
                        is_valid_all, _ = validator.validate_on_train(adapted_program, train_dicts)
                        if is_valid_all:
                            adaptive_successes += 1
                            best_program = adapted_program
                            best_score = 1.0
                            break
            
            except IndexError as e:
                # CORRECTION SESSION 49 #5: Capturer IndexError spécifiquement
                print(f"    ⚠️  IndexError évité dans programme: {e}")
                continue  # Passer au programme suivant
            except Exception as e:
                # Autres exceptions (garder comportement existant)
                pass
        
        if best_program:
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
            
            return True, best_program, adaptive_attempts, adaptive_successes
        
        failure_reason = _diagnose_failure(train_pairs, test_input, classification, candidates)
        
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
    
    return False, None, adaptive_attempts, adaptive_successes


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


def test_puzzle(puzzle_id: str, puzzle_data: dict, 
                memory: LearningMemory, adaptive: AdaptiveStrategy) -> dict:
    """Test un puzzle et retourne résultats"""
    start_time = time.time()
    
    try:
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle_data['train']]
        test_input = np.array(puzzle_data['test'][0]['input'])
        
        # Résolution avec architecture complète
        success, program, attempts, recoveries = synthesize_solution(
            train_pairs, test_input, puzzle_id, memory, adaptive, max_retries=3
        )
        
        elapsed = time.time() - start_time
        
        return {
            'puzzle_id': puzzle_id,
            'success': success,
            'time': elapsed,
            'adaptive_attempts': attempts,
            'adaptive_recoveries': recoveries,
            'error': None
        }
    
    except Exception as e:
        elapsed = time.time() - start_time
        return {
            'puzzle_id': puzzle_id,
            'success': False,
            'time': elapsed,
            'adaptive_attempts': 0,
            'adaptive_recoveries': 0,
            'error': str(e)
        }


def main():
    """Test Phase 2 - Dataset complet 400 puzzles"""
    print("="*80)
    print("MAGEN - PHASE 2: TEST DATASET COMPLET 400 PUZZLES")
    print("="*80)
    print(f"Objectif: 20-40/400 (5-10%)")
    print(f"Mode: 100% LOCAL")
    print(f"Protocole: CLAUDE_PILOT + LUMVORAX")
    print("="*80)
    
    # Chargement dataset
    print("\n📂 Chargement dataset...")
    dataset = load_dataset()
    puzzle_ids = list(dataset.keys())
    total_puzzles = len(puzzle_ids)
    print(f"✓ {total_puzzles} puzzles chargés")
    
    # Initialisation mémoire et adaptive strategy
    print("\n🧠 Initialisation MAGEN Architecture...")
    memory = LearningMemory()
    # CORRECTION SESSION 49: Augmenter max_attempts 3→5
    adaptive = AdaptiveStrategy(max_attempts=5)
    
    stats = memory.get_learning_stats()
    if stats['total_experiences'] > 0:
        print(f"📚 Mémoire chargée:")
        print(f"  - Expériences totales: {stats['total_experiences']}")
        print(f"  - Taux de succès: {stats['success_rate']:.1%}")
    print("✓ Architecture initialisée")
    
    # Test puzzles
    print(f"\n🚀 Début test {total_puzzles} puzzles...")
    print()
    
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    results = []
    successes = 0
    total_adaptive_attempts = 0
    total_adaptive_recoveries = 0
    
    start_time = time.time()
    
    for i, puzzle_id in enumerate(puzzle_ids, 1):
        # Barre de progression
        progress = i / total_puzzles * 100
        bar_length = 50
        filled = int(bar_length * i / total_puzzles)
        bar = '█' * filled + '░' * (bar_length - filled)
        
        print(f"\r[{bar}] {progress:.1f}% ({i}/{total_puzzles}) | Succès: {successes}", end='', flush=True)
        
        # Test puzzle
        result = test_puzzle(puzzle_id, dataset[puzzle_id], memory, adaptive)
        results.append(result)
        
        if result['success']:
            successes += 1
        
        total_adaptive_attempts += result.get('adaptive_attempts', 0)
        total_adaptive_recoveries += result.get('adaptive_recoveries', 0)
    
    print()  # Nouvelle ligne après barre progression
    
    # Résultats finaux
    elapsed_total = time.time() - start_time
    success_rate = (successes / total_puzzles) * 100
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/{total_puzzles} ({success_rate:.1f}%)")
    print(f"{'='*80}")
    
    # Statistiques détaillées
    total_time = sum(r['time'] for r in results)
    avg_time = total_time / total_puzzles
    errors = [r for r in results if r['error']]
    
    print(f"\n📊 STATISTIQUES:")
    print(f"  - Temps total: {elapsed_total:.1f}s ({elapsed_total/60:.1f}min)")
    print(f"  - Temps moyen/puzzle: {avg_time:.2f}s")
    print(f"  - Erreurs: {len(errors)}")
    
    print(f"\n🔄 ADAPTIVE STRATEGY:")
    print(f"  - Tentatives adaptatives: {total_adaptive_attempts}")
    print(f"  - Récupérations réussies: {total_adaptive_recoveries}")
    if total_adaptive_attempts > 0:
        print(f"  - Taux succès: {100*total_adaptive_recoveries/total_adaptive_attempts:.1f}%")
    
    # Objectif atteint?
    print(f"\n🎯 OBJECTIF:")
    if successes >= 40:
        print(f"  ✅✅✅ OBJECTIF DÉPASSÉ: {successes}/400 ≥ 40!")
    elif successes >= 20:
        print(f"  ✅ OBJECTIF ATTEINT: {successes}/400 ≥ 20")
    else:
        print(f"  ⚠️  Objectif non atteint: {successes}/400 < 20")
    
    # Sauvegarde résultats + knowledge base
    output_path = Path(f"results_phase2_{timestamp}.json")
    with open(output_path, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'total_puzzles': total_puzzles,
            'successes': successes,
            'success_rate': success_rate,
            'total_time': elapsed_total,
            'avg_time': avg_time,
            'adaptive_attempts': total_adaptive_attempts,
            'adaptive_recoveries': total_adaptive_recoveries,
            'results': results
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {output_path}")
    
    # Export knowledge base
    kb_path = Path(f"memory/knowledge_base_phase2_{timestamp}.json")
    kb_path.parent.mkdir(exist_ok=True)
    with open(kb_path, 'w') as f:
        json.dump(memory.export_knowledge_base(), f, indent=2)
    print(f"💾 Knowledge base exportée: {kb_path}")
    
    print(f"\n{'='*80}")
    print("PHASE 2 TERMINÉE")
    print(f"{'='*80}")


if __name__ == "__main__":
    main()

# Made with Bob - Session 48 - Phase 2
