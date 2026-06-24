"""
Test MAGEN V28.2.2 sur Puzzles ARC-AGI-3 Réels
Via API officielle ARC Prize 2026

Objectif: Valider si 42.9% est limite réelle ou artefact environnements synthétiques

API Key: b6ebc9c5-a296-4578-be69-0cc16c9455d4
Documentation: https://docs.arcprize.org/llms.txt

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path
import json
from datetime import datetime

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

from decision_kernel_minimal import DecisionKernelMinimal
from world_model_minimal import WorldModelMinimal

# Configuration API
API_KEY = "b6ebc9c5-a296-4578-be69-0cc16c9455d4"
API_BASE_URL = "https://three.arcprize.org"

def setup_arc_api():
    """Configure environnement ARC-AGI-3."""
    try:
        from arc_agi import Arcade, OperationMode
        
        # Mode COMPETITION requis pour leaderboard
        arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        
        print("✅ API ARC-AGI-3 configurée (mode COMPETITION)")
        return arcade
        
    except ImportError:
        print("❌ Module arc-agi non installé")
        print("\nInstallation requise:")
        print("  pip install arc-agi")
        print("  export ARC_API_KEY=b6ebc9c5-a296-4578-be69-0cc16c9455d4")
        return None


def convert_arc_state_to_magen(arc_state):
    """
    Convertit état ARC-AGI-3 (FrameDataRaw) vers format MAGEN.
    
    ARC state (FrameDataRaw):
    - frame: list[np.array] - liste contenant 1 grille (H, W) avec valeurs 0-11
    - available_actions: list[int] - actions disponibles
    
    MAGEN state:
    - grid: np.array
    - agent_pos: (x, y)
    - goal_pos: (x, y) ou goal_positions: [(x,y), ...]
    - obstacles: set[(x,y), ...]
    """
    import numpy as np
    
    # Extraire grille depuis FrameDataRaw.frame[0]
    grid = arc_state.frame[0].copy()
    
    magen_state = {
        'grid': grid,
        'grid_size': grid.shape[::-1],  # (W, H)
    }
    
    # ARC-AGI utilise valeurs 0-11 pour couleurs
    # Heuristique : détecter agent/goals/obstacles par fréquence
    
    # Compter fréquences valeurs
    unique, counts = np.unique(grid, return_counts=True)
    freq_map = dict(zip(unique, counts))
    
    # Background = valeur la plus fréquente
    background_val = max(freq_map, key=lambda k: freq_map[k])
    
    # Agent = valeur rare (< 5 occurrences) non-background
    agent_pos = None
    for val in unique:
        if val != background_val and freq_map[val] < 5:
            positions = np.argwhere(grid == val)
            if len(positions) > 0:
                y, x = positions[0]
                agent_pos = (x, y)
                break
    
    if agent_pos is None:
        # Fallback : coin supérieur gauche
        agent_pos = (0, 0)
    
    magen_state['agent_pos'] = agent_pos
    
    # Goals = autres valeurs rares (< 10 occurrences)
    goals = []
    for val in unique:
        if val != background_val and freq_map[val] < 10 and val != grid[agent_pos[1], agent_pos[0]]:
            positions = np.argwhere(grid == val)
            goals.extend([(x, y) for y, x in positions])
    
    if not goals:
        # Fallback : coin opposé
        goals = [(grid.shape[1]-1, grid.shape[0]-1)]
    
    if len(goals) == 1:
        magen_state['goal_pos'] = goals[0]
    else:
        magen_state['goal_positions'] = goals
        magen_state['goals_reached'] = []
    
    # Obstacles = valeur 1 (murs communs) ou bords
    obstacles = set()
    if 1 in unique:
        obstacle_positions = np.argwhere(grid == 1)
        obstacles = set((x, y) for y, x in obstacle_positions)
    
    magen_state['obstacles'] = obstacles
    
    return magen_state


def run_arc_puzzle(arcade, game_id: str, kernel: DecisionKernelMinimal, world_model: WorldModelMinimal, max_steps: int = 100):
    """
    Exécute MAGEN sur un puzzle ARC réel.
    
    Args:
        arcade: Instance Arcade API
        game_id: ID du jeu (ex: "game-001-v1")
        kernel: Decision Kernel
        world_model: World Model
        max_steps: Steps maximum
    
    Returns:
        dict: Résultats (success, steps, reward, etc.)
    """
    print(f"\n{'='*80}")
    print(f"PUZZLE: {game_id}")
    print(f"{'='*80}\n")
    
    # Initialiser jeu via make() (pas reset())
    try:
        env = arcade.make(game_id, seed=0)
        if env is None:
            print(f"❌ Erreur make: environnement None")
            return {'success': False, 'error': 'make returned None'}
        
        # Reset environnement
        state = env.reset()
        
        # Vérifier state valide
        if state is None or not hasattr(state, 'frame'):
            print(f"❌ Erreur reset: state invalide")
            return {'success': False, 'error': 'reset returned invalid state'}
            
    except Exception as e:
        print(f"❌ Erreur make/reset: {e}")
        return {'success': False, 'error': str(e)}
    
    # Convertir état ARC → MAGEN
    magen_state = convert_arc_state_to_magen(state)
    
    print(f"Grid: {magen_state['grid_size']}")
    print(f"Agent: {magen_state.get('agent_pos', 'N/A')}")
    print(f"Goal: {magen_state.get('goal_pos', magen_state.get('goal_positions', 'N/A'))}")
    
    # Actions disponibles
    # MAGEN utilise strings, API ARC utilise GameAction enum
    from arcengine.enums import GameAction
    
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']  # up, down, left, right
    action_to_enum = {
        'ACTION1': GameAction.ACTION1,
        'ACTION2': GameAction.ACTION2,
        'ACTION3': GameAction.ACTION3,
        'ACTION4': GameAction.ACTION4,
    }
    
    # Métriques
    total_reward = 0.0
    step = 0
    done = False
    trajectory = []
    
    while not done and step < max_steps:
        step += 1
        
        # Décision MAGEN
        action, decision_info = kernel.select_action(
            magen_state, actions,
            world_model=world_model
        )
        
        # Convertir action MAGEN → GameAction enum
        action_enum = action_to_enum.get(action, GameAction.ACTION1)  # Fallback: ACTION1
        
        # Exécuter action via EnvironmentWrapper
        try:
            # API ARC retourne FrameDataRaw, pas tuple (state, reward, done, info)
            result = env.step(action_enum)
            
            # Extraire infos depuis FrameDataRaw
            from arcengine.enums import GameState
            done = (result.state == GameState.WIN or result.state == GameState.GAME_OVER)
            
            # Reward basé sur progression
            prev_levels = magen_state.get('levels_completed', 0)
            curr_levels = result.levels_completed
            reward = float(curr_levels - prev_levels)
            
            # Bonus si WIN
            if result.state == GameState.WIN:
                reward += 10.0
            
            total_reward += reward
            
            # Convertir next_state
            next_magen_state = convert_arc_state_to_magen(result)
            next_magen_state['levels_completed'] = curr_levels
            
            # Observer transition
            world_model.observe_transition(magen_state, action, next_magen_state)
            
            # Apprentissage
            kernel.update_weights(reward)
            
            # Trajectoire
            trajectory.append({
                'step': step,
                'action': action,
                'reward': reward,
                'state': result.state.value,
                'levels': curr_levels,
                'scores': decision_info['scores']
            })
            
            magen_state = next_magen_state
            
        except Exception as e:
            print(f"❌ Erreur step {step}: {e}")
            break
    
    # Résultats
    success = done and total_reward > 0
    
    print(f"\nRésultats:")
    print(f"  Success: {'✅' if success else '❌'}")
    print(f"  Steps: {step}")
    print(f"  Reward: {total_reward:.2f}")
    
    return {
        'game_id': game_id,
        'success': success,
        'steps': step,
        'reward': total_reward,
        'trajectory': trajectory
    }


def main():
    """Test MAGEN V28.2.2 sur puzzles ARC réels."""
    
    print("="*80)
    print("TEST MAGEN V28.2.2 - JEUX ARCADE ARC-AGI-3 (MODE COMPETITION)")
    print("="*80)
    print()
    
    # Setup API
    arcade = setup_arc_api()
    if arcade is None:
        print("\n⚠️  Installation arc-agi requise. Voir instructions ci-dessus.")
        return
    
    # Créer Decision Kernel V28.2.2
    kernel = DecisionKernelMinimal(
        learning_rate=0.01,
        exploration_rate=0.2,
        temperature=1.0
    )
    
    # Créer World Model
    world_model = WorldModelMinimal(grid_size=(64, 64))
    
    # Obtenir liste environnements disponibles via API
    try:
        envs = arcade.get_environments()
        print(f"✅ {len(envs)} environnements disponibles")
        
        # Trier par difficulté (baseline_actions = nombre d'actions optimal)
        sorted_envs = sorted(envs, key=lambda e: sum(e.baseline_actions))
        
        # Sélectionner 3 jeux ARCADE les plus faciles
        test_puzzles = [env.game_id for env in sorted_envs[:3]]
        print(f"\n🎯 Test sur 3 jeux ARCADE les plus faciles:")
        for i, env in enumerate(sorted_envs[:3]):
            print(f"  {i+1}. {env.title} ({env.game_id}): {sum(env.baseline_actions)} actions baseline")
        print()
        
    except Exception as e:
        print(f"❌ Erreur get_environments: {e}")
        # Fallback: IDs connus
        test_puzzles = ["ls20", "ls21", "ls22", "ls23", "ls24"]
        print(f"Fallback: test sur {test_puzzles}")
    
    # Résultats
    results = []
    
    for game_id in test_puzzles:
        result = run_arc_puzzle(arcade, game_id, kernel, world_model, max_steps=50)
        results.append(result)
    
    # Analyse globale
    print("\n" + "="*80)
    print("ANALYSE GLOBALE")
    print("="*80)
    print()
    
    successes = sum(1 for r in results if r.get('success', False))
    total = len(results)
    success_rate = (successes / total * 100) if total > 0 else 0
    
    print(f"Puzzles résolus: {successes}/{total} ({success_rate:.1f}%)")
    print()
    
    # Comparaison avec environnements synthétiques
    print("Comparaison:")
    print(f"  Environnements synthétiques: 42.9% (3/7 goals)")
    print(f"  Puzzles ARC réels: {success_rate:.1f}% ({successes}/{total})")
    print()
    
    if success_rate > 42.9:
        print("✅ MAGEN performe MIEUX sur ARC réels")
    elif success_rate < 42.9:
        print("⚠️  MAGEN performe MOINS BIEN sur ARC réels")
    else:
        print("➡️  Performance similaire")
    
    # Sauvegarder résultats
    log_dir = Path(__file__).parent / 'logs_arc_real'
    log_dir.mkdir(exist_ok=True)
    
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    results_file = log_dir / f'arc_real_results_{timestamp}.json'
    
    with open(results_file, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'version': 'V28.2.2',
            'success_rate': success_rate,
            'results': results
        }, f, indent=2)
    
    print(f"\nRésultats sauvegardés: {results_file}")
    print()
    print("="*80)
    print("TEST TERMINÉ")
    print("="*80)


if __name__ == '__main__':
    main()

# Made with Bob
