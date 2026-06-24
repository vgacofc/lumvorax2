"""
Test MAGEN V25 Agent - Local avec Logs Forensiques Profonds
Test sur 3 jeux ARC-AGI-3 avec Planification Causale
Architecture: V22 + V23 + V24 + V25 (CausalTransitionGraph + BFS Planning)
"""

import os
import sys
import json
import time
from datetime import datetime
from pathlib import Path
from arc_agi import Arcade  # type: ignore
from arcengine import GameAction  # type: ignore

# Import agent V25
from magen_arc_agi_agent_v25 import MAGENArcAgiAgentV25


def test_magen_v25_local():
    """Test agent MAGEN V25 sur 3 jeux locaux avec logs forensiques profonds"""
    
    print("\n" + "="*80)
    print("🧪 TEST AGENT MAGEN V25 - ARC-AGI-3 LOCAL + LOGS FORENSIQUES")
    print("="*80)
    print("Architecture V22: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17 + C18")
    print("Architecture V23: ActionDiscovery + WorldModel + InformationGainReward")
    print("Architecture V24: StateEncoder + GoalGrounder + SemanticActionLibrary")
    print("Architecture V25: CausalTransitionGraph + BFS Planning + Cycle Detection")
    print("Nouveautés V25:")
    print("  - CausalTransitionGraph: Graphe états → actions → objectifs")
    print("  - BFS Planning: Planification dirigée vers goal")
    print("  - Cycle Detection: Évitement répétitions trajectoire")
    print("  - Priorité décision: Planning > Exploitation > Exploration")
    print("  - Logs forensiques: Traçabilité bit-level + nanoseconde")
    print("Test: 3 jeux avec max 200 actions par jeu")
    print("="*80 + "\n")
    
    # Configuration
    api_key = os.getenv('ARC_API_KEY')
    if not api_key:
        print("❌ Erreur: ARC_API_KEY non définie")
        print("💡 Définir avec: export ARC_API_KEY='votre_clé'")
        return
    
    session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Créer répertoire logs forensiques
    logs_dir = Path(f"logs_forensic_v25_{session_id}")
    logs_dir.mkdir(exist_ok=True)
    print(f"📁 Répertoire logs forensiques: {logs_dir}\n")
    
    # Initialiser agent avec logs forensiques
    forensic_log_path = logs_dir / "forensic_log.jsonl"
    agent = MAGENArcAgiAgentV25(forensic_log_path=str(forensic_log_path))
    
    print(f"✅ Agent V25 initialisé avec logs forensiques")
    print(f"📝 Fichier logs: {forensic_log_path}\n")
    
    # Initialiser arcade
    print("📡 Initialisation ARC-AGI Arcade...")
    from arc_agi.base import OperationMode  # type: ignore
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
    print(f"🔒 Mode: PRIVÉ (aucune soumission)")
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
        
        print(f"[MAGEN V25] Début jeu - Max actions: {max_actions_per_game}")
        print(f"[MAGEN V25] Logs forensiques: ACTIVÉS (bit-level + nanoseconde)")
        print(f"[MAGEN V25] Planification: BFS + Cycle Detection\n")
        
        while obs.state not in ['WIN', 'GAME_OVER'] and actions_taken < max_actions_per_game:
            # Perception (avec StateEncoder)
            grid_obs = agent.perceive(obs)
            
            # Décision (avec CausalTransitionGraph + BFS Planning)
            available_actions = [f"ACTION{i}" for i in range(1, 8)] + ["RESET"]
            decision = agent.decide(grid_obs, available_actions)
            
            # Exécution
            obs_before = obs
            obs = env.step(decision.action)
            actions_taken += 1
            
            # Apprentissage (avec CausalTransitionGraph)
            grid_obs_after = agent.perceive(obs)
            agent.learn(grid_obs, decision, grid_obs_after)
            
            # Logging périodique avec statistiques V25
            if actions_taken % 20 == 0:
                stats = agent.get_statistics()
                print(f"[MAGEN V25] Actions: {actions_taken}/{max_actions_per_game}")
                print(f"  Niveaux: {obs.levels_completed} | État: {obs.state}")
                print(f"  Phase: {stats['current_phase']}")
                print(f"  Graphe: {stats['causal_graph_stats'].get('total_nodes', 0)} nodes, {stats['causal_graph_stats'].get('total_edges', 0)} edges")
                print(f"  Plans générés: {stats['causal_graph_stats'].get('total_paths_found', 0)}")
                print(f"  Cycles détectés: {stats['causal_graph_stats'].get('cycles_detected', 0)}")
                print(f"  Actions depuis plans: {stats.get('planning_actions', 0)}")
        
        end_time = time.time()
        duration = end_time - start_time
        
        # Résultats
        result = {
            'game_id': game_id,
            'score': float(obs.levels_completed),
            'levels_completed': obs.levels_completed,
            'actions_taken': actions_taken,
            'duration': duration,
            'success': obs.levels_completed > 0,
            'final_state': str(obs.state)
        }
        results.append(result)
        
        print(f"\n[MAGEN V25] Résultats finaux:")
        print(f"  Niveaux complétés: {obs.levels_completed}")
        print(f"  Actions: {actions_taken}")
        print(f"  Succès: {result['success']}")
        print(f"  État final: {obs.state}")
        print(f"  Durée: {duration:.2f}s\n")
        
        # Statistiques agent V25 détaillées
        stats = agent.get_statistics()
        print(f"[MAGEN V25] Statistiques détaillées:")
        print(f"  Phase exploration: {stats.get('exploration_actions', 0)} actions")
        print(f"  Phase exploitation: {stats.get('exploitation_actions', 0)} actions")
        print(f"  Phase planning: {stats.get('planning_actions', 0)} actions")
        print(f"  Phase actuelle: {stats['current_phase']}")
        print(f"\n  CausalTransitionGraph (V25):")
        print(f"    Nodes: {stats['causal_graph_stats'].get('total_nodes', 0)}")
        print(f"    Edges: {stats['causal_graph_stats'].get('total_edges', 0)}")
        print(f"    Goals identifiés: {stats['causal_graph_stats'].get('goal_nodes', 0)}")
        print(f"    Plans générés: {stats['causal_graph_stats'].get('total_paths_found', 0)}")
        print(f"    Plans réussis: {stats['causal_graph_stats'].get('successful_plans', 0)}")
        print(f"    Cycles détectés: {stats['causal_graph_stats'].get('cycles_detected', 0)}")
        print(f"    Temps planning moyen: {stats['causal_graph_stats'].get('avg_planning_time_ns', 0)/1000:.2f} µs")
        print(f"\n  StateEncoder (V24):")
        print(f"    Embeddings générés: {stats['state_encoder_stats'].get('embeddings_generated', 0)}")
        print(f"    Temps encoding moyen: {stats['state_encoder_stats'].get('avg_encoding_time_ns', 0)/1000:.2f} µs")
        print(f"\n  GoalGrounder (V24):")
        print(f"    Actions prioritaires: {stats['goal_grounder_stats'].get('priority_actions_count', 0)}")
        print(f"    Actions abandonnées: {stats['goal_grounder_stats'].get('abandoned_actions_count', 0)}")
        print(f"    Goal progress moyen: {stats['goal_grounder_stats'].get('avg_goal_progress', 0.0):.3f}")
        print(f"\n  SemanticActionLibrary (V24):")
        print(f"    Actions sémantiques découvertes: {stats['semantic_library_stats'].get('semantic_actions_discovered', 0)}")
        print(f"    Confidence moyenne: {stats['semantic_library_stats'].get('avg_semantic_confidence', 0.0):.3f}")
        print(f"\n  World Model (V23):")
        print(f"    Accuracy: {stats.get('world_model_stats', {}).get('accuracy', 0.0):.2f}")
        print(f"    Prédictions: {stats.get('world_model_stats', {}).get('predictions', 0)}")
        print()
        
        # Sauvegarder logs forensiques du jeu
        game_log_file = logs_dir / f"game_{game_id}_{session_id}.json"
        with open(game_log_file, 'w') as f:
            json.dump({
                'game_id': game_id,
                'result': result,
                'statistics': stats
            }, f, indent=2)
        print(f"💾 Logs jeu sauvegardés: {game_log_file}\n")
    
    # Résumé final
    print("\n" + "="*80)
    print("📊 RÉSUMÉ FINAL - MAGEN V25")
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
    
    # Statistiques globales V25
    final_stats = agent.get_statistics()
    print("📈 STATISTIQUES GLOBALES V25:")
    print(f"  Total nodes graphe: {final_stats['causal_graph_stats'].get('total_nodes', 0)}")
    print(f"  Total edges graphe: {final_stats['causal_graph_stats'].get('total_edges', 0)}")
    print(f"  Total plans générés: {final_stats['causal_graph_stats'].get('total_paths_found', 0)}")
    print(f"  Total plans réussis: {final_stats['causal_graph_stats'].get('successful_plans', 0)}")
    print(f"  Total cycles détectés: {final_stats['causal_graph_stats'].get('cycles_detected', 0)}")
    print(f"  Total actions planning: {final_stats.get('planning_actions', 0)}")
    print(f"  Total embeddings: {final_stats['state_encoder_stats'].get('embeddings_generated', 0)}")
    print(f"  Avg goal progress: {final_stats['goal_grounder_stats'].get('avg_goal_progress', 0.0):.3f}")
    print()
    
    # Sauvegarder résultats globaux
    results_file = logs_dir / f"results_summary_{session_id}.json"
    with open(results_file, 'w') as f:
        json.dump({
            'session_id': session_id,
            'agent_version': 'V25',
            'architecture': {
                'v22': 'TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17 + C18',
                'v23': 'ActionDiscovery + WorldModel + InformationGainReward',
                'v24': 'StateEncoder + GoalGrounder + SemanticActionLibrary',
                'v25': 'CausalTransitionGraph + BFS Planning + Cycle Detection'
            },
            'games_played': num_games,
            'results': results,
            'final_stats': final_stats,
            'forensic_log_path': str(forensic_log_path)
        }, f, indent=2)
    
    print(f"💾 Résultats globaux sauvegardés: {results_file}")
    print(f"📝 Logs forensiques complets: {forensic_log_path}")
    print(f"📁 Tous les fichiers dans: {logs_dir}\n")
    
    # Analyse logs forensiques
    print("="*80)
    print("🔍 ANALYSE LOGS FORENSIQUES")
    print("="*80)
    
    if forensic_log_path.exists():
        with open(forensic_log_path, 'r') as f:
            forensic_logs = [json.loads(line) for line in f]
        
        print(f"Total événements forensiques: {len(forensic_logs)}")
        
        # Compter par type
        event_types = {}
        for log in forensic_logs:
            event_type = log['event_type']
            event_types[event_type] = event_types.get(event_type, 0) + 1
        
        print("\nÉvénements par type:")
        for event_type, count in sorted(event_types.items(), key=lambda x: x[1], reverse=True):
            print(f"  {event_type}: {count}")
        
        # Compter par composant
        components = {}
        for log in forensic_logs:
            component = log['component']
            components[component] = components.get(component, 0) + 1
        
        print("\nÉvénements par composant:")
        for component, count in sorted(components.items(), key=lambda x: x[1], reverse=True):
            print(f"  {component}: {count}")
        
        print()
    
    print("="*80)
    print("✅ TEST TERMINÉ")
    print("="*80)
    print(f"Résultats dans: {logs_dir}")
    print(f"Logs forensiques: {forensic_log_path}")
    print("="*80 + "\n")
    
    # Vérification objectif RÈGLE #3
    total_score = sum(r['score'] for r in results)
    if total_score > 0:
        print("🎉 OBJECTIF ATTEINT: Score >0/3 ✅")
    else:
        print("⚠️  OBJECTIF NON ATTEINT: Score = 0/3 - Itération nécessaire")
    print()


if __name__ == "__main__":
    test_magen_v25_local()

# Made with Bob - Session 70