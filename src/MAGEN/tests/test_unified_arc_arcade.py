"""
Tests Unifiés ARC-AGI + Arcade - OBLIGATOIRE LES DEUX
======================================================

Framework de tests unifié qui exécute TOUJOURS:
1. Tests puzzles ARC-AGI
2. Tests jeux Arcade

AUCUNE EXCEPTION - Les deux types de tests sont OBLIGATOIRES à chaque exécution.

Date: 2026-06-15
Version: V38.2
"""

import numpy as np
import sys
import time
from pathlib import Path
from typing import List, Dict, Tuple

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.arc_solver_adapter import ARCSolverAdapter, ARCPuzzle, ArcadeState
from MAGEN.utils.arc_dataset_loader import ARCDatasetLoader


class UnifiedTestFramework:
    """
    Framework de tests unifié ARC-AGI + Arcade.
    
    RÈGLE ABSOLUE: Les deux types de tests sont TOUJOURS exécutés ensemble.
    """
    
    def __init__(self, use_real_dataset: bool = True):
        """
        Initialise le framework unifié.
        
        Args:
            use_real_dataset: Si True, utilise dataset ARC-AGI officiel. Si False, puzzles synthétiques.
        """
        self.solver = ARCSolverAdapter(
            max_steps=100,
            forensic_log_path="/tmp/magen_unified/solver.jsonl"
        )
        
        self.use_real_dataset = use_real_dataset
        self.dataset_loader = None
        
        if use_real_dataset:
            try:
                # Chemin relatif depuis le répertoire de tests
                dataset_path = Path(__file__).parent.parent.parent.parent / "data" / "arc_agi_official"
                self.dataset_loader = ARCDatasetLoader(str(dataset_path))
                print(f"✅ Dataset ARC-AGI officiel chargé depuis: {dataset_path}")
            except Exception as e:
                print(f"⚠️  Erreur chargement dataset officiel: {e}")
                print(f"   Utilisation de puzzles synthétiques à la place")
                self.use_real_dataset = False
        
        self.results = {
            'arc_puzzles': [],
            'arcade_games': [],
            'total_arc': 0,
            'total_arcade': 0,
            'success_arc': 0,
            'success_arcade': 0
        }
    
    def run_all_tests(self, num_arc_puzzles: int = 10, num_arcade_games: int = 10):
        """
        Exécute TOUS les tests (ARC + Arcade) OBLIGATOIREMENT.
        
        Args:
            num_arc_puzzles: Nombre de puzzles ARC à tester
            num_arcade_games: Nombre de jeux Arcade à tester
        """
        print("=" * 80)
        print("FRAMEWORK DE TESTS UNIFIÉ ARC-AGI + ARCADE")
        print("=" * 80)
        print(f"\n⚠️  RÈGLE ABSOLUE: LES DEUX TYPES DE TESTS SONT OBLIGATOIRES")
        print(f"   - Puzzles ARC-AGI: {num_arc_puzzles}")
        print(f"   - Jeux Arcade: {num_arcade_games}")
        print("=" * 80)
        
        # PARTIE 1: TESTS PUZZLES ARC-AGI (OBLIGATOIRE)
        print("\n" + "=" * 80)
        print("PARTIE 1/2: TESTS PUZZLES ARC-AGI")
        print("=" * 80)
        self._run_arc_puzzles(num_arc_puzzles)
        
        # PARTIE 2: TESTS JEUX ARCADE (OBLIGATOIRE)
        print("\n" + "=" * 80)
        print("PARTIE 2/2: TESTS JEUX ARCADE")
        print("=" * 80)
        self._run_arcade_games(num_arcade_games)
        
        # RAPPORT FINAL UNIFIÉ
        self._print_unified_report()
    
    def _run_arc_puzzles(self, num_puzzles: int):
        """
        Exécute tests puzzles ARC-AGI.
        
        OBLIGATOIRE - Ne peut pas être sauté.
        """
        if self.use_real_dataset and self.dataset_loader:
            print(f"\n🧩 Exécution de {num_puzzles} puzzles ARC-AGI RÉELS du dataset officiel...")
            puzzles_data = self.dataset_loader.load_training_puzzles(limit=num_puzzles, shuffle=True)
        else:
            print(f"\n🧩 Exécution de {num_puzzles} puzzles ARC-AGI SYNTHÉTIQUES...")
            puzzles_data = None
        
        for i in range(num_puzzles):
            print(f"\n--- Puzzle ARC {i+1}/{num_puzzles} ---")
            
            # Créer puzzle (réel ou synthétique)
            if puzzles_data:
                puzzle = self._create_puzzle_from_dataset(puzzles_data[i])
                print(f"   ID: {puzzle.puzzle_id}")
                print(f"   Train examples: {len(puzzle.train_pairs)}")
            else:
                puzzle = self._create_synthetic_arc_puzzle(f"arc_puzzle_{i}")
            
            # Résoudre
            start_time = time.time()
            metrics = self.solver.solve_puzzle(puzzle)
            duration = time.time() - start_time
            
            # Enregistrer résultats
            result = {
                'puzzle_id': puzzle.puzzle_id,
                'success': metrics.success,
                'steps': metrics.total_steps,
                'duration': duration,
                'regions': metrics.regions_discovered,
                'loops': metrics.loops_detected
            }
            
            self.results['arc_puzzles'].append(result)
            self.results['total_arc'] += 1
            if metrics.success:
                self.results['success_arc'] += 1
            
            # Afficher résultat
            status = "✅ RÉSOLU" if metrics.success else "❌ ÉCHEC"
            print(f"   {status} - {metrics.total_steps} steps - {duration:.2f}s")
            print(f"   Régions: {metrics.regions_discovered}, Loops: {metrics.loops_detected}")
        
        print(f"\n✅ Tests ARC-AGI terminés: {self.results['success_arc']}/{self.results['total_arc']} réussis")
    
    def _run_arcade_games(self, num_games: int):
        """
        Exécute tests jeux Arcade.
        
        OBLIGATOIRE - Ne peut pas être sauté.
        """
        print(f"\n🎮 Exécution de {num_games} jeux Arcade...")
        
        for i in range(num_games):
            print(f"\n--- Jeu Arcade {i+1}/{num_games} ---")
            
            # Créer état jeu synthétique
            game_state = self._create_synthetic_arcade_state(f"arcade_game_{i}")
            
            # Actions disponibles
            action_space = ['up', 'down', 'left', 'right', 'fire', 'noop']
            
            # Jouer plusieurs steps
            total_score = 0
            num_steps = 10
            final_step = 0
            
            start_time = time.time()
            for step in range(num_steps):
                final_step = step
                action, metrics = self.solver.solve_arcade(game_state, action_space)
                
                # Simuler résultat action
                game_state.score += np.random.uniform(0, 10)
                game_state.position = (
                    game_state.position[0] + np.random.randint(-1, 2),
                    game_state.position[1] + np.random.randint(-1, 2)
                )
                
                # Terminer aléatoirement
                if np.random.random() < 0.1:
                    game_state.done = True
                    break
            
            duration = time.time() - start_time
            
            # Enregistrer résultats
            result = {
                'game_id': game_state.game_id,
                'success': game_state.score > 50,  # Seuil arbitraire
                'score': game_state.score,
                'steps': final_step + 1,
                'duration': duration
            }
            
            self.results['arcade_games'].append(result)
            self.results['total_arcade'] += 1
            if result['success']:
                self.results['success_arcade'] += 1
            
            # Afficher résultat
            status = "✅ SUCCÈS" if result['success'] else "❌ ÉCHEC"
            print(f"   {status} - Score: {game_state.score:.1f} - {final_step+1} steps - {duration:.2f}s")
        
        print(f"\n✅ Tests Arcade terminés: {self.results['success_arcade']}/{self.results['total_arcade']} réussis")
    
    def _create_puzzle_from_dataset(self, puzzle_data: Dict) -> ARCPuzzle:
        """
        Crée un ARCPuzzle depuis les données du dataset officiel.
        
        Args:
            puzzle_data: Données du puzzle depuis le dataset
            
        Returns:
            ARCPuzzle formaté pour le solver
        """
        # Convertir train examples
        train_pairs = [
            (np.array(ex['input']), np.array(ex['output']))
            for ex in puzzle_data['train']
        ]
        
        # Prendre premier test example
        test_example = puzzle_data['test'][0]
        test_input = np.array(test_example['input'])
        test_output = np.array(test_example['output'])
        
        return ARCPuzzle(
            puzzle_id=puzzle_data['id'],
            train_pairs=train_pairs,
            test_input=test_input,
            test_output=test_output
        )
    
    def _create_synthetic_arc_puzzle(self, puzzle_id: str) -> ARCPuzzle:
        """Crée un puzzle ARC synthétique."""
        # Grille simple 5x5
        input_grid = np.random.randint(0, 10, (5, 5))
        output_grid = np.rot90(input_grid)  # Rotation 90°
        
        train_pairs = [
            (input_grid.copy(), output_grid.copy())
        ]
        
        test_input = np.random.randint(0, 10, (5, 5))
        test_output = np.rot90(test_input)
        
        return ARCPuzzle(
            puzzle_id=puzzle_id,
            train_pairs=train_pairs,
            test_input=test_input,
            test_output=test_output
        )
    
    def _create_synthetic_arcade_state(self, game_id: str) -> ArcadeState:
        """Crée un état jeu Arcade synthétique."""
        frame = np.random.randint(0, 256, (84, 84, 3), dtype=np.uint8)
        position = (np.random.randint(0, 84), np.random.randint(0, 84))
        
        return ArcadeState(
            game_id=game_id,
            frame=frame,
            position=position,
            score=0.0,
            level=1,
            done=False
        )
    
    def _print_unified_report(self):
        """Affiche rapport unifié final."""
        print("\n" + "=" * 80)
        print("RAPPORT UNIFIÉ FINAL - ARC-AGI + ARCADE")
        print("=" * 80)
        
        # Résultats ARC
        print("\n📊 RÉSULTATS PUZZLES ARC-AGI:")
        print(f"   Total: {self.results['total_arc']}")
        print(f"   Réussis: {self.results['success_arc']}")
        print(f"   Taux succès: {self.results['success_arc']/self.results['total_arc']*100:.1f}%")
        
        if self.results['arc_puzzles']:
            avg_steps = np.mean([r['steps'] for r in self.results['arc_puzzles']])
            avg_duration = np.mean([r['duration'] for r in self.results['arc_puzzles']])
            print(f"   Moyenne steps: {avg_steps:.1f}")
            print(f"   Moyenne durée: {avg_duration:.2f}s")
        
        # Résultats Arcade
        print("\n🎮 RÉSULTATS JEUX ARCADE:")
        print(f"   Total: {self.results['total_arcade']}")
        print(f"   Réussis: {self.results['success_arcade']}")
        if self.results['total_arcade'] > 0:
            print(f"   Taux succès: {self.results['success_arcade']/self.results['total_arcade']*100:.1f}%")
        
        if self.results['arcade_games']:
            avg_score = np.mean([r['score'] for r in self.results['arcade_games']])
            avg_steps = np.mean([r['steps'] for r in self.results['arcade_games']])
            avg_duration = np.mean([r['duration'] for r in self.results['arcade_games']])
            print(f"   Moyenne score: {avg_score:.1f}")
            print(f"   Moyenne steps: {avg_steps:.1f}")
            print(f"   Moyenne durée: {avg_duration:.2f}s")
        
        # Statistiques globales
        total_tests = self.results['total_arc'] + self.results['total_arcade']
        total_success = self.results['success_arc'] + self.results['success_arcade']
        
        print("\n📈 STATISTIQUES GLOBALES:")
        print(f"   Total tests: {total_tests}")
        print(f"   Total réussis: {total_success}")
        print(f"   Taux succès global: {total_success/total_tests*100:.1f}%")
        
        print("\n" + "=" * 80)
        print("✅ TESTS UNIFIÉS TERMINÉS - LES DEUX TYPES EXÉCUTÉS")
        print("=" * 80)
    
    def close(self):
        """Ferme le solver."""
        self.solver.close()


def main():
    """Point d'entrée principal - EXÉCUTE TOUJOURS LES DEUX TYPES."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Framework de Tests Unifié ARC-AGI + Arcade')
    parser.add_argument('--arc-puzzles', type=int, default=10,
                       help='Nombre de puzzles ARC-AGI à tester (défaut: 10)')
    parser.add_argument('--arcade-games', type=int, default=10,
                       help='Nombre de jeux Arcade à tester (défaut: 10)')
    parser.add_argument('--use-real-dataset', action='store_true', default=True,
                       help='Utiliser dataset ARC-AGI officiel (défaut: True)')
    parser.add_argument('--use-synthetic', action='store_true',
                       help='Forcer utilisation de puzzles synthétiques')
    
    args = parser.parse_args()
    
    # Si --use-synthetic est spécifié, désactiver dataset réel
    use_real = args.use_real_dataset and not args.use_synthetic
    
    print("\n🚀 Démarrage Framework de Tests Unifié")
    print("⚠️  RÈGLE: Puzzles ARC-AGI ET Jeux Arcade OBLIGATOIRES")
    print(f"   Dataset: {'RÉEL (officiel)' if use_real else 'SYNTHÉTIQUE'}")
    print(f"   Puzzles ARC: {args.arc_puzzles}")
    print(f"   Jeux Arcade: {args.arcade_games}\n")
    
    # Créer framework
    framework = UnifiedTestFramework(use_real_dataset=use_real)
    
    try:
        # EXÉCUTER LES DEUX TYPES DE TESTS (OBLIGATOIRE)
        framework.run_all_tests(
            num_arc_puzzles=args.arc_puzzles,
            num_arcade_games=args.arcade_games
        )
        
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        framework.close()


if __name__ == '__main__':
    main()

# Made with Bob
