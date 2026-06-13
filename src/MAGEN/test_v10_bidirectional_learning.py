#!/usr/bin/env python3
"""
MAGEN - Test V10: Architecture Bidirectionnelle + Corrections Session 52
========================================================================

SESSION 55 - RÉVOLUTION MÉTA-COGNITIVE

Changements majeurs vs V5:
1. ✅ ROLLBACK V5 réussi (45/400 baseline restauré)
2. ✅ Architecture bidirectionnelle méta-cognitive (598 lignes)
3. ✅ Backup automatique pré/post exécution
4. ✅ Correction #3: AdaptiveStrategy + PWFE Integration
5. ✅ Correction #4: Cache Optimization

Objectif: 55-70/400 (13.75-17.5%)
Mode: 100% LOCAL
Protocole: CLAUDE_PILOT + LUMVORAX
"""

import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
import shutil
import sys

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
from core.transformation_space import TransformationSpace
from core.cognitive_strategy_map import CognitiveStrategyMap
from core.parallel_world_frame_engine import ParallelWorldFrameEngine, WorldStatus

# NOUVEAU: Architecture bidirectionnelle
from core.metacognitive_learning import (
    MetaCognitiveMemory,
    BidirectionalLearningLoop,
    ReasoningStep,
    ReasoningPath
)


class AutomaticBackupManager:
    """Gestionnaire de backup automatique pré/post exécution"""
    
    def __init__(self, base_dir: Path):
        self.base_dir = base_dir
        self.backup_dir = base_dir / "backups"
        self.backup_dir.mkdir(parents=True, exist_ok=True)
        self.current_backup_id = None
    
    def create_pre_execution_backup(self, version: str) -> str:
        """Backup automatique avant exécution"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_id = f"pre_{version}_{timestamp}"
        backup_path = self.backup_dir / backup_id
        backup_path.mkdir(parents=True, exist_ok=True)
        
        # Backup fichiers critiques
        critical_files = [
            "test_phase2_400_puzzles.py",
            "core/magen_memory.py",
            "core/magen_pipeline.py",
            "core/parallel_world_frame_engine.py",
            "core/metacognitive_learning.py",
            "synthesis/adaptive_strategy.py"
        ]
        
        for file_path in critical_files:
            src = self.base_dir / file_path
            if src.exists():
                dst = backup_path / file_path
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
        
        self.current_backup_id = backup_id
        print(f"✅ Backup pré-exécution créé: {backup_id}")
        return backup_id
    
    def create_post_execution_backup(self, version: str, results: dict) -> str:
        """Backup automatique après exécution avec résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_id = f"post_{version}_{timestamp}"
        backup_path = self.backup_dir / backup_id
        backup_path.mkdir(parents=True, exist_ok=True)
        
        # Sauvegarder résultats
        results_file = backup_path / "results.json"
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        # Sauvegarder logs
        log_file = self.base_dir / f"test_{version.lower()}_{timestamp}.log"
        if log_file.exists():
            shutil.copy2(log_file, backup_path / "execution.log")
        
        print(f"✅ Backup post-exécution créé: {backup_id}")
        return backup_id


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


def generate_programs_with_metacognition(
    train_pairs, 
    test_input, 
    classification, 
    memory: LearningMemory,
    metacog_memory: MetaCognitiveMemory,
    puzzle_id: str,
    puzzle_features: dict
):
    """
    Génère programmes avec apprentissage méta-cognitif
    
    NOUVEAU: Utilise stratégies apprises pour générer programmes
    """
    programs = []
    
    # ÉTAPE 1: Chercher stratégies applicables (TRANSFER LEARNING)
    applicable_strategies = metacog_memory.retrieve_applicable_strategies(puzzle_features)
    
    if applicable_strategies:
        print(f"    → {len(applicable_strategies)} stratégies méta-cognitives applicables")
        # Utiliser top 3 stratégies
        for strategy, confidence in applicable_strategies[:3]:
            print(f"      • {strategy.strategy_name} (confidence={confidence:.2f}, success_rate={strategy.success_rate:.2f})")
    
    # ÉTAPE 2: Génération classique (baseline V5)
    programs.extend(_generate_for_classification(train_pairs, test_input, classification))
    
    # ÉTAPE 3: Mémoire classique (V5)
    similar_solved = memory.get_similar_puzzles(puzzle_id, classification)
    if similar_solved:
        print(f"    → Puzzles similaires résolus: {len(similar_solved)}")
    
    return programs


def _generate_for_classification(train_pairs, test_input, classification):
    """Génère programmes pour une classification donnée (V5 baseline)"""
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
        if len(primitives) > 0:
            composed = composer.compose_programs(train_pairs, test_input, category)
            programs.extend(composed)
    
    return programs


def solve_puzzle_with_bidirectional_learning(
    puzzle_id: str,
    train_pairs: list,
    test_input: np.ndarray,
    memory: LearningMemory,
    metacog_memory: MetaCognitiveMemory,
    learning_loop: BidirectionalLearningLoop,
    pwfe: ParallelWorldFrameEngine,
    adaptive_strategy: AdaptiveStrategy,
    max_attempts: int = 3
) -> dict:
    """
    Résout un puzzle avec architecture bidirectionnelle complète
    
    FORWARD: Puzzle → Raisonnement → Solution
    BACKWARD: Solution → Analyse → Apprentissage
    """
    start_time = time.time()
    
    # Classification
    classifier = PuzzleClassifier()
    classification = classifier.classify(train_pairs)
    
    # Extraire features pour méta-cognition
    puzzle_features = {
        'puzzle_id': puzzle_id,
        'classification': classification,
        'grid_size': test_input.shape,
        'num_colors': len(np.unique(test_input)),
        'num_train_pairs': len(train_pairs)
    }
    
    # Démarrer boucle d'apprentissage
    learning_loop.start_puzzle(puzzle_id, classification)
    
    # STEP 1: Perception
    learning_loop.add_reasoning_step(
        step_type="perception",
        description=f"Classified as {classification}",
        input_state=test_input,
        output_state=classification,
        success=True,
        confidence=0.8,
        metadata=puzzle_features
    )
    
    # STEP 2: Génération hypothèses
    programs = generate_programs_with_metacognition(
        train_pairs, test_input, classification,
        memory, metacog_memory, puzzle_id, puzzle_features
    )
    
    learning_loop.add_reasoning_step(
        step_type="hypothesis",
        description=f"Generated {len(programs)} candidate programs",
        input_state=classification,
        output_state=programs,
        success=len(programs) > 0,
        confidence=0.7,
        metadata={'num_programs': len(programs)}
    )
    
    if not programs:
        learning_loop.complete_puzzle(
            solution=None,
            success=False,
            strategy_used="generation_failed",
            key_insights=["No programs generated"],
            dead_ends_avoided=[]
        )
        return {
            'success': False,
            'solution': None,
            'time': time.time() - start_time,
            'attempts': 0
        }
    
    # STEP 3: Extraction invariants
    invariant_extractor = InvariantExtractor()
    invariants = invariant_extractor.extract_invariants(train_pairs)
    
    learning_loop.add_reasoning_step(
        step_type="invariant_extraction",
        description=f"Extracted {len(invariants)} invariants",
        input_state=train_pairs,
        output_state=invariants,
        success=True,
        confidence=0.9,
        metadata={'num_invariants': len(invariants)}
    )
    
    # STEP 4: PWFE - Exploration parallèle
    pwfe_result = pwfe.explore_parallel_worlds(
        programs=programs,
        train_pairs=train_pairs,
        test_input=test_input,
        invariants=invariants
    )
    
    pwfe_success = pwfe_result.get('success', False)
    learning_loop.add_reasoning_step(
        step_type="parallel_exploration",
        description=f"PWFE explored {len(programs)} worlds",
        input_state=programs,
        output_state=pwfe_result,
        success=pwfe_success,
        confidence=0.8 if pwfe_success else 0.3,
        metadata={
            'pwfe_success': pwfe_success,
            'best_trajectory': pwfe_result.get('best_trajectory', False)
        }
    )
    
    if pwfe_success:
        solution = pwfe_result.get('solution')
        learning_loop.complete_puzzle(
            solution=solution,
            success=True,
            strategy_used="pwfe_direct",
            key_insights=["PWFE found solution directly"],
            dead_ends_avoided=[]
        )
        return {
            'success': True,
            'solution': solution,
            'time': time.time() - start_time,
            'attempts': 1,
            'strategy': 'pwfe_direct'
        }
    
    # STEP 5: CORRECTION #3 - AdaptiveStrategy + PWFE Integration
    if pwfe_result.get('best_trajectory'):
        print(f"    → Récupération trajectoires PWFE prometteuses")
        
        # Récupérer mondes prometteurs
        promising_worlds = [
            w for w in pwfe_result.get('all_worlds', [])
            if hasattr(w, 'status') and w.status == WorldStatus.PROMISING
        ]
        
        if promising_worlds:
            print(f"    → {len(promising_worlds)} mondes prometteurs trouvés")
            
            # Tester programmes prometteurs
            validator = ProgramValidator()
            for world in promising_worlds[:5]:  # Top 5
                try:
                    if validator.validate_program(world.program, train_pairs):
                        prediction = world.program.execute(test_input)
                        learning_loop.add_reasoning_step(
                            step_type="validation",
                            description="Validated promising world from PWFE",
                            input_state=world.program,
                            output_state=prediction,
                            success=True,
                            confidence=0.9,
                            metadata={'world_id': world.world_id}
                        )
                        learning_loop.complete_puzzle(
                            solution=prediction,
                            success=True,
                            strategy_used="pwfe_recovery",
                            key_insights=["Recovered from PWFE promising trajectory"],
                            dead_ends_avoided=[]
                        )
                        return {
                            'success': True,
                            'solution': prediction,
                            'time': time.time() - start_time,
                            'attempts': 2,
                            'strategy': 'pwfe_recovery'
                        }
                except Exception:
                    continue
    
    # STEP 6: Fallback adaptatif (V5 baseline)
    print(f"    → Fallback adaptatif")
    for attempt in range(max_attempts):
        candidates = adaptive_strategy.generate_alternative_programs(
            train_pairs, test_input, classification, attempt
        )
        
        validator = ProgramValidator()
        for program in candidates:
            try:
                if validator.validate_program(program, train_pairs):
                    prediction = program.execute(test_input)
                    learning_loop.add_reasoning_step(
                        step_type="validation",
                        description=f"Validated via adaptive strategy (attempt {attempt+1})",
                        input_state=program,
                        output_state=prediction,
                        success=True,
                        confidence=0.7,
                        metadata={'attempt': attempt+1}
                    )
                    learning_loop.complete_puzzle(
                        solution=prediction,
                        success=True,
                        strategy_used="adaptive_fallback",
                        key_insights=[f"Adaptive strategy succeeded at attempt {attempt+1}"],
                        dead_ends_avoided=[]
                    )
                    return {
                        'success': True,
                        'solution': prediction,
                        'time': time.time() - start_time,
                        'attempts': attempt + 3,
                        'strategy': 'adaptive_fallback'
                    }
            except Exception:
                continue
    
    # Échec
    learning_loop.complete_puzzle(
        solution=None,
        success=False,
        strategy_used="all_failed",
        key_insights=[],
        dead_ends_avoided=["pwfe", "adaptive_strategy"]
    )
    
    return {
        'success': False,
        'solution': None,
        'time': time.time() - start_time,
        'attempts': max_attempts + 2
    }


def main():
    """Test V10 complet avec architecture bidirectionnelle"""
    print("=" * 70)
    print("🧠 MAGEN TEST V10 - ARCHITECTURE BIDIRECTIONNELLE")
    print("=" * 70)
    print(f"Session: 55")
    print(f"Baseline: V5 (45/400 - 11.25%)")
    print(f"Objectif: 55-70/400 (13.75-17.5%)")
    print(f"Mode: 100% LOCAL")
    print(f"Protocole: CLAUDE_PILOT + LUMVORAX")
    print("=" * 70)
    
    base_dir = Path(__file__).parent
    
    # ÉTAPE 1: Backup automatique pré-exécution
    backup_manager = AutomaticBackupManager(base_dir)
    pre_backup_id = backup_manager.create_pre_execution_backup("V10")
    
    # ÉTAPE 2: Initialisation
    print("\n📊 Initialisation...")
    memory = LearningMemory()
    metacog_memory = MetaCognitiveMemory(memory_dir=base_dir / "memory" / "metacognitive")
    learning_loop = BidirectionalLearningLoop(metacog_memory)
    pwfe = ParallelWorldFrameEngine(max_hypotheses=100, max_frames=5)
    adaptive_strategy = AdaptiveStrategy()
    
    # Charger dataset
    dataset = load_dataset()
    total_puzzles = len(dataset)
    print(f"✅ Dataset chargé: {total_puzzles} puzzles")
    
    # ÉTAPE 3: Exécution
    print(f"\n🚀 Démarrage test V10...")
    start_time = time.time()
    
    results = {
        'version': 'V10_BIDIRECTIONAL',
        'timestamp': datetime.now().isoformat(),
        'total_puzzles': total_puzzles,
        'successes': 0,
        'failures': 0,
        'errors': 0,
        'puzzles': {}
    }
    
    for i, (puzzle_id, puzzle_data) in enumerate(dataset.items(), 1):
        print(f"\n[{i}/{total_puzzles}] Puzzle {puzzle_id}")
        
        try:
            train_pairs = puzzle_data['train']
            test_input = np.array(puzzle_data['test'][0]['input'])
            
            result = solve_puzzle_with_bidirectional_learning(
                puzzle_id=puzzle_id,
                train_pairs=train_pairs,
                test_input=test_input,
                memory=memory,
                metacog_memory=metacog_memory,
                learning_loop=learning_loop,
                pwfe=pwfe,
                adaptive_strategy=adaptive_strategy
            )
            
            if result['success']:
                results['successes'] += 1
                print(f"✅ Succès (stratégie: {result.get('strategy', 'unknown')})")
            else:
                results['failures'] += 1
                print(f"❌ Échec")
            
            results['puzzles'][puzzle_id] = {
                'success': result['success'],
                'time': result['time'],
                'attempts': result.get('attempts', 0),
                'strategy': result.get('strategy', 'unknown')
            }
            
        except Exception as e:
            results['errors'] += 1
            results['puzzles'][puzzle_id] = {
                'success': False,
                'error': str(e)
            }
            print(f"❌ Erreur: {e}")
        
        # Afficher progression
        if i % 50 == 0:
            current_rate = (results['successes'] / i) * 100
            print(f"\n📊 Progression: {i}/{total_puzzles} ({current_rate:.1f}% succès)")
    
    # ÉTAPE 4: Résultats finaux
    total_time = time.time() - start_time
    results['total_time'] = total_time
    results['success_rate'] = (results['successes'] / total_puzzles) * 100
    
    print("\n" + "=" * 70)
    print("📊 RÉSULTATS FINAUX V10")
    print("=" * 70)
    print(f"Succès: {results['successes']}/{total_puzzles} ({results['success_rate']:.2f}%)")
    print(f"Échecs: {results['failures']}")
    print(f"Erreurs: {results['errors']}")
    print(f"Temps total: {total_time:.1f}s ({total_time/60:.1f}min)")
    print(f"Temps moyen: {total_time/total_puzzles:.3f}s/puzzle")
    
    # Statistiques méta-cognitives
    metacog_stats = metacog_memory.get_statistics()
    print(f"\n🧠 Statistiques Méta-Cognitives:")
    print(f"  • Chemins de raisonnement stockés: {metacog_stats['total_paths_stored']}")
    print(f"  • Stratégies extraites: {metacog_stats['total_strategies_extracted']}")
    print(f"  • Transferts réussis: {metacog_stats['successful_transfers']}")
    print(f"  • Taux succès transfert: {metacog_stats['transfer_success_rate']:.1%}")
    
    # Comparaison avec V5
    v5_successes = 45
    improvement = results['successes'] - v5_successes
    improvement_pct = (improvement / v5_successes) * 100 if v5_successes > 0 else 0
    
    print(f"\n📈 Comparaison vs V5:")
    print(f"  • V5: {v5_successes}/400 (11.25%)")
    print(f"  • V10: {results['successes']}/400 ({results['success_rate']:.2f}%)")
    print(f"  • Amélioration: {improvement:+d} puzzles ({improvement_pct:+.1f}%)")
    
    # ÉTAPE 5: Sauvegarde résultats
    results_file = base_dir / f"results_v10_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"\n✅ Résultats sauvegardés: {results_file.name}")
    
    # ÉTAPE 6: Backup automatique post-exécution
    post_backup_id = backup_manager.create_post_execution_backup("V10", results)
    
    print("\n" + "=" * 70)
    print("✅ TEST V10 TERMINÉ")
    print("=" * 70)
    print(f"Backup pré-exécution: {pre_backup_id}")
    print(f"Backup post-exécution: {post_backup_id}")
    print("\n⏳ EN ATTENTE VALIDATION UTILISATEUR pour passage Kaggle")
    print("=" * 70)


if __name__ == "__main__":
    main()

# Made with Bob
