#!/usr/bin/env python3
"""
Test MAGEN ARC-AGI Agent V25 ENRICHED - Itération #10
Session 70 - Solution A Révisée Complète

Modifications V25 Enriched:
1. StateEncoder V25 (256D) avec relations spatiales + hash topologique
2. Pénalisation répétition (3/5/10 dernières actions)
3. Cycle avoidance actif (force_rare_action, inject_noise, reset_exploration)
4. Reward shaping intrinsèque (nouveauté, diversité, désynchronisation)
5. Threshold graphe causal: 0.85 → 0.70
6. Exploration: 28.5% → 80%

Métriques Cibles:
- Graphe: >10 nodes, >20 edges (vs 1, 1)
- Plans: >1 généré (vs 0)
- Score: >0/3 (vs 0/3)
- Cycles: <100 (vs 570)
- Hashes uniques: >50 (vs ~1)
- Exploration: 80% (vs 28.5%)
"""

import os
import sys
import json
import time
from datetime import datetime
from pathlib import Path

# Ajouter le répertoire parent au PYTHONPATH
sys.path.insert(0, str(Path(__file__).parent))

# Importer l'agent V25 Enriched
from magen_arc_agi_agent_v25_enriched import MAGENArcAgiAgentV25Enriched

def test_agent_v25_enriched_local():
    """Test complet de l'agent V25 Enriched en local."""
    
    print("=" * 80)
    print("TEST MAGEN ARC-AGI AGENT V25 ENRICHED - ITÉRATION #10")
    print("Session 70 - Solution A Révisée Complète")
    print("=" * 80)
    print()
    
    # Vérifier la clé API
    api_key = os.environ.get('ARC_API_KEY')
    if not api_key:
        print("❌ ERREUR: Variable d'environnement ARC_API_KEY non définie")
        print("   Définir avec: export ARC_API_KEY='votre_clé'")
        return False
    
    print(f"✅ Clé API trouvée: {api_key[:8]}...{api_key[-8:]}")
    print()
    
    # Créer l'agent V25 Enriched
    print("📦 Création de l'agent V25 Enriched...")
    try:
        agent = MAGENArcAgiAgentV25Enriched()
        print("✅ Agent V25 Enriched créé avec succès")
        print(f"   - StateEncoder: 256D (vs 128D V25)")
        print(f"   - Threshold graphe: 0.70 (vs 0.85 V25)")
        print(f"   - Exploration: 80% (vs 28.5% V25)")
        print(f"   - Pénalisation répétition: ACTIVE")
        print(f"   - Cycle avoidance: ACTIF")
        print(f"   - Reward shaping: INTRINSÈQUE")
        print()
    except Exception as e:
        print(f"❌ ERREUR lors de la création de l'agent: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # Jeux de test (3 jeux interactifs)
    test_games = [
        'e21a174a',  # Jeu 1
        '3906de3d',  # Jeu 2
        'a699fb00'   # Jeu 3
    ]
    
    # Statistiques globales
    global_stats = {
        'total_games': len(test_games),
        'total_actions': 0,
        'total_rewards': 0.0,
        'total_intrinsic_rewards': 0.0,
        'games_completed': 0,
        'games_failed': 0,
        'total_cycles': 0,
        'unique_hashes': set(),
        'unique_transitions': set(),
        'exploration_actions': 0,
        'exploitation_actions': 0,
        'start_time': time.time()
    }
    
    # Tester chaque jeu
    for game_idx, game_id in enumerate(test_games, 1):
        print(f"🎮 JEU {game_idx}/{len(test_games)}: {game_id}")
        print("-" * 80)
        
        try:
            # Réinitialiser l'agent pour chaque jeu
            agent.reset()
            
            # Créer l'environnement (import conditionnel)
            try:
                from arc_prize_2024_env import ArcPrize2024Env
            except ImportError:
                print("⚠️ Module arc_prize_2024_env non trouvé, utilisation du mock")
                # Mock simple pour tests locaux
                class ArcPrize2024Env:
                    def __init__(self, api_key):
                        self.action_space = type('obj', (object,), {'n': 10})()
                    def reset(self, task_id):
                        import numpy as np
                        obs = {'frame': np.zeros((10, 10), dtype=np.int32)}
                        info = {}
                        return obs, info
                    def step(self, action):
                        import numpy as np
                        obs = {'frame': np.zeros((10, 10), dtype=np.int32)}
                        reward = 0.0
                        terminated = False
                        truncated = False
                        info = {}
                        return obs, reward, terminated, truncated, info
                    def close(self):
                        pass
            
            env = ArcPrize2024Env(api_key=api_key)
            
            # Démarrer le jeu
            obs, info = env.reset(task_id=game_id)
            print(f"✅ Jeu démarré: {game_id}")
            print(f"   Frame shape: {obs['frame'].shape}")
            print()
            
            # Boucle de jeu (max 200 actions)
            max_actions = 200
            done = False
            episode_reward = 0.0
            episode_intrinsic_reward = 0.0
            action_count = 0
            
            while not done and action_count < max_actions:
                action_count += 1
                
                # Percevoir l'état
                grid_obs = agent.perceive(obs)
                
                # Décider de l'action (convertir en strings pour l'agent)
                num_actions = getattr(env.action_space, 'n', 10)
                available_actions = [f"ACTION{i}" for i in range(num_actions)]
                action_decision = agent.decide_exploration(grid_obs, available_actions)
                
                # L'action retournée est un GameAction (enum)
                game_action = action_decision.action
                
                # Convertir GameAction en index pour l'environnement
                # GameAction.ACTION1 = 1, ACTION2 = 2, etc.
                action_idx = game_action.value - 1 if hasattr(game_action, 'value') else 0
                
                # Convertir en string pour learn()
                action_str = f"ACTION{action_idx}"
                
                # Exécuter l'action
                obs_next, reward, terminated, truncated, info = env.step(action_idx)
                done = terminated or truncated
                
                # Apprendre (passer l'action string à l'agent)
                grid_obs_next = agent.perceive(obs_next)
                agent.learn(grid_obs, action_str, grid_obs_next, reward, done)
                
                # Statistiques
                episode_reward += reward
                intrinsic_reward = agent.stats.get('total_intrinsic_reward', 0.0) - episode_intrinsic_reward
                episode_intrinsic_reward = agent.stats.get('total_intrinsic_reward', 0.0)
                
                # Affichage périodique
                if action_count % 50 == 0:
                    print(f"   Action {action_count}/{max_actions}: reward={reward:.2f}, intrinsic={intrinsic_reward:.3f}")
                
                # Mettre à jour l'observation
                obs = obs_next
            
            # Fermer l'environnement
            env.close()
            
            # Résultats du jeu
            print()
            print(f"📊 RÉSULTATS JEU {game_idx}:")
            print(f"   Actions: {action_count}")
            print(f"   Reward total: {episode_reward:.2f}")
            print(f"   Reward intrinsèque: {episode_intrinsic_reward:.3f}")
            print(f"   Statut: {'✅ COMPLÉTÉ' if done else '⏸️ TRONQUÉ'}")
            print()
            
            # Statistiques agent (sauvegarder pour usage global)
            game_stats = agent.get_stats()
            print(f"📈 STATISTIQUES AGENT:")
            print(f"   Graphe causal: {game_stats.get('causal_graph_nodes', 0)} nodes, {game_stats.get('causal_graph_edges', 0)} edges")
            print(f"   Plans générés: {game_stats.get('plans_generated', 0)}")
            print(f"   Cycles détectés: {game_stats.get('cycles_detected', 0)}")
            print(f"   Hashes uniques: {game_stats.get('unique_hashes', 0)}")
            print(f"   Transitions uniques: {game_stats.get('unique_transitions', 0)}")
            print(f"   Exploration: {game_stats.get('exploration_actions', 0)} ({game_stats.get('exploration_actions', 0) / action_count * 100:.1f}%)")
            print(f"   Exploitation: {game_stats.get('exploitation_actions', 0)} ({game_stats.get('exploitation_actions', 0) / action_count * 100:.1f}%)")
            print()
            
            # Mettre à jour statistiques globales
            global_stats['total_actions'] += action_count
            global_stats['total_rewards'] += episode_reward
            global_stats['total_intrinsic_rewards'] += episode_intrinsic_reward
            global_stats['total_cycles'] += game_stats.get('cycles_detected', 0)
            global_stats['exploration_actions'] += game_stats.get('exploration_actions', 0)
            global_stats['exploitation_actions'] += game_stats.get('exploitation_actions', 0)
            
            if done:
                global_stats['games_completed'] += 1
            else:
                global_stats['games_failed'] += 1
            
        except Exception as e:
            print(f"❌ ERREUR lors du test du jeu {game_id}: {e}")
            import traceback
            traceback.print_exc()
            global_stats['games_failed'] += 1
        
        print()
    
    # Résultats globaux
    elapsed_time = time.time() - global_stats['start_time']
    
    print("=" * 80)
    print("📊 RÉSULTATS GLOBAUX V25 ENRICHED - ITÉRATION #10")
    print("=" * 80)
    print()
    print(f"🎮 Jeux testés: {global_stats['total_games']}")
    print(f"   ✅ Complétés: {global_stats['games_completed']}")
    print(f"   ❌ Échoués: {global_stats['games_failed']}")
    print(f"   📊 Score: {global_stats['games_completed']}/{global_stats['total_games']}")
    print()
    print(f"🎯 Actions:")
    print(f"   Total: {global_stats['total_actions']}")
    print(f"   Exploration: {global_stats['exploration_actions']} ({global_stats['exploration_actions'] / max(global_stats['total_actions'], 1) * 100:.1f}%)")
    print(f"   Exploitation: {global_stats['exploitation_actions']} ({global_stats['exploitation_actions'] / max(global_stats['total_actions'], 1) * 100:.1f}%)")
    print()
    print(f"💰 Rewards:")
    print(f"   Total: {global_stats['total_rewards']:.2f}")
    print(f"   Intrinsèque: {global_stats['total_intrinsic_rewards']:.3f}")
    print()
    print(f"🔄 Cycles détectés: {global_stats['total_cycles']}")
    print()
    print(f"⏱️ Temps d'exécution: {elapsed_time:.2f}s")
    print()
    
    # Récupérer les stats finales de l'agent
    final_stats = agent.get_stats()
    
    # Comparaison avec V25
    print("=" * 80)
    print("📈 COMPARAISON V25 vs V25 ENRICHED")
    print("=" * 80)
    print()
    print("Métriques                  | V25 (Test #9) | V25 Enriched (Test #10) | Objectif")
    print("-" * 80)
    print(f"Graphe nodes               | 1             | {final_stats.get('causal_graph_nodes', 0):15} | >10")
    print(f"Graphe edges               | 1             | {final_stats.get('causal_graph_edges', 0):15} | >20")
    print(f"Plans générés              | 0             | {final_stats.get('plans_generated', 0):15} | >1")
    print(f"Score                      | 0/3           | {global_stats['games_completed']}/3{' ' * 12} | >0/3")
    print(f"Cycles détectés            | 570           | {global_stats['total_cycles']:15} | <100")
    print(f"Hashes uniques             | ~1            | {final_stats.get('unique_hashes', 0):15} | >50")
    print(f"Exploration %              | 28.5%         | {global_stats['exploration_actions'] / max(global_stats['total_actions'], 1) * 100:14.1f}% | 80%")
    print(f"Intrinsic rewards          | 0.0           | {global_stats['total_intrinsic_rewards']:14.3f} | >5.0")
    print()
    
    # Verdict
    print("=" * 80)
    print("🎯 VERDICT")
    print("=" * 80)
    print()
    
    success_criteria = {
        'graphe_nodes': final_stats.get('causal_graph_nodes', 0) > 10,
        'graphe_edges': final_stats.get('causal_graph_edges', 0) > 20,
        'plans_generated': final_stats.get('plans_generated', 0) > 1,
        'score': global_stats['games_completed'] > 0,
        'cycles': global_stats['total_cycles'] < 100,
        'hashes': final_stats.get('unique_hashes', 0) > 50,
        'exploration': (global_stats['exploration_actions'] / max(global_stats['total_actions'], 1)) > 0.70,
        'intrinsic_rewards': global_stats['total_intrinsic_rewards'] > 5.0
    }
    
    success_count = sum(success_criteria.values())
    total_criteria = len(success_criteria)
    
    print(f"Critères de succès: {success_count}/{total_criteria}")
    print()
    for criterion, passed in success_criteria.items():
        status = "✅" if passed else "❌"
        print(f"   {status} {criterion}")
    print()
    
    if success_count >= 6:
        print("🎉 SUCCÈS: V25 Enriched montre des améliorations significatives!")
        print("   → Prêt pour itération suivante ou soumission Kaggle")
    elif success_count >= 4:
        print("⚠️ PROGRÈS: V25 Enriched montre des améliorations partielles")
        print("   → Analyser logs et itérer")
    else:
        print("❌ ÉCHEC: V25 Enriched nécessite des corrections supplémentaires")
        print("   → Analyser logs forensiques et réviser architecture")
    print()
    
    # Sauvegarder les résultats
    results_file = f"test_results_v25_enriched_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    results_data = {
        'version': 'V25_ENRICHED',
        'iteration': 10,
        'timestamp': datetime.now().isoformat(),
        'global_stats': {
            'total_games': global_stats['total_games'],
            'games_completed': global_stats['games_completed'],
            'games_failed': global_stats['games_failed'],
            'total_actions': global_stats['total_actions'],
            'total_rewards': global_stats['total_rewards'],
            'total_intrinsic_rewards': global_stats['total_intrinsic_rewards'],
            'total_cycles': global_stats['total_cycles'],
            'exploration_actions': global_stats['exploration_actions'],
            'exploitation_actions': global_stats['exploitation_actions'],
            'elapsed_time': elapsed_time
        },
        'agent_stats': final_stats,
        'success_criteria': success_criteria,
        'success_count': success_count
    }
    
    with open(results_file, 'w') as f:
        json.dump(results_data, f, indent=2)
    
    print(f"💾 Résultats sauvegardés: {results_file}")
    print()
    
    return success_count >= 4

if __name__ == '__main__':
    success = test_agent_v25_enriched_local()
    sys.exit(0 if success else 1)

# Made with Bob
