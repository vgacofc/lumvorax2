"""
Validation Advanced Pipeline - Test Intégration Complète
=========================================================

Test du pipeline avancé avec:
- 29 primitives symboliques
- Hiérarchie 3 niveaux
- Règles contextuelles
- Composition transformations
- Validation forensique

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot + LumVorax
"""

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))

import json
import time
import numpy as np
import traceback
from typing import List, Tuple

from core.advanced_pipeline import AdvancedPipeline
from forensic.simple_logger import SimpleLogger


def load_dataset(path: str) -> List[dict]:
    """Charger dataset ARC"""
    with open(path, 'r') as f:
        data = json.load(f)
    
    # Convertir dict en liste avec IDs
    puzzles = []
    for puzzle_id, puzzle_data in data.items():
        puzzle_data['id'] = puzzle_id
        puzzles.append(puzzle_data)
    
    return puzzles


def validate_advanced_pipeline():
    """Validation complète pipeline avancé"""
    print("\n" + "="*80)
    print("VALIDATION ADVANCED PIPELINE - INTÉGRATION COMPLÈTE")
    print("="*80)
    print("\nProtocole: Claude Pilot + LumVorax")
    print("Mode: LOCAL (100%)")
    print()
    
    # Logger forensique
    forensic = SimpleLogger("logs/magen/advanced_validation")
    forensic.log_event("SESSION_START", {
        "mode": "LOCAL",
        "protocol": "CLAUDE_PILOT",
        "pipeline": "ADVANCED_FULL_INTEGRATION"
    })
    
    # Charger dataset
    dataset_path = Path(__file__).parent / "arc-agi_training_challenges.json"
    puzzles = load_dataset(str(dataset_path))
    
    print(f"Dataset: {len(puzzles)} puzzles")
    forensic.log_event("DATASET_LOADED", {
        "path": str(dataset_path),
        "total": len(puzzles),
        "selected": len(puzzles)
    })
    
    # Créer pipeline avancé
    print("\nInitialisation pipeline avancé...")
    pipeline = AdvancedPipeline(
        beam_width=10,
        max_depth=5,
        use_memory=False,  # Désactiver memory pour premier test
        verbose=True
    )
    
    forensic.log_event("PIPELINE_INIT", {
        "beam_width": 10,
        "max_depth": 5,
        "use_memory": False
    })
    
    # Tester chaque puzzle
    results = []
    
    for puzzle_data in puzzles:
        puzzle_id = puzzle_data['id']
        train_pairs = []
        
        # Préparer training pairs
        for pair in puzzle_data['train']:
            input_grid = np.array(pair['input'], dtype=int)
            output_grid = np.array(pair['output'], dtype=int)
            train_pairs.append((input_grid, output_grid))
        
        # Test input
        test_input = np.array(puzzle_data['test'][0]['input'], dtype=int)
        test_output = np.array(puzzle_data['test'][0]['output'], dtype=int)
        
        print(f"\n{'='*80}")
        print(f"Puzzle: {puzzle_id}")
        print(f"Train pairs: {len(train_pairs)}")
        print(f"Test shape: {test_input.shape}")
        print(f"{'='*80}")
        
        start_time = time.time()
        
        # Résoudre puzzle
        try:
            prediction = pipeline.solve_puzzle(train_pairs, test_input)
            
            # Vérifier succès
            exact_match = np.array_equal(prediction, test_output)
            elapsed = time.time() - start_time
            
            result = {
                "puzzle_id": puzzle_id,
                "success": exact_match,
                "elapsed": elapsed,
                "train_pairs": len(train_pairs),
                "test_shape": test_input.shape
            }
            results.append(result)
            
            # Log forensique
            forensic.log_event("PUZZLE_SOLVED", {
                "puzzle_id": puzzle_id,
                "success": exact_match,
                "elapsed": elapsed
            })
            
            # Afficher résultat
            status = "✓ SUCCÈS" if exact_match else "✗ ÉCHEC"
            print(f"\n{status}")
            print(f"Temps: {elapsed:.3f}s")
            
            if exact_match:
                pipeline.puzzles_solved += 1
            
        except Exception as e:
            error_traceback = traceback.format_exc()
            print(f"\n✗ ERREUR: {e}")
            print(f"TRACEBACK:\n{error_traceback}")
            forensic.log_event("PUZZLE_ERROR", {
                "puzzle_id": puzzle_id,
                "error": str(e),
                "traceback": error_traceback
            })
            results.append({
                "puzzle_id": puzzle_id,
                "success": False,
                "error": str(e)
            })
    
    # Statistiques finales
    print(f"\n{'='*80}")
    print("RÉSULTATS FINAUX")
    print(f"{'='*80}")
    
    successes = sum(1 for r in results if r.get('success', False))
    total = len(results)
    success_rate = (successes / total * 100) if total > 0 else 0.0
    
    print(f"\nSuccès: {successes}/{total} ({success_rate:.1f}%)")
    print(f"\nDétails par puzzle:")
    for r in results:
        status = "✓" if r.get('success', False) else "✗"
        elapsed = r.get('elapsed', 0)
        print(f"  {status} {r['puzzle_id']}: {elapsed:.3f}s")
    
    # Statistiques pipeline
    print(f"\n{'='*80}")
    print("STATISTIQUES PIPELINE")
    print(f"{'='*80}")
    
    stats = pipeline.get_statistics()
    print(f"\nPuzzles résolus: {stats['puzzles_solved']}/{stats['total_puzzles']}")
    print(f"Taux succès: {stats['success_rate']:.1%}")
    print(f"\nPrimitives: {stats['primitives']['total_transforms']} transformations")
    print(f"Object-level: {stats['object_level']['transforms_applied']} transformations")
    print(f"Scene-level: {stats['scene_level']['transforms_applied']} transformations")
    print(f"Global-level: {stats['global_level']['transforms_applied']} transformations")
    print(f"Règles détectées: {stats['rules_detected']}")
    
    # Log final
    forensic.log_event("VALIDATION_COMPLETE", {
        "total": total,
        "successes": successes,
        "failures": total - successes,
        "success_rate": success_rate,
        "results": results,
        "pipeline_stats": stats
    })
    
    # Sauvegarder logs
    log_path = forensic.save_logs()
    print(f"\n{'='*80}")
    print(f"Logs forensiques: {log_path}")
    print(f"{'='*80}")
    
    # Comparaison avec baseline
    print(f"\n{'='*80}")
    print("COMPARAISON AVEC BASELINE")
    print(f"{'='*80}")
    print(f"\nBaseline (copie input): 0/3 (0.0%)")
    print(f"Advanced Pipeline:      {successes}/{total} ({success_rate:.1f}%)")
    
    if success_rate > 0:
        print(f"\n🎉 AMÉLIORATION: +{success_rate:.1f}% vs baseline!")
    else:
        print(f"\n⚠️  Aucune amélioration vs baseline")
        print(f"   Analyse forensique nécessaire")
    
    print(f"\n{'='*80}")
    print("✓ VALIDATION COMPLÉTÉE")
    print(f"{'='*80}\n")
    
    return results, stats


if __name__ == "__main__":
    results, stats = validate_advanced_pipeline()

# Made with Bob
