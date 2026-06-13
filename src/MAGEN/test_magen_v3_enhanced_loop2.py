"""
MAGEN V3 ENHANCED - LOOP 2
Test avec CognitiveLogger + ActionPlanner intégrés
Objectif: 1 SUCCÈS MINIMUM sur 5 jeux ARC-AGI-3
"""

import sys
import time
import json
from pathlib import Path
from datetime import datetime
import numpy as np

# Ajouter le répertoire parent au path
sys.path.append(str(Path(__file__).parent))

# Imports MAGEN
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception
from agent.arc_agent_v3_cognitive import ARCAgentV3Cognitive
from adapters.arc_agi_adapter import ARCAGIAdapter
from forensic.lum_vorax_logger import LumVoraxLogger


def run_loop2_test(num_games: int = 5, max_actions: int = 200):
    """
    Exécuter LOOP 2 avec V3 Enhanced
    
    Args:
        num_games: Nombre de jeux à tester
        max_actions: Actions max par jeu
    """
    print("="*80)
    print("🚀 MAGEN V3 ENHANCED - LOOP 2")
    print("="*80)
    print(f"Configuration:")
    print(f"  - Jeux à tester: {num_games}")
    print(f"  - Max actions/jeu: {max_actions}")
    print(f"  - CognitiveLogger: ✅ ACTIVÉ")
    print(f"  - ActionPlanner: ✅ ACTIVÉ")
    print(f"  - LumVorax Forensic: ✅ ACTIVÉ")
    print("="*80)
    
    # Créer répertoires
    base_dir = Path(__file__).parent
    save_dir = base_dir / "data"
    log_dir = base_dir / "logs"
    cognitive_log_dir = log_dir / "cognitive"
    
    save_dir.mkdir(exist_ok=True)
    log_dir.mkdir(exist_ok=True)
    cognitive_log_dir.mkdir(exist_ok=True)
    
    # Timestamp pour cette exécution
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Initialiser logger forensique LumVorax
    lum_logger = LumVoraxLogger(
        log_dir=log_dir,
        experiment_name=f"loop2_v3_enhanced_{timestamp}"
    )
    lum_logger.log_experiment_start({
        "version": "V3_ENHANCED",
        "loop": 2,
        "num_games": num_games,
        "max_actions": max_actions,
        "cognitive_logger": True,
        "action_planner": True
    })
    
    # Initialiser composants MAGEN
    memory = MAGENMemory(save_dir=str(save_dir))
    perception = ARCPerception(max_grid_size=64)
    
    # Créer agent V3 Enhanced
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        exploration_rate=0.05,  # Très faible (exploitation cognitive)
        max_actions_per_level=max_actions,
        verbose=True
    )
    
    # Créer adaptateur ARC-AGI-3
    adapter = ARCAGIAdapter(
        dataset_path=base_dir / "arc-agi_evaluation_challenges.json",
        verbose=True
    )
    
    # Statistiques globales
    total_games = 0
    successful_games = 0
    total_actions = 0
    game_results = []
    
    print(f"\n{'='*80}")
    print(f"🎮 DÉBUT TESTS - {num_games} JEUX")
    print(f"{'='*80}\n")
    
    # Tester N jeux
    for game_idx in range(num_games):
        game_id = f"game_{game_idx+1:03d}"
        
        print(f"\n{'='*80}")
        print(f"🎮 JEU {game_idx+1}/{num_games}: {game_id}")
        print(f"{'='*80}")
        
        try:
            # Créer environnement
            env = adapter.create_environment(game_index=game_idx)
            
            # Démarrer nouveau jeu avec cognitive logger
            agent.start_new_game(game_id, log_dir=cognitive_log_dir)
            
            # Obtenir training pairs et test input
            training_pairs = adapter.get_training_pairs(game_idx)
            test_input = adapter.get_test_input(game_idx)
            
            print(f"\n📚 Training pairs: {len(training_pairs)}")
            print(f"🧪 Test input shape: {test_input.shape}")
            
            # Analyser training pairs
            agent.analyze_training_pairs(training_pairs)
            
            # Prédire test output
            predicted_output = agent.predict_test_output(test_input)
            
            if predicted_output is not None:
                print(f"✅ Prédiction générée: {predicted_output.shape}")
            else:
                print(f"⚠️ Aucune prédiction générée")
            
            # Reset environnement
            obs, info = env.reset()
            
            # Extraire grille initiale
            if hasattr(obs, 'grid'):
                current_grid = obs.grid
            elif isinstance(obs, dict) and 'grid' in obs:
                current_grid = obs['grid']
            else:
                current_grid = obs
            
            print(f"\n🎯 Grille initiale: {current_grid.shape}")
            
            # Boucle d'actions
            actions_taken = 0
            done = False
            success = False
            last_grid_hash = None
            identical_count = 0
            
            while not done and actions_taken < max_actions:
                # Obtenir actions disponibles
                available_actions = list(range(env.action_space.n))
                
                # Choisir action (avec planification si prédiction disponible)
                action = agent.choose_action(
                    grid=current_grid,
                    available_actions=available_actions,
                    training_pairs=training_pairs,
                    test_input=test_input
                )
                
                # Exécuter action
                try:
                    obs, reward, terminated, truncated, info = env.step(action)
                    done = terminated or truncated
                    actions_taken += 1
                    
                    # Extraire nouvelle grille
                    if hasattr(obs, 'grid'):
                        current_grid = obs.grid
                    elif isinstance(obs, dict) and 'grid' in obs:
                        current_grid = obs['grid']
                    else:
                        current_grid = obs
                    
                    # Vérifier si grille identique (éviter boucle infinie)
                    grid_hash = hash(current_grid.tobytes())
                    if grid_hash == last_grid_hash:
                        identical_count += 1
                        if identical_count >= 10:
                            print(f"\n⚠️ Grille identique {identical_count} fois - Arrêt")
                            done = True
                    else:
                        identical_count = 0
                    last_grid_hash = grid_hash
                    
                    # Vérifier succès
                    if reward > 0:
                        success = True
                        print(f"\n🎉 SUCCÈS! Reward: {reward}, Actions: {actions_taken}")
                        done = True
                    
                    # Log progression
                    if actions_taken % 20 == 0:
                        print(f"  Actions: {actions_taken}/{max_actions}, Reward: {reward:.2f}")
                
                except Exception as e:
                    print(f"\n❌ Erreur step: {e}")
                    done = True
            
            # Enregistrer résultat
            agent.record_level_complete(success, actions_taken)
            
            # Statistiques
            total_games += 1
            total_actions += actions_taken
            if success:
                successful_games += 1
            
            # Résultat du jeu
            game_result = {
                "game_id": game_id,
                "game_index": game_idx,
                "success": success,
                "actions_taken": actions_taken,
                "reward": float(reward) if 'reward' in locals() else 0.0,
                "predicted_output_generated": predicted_output is not None,
                "predicted_output_shape": predicted_output.shape if predicted_output is not None else None
            }
            game_results.append(game_result)
            
            # Log forensique
            lum_logger.log_game_result(game_result)
            
            # Générer résumé cognitif
            if agent.cognitive_logger:
                cognitive_summary = agent.cognitive_logger.generate_summary()
                reasoning_pct = agent.cognitive_logger.get_reasoning_percentage()
                print(f"\n📊 Cognitive Summary:")
                print(f"   Total events: {cognitive_summary['total_events']}")
                print(f"   Duration: {cognitive_summary['total_duration_ms']:.2f}ms")
                print(f"   Reasoning captured: {reasoning_pct:.1f}%")
                
                game_result["cognitive_summary"] = cognitive_summary
                game_result["reasoning_percentage"] = reasoning_pct
            
            print(f"\n{'='*80}")
            print(f"📊 RÉSULTAT JEU {game_idx+1}")
            print(f"{'='*80}")
            print(f"  Succès: {'✅ OUI' if success else '❌ NON'}")
            print(f"  Actions: {actions_taken}/{max_actions}")
            print(f"  Prédiction: {'✅ Générée' if predicted_output is not None else '❌ Non générée'}")
            print(f"{'='*80}")
            
        except Exception as e:
            print(f"\n❌ ERREUR JEU {game_idx+1}: {e}")
            import traceback
            traceback.print_exc()
            
            game_result = {
                "game_id": game_id,
                "game_index": game_idx,
                "success": False,
                "error": str(e)
            }
            game_results.append(game_result)
            total_games += 1
    
    # Statistiques finales
    success_rate = (successful_games / total_games * 100) if total_games > 0 else 0
    avg_actions = total_actions / total_games if total_games > 0 else 0
    
    print(f"\n{'='*80}")
    print(f"📊 STATISTIQUES FINALES LOOP 2")
    print(f"{'='*80}")
    print(f"  Jeux testés: {total_games}")
    print(f"  Succès: {successful_games} ({success_rate:.1f}%)")
    print(f"  Échecs: {total_games - successful_games}")
    print(f"  Actions totales: {total_actions}")
    print(f"  Actions moyennes: {avg_actions:.1f}")
    print(f"{'='*80}")
    
    # Statistiques agent
    agent_stats = agent.get_statistics()
    print(f"\n📈 STATISTIQUES AGENT V3 ENHANCED")
    print(f"{'='*80}")
    for key, value in agent_stats.items():
        print(f"  {key}: {value}")
    print(f"{'='*80}")
    
    # Sauvegarder résultats
    results_file = log_dir / f"loop2_results_{timestamp}.json"
    results_data = {
        "timestamp": timestamp,
        "version": "V3_ENHANCED",
        "loop": 2,
        "configuration": {
            "num_games": num_games,
            "max_actions": max_actions,
            "exploration_rate": 0.05,
            "cognitive_logger": True,
            "action_planner": True
        },
        "summary": {
            "total_games": total_games,
            "successful_games": successful_games,
            "success_rate": success_rate,
            "total_actions": total_actions,
            "avg_actions": avg_actions
        },
        "agent_statistics": agent_stats,
        "game_results": game_results
    }
    
    with open(results_file, 'w') as f:
        json.dump(results_data, f, indent=2, default=str)
    
    print(f"\n💾 Résultats sauvegardés: {results_file}")
    
    # Log forensique final
    lum_logger.log_experiment_end(results_data)
    lum_logger.finalize()
    
    print(f"\n🔒 Logs forensiques LumVorax finalisés")
    print(f"{'='*80}\n")
    
    return results_data


if __name__ == "__main__":
    print("\n" + "="*80)
    print("🧠 MAGEN V3 ENHANCED - LOOP 2")
    print("CognitiveLogger + ActionPlanner + LumVorax Forensic")
    print("="*80 + "\n")
    
    # Exécuter LOOP 2
    results = run_loop2_test(num_games=5, max_actions=200)
    
    # Afficher résumé final
    print("\n" + "="*80)
    print("✅ LOOP 2 TERMINÉ")
    print("="*80)
    print(f"Succès: {results['summary']['successful_games']}/{results['summary']['total_games']}")
    print(f"Taux de succès: {results['summary']['success_rate']:.1f}%")
    
    if results['summary']['successful_games'] > 0:
        print(f"\n🎉 OBJECTIF ATTEINT: {results['summary']['successful_games']} SUCCÈS!")
    else:
        print(f"\n⚠️ Aucun succès - Analyse forensique requise pour LOOP 3")
    
    print("="*80 + "\n")

# Made with Bob
