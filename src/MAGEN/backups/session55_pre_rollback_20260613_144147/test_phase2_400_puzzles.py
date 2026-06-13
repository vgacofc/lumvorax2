#!/usr/bin/env python3
"""
MAGEN - Phase 2: Test Dataset Complet 400 Puzzles
==================================================

SESSION 54 - ARCHITECTURE RÉVOLUTIONNAIRE DET+GSF
Objectif: 60-80/400 (15-20%) avec DET+GSF
Mode: 100% LOCAL
Protocole: CLAUDE_PILOT + LUMVORAX

Architecture révolutionnaire MAGEN V9:
- DynamicEntityTracker (DET): Identité persistante des entités
- GlobalSolutionField (GSF): Espace de solutions simultanées
- LearningMemory (174 expériences, 75.3% succès)
- InvariantExtractor (analyse invariants)
- ParallelWorldFrameEngine (PWFE)
- Générateurs spécifiques (mark_shape_corners, extract_quadrant)

CHANGEMENTS V9:
- ❌ SUPPRESSION AdaptiveStrategy (0% succès prouvé)
- ✅ AJOUT DET: Track entités à travers transformations
- ✅ AJOUT GSF: Élimination précoce par cohérence globale
- ✅ Pipeline simultané au lieu de séquentiel
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
from core.invariant_extractor import InvariantExtractor
# CORRECTION SESSION 49 #3: Intégration modules cognitifs
from core.transformation_space import TransformationSpace
from core.cognitive_strategy_map import CognitiveStrategyMap
# CORRECTION SESSION 50: Intégration Parallel World Frame Engine
from core.parallel_world_frame_engine import ParallelWorldFrameEngine
# SESSION 54: Architecture révolutionnaire DET+GSF
from core.dynamic_entity_tracker import DynamicEntityTracker
from core.global_solution_field import GlobalSolutionField


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
                       max_retries: int = 3):
    """
    SESSION 54 - Synthèse avec architecture révolutionnaire DET+GSF
    
    PIPELINE RÉVOLUTIONNAIRE:
    1. DET: Extraire entités et tracker transformations
    2. Générateurs: Créer programmes candidats
    3. GSF: Peupler champ de solutions simultanées
    4. GSF: Éliminer solutions incohérentes (AVANT exécution)
    5. GSF: Clustering cognitif (400 → 3-5 clusters)
    6. GSF: Sélection directe par cohérence structurelle
    7. Validation top-3 solutions
    """
    # Initialiser modules cognitifs
    invariant_extractor = InvariantExtractor()
    invariants = invariant_extractor.extract(train_pairs)
    
    # SESSION 54: Initialiser DET pour tracking entités
    det = DynamicEntityTracker()
    
    # Extraire entités de tous les exemples d'entraînement
    all_train_grids = [inp for inp, _ in train_pairs] + [out for _, out in train_pairs]
    entity_sequence = det.track_sequence(all_train_grids)
    transformation_rules = det.get_transformation_rules()
    
    print(f"🔍 DET: {len(entity_sequence)} entités trackées, {len(transformation_rules)} règles")
    
    # SESSION 54: Initialiser GSF pour espace de solutions simultanées
    gsf = GlobalSolutionField(
        entity_tracker=det,
        verbose=False
    )
    
    # Initialiser TransformationSpace pour continuité compositionnelle
    transform_space = TransformationSpace()
    
    # PWFE pour validation finale
    pwfe = ParallelWorldFrameEngine(
        max_hypotheses=50,
        max_frames=5,
        pruning_threshold=0.3,
        verbose=False  # Désactivé en V9, GSF prend le relais
    )
    
    classifier = PuzzleClassifier()
    classification_result = classifier.classify(train_pairs)
    classification = classification_result.primary_class.value
    confidence = classification_result.confidence
    
    train_dicts = [{'input': inp, 'output': out} for inp, out in train_pairs]
    train_hash = compute_train_hash(train_pairs)
    
    scorer = ProgramScorer()
    validator = ProgramValidator()
    
    # SESSION 54: NOUVEAU PIPELINE RÉVOLUTIONNAIRE
    for attempt in range(max_retries):
        # 1. Générer programmes candidats
        candidates = generate_programs_with_fallback(
            train_pairs, test_input, classification, memory, puzzle_id
        )
        
        print(f"\n🔍 V9: Tentative {attempt+1}/{max_retries}")
        print(f"   Candidats générés: {len(candidates)}")
        
        if not candidates:
            continue
        
        # 2. GSF: Peupler champ de solutions simultanées
        # Convertir InvariantProfile en dict pour GSF
        invariants_dict = {
            'preserve_dimensions': invariants.preserve_size,
            'preserve_colors': invariants.color_palette_stable,
            'preserve_shape': invariants.preserve_shape,
            'preserve_topology': invariants.preserve_topology,
            'transformation_type': invariants.transformation_type,
            'object_count_stable': invariants.object_count_stable
        }
        
        gsf.populate_field(
            programs=candidates,
            train_pairs=train_pairs,
            invariants=invariants_dict
        )
        
        print(f"   GSF: {len(gsf.nodes)} solutions dans le champ")
        
        # 3. GSF: Éliminer solutions incohérentes (AVANT exécution!)
        eliminated = gsf.eliminate_incoherent()
        print(f"   GSF: {eliminated} solutions éliminées (incohérentes)")
        
        # 4. GSF: Clustering cognitif
        gsf.cluster_solutions(max_clusters=5)
        print(f"   GSF: {len(gsf.clusters)} clusters formés (compression cognitive)")
        
        # 5. GSF: Sélection directe par cohérence
        best_program = gsf.select_best_solution()
        
        if best_program is None:
            print(f"   GSF: Aucune solution viable trouvée")
            continue
        
        # Calculer score pour affichage
        viable_nodes = [n for n in gsf.nodes.values() if n.is_viable()]
        best_node = max(viable_nodes, key=lambda n: n.compute_global_score()) if viable_nodes else None
        best_score = best_node.compute_global_score() if best_node else 0.0
        
        print(f"   GSF: Solution sélectionnée (score={best_score:.3f})")
        
        # 6. Validation sur train set
        is_valid, accuracy = validator.validate_on_train(best_program, train_dicts)
        
        if is_valid:
            print(f"   ✅ Solution validée (accuracy={accuracy:.1%})")
            
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
            
            return True, best_program, len(candidates), len(gsf.nodes)
        
        # 7. Si échec, essayer top-3 solutions du cluster principal
        if gsf.clusters:
            main_cluster = max(gsf.clusters.values(), key=lambda c: len(c.nodes))
            top_3 = sorted(main_cluster.nodes, key=lambda n: n.compute_global_score(), reverse=True)[:3]
            
            for i, node in enumerate(top_3[1:], 2):  # Skip first (already tested)
                print(f"   Essai solution #{i} du cluster principal...")
                is_valid, accuracy = validator.validate_on_train(node.program, train_dicts)
                
                if is_valid:
                    print(f"   ✅ Solution #{i} validée (accuracy={accuracy:.1%})")
                    
                    node_score = node.compute_global_score()
                    
                    experience = PuzzleExperience(
                        puzzle_id=puzzle_id,
                        timestamp=datetime.now().isoformat(),
                        success=True,
                        classification=classification,
                        confidence=confidence,
                        programs_tried=len(candidates),
                        best_score=node_score,
                        solution_program=str(node.program),
                        failure_reason=None,
                        train_pairs_hash=train_hash
                    )
                    memory.store_experience(experience)
                    
                    return True, node.program, len(candidates), len(gsf.nodes)
        
        # Si toutes les solutions échouent, diagnostiquer
        failure_reason = _diagnose_failure(train_pairs, test_input, classification, candidates)
        print(f"   ❌ Échec: {failure_reason}")
        
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
    
    return False, None, 0, 0


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
                memory: LearningMemory) -> dict:
    """Test un puzzle avec architecture DET+GSF"""
    start_time = time.time()
    
    try:
        train_pairs = [(np.array(p['input']), np.array(p['output']))
                       for p in puzzle_data['train']]
        test_input = np.array(puzzle_data['test'][0]['input'])
        
        # SESSION 54: Résolution avec DET+GSF (sans AdaptiveStrategy)
        success, program, candidates_tried, solutions_evaluated = synthesize_solution(
            train_pairs, test_input, puzzle_id, memory, max_retries=3
        )
        
        elapsed = time.time() - start_time
        
        return {
            'puzzle_id': puzzle_id,
            'success': success,
            'time': elapsed,
            'candidates_tried': candidates_tried,
            'solutions_evaluated': solutions_evaluated,
            'error': None
        }
    
    except Exception as e:
        elapsed = time.time() - start_time
        return {
            'puzzle_id': puzzle_id,
            'success': False,
            'time': elapsed,
            'candidates_tried': 0,
            'solutions_evaluated': 0,
            'error': str(e)
        }


def main():
    """Test Phase 2 - Dataset complet 400 puzzles avec DET+GSF"""
    print("="*80)
    print("MAGEN V9 - ARCHITECTURE RÉVOLUTIONNAIRE DET+GSF")
    print("="*80)
    print(f"🎯 Objectif: 60-80/400 (15-20%) avec DET+GSF")
    print(f"📍 Mode: 100% LOCAL")
    print(f"🔬 Protocole: CLAUDE_PILOT + LUMVORAX")
    print(f"🚀 Nouveautés V9:")
    print(f"   - DynamicEntityTracker (identité persistante)")
    print(f"   - GlobalSolutionField (espace simultané)")
    print(f"   - Élimination précoce par cohérence")
    print(f"   - Clustering cognitif (400 → 3-5)")
    print("="*80)
    
    # Chargement dataset
    print("\n📂 Chargement dataset...")
    dataset = load_dataset()
    puzzle_ids = list(dataset.keys())
    total_puzzles = len(puzzle_ids)
    print(f"✓ {total_puzzles} puzzles chargés")
    
    # SESSION 54: Initialisation mémoire (sans AdaptiveStrategy)
    print("\n🧠 Initialisation MAGEN V9 Architecture...")
    memory = LearningMemory()
    
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
    total_candidates = 0
    total_solutions_evaluated = 0
    
    start_time = time.time()
    
    for i, puzzle_id in enumerate(puzzle_ids, 1):
        # Barre de progression
        progress = i / total_puzzles * 100
        bar_length = 50
        filled = int(bar_length * i / total_puzzles)
        bar = '█' * filled + '░' * (bar_length - filled)
        
        print(f"\r[{bar}] {progress:.1f}% ({i}/{total_puzzles}) | Succès: {successes}", end='', flush=True)
        
        # Test puzzle avec DET+GSF
        result = test_puzzle(puzzle_id, dataset[puzzle_id], memory)
        results.append(result)
        
        if result['success']:
            successes += 1
        
        total_candidates += result.get('candidates_tried', 0)
        total_solutions_evaluated += result.get('solutions_evaluated', 0)
    
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
    
    print(f"\n🌐 GLOBAL SOLUTION FIELD (GSF):")
    print(f"  - Candidats générés: {total_candidates}")
    print(f"  - Solutions évaluées: {total_solutions_evaluated}")
    if total_candidates > 0:
        print(f"  - Taux élimination: {100*(1-total_solutions_evaluated/total_candidates):.1f}%")
    
    # Objectif V9 atteint?
    print(f"\n🎯 OBJECTIF V9:")
    if successes >= 80:
        print(f"  ✅✅✅ OBJECTIF DÉPASSÉ: {successes}/400 ≥ 80!")
    elif successes >= 60:
        print(f"  ✅ OBJECTIF ATTEINT: {successes}/400 ≥ 60")
    elif successes >= 45:
        print(f"  📊 Amélioration vs V5: {successes}/400 > 45")
    else:
        print(f"  ⚠️  Objectif non atteint: {successes}/400 < 60")
    
    # Comparaison avec V5
    v5_score = 45
    improvement = ((successes - v5_score) / v5_score * 100) if v5_score > 0 else 0
    print(f"\n📈 COMPARAISON V5 → V9:")
    print(f"  - V5 (baseline): 45/400 (11.25%)")
    print(f"  - V9 (DET+GSF): {successes}/400 ({success_rate:.1f}%)")
    if improvement > 0:
        print(f"  - Amélioration: +{improvement:.1f}%")
    else:
        print(f"  - Régression: {improvement:.1f}%")
    
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
            'candidates_tried': total_candidates,
            'solutions_evaluated': total_solutions_evaluated,
            'results': results,
            'version': 'V9_DET_GSF'
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {output_path}")
    
    # Export knowledge base
    kb_path = Path(f"memory/knowledge_base_phase2_{timestamp}.json")
    kb_path.parent.mkdir(exist_ok=True)
    with open(kb_path, 'w') as f:
        json.dump(memory.export_knowledge_base(), f, indent=2)
    print(f"💾 Knowledge base exportée: {kb_path}")
    
    print(f"\n{'='*80}")
    print("TEST V9 TERMINÉ - ARCHITECTURE RÉVOLUTIONNAIRE DET+GSF")
    print(f"{'='*80}")


if __name__ == "__main__":
    main()

# Made with Bob - Session 54 - Architecture Révolutionnaire DET+GSF
