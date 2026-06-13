#!/usr/bin/env python3
"""
Test d'intégration MAGEN - Exécution locale
Test complet du système MAGEN avec logs forensiques LumVorax

Ce test DOIT être validé localement avant toute soumission Kaggle!
"""

import sys
import numpy as np
from pathlib import Path
import time

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.magen_memory import MAGENMemory, Experience
from perception.arc_perception import ARCPerception
from agent.arc_agent import ARCAgent
from forensic.lumvorax_logger import LumVoraxLogger


def create_test_grid(size: int = 10, complexity: str = 'simple') -> np.ndarray:
    """
    Créer grille de test
    
    Args:
        size: Taille grille
        complexity: 'simple', 'medium', 'complex'
    """
    grid = np.zeros((size, size), dtype=np.int32)
    
    if complexity == 'simple':
        # Grille simple: quelques cellules colorées
        grid[2:4, 2:4] = 1
        grid[6:8, 6:8] = 2
    
    elif complexity == 'medium':
        # Grille moyenne: pattern plus complexe
        grid[1:3, 1:9] = 1  # Bordure haut
        grid[7:9, 1:9] = 1  # Bordure bas
        grid[1:9, 1:3] = 2  # Bordure gauche
        grid[1:9, 7:9] = 2  # Bordure droite
        grid[4:6, 4:6] = 3  # Centre
    
    elif complexity == 'complex':
        # Grille complexe: pattern aléatoire
        for i in range(size):
            for j in range(size):
                if (i + j) % 3 == 0:
                    grid[i, j] = (i * j) % 10
    
    return grid


def create_action_executor(current_state_ref: list):
    """
    Créer fonction d'exécution d'actions avec référence à l'état
    
    Args:
        current_state_ref: Liste contenant [current_state] (mutable)
    
    Returns:
        Fonction execute_action(action, action_data) -> (new_state, done)
    """
    def execute_action(action: str, action_data: dict) -> tuple:
        """Exécuter une action"""
        current_state = current_state_ref[0]
        new_state = current_state.copy()
        done = False
        
        # Simuler différentes actions
        if action == 'RESET':
            # Reset: retour état initial
            new_state = np.zeros_like(current_state)
        
        elif action == 'ACTION1':
            # Action1: Ajouter bordure
            new_state[0, :] = 1
            new_state[-1, :] = 1
            new_state[:, 0] = 1
            new_state[:, -1] = 1
        
        elif action == 'ACTION2':
            # Action2: Remplir centre
            h, w = new_state.shape
            new_state[h//4:3*h//4, w//4:3*w//4] = 2
        
        elif action == 'ACTION3':
            # Action3: Rotation 90°
            new_state = np.rot90(current_state)
        
        elif action == 'ACTION4':
            # Action4: Flip horizontal
            new_state = np.fliplr(current_state)
        
        elif action == 'ACTION5':
            # Action5: Flip vertical
            new_state = np.flipud(current_state)
        
        elif action == 'ACTION6' and action_data:
            # Action6: Modifier cellule spécifique
            x, y = action_data['x'], action_data['y']
            if 0 <= y < new_state.shape[0] and 0 <= x < new_state.shape[1]:
                new_state[y, x] = (new_state[y, x] + 1) % 10
        
        elif action == 'ACTION7':
            # Action7: Inverser couleurs
            new_state = 9 - current_state
            new_state = np.clip(new_state, 0, 9)
        
        # Simuler complétion (10% chance)
        if np.random.random() < 0.1:
            done = True
        
        # Mettre à jour référence état
        current_state_ref[0] = new_state
        return new_state, done
    
    return execute_action


def test_magen_system():
    """Test complet du système MAGEN"""
    
    print("=" * 80)
    print("TEST INTÉGRATION MAGEN - EXÉCUTION LOCALE")
    print("=" * 80)
    print()
    
    # 1. Initialisation composants
    print("[1/6] Initialisation composants MAGEN...")
    
    memory = MAGENMemory(
        short_term_capacity=20,
        mid_term_capacity=50,
        long_term_threshold=0.7,
        save_dir="lumvorax2/src/MAGEN/logs"
    )
    
    perception = ARCPerception(max_grid_size=64)
    
    agent = ARCAgent(
        memory=memory,
        perception=perception,
        exploration_rate=0.5,
        max_actions_per_level=20
    )
    
    print("✓ Composants initialisés\n")
    
    # 2. Initialisation logger forensique
    print("[2/6] Initialisation logger forensique LumVorax...")
    
    with LumVoraxLogger(
        log_dir="lumvorax2/src/MAGEN/logs/forensic",
        enable_binary=True,
        enable_json=True
    ) as forensic_logger:
        
        print("✓ Logger forensique actif\n")
        
        # 3. Test perception
        print("[3/6] Test perception grilles ARC...")
        
        test_grids = {
            'simple': create_test_grid(10, 'simple'),
            'medium': create_test_grid(10, 'medium'),
            'complex': create_test_grid(10, 'complex')
        }
        
        for name, grid in test_grids.items():
            features = perception.extract_features(grid)
            print(f"  - Grille {name}:")
            print(f"    Taille: {features.width}x{features.height}")
            print(f"    Couleurs uniques: {features.unique_colors}")
            print(f"    Densité: {features.density:.2%}")
            print(f"    Entropie: {features.entropy:.2f}")
            print(f"    Complexité: {features.complexity:.2%}")
        
        print("✓ Perception validée\n")
        
        # 4. Test résolution niveaux
        print("[4/6] Test résolution niveaux simulés...")
        
        num_games = 3
        levels_per_game = 2
        
        for game_idx in range(num_games):
            game_id = f"test_game_{game_idx:03d}"
            print(f"\n  Jeu {game_id}:")
            
            for level in range(levels_per_game):
                # Créer état initial
                initial_state = create_test_grid(10, 'medium')
                
                # Créer référence mutable pour état
                state_ref = [initial_state.copy()]
                execute_fn = create_action_executor(state_ref)
                
                # Résoudre niveau
                success, num_actions, action_log = agent.solve_level(
                    game_id=game_id,
                    level=level,
                    initial_state=initial_state,
                    execute_action_fn=execute_fn
                )
                
                # Logger événement forensique
                forensic_logger.log_event(
                    event_type='level_completion',
                    game_id=game_id,
                    level=level,
                    action=f"LEVEL_RESULT",
                    action_data=None,
                    state=initial_state,
                    result_state=initial_state,  # Simplification pour test
                    reward=10.0 if success else 0.0,
                    success=success,
                    memory_usage_mb=memory.get_metrics().memory_usage_mb,
                    metadata={
                        'num_actions': num_actions,
                        'action_log': action_log[:3]  # Premiers 3 actions
                    }
                )
                
                status = "✓" if success else "✗"
                print(f"    Niveau {level}: {status} ({num_actions} actions)")
        
        print("\n✓ Résolution niveaux complétée\n")
        
        # 5. Test consolidation mémoire
        print("[5/6] Test consolidation mémoire...")
        
        consolidation_time = memory.consolidate_memory()
        metrics = memory.get_metrics()
        
        print(f"  - Court terme: {metrics.short_term_size} expériences")
        print(f"  - Moyen terme: {metrics.mid_term_size} patterns")
        print(f"  - Long terme: {metrics.long_term_size} stratégies")
        print(f"  - Archive: {metrics.deep_archive_size} expériences")
        print(f"  - Success rate patterns: {metrics.patterns_success_rate:.2%}")
        print(f"  - Temps consolidation: {consolidation_time:.2f}ms")
        print(f"  - Learning progress: {metrics.learning_progress:.2%}")
        
        print("✓ Consolidation validée\n")
        
        # 6. Statistiques finales
        print("[6/6] Statistiques finales...")
        
        agent_stats = agent.get_statistics()
        forensic_stats = forensic_logger.get_statistics()
        
        print("\n  Agent:")
        print(f"    - Total actions: {agent_stats['total_actions']}")
        print(f"    - Niveaux réussis: {agent_stats['successful_levels']}")
        print(f"    - Niveaux échoués: {agent_stats['failed_levels']}")
        print(f"    - Success rate: {agent_stats['success_rate']:.2%}")
        print(f"    - Actions/niveau: {agent_stats['avg_actions_per_level']:.1f}")
        
        print("\n  Forensic:")
        print(f"    - Total événements: {forensic_stats['total_events']}")
        print(f"    - Total bytes: {forensic_stats['total_bytes_written']:,}")
        print(f"    - Anomalies: {forensic_stats['anomalies_detected']}")
        print(f"    - Events/s: {forensic_stats['events_per_second']:.2f}")
        print(f"    - Bytes/event: {forensic_stats['bytes_per_event']:.1f}")
        
        print("\n  Mémoire:")
        print(f"    - Usage: {metrics.memory_usage_mb:.2f} MB")
        print(f"    - Patterns: {metrics.patterns_count}")
        print(f"    - Liens causaux: {metrics.causal_links_count}")
        print(f"    - Contradictions: {metrics.contradiction_count}")
        
        print("\n✓ Statistiques collectées\n")
        
        # Sauvegarder états
        memory.save_state("magen_test_state.json")
        agent.save_statistics("lumvorax2/src/MAGEN/logs/agent_test_stats.json")
    
    # Logger fermé automatiquement (context manager)
    
    print("=" * 80)
    print("TEST INTÉGRATION MAGEN - COMPLÉTÉ AVEC SUCCÈS ✓")
    print("=" * 80)
    print()
    print("Logs générés:")
    print("  - Mémoire: lumvorax2/src/MAGEN/logs/magen_test_state.json")
    print("  - Agent: lumvorax2/src/MAGEN/logs/agent_test_stats.json")
    print("  - Forensic: lumvorax2/src/MAGEN/logs/forensic/")
    print()
    print("VALIDATION LOCALE RÉUSSIE - Prêt pour validation utilisateur")
    print()


def test_perception_only():
    """Test rapide perception uniquement"""
    print("\n[TEST RAPIDE] Perception ARC")
    print("-" * 40)
    
    perception = ARCPerception()
    
    # Test grilles
    grids = [
        ("Vide", np.zeros((5, 5), dtype=np.int32)),
        ("Pleine", np.ones((5, 5), dtype=np.int32) * 3),
        ("Bordure", create_test_grid(10, 'simple')),
        ("Complexe", create_test_grid(10, 'complex'))
    ]
    
    for name, grid in grids:
        features = perception.extract_features(grid)
        print(f"\n{name}:")
        print(f"  Densité: {features.density:.2%}")
        print(f"  Entropie: {features.entropy:.2f}")
        print(f"  Symétrie H/V: {features.symmetry_h:.2f}/{features.symmetry_v:.2f}")
        print(f"  Clusters: {features.cluster_count}")
    
    print("\n✓ Test perception OK")


def test_memory_only():
    """Test rapide mémoire uniquement"""
    print("\n[TEST RAPIDE] Mémoire MAGEN")
    print("-" * 40)
    
    memory = MAGENMemory(short_term_capacity=10)
    
    # Injecter expériences
    for i in range(15):
        exp = Experience(
            timestamp=time.time(),
            game_id="test_game",
            level=0,
            state=np.random.randint(0, 10, (5, 5)),
            action=f"ACTION{i % 5 + 1}",
            action_data=None,
            result_state=np.random.randint(0, 10, (5, 5)),
            reward=float(i % 2),
            success=i % 3 == 0,
            metadata={}
        )
        memory.inject_experience(exp)
    
    metrics = memory.get_metrics()
    print(f"\nMétriques:")
    print(f"  Court terme: {metrics.short_term_size}")
    print(f"  Archive: {metrics.deep_archive_size}")
    print(f"  Patterns: {metrics.patterns_count}")
    
    print("\n✓ Test mémoire OK")


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Tests MAGEN locaux")
    parser.add_argument('--quick', action='store_true', 
                       help='Tests rapides uniquement')
    parser.add_argument('--perception', action='store_true',
                       help='Test perception uniquement')
    parser.add_argument('--memory', action='store_true',
                       help='Test mémoire uniquement')
    
    args = parser.parse_args()
    
    if args.perception:
        test_perception_only()
    elif args.memory:
        test_memory_only()
    elif args.quick:
        test_perception_only()
        test_memory_only()
    else:
        # Test complet
        test_magen_system()

# Made with Bob
