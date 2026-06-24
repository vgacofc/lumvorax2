"""
Tests sur VRAIS Jeux Arcade ARC-AGI-3
=====================================

Script pour tester MAGEN sur les jeux Arcade officiels de la compétition ARC-AGI-3.
Utilise l'API officielle pour accéder aux environnements réels.

Date: 2026-06-15
Version: V38.2
"""

import os
import sys
import json
import time
from pathlib import Path
from typing import List, Dict
from datetime import datetime

# Configuration API
API_KEY = "b6ebc9c5-a296-4578-be69-0cc16c9455d4"
os.environ['ARC_API_KEY'] = API_KEY

# Import arc-agi depuis venv
sys.path.insert(0, str(Path(__file__).parent.parent.parent / "venv_arc_agi" / "lib" / "python3.12" / "site-packages"))

try:
    import arc_agi
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
    print("✅ arc-agi imported successfully")
except ImportError as e:
    print(f"❌ Error importing arc-agi: {e}")
    print("Please run: cd lumvorax2 && ./venv_arc_agi/bin/pip install arc-agi")
    sys.exit(1)


class RealArcadeGameTester:
    """Testeur pour jeux Arcade ARC-AGI-3 réels"""
    
    def __init__(self, operation_mode=OperationMode.OFFLINE):
        """
        Initialise le testeur
        
        Args:
            operation_mode: Mode d'opération (OFFLINE, ONLINE, COMPETITION)
        """
        self.arc = Arcade(
            arc_api_key=API_KEY,
            operation_mode=operation_mode
        )
        self.results = []
        self.forensic_log_path = "/tmp/magen_arcade_real/forensic.jsonl"
        
        # Créer répertoire logs
        Path(self.forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
    
    def list_available_games(self) -> List[Dict]:
        """Liste tous les jeux Arcade disponibles"""
        print("\n🎮 Listing available Arcade games...")
        
        try:
            environments = self.arc.get_environments()
            
            print(f"\n✅ Found {len(environments)} games:")
            for i, env in enumerate(environments, 1):
                print(f"   {i}. {env.game_id}: {env.title}")
            
            return environments
        
        except Exception as e:
            print(f"❌ Error listing games: {e}")
            return []
    
    def test_single_game(self, game_id: str, max_steps: int = 100) -> Dict:
        """
        Teste un jeu Arcade
        
        Args:
            game_id: ID du jeu (ex: 'ls20', 'ft09', 'vc33')
            max_steps: Nombre maximum de steps
            
        Returns:
            Résultats du test
        """
        print(f"\n--- Testing game: {game_id} ---")
        
        start_time = time.time()
        
        try:
            # Créer environnement
            env = self.arc.make(game_id, seed=0, save_recording=True)
            
            # Actions disponibles
            actions = [
                GameAction.ACTION1,  # up
                GameAction.ACTION2,  # down
                GameAction.ACTION3,  # left
                GameAction.ACTION4,  # right
                GameAction.ACTION5,  # interact
            ]
            
            # Jouer
            step = 0
            total_score = 0.0
            levels_completed = 0
            
            for step in range(max_steps):
                # Sélectionner action (simple random pour test)
                import random
                action = random.choice(actions)
                
                # Exécuter action
                obs = env.step(action)
                
                if obs:
                    # Extraire score et levels de manière sécurisée
                    total_score = getattr(obs, 'score', 0.0)
                    levels_completed = getattr(obs, 'levels_completed', 0)
                    
                    # Si pas d'attributs directs, essayer via state
                    if total_score == 0.0 and hasattr(obs, 'state'):
                        total_score = getattr(obs.state, 'score', 0.0)
                        levels_completed = getattr(obs.state, 'levels_completed', 0)
                    
                    # Log forensic
                    self._log_forensic(game_id, step, action, obs)
                    
                    # Vérifier si terminé
                    if hasattr(obs, 'state') and hasattr(obs.state, 'name'):
                        if obs.state.name == "GAME_OVER":
                            break
                else:
                    break
            
            duration = time.time() - start_time
            
            # Résultats
            result = {
                'game_id': game_id,
                'steps': step + 1,
                'score': total_score,
                'levels_completed': levels_completed,
                'duration': duration,
                'success': total_score > 0
            }
            
            self.results.append(result)
            
            # Afficher
            status = "✅ SUCCESS" if result['success'] else "❌ FAILED"
            print(f"   {status} - Score: {total_score:.1f} - {step+1} steps - {duration:.2f}s")
            
            return result
        
        except Exception as e:
            print(f"   ❌ ERROR: {e}")
            return {
                'game_id': game_id,
                'error': str(e),
                'success': False
            }
    
    def _log_forensic(self, game_id: str, step: int, action, obs):
        """Log forensic pour chaque step"""
        # Extraire données de manière sécurisée
        state_name = 'UNKNOWN'
        score = 0.0
        levels = 0
        
        if obs:
            if hasattr(obs, 'state') and hasattr(obs.state, 'name'):
                state_name = obs.state.name
            
            score = getattr(obs, 'score', 0.0)
            levels = getattr(obs, 'levels_completed', 0)
            
            # Fallback via state
            if score == 0.0 and hasattr(obs, 'state'):
                score = getattr(obs.state, 'score', 0.0)
                levels = getattr(obs.state, 'levels_completed', 0)
        
        log_entry = {
            'timestamp': datetime.utcnow().isoformat(),
            'game_id': game_id,
            'step': step,
            'action': action.name if hasattr(action, 'name') else str(action),
            'state': state_name,
            'score': score,
            'levels_completed': levels
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(log_entry) + '\n')
    
    def test_all_games(self, max_steps_per_game: int = 100):
        """Teste tous les jeux disponibles"""
        print("\n" + "=" * 80)
        print("TESTING ALL REAL ARCADE GAMES")
        print("=" * 80)
        
        # Lister jeux
        games = self.list_available_games()
        
        if not games:
            print("❌ No games found")
            return
        
        # Tester chaque jeu
        print(f"\n🎮 Testing {len(games)} games...")
        
        for game in games:
            # Gérer à la fois objets et dicts
            game_id = game.game_id if hasattr(game, 'game_id') else game.get('game_id', 'unknown')
            self.test_single_game(game_id, max_steps_per_game)
        
        # Rapport final
        self._print_report()
    
    def _print_report(self):
        """Affiche rapport final"""
        print("\n" + "=" * 80)
        print("FINAL REPORT - REAL ARCADE GAMES")
        print("=" * 80)
        
        total = len(self.results)
        success = sum(1 for r in self.results if r.get('success', False))
        
        print(f"\n📊 RESULTS:")
        print(f"   Total games: {total}")
        print(f"   Successful: {success}")
        print(f"   Success rate: {success/total*100:.1f}%" if total > 0 else "   Success rate: N/A")
        
        if self.results:
            avg_score = sum(r.get('score', 0) for r in self.results) / len(self.results)
            avg_steps = sum(r.get('steps', 0) for r in self.results) / len(self.results)
            avg_duration = sum(r.get('duration', 0) for r in self.results) / len(self.results)
            
            print(f"   Average score: {avg_score:.1f}")
            print(f"   Average steps: {avg_steps:.1f}")
            print(f"   Average duration: {avg_duration:.2f}s")
        
        print(f"\n📝 Forensic log: {self.forensic_log_path}")
        print("=" * 80)


def main():
    """Point d'entrée principal"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Test MAGEN on real ARC-AGI-3 Arcade games')
    parser.add_argument('--mode', choices=['offline', 'online', 'competition'], 
                       default='offline',
                       help='Operation mode (default: offline)')
    parser.add_argument('--game', type=str, 
                       help='Test specific game (e.g., ls20, ft09, vc33)')
    parser.add_argument('--max-steps', type=int, default=100,
                       help='Maximum steps per game (default: 100)')
    parser.add_argument('--list-only', action='store_true',
                       help='Only list available games without testing')
    
    args = parser.parse_args()
    
    # Mapper mode
    mode_map = {
        'offline': OperationMode.OFFLINE,
        'online': OperationMode.ONLINE,
        'competition': OperationMode.COMPETITION
    }
    
    print("\n🚀 Real Arcade Game Tester - ARC-AGI-3")
    print(f"   Mode: {args.mode.upper()}")
    print(f"   API Key: {API_KEY[:20]}...")
    
    # Créer testeur
    tester = RealArcadeGameTester(operation_mode=mode_map[args.mode])
    
    if args.list_only:
        # Lister seulement
        tester.list_available_games()
    
    elif args.game:
        # Tester un jeu spécifique
        tester.test_single_game(args.game, args.max_steps)
        tester._print_report()
    
    else:
        # Tester tous les jeux
        tester.test_all_games(args.max_steps)


if __name__ == '__main__':
    main()

# Made with Bob
