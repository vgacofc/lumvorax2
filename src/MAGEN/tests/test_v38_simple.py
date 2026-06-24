#!/usr/bin/env python3
"""
Test V38 Simplifié: Architecture Cognitive
===========================================

Test minimal sur puzzles synthétiques pour valider:
- C0: AffordanceDiscoveryEngine
- C17: ActionReputationSystem  
- C18: DynamicBudgetAllocator
- C19: TrajectoryStagnationDetector

LEÇON-73.1: Forensic logging activé
"""

import json
import sys
import time
from pathlib import Path
from typing import List, Tuple

import numpy as np

# Ajouter src au path
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.arc_solver_adapter import ARCSolverAdapter, ARCPuzzle


def create_synthetic_puzzle(puzzle_id: str) -> ARCPuzzle:
    """Crée puzzle synthétique simple."""
    # Puzzle simple: copier input → output
    train_input = np.array([[1, 2], [3, 4]])
    train_output = np.array([[1, 2], [3, 4]])
    
    test_input = np.array([[5, 6], [7, 8]])
    test_output = np.array([[5, 6], [7, 8]])
    
    return ARCPuzzle(
        puzzle_id=puzzle_id,
        train_pairs=[(train_input, train_output)],
        test_input=test_input,
        test_output=test_output
    )


def test_v38_cognitive_modules():
    """Test modules cognitifs V38."""
    print("=" * 80)
    print("TEST V38: ARCHITECTURE COGNITIVE (VERSION SIMPLIFIÉE)")
    print("=" * 80)
    print()
    
    # 1. Créer puzzles synthétiques
    puzzles = [
        create_synthetic_puzzle(f"synthetic_{i:03d}")
        for i in range(5)
    ]
    print(f"✅ {len(puzzles)} puzzles synthétiques créés\n")
    
    # 2. Initialiser solver V38
    log_dir = Path("/tmp/magen_v38_simple")
    log_dir.mkdir(parents=True, exist_ok=True)
    
    solver = ARCSolverAdapter(
        max_steps=100,
        forensic_log_path=str(log_dir / "solver.jsonl")
    )
    
    print("🚀 Solver V38 initialisé")
    print(f"   Forensic: {solver.forensic is not None}")
    print(f"   Métacognition: {solver.metacog is not None}")
    print(f"   World Model: {solver.world_model is not None}")
    print(f"   C0 Affordance: {solver.affordance_engine is not None}")
    print(f"   C17 Reputation: {solver.reputation_system is not None}")
    print(f"   C18 Budget: {solver.budget_allocator is not None}")
    print(f"   C19 Stagnation: {solver.stagnation_detector is not None}")
    print()
    
    # 3. Tester résolution
    results = {
        'total': len(puzzles),
        'solved': 0,
        'failed': 0,
        'puzzles': []
    }
    
    print("=" * 80)
    print("EXÉCUTION TESTS")
    print("=" * 80)
    print()
    
    for i, puzzle in enumerate(puzzles, 1):
        print(f"[{i}/{len(puzzles)}] Test: {puzzle.puzzle_id}")
        
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
            
            print(f"   {status} en {duration:.3f}s")
            print(f"   Steps: {metrics.total_steps}")
            print(f"   Coherence: {metrics.world_coherence:.3f}")
            print(f"   Regions: {metrics.regions_discovered}")
            print(f"   Loops: {metrics.loops_detected}")
            
            results['puzzles'].append({
                'puzzle_id': puzzle.puzzle_id,
                'success': metrics.success,
                'duration': duration,
                'steps': metrics.total_steps
            })
            
        except Exception as e:
            print(f"   ❌ ERREUR: {e}")
            results['failed'] += 1
            results['puzzles'].append({
                'puzzle_id': puzzle.puzzle_id,
                'error': str(e)
            })
        
        print()
    
    # 4. Résultats
    print("=" * 80)
    print("RÉSULTATS FINAUX")
    print("=" * 80)
    print()
    print(f"Total: {results['total']}")
    print(f"✅ Résolus: {results['solved']} ({results['solved']/results['total']*100:.1f}%)")
    print(f"❌ Échoués: {results['failed']} ({results['failed']/results['total']*100:.1f}%)")
    print()
    
    # 5. Stats modules cognitifs
    print("=" * 80)
    print("STATISTIQUES MODULES COGNITIFS")
    print("=" * 80)
    print()
    
    if solver.reputation_system:
        stats = solver.reputation_system.get_statistics()
        print("C17 - ActionReputationSystem:")
        print(f"   Actions: {stats['total_actions']}")
        print(f"   Tentatives: {stats['total_attempts']}")
        print(f"   Succès: {stats['global_success_rate']:.3f}")
        print()
    
    if solver.budget_allocator:
        stats = solver.budget_allocator.get_statistics()
        print("C18 - DynamicBudgetAllocator:")
        print(f"   Budget total: {stats['total_budget']:.1f}")
        print(f"   Dépensé: {stats['spent_budget']:.1f}")
        print(f"   Restant: {stats['remaining_budget']:.1f}")
        print(f"   Utilisation: {stats['utilization_rate']:.1%}")
        print()
    
    if solver.stagnation_detector:
        stats = solver.stagnation_detector.get_statistics()
        print("C19 - TrajectoryStagnationDetector:")
        print(f"   Trajectoires: {stats.get('total_trajectories', 0)}")
        print(f"   Stagnations: {stats.get('stagnations_detected', 0)}")
        print(f"   Taux: {stats.get('stagnation_rate', 0.0):.1%}")
        print()
    
    if solver.affordance_engine:
        stats = solver.affordance_engine.get_statistics()
        print("C0 - AffordanceDiscoveryEngine:")
        print(f"   Découvertes: {stats.get('total_discoveries', 0)}")
        print(f"   Affordances: {stats.get('affordances_found', 0)}")
        print(f"   Filtrées: {stats.get('actions_filtered', 0)}")
        print(f"   Réduction: {stats.get('reduction_rate', 0.0):.1%}")
        print()
    
    # 6. Analyser logs forensic
    print("=" * 80)
    print("ANALYSE LOGS FORENSIC")
    print("=" * 80)
    print()
    
    log_files = list(log_dir.glob("*.jsonl"))
    print(f"📋 {len(log_files)} fichiers logs trouvés:")
    
    for log_file in sorted(log_files):
        print(f"\n📄 {log_file.name}")
        
        try:
            with open(log_file, 'r') as f:
                events = [json.loads(line) for line in f if line.strip()]
            
            print(f"   Événements: {len(events)}")
            
            # Compter types d'événements
            event_types = {}
            for event in events:
                event_type = event.get('event_type', 'unknown')
                event_types[event_type] = event_types.get(event_type, 0) + 1
            
            print("   Types:")
            for event_type, count in sorted(event_types.items()):
                print(f"      - {event_type}: {count}")
            
        except Exception as e:
            print(f"   ⚠️  Erreur lecture: {e}")
    
    print()
    print(f"📊 Logs sauvegardés dans: {log_dir}")
    
    return results['solved'] > 0


if __name__ == "__main__":
    success = test_v38_cognitive_modules()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    success = test_v38_cognitive_modules()
    sys.exit(0 if success else 1)

# Made with Bob
