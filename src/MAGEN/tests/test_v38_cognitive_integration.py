#!/usr/bin/env python3
"""
Test V38: Intégration Architecture Cognitive (C0+C17+C18+C19)
=============================================================

Test sur VRAIS puzzles ARC-AGI-3 avec:
- C0: AffordanceDiscoveryEngine (filtrage pré-exploration)
- C17: ActionReputationSystem (mémoire valeur actions)
- C18: DynamicBudgetAllocator (allocation attentionnelle)
- C19: TrajectoryStagnationDetector (détection stagnation)

LEÇON-73.1: Forensic logging activé pour analyse cognitive
"""

import json
import sys
import time
from pathlib import Path
from typing import Dict, List

import numpy as np

# Ajouter src au path
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.arc_solver_adapter import ARCSolverAdapter
from MAGEN.core.magen_memory import MAGENMemory
from MAGEN.core.world_model_minimal import WorldModelMinimal
from MAGEN.core.action_reputation_system import ActionReputationSystem
from MAGEN.core.dynamic_budget_allocator import DynamicBudgetAllocator
from MAGEN.core.trajectory_stagnation_detector import TrajectoryStagnationDetector
from MAGEN.core.affordance_discovery_engine import AffordanceDiscoveryEngine
from MAGEN.forensic.forensic_logger import ForensicLogger


def load_arc_dataset(dataset_path: str, max_puzzles: int = 10) -> List[Dict]:
    """Charge dataset ARC-AGI-3 réel."""
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    puzzles = []
    for puzzle_id, puzzle_data in list(data.items())[:max_puzzles]:
        # Convertir en format attendu
        train_examples = []
        for example in puzzle_data.get('train', []):
            train_examples.append({
                'input': np.array(example['input']),
                'output': np.array(example['output'])
            })
        
        test_examples = []
        for example in puzzle_data.get('test', []):
            test_examples.append({
                'input': np.array(example['input']),
                'output': np.array(example['output'])
            })
        
        puzzles.append({
            'puzzle_id': puzzle_id,
            'train': train_examples,
            'test': test_examples
        })
    
    return puzzles


def test_v38_cognitive_architecture():
    """Test architecture cognitive V38 sur puzzles réels."""
    print("=" * 80)
    print("TEST V38: ARCHITECTURE COGNITIVE (C0+C17+C18+C19)")
    print("=" * 80)
    print()
    
    # 1. Charger dataset ARC-AGI-3 réel
    dataset_path = Path(__file__).parent.parent.parent.parent / "data" / "arc-agi_training_challenges.json"
    
    if not dataset_path.exists():
        print(f"❌ Dataset non trouvé: {dataset_path}")
        print("   Télécharger depuis: https://github.com/fchollet/ARC-AGI")
        return False
    
    print(f"📂 Chargement dataset: {dataset_path}")
    puzzles = load_arc_dataset(str(dataset_path), max_puzzles=10)
    print(f"✅ {len(puzzles)} puzzles chargés\n")
    
    # 2. Initialiser forensic logger
    forensic = ForensicLogger(
        log_dir="/tmp/magen_v38_test",
        session_id=f"v38_test_{int(time.time())}"
    )
    
    # 3. Initialiser modules cognitifs
    print("🧠 Initialisation modules cognitifs V38:")
    
    # C0: Affordance Discovery
    affordance_engine = AffordanceDiscoveryEngine(
        min_plausibility=0.3,
        forensic=forensic
    )
    print("   ✅ C0: AffordanceDiscoveryEngine")
    
    # C17: Action Reputation
    reputation_system = ActionReputationSystem(
        decay_rate=0.95,
        min_attempts=3,
        forensic=forensic
    )
    print("   ✅ C17: ActionReputationSystem")
    
    # C18: Dynamic Budget
    budget_allocator = DynamicBudgetAllocator(
        total_budget=1000.0,
        min_allocation=10.0,
        forensic=forensic
    )
    print("   ✅ C18: DynamicBudgetAllocator")
    
    # C19: Trajectory Stagnation
    stagnation_detector = TrajectoryStagnationDetector(
        window_size=20,
        variance_threshold=0.01,
        loop_threshold=0.7,
        forensic=forensic
    )
    print("   ✅ C19: TrajectoryStagnationDetector")
    print()
    
    # 4. Initialiser solver avec modules cognitifs
    memory = MAGENMemory(capacity=1000)
    world_model = WorldModelMinimal(forensic=forensic)
    
    solver = ARCSolverAdapter(
        memory=memory,
        world_model=world_model,
        forensic=forensic,
        affordance_engine=affordance_engine,
        reputation_system=reputation_system,
        budget_allocator=budget_allocator,
        stagnation_detector=stagnation_detector
    )
    
    print("🚀 Solver V38 initialisé avec architecture cognitive complète\n")
    
    # 5. Tester sur puzzles réels
    results = {
        'total_puzzles': len(puzzles),
        'solved': 0,
        'failed': 0,
        'puzzles': []
    }
    
    print("=" * 80)
    print("EXÉCUTION TESTS SUR PUZZLES RÉELS")
    print("=" * 80)
    print()
    
    for i, puzzle_data in enumerate(puzzles, 1):
        puzzle_id = puzzle_data['puzzle_id']
        print(f"[{i}/{len(puzzles)}] Test puzzle: {puzzle_id}")
        
        # Créer objet puzzle simplifié
        class SimplePuzzle:
            def __init__(self, data):
                self.puzzle_id = data['puzzle_id']
                self.train_examples = data['train']
                self.test_input = data['test'][0]['input']
                self.test_output = data['test'][0]['output']
        
        puzzle = SimplePuzzle(puzzle_data)
        
        # Résoudre
        start_time = time.time()
        try:
            metrics = solver.solve_puzzle(puzzle, max_steps=100)
            duration = time.time() - start_time
            
            success = metrics.success
            if success:
                results['solved'] += 1
                status = "✅ RÉSOLU"
            else:
                results['failed'] += 1
                status = "❌ ÉCHOUÉ"
            
            print(f"   {status} en {duration:.2f}s")
            print(f"   Steps: {metrics.total_steps}, Coherence: {metrics.world_coherence:.3f}")
            print(f"   Regions: {metrics.regions_discovered}, Loops: {metrics.loops_detected}")
            
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'success': success,
                'duration': duration,
                'steps': metrics.total_steps,
                'coherence': metrics.world_coherence,
                'regions': metrics.regions_discovered,
                'loops': metrics.loops_detected
            })
            
        except Exception as e:
            print(f"   ❌ ERREUR: {e}")
            results['failed'] += 1
            results['puzzles'].append({
                'puzzle_id': puzzle_id,
                'success': False,
                'error': str(e)
            })
        
        print()
    
    # 6. Statistiques finales
    print("=" * 80)
    print("RÉSULTATS FINAUX V38")
    print("=" * 80)
    print()
    print(f"Total puzzles testés: {results['total_puzzles']}")
    print(f"✅ Résolus: {results['solved']} ({results['solved']/results['total_puzzles']*100:.1f}%)")
    print(f"❌ Échoués: {results['failed']} ({results['failed']/results['total_puzzles']*100:.1f}%)")
    print()
    
    # 7. Statistiques modules cognitifs
    print("=" * 80)
    print("STATISTIQUES MODULES COGNITIFS")
    print("=" * 80)
    print()
    
    # C17: Réputation
    if reputation_system:
        rep_stats = reputation_system.get_statistics()
        print("C17 - ActionReputationSystem:")
        print(f"   Actions trackées: {rep_stats['total_actions']}")
        print(f"   Tentatives totales: {rep_stats['total_attempts']}")
        print(f"   Taux succès global: {rep_stats['global_success_rate']:.3f}")
        print()
    
    # C18: Budget
    if budget_allocator:
        budget_stats = budget_allocator.get_statistics()
        print("C18 - DynamicBudgetAllocator:")
        print(f"   Budget total: {budget_stats['total_budget']:.1f}")
        print(f"   Budget dépensé: {budget_stats['spent_budget']:.1f}")
        print(f"   Budget restant: {budget_stats['remaining_budget']:.1f}")
        print(f"   Utilisation: {budget_stats['utilization_rate']:.1%}")
        print()
    
    # C19: Stagnation
    if stagnation_detector:
        stag_stats = stagnation_detector.get_statistics()
        print("C19 - TrajectoryStagnationDetector:")
        print(f"   Trajectoires analysées: {stag_stats['total_trajectories']}")
        print(f"   Stagnations détectées: {stag_stats['stagnations_detected']}")
        print(f"   Taux stagnation: {stag_stats['stagnation_rate']:.1%}")
        print()
    
    # C0: Affordances
    if affordance_engine:
        aff_stats = affordance_engine.get_statistics()
        print("C0 - AffordanceDiscoveryEngine:")
        print(f"   Découvertes totales: {aff_stats['total_discoveries']}")
        print(f"   Affordances trouvées: {aff_stats['affordances_found']}")
        print(f"   Actions filtrées: {aff_stats['actions_filtered']}")
        print(f"   Taux réduction: {aff_stats['reduction_rate']:.1%}")
        print()
    
    # 8. Sauvegarder résultats
    results_path = Path("/tmp/magen_v38_test") / "test_results.json"
    results_path.parent.mkdir(parents=True, exist_ok=True)
    with open(results_path, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"📊 Résultats sauvegardés: {results_path}")
    print(f"📋 Logs forensic: {forensic.log_dir}")
    print()
    
    return results['solved'] > 0


if __name__ == "__main__":
    success = test_v38_cognitive_architecture()
    sys.exit(0 if success else 1)

# Made with Bob
