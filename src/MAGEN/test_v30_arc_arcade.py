"""
Test MAGEN V30 sur ARC Arcade Réels (3 jeux faciles)
====================================================

Test V30 avec TVT sur jeux Arcade officiels via API.

Date : 2026-06-15
"""

import sys
import os
import json
import time
from datetime import datetime
from typing import Dict, List, Tuple
import numpy as np

# Ajouter path MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'core'))

from decision_kernel_v30_tvt import DecisionKernelV30TVT

# Import ARC-AGI-3
try:
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
except ImportError as e:
    print(f"ERROR: arc-agi module not found: {e}")
    print("Install with: pip install arc-agi")
    sys.exit(1)


def setup_arc_api():
    """Configure API ARC-AGI-3."""
    api_key = "b6ebc9c5-a296-4578-be69-0cc16c9455d4"
    os.environ['ARC_API_KEY'] = api_key
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    return arcade


def convert_arc_state_to_magen(result) -> Dict:
    """Convertit FrameDataRaw ARC → format MAGEN."""
    # Extraire grille (première frame)
    grid = result.frame[0] if result.frame else np.zeros((64, 64))
    
    # Position agent (chercher valeur 5 dans grille)
    agent_positions = np.argwhere(grid == 5)
    if len(agent_positions) > 0:
        agent_pos = tuple(agent_positions[0])
    else:
        agent_pos = (0, 0)
    
    # Goal (chercher valeur 10)
    goal_positions = np.argwhere(grid == 10)
    if len(goal_positions) > 0:
        goal_pos = tuple(goal_positions[0])
    else:
        goal_pos = (63, 63)
    
    # Obstacles (valeur -1)
    obstacles = [tuple(pos) for pos in np.argwhere(grid == -1)]
    
    return {
        'grid': grid,
        'grid_size': grid.shape,
        'agent_pos': agent_pos,
        'goal_pos': goal_pos,
        'obstacles': obstacles,
        'last_reward': 0.0
    }


def run_v30_on_arcade_game(
    arcade: Arcade,
    game_id: str,
    max_steps: int = 50
) -> Dict:
    """
    Exécute V30 sur un jeu Arcade réel.
    
    Args:
        arcade: Instance Arcade
        game_id: ID du jeu
        max_steps: Steps maximum
    
    Returns:
        Résultats test
    """
    print(f"\n{'='*80}")
    print(f"Test V30 sur {game_id}")
    print(f"{'='*80}")
    
    # Créer environnement
    env = arcade.make(game_id)
    
    # Créer kernel V30
    kernel = DecisionKernelV30TVT(
        learning_rate=0.01,
        exploration_rate=0.2,
        temperature=1.0,
        stagnation_threshold=10,
        collapse_variance_threshold=0.02,
        enable_policy_breaking=True
    )
    
    # Reset
    result = env.reset()
    state = convert_arc_state_to_magen(result)
    
    # Actions disponibles
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    action_to_enum = {
        'ACTION1': GameAction.ACTION1,
        'ACTION2': GameAction.ACTION2,
        'ACTION3': GameAction.ACTION3,
        'ACTION4': GameAction.ACTION4
    }
    
    # Exécution
    trajectory = []
    prev_levels = 0
    total_reward = 0.0
    
    for step in range(max_steps):
        # Sélection action V30
        action_str, decision_info = kernel.select_action(state, actions)
        action_enum = action_to_enum[action_str]
        
        # Exécuter dans Arcade
        result = env.step(action_enum)
        
        # Extraire reward
        curr_levels = result.levels_completed
        reward = float(curr_levels - prev_levels)
        prev_levels = curr_levels
        
        # Nouveau state
        next_state = convert_arc_state_to_magen(result)
        next_state['last_reward'] = reward
        
        # Update kernel
        done = (result.state.name == 'WIN')
        kernel.update_after_step(state, action_str, next_state, reward, done)
        
        # Log trajectory
        trajectory.append({
            'step': step,
            'action': action_str,
            'reward': reward,
            'game_state': result.state.name,
            'levels_completed': curr_levels,
            'scores': decision_info.get('scores', {}),
            'policy_broken': decision_info.get('policy_broken', False)
        })
        
        total_reward += reward
        state = next_state
        
        # Affichage
        if step % 10 == 0 or reward > 0:
            print(f"  Step {step}: {action_str} | Reward: {reward:.2f} | State: {result.state.name} | Levels: {curr_levels}")
        
        if done:
            print(f"  ✓ WIN at step {step}!")
            break
        
        if result.state.name == 'GAME_OVER':
            print(f"  ✗ GAME_OVER at step {step}")
            break
    
    # Métriques finales
    success = (result.state.name == 'WIN')
    metrics = kernel.get_extended_metrics()
    
    print(f"\n  Résultat : {'SUCCESS' if success else 'FAIL'}")
    print(f"  Steps : {len(trajectory)}")
    print(f"  Reward total : {total_reward:.2f}")
    print(f"  Policy breaks : {metrics.get('total_policy_breaks', 0)}")
    print(f"  Stagnation steps : {metrics.get('current_stagnation_steps', 0)}")
    
    return {
        'game_id': game_id,
        'success': success,
        'steps': len(trajectory),
        'total_reward': total_reward,
        'final_state': result.state.name,
        'levels_completed': result.levels_completed,
        'metrics': metrics,
        'trajectory': trajectory
    }


def main():
    """Test V30 sur 3 jeux Arcade les plus faciles."""
    print("=" * 80)
    print("TEST MAGEN V30 SUR ARC ARCADE RÉELS")
    print("=" * 80)
    
    # Setup API
    print("\n[1] Configuration API ARC-AGI-3...")
    arcade = setup_arc_api()
    print("  ✓ API configurée (mode COMPETITION)")
    
    # 3 jeux les plus faciles
    games = [
        'cd82-fb555c5d',  # CD82
        'ft09-0d8bbf25',  # FT09
        'ls20-c8b7cc0f'   # LS20
    ]
    
    # Résultats
    results = {
        'timestamp': datetime.now().isoformat(),
        'version': 'V30.0.0-TVT',
        'games': []
    }
    
    # Test chaque jeu
    for game_id in games:
        try:
            result = run_v30_on_arcade_game(arcade, game_id, max_steps=50)
            results['games'].append(result)
        except Exception as e:
            print(f"\n  ✗ ERROR on {game_id}: {e}")
            import traceback
            traceback.print_exc()
            results['games'].append({
                'game_id': game_id,
                'success': False,
                'error': str(e)
            })
    
    # Statistiques globales
    print(f"\n{'='*80}")
    print("STATISTIQUES GLOBALES")
    print(f"{'='*80}")
    
    success_count = sum(1 for r in results['games'] if r.get('success', False))
    print(f"\nTaux succès : {success_count}/{len(games)} ({100*success_count/len(games):.1f}%)")
    
    total_policy_breaks = sum(
        r.get('metrics', {}).get('total_policy_breaks', 0)
        for r in results['games']
    )
    print(f"Policy breaks total : {total_policy_breaks}")
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"test_v30_arcade_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nRésultats sauvegardés : {output_file}")
    
    return results


if __name__ == '__main__':
    results = main()

# Made with Bob
