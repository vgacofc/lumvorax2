#!/usr/bin/env python3
"""
Test MAGEN avec Arcade.make() direct - Jeux ARC-AGI-3 réels
Exécution locale uniquement - Logs forensiques complets
"""

import sys
import os
import json
import logging
import time
from pathlib import Path
from typing import Dict, Any, Optional
import numpy as np

# Import tqdm pour barres de progression
try:
    from tqdm import tqdm
    TQDM_AVAILABLE = True
except ImportError:
    TQDM_AVAILABLE = False
    tqdm = lambda x, **kwargs: x  # Fallback si tqdm non disponible

# Ajouter les chemins nécessaires
MAGEN_ROOT = Path(__file__).parent.parent
sys.path.insert(0, str(MAGEN_ROOT))

# Imports MAGEN
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception
from agent.arc_agent import ARCAgent
from forensic.simple_logger import SimpleLogger

# Imports ARC-AGI-3
from arc_agi import Arcade
from arcengine import GameAction, GameState

# Configuration logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s | %(levelname)s | %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)


class MAGENArcadeRunner:
    """Exécuteur MAGEN avec Arcade direct"""
    
    def __init__(self, forensic_mode: str = "full", max_actions: int = 80):
        self.forensic_mode = forensic_mode
        self.max_actions = max_actions
        
        # Composants MAGEN
        self.memory = MAGENMemory()
        self.perception = ARCPerception()
        self.agent = ARCAgent(
            memory=self.memory,
            perception=self.perception,
            exploration_rate=0.3
        )
        
        # Arcade
        self.arcade = Arcade()
        
        # Résultats
        self.results: Dict[str, Any] = {}
        
        # Logger forensique
        self.forensic_logger: Optional[LumVoraxLogger] = None
        
    def setup_forensic_logging(self, game_id: str):
        """Configure le logging forensique pour un jeu"""
        log_dir = MAGEN_ROOT / "logs" / "forensic" / "arcade_direct" / game_id.replace('-', '_')
        log_dir.mkdir(parents=True, exist_ok=True)
        
        self.forensic_logger = SimpleLogger(log_dir=str(log_dir))
        
        logger.info(f"🔐 Forensic logging activé: {log_dir}")
        
        # Log événement démarrage
        self.forensic_logger.log_event(
            event_type="GAME_START",
            data={
                "game_id": game_id,
                "timestamp": time.time(),
                "forensic_mode": self.forensic_mode
            }
        )
    
    def extract_grid_from_frame(self, frame_data) -> Optional[np.ndarray]:
        """Extrait la grille depuis FrameDataRaw"""
        try:
            # FrameDataRaw.frame est une liste de numpy arrays
            # frame[0] contient la grille du niveau actuel
            if hasattr(frame_data, 'frame') and len(frame_data.frame) > 0:
                grid = frame_data.frame[0]
                if isinstance(grid, np.ndarray):
                    return grid
            
            logger.warning(f"⚠️  Impossible d'extraire grille depuis: {type(frame_data)}")
            return None
            
        except Exception as e:
            logger.error(f"❌ Erreur extraction grille: {e}")
            return None
    
    def run_game(self, game_id: str) -> Dict[str, Any]:
        """
        Exécute MAGEN sur un jeu ARC-AGI-3 via Arcade
        
        Returns:
            Résultats détaillés avec métriques
        """
        logger.info(f"\n{'='*70}")
        logger.info(f"🎮 DÉMARRAGE JEU: {game_id}")
        logger.info(f"{'='*70}")
        
        start_time = time.time()
        total_actions = 0  # Initialiser dès le début
        
        # Setup forensic
        self.setup_forensic_logging(game_id)
        
        try:
            # Créer environnement avec Arcade
            logger.info(f"📦 Création environnement via Arcade.make('{game_id}')...")
            env = self.arcade.make(game_id)
            logger.info(f"✅ Environnement créé: {type(env)}")
            
            # Observer état initial (FrameDataRaw est un objet Pydantic)
            initial_obs = env.observation_space
            logger.info(f"📊 État initial: {initial_obs}")
            
            # Log forensique état initial (accès attributs Pydantic)
            self.forensic_logger.log_event(
                event_type="ENV_CREATED",
                data={
                    "game_id": game_id,
                    "state": getattr(initial_obs, 'state', 'UNKNOWN'),
                    "levels_completed": getattr(initial_obs, 'levels_completed', 0),
                    "win_levels": getattr(initial_obs, 'win_levels', 0),
                    "available_actions": getattr(initial_obs, 'available_actions', [])
                }
            )
            
            # Métriques
            levels_completed = 0
            actions_history = []
            
            # Boucle principale
            done = False
            current_level = 0
            
            while not done and total_actions < self.max_actions:
                # Observer état actuel (FrameDataRaw)
                obs = env.observation_space
                state = getattr(obs, 'state', 'UNKNOWN')
                
                logger.info(f"\n--- Action {total_actions + 1}/{self.max_actions} ---")
                logger.info(f"État: {state}")
                logger.info(f"Niveaux complétés: {getattr(obs, 'levels_completed', 0)}/{getattr(obs, 'win_levels', 0)}")
                
                # Extraire grille si disponible (passer obs, pas env!)
                grid = self.extract_grid_from_frame(obs)
                
                if grid is not None:
                    # Grille extraite avec succès !
                    logger.info(f"✅ Grille extraite: shape={grid.shape}, unique_colors={len(np.unique(grid))}")
                    
                    # Extraire features avec _flood_fill CORRIGÉ (version itérative)
                    try:
                        features = self.perception.extract_features(grid)
                        logger.info(f"🧠 Features: densité={features.density:.3f}, sym_h={features.symmetry_h:.3f}, sym_v={features.symmetry_v:.3f}, clusters={features.cluster_count}")
                    except Exception as e:
                        logger.warning(f"⚠️  Erreur extraction features: {e}")
                        features = None
                    
                    # Choisir action aléatoire simple (TODO: implémenter stratégie)
                    import random
                    action_id = random.choice([1, 2, 3, 4, 5, 6, 7])
                    logger.info(f"🎲 Action choisie: {action_id}")
                else:
                    # Pas de grille, action par défaut
                    available = getattr(obs, 'available_actions', [6])
                    action_id = available[0] if available else 6
                    logger.info(f"⚠️  Pas de grille, action par défaut: {action_id}")
                
                logger.info(f"🎯 Action choisie: {action_id}")
                
                # Log forensique action
                self.forensic_logger.log_event(
                    event_type="ACTION_CHOSEN",
                    data={
                        "action_id": action_id,
                        "total_actions": total_actions,
                        "state": state,
                        "has_grid": grid is not None
                    }
                )
                
                # Exécuter action
                try:
                    frame_data = env.step(action_id)
                    
                    # Observer résultat (FrameDataRaw)
                    new_obs = env.observation_space
                    new_state = getattr(new_obs, 'state', 'UNKNOWN')
                    
                    logger.info(f"📥 Résultat: {new_state}")
                    
                    # Vérifier progression
                    new_levels = getattr(new_obs, 'levels_completed', 0)
                    if new_levels > levels_completed:
                        logger.info(f"🎉 NIVEAU COMPLÉTÉ ! ({new_levels}/{getattr(new_obs, 'win_levels', 0)})")
                        levels_completed = new_levels
                        
                        # Log forensique succès
                        self.forensic_logger.log_event(
                            event_type="LEVEL_COMPLETED",
                            data={
                                "level": levels_completed,
                                "total_actions": total_actions,
                                "timestamp": time.time()
                            }
                        )
                    
                    # Vérifier fin
                    if new_state in ['WON', 'LOST', 'FINISHED']:
                        done = True
                        logger.info(f"🏁 JEU TERMINÉ: {new_state}")
                    
                    # Historique
                    actions_history.append({
                        "action": action_id,
                        "state_before": state,
                        "state_after": new_state,
                        "levels_completed": new_levels
                    })
                    
                    total_actions += 1
                    
                except Exception as e:
                    logger.error(f"❌ Erreur step: {e}")
                    done = True
            
            # Résultats finaux
            duration = time.time() - start_time
            
            final_obs = env.observation_space
            final_state = getattr(final_obs, 'state', 'UNKNOWN')
            final_levels = getattr(final_obs, 'levels_completed', 0)
            win_levels = getattr(final_obs, 'win_levels', 0)
            
            success = final_state == 'WON' or final_levels == win_levels
            
            results = {
                "game_id": game_id,
                "success": success,
                "final_state": final_state,
                "levels_completed": final_levels,
                "win_levels": win_levels,
                "total_actions": total_actions,
                "duration_seconds": duration,
                "actions_history": actions_history,
                "memory_stats": {
                    "short_term": len(self.memory.short_term)
                }
            }
            
            # Log forensique final
            self.forensic_logger.log_event(
                event_type="GAME_FINISHED",
                data=results
            )
            
            # Sauvegarder logs forensiques
            if self.forensic_logger:
                log_file = self.forensic_logger.save_logs()
                results["forensic_log"] = str(log_file)
                logger.info(f"💾 Logs forensiques sauvegardés: {log_file}")
            
            logger.info(f"\n{'='*70}")
            logger.info(f"✅ JEU TERMINÉ: {game_id}")
            logger.info(f"   Succès: {success}")
            logger.info(f"   Niveaux: {final_levels}/{win_levels}")
            logger.info(f"   Actions: {total_actions}")
            logger.info(f"   Durée: {duration:.2f}s")
            logger.info(f"{'='*70}\n")
            
            return results
            
        except Exception as e:
            logger.error(f"❌ ERREUR CRITIQUE: {e}", exc_info=True)
            
            # Log forensique erreur
            if self.forensic_logger:
                self.forensic_logger.log_event(
                    event_type="ERROR",
                    data={
                        "error": str(e),
                        "game_id": game_id,
                        "total_actions": total_actions
                    }
                )
                self.forensic_logger.save_logs()
            
            return {
                "game_id": game_id,
                "success": False,
                "error": str(e),
                "total_actions": total_actions
            }


def main():
    """Point d'entrée principal - MODE CLAUDE PILOT"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Test MAGEN avec Arcade direct - MODE CLAUDE PILOT")
    parser.add_argument("--games", type=int, default=1, help="Nombre de jeux à tester")
    parser.add_argument("--forensic", choices=["full", "minimal", "none"], default="full")
    parser.add_argument("--max-actions", type=int, default=80)
    parser.add_argument("--progress", action="store_true", help="Afficher barres de progression")
    
    args = parser.parse_args()
    
    # Charger liste des jeux
    metadata_file = MAGEN_ROOT / "data" / "arc_agi_3" / "arc_agi_3_games" / "metadata.json"
    
    if not metadata_file.exists():
        logger.error(f"❌ Fichier metadata introuvable: {metadata_file}")
        return 1
    
    with open(metadata_file) as f:
        metadata = json.load(f)
    
    games = metadata["games"][:args.games]
    
    logger.info(f"\n🚀 DÉMARRAGE TESTS MAGEN ARCADE")
    logger.info(f"   Jeux à tester: {len(games)}")
    logger.info(f"   Mode forensique: {args.forensic}")
    logger.info(f"   Max actions: {args.max_actions}\n")
    
    # Créer runner
    runner = MAGENArcadeRunner(
        forensic_mode=args.forensic,
        max_actions=args.max_actions
    )
    
    # Exécuter tests avec barre de progression
    all_results = []
    
    # Barre de progression globale
    game_iterator = tqdm(games, desc="🎮 Tests MAGEN", unit="jeu") if (args.progress and TQDM_AVAILABLE) else games
    
    for i, game_id in enumerate(game_iterator if (args.progress and TQDM_AVAILABLE) else games, 1):
        if not (args.progress and TQDM_AVAILABLE):
            logger.info(f"\n{'#'*70}")
            logger.info(f"# TEST {i}/{len(games)}: {game_id}")
            logger.info(f"{'#'*70}")
        else:
            game_iterator.set_description(f"🎮 Test {i}/{len(games)}: {game_id[:10]}")
        
        result = runner.run_game(game_id)
        all_results.append(result)
        
        # Pause entre jeux
        if i < len(games):
            time.sleep(1)
    
    # Rapport final
    logger.info(f"\n{'='*70}")
    logger.info(f"📊 RAPPORT FINAL")
    logger.info(f"{'='*70}")
    
    successes = sum(1 for r in all_results if r.get("success", False))
    total_actions = sum(r.get("total_actions", 0) for r in all_results)
    total_duration = sum(r.get("duration_seconds", 0) for r in all_results)
    
    logger.info(f"Jeux testés: {len(all_results)}")
    logger.info(f"Succès: {successes}/{len(all_results)} ({successes/len(all_results)*100:.1f}%)")
    logger.info(f"Actions totales: {total_actions}")
    logger.info(f"Durée totale: {total_duration:.2f}s")
    logger.info(f"Moyenne actions/jeu: {total_actions/len(all_results):.1f}")
    
    # Sauvegarder résultats
    results_file = MAGEN_ROOT / "logs" / "forensic" / "arcade_direct" / "results_summary.json"
    results_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(results_file, 'w') as f:
        json.dump({
            "timestamp": time.time(),
            "games_tested": len(all_results),
            "successes": successes,
            "total_actions": total_actions,
            "total_duration": total_duration,
            "results": all_results
        }, f, indent=2)
    
    logger.info(f"\n💾 Résultats sauvegardés: {results_file}")
    logger.info(f"{'='*70}\n")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
