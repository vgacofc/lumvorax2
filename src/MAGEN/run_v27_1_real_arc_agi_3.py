#!/usr/bin/env python3
"""
Runner MAGEN V27.1 CAUSAL - Exécution Réelle ARC-AGI-3
========================================================

Exécute agent V27.1 sur jeux réels ARC-AGI-3 avec:
- Logging forensic bit-level nanoseconde
- Inférence causale joueur
- Reward dense 6 couches
- Validation contextuelle actions

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14
Session: 72
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import json
import time
from typing import Dict, List
import numpy as np

def clean_json(obj):
    """Convertit récursivement numpy types en types Python natifs pour JSON"""
    if isinstance(obj, dict):
        return {k: clean_json(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [clean_json(v) for v in obj]
    if isinstance(obj, np.integer):
        return int(obj)
    if isinstance(obj, np.floating):
        return float(obj)
    if isinstance(obj, np.ndarray):
        return obj.tolist()
    if isinstance(obj, (np.bool_, bool)):
        return bool(obj)
    return obj


# Import agent V27.1
from magen_arc_agi_agent_v27_1_causal import MAGENArcAGIAgentV271

# Import ARC-AGI API
try:
    from arc_agi import Arcade, OperationMode
    ARC_AGI_AVAILABLE = True
except ImportError:
    print("⚠️ arc-agi non installé. Installer avec: pip install arc-agi")
    ARC_AGI_AVAILABLE = False


def load_api_key() -> str:
    """Charge clé API depuis guide competition"""
    guide_path = "guide competition arc3.json"
    
    if not os.path.exists(guide_path):
        raise FileNotFoundError(f"Fichier {guide_path} introuvable")
    
    with open(guide_path, 'r') as f:
        guide = json.load(f)
    
    api_key = guide.get('api_key')
    if not api_key:
        raise ValueError("Clé API introuvable dans guide")
    
    return api_key


def run_game(agent: MAGENArcAGIAgentV271, arcade: 'Arcade', game_id: str, max_actions: int = 50) -> Dict:
    """
    Exécute un jeu complet avec agent V27.1.
    
    Args:
        agent: Agent V27.1
        arcade: Instance Arcade
        game_id: ID du jeu
        max_actions: Actions max par jeu
    
    Returns:
        result: Résultats jeu
    """
    print(f"\n{'='*80}")
    print(f"JEU: {game_id}")
    print(f"{'='*80}")
    
    timestamp_start = int(time.time_ns())
    
    # Créer environnement
    try:
        env = arcade.make(game_id)
    except Exception as e:
        print(f"❌ Erreur création env: {e}")
        return {
            'game_id': game_id,
            'success': False,
            'error': str(e),
            'actions_count': 0,
            'score': 0.0
        }
    
    # Reset agent
    agent.reset()
    
    # Observation initiale
    obs = env.reset()
    
    actions_executed = []
    rewards_received = []
    states_visited = []
    
    done = False
    action_count = 0
    error_occurred = False  # Initialiser avant boucle
    
    while not done and action_count < max_actions:
        # Perception
        perception = agent.perceive(obs)
        states_visited.append(perception)
        
        # Décision - Actions valides ARC-AGI-3 (1-7 + RESET)
        available_actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4',
                           'ACTION5', 'ACTION6', 'ACTION7', 'RESET']
        decision = agent.decide(perception, available_actions)
        
        action_name = decision['action_name']
        
        print(f"\n[Action {action_count+1}/{max_actions}]")
        print(f"  Action: {action_name}")
        print(f"  Agent détecté (causal): {perception.get('agent_detected_causal', False)}")
        print(f"  Confiance: {perception.get('agent_confidence', 0.0):.3f}")
        print(f"  Validation: {decision.get('action_validated', False)}")
        
        # Exécution (convertir string → GameAction si nécessaire)
        try:
            # Vérifier si action_name est dans le mapping
            from arcengine import GameAction
            if hasattr(GameAction, action_name):
                action_to_execute = getattr(GameAction, action_name)
            else:
                action_to_execute = action_name  # Fallback
            
            obs_next = env.step(action_to_execute)
            
            # FIX CRITIQUE #3: Reward dense basé sur delta state
            reward_sparse = 1.0 if obs_next.state == 'LEVEL_COMPLETE' else 0.0
            
            # Calculer reward dense (changement état)
            perception_next = agent.perceive(obs_next)
            reward_dense = 0.0
            
            # Bonus si unique_hashes augmente (nouvel état exploré)
            hashes_before = perception.get('unique_hashes', 0)
            hashes_after = perception_next.get('unique_hashes', 0)
            if hashes_after > hashes_before:
                reward_dense += 0.01 * (hashes_after - hashes_before)
            
            # Bonus si motion détecté (action a un effet)
            motion_after = perception_next.get('motion', 0.0)
            if motion_after > 0.0:
                reward_dense += 0.05
            
            # Bonus si objects change (interaction)
            objects_before = perception.get('objects', 0)
            objects_after = perception_next.get('objects', 0)
            if objects_after != objects_before:
                reward_dense += 0.02
            
            # Reward final = sparse + dense
            reward_env = reward_sparse + reward_dense
            
            done = obs_next.state in ['LEVEL_COMPLETE', 'GAME_OVER']
            error_occurred = False
        except Exception as e:
            print(f"  ⚠️ Erreur API: {e}")
            obs_next = obs  # Garder état précédent
            reward_env = 0.0
            done = False
            error_occurred = True
            
            # Apprendre de l'erreur
            if hasattr(agent, 'action_validator'):
                from core.contextual_action_validator import ActionContext
                grid = perception.get('grid')
                if grid is not None and isinstance(grid, np.ndarray):
                    grid_hash = agent.action_validator._hash_grid(grid)
                else:
                    grid_hash = "unknown"
                
                context = ActionContext(
                    grid_hash=grid_hash,
                    agent_position=perception.get('agent_position_causal'),
                    goal_position=perception.get('goal_position'),
                    entities_count=perception.get('mobile_entities', 0) + perception.get('stable_entities', 0),
                    state_name=perception.get('state', 'UNKNOWN'),
                    timestamp=int(time.time_ns())
                )
                
                agent.action_validator.learn_from_execution(
                    action=action_name,
                    context=context,
                    success=False,
                    error_code=500,
                    error_message=str(e)
                )
        
        # Apprentissage
        if not error_occurred:
            # state_before = perception complète (déjà capturée)
            state_before = perception  # Utiliser perception complète avec tous les champs
            
            # state_after déjà calculé dans reward dense (ligne 149)
            state_after = perception_next  # Utiliser perception complète
            
            agent.learn(state_before, action_name, state_after, reward_env, done)
        
        # Enregistrer
        actions_executed.append(action_name)
        rewards_received.append(reward_env)
        
        # Mise à jour
        if not error_occurred:
            obs = obs_next
        
        action_count += 1
        
        # Afficher état
        if not error_occurred:
            print(f"  État: {obs.state}")
            print(f"  Reward: {reward_env:.3f}")
    
    timestamp_end = int(time.time_ns())
    duration_s = (timestamp_end - timestamp_start) / 1e9
    
    # Résultats
    final_score = sum(rewards_received)
    
    # FIX CRITIQUE #1: Clean numpy types AVANT d'ajouter à results
    result = clean_json({
        'game_id': game_id,
        'success': final_score > 0,
        'actions_count': action_count,
        'score': final_score,
        'duration_s': duration_s,
        'actions_executed': actions_executed,
        'rewards_received': rewards_received,
        'final_state': obs.state if not error_occurred else 'ERROR',
        'agent_stats': agent.get_statistics()
    })
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT: {'✅ SUCCÈS' if result['success'] else '❌ ÉCHEC'}")
    print(f"Score: {final_score:.1f}")
    print(f"Actions: {action_count}")
    print(f"Durée: {duration_s:.2f}s")
    print(f"{'='*80}")
    
    return result


def main():
    """Exécution principale"""
    print("="*80)
    print("MAGEN V27.1 CAUSAL - EXÉCUTION RÉELLE ARC-AGI-3")
    print("="*80)
    print(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Session: 72")
    print()
    
    # Vérifier arc-agi
    if not ARC_AGI_AVAILABLE:
        print("❌ arc-agi non disponible")
        return
    
    # Charger API key
    try:
        api_key = load_api_key()
        print(f"✅ API key chargée")
    except Exception as e:
        print(f"❌ Erreur chargement API key: {e}")
        return
    
    # Créer agent V27.1
    print("\n[INITIALISATION AGENT V27.1]")
    agent = MAGENArcAGIAgentV271()
    print("✅ Agent V27.1 initialisé")
    
    # Créer Arcade
    print("\n[CONNEXION ARC-AGI-3]")
    try:
        arcade = Arcade(arc_api_key=api_key, operation_mode=OperationMode.NORMAL)
        print("✅ Arcade connecté")
    except Exception as e:
        print(f"❌ Erreur connexion Arcade: {e}")
        return
    
    # Liste jeux à tester (10 jeux)
    games_to_test = [
        "ar25",  # Jeu 1
        "cd82",  # Jeu 2
        "ar25",  # Jeu 1 (retry)
        "cd82",  # Jeu 2 (retry)
        "ar25",  # Jeu 1 (retry 2)
        "cd82",  # Jeu 2 (retry 2)
        "ar25",  # Jeu 1 (retry 3)
        "cd82",  # Jeu 2 (retry 3)
        "ar25",  # Jeu 1 (retry 4)
        "cd82",  # Jeu 2 (retry 4)
    ]
    
    # Exécuter jeux
    results = []
    
    for i, game_id in enumerate(games_to_test, 1):
        print(f"\n\n{'#'*80}")
        print(f"# JEU {i}/{len(games_to_test)}: {game_id}")
        print(f"{'#'*80}")
        
        result = run_game(agent, arcade, game_id, max_actions=20)
        results.append(result)
        
        # Pause entre jeux
        if i < len(games_to_test):
            time.sleep(1)
    
    # Résumé global
    print("\n\n" + "="*80)
    print("RÉSUMÉ GLOBAL V27.1")
    print("="*80)
    
    total_games = len(results)
    successful_games = sum(1 for r in results if r['success'])
    total_actions = sum(r['actions_count'] for r in results)
    total_score = sum(r['score'] for r in results)
    total_duration = sum(r['duration_s'] for r in results)
    
    print(f"\nJeux joués: {total_games}")
    print(f"Jeux réussis: {successful_games}/{total_games} ({successful_games/total_games*100:.1f}%)")
    print(f"Actions totales: {total_actions}")
    print(f"Score total: {total_score:.1f}")
    print(f"Durée totale: {total_duration:.2f}s")
    print(f"Actions/seconde: {total_actions/total_duration:.1f}")
    
    # Sauvegarder résultats
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    results_file = f"results_v27_1_real/results_{timestamp}.json"
    
    os.makedirs("results_v27_1_real", exist_ok=True)
    
    with open(results_file, 'w') as f:
        # FIX CRITIQUE #1: Clean numpy types avant JSON dump
        data = clean_json({
            'timestamp': timestamp,
            'agent_version': 'V27.1_CAUSAL',
            'total_games': total_games,
            'successful_games': successful_games,
            'total_actions': total_actions,
            'total_score': total_score,
            'total_duration_s': total_duration,
            'results': results
        })
        json.dump(data, f, indent=2)
    
    print(f"\n✅ Résultats sauvegardés: {results_file}")
    
    # Logs forensic
    print(f"\n✅ Logs forensic disponibles dans: logs_forensic_v27_1/")
    
    print("\n" + "="*80)
    print("EXÉCUTION TERMINÉE")
    print("="*80)


if __name__ == "__main__":
    main()

# Made with Bob