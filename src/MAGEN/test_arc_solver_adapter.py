"""
Test ARC Solver Adapter V36.2
Test unitaire de l'intégration V35+V36

Tests:
1. Initialisation adaptateur
2. Solve puzzle simple
3. Métriques collectées
4. Forensic généré

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path
import numpy as np
from datetime import datetime

# Ajouter le chemin du module
sys.path.insert(0, str(Path(__file__).parent))

from core.arc_solver_adapter import (
    ARCSolverAdapter,
    ARCPuzzle,
    ArcadeState,
    SolverMetrics
)


def test_adapter_initialization():
    """Test 1: Initialisation adaptateur."""
    print("\n" + "="*60)
    print("TEST 1: Initialisation ARCSolverAdapter")
    print("="*60)
    
    adapter = ARCSolverAdapter(max_steps=100)
    
    assert adapter.max_steps == 100, "max_steps incorrect"
    assert adapter.stats['puzzles_solved'] == 0, "Stats initiales incorrectes"
    assert adapter.metacog is not None, "MetaCognitiveController non initialisé"
    assert adapter.world_model is not None, "WorldModelPersistent non initialisé"
    
    print("✅ Adaptateur initialisé")
    print(f"   - max_steps: {adapter.max_steps}")
    print(f"   - metacog: {'✓' if adapter.metacog else '✗'}")
    print(f"   - world_model: {'✓' if adapter.world_model else '✗'}")
    print(f"   - forensic: {'✓' if adapter.forensic else '✗'}")
    
    adapter.close()
    return True


def test_solve_simple_puzzle():
    """Test 2: Résolution puzzle simple."""
    print("\n" + "="*60)
    print("TEST 2: Résolution puzzle simple")
    print("="*60)
    
    adapter = ARCSolverAdapter(max_steps=50)
    
    # Créer puzzle simple (grille 5x5)
    train_input = np.array([
        [0, 0, 1, 0, 0],
        [0, 1, 1, 1, 0],
        [1, 1, 1, 1, 1],
        [0, 1, 1, 1, 0],
        [0, 0, 1, 0, 0]
    ])
    
    train_output = np.array([
        [1, 1, 1, 1, 1],
        [1, 1, 1, 1, 1],
        [1, 1, 1, 1, 1],
        [1, 1, 1, 1, 1],
        [1, 1, 1, 1, 1]
    ])
    
    test_input = np.array([
        [0, 1, 0, 1, 0],
        [1, 1, 1, 1, 1],
        [0, 1, 1, 1, 0],
        [1, 1, 1, 1, 1],
        [0, 1, 0, 1, 0]
    ])
    
    puzzle = ARCPuzzle(
        puzzle_id="test_001",
        train_pairs=[(train_input, train_output)],
        test_input=test_input
    )
    
    # Résoudre
    metrics = adapter.solve_puzzle(puzzle)
    
    assert metrics.puzzle_id == "test_001", "puzzle_id incorrect"
    assert metrics.total_steps > 0, "Aucun step exécuté"
    assert metrics.end_time > metrics.start_time, "Temps incorrect"
    
    print("✅ Puzzle résolu")
    print(f"   - puzzle_id: {metrics.puzzle_id}")
    print(f"   - steps: {metrics.total_steps}")
    print(f"   - régions: {metrics.regions_discovered}")
    print(f"   - loops: {metrics.loops_detected}")
    print(f"   - cohérence: {metrics.world_coherence:.2%}")
    print(f"   - durée: {metrics.end_time - metrics.start_time:.3f}s")
    
    adapter.close()
    return True


def test_metrics_collection():
    """Test 3: Collecte métriques."""
    print("\n" + "="*60)
    print("TEST 3: Collecte métriques")
    print("="*60)
    
    adapter = ARCSolverAdapter(max_steps=30)
    
    # Résoudre 2 puzzles
    for i in range(2):
        puzzle = ARCPuzzle(
            puzzle_id=f"test_{i:03d}",
            train_pairs=[(np.ones((3, 3)), np.zeros((3, 3)))],
            test_input=np.ones((3, 3))
        )
        adapter.solve_puzzle(puzzle)
    
    summary = adapter.get_metrics_summary()
    
    assert summary['total_puzzles'] == 2, "Nombre puzzles incorrect"
    assert summary['avg_steps'] > 0, "avg_steps incorrect"
    assert summary['avg_time'] > 0, "avg_time incorrect"
    
    print("✅ Métriques collectées")
    print(f"   - total_puzzles: {summary['total_puzzles']}")
    print(f"   - success_rate: {summary['success_rate']:.1%}")
    print(f"   - avg_steps: {summary['avg_steps']:.1f}")
    print(f"   - avg_time: {summary['avg_time']:.3f}s")
    print(f"   - total_loops: {summary['total_loops']}")
    
    adapter.close()
    return True


def test_forensic_logging():
    """Test 4: Logging forensic."""
    print("\n" + "="*60)
    print("TEST 4: Logging forensic")
    print("="*60)
    
    log_path = "logs/magen_v36/test_adapter.jsonl"
    adapter = ARCSolverAdapter(
        max_steps=20,
        forensic_log_path=log_path
    )
    
    # Résoudre puzzle
    puzzle = ARCPuzzle(
        puzzle_id="test_forensic",
        train_pairs=[(np.ones((2, 2)), np.zeros((2, 2)))],
        test_input=np.ones((2, 2))
    )
    adapter.solve_puzzle(puzzle)
    adapter.close()
    
    # Vérifier logs générés
    log_file = Path(log_path)
    assert log_file.exists(), "Fichier log non créé"
    
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    assert len(lines) > 0, "Aucun événement forensic"
    
    # Compter types événements
    events = {}
    for line in lines:
        import json
        event = json.loads(line)
        event_type = event.get('event_type', 'unknown')
        events[event_type] = events.get(event_type, 0) + 1
    
    print("✅ Forensic généré")
    print(f"   - fichier: {log_path}")
    print(f"   - événements: {len(lines)}")
    print(f"   - types:")
    for event_type, count in sorted(events.items()):
        print(f"     • {event_type}: {count}")
    
    return True


def main():
    """Exécute tous les tests."""
    print("\n" + "="*60)
    print("🧪 TESTS ARC SOLVER ADAPTER V36.2")
    print("="*60)
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    tests = [
        ("Initialisation", test_adapter_initialization),
        ("Solve puzzle", test_solve_simple_puzzle),
        ("Métriques", test_metrics_collection),
        ("Forensic", test_forensic_logging)
    ]
    
    results = []
    for name, test_func in tests:
        try:
            success = test_func()
            results.append((name, success))
        except Exception as e:
            print(f"\n❌ ÉCHEC: {name}")
            print(f"   Erreur: {str(e)}")
            import traceback
            traceback.print_exc()
            results.append((name, False))
    
    # Résumé
    print("\n" + "="*60)
    print("📊 RÉSUMÉ DES TESTS")
    print("="*60)
    
    for name, success in results:
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} - {name}")
    
    passed = sum(1 for _, s in results if s)
    total = len(results)
    
    print("\n" + "="*60)
    print(f"Résultat: {passed}/{total} tests réussis ({passed/total*100:.1f}%)")
    print("="*60)
    
    return passed == total


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)

# Made with Bob
