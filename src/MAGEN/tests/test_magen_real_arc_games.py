#!/usr/bin/env python3
"""
Test MAGEN avec jeux ARC-AGI-3 réels
Exécution locale uniquement - Logs forensiques complets
"""

import sys
import os
import argparse
import logging
import time
from pathlib import Path
from typing import List, Dict, Any, Optional
import json

# Ajouter les chemins nécessaires
MAGEN_ROOT = Path(__file__).parent.parent
sys.path.insert(0, str(MAGEN_ROOT))
sys.path.insert(0, str(MAGEN_ROOT / "data" / "arc_agi_3"))

# Imports MAGEN
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception
from agent.arc_agent import ARCAgent
from forensic.lumvorax_logger import LumVoraxLogger

# Imports ARC-AGI-3 (après installation wheels)
try:
    from adapters.arc_agi_adapter import create_adapter, ARCGameEnvironment
    from arcengine import GameAction, GameState
    ARC_AVAILABLE = True
except ImportError as e:
    print(f"⚠️  arc_agi non disponible: {e}")
    print("\n📦 Installation requise:")
    print("  cd lumvorax2/src/MAGEN")
    print("  venv/bin/pip install data/arc_agi_3/arc_agi_3_wheels/*.whl")
    ARC_AVAILABLE = False

logger = logging.getLogger(__name__)


class MAGENARCRunner:
    """Exécuteur MAGEN pour jeux ARC-AGI-3 réels"""
    
    def __init__(
        self,
        games_to_test: List[str],
        forensic_mode: str = "full",
        max_actions_per_level: int = 80
    ):
        self.games_to_test = games_to_test
        self.forensic_mode = forensic_mode
        self.max_actions = max_actions_per_level
        
        # Composants MAGEN
        self.memory = MAGENMemory()
        self.perception = ARCPerception()
        self.agent = ARCAgent(
            memory=self.memory,
            perception=self.perception,
            exploration_rate=0.3  # Plus conservateur pour tests réels
        )
        
        # Adaptateur ARC-AGI-3
        self.adapter = create_adapter(MAGEN_ROOT)
        
        # Résultats
        self.results: Dict[str, Any] = {}
        
        # Logger forensique
        log_dir = MAGEN_ROOT / "logs" / "forensic"
        log_dir.mkdir(parents=True, exist_ok=True)
        self.forensic_logger: Optional[LumVoraxLogger] = None
        
    def setup_forensic_logging(self, game_id: str):
        """Configure le logging forensique pour un jeu"""
        log_dir = MAGEN_ROOT / "logs" / "forensic"
        log_prefix = f"magen_arc_{game_id}"
        
        self.forensic_logger = LumVoraxLogger(
            log_dir=log_dir,
            log_prefix=log_prefix,
            secret_key=os.getenv("LUMVORAX_SECRET_KEY", "magen_default_key_2026")
        )
        
        logger.info(f"🔐 Forensic logging activé: {log_prefix}")
    
    def run_game(self, game_id: str) -> Dict[str, Any]:
        """
        Exécute MAGEN sur un jeu ARC-AGI-3
        
        Returns:
            Résultats détaillés avec métriques
        """
        logger.info(f"\n{'='*60}")
        logger.info(f"🎮 Démarrage jeu: {game_id}")
        logger.info(f"{'='*60}")
        
        # Charger le jeu
        env = self.adapter.load_game(game_id)
        if env is None:
            logger.error(f"❌ Impossible de charger {game_id}")
            return {"error": "load_failed"}
        
        # Setup forensique
        self.setup_forensic_logging(game_id)
        
        start_time = time.time()
        game_results = {
            "game_id": game_id,
            "baseline_actions": env.baseline_actions,
            "levels": [],
            "total_actions": 0,
            "levels_completed": 0,
            "success": False,
            "duration_seconds": 0,
            "forensic_events": 0
        }
        
        with self.forensic_logger as flog:
            flog.log_event(
                "game_start",
                {
                    "game_id": game_id,
                    "baseline_actions": env.baseline_actions,
                    "max_actions": self.max_actions
                }
            )
            
            # Boucle sur les niveaux
            level_idx = 0
            done = False
            
            while not done and level_idx < len(env.baseline_actions):
                level_start = time.time()
                baseline_actions = env.baseline_actions[level_idx]
                
                logger.info(f"\n📊 Niveau {level_idx + 1}/{len(env.baseline_actions)}")
                logger.info(f"   Baseline humain: {baseline_actions} actions")
                
                flog.log_event(
                    "level_start",
                    {
                        "level": level_idx,
                        "baseline_actions": baseline_actions
                    }
                )
                
                # Extraire la grille actuelle
                grid = env.get_grid()
                if grid is None:
                    logger.warning(f"⚠️  Pas de grille pour niveau {level_idx}")
                    break
                
                # Résoudre avec MAGEN
                level_result = self._solve_level_with_magen(
                    env, grid, level_idx, baseline_actions, flog
                )
                
                game_results["levels"].append(level_result)
                game_results["total_actions"] += level_result["actions_taken"]
                
                if level_result["success"]:
                    game_results["levels_completed"] += 1
                    logger.info(f"✅ Niveau {level_idx + 1} réussi en {level_result['actions_taken']} actions")
                else:
                    logger.warning(f"❌ Niveau {level_idx + 1} échoué")
                    break
                
                level_idx += 1
                
                # Vérifier état du jeu
                info = env.get_info()
                if info["state"] in [GameState.WON, GameState.LOST]:
                    done = True
            
            # Résultats finaux
            game_results["duration_seconds"] = time.time() - start_time
            game_results["success"] = (game_results["levels_completed"] == len(env.baseline_actions))
            game_results["forensic_events"] = len(flog.events)
            
            # Calculer score vs baseline
            if game_results["levels_completed"] > 0:
                baseline_total = sum(env.baseline_actions[:game_results["levels_completed"]])
                magen_total = game_results["total_actions"]
                game_results["efficiency_ratio"] = baseline_total / max(magen_total, 1)
                game_results["score"] = (baseline_total / max(magen_total, 1)) ** 2
            else:
                game_results["efficiency_ratio"] = 0.0
                game_results["score"] = 0.0
            
            flog.log_event(
                "game_end",
                {
                    "levels_completed": game_results["levels_completed"],
                    "total_actions": game_results["total_actions"],
                    "score": game_results["score"],
                    "duration": game_results["duration_seconds"]
                }
            )
        
        # Afficher résumé
        self._print_game_summary(game_results)
        
        return game_results
    
    def _solve_level_with_magen(
        self,
        env: ARCGameEnvironment,
        grid: Any,
        level_idx: int,
        baseline_actions: int,
        flog: LumVoraxLogger
    ) -> Dict[str, Any]:
        """Résout un niveau avec MAGEN"""
        
        level_result = {
            "level": level_idx,
            "baseline_actions": baseline_actions,
            "actions_taken": 0,
            "success": False,
            "actions_log": []
        }
        
        # Extraire features avec perception MAGEN
        features = self.perception.extract_features(grid)
        
        flog.log_event(
            "perception",
            {
                "level": level_idx,
                "grid_shape": grid.shape if hasattr(grid, 'shape') else None,
                "features": {k: float(v) if isinstance(v, (int, float)) else str(v) 
                           for k, v in features.items()}
            }
        )
        
        # Boucle d'actions
        for action_idx in range(self.max_actions):
            # Choisir action avec agent MAGEN
            action_name = self.agent.choose_action(grid, features)
            
            # Convertir en GameAction ARC
            arc_action = self._convert_to_arc_action(action_name)
            
            flog.log_event(
                "action",
                {
                    "level": level_idx,
                    "action_idx": action_idx,
                    "action": action_name,
                    "arc_action": str(arc_action)
                }
            )
            
            # Exécuter dans l'environnement
            frame, done = env.step(arc_action)
            
            level_result["actions_taken"] += 1
            level_result["actions_log"].append({
                "action": action_name,
                "arc_action": str(arc_action)
            })
            
            if done or frame is None:
                break
            
            # Vérifier succès niveau
            if env.levels_completed > level_idx:
                level_result["success"] = True
                break
            
            # Mettre à jour grille
            new_grid = env.get_grid()
            if new_grid is not None:
                grid = new_grid
                features = self.perception.extract_features(grid)
        
        # Mémoriser expérience
        self.memory.inject_experience({
            "level": level_idx,
            "grid_features": features,
            "actions_taken": level_result["actions_taken"],
            "success": level_result["success"],
            "baseline_actions": baseline_actions
        })
        
        return level_result
    
    def _convert_to_arc_action(self, magen_action: str) -> GameAction:
        """Convertit une action MAGEN en GameAction ARC"""
        # Mapping MAGEN → ARC
        action_map = {
            "RESET": GameAction.RESET,
            "ACTION1": GameAction.ACTION1,
            "ACTION2": GameAction.ACTION2,
            "ACTION3": GameAction.ACTION3,
            "ACTION4": GameAction.ACTION4,
            "ACTION5": GameAction.ACTION5,
            "ACTION6": GameAction.ACTION6,
            "ACTION7": GameAction.ACTION7
        }
        return action_map.get(magen_action, GameAction.ACTION1)
    
    def _print_game_summary(self, results: Dict[str, Any]):
        """Affiche un résumé des résultats"""
        logger.info(f"\n{'='*60}")
        logger.info(f"📊 RÉSUMÉ: {results['game_id']}")
        logger.info(f"{'='*60}")
        logger.info(f"✅ Niveaux complétés: {results['levels_completed']}/{len(results['baseline_actions'])}")
        logger.info(f"🎯 Actions totales: {results['total_actions']}")
        logger.info(f"📈 Baseline totale: {sum(results['baseline_actions'][:results['levels_completed']])}")
        logger.info(f"⚡ Efficacité: {results['efficiency_ratio']:.2%}")
        logger.info(f"🏆 Score: {results['score']:.4f}")
        logger.info(f"⏱️  Durée: {results['duration_seconds']:.2f}s")
        logger.info(f"🔐 Événements forensiques: {results['forensic_events']}")
        logger.info(f"{'='*60}\n")
    
    def run_all_games(self) -> Dict[str, Any]:
        """Exécute MAGEN sur tous les jeux sélectionnés"""
        logger.info(f"\n🚀 Démarrage tests MAGEN sur {len(self.games_to_test)} jeux")
        logger.info(f"📋 Jeux: {', '.join(self.games_to_test)}")
        
        overall_start = time.time()
        
        for game_id in self.games_to_test:
            try:
                result = self.run_game(game_id)
                self.results[game_id] = result
            except Exception as e:
                logger.error(f"❌ Erreur sur {game_id}: {e}", exc_info=True)
                self.results[game_id] = {"error": str(e)}
        
        overall_duration = time.time() - overall_start
        
        # Résumé global
        self._print_overall_summary(overall_duration)
        
        # Sauvegarder résultats
        self._save_results()
        
        return self.results
    
    def _print_overall_summary(self, duration: float):
        """Affiche le résumé global"""
        logger.info(f"\n{'='*60}")
        logger.info(f"🏁 RÉSUMÉ GLOBAL")
        logger.info(f"{'='*60}")
        
        total_games = len(self.results)
        successful_games = sum(1 for r in self.results.values() if r.get("success", False))
        total_levels = sum(r.get("levels_completed", 0) for r in self.results.values())
        total_actions = sum(r.get("total_actions", 0) for r in self.results.values())
        avg_score = sum(r.get("score", 0) for r in self.results.values()) / max(total_games, 1)
        
        logger.info(f"🎮 Jeux testés: {total_games}")
        logger.info(f"✅ Jeux réussis: {successful_games} ({successful_games/max(total_games,1):.1%})")
        logger.info(f"📊 Niveaux complétés: {total_levels}")
        logger.info(f"🎯 Actions totales: {total_actions}")
        logger.info(f"🏆 Score moyen: {avg_score:.4f}")
        logger.info(f"⏱️  Durée totale: {duration:.2f}s")
        logger.info(f"{'='*60}\n")
    
    def _save_results(self):
        """Sauvegarde les résultats en JSON"""
        results_dir = MAGEN_ROOT / "logs" / "results"
        results_dir.mkdir(parents=True, exist_ok=True)
        
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        results_file = results_dir / f"magen_arc_results_{timestamp}.json"
        
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        logger.info(f"💾 Résultats sauvegardés: {results_file}")


def main():
    """Point d'entrée principal"""
    parser = argparse.ArgumentParser(
        description="Test MAGEN avec jeux ARC-AGI-3 réels (LOCAL UNIQUEMENT)"
    )
    parser.add_argument(
        "--games",
        type=int,
        default=3,
        help="Nombre de jeux à tester (défaut: 3)"
    )
    parser.add_argument(
        "--game-ids",
        nargs="+",
        help="IDs spécifiques de jeux à tester (ex: ar25 bp35)"
    )
    parser.add_argument(
        "--forensic-full",
        action="store_true",
        help="Mode forensique complet (tous les événements)"
    )
    parser.add_argument(
        "--max-actions",
        type=int,
        default=80,
        help="Actions max par niveau (défaut: 80)"
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Mode debug (logs verbeux)"
    )
    
    args = parser.parse_args()
    
    # Configuration logging
    log_level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(
        level=log_level,
        format='%(asctime)s | %(levelname)s | %(message)s',
        handlers=[
            logging.StreamHandler(),
            logging.FileHandler(MAGEN_ROOT / "logs" / "magen_arc_test.log", mode='w')
        ]
    )
    
    # Vérifier disponibilité arc_agi
    if not ARC_AVAILABLE:
        logger.error("❌ arc_agi non disponible - Installation requise")
        return 1
    
    # Découvrir jeux disponibles
    adapter = create_adapter(MAGEN_ROOT)
    available_games = adapter.discover_games()
    
    if not available_games:
        logger.error("❌ Aucun jeu ARC-AGI-3 trouvé")
        return 1
    
    # Sélectionner jeux à tester
    if args.game_ids:
        games_to_test = [g for g in args.game_ids if g in available_games]
        if not games_to_test:
            logger.error(f"❌ Aucun jeu valide dans: {args.game_ids}")
            return 1
    else:
        games_to_test = available_games[:args.games]
    
    logger.info(f"🎮 {len(games_to_test)} jeux sélectionnés: {', '.join(games_to_test)}")
    
    # Créer runner
    forensic_mode = "full" if args.forensic_full else "standard"
    runner = MAGENARCRunner(
        games_to_test=games_to_test,
        forensic_mode=forensic_mode,
        max_actions_per_level=args.max_actions
    )
    
    # Exécuter tests
    try:
        results = runner.run_all_games()
        logger.info("✅ Tests terminés avec succès")
        return 0
    except Exception as e:
        logger.error(f"❌ Erreur fatale: {e}", exc_info=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
