#!/usr/bin/env python3
"""
Test MAGEN V28 sur UN jeu à la fois - TEMPS ILLIMITÉ
Ne passe au suivant QUE si WIN
Logs forensic temps réel
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from arc_agi import Arcade, OperationMode
from arcengine import GameAction
from MAGEN.core.decision_kernel_minimal import DecisionKernelMinimal
from MAGEN.core.world_model_minimal import WorldModelMinimal
import json
from datetime import datetime
import time

def convert_arc_state_to_magen(result):
    """Convertit result ARC en format MAGEN"""
    if not result or not hasattr(result, 'grid'):
        return {
            'position': (0, 0),
            'goal': (0, 0),
            'grid': [[0]],
            'levels_completed': 0
        }
    
    grid = result.grid
    height = len(grid)
    width = len(grid[0]) if height > 0 else 0
    
    return {
        'position': (width // 2, height // 2),
        'goal': (width - 1, height - 1),
        'grid': grid,
        'levels_completed': getattr(result, 'levels_completed', 0)
    }

def run_v28_unlimited(arc, game_id):
    """
    Exécute V28 sur un jeu SANS LIMITE DE TEMPS
    Tourne jusqu'à WIN ou abandon manuel
    """
    
    print("\n" + "="*80)
    print(f"DÉMARRAGE TEST ILLIMITÉ : {game_id}")
    print("="*80)
    print(f"Temps : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("Objectif : WIN (pas de limite steps)")
    print("="*80 + "\n")
    
    try:
        # Créer environnement
        env = arc.make(game_id, render_mode=None)
        if not env:
            print(f"✗ Impossible de créer environnement pour {game_id}")
            return None
        
        # Initialiser kernel
        world_model = WorldModelMinimal()
        kernel = DecisionKernelMinimal(
            learning_rate=0.01,
            exploration_rate=0.2,
            temperature=1.0
        )
        
        # Reset
        result = env.reset()
        state = convert_arc_state_to_magen(result)
        
        # Variables tracking
        total_reward = 0.0
        prev_levels = 0
        step = 0
        start_time = time.time()
        last_log_time = start_time
        
        # Log forensic
        forensic_log = []
        
        print(f"[INIT] Jeu démarré")
        print(f"[INIT] Grid size: {len(state['grid'])}x{len(state['grid'][0]) if state['grid'] else 0}")
        print(f"[INIT] Levels: {state['levels_completed']}")
        print()
        
        # Boucle INFINIE jusqu'à WIN ou GAME_OVER
        while True:
            step += 1
            
            # Sélectionner action
            available_actions = [GameAction.ACTION1, GameAction.ACTION2, 
                               GameAction.ACTION3, GameAction.ACTION4]
            action, decision_info = kernel.select_action(
                state, 
                available_actions,
                world_model=world_model
            )
            
            # Exécuter action
            result = env.step(action)
            
            # Extraire reward
            curr_levels = result.levels_completed
            reward = float(curr_levels - prev_levels)
            prev_levels = curr_levels
            
            # Nouveau state
            next_state = convert_arc_state_to_magen(result)
            total_reward += reward
            
            # Update kernel
            kernel.update_weights(reward)
            
            # Check état
            game_state = result.state.name
            
            # Log forensic (convertir GameAction en str pour JSON)
            scores_dict = decision_info.get('scores', {})
            scores_serializable = {str(k): v for k, v in scores_dict.items()}
            
            forensic_entry = {
                'step': step,
                'action': str(action),
                'reward': reward,
                'total_reward': total_reward,
                'game_state': game_state,
                'levels_completed': curr_levels,
                'scores': scores_serializable,
                'exploration': decision_info.get('is_exploration', False)
            }
            forensic_log.append(forensic_entry)
            
            # Log temps réel (toutes les 10 steps)
            current_time = time.time()
            if step % 10 == 0 or reward > 0 or game_state != 'NOT_FINISHED':
                elapsed = current_time - start_time
                rate = step / elapsed if elapsed > 0 else 0
                
                print(f"[STEP {step:5d}] {str(action)} | "
                      f"Reward: {reward:+.2f} (total: {total_reward:.2f}) | "
                      f"Levels: {curr_levels} | "
                      f"State: {game_state} | "
                      f"Rate: {rate:.1f} steps/sec")
                
                # Si reward positif, log détaillé
                if reward > 0:
                    print(f"  ✓ PROGRÈS ! Niveau complété !")
                    print(f"  Scores: {decision_info.get('scores', {})}")
            
            state = next_state
            
            # Check conditions arrêt
            if game_state == 'WIN':
                elapsed = time.time() - start_time
                print("\n" + "="*80)
                print(f"✓✓✓ VICTOIRE ! ✓✓✓")
                print("="*80)
                print(f"Steps: {step}")
                print(f"Reward total: {total_reward}")
                print(f"Levels complétés: {curr_levels}")
                print(f"Temps: {elapsed:.1f}s ({step/elapsed:.1f} steps/sec)")
                print("="*80 + "\n")
                
                # Sauvegarder forensic
                forensic_file = f"forensic_{game_id}_WIN_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
                with open(forensic_file, 'w') as f:
                    json.dump({
                        'game_id': game_id,
                        'success': True,
                        'steps': step,
                        'total_reward': total_reward,
                        'levels_completed': curr_levels,
                        'elapsed_time': elapsed,
                        'forensic': forensic_log
                    }, f, indent=2)
                print(f"Forensic sauvegardé : {forensic_file}\n")
                
                return {
                    'game_id': game_id,
                    'success': True,
                    'steps': step,
                    'total_reward': total_reward,
                    'levels_completed': curr_levels,
                    'elapsed_time': elapsed
                }
            
            elif game_state == 'GAME_OVER':
                elapsed = time.time() - start_time
                print("\n" + "="*80)
                print(f"✗ GAME OVER")
                print("="*80)
                print(f"Steps: {step}")
                print(f"Reward total: {total_reward}")
                print(f"Levels complétés: {curr_levels}")
                print(f"Temps: {elapsed:.1f}s")
                print("="*80 + "\n")
                
                # Sauvegarder forensic
                forensic_file = f"forensic_{game_id}_GAMEOVER_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
                with open(forensic_file, 'w') as f:
                    json.dump({
                        'game_id': game_id,
                        'success': False,
                        'steps': step,
                        'total_reward': total_reward,
                        'levels_completed': curr_levels,
                        'elapsed_time': elapsed,
                        'forensic': forensic_log
                    }, f, indent=2)
                print(f"Forensic sauvegardé : {forensic_file}\n")
                
                return {
                    'game_id': game_id,
                    'success': False,
                    'steps': step,
                    'total_reward': total_reward,
                    'levels_completed': curr_levels,
                    'elapsed_time': elapsed
                }
        
    except KeyboardInterrupt:
        print("\n\n✗ Arrêt manuel (Ctrl+C)")
        return None
    except Exception as e:
        print(f"\n\n✗ Erreur : {e}")
        import traceback
        traceback.print_exc()
        return None

def main():
    """Test séquentiel : un jeu à la fois, temps illimité"""
    
    print("="*80)
    print("TEST MAGEN V28 - MODE ILLIMITÉ - UN JEU À LA FOIS")
    print("="*80)
    print("Stratégie : Gagner chaque jeu avant de passer au suivant")
    print("Temps : ILLIMITÉ (pas de limite steps)")
    print("="*80 + "\n")
    
    # Mode NORMAL
    arc = Arcade(operation_mode=OperationMode.NORMAL)
    games = arc.get_environments()
    
    print(f"Jeux disponibles : {len(games)}")
    print()
    
    # Trier par difficulté (heuristique : nom alphabétique)
    games_sorted = sorted(games, key=lambda g: g.game_id)
    
    print("Ordre de test (alphabétique) :")
    for i, game in enumerate(games_sorted[:10], 1):
        print(f"  {i}. {game.game_id}")
    print()
    
    # Test séquentiel
    results = []
    wins = 0
    
    for i, game_info in enumerate(games_sorted, 1):
        game_id = game_info.game_id
        
        print(f"\n{'#'*80}")
        print(f"# JEU {i}/{len(games_sorted)} : {game_id}")
        print(f"# Victoires jusqu'ici : {wins}/{i-1}")
        print(f"{'#'*80}\n")
        
        result = run_v28_unlimited(arc, game_id)
        
        if result:
            results.append(result)
            if result['success']:
                wins += 1
                print(f"✓ Victoire ! On passe au suivant ({wins} victoires)")
            else:
                print(f"✗ Échec. On passe au suivant quand même")
        else:
            print(f"✗ Erreur ou arrêt manuel")
            break
        
        # Pause entre jeux
        print("\nPause 2 secondes avant jeu suivant...\n")
        time.sleep(2)
    
    # Résumé final
    print("\n" + "="*80)
    print("RÉSUMÉ FINAL")
    print("="*80)
    print(f"Jeux testés : {len(results)}/{len(games_sorted)}")
    print(f"Victoires : {wins}/{len(results)}")
    print(f"Taux succès : {wins/len(results)*100:.1f}%" if results else "0%")
    print("="*80)

if __name__ == "__main__":
    main()

# Made with Bob
