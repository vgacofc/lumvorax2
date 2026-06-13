#!/usr/bin/env python3
"""
Adaptateur ARC-AGI-3 pour MAGEN - Version 2 Simplifiée
Utilise directement le pattern des agents officiels
"""

import sys
import os
import json
import logging
import importlib.util
from pathlib import Path
from typing import Optional, Dict, Any, List
import numpy as np

# Ajouter les chemins nécessaires
MAGEN_ROOT = Path(__file__).parent.parent
sys.path.insert(0, str(MAGEN_ROOT))
sys.path.insert(0, str(MAGEN_ROOT / "data" / "arc_agi_3"))

try:
    from arc_agi import EnvironmentWrapper
    from arcengine import GameAction, GameState, FrameData
except ImportError as e:
    print(f"⚠️  Erreur import arc_agi: {e}")
    sys.exit(1)

logger = logging.getLogger(__name__)


class SimpleMAGENAgent:
    """Agent MAGEN simplifié pour tests ARC-AGI-3"""
    
    def __init__(self, game_module, game_id: str, magen_memory, magen_perception, magen_agent):
        self.game_id = game_id
        self.magen_memory = magen_memory
        self.magen_perception = magen_perception
        self.magen_agent = magen_agent
        
        # Créer logger
        env_logger = logging.getLogger(f"arc_env_{game_id}")
        
        # Créer environnement
        self.env = EnvironmentWrapper(
            game_module,
            logger=env_logger,
            scorecard_id=f"magen_{game_id}"
        )
        
        self.frames: List[FrameData] = []
        self.action_count = 0
        self.max_actions = 80
        
    def run(self) -> Dict[str, Any]:
        """Exécute l'agent sur le jeu"""
        results = {
            "game_id": self.game_id,
            "levels_completed": 0,
            "total_actions": 0,
            "success": False,
            "levels": []
        }
        
        # Boucle principale
        done = False
        while not done and self.action_count < self.max_actions:
            # Obtenir frame actuel
            if len(self.frames) == 0:
                # Premier frame - doit RESET
                action = GameAction.RESET
            else:
                latest_frame = self.frames[-1]
                
                # Vérifier si terminé
                if latest_frame.state == GameState.WIN:
                    results["success"] = True
                    done = True
                    break
                elif latest_frame.state in [GameState.NOT_PLAYED, GameState.GAME_OVER]:
                    action = GameAction.RESET
                else:
                    # Choisir action avec MAGEN
                    action = self._choose_magen_action(latest_frame)
            
            # Exécuter action
            try:
                frame_data = self.env.step(action, data={}, reasoning={})
                if frame_data:
                    self.frames.append(frame_data)
                    self.action_count += 1
                    
                    # Logger
                    logger.debug(f"Action {self.action_count}: {action.name}, State: {frame_data.state}")
                else:
                    logger.warning("Pas de frame retourné")
                    done = True
            except Exception as e:
                logger.error(f"Erreur step: {e}")
                done = True
        
        # Résultats finaux
        if len(self.frames) > 0:
            final_frame = self.frames[-1]
            results["levels_completed"] = final_frame.levels_completed
            results["total_actions"] = self.action_count
            results["success"] = (final_frame.state == GameState.WIN)
        
        return results
    
    def _choose_magen_action(self, frame: FrameData) -> GameAction:
        """Choisir action avec MAGEN"""
        # Extraire grille du frame
        if frame.frame and len(frame.frame) > 0:
            grid = np.array(frame.frame[0])
            
            # Extraire features
            features = self.magen_perception.extract_features(grid)
            
            # Choisir action (simplifié - utilise exploration aléatoire)
            import random
            if random.random() < 0.3:  # exploration
                actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3,
                          GameAction.ACTION4, GameAction.ACTION5, GameAction.ACTION6, GameAction.ACTION7]
                return random.choice(actions)
            else:
                # exploitation - pour l'instant aléatoire aussi
                actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3,
                          GameAction.ACTION4, GameAction.ACTION5, GameAction.ACTION6, GameAction.ACTION7]
                return random.choice(actions)
        
        return GameAction.ACTION1


def test_single_game(game_id: str, magen_root: Path):
    """Test un seul jeu"""
    from core.magen_memory import MAGENMemory
    from perception.arc_perception import ARCPerception
    from agent.arc_agent import ARCAgent
    
    logger.info(f"🎮 Test jeu: {game_id}")
    
    # Charger module jeu
    data_dir = magen_root / "data" / "arc_agi_3" / "environment_files"
    game_dir = data_dir / game_id
    
    if not game_dir.exists():
        logger.error(f"Jeu introuvable: {game_id}")
        return None
    
    # Trouver hash dir
    hash_dirs = [d for d in game_dir.iterdir() if d.is_dir()]
    if not hash_dirs:
        logger.error(f"Pas de hash dir pour {game_id}")
        return None
    
    hash_dir = hash_dirs[0]
    game_file = hash_dir / f"{game_id}.py"
    
    if not game_file.exists():
        logger.error(f"Fichier jeu introuvable: {game_file}")
        return None
    
    # Charger module
    spec = importlib.util.spec_from_file_location(f"game_{game_id}", game_file)
    if spec is None or spec.loader is None:
        logger.error(f"Impossible de charger spec pour {game_id}")
        return None
    
    game_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(game_module)
    
    # Créer composants MAGEN
    memory = MAGENMemory()
    perception = ARCPerception()
    agent = ARCAgent(memory=memory, perception=perception)
    
    # Créer agent simple
    simple_agent = SimpleMAGENAgent(game_module, game_id, memory, perception, agent)
    
    # Exécuter
    results = simple_agent.run()
    
    logger.info(f"✅ Résultats {game_id}:")
    logger.info(f"   Niveaux: {results['levels_completed']}")
    logger.info(f"   Actions: {results['total_actions']}")
    logger.info(f"   Succès: {results['success']}")
    
    return results


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s | %(levelname)s | %(message)s'
    )
    
    magen_root = Path(__file__).parent.parent
    
    # Test sur ar25
    results = test_single_game("ar25", magen_root)
    
    if results:
        print(f"\n✅ Test terminé:")
        print(json.dumps(results, indent=2))

# Made with Bob
