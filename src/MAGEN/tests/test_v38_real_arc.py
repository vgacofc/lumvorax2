#!/usr/bin/env python3
"""
Test V38 sur Dataset ARC-AGI-3 RÉEL
====================================

Test architecture cognitive sur vrais puzzles ARC.
Analyse logs forensic après chaque exécution.
"""

import json
import sys
import time
from pathlib import Path
from typing import Dict, List

import numpy as np

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.arc_solver_adapter import ARCSolverAdapter, ARCPuzzle


def load_real_arc_dataset(dataset_path: str, max_puzzles: int = 10) -> List[Dict]:
    """Charge dataset ARC-AGI-3 réel."""
    print(f"📂 Chargement dataset: {dataset_path}")
    
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    puzzles = []
    for puzzle_id, puzzle_data in list(data.items())[:max_puzzles]:
        # Convertir format ARC
        train_pairs = []
        for example in puzzle_data.get('train', []):
            train_pairs.append((
                np.array(example['input']),
                np.array(example['output'])
            ))
        
        test_examples = puzzle_data.get('test', [])
        if test_examples:
            test_input = np.array(test_examples[0]['input'])
            test_output = np.array(test_examples[0]['output'])
        else:
            continue
        
        puzzles.append({
            'puzzle_id': puzzle_id,
            'train_pairs': train_pairs,
            'test_input': test_input,
            'test_output': test_output
        })
    
    print(f"✅ {len(puzzles)} puzzles chargés\n")
    return puzzles


def analyze_forensic_logs(log_dir: Path, puzzle_id: str):
    """Analyse logs forensic pour un puzzle."""
    print(f"\n{'='*80}")
    print(f"ANALYSE LOGS FORENSIC: {puzzle_id}")
    print('='*80)
    
    log_files = {
        'affordance': log_dir / 'solver_affordance.jsonl',
        'reputation': log_dir / 'solver_reputation.jsonl',
        'budget': log_dir / 'solver_budget.jsonl',
        'stagnation': log_dir / 'solver_stagnation.jsonl'
    }
    
    for name, log_file in log_files.items():
        if not log_file.exists():
            continue
        
        with open(log_file, 'r') as f:
            events = [json.loads(line) for line in f if line.strip()]
        
        if not events:
            continue
        
        print(f"\n📄 {name.upper()}: {len(events)} événements")
        
        # Compter types
        event_types = {}
        for event in events:
            et = event.get('event_type', 'unknown')
            event_types[et] = event_types.get(et, 0) + 1
        
        for et, count in sorted(event_types.items())[:5]:
            print(f"   - {et}: {count}")
        
        # Extraire métriques clés
        if name == 'affordance':
            discoveries = [e for e in events if e.get('event_type') == 'affordances_discovered']
            if discoveries:
                last = discoveries[-1]['data']
                print(f"   → Affordances trouvées: {last.get('affordances_found', 0)}")
                print(f"   → Top plausibility: {last.get('top_plausibility', 0):.3f}")
        
        elif name == 'reputation':
            attempts = [e for e in events if e.get('event_type') == 'action_attempt_recorded']
            if attempts:
                successes = sum(1 for e in attempts if e['data'].get('success'))
                print(f"   → Tentatives: {len(attempts)}")
                print(f"   → Succès: {successes} ({successes/len(attempts)*100:.1f}%)")
        
        elif name == 'budget':
            consumed = [e for e in events if e.get('event_type') == 'budget_consumed']
            if consumed:
                total = sum(e['data'].get('amount', 0) for e in consumed)
                print(f"   → Budget consommé: {total:.1f}")
        
        elif name == 'stagnation':
            detections = [e for e in events if e.get('event_type') == 'stagnation_detected']
            if detections:
                last = detections[-1]['data']
                print(f"   → Stagnations: {len(detections)}")
                print(f"   → Loop ratio: {last.get('loop_ratio', 0):.3f}")
                print(f"   → Should abandon: {last.get('should_abandon', False)}")


def test_real_arc_puzzles():
    """Test sur vrais puzzles ARC-AGI-3."""
    print("="*80)
    print("TEST V38: DATASET ARC-AGI-3 RÉEL")
    print("="*80)
    print()
    
    # Charger dataset
    dataset_path = Path(__file__).parent.parent / "arc-agi_training_challenges.json"
    
    if not dataset_path.exists():
        print(f"❌ Dataset non trouvé: {dataset_path}")
        return False
    
    puzzles = load_real_arc_dataset(str(dataset_path), max_puzzles=10)
    
    # Initialiser solver
    log_dir = Path("/tmp/magen_v38_real")
    log_dir.mkdir(parents=True, exist_ok=True)
    
    solver = ARCSolverAdapter(
        max_steps=50,  # Limité pour tests rapides
        forensic_log_path=str(log_dir / "solver.jsonl")
    )
    
    print("🚀 Solver V38 initialisé")
    print(f"   Max steps: 50")
    print(f"   Logs: {log_dir}")
    print()
    
    # Tester puzzles
    results = {
        'total': len(puzzles),
        'solved': 0,
        'failed': 0,
        'puzzles': []
    }
    
    print("="*80)
    print("EXÉCUTION TESTS")
    print("="*80)
    print()
    
    for i, puzzle_data in enumerate(puzzles, 1):
        puzzle_id = puzzle_data['puzzle_id']
        print(f"\n[{i}/{len(puzzles)}] Puzzle: {puzzle_id}")
        print(f"   Train examples: {len(puzzle_data['train_pairs'])}")
        print(f"   Test input shape: {puzzle_data['test_input'].shape}")
        print(f"   Test output shape: {puzzle_data['test_output'].shape}")
        
        # Créer puzzle
        puzzle = ARCPuzzle(
            puzzle_id=puzzle_id,
            train_pairs=puzzle_data['train_pairs'],
            test_input=puzzle_data['test_input'],
            test_output=puzzle_data['test_output']
        )
        
        # Résoudre
        start = time.time()
        try:
            metrics = solver.solve_puzzle(puzzle)
            duration = time.time() - start
            
            if metrics.success:
                results['solved'] += 1
                status = "✅ RÉSOLU"
            else:
                results['failed'] += 1
                status = "❌ ÉCHOUÉ"
            
            print(f"   {status} en {duration:.2f}s")
            print(f"   Steps: {metrics.total_steps}")
            print(f"   Coherence: {metrics.world_coherence:.3f}")
            print(f"   Regions: {metrics.regions_discovered}")
            print(f"   Loops: {metrics.loops_detected}")
            
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'success': metrics.success,
                'duration': duration,
                'steps': metrics.total_steps,
                'coherence': metrics.world_coherence
            })
            
            # Analyser logs après chaque puzzle
            analyze_forensic_logs(log_dir, puzzle_id)
            
        except Exception as e:
            print(f"   ❌ ERREUR: {e}")
            results['failed'] += 1
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'error': str(e)
            })
        
        print()
    
    # Résultats finaux
    print("="*80)
    print("RÉSULTATS FINAUX")
    print("="*80)
    print()
    print(f"Total: {results['total']}")
    print(f"✅ Résolus: {results['solved']} ({results['solved']/results['total']*100:.1f}%)")
    print(f"❌ Échoués: {results['failed']} ({results['failed']/results['total']*100:.1f}%)")
    print()
    
    # Stats globales
    if solver.reputation_system:
        stats = solver.reputation_system.get_statistics()
        print("C17 - Réputation:")
        print(f"   Actions: {stats['total_actions']}")
        print(f"   Tentatives: {stats['total_attempts']}")
        print(f"   Succès: {stats['global_success_rate']:.3f}")
        print()
    
    if solver.budget_allocator:
        stats = solver.budget_allocator.get_statistics()
        print("C18 - Budget:")
        print(f"   Total: {stats['total_budget']:.1f}")
        print(f"   Dépensé: {stats['spent_budget']:.1f}")
        print(f"   Utilisation: {stats['utilization_rate']:.1%}")
        print()
    
    # Sauvegarder résultats
    results_path = log_dir / "test_results.json"
    with open(results_path, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"📊 Résultats: {results_path}")
    print(f"📋 Logs: {log_dir}")
    print()
    
    return results['solved'] > 0


if __name__ == "__main__":
    success = test_real_arc_puzzles()
    sys.exit(0 if success else 1)

# Made with Bob
