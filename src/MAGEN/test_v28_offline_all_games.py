#!/usr/bin/env python3
"""
Test MAGEN V28.2.2 sur TOUS les jeux ARC-AGI-3 en mode OFFLINE
Mode OFFLINE = 2000 FPS, pas de rate limits, pas d'API key
"""

import sys
import os

# Add parent directories to path
current_dir = os.path.dirname(os.path.abspath(__file__))
magen_dir = current_dir
src_dir = os.path.dirname(magen_dir)
sys.path.insert(0, src_dir)

from arc_agi import Arcade, OperationMode
from arcengine import GameAction
from MAGEN.core.decision_kernel_minimal import DecisionKernelMinimal
from MAGEN.core.world_model_minimal import WorldModelMinimal
import json
from datetime import datetime
import time

def convert_arc_state_to_magen(frame_data):
    """Convertit FrameDataRaw en format MAGEN"""
    if not frame_data or not hasattr(frame_data, 'grid'):
        return {
            'position': (0, 0),
            'goal': (0, 0),
            'grid': [[0]],
            'levels_completed': 0
        }
    
    grid = frame_data.grid
    height = len(grid)
    width = len(grid[0]) if height > 0 else 0
    
    return {
        'position': (width // 2, height // 2),
        'goal': (width - 1, height - 1),
        'grid': grid,
        'levels_completed': getattr(frame_data, 'levels_completed', 0)
    }

def run_v28_on_game(arc, game_id, max_steps=50):
    """Exécute V28.2.2 sur un jeu"""
    
    try:
        # Créer environnement (pas de render pour vitesse)
        env = arc.make(game_id, render_mode=None)
        if not env:
            return {
                'game_id': game_id,
                'success': False,
                'error': 'Failed to create environment',
                'steps': 0,
                'total_reward': 0.0
            }
        
        # Initialiser kernel et world model
        world_model = WorldModelMinimal()
        kernel = DecisionKernelMinimal()
        
        # Reset (retourne UN objet result)
        result = env.reset()
        state = convert_arc_state_to_magen(result)
        
        total_reward = 0.0
        prev_levels = 0
        
        for step in range(max_steps):
            # Sélectionner action
            available_actions = [GameAction.ACTION1, GameAction.ACTION2,
                               GameAction.ACTION3, GameAction.ACTION4]
            action, decision_info = kernel.select_action(
                state,
                available_actions,
                world_model=world_model
            )
            
            # Exécuter action (retourne UN objet result)
            result = env.step(action)
            
            # Extraire reward
            curr_levels = result.levels_completed
            reward = float(curr_levels - prev_levels)
            prev_levels = curr_levels
            
            # Nouveau state
            next_state = convert_arc_state_to_magen(result)
            total_reward += reward
            
            # Update kernel weights
            kernel.update_weights(reward)
            
            state = next_state
            
            # Check done
            game_state = result.state.name
            if game_state in ['WIN', 'GAME_OVER']:
                break
        
        return {
            'game_id': game_id,
            'success': game_state == 'WIN',
            'steps': step + 1,
            'total_reward': float(total_reward),
            'final_state': game_state,
            'levels_completed': curr_levels
        }
        
    except Exception as e:
        return {
            'game_id': game_id,
            'success': False,
            'error': str(e),
            'steps': 0,
            'total_reward': 0.0
        }

def test_v28_offline_all():
    """Test V28.2.2 sur tous les jeux disponibles"""
    
    print("="*80)
    print("TEST MAGEN V28 MINIMAL - MODE OFFLINE - TOUS LES JEUX")
    print("="*80)
    print()
    
    # Mode NORMAL (local + remote, pas de rate limits si local)
    print("[1] Initialisation mode NORMAL (local + remote)...")
    arc = Arcade(operation_mode=OperationMode.NORMAL)
    print("  ✓ Mode NORMAL activé")
    print()
    
    # Liste tous les jeux
    print("[2] Récupération liste des jeux...")
    games = arc.get_environments()
    print(f"  ✓ {len(games)} jeux trouvés")
    print()
    
    # Test chaque jeu
    print("[3] Test V28 Minimal sur chaque jeu...")
    print()
    
    results = []
    start_time = time.time()
    
    for i, game_info in enumerate(games):
        game_id = game_info.game_id
        
        # Progress
        print(f"[{i+1}/{len(games)}] {game_id}...", end=" ", flush=True)
        
        # Test
        result = run_v28_on_game(arc, game_id, max_steps=50)
        results.append(result)
        
        # Status
        if result['success']:
            print(f"✓ WIN ({result['steps']} steps, reward={result['total_reward']:.2f})")
        else:
            status = result.get('final_state', result.get('error', 'UNKNOWN'))
            print(f"✗ {status} ({result['steps']} steps)")
        
        # Stats intermédiaires tous les 50 jeux
        if (i + 1) % 50 == 0:
            success_rate = sum(1 for r in results if r['success']) / len(results)
            elapsed = time.time() - start_time
            rate = len(results) / elapsed
            print(f"  → Progress: {success_rate:.1%} success, {rate:.1f} games/sec")
            print()
    
    elapsed_time = time.time() - start_time
    
    # Statistiques finales
    print()
    print("="*80)
    print("STATISTIQUES FINALES")
    print("="*80)
    
    success_count = sum(1 for r in results if r['success'])
    success_rate = success_count / len(results) if results else 0
    
    print(f"Total jeux testés : {len(results)}")
    print(f"Succès : {success_count}/{len(results)} ({success_rate:.1%})")
    print(f"Échecs : {len(results) - success_count}/{len(results)}")
    print(f"Temps total : {elapsed_time:.1f}s")
    print(f"Vitesse : {len(results)/elapsed_time:.1f} jeux/sec")
    print()
    
    # Distribution par état final
    states = {}
    for r in results:
        state = r.get('final_state', r.get('error', 'UNKNOWN'))
        states[state] = states.get(state, 0) + 1
    
    print("Distribution états finaux :")
    for state, count in sorted(states.items(), key=lambda x: -x[1]):
        print(f"  {state}: {count} ({count/len(results):.1%})")
    print()
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"test_v28_offline_all_{timestamp}.json"
    
    output = {
        'timestamp': timestamp,
        'version': 'V28_MINIMAL',
        'mode': 'NORMAL',
        'total_games': len(games),
        'success_count': success_count,
        'success_rate': success_rate,
        'elapsed_time': elapsed_time,
        'games_per_second': len(results) / elapsed_time,
        'results': results,
        'state_distribution': states
    }
    
    with open(filename, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"Résultats sauvegardés : {filename}")
    print()
    
    return results

if __name__ == "__main__":
    try:
        results = test_v28_offline_all()
        
        # Résumé final
        success_count = sum(1 for r in results if r['success'])
        print("="*80)
        print(f"✓ Test terminé : {success_count}/{len(results)} succès ({success_count/len(results):.1%})")
        print("="*80)
        
    except KeyboardInterrupt:
        print("\n\n✗ Test interrompu par l'utilisateur")
    except Exception as e:
        print(f"\n\n✗ Erreur fatale : {e}")
        import traceback
        traceback.print_exc()

# Made with Bob
