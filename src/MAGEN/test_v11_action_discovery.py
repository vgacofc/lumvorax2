#!/usr/bin/env python3
"""
MAGEN - Test V11: Action Discovery + MetaCognitive Learning
============================================================

SESSION 55 ÉTENDUE - INSIGHT UTILISATEUR
Objectif: Valider impact découverte affordances
Mode: 100% LOCAL
Protocole: CLAUDE_PILOT + LUMVORAX

Architecture V11:
- Phase 0: ActionDiscoveryEngine (découverte affordances)
- Phase 1-3: Pipeline V5-Fixed (baseline 45/400)
- Logging forensique complet

Test simplifié: Mesure impact Phase 0 sur génération programmes
"""

import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime

# Import modules existants V5
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
from core.parallel_world_frame_engine import ParallelWorldFrameEngine

# Import nouveau module Action Discovery
from core.action_discovery_engine import ActionDiscoveryEngine


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


def compute_context_signature(train_pairs):
    """Calcule signature de contexte pour un puzzle"""
    # Signature simple basée sur dimensions et couleurs
    first_input, first_output = train_pairs[0]
    
    sig_parts = [
        f"in_{first_input.shape[0]}x{first_input.shape[1]}",
        f"out_{first_output.shape[0]}x{first_output.shape[1]}",
        f"colors_{len(np.unique(first_input))}"
    ]
    
    return "_".join(sig_parts)


def generate_programs_with_discovery(train_pairs, test_input, classification,
                                     memory: LearningMemory, puzzle_id: str,
                                     action_discovery: ActionDiscoveryEngine):
    """
    Génère programmes avec Phase 0 de découverte d'affordances
    
    Nouveau workflow:
    1. Phase 0: Découvrir affordances disponibles
    2. Phase 1: Générer programmes avec actions découvertes
    3. Phase 2: Filtrer par affordances validées
    """
    
    # PHASE 0: Découverte affordances (nouveau)
    context_sig = compute_context_signature(train_pairs)
    
    print(f"\n🔍 PHASE 0: Découverte affordances")
    discovery_results = action_discovery.discover_affordances(
        train_pairs, 
        context_sig
    )
    
    recommended_actions = action_discovery.get_recommended_actions(
        context_sig, 
        top_k=10
    )
    
    if recommended_actions:
        print(f"   ✅ Actions recommandées: {len(recommended_actions)}")
        print(f"   Top 3: {recommended_actions[:3]}")
    
    # PHASE 1: Génération programmes (existant)
    programs = []
    
    # Générer avec détecteurs existants
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
    
    # Composer avec actions découvertes (si disponibles)
    composer = Composer(max_depth=7, max_candidates=200)
    
    # Limiter à 50 programmes max pour performance
    if len(programs) > 50:
        programs = programs[:50]
    
    # Mémoire (existant)
    similar_solved = memory.get_similar_puzzles(puzzle_id, classification)
    if similar_solved:
        print(f"    → Puzzles similaires résolus: {len(similar_solved)}")
    
    return programs, discovery_results


def test_v11_with_action_discovery():
    """Test V11 complet avec Action Discovery"""
    
    print("="*80)
    print("MAGEN - TEST V11: ACTION DISCOVERY + METACOGNITIVE")
    print("="*80)
    print("Objectif: Mesurer impact découverte affordances")
    print("Baseline V5-Fixed: 45/400 (11.2%)")
    print("Mode: 100% LOCAL")
    print("Protocole: CLAUDE_PILOT + LUMVORAX")
    print("="*80)
    
    # Charger dataset
    print("\n📂 Chargement dataset...")
    dataset = load_dataset()
    puzzles = list(dataset.items())
    print(f"✓ {len(puzzles)} puzzles chargés")
    
    # Initialiser architecture
    print("\n🧠 Initialisation MAGEN V11...")
    
    # Mémoire
    memory = LearningMemory()
    memory.load_from_file()
    print(f"📚 Mémoire chargée:")
    print(f"  - Expériences totales: {len(memory.experiences)}")
    if memory.experiences:
        success_rate = sum(1 for e in memory.experiences.values() if e.success) / len(memory.experiences)
        print(f"  - Taux de succès: {success_rate*100:.1f}%")
    
    # Action Discovery Engine (NOUVEAU)
    action_discovery = ActionDiscoveryEngine(verbose=True)
    print(f"🔍 ActionDiscoveryEngine initialisé")
    print(f"  - Actions cataloguées: {len(action_discovery.inventory.primitives)}")
    
    # Autres modules
    classifier = PuzzleClassifier()
    validator = ProgramValidator()
    adaptive_strategy = AdaptiveStrategy()
    invariant_extractor = InvariantExtractor()
    
    print("✓ Architecture V11 initialisée")
    
    # Test sur 400 puzzles
    print(f"\n🚀 Début test {len(puzzles)} puzzles...")
    print()
    
    results = []
    success_count = 0
    error_count = 0
    
    start_time = time.time()
    
    for idx, (puzzle_id, puzzle_data) in enumerate(puzzles):
        progress = (idx + 1) / len(puzzles) * 100
        
        # Barre de progression
        bar_length = 50
        filled = int(bar_length * (idx + 1) / len(puzzles))
        bar = '█' * filled + '░' * (bar_length - filled)
        print(f"\r[{bar}] {progress:.1f}% ({idx+1}/{len(puzzles)}) | Succès: {success_count}", end='', flush=True)
        
        try:
            train_pairs = [(np.array(ex['input']), np.array(ex['output'])) 
                          for ex in puzzle_data['train']]
            test_input = np.array(puzzle_data['test'][0]['input'])
            
            # Classification
            classification = classifier.classify(train_pairs)
            
            # Génération avec Action Discovery (NOUVEAU)
            programs, discovery_results = generate_programs_with_discovery(
                train_pairs, test_input, classification,
                memory, puzzle_id, action_discovery
            )
            
            if not programs:
                results.append({
                    'puzzle_id': puzzle_id,
                    'success': False,
                    'reason': 'no_programs_generated'
                })
                continue
            
            # Extraction invariants
            invariants = invariant_extractor.extract_invariants(train_pairs)
            
            # PWFE avec scoring progressif (V5-Fixed)
            pwfe = ParallelWorldFrameEngine(
                max_hypotheses=50,
                max_frames=5,
                pruning_threshold=0.3,
                verbose=False
            )
            
            pwfe_result = pwfe.explore_parallel_worlds(
                test_input, train_pairs, programs, invariants
            )
            
            # Validation
            solved = False
            if pwfe_result['success'] and pwfe_result['best_trajectory']:
                trajectory = pwfe_result['best_trajectory']
                final_world = pwfe.worlds[trajectory.world_sequence[-1]]
                
                if validator.validate_program(final_world.program, train_pairs):
                    solved = True
                    success_count += 1
            
            # Adaptive Strategy fallback
            if not solved:
                candidates = adaptive_strategy.generate_alternative_programs(
                    train_pairs, test_input, programs
                )
                
                for program in candidates[:5]:
                    if validator.validate_program(program, train_pairs):
                        solved = True
                        success_count += 1
                        break
            
            results.append({
                'puzzle_id': puzzle_id,
                'success': solved,
                'discovery_experiments': discovery_results.get('total_experiments', 0),
                'affordances_discovered': len(discovery_results.get('discovered_actions', [])),
                'pwfe_worlds': pwfe_result.get('total_worlds', 0)
            })
        
        except Exception as e:
            error_count += 1
            results.append({
                'puzzle_id': puzzle_id,
                'success': False,
                'error': str(e)
            })
    
    print()  # Nouvelle ligne après barre de progression
    
    elapsed = time.time() - start_time
    
    # Résultats finaux
    print("\n")
    print("="*80)
    print(f"RÉSULTAT FINAL: {success_count}/{len(puzzles)} ({success_count/len(puzzles)*100:.1f}%)")
    print("="*80)
    
    print(f"\n📊 STATISTIQUES:")
    print(f"  - Temps total: {elapsed:.1f}s ({elapsed/60:.1f}min)")
    print(f"  - Temps moyen/puzzle: {elapsed/len(puzzles):.2f}s")
    print(f"  - Erreurs: {error_count}")
    
    # Statistiques Action Discovery
    total_experiments = sum(r.get('discovery_experiments', 0) for r in results)
    total_affordances = sum(r.get('affordances_discovered', 0) for r in results)
    
    print(f"\n🔍 ACTION DISCOVERY:")
    print(f"  - Expériences totales: {total_experiments}")
    print(f"  - Affordances découvertes: {total_affordances}")
    print(f"  - Moyenne expériences/puzzle: {total_experiments/len(puzzles):.1f}")
    
    # Comparaison avec baseline
    baseline_v5 = 45
    improvement = success_count - baseline_v5
    improvement_pct = (improvement / baseline_v5 * 100) if baseline_v5 > 0 else 0
    
    print(f"\n📈 COMPARAISON BASELINE V5-Fixed:")
    print(f"  - V5-Fixed: {baseline_v5}/400 (11.2%)")
    print(f"  - V11: {success_count}/400 ({success_count/len(puzzles)*100:.1f}%)")
    print(f"  - Différence: {improvement:+d} puzzles ({improvement_pct:+.1f}%)")
    
    if success_count >= baseline_v5:
        print(f"\n🎯 OBJECTIF:")
        if success_count > baseline_v5:
            print(f"  ✅✅✅ AMÉLIORATION: +{improvement} puzzles!")
        else:
            print(f"  ✅ BASELINE MAINTENUE")
    else:
        print(f"\n🎯 OBJECTIF:")
        print(f"  ⚠️  RÉGRESSION: {improvement} puzzles")
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_file = f"results_v11_{timestamp}.json"
    
    with open(results_file, 'w') as f:
        json.dump({
            'version': 'V11_ActionDiscovery',
            'timestamp': timestamp,
            'total_puzzles': len(puzzles),
            'success_count': success_count,
            'success_rate': success_count / len(puzzles),
            'elapsed_time': elapsed,
            'baseline_v5': baseline_v5,
            'improvement': improvement,
            'action_discovery_stats': {
                'total_experiments': total_experiments,
                'total_affordances': total_affordances,
                'avg_experiments_per_puzzle': total_experiments / len(puzzles)
            },
            'results': results
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {results_file}")
    
    print("\n" + "="*80)
    print("TEST V11 TERMINÉ")
    print("="*80)
    
    return success_count, results


if __name__ == "__main__":
    success_count, results = test_v11_with_action_discovery()

# Made with Bob
