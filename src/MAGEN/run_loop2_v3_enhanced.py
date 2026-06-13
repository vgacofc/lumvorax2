#!/usr/bin/env python3
"""
MAGEN V3 ENHANCED - LOOP 2
Test avec CognitiveLogger + ActionPlanner intégrés
Utilise API Arcade directement comme LOOP 1
"""

import sys
import os
import time
import json
import logging
from pathlib import Path
from datetime import datetime
from typing import List, Tuple
import numpy as np

# Configuration logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s | %(levelname)s | %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

# Ajouter MAGEN au path
MAGEN_ROOT = Path(__file__).parent
sys.path.insert(0, str(MAGEN_ROOT))

# Imports MAGEN
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception
from agent.arc_agent_v3_cognitive import ARCAgentV3Cognitive
from forensic.lumvorax_logger import LumVoraxLogger

# Imports ARC-AGI-3
try:
    from arc_agi import Arcade
    from arcengine import GameAction, GameState
except ImportError as e:
    logger.error(f"❌ Erreur import arc_agi: {e}")
    logger.error("Installation requise: pip install data/arc_agi_3/arc_agi_3_wheels/*.whl")
    sys.exit(1)


def extract_grid_from_frame(frame) -> np.ndarray:
    """Extraire grille numpy depuis frame ARC-AGI-3 (gère listes et arrays 3D)"""
    try:
        # Frame est une liste (format ARC-AGI-3 standard)
        if isinstance(frame, list):
            grid = np.array(frame, dtype=np.int32)
            # Si 3D (channels, H, W), prendre premier canal
            if len(grid.shape) == 3:
                return grid[0]
            return grid
        
        # Frame est FrameDataRaw avec attribut .frame
        if hasattr(frame, 'frame') and frame.frame:
            grid = np.array(frame.frame, dtype=np.int32)
            if len(grid.shape) == 3:
                return grid[0]
            return grid
        
        # Autres formats
        elif hasattr(frame, 'grid'):
            grid = np.array(frame.grid, dtype=np.int32)
            if len(grid.shape) == 3:
                return grid[0]
            return grid
        else:
            logger.warning(f"Frame format non supporté: {type(frame)}")
            return np.zeros((10, 10), dtype=np.int32)
    except Exception as e:
        logger.error(f"Erreur extraction grille: {e}")
        return np.zeros((10, 10), dtype=np.int32)


def get_training_pairs_from_env(env) -> List[Tuple[np.ndarray, np.ndarray]]:
    """
    Extraire training pairs depuis environnement ARC-AGI-3
    
    Note: ARC-AGI-3 ne fournit PAS directement les training pairs
    Cette fonction retourne une liste vide pour l'instant
    TODO: Implémenter extraction depuis métadonnées jeu
    """
    # Pour l'instant, retourner liste vide
    # Les training pairs ne sont pas accessibles via l'API Arcade
    return []


def run_loop2_v3_enhanced(num_games: int = 5, max_actions: int = 200):
    """
    Exécuter LOOP 2 avec Agent V3 Enhanced
    
    Args:
        num_games: Nombre de jeux à tester
        max_actions: Actions max par jeu
    """
    logger.info("="*80)
    logger.info("🚀 MAGEN V3 ENHANCED - LOOP 2")
    logger.info("="*80)
    logger.info(f"Configuration:")
    logger.info(f"  - Jeux à tester: {num_games}")
    logger.info(f"  - Max actions/jeu: {max_actions}")
    logger.info(f"  - CognitiveLogger: ✅ ACTIVÉ")
    logger.info(f"  - ActionPlanner: ✅ ACTIVÉ")
    logger.info(f"  - LumVorax Forensic: ✅ ACTIVÉ")
    logger.info("="*80)
    
    # Créer répertoires
    save_dir = MAGEN_ROOT / "data"
    log_dir = MAGEN_ROOT / "logs"
    cognitive_log_dir = log_dir / "cognitive"
    
    save_dir.mkdir(exist_ok=True)
    log_dir.mkdir(exist_ok=True)
    cognitive_log_dir.mkdir(exist_ok=True)
    
    # Timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Logger forensique
    lum_logger = LumVoraxLogger(
        log_dir=str(log_dir / "forensic"),
        enable_binary=True,
        enable_json=True
    )
    logger.info(f"✅ LumVorax Logger initialisé")
    
    # Initialiser composants MAGEN
    memory = MAGENMemory(save_dir=str(save_dir))
    perception = ARCPerception(max_grid_size=64)
    
    # Agent V3 Enhanced
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        exploration_rate=0.05,  # Très faible (exploitation cognitive)
        max_actions_per_level=max_actions,
        verbose=True
    )
    
    logger.info(f"\n{'='*80}")
    logger.info(f"🎮 DÉBUT TESTS - {num_games} JEUX")
    logger.info(f"{'='*80}\n")
    
    # Obtenir liste jeux disponibles
    try:
        arcade = Arcade()
        # L'API Arcade retourne directement les environnements
        # On utilise les IDs de jeux connus
        # Liste jeux VARIÉS (format correct: juste le code)
        available_games = [
            "bp35",  # Seul jeu testé et fonctionnel
            "ar25",
            "cd82",
            "cn04",
            "dc22",
            "ft09",
            "ka59",
            "lf52",
            "ls20",
            "m0r0"
        ]
        logger.info(f"📋 {len(available_games)} jeux sélectionnés")
        
        # Sélectionner N jeux
        selected_games = available_games[:num_games]
        
    except Exception as e:
        logger.error(f"❌ Erreur accès Arcade: {e}")
        import traceback
        traceback.print_exc()
        return None
    
    # Statistiques
    total_games = 0
    successful_games = 0
    total_actions = 0
    game_results = []
    
    # Tester chaque jeu
    for game_idx, game_id in enumerate(selected_games):
        logger.info(f"\n{'='*80}")
        logger.info(f"🎮 JEU {game_idx+1}/{num_games}: {game_id}")
        logger.info(f"{'='*80}")
        
        try:
            # Créer environnement
            env = arcade.make(game_id)
            
            # Démarrer nouveau jeu avec cognitive logger
            agent.start_new_game(game_id, log_dir=cognitive_log_dir)
            
            # Obtenir training pairs (vide pour l'instant)
            training_pairs = get_training_pairs_from_env(env)
            
            if training_pairs:
                logger.info(f"📚 Training pairs: {len(training_pairs)}")
                agent.analyze_training_pairs(training_pairs)
            else:
                logger.info(f"⚠️ Pas de training pairs disponibles")
            
            # Reset environnement
            obs = env.reset()
            
            # Extraire grille initiale
            current_grid = extract_grid_from_frame(obs)
            logger.info(f"🎯 Grille initiale: {current_grid.shape}")
            
            # Boucle d'actions
            actions_taken = 0
            done = False
            success = False
            last_grid_hash = None
            identical_count = 0
            
            while not done and actions_taken < max_actions:
                # Actions disponibles (filtrer ACTION6 qui crash bp35)
                if isinstance(env.action_space, list):
                    # Liste de GameAction objects filtrés
                    safe_action_objects = [a for a in env.action_space if a.value != 6]
                    available_actions = [a.value for a in safe_action_objects]
                else:
                    available_actions = [i for i in range(env.action_space.n) if i != 6]
                    safe_action_objects = None
                
                # Si aucune action disponible, utiliser ACTION1
                if not available_actions:
                    available_actions = [1]
                    if isinstance(env.action_space, list):
                        safe_action_objects = [a for a in env.action_space if a.value == 1]
                
                # Choisir action (retourne un index dans available_actions)
                action_idx = agent.choose_action(
                    grid=current_grid,
                    available_actions=list(range(len(available_actions))),
                    training_pairs=training_pairs if training_pairs else None,
                    test_input=current_grid
                )
                
                # Convertir index en GameAction réel DEPUIS LA LISTE FILTRÉE
                if isinstance(env.action_space, list):
                    action = safe_action_objects[action_idx]
                else:
                    action = GameAction(available_actions[action_idx])
                
                # Exécuter action (API ARC-AGI-3 retourne FrameDataRaw)
                try:
                    result = env.step(action)
                    obs = result.frame
                    
                    # Déterminer done et reward basé sur GameState
                    done = (result.state != GameState.NOT_FINISHED)
                    reward = 1.0 if result.state == GameState.WIN else 0.0
                    actions_taken += 1
                    
                    # Extraire nouvelle grille
                    current_grid = extract_grid_from_frame(obs)
                    
                    # Vérifier grille identique
                    grid_hash = hash(current_grid.tobytes())
                    if grid_hash == last_grid_hash:
                        identical_count += 1
                        if identical_count >= 10:
                            logger.info(f"⚠️ Grille identique {identical_count} fois - Arrêt")
                            done = True
                    else:
                        identical_count = 0
                    last_grid_hash = grid_hash
                    
                    # Vérifier succès
                    if reward > 0:
                        success = True
                        logger.info(f"\n🎉 SUCCÈS! Reward: {reward}, Actions: {actions_taken}")
                        done = True
                    
                    # Log progression
                    if actions_taken % 20 == 0:
                        logger.info(f"  Actions: {actions_taken}/{max_actions}, Reward: {reward:.2f}")
                
                except Exception as e:
                    logger.error(f"❌ Erreur step: {e}")
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
                "reward": float(reward) if 'reward' in locals() else 0.0
            }
            game_results.append(game_result)
            
            # Log forensique (événement simple)
            logger.info(f"📝 Résultat jeu enregistré: {game_result}")
            
            # Résumé cognitif
            if agent.cognitive_logger:
                cognitive_summary = agent.cognitive_logger.generate_summary()
                reasoning_pct = agent.cognitive_logger.get_reasoning_percentage()
                logger.info(f"\n📊 Cognitive Summary:")
                logger.info(f"   Total events: {cognitive_summary['total_events']}")
                logger.info(f"   Duration: {cognitive_summary['total_duration_ms']:.2f}ms")
                logger.info(f"   Reasoning captured: {reasoning_pct:.1f}%")
                
                game_result["cognitive_summary"] = cognitive_summary
                game_result["reasoning_percentage"] = reasoning_pct
            
            logger.info(f"\n{'='*80}")
            logger.info(f"📊 RÉSULTAT JEU {game_idx+1}")
            logger.info(f"{'='*80}")
            logger.info(f"  Succès: {'✅ OUI' if success else '❌ NON'}")
            logger.info(f"  Actions: {actions_taken}/{max_actions}")
            logger.info(f"{'='*80}")
            
        except Exception as e:
            logger.error(f"❌ ERREUR JEU {game_idx+1}: {e}")
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
    
    logger.info(f"\n{'='*80}")
    logger.info(f"📊 STATISTIQUES FINALES LOOP 2")
    logger.info(f"{'='*80}")
    logger.info(f"  Jeux testés: {total_games}")
    logger.info(f"  Succès: {successful_games} ({success_rate:.1f}%)")
    logger.info(f"  Échecs: {total_games - successful_games}")
    logger.info(f"  Actions totales: {total_actions}")
    logger.info(f"  Actions moyennes: {avg_actions:.1f}")
    logger.info(f"{'='*80}")
    
    # Statistiques agent
    agent_stats = agent.get_statistics()
    logger.info(f"\n📈 STATISTIQUES AGENT V3 ENHANCED")
    logger.info(f"{'='*80}")
    for key, value in agent_stats.items():
        logger.info(f"  {key}: {value}")
    logger.info(f"{'='*80}")
    
    # Sauvegarder résultats
    results_file = log_dir / f"loop2_results_{timestamp}.json"
    results_data = {
        "timestamp": timestamp,
        "version": "V3_ENHANCED",
        "loop": 2,
        "configuration": {
            "num_games": num_games,
            "max_actions": max_actions,
            "exploration_rate": 0.05
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
    
    logger.info(f"\n💾 Résultats sauvegardés: {results_file}")
    
    # Sauvegarder résumé forensique
    logger.info(f"🔒 Logs forensiques LumVorax: {lum_logger.log_dir}")
    logger.info(f"{'='*80}\n")
    
    return results_data


if __name__ == "__main__":
    logger.info("\n" + "="*80)
    logger.info("🧠 MAGEN V3 ENHANCED - LOOP 2")
    logger.info("CognitiveLogger + ActionPlanner + LumVorax Forensic")
    logger.info("="*80 + "\n")
    
    # Exécuter LOOP 2
    results = run_loop2_v3_enhanced(num_games=5, max_actions=200)
    
    if results:
        # Afficher résumé final
        logger.info("\n" + "="*80)
        logger.info("✅ LOOP 2 TERMINÉ")
        logger.info("="*80)
        logger.info(f"Succès: {results['summary']['successful_games']}/{results['summary']['total_games']}")
        logger.info(f"Taux de succès: {results['summary']['success_rate']:.1f}%")
        
        if results['summary']['successful_games'] > 0:
            logger.info(f"\n🎉 OBJECTIF ATTEINT: {results['summary']['successful_games']} SUCCÈS!")
        else:
            logger.info(f"\n⚠️ Aucun succès - Analyse forensique requise pour LOOP 3")
        
        logger.info("="*80 + "\n")

# Made with Bob
