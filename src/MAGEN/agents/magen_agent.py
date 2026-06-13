"""
MAGEN Agent - Mémoire Artificielle GEN pour ARC-AGI-3
Hérite de la classe Agent officielle ARC-AGI-3
Intègre: Mémoire multi-échelle + Perception + Exploration + Forensic LumVorax
"""

import sys
import os
import logging
from typing import Optional
import numpy as np

# Ajouter le chemin MAGEN au sys.path
MAGEN_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if MAGEN_ROOT not in sys.path:
    sys.path.insert(0, MAGEN_ROOT)

# Imports ARC-AGI-3 officiels
from arcengine import FrameData, FrameDataRaw, GameAction, GameState

# Imports MAGEN
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception
from agent.arc_agent import ARCAgent as MAGENExplorer
from forensic.lumvorax_logger import LumVoraxLogger

logger = logging.getLogger(__name__)


class MAGENAgent:
    """
    Agent MAGEN pour ARC-AGI-3
    
    Architecture 4 couches:
    1. Mémoire multi-échelle (court/moyen/long terme + archive)
    2. Perception grilles (features spatiales, patterns, transformations)
    3. Exploration (stratégie exploration/exploitation)
    4. Forensic (logging bit-level nanoseconde)
    """
    
    MAX_ACTIONS: int = 80
    ROOT_URL: str
    
    def __init__(
        self,
        card_id: str,
        game_id: str,
        agent_name: str,
        ROOT_URL: str,
        record: bool,
        arc_env,  # EnvironmentWrapper
        tags: Optional[list[str]] = None,
    ) -> None:
        """Initialisation agent MAGEN"""
        
        # Attributs officiels Agent
        self.ROOT_URL = ROOT_URL
        self.card_id = card_id
        self.game_id = game_id
        self.guid = ""
        self.agent_name = agent_name
        self.tags = tags or []
        self.frames = [FrameData(levels_completed=0)]
        self._cleanup = True
        self.action_counter = 0
        self.timer = 0
        self.arc_env = arc_env
        
        # Headers API
        self.headers = {
            "X-API-Key": os.getenv("ARC_API_KEY", ""),
            "Accept": "application/json",
        }
        
        # Composants MAGEN
        logger.info(f"🚀 Initialisation MAGEN Agent pour jeu {game_id}")
        
        # 1. Mémoire multi-échelle
        self.memory = MAGENMemory()
        logger.info("✅ Mémoire MAGEN initialisée (4 niveaux)")
        
        # 2. Perception
        self.perception = ARCPerception()
        logger.info("✅ Perception ARC initialisée (64D features)")
        
        # 3. Explorateur
        self.explorer = MAGENExplorer(
            exploration_rate=0.5,
            memory=self.memory,
            perception=self.perception
        )
        logger.info("✅ Explorateur MAGEN initialisé (rate 0.5)")
        
        # 4. Forensic Logger
        log_dir = os.path.join(MAGEN_ROOT, "logs", "forensic", game_id)
        os.makedirs(log_dir, exist_ok=True)
        self.forensic = LumVoraxLogger(log_dir=log_dir)
        logger.info(f"✅ Forensic LumVorax initialisé ({log_dir})")
        
        # Log événement initialisation
        self.forensic.log_event("agent_init", {
            "game_id": game_id,
            "card_id": card_id,
            "agent_name": agent_name,
            "exploration_rate": 0.5
        })
        
        logger.info("🎯 MAGEN Agent prêt !")
    
    def main(self) -> None:
        """Boucle principale agent - joue le jeu jusqu'à la fin"""
        import time
        
        self.timer = time.time()
        
        logger.info(f"🎮 Démarrage jeu {self.game_id}")
        self.forensic.log_event("game_start", {"game_id": self.game_id})
        
        while (
            not self.is_done(self.frames, self.frames[-1])
            and self.action_counter <= self.MAX_ACTIONS
        ):
            # Choisir action
            action = self.choose_action(
                self.frames,
                self._convert_raw_frame_data(
                    self.arc_env.observation_space if self.arc_env else None
                ),
            )
            
            # Log action
            self.forensic.log_event("action_chosen", {
                "action": action.name,
                "counter": self.action_counter
            })
            
            # Exécuter action
            if frame := self.take_action(action):
                self.append_frame(frame)
                
                # Log résultat
                self.forensic.log_event("action_result", {
                    "action": action.name,
                    "levels_completed": frame.levels_completed,
                    "state": frame.state.name
                })
                
                logger.info(
                    f"{self.game_id} - {action.name}: "
                    f"count {self.action_counter}, "
                    f"levels {frame.levels_completed}, "
                    f"fps {self.fps:.2f}"
                )
            
            self.action_counter += 1
        
        # Fin du jeu
        elapsed = time.time() - self.timer
        self.forensic.log_event("game_end", {
            "game_id": self.game_id,
            "actions": self.action_counter,
            "levels_completed": self.levels_completed,
            "elapsed_seconds": elapsed
        })
        
        logger.info(
            f"🏁 Jeu {self.game_id} terminé: "
            f"{self.levels_completed} niveaux, "
            f"{self.action_counter} actions, "
            f"{elapsed:.2f}s"
        )
        
        self.cleanup()
    
    def choose_action(
        self,
        frames: list[FrameData],
        observation: Optional[FrameDataRaw]
    ) -> GameAction:
        """Choisir action basée sur mémoire et perception"""
        
        if not observation or not observation.grid:
            return GameAction.RESET
        
        # Extraire features grille actuelle
        grid = np.array(observation.grid)
        features = self.perception.extract_features(grid)
        
        # Récupérer patterns similaires de la mémoire
        similar_patterns = self.memory.retrieve_similar_pattern(features, top_k=5)
        
        # Stratégie exploration/exploitation
        if self.explorer.should_explore():
            # Exploration: action aléatoire
            action = self.explorer.explore()
        else:
            # Exploitation: meilleure action connue
            if similar_patterns:
                # Utiliser pattern le plus similaire
                best_pattern = similar_patterns[0]
                action_name = best_pattern.get("action", "ACTION1")
                try:
                    action = GameAction[action_name]
                except KeyError:
                    action = GameAction.ACTION1
            else:
                # Pas de pattern connu, explorer
                action = self.explorer.explore()
        
        return action
    
    def take_action(self, action: GameAction) -> Optional[FrameData]:
        """Exécuter action dans environnement"""
        if not self.arc_env:
            return None
        
        try:
            frame = self.arc_env.step(action)
            
            # Mémoriser expérience
            if frame and frame.grid:
                grid = np.array(frame.grid)
                features = self.perception.extract_features(grid)
                
                experience = {
                    "state": features,
                    "action": action.name,
                    "result": frame.state.name,
                    "levels_completed": frame.levels_completed,
                    "success": frame.state == GameState.LEVEL_COMPLETE
                }
                
                self.memory.inject_experience(experience)
            
            return frame
        except Exception as e:
            logger.error(f"Erreur take_action: {e}")
            self.forensic.log_event("action_error", {
                "action": action.name,
                "error": str(e)
            })
            return None
    
    def is_done(self, frames: list[FrameData], frame: FrameData) -> bool:
        """Vérifier si jeu terminé"""
        return frame.state in [
            GameState.GAME_COMPLETE,
            GameState.GAME_OVER
        ]
    
    def append_frame(self, frame: FrameData) -> None:
        """Ajouter frame à l'historique"""
        self.frames.append(frame)
    
    def _convert_raw_frame_data(self, raw: any) -> Optional[FrameDataRaw]:
        """Convertir observation en FrameDataRaw"""
        if not raw:
            return None
        
        try:
            if hasattr(raw, 'grid'):
                return FrameDataRaw(grid=raw.grid)
            return None
        except Exception as e:
            logger.error(f"Erreur conversion frame: {e}")
            return None
    
    @property
    def state(self) -> GameState:
        """État actuel du jeu"""
        return self.frames[-1].state
    
    @property
    def levels_completed(self) -> int:
        """Nombre de niveaux complétés"""
        return self.frames[-1].levels_completed
    
    @property
    def seconds(self) -> float:
        """Temps écoulé en secondes"""
        import time
        return time.time() - self.timer
    
    @property
    def fps(self) -> float:
        """Actions par seconde"""
        if self.seconds == 0:
            return 0.0
        return self.action_counter / self.seconds
    
    def cleanup(self) -> None:
        """Nettoyage ressources"""
        logger.info(f"🧹 Cleanup agent {self.game_id}")
        
        # Sauvegarder mémoire
        memory_file = os.path.join(
            MAGEN_ROOT, "logs", "memory", f"{self.game_id}_memory.json"
        )
        os.makedirs(os.path.dirname(memory_file), exist_ok=True)
        self.memory.save_state(memory_file)
        logger.info(f"💾 Mémoire sauvegardée: {memory_file}")
        
        # Finaliser logs forensiques
        self.forensic.log_event("agent_cleanup", {
            "game_id": self.game_id,
            "total_actions": self.action_counter,
            "total_seconds": self.seconds
        })
        
        # Valider intégrité logs
        if self.forensic.validate_integrity():
            logger.info("✅ Logs forensiques validés (intégrité OK)")
        else:
            logger.warning("⚠️ Logs forensiques: anomalies détectées")
        
        logger.info("✅ Cleanup terminé")


# Export pour AVAILABLE_AGENTS
__all__ = ["MAGENAgent"]

# Made with Bob
