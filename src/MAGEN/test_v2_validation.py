"""
Test de Validation Architecture V2 - Program Synthesis Engine

Objectif: Valider que V2 reproduit au minimum les 11 succès de Session 26
Puzzles cibles: 2dee498d, 3aa6fb7a, 3c9b0459, 5bd6f4ac, 6150a2bd, 
                67a3c6ac, 68b16354, 88a62173, 9172f3a0, c59eb873, ed36ccf7
"""

import json
import time
import numpy as np
from pathlib import Path
from typing import Dict, List
import sys

# Import modules V2
from dsl.program import Program
from dsl.composer import Composer
from dsl.primitives import *
from synthesis.synthesizer import ProgramSynthesizer
from synthesis.scorer import ProgramScorer
from synthesis.validator import ProgramValidator


# Liste des 11 puzzles réussis Session 26
SUCCESS_PUZZLES_S26 = [
    '2dee498d', '3aa6fb7a', '3c9b0459', '5bd6f4ac', '6150a2bd',
    '67a3c6ac', '68b16354', '88a62173', '9172f3a0', 'c59eb873', 'ed36ccf7'
]


def load_dataset(path: str = "arc-agi_training_challenges.json") -> Dict:
    """Charge le dataset ARC"""
    with open(path, 'r') as f:
        return json.load(f)


def test_single_puzzle_v2(puzzle_id: str, puzzle_data: Dict, 
                         synthesizer: ProgramSynthesizer) -> Dict:
    """
    Test un puzzle avec Architecture V2
    
    Returns:
        Dict avec résultats détaillés
    """
    start_time = time.time()
    
    # Préparer train pairs
    train_pairs = []
    for pair in puzzle_data['train']:
        train_pairs.append({
            'input': np.array(pair['input']),
            'output': np.array(pair['output'])
        })
    
    # Test pairs
    test_pairs = []
    for pair in puzzle_data['test']:
        test_pairs.append({
            'input': np.array(pair['input']),
            'output': np.array(pair['output'])
        })
    
    # Synthétiser programme
    try:
        result = synthesizer.synthesize(train_pairs)
        
        if result is None:
            return {
                'puzzle_id': puzzle_id,
                'success': False,
                'train_accuracy': 0.0,
                'test_accuracy': 0.0,
                'elapsed': time.time() - start_time,
                'error': 'No program synthesized'
            }
        
        # Valider sur test
        test_correct = 0
        for test_pair in test_pairs:
            try:
                predicted = result.program.execute(test_pair['input'])
                if np.array_equal(predicted, test_pair['output']):
                    test_correct += 1
            except Exception:
                pass
        
        test_accuracy = test_correct / len(test_pairs) if test_pairs else 0.0
        
        return {
            'puzzle_id': puzzle_id,
            'success': (test_accuracy == 1.0),
            'train_accuracy': result.train_accuracy,
            'test_accuracy': test_accuracy,
            'elapsed': time.time() - start_time,
            'program_complexity': result.program.complexity(),
            'program_length': result.program.length(),
            'program_str': result.program.to_string()
        }
    
    except Exception as e:
        return {
            'puzzle_id': puzzle_id,
            'success': False,
            'train_accuracy': 0.0,
            'test_accuracy': 0.0,
            'elapsed': time.time() - start_time,
            'error': str(e)
        }


def run_validation_v2():
    """
    Lance validation complète V2 sur les 11 succès Session 26
    """
    print("=" * 80)
    print("TEST VALIDATION ARCHITECTURE V2 - PROGRAM SYNTHESIS ENGINE")
    print("=" * 80)
    print(f"\nObjectif: Reproduire 11/11 succès Session 26")
    print(f"Puzzles cibles: {len(SUCCESS_PUZZLES_S26)}")
    print()
    
    # Charger dataset
    print("Chargement dataset...")
    dataset = load_dataset()
    print(f"✓ Dataset chargé: {len(dataset)} puzzles")
    
    # Initialiser synthesizer
    print("\nInitialisation Program Synthesizer V2...")
    synthesizer = ProgramSynthesizer(
        max_candidates=100,
        max_depth=5,
        timeout=5.0,
        enable_simplification=True
    )
    print("✓ Synthesizer initialisé")
    
    # Tester chaque puzzle
    print("\n" + "=" * 80)
    print("TESTS INDIVIDUELS")
    print("=" * 80)
    
    results = []
    successes = 0
    total_time = 0.0
    
    for i, puzzle_id in enumerate(SUCCESS_PUZZLES_S26, 1):
        print(f"\n[{i}/11] Test puzzle {puzzle_id}...")
        
        if puzzle_id not in dataset:
            print(f"  ⚠️  Puzzle non trouvé dans dataset")
            continue
        
        puzzle_data = dataset[puzzle_id]
        result = test_single_puzzle_v2(puzzle_id, puzzle_data, synthesizer)
        results.append(result)
        
        # Affichage résultat
        if result['success']:
            successes += 1
            print(f"  ✅ SUCCÈS")
            print(f"     Train: {result['train_accuracy']:.1%}")
            print(f"     Test:  {result['test_accuracy']:.1%}")
            print(f"     Temps: {result['elapsed']:.3f}s")
            if 'program_complexity' in result:
                print(f"     Complexité: {result['program_complexity']}")
                print(f"     Programme: {result['program_str'][:100]}...")
        else:
            print(f"  ❌ ÉCHEC")
            print(f"     Train: {result['train_accuracy']:.1%}")
            print(f"     Test:  {result['test_accuracy']:.1%}")
            print(f"     Temps: {result['elapsed']:.3f}s")
            if 'error' in result:
                print(f"     Erreur: {result['error']}")
        
        total_time += result['elapsed']
    
    # Rapport final
    print("\n" + "=" * 80)
    print("RAPPORT FINAL")
    print("=" * 80)
    
    success_rate = successes / len(SUCCESS_PUZZLES_S26) * 100
    avg_time = total_time / len(SUCCESS_PUZZLES_S26)
    
    print(f"\n📊 RÉSULTATS:")
    print(f"   Succès:        {successes}/{len(SUCCESS_PUZZLES_S26)} ({success_rate:.1f}%)")
    print(f"   Temps total:   {total_time:.2f}s")
    print(f"   Temps moyen:   {avg_time:.3f}s/puzzle")
    
    # Comparaison avec Session 26
    print(f"\n📈 COMPARAISON SESSION 26:")
    print(f"   Session 26 (V1):  11/11 (100%)")
    print(f"   V2 actuel:        {successes}/11 ({success_rate:.1f}%)")
    
    if successes >= 11:
        print(f"\n✅ OBJECTIF ATTEINT: V2 égale ou dépasse V1!")
    elif successes >= 8:
        print(f"\n⚠️  OBJECTIF PARTIEL: V2 proche de V1 ({successes}/11)")
    else:
        print(f"\n❌ OBJECTIF NON ATTEINT: V2 en-dessous de V1 ({successes}/11)")
    
    # Statistiques synthesizer
    print(f"\n📊 STATISTIQUES SYNTHESIZER:")
    stats = synthesizer.get_statistics()
    print(f"   Total syntheses:     {stats['total_syntheses']}")
    print(f"   Syntheses réussies:  {stats['successful_syntheses']}")
    print(f"   Taux de succès:      {stats['success_rate']:.1%}")
    print(f"   Candidats générés:   {stats['total_candidates_generated']}")
    print(f"   Candidats validés:   {stats['total_candidates_validated']}")
    print(f"   Temps moyen:         {stats['avg_synthesis_time']:.3f}s")
    
    # Sauvegarder résultats
    output_file = f"logs/magen/v2_validation/validation_v2_{int(time.time())}.json"
    Path(output_file).parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_file, 'w') as f:
        json.dump({
            'timestamp': time.time(),
            'objective': 'Reproduce 11 Session 26 successes with V2',
            'target_puzzles': SUCCESS_PUZZLES_S26,
            'results': results,
            'summary': {
                'successes': successes,
                'total': len(SUCCESS_PUZZLES_S26),
                'success_rate': success_rate,
                'total_time': total_time,
                'avg_time': avg_time
            },
            'synthesizer_stats': stats
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {output_file}")
    print("\n" + "=" * 80)
    
    return successes, len(SUCCESS_PUZZLES_S26)


if __name__ == "__main__":
    try:
        successes, total = run_validation_v2()
        sys.exit(0 if successes >= 11 else 1)
    except Exception as e:
        print(f"\n❌ ERREUR CRITIQUE: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(2)

# Made with Bob
