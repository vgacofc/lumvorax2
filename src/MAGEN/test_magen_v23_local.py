"""
Test MAGEN V23 Agent - Local
Test sur 3 jeux ARC-AGI-3 avec Action Discovery + World Model
"""

import os
import sys
import json
import time
from datetime import datetime
from arc_agi import Arcade
from arcengine import GameAction

# Import agent V23
from magen_arc_agi_agent_v23 import MAGENArcAgiAgentV23


def test_magen_v23_local():
    """Test agent MAGEN V23 sur 3 jeux locaux"""
    
    print("\n" + "="*80)
    print("🧪 TEST AGENT MAGEN V23 - ARC-AGI-3 LOCAL")
    print("="*80)
    print("Architecture: V22 (TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18)")
    print("            + V23 (ActionDiscovery+WorldModel+InformationGainReward)")
    print("Test: 3 jeux avec max 200 actions par jeu")
    print("="*80 + "\n")
    
    # Configuration
    api_key = os.getenv('ARC_API_KEY')
    if not api_key:
        print("❌ Erreur: ARC_API_KEY non définie")
        return
    
    session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Initialiser agent
    agent = MAGENArcAgiAgentV23()
    
    # Initialiser arcade
    print("📡 Initialisation ARC-AGI Arcade...")
    from arc_agi.base import OperationMode
    arcade = Arcade(
        arc_api_key=api_key,
        operation_mode=OperationMode.NORMAL  # Mode normal (pas de compétition)
    )
    print(f"✅ Arcade initialisé")
    print(f"🔒 Mode: NORMAL (test local)\n")
    
    # Récupérer jeux
    print("🎮 Récupération liste des jeux...")
    envs = arcade.get_environments()
    games = [env.game_id for env in envs]
    print(f"✅ {len(games)} jeux disponibles\n")
    
    # Tester sur 3 jeux
    num_games = 3
    max_actions_per_game = 200
    
    results = []
    
    print(f"🎯 Test: {num_games} jeux")
    print(f"🔒 Mode: PRIVÉ")
    print(f"⏱️  Début: {datetime.now().isoformat()}\n")
    
    for i, game_id in enumerate(games[:num_games]):
        print("="*80)
        print(f"📍 Progression: {i+1}/{num_games} ({(i+1)/num_games*100:.1f}%)")
        print("="*80)
        print(f"\n🎮 Jeu: {game_id}\n")
        
        # Réinitialiser agent pour nouveau jeu
        agent.reset_for_new_game()
        
        # Créer environnement
        env = arcade.make(game_id)
        
        # Jouer
        start_time = time.time()
        obs = env.reset()
        actions_taken = 0
        
        print(f"[MAGEN V23] Début jeu - Max actions: {max_actions_per_game}\n")
        
        while obs.state not in ['WIN', 'GAME_OVER'] and actions_taken < max_actions_per_game:
            # Perception
            grid_obs = agent.perceive(obs)
            
            # Décision
            available_actions = [f"ACTION{i}" for i in range(1, 8)] + ["RESET"]
            decision = agent.decide(grid_obs, available_actions)
            
            # Exécution
            obs = env.step(decision.action)
            actions_taken += 1
            
            # Apprentissage
            grid_obs_after = agent.perceive(obs)
            agent.learn(grid_obs, decision, grid_obs_after)
            
            # Logging périodique
            if actions_taken % 20 == 0:
                print(f"[MAGEN V23] Actions: {actions_taken}/{max_actions_per_game} | Niveaux: {obs.levels_completed} | État: {obs.state}")
        
        end_time = time.time()
        duration = end_time - start_time
        
        # Résultats
        result = {
            'game_id': game_id,
            'score': float(obs.levels_completed),  # FIX: obs.score n'existe pas
            'levels_completed': obs.levels_completed,
            'actions_taken': actions_taken,
            'duration': duration,
            'success': obs.levels_completed > 0,
            'final_state': str(obs.state)
        }
        results.append(result)
        
        print(f"\n[MAGEN V23] Résultats finaux:")
        print(f"  Niveaux complétés: {obs.levels_completed}")
        print(f"  Actions: {actions_taken}")
        print(f"  Succès: {result['success']}")
        print(f"  État final: {obs.state}")
        print(f"  Durée: {duration:.2f}s\n")
        
        # Statistiques agent
        stats = agent.get_statistics()
        print(f"[MAGEN V23] Statistiques agent:")
        print(f"  Phase exploration: {stats['exploration_actions']} actions")
        print(f"  Phase exploitation: {stats['exploitation_actions']} actions")
        print(f"  Actions découvertes: {stats['actions_discovered']}")
        print(f"  World model accuracy: {stats['world_model_accuracy']:.2f}")
        print(f"  Avg info gain reward: {stats['avg_info_gain_reward']:.3f}")
        print()
    
    # Résumé final
    print("\n" + "="*80)
    print("📊 RÉSUMÉ FINAL - MAGEN V23")
    print("="*80)
    print(f"Session: {session_id}")
    print(f"Jeux joués: {num_games}")
    print(f"Score total: {sum(r['score'] for r in results):.2f}")
    print(f"Niveaux complétés: {sum(r['levels_completed'] for r in results)}")
    print(f"Succès: {sum(1 for r in results if r['success'])}/{num_games}")
    print(f"Durée totale: {sum(r['duration'] for r in results):.2f}s")
    print("="*80)
    print("🔒 Mode: PRIVÉ (aucune soumission publique)")
    print("="*80 + "\n")
    
    # Sauvegarder résultats
    results_file = f"results_magen_v23_{session_id}.json"
    with open(results_file, 'w') as f:
        json.dump({
            'session_id': session_id,
            'agent_version': 'V23',
            'games_played': num_games,
            'results': results,
            'final_stats': agent.get_statistics()
        }, f, indent=2)
    
    print(f"💾 Résultats sauvegardés: {results_file}\n")
    
    print("="*80)
    print("✅ TEST TERMINÉ")
    print("="*80)
    print(f"Résultats sauvegardés dans: {results_file}")
    print("="*80 + "\n")


if __name__ == "__main__":
    test_magen_v23_local()

# Made with Bob
