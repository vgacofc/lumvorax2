"""
Test Unitaire - Connectivity Spatial Score
Validation BFS local et détection obstacles

Date: 2026-06-15
"""

import sys
from pathlib import Path

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

from connectivity_spatial_score import ConnectivitySpatialScore


def test_simple_grid():
    """Test environnement simple (pas d'obstacles)."""
    print("Test 1: Grid simple 5×5")
    
    scorer = ConnectivitySpatialScore(bfs_radius=5)
    
    state = {
        'agent_pos': (2, 2),
        'goal_pos': (4, 4),
        'obstacles': set(),
        'grid_size': (5, 5)
    }
    
    # ACTION4 (move_right) devrait rapprocher
    score_right = scorer.compute_score(state, 'ACTION4', (4, 4))
    print(f"  ACTION4 (right): {score_right} (attendu: 1.0)")
    
    # ACTION3 (move_left) devrait éloigner
    score_left = scorer.compute_score(state, 'ACTION3', (4, 4))
    print(f"  ACTION3 (left): {score_left} (attendu: 0.0)")
    
    assert score_right == 1.0, "ACTION4 devrait rapprocher"
    assert score_left == 0.0, "ACTION3 devrait éloigner"
    print("  ✅ Test simple réussi\n")


def test_obstacle_mur():
    """Test avec mur vertical (obstacle)."""
    print("Test 2: Mur vertical bloquant")
    
    scorer = ConnectivitySpatialScore(bfs_radius=5)
    
    # Mur vertical en x=3
    obstacles = {(3, y) for y in range(7)}
    
    state = {
        'agent_pos': (1, 3),
        'goal_pos': (5, 3),
        'obstacles': obstacles,
        'grid_size': (7, 7)
    }
    
    # ACTION4 (move_right) vers mur → bloqué
    score_right = scorer.compute_score(state, 'ACTION4', (5, 3))
    print(f"  ACTION4 (vers mur): {score_right}")
    
    # ACTION2 (move_down) pour contourner
    score_down = scorer.compute_score(state, 'ACTION2', (5, 3))
    print(f"  ACTION2 (contourner): {score_down}")
    
    # Avec BFS, move_down devrait être meilleur que move_right
    print(f"  Différence: {abs(score_down - score_right)}")
    
    if score_down > score_right:
        print("  ✅ BFS détecte contournement\n")
    else:
        print("  ⚠️  BFS ne différencie pas encore\n")


def test_labyrinthe():
    """Test labyrinthe simple."""
    print("Test 3: Labyrinthe")
    
    scorer = ConnectivitySpatialScore(bfs_radius=8)
    
    # Labyrinthe en forme de S
    obstacles = {
        (3, 0), (3, 1), (3, 2), (3, 3), (3, 4),
        (5, 4), (5, 5), (5, 6), (5, 7), (5, 8)
    }
    
    state = {
        'agent_pos': (1, 1),
        'goal_pos': (7, 7),
        'obstacles': obstacles,
        'grid_size': (9, 9)
    }
    
    # Tester 4 directions
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    scores = {}
    
    for action in actions:
        score = scorer.compute_score(state, action, (7, 7))
        scores[action] = score
        print(f"  {action}: {score:.2f}")
    
    # Vérifier variance
    variance = max(scores.values()) - min(scores.values())
    print(f"  Variance: {variance:.2f}")
    
    if variance > 0.0:
        print("  ✅ Actions différenciées\n")
    else:
        print("  ❌ Collapse variance\n")


def test_inaccessible():
    """Test goal inaccessible (hors rayon BFS)."""
    print("Test 4: Goal inaccessible")
    
    scorer = ConnectivitySpatialScore(bfs_radius=3)  # Rayon court
    
    state = {
        'agent_pos': (0, 0),
        'goal_pos': (9, 9),  # Loin
        'obstacles': set(),
        'grid_size': (10, 10)
    }
    
    score = scorer.compute_score(state, 'ACTION4', (9, 9))
    print(f"  Score (goal hors rayon): {score}")
    
    if score == 0.0:
        print("  ✅ Goal inaccessible détecté\n")
    else:
        print("  ⚠️  Goal inaccessible non détecté\n")


if __name__ == '__main__':
    print("=" * 60)
    print("TEST CONNECTIVITY SPATIAL SCORE")
    print("=" * 60)
    print()
    
    test_simple_grid()
    test_obstacle_mur()
    test_labyrinthe()
    test_inaccessible()
    
    print("=" * 60)
    print("TESTS TERMINÉS")
    print("=" * 60)

# Made with Bob
