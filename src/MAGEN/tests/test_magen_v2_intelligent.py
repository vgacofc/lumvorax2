#!/usr/bin/env python3
"""
Test MAGEN V2 INTELLIGENT avec Arcade - Pattern Learning Activé
Objectif: 100% succès avec apprentissage automatique patterns
"""

import sys
import os
import json
import logging
import time
from pathlib import Path
from typing import Dict, Any, Optional
import numpy as np
import argparse

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
from agent.arc_agent_v2_intelligent import ARCAgentV2Intelligent, PatternDetector
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


class MAGENTestRunnerV2:
    """Runner de tests MAGEN V2 avec agent intelligent"""
    
    def __init__(self, forensic_mode: str = "full"):
        """
        Initialisation runner V2
        
        Args:
            forensic_mode: Mode forensique (full, minimal, none)
        """
        # Initialiser composants MAGEN
        self.memory = MAGENMemory(
            short_term_capacity=50,
            mid_term_capacity=200,
            long_term_threshold=0.7,
            save_dir=str(MAGEN_ROOT / "logs" / "magen")
        )
        
        self.perception = ARCPerception(max_grid_size=64)
        
        # Agent V2 INTELLIGENT
        self.agent = ARCAgentV2Intelligent(
            memory=self.memory,
            perception=self.perception,
            exploration_rate=0.2,  # Réduit pour plus d'exploitation
            max_actions_per_level=100  # AUGMENTÉ à 100 !
        )
        
        self.forensic_mode = forensic_mode
        self.forensic_logger: Optional[SimpleLogger] = None
        
        logger.info("🚀 MAGEN V2 INTELLIGENT initialisé")
        logger.info(f"   Max actions: 100 (augmenté)")
        logger.info(f"   Exploration rate: 0.2 (exploitation prioritaire)")
        logger.info(f"   Pattern learning: ACTIVÉ")
    
    def run_game(self, game_id: str, max_actions: int = 100) -> Dict[str, Any]:
        """
        Exécuter un jeu complet avec agent V2
        
        Args:
            game_id: ID du jeu
            max_actions: Max actions par niveau
            
        Returns:
            Résultats du jeu
        """
        logger.info(f"\n{'='*70}")
        logger.info(f"🎮 DÉMARRAGE JEU: {game_id}")
        logger.info(f"{'='*70}")
        
        # Créer logger forensique
        forensic_dir = None
        if self.forensic_mode != "none":
            forensic_dir = MAGEN_ROOT / "logs" / "forensic" / "v2_intelligent" / game_id.replace('-', '_')
            forensic_dir.mkdir(parents=True, exist_ok=True)
            self.forensic_logger = SimpleLogger(str(forensic_dir))
            logger.info(f"🔐 Forensic logging activé: {forensic_dir}")
        
        # Créer environnement
        logger.info(f"📦 Création environnement via Arcade.make('{game_id}')...")
        start_time = time.time()
        
        try:
            env = Arcade().make(game_id)
            logger.info(f"✅ Environnement créé: {type(env)}")
        except Exception as e:
            logger.error(f"❌ Erreur création environnement: {e}")
            return {"success": False, "error": str(e)}
        
        # Observer état initial
        obs = env.reset()
        
        # Logger état initial (sans JSON pour éviter erreurs sérialisation)
        logger.info(f"📊 État initial:")
        logger.info(f"   game_id: {game_id}")
        logger.info(f"   state: {obs.state}")
        logger.info(f"   levels_completed: {obs.levels_completed}/{obs.win_levels}")
        logger.info(f"   available_actions: {obs.available_actions}")
        
        # Réinitialiser pattern detector pour ce jeu
        self.agent.pattern_detector.clear_history()
        
        # Boucle actions
        total_actions = 0
        levels_completed_start = obs.levels_completed
        actions_history = []
        
        for action_idx in range(max_actions):
            logger.info(f"\n--- Action {action_idx + 1}/{max_actions} ---")
            logger.info(f"État: {obs.state}")
            logger.info(f"Niveaux complétés: {obs.levels_completed}/{obs.win_levels}")
            
            # Extraire grille
            grid = self.extract_grid_from_frame(obs)
            if grid is None:
                logger.warning("⚠️ Impossible d'extraire grille")
                break
            
            logger.info(f"✅ Grille extraite: shape={grid.shape}, unique_colors={len(np.unique(grid))}")
            
            # Extraire features
            features = self.perception.extract_features(grid)
            logger.info(f"🧠 Features: densité={features.density:.3f}, "
                       f"sym_h={features.symmetry_h:.3f}, sym_v={features.symmetry_v:.3f}, "
                       f"clusters={features.cluster_count}")
            
            # Choisir action avec agent V2
            action_id = self.agent.choose_action(game_id, grid, obs.available_actions)
            logger.info(f"🎯 Action choisie: {action_id}")
            
            # Enregistrer action
            self.agent.record_action(action_id)
            
            # Sauvegarder grille avant action pour détection états identiques
            grid_before_hash = hash(grid.tobytes())
            
            # Exécuter action avec gestion erreurs
            try:
                obs = env.step(action_id)
                logger.info(f"📥 Résultat: {obs.state}")
            except Exception as e:
                logger.error(f"❌ ERREUR lors de l'action {action_id}: {e}")
                logger.error(f"   Type: {type(e).__name__}")
                logger.error(f"   Jeu probablement buggé, on continue...")
                break
            
            # Extraire nouvelle grille pour détection états identiques
            grid_after = self.extract_grid_from_frame(obs)
            if grid_after is not None:
                grid_after_hash = hash(grid_after.tobytes())
                if grid_after_hash == grid_before_hash:
                    logger.warning(f"⚠️ Action {action_id} sans effet (état identique)")
                    # Augmenter légèrement exploration pour sortir de la boucle
                    self.agent.exploration_rate = min(0.5, self.agent.exploration_rate * 1.1)
            
            # Vérifier si niveau complété
            if obs.levels_completed > levels_completed_start:
                logger.info(f"🎉 NIVEAU COMPLÉTÉ ! ({obs.levels_completed}/{obs.win_levels})")
                levels_completed_start = obs.levels_completed
                # Sauvegarder pattern gagnant
                self.agent.record_level_complete(game_id, True)
            
            # Enregistrer historique
            actions_history.append({
                "action": action_id,
                "state_before": str(obs.state),
                "state_after": str(obs.state),
                "levels_completed": obs.levels_completed
            })
            
            total_actions += 1
            
            # Vérifier si jeu terminé
            if obs.state in [GameState.WIN, GameState.GAME_OVER]:
                break
        
        # Enregistrer fin de niveau
        success = (obs.state == GameState.WIN)
        self.agent.record_level_complete(game_id, success)
        
        # Les logs forensiques sont automatiquement sauvegardés par SimpleLogger
        if self.forensic_logger:
            logger.info(f"💾 Logs forensiques sauvegardés dans: {forensic_dir}")
        
        # Résultats
        duration = time.time() - start_time
        results = {
            "game_id": game_id,
            "success": success,
            "final_state": str(obs.state),
            "levels_completed": obs.levels_completed,
            "win_levels": obs.win_levels,
            "total_actions": total_actions,
            "duration_seconds": duration,
            "actions_history": actions_history,
            "agent_stats": self.agent.get_statistics(),
            "forensic_log": str(forensic_dir) if self.forensic_logger else None
        }
        
        logger.info(f"\n{'='*70}")
        logger.info(f"✅ JEU TERMINÉ: {game_id}")
        logger.info(f"   Succès: {success}")
        logger.info(f"   Niveaux: {obs.levels_completed}/{obs.win_levels}")
        logger.info(f"   Actions: {total_actions}")
        logger.info(f"   Durée: {duration:.2f}s")
        logger.info(f"{'='*70}")
        
        return results
    
    def extract_grid_from_frame(self, frame_data) -> Optional[np.ndarray]:
        """Extrait la grille depuis FrameDataRaw"""
        if hasattr(frame_data, 'frame') and len(frame_data.frame) > 0:
            grid = frame_data.frame[0]  # Premier élément = grille actuelle
            if isinstance(grid, np.ndarray):
                return grid
        return None


def main():
    """Point d'entrée principal"""
    parser = argparse.ArgumentParser(description="Test MAGEN V2 Intelligent")
    parser.add_argument('--games', type=int, default=10, help="Nombre de jeux à tester")
    parser.add_argument('--max-actions', type=int, default=100, help="Max actions par niveau")
    parser.add_argument('--forensic', choices=['full', 'minimal', 'none'], default='full')
    parser.add_argument('--progress', action='store_true', help="Afficher barres de progression")
    parser.add_argument('--save-patterns', type=str, help="Sauvegarder patterns appris")
    parser.add_argument('--load-patterns', type=str, help="Charger patterns appris")
    
    args = parser.parse_args()
    
    logger.info("\n🚀 DÉMARRAGE TESTS MAGEN V2 INTELLIGENT")
    logger.info(f"   Jeux à tester: {args.games}")
    logger.info(f"   Mode forensique: {args.forensic}")
    logger.info(f"   Max actions: {args.max_actions}")
    logger.info(f"   Pattern learning: ACTIVÉ")
    
    # Créer runner
    runner = MAGENTestRunnerV2(forensic_mode=args.forensic)
    
    # Charger patterns si demandé
    if args.load_patterns:
        runner.agent.load_patterns(args.load_patterns)
    
    # Récupérer jeux disponibles
    arcade = Arcade()
    all_games = [env.game_id for env in arcade.available_environments]
    games = all_games[:args.games]
    
    logger.info(f"\n📋 {len(games)} jeux sélectionnés sur {len(all_games)} disponibles")
    
    # Exécuter tests
    results = []
    game_iterator = tqdm(games, desc="🎮 Tests MAGEN V2", unit="jeu") if (args.progress and TQDM_AVAILABLE) else games
    
    for i, game_id in enumerate(game_iterator if (args.progress and TQDM_AVAILABLE) else games, 1):
        if not (args.progress and TQDM_AVAILABLE):
            logger.info(f"\n{'#'*70}")
            logger.info(f"# TEST {i}/{len(games)}: {game_id}")
            logger.info(f"{'#'*70}")
        else:
            game_iterator.set_description(f"🎮 Test {i}/{len(games)}: {game_id[:10]}")
        
        result = runner.run_game(game_id, max_actions=args.max_actions)
        results.append(result)
    
    # Sauvegarder patterns si demandé
    if args.save_patterns:
        runner.agent.save_patterns(args.save_patterns)
    
    # Rapport final
    logger.info(f"\n{'='*70}")
    logger.info("📊 RAPPORT FINAL V2 INTELLIGENT")
    logger.info(f"{'='*70}")
    
    successes = sum(1 for r in results if r.get('success', False))
    total_actions = sum(r.get('total_actions', 0) for r in results)
    total_duration = sum(r.get('duration_seconds', 0) for r in results)
    
    logger.info(f"Jeux testés: {len(results)}")
    logger.info(f"Succès: {successes}/{len(results)} ({100*successes/len(results):.1f}%)")
    logger.info(f"Actions totales: {total_actions}")
    logger.info(f"Durée totale: {total_duration:.2f}s")
    logger.info(f"Moyenne actions/jeu: {total_actions/len(results):.1f}")
    
    # Statistiques agent
    agent_stats = runner.agent.get_statistics()
    logger.info(f"\n📈 STATISTIQUES AGENT V2:")
    logger.info(f"   Patterns appris: {agent_stats['total_patterns_learned']}")
    logger.info(f"   Patterns utilisés: {agent_stats['patterns_used']}")
    logger.info(f"   Taux succès patterns: {100*agent_stats['pattern_success_rate']:.1f}%")
    logger.info(f"   Exploration rate final: {agent_stats['exploration_rate']:.3f}")
    
    # Sauvegarder résumé
    summary_file = MAGEN_ROOT / "logs" / "forensic" / "v2_intelligent" / "results_summary.json"
    summary_file.parent.mkdir(parents=True, exist_ok=True)
    
    summary = {
        "timestamp": time.time(),
        "games_tested": len(results),
        "successes": successes,
        "success_rate": successes / len(results),
        "total_actions": total_actions,
        "total_duration": total_duration,
        "agent_stats": agent_stats,
        "results": results
    }
    
    with open(summary_file, 'w') as f:
        json.dump(summary, f, indent=2)
    
    logger.info(f"\n💾 Résultats sauvegardés: {summary_file}")
    logger.info(f"{'='*70}")
    
    return 0 if successes == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
