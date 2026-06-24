#!/usr/bin/env python3
"""
Test MAGEN V35 sur 10 puzzles ARC réels - Validation Rapide
Intègre TrajectoryUtilityModel, ValueOfStoppingEstimator, GlobalRegretDetector
"""

import json
import time
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Ajouter le chemin du projet
sys.path.insert(0, str(Path(__file__).parent))

from integrate_v35_to_solver import MAGENSolverV35


def load_arc_puzzles(limit: int = 10) -> List[Dict[str, Any]]:
    """Charger les puzzles ARC depuis le dataset"""
    arc_path = Path(__file__).parent / "arc-agi_training_challenges.json"
    
    if not arc_path.exists():
        print(f"⚠️  Dataset ARC non trouvé: {arc_path}")
        print("Création de puzzles de test synthétiques...")
        return create_synthetic_puzzles(limit)
    
    with open(arc_path, 'r') as f:
        data = json.load(f)
    
    puzzles = []
    for puzzle_id, puzzle_data in list(data.items())[:limit]:
        puzzles.append({
            'id': puzzle_id,
            'train': puzzle_data.get('train', []),
            'test': puzzle_data.get('test', [])
        })
    
    return puzzles


def create_synthetic_puzzles(count: int) -> List[Dict[str, Any]]:
    """Créer des puzzles synthétiques pour test"""
    puzzles = []
    for i in range(count):
        puzzle = {
            'id': f'synthetic_{i:03d}',
            'train': [
                {
                    'input': [[0, 1], [1, 0]],
                    'output': [[1, 0], [0, 1]]
                }
            ],
            'test': [
                {
                    'input': [[0, 1, 0], [1, 0, 1], [0, 1, 0]],
                    'output': [[1, 0, 1], [0, 1, 0], [1, 0, 1]]
                }
            ]
        }
        puzzles.append(puzzle)
    
    return puzzles


def run_test_suite():
    """Exécuter la suite de tests sur 10 puzzles ARC"""
    
    print("=" * 80)
    print("MAGEN V35 - TEST RÉEL SUR 10 PUZZLES ARC")
    print("=" * 80)
    print()
    
    # Timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = Path(__file__).parent / "logs" / "test_v35_arc"
    log_dir.mkdir(parents=True, exist_ok=True)
    
    forensic_path = log_dir / f"test_v35_arc_10puzzles_{timestamp}.jsonl"
    results_path = log_dir / f"results_v35_arc_10puzzles_{timestamp}.json"
    
    print(f"📁 Logs forensic: {forensic_path}")
    print(f"📊 Résultats: {results_path}")
    print()
    
    # Charger puzzles
    print("📦 Chargement puzzles ARC...")
    puzzles = load_arc_puzzles(limit=10)
    print(f"✅ {len(puzzles)} puzzles chargés")
    print()
    
    # Initialiser solver V35
    print("🚀 Initialisation MAGEN V35...")
    solver = MAGENSolverV35(
        forensic_log_path=str(forensic_path),
        enable_trajectory_monitoring=True,
        enable_stopping_estimation=True,
        enable_regret_detection=True
    )
    print("✅ Solver V35 initialisé")
    print()
    
    # Résultats
    results = {
        'timestamp': timestamp,
        'version': 'V35',
        'total_puzzles': len(puzzles),
        'puzzles': [],
        'summary': {
            'solved': 0,
            'failed': 0,
            'timeout': 0,
            'total_time': 0.0,
            'avg_time': 0.0,
            'avg_steps': 0.0,
            'avg_trajectory_utility': 0.0,
            'avg_value_of_stopping': 0.0,
            'avg_regret_level': 0.0
        }
    }
    
    # Tester chaque puzzle
    print("🧩 Test des puzzles...")
    print("-" * 80)
    
    for idx, puzzle in enumerate(puzzles, 1):
        puzzle_id = puzzle['id']
        print(f"\n[{idx}/{len(puzzles)}] Puzzle: {puzzle_id}")
        
        start_time = time.time()
        
        try:
            # Extraire données du puzzle
            training_pairs = puzzle.get('train', [])
            test_cases = puzzle.get('test', [])
            test_input = test_cases[0]['input'] if test_cases else [[0]]
            
            # Résoudre le puzzle
            solution = solver.solve_puzzle(
                puzzle_id=puzzle_id,
                training_pairs=training_pairs,
                test_input=test_input
            )
            
            elapsed = time.time() - start_time
            
            # Collecter métriques
            puzzle_result = {
                'puzzle_id': puzzle_id,
                'status': solution.get('status', 'unknown'),
                'solved': solution.get('solved', False),
                'time': elapsed,
                'steps': solution.get('steps', 0),
                'trajectory_utility': solution.get('trajectory_utility', 0.0),
                'value_of_stopping': solution.get('value_of_stopping', 0.0),
                'regret_level': solution.get('regret_level', 'NONE'),
                'interventions': solution.get('interventions', 0),
                'error': solution.get('error', None)
            }
            
            results['puzzles'].append(puzzle_result)
            
            # Mise à jour statistiques
            if puzzle_result['solved']:
                results['summary']['solved'] += 1
                print(f"  ✅ RÉSOLU en {elapsed:.2f}s ({puzzle_result['steps']} steps)")
            else:
                results['summary']['failed'] += 1
                print(f"  ❌ ÉCHEC en {elapsed:.2f}s")
            
            print(f"  📊 Trajectory Utility: {puzzle_result['trajectory_utility']:.3f}")
            print(f"  📊 Value of Stopping: {puzzle_result['value_of_stopping']:.3f}")
            print(f"  📊 Regret Level: {puzzle_result['regret_level']}")
            
        except TimeoutError:
            elapsed = time.time() - start_time
            results['summary']['timeout'] += 1
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'status': 'timeout',
                'solved': False,
                'time': elapsed,
                'error': 'Timeout'
            })
            print(f"  ⏱️  TIMEOUT après {elapsed:.2f}s")
            
        except Exception as e:
            elapsed = time.time() - start_time
            results['summary']['failed'] += 1
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'status': 'error',
                'solved': False,
                'time': elapsed,
                'error': str(e)
            })
            print(f"  ❌ ERREUR: {e}")
    
    # Calculer statistiques finales
    total_time = sum(p.get('time', 0) for p in results['puzzles'])
    solved_puzzles = [p for p in results['puzzles'] if p.get('solved', False)]
    
    results['summary']['total_time'] = total_time
    results['summary']['avg_time'] = total_time / len(puzzles) if puzzles else 0
    
    if solved_puzzles:
        results['summary']['avg_steps'] = sum(p.get('steps', 0) for p in solved_puzzles) / len(solved_puzzles)
        results['summary']['avg_trajectory_utility'] = sum(p.get('trajectory_utility', 0) for p in solved_puzzles) / len(solved_puzzles)
        results['summary']['avg_value_of_stopping'] = sum(p.get('value_of_stopping', 0) for p in solved_puzzles) / len(solved_puzzles)
    
    # Sauvegarder résultats
    with open(results_path, 'w') as f:
        json.dump(results, f, indent=2)
    
    # Afficher résumé
    print()
    print("=" * 80)
    print("RÉSUMÉ FINAL")
    print("=" * 80)
    print(f"Total puzzles: {len(puzzles)}")
    print(f"Résolus: {results['summary']['solved']} ({results['summary']['solved']/len(puzzles)*100:.1f}%)")
    print(f"Échecs: {results['summary']['failed']}")
    print(f"Timeouts: {results['summary']['timeout']}")
    print(f"Temps total: {results['summary']['total_time']:.2f}s")
    print(f"Temps moyen: {results['summary']['avg_time']:.2f}s")
    
    if solved_puzzles:
        print(f"Steps moyen: {results['summary']['avg_steps']:.1f}")
        print(f"Trajectory Utility moyen: {results['summary']['avg_trajectory_utility']:.3f}")
        print(f"Value of Stopping moyen: {results['summary']['avg_value_of_stopping']:.3f}")
    
    print()
    print(f"📁 Résultats sauvegardés: {results_path}")
    print(f"📁 Logs forensic: {forensic_path}")
    print()
    
    # Fermer solver
    solver.close()
    
    return results


if __name__ == "__main__":
    try:
        results = run_test_suite()
        sys.exit(0 if results['summary']['solved'] > 0 else 1)
    except KeyboardInterrupt:
        print("\n⚠️  Test interrompu par l'utilisateur")
        sys.exit(130)
    except Exception as e:
        print(f"\n❌ ERREUR FATALE: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
