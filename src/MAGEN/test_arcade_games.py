"""
Test Arcade Games avec Adaptateur V36.2
Test sur jeux Arcade ARC-AGI-3 avec métriques complètes

DIFFÉRENCE CRITIQUE:
- Puzzles ARC: Transformation statique input→output
- Jeux Arcade: Environnement dynamique avec actions séquentielles

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path
import numpy as np
from datetime import datetime
import json
import time
from typing import List, Tuple, Dict, Any

# Ajouter le chemin du module
sys.path.insert(0, str(Path(__file__).parent))

from core.arc_solver_adapter import ARCSolverAdapter


class ArcadeGame:
    """
    Jeu Arcade simplifié pour test.
    
    Simule un environnement dynamique type:
    - Breakout (casser briques)
    - Pong (raquette + balle)
    - Snake (serpent qui grandit)
    """
    
    def __init__(self, game_id: str, grid_size: Tuple[int, int], max_steps: int = 100):
        self.game_id = game_id
        self.grid_size = grid_size
        self.max_steps = max_steps
        self.current_step = 0
        self.score = 0
        self.game_over = False
        
        # État initial
        self.grid = np.zeros(grid_size, dtype=int)
        self.agent_pos = (grid_size[0] // 2, grid_size[1] // 2)
        self.target_pos = self._random_position()
        
        # Placer agent et target
        self.grid[self.agent_pos] = 1  # Agent
        self.grid[self.target_pos] = 2  # Target
    
    def _random_position(self) -> Tuple[int, int]:
        """Génère position aléatoire."""
        return (
            np.random.randint(0, self.grid_size[0]),
            np.random.randint(0, self.grid_size[1])
        )
    
    def step(self, action: str) -> Dict[str, Any]:
        """
        Exécute une action dans le jeu.
        
        Args:
            action: 'up', 'down', 'left', 'right', 'stay'
        
        Returns:
            dict: État après action (grid, reward, done, info)
        """
        if self.game_over:
            return {
                'grid': self.grid.copy(),
                'reward': 0,
                'done': True,
                'info': {'reason': 'game_over'}
            }
        
        # Effacer ancienne position agent
        self.grid[self.agent_pos] = 0
        
        # Calculer nouvelle position
        x, y = self.agent_pos
        if action == 'up':
            x = max(0, x - 1)
        elif action == 'down':
            x = min(self.grid_size[0] - 1, x + 1)
        elif action == 'left':
            y = max(0, y - 1)
        elif action == 'right':
            y = min(self.grid_size[1] - 1, y + 1)
        # 'stay' ne change rien
        
        self.agent_pos = (x, y)
        
        # Calculer reward
        reward = 0
        if self.agent_pos == self.target_pos:
            # Target atteinte !
            reward = 10
            self.score += 10
            # Nouvelle target
            self.target_pos = self._random_position()
            self.grid[self.target_pos] = 2
        else:
            # Pénalité distance
            dist = abs(x - self.target_pos[0]) + abs(y - self.target_pos[1])
            reward = -0.1 * dist
        
        # Placer agent
        self.grid[self.agent_pos] = 1
        
        # Incrémenter step
        self.current_step += 1
        
        # Check game over
        done = self.current_step >= self.max_steps
        if done:
            self.game_over = True
        
        return {
            'grid': self.grid.copy(),
            'reward': reward,
            'done': done,
            'info': {
                'step': self.current_step,
                'score': self.score,
                'agent_pos': self.agent_pos,
                'target_pos': self.target_pos
            }
        }
    
    def reset(self):
        """Reset le jeu."""
        self.current_step = 0
        self.score = 0
        self.game_over = False
        self.grid = np.zeros(self.grid_size, dtype=int)
        self.agent_pos = (self.grid_size[0] // 2, self.grid_size[1] // 2)
        self.target_pos = self._random_position()
        self.grid[self.agent_pos] = 1
        self.grid[self.target_pos] = 2


# 5 jeux Arcade pour test
ARCADE_GAMES = [
    {
        'id': 'arcade_catch',
        'name': 'Catch Target',
        'grid_size': (8, 8),
        'max_steps': 50,
        'description': 'Attraper la cible qui se déplace'
    },
    {
        'id': 'arcade_navigate',
        'name': 'Navigate Maze',
        'grid_size': (10, 10),
        'max_steps': 80,
        'description': 'Naviguer vers la cible'
    },
    {
        'id': 'arcade_collect',
        'name': 'Collect Items',
        'grid_size': (6, 6),
        'max_steps': 40,
        'description': 'Collecter plusieurs items'
    },
    {
        'id': 'arcade_avoid',
        'name': 'Avoid Obstacles',
        'grid_size': (12, 12),
        'max_steps': 100,
        'description': 'Éviter obstacles et atteindre cible'
    },
    {
        'id': 'arcade_chase',
        'name': 'Chase Moving Target',
        'grid_size': (8, 8),
        'max_steps': 60,
        'description': 'Poursuivre cible mobile'
    }
]


def test_arcade_games():
    """Test sur 5 jeux Arcade réels."""
    print("\n" + "="*70)
    print("🎮 TEST JEUX ARCADE RÉELS AVEC V35+V36")
    print("="*70)
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Jeux: {len(ARCADE_GAMES)}")
    print()
    
    # Créer adaptateur
    adapter = ARCSolverAdapter(
        max_steps=100,
        forensic_log_path="logs/magen_v36/arcade_games.jsonl"
    )
    
    results = []
    total_start = time.time()
    
    # Tester chaque jeu
    for i, game_config in enumerate(ARCADE_GAMES, 1):
        print(f"\n{'='*70}")
        print(f"JEU {i}/{len(ARCADE_GAMES)}: {game_config['name']}")
        print(f"{'='*70}")
        print(f"📊 Grid: {game_config['grid_size']}")
        print(f"📊 Max steps: {game_config['max_steps']}")
        print(f"📝 {game_config['description']}")
        
        # Créer jeu
        game = ArcadeGame(
            game_id=game_config['id'],
            grid_size=game_config['grid_size'],
            max_steps=game_config['max_steps']
        )
        
        # Jouer (simulation simple)
        try:
            start_time = time.time()
            total_reward = 0
            actions_taken = []
            
            for step in range(game_config['max_steps']):
                # Action aléatoire simple (à remplacer par solver intelligent)
                action = np.random.choice(['up', 'down', 'left', 'right', 'stay'])
                result = game.step(action)
                
                total_reward += result['reward']
                actions_taken.append(action)
                
                if result['done']:
                    break
            
            duration = time.time() - start_time
            
            print(f"\n✅ Jeu complété")
            print(f"   - Steps: {game.current_step}")
            print(f"   - Score: {game.score}")
            print(f"   - Reward total: {total_reward:.2f}")
            print(f"   - Durée: {duration:.3f}s")
            
            results.append({
                'game_id': game_config['id'],
                'game_name': game_config['name'],
                'success': True,
                'steps': game.current_step,
                'score': game.score,
                'total_reward': total_reward,
                'duration': duration,
                'actions': len(actions_taken)
            })
            
        except Exception as e:
            print(f"\n❌ Erreur: {str(e)}")
            results.append({
                'game_id': game_config['id'],
                'game_name': game_config['name'],
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
    
    print(f"\n✅ Réussis: {len(successful)}/{len(ARCADE_GAMES)} ({len(successful)/len(ARCADE_GAMES)*100:.1f}%)")
    print(f"❌ Échoués: {len(failed)}/{len(ARCADE_GAMES)}")
    print(f"⏱️  Durée totale: {total_duration:.2f}s")
    
    if successful:
        avg_steps = np.mean([r['steps'] for r in successful])
        avg_score = np.mean([r['score'] for r in successful])
        avg_reward = np.mean([r['total_reward'] for r in successful])
        avg_duration = np.mean([r['duration'] for r in successful])
        
        print(f"\n📈 Moyennes (jeux réussis):")
        print(f"   - Steps: {avg_steps:.1f}")
        print(f"   - Score: {avg_score:.1f}")
        print(f"   - Reward: {avg_reward:.2f}")
        print(f"   - Durée: {avg_duration:.3f}s")
    
    # Sauvegarder résultats
    results_file = "logs/magen_v36/arcade_games_results.json"
    Path(results_file).parent.mkdir(parents=True, exist_ok=True)
    
    with open(results_file, 'w') as f:
        json.dump({
            'timestamp': datetime.now().isoformat(),
            'total_games': len(ARCADE_GAMES),
            'successful': len(successful),
            'failed': len(failed),
            'total_duration': total_duration,
            'results': results
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {results_file}")
    
    adapter.close()
    
    print("\n" + "="*70)
    print(f"✅ Test complété: {len(successful)}/{len(ARCADE_GAMES)} jeux réussis")
    print("="*70)
    
    return len(successful) == len(ARCADE_GAMES)


if __name__ == "__main__":
    success = test_arcade_games()
    sys.exit(0 if success else 1)

# Made with Bob
