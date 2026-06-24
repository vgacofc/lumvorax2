"""
Test ARC Real Puzzles avec Adaptateur V36.2
Test sur 10 vrais puzzles ARC avec métriques complètes

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path
import numpy as np
from datetime import datetime
import json
import time

# Ajouter le chemin du module
sys.path.insert(0, str(Path(__file__).parent))

from core.arc_solver_adapter import ARCSolverAdapter, ARCPuzzle


# 10 puzzles ARC réels simplifiés pour test
REAL_PUZZLES = [
    {
        'id': 'arc_001',
        'train': [
            (np.array([[0,0,1],[0,1,0],[1,0,0]]), np.array([[1,1,1],[1,1,1],[1,1,1]])),
        ],
        'test': np.array([[0,1,0],[1,0,1],[0,1,0]])
    },
    {
        'id': 'arc_002',
        'train': [
            (np.array([[1,0],[0,1]]), np.array([[0,1],[1,0]])),
        ],
        'test': np.array([[1,1],[1,1]])
    },
    {
        'id': 'arc_003',
        'train': [
            (np.array([[0,0,0,0],[0,1,1,0],[0,1,1,0],[0,0,0,0]]), 
             np.array([[1,1,1,1],[1,0,0,1],[1,0,0,1],[1,1,1,1]])),
        ],
        'test': np.array([[0,0,0],[0,1,0],[0,0,0]])
    },
    {
        'id': 'arc_004',
        'train': [
            (np.ones((3,3)), np.zeros((3,3))),
        ],
        'test': np.ones((4,4))
    },
    {
        'id': 'arc_005',
        'train': [
            (np.array([[1,2,3],[4,5,6],[7,8,9]]), np.array([[9,8,7],[6,5,4],[3,2,1]])),
        ],
        'test': np.array([[1,1,1],[2,2,2],[3,3,3]])
    },
    {
        'id': 'arc_006',
        'train': [
            (np.array([[0,1,0],[1,1,1],[0,1,0]]), np.array([[1,1,1],[1,0,1],[1,1,1]])),
        ],
        'test': np.array([[1,0,1],[0,0,0],[1,0,1]])
    },
    {
        'id': 'arc_007',
        'train': [
            (np.array([[1,2],[3,4]]), np.array([[4,3],[2,1]])),
        ],
        'test': np.array([[5,6],[7,8]])
    },
    {
        'id': 'arc_008',
        'train': [
            (np.array([[0,0,1,1],[0,0,1,1],[1,1,0,0],[1,1,0,0]]), 
             np.array([[1,1,0,0],[1,1,0,0],[0,0,1,1],[0,0,1,1]])),
        ],
        'test': np.array([[1,1,1],[1,0,1],[1,1,1]])
    },
    {
        'id': 'arc_009',
        'train': [
            (np.array([[1,0,1,0,1]]), np.array([[0,1,0,1,0]])),
        ],
        'test': np.array([[1,1,0,0,1,1]])
    },
    {
        'id': 'arc_010',
        'train': [
            (np.array([[0,1,2],[3,4,5],[6,7,8]]), np.array([[8,7,6],[5,4,3],[2,1,0]])),
        ],
        'test': np.array([[1,2,3,4],[5,6,7,8]])
    }
]


def test_real_puzzles():
    """Test sur 10 puzzles ARC réels."""
    print("\n" + "="*70)
    print("🧪 TEST PUZZLES ARC RÉELS AVEC V35+V36")
    print("="*70)
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Puzzles: {len(REAL_PUZZLES)}")
    print()
    
    # Créer adaptateur
    adapter = ARCSolverAdapter(
        max_steps=100,
        forensic_log_path="logs/magen_v36/arc_real_puzzles.jsonl"
    )
    
    results = []
    total_start = time.time()
    
    # Tester chaque puzzle
    for i, puzzle_data in enumerate(REAL_PUZZLES, 1):
        print(f"\n{'='*70}")
        print(f"PUZZLE {i}/{len(REAL_PUZZLES)}: {puzzle_data['id']}")
        print(f"{'='*70}")
        
        # Créer puzzle
        puzzle = ARCPuzzle(
            puzzle_id=puzzle_data['id'],
            train_pairs=puzzle_data['train'],
            test_input=puzzle_data['test']
        )
        
        print(f"📊 Input shape: {puzzle.test_input.shape}")
        print(f"📊 Train pairs: {len(puzzle.train_pairs)}")
        
        # Résoudre
        try:
            metrics = adapter.solve_puzzle(puzzle)
            
            print(f"\n✅ Puzzle complété")
            print(f"   - Steps: {metrics.total_steps}")
            print(f"   - Durée: {metrics.end_time - metrics.start_time:.3f}s")
            print(f"   - Régions: {metrics.regions_discovered}")
            print(f"   - Loops: {metrics.loops_detected}")
            print(f"   - Cohérence: {metrics.world_coherence:.2%}")
            print(f"   - Décisions métacog: {metrics.metacog_decisions}")
            
            results.append({
                'puzzle_id': puzzle_data['id'],
                'success': True,
                'steps': metrics.total_steps,
                'duration': metrics.end_time - metrics.start_time,
                'regions': metrics.regions_discovered,
                'loops': metrics.loops_detected,
                'coherence': metrics.world_coherence,
                'metacog_decisions': metrics.metacog_decisions
            })
            
        except Exception as e:
            print(f"\n❌ Erreur: {str(e)}")
            results.append({
                'puzzle_id': puzzle_data['id'],
                'success': False,
                'error': str(e)
            })
    
    total_duration = time.time() - total_start
    
    # Résumé global
    print("\n" + "="*70)
    print("📊 RÉSUMÉ GLOBAL")
    print("="*70)
    
    successful = [r for r in results if r.get('success', False)]
    failed = [r for r in results if not r.get('success', False)]
    
    print(f"\n✅ Réussis: {len(successful)}/{len(REAL_PUZZLES)} ({len(successful)/len(REAL_PUZZLES)*100:.1f}%)")
    print(f"❌ Échoués: {len(failed)}/{len(REAL_PUZZLES)}")
    print(f"⏱️  Durée totale: {total_duration:.2f}s")
    
    if successful:
        avg_steps = np.mean([r['steps'] for r in successful])
        avg_duration = np.mean([r['duration'] for r in successful])
        avg_regions = np.mean([r['regions'] for r in successful])
        avg_loops = np.mean([r['loops'] for r in successful])
        avg_coherence = np.mean([r['coherence'] for r in successful])
        avg_metacog = np.mean([r['metacog_decisions'] for r in successful])
        
        print(f"\n📈 Moyennes (puzzles réussis):")
        print(f"   - Steps: {avg_steps:.1f}")
        print(f"   - Durée: {avg_duration:.3f}s")
        print(f"   - Régions: {avg_regions:.1f}")
        print(f"   - Loops: {avg_loops:.1f}")
        print(f"   - Cohérence: {avg_coherence:.2%}")
        print(f"   - Décisions métacog: {avg_metacog:.1f}")
    
    # Métriques adaptateur
    summary = adapter.get_metrics_summary()
    print(f"\n📊 Métriques adaptateur:")
    print(f"   - Total puzzles: {summary['total_puzzles']}")
    print(f"   - Success rate: {summary['success_rate']:.1%}")
    print(f"   - Avg steps: {summary['avg_steps']:.1f}")
    print(f"   - Avg time: {summary['avg_time']:.3f}s")
    print(f"   - Total loops: {summary['total_loops']}")
    
    # Sauvegarder résultats
    results_file = "logs/magen_v36/arc_real_puzzles_results.json"
    Path(results_file).parent.mkdir(parents=True, exist_ok=True)
    
    with open(results_file, 'w') as f:
        json.dump({
            'timestamp': datetime.now().isoformat(),
            'total_puzzles': len(REAL_PUZZLES),
            'successful': len(successful),
            'failed': len(failed),
            'total_duration': total_duration,
            'results': results,
            'adapter_summary': summary
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {results_file}")
    
    adapter.close()
    
    print("\n" + "="*70)
    print(f"✅ Test complété: {len(successful)}/{len(REAL_PUZZLES)} puzzles réussis")
    print("="*70)
    
    return len(successful) == len(REAL_PUZZLES)


if __name__ == "__main__":
    success = test_real_puzzles()
    sys.exit(0 if success else 1)

# Made with Bob
