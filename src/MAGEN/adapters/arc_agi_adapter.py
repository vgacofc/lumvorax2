#!/usr/bin/env python3
"""
Adaptateur ARC-AGI-3 pour MAGEN
Permet d'utiliser le système MAGEN avec les environnements ARC-AGI-3 officiels
"""

import sys
import os
import json
import logging
import importlib.util
from pathlib import Path
from typing import Optional, Dict, Any, List, Tuple
import numpy as np

# Ajouter le chemin vers arc_agi si nécessaire
sys.path.insert(0, str(Path(__file__).parent.parent / "data" / "arc_agi_3"))

try:
    from arc_agi import EnvironmentWrapper
    from arcengine import GameAction, GameState, FrameData
except ImportError as e:
    print(f"⚠️  Erreur import arc_agi: {e}")
    print("Installation des wheels nécessaire:")
    print("  cd lumvorax2/src/MAGEN")
    print("  venv/bin/pip install data/arc_agi_3/arc_agi_3_wheels/*.whl")
    sys.exit(1)

logger = logging.getLogger(__name__)


class ARCGameEnvironment:
    """Wrapper pour un jeu ARC-AGI-3 spécifique"""
    
    def __init__(self, game_id: str, game_path: Path, metadata: Dict[str, Any]):
        self.game_id = game_id
        self.game_path = game_path
        self.metadata = metadata
        self.baseline_actions = metadata.get("baseline_actions", [])
        self.env: Optional[EnvironmentWrapper] = None
        self.current_frame: Optional[FrameData] = None
        self.action_count = 0
        self.levels_completed = 0
        
    def load(self) -> bool:
        """Charge l'environnement du jeu"""
        try:
            # Charger le module Python du jeu
            spec = importlib.util.spec_from_file_location(
                f"game_{self.game_id}", 
                self.game_path
            )
            if spec is None or spec.loader is None:
                logger.error(f"Impossible de charger {self.game_path}")
                return False
                
            game_module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(game_module)
            
            # Créer logger simple
            import logging
            env_logger = logging.getLogger(f"arc_env_{self.game_id}")
            
            # Créer l'environnement wrapper avec logger et scorecard_id
            self.env = EnvironmentWrapper(
                game_module,
                logger=env_logger,
                scorecard_id=f"magen_{self.game_id}"
            )
            
            # Obtenir l'observation initiale
            initial_obs = self.env.observation_space
            if initial_obs:
                self.current_frame = self._convert_observation(initial_obs)
                logger.info(f"✅ Jeu {self.game_id} chargé - {len(self.baseline_actions)} niveaux")
                return True
            else:
                logger.error(f"Pas d'observation initiale pour {self.game_id}")
                return False
                
        except Exception as e:
            logger.error(f"Erreur chargement {self.game_id}: {e}")
            return False
    
    def _convert_observation(self, obs: Any) -> FrameData:
        """Convertit une observation en FrameData"""
        # L'observation contient les informations du frame
        return FrameData(
            game_id=self.game_id,
            frame=obs.frame if hasattr(obs, 'frame') else [],
            state=obs.state if hasattr(obs, 'state') else GameState.PLAYING,
            levels_completed=obs.levels_completed if hasattr(obs, 'levels_completed') else 0,
            win_levels=obs.win_levels if hasattr(obs, 'win_levels') else []
        )
    
    def step(self, action: GameAction) -> Tuple[Optional[FrameData], bool]:
        """
        Exécute une action dans l'environnement
        
        Returns:
            (frame, done): Frame résultant et flag de fin
        """
        if self.env is None:
            return None, True
            
        try:
            # Exécuter l'action
            obs = self.env.step(action, data={}, reasoning={})
            
            if obs is None:
                return None, True
            
            # Convertir en FrameData
            frame = self._convert_observation(obs)
            self.current_frame = frame
            self.action_count += 1
            self.levels_completed = frame.levels_completed
            
            # Vérifier si terminé
            done = (
                frame.state in [GameState.WON, GameState.LOST] or
                self.action_count >= 80  # MAX_ACTIONS
            )
            
            return frame, done
            
        except Exception as e:
            logger.error(f"Erreur step {self.game_id}: {e}")
            return None, True
    
    def reset(self) -> Optional[FrameData]:
        """Reset l'environnement"""
        if self.env:
            try:
                obs = self.env.reset()
                if obs:
                    self.current_frame = self._convert_observation(obs)
                    self.action_count = 0
                    self.levels_completed = 0
                    return self.current_frame
            except Exception as e:
                logger.error(f"Erreur reset {self.game_id}: {e}")
        return None
    
    def get_grid(self) -> Optional[np.ndarray]:
        """Extrait la grille actuelle pour MAGEN"""
        if self.current_frame and self.current_frame.frame:
            try:
                # Le frame contient une liste de grilles (une par niveau)
                if len(self.current_frame.frame) > 0:
                    grid = np.array(self.current_frame.frame[0])
                    return grid
            except Exception as e:
                logger.error(f"Erreur extraction grille: {e}")
        return None
    
    def get_info(self) -> Dict[str, Any]:
        """Informations sur l'état actuel"""
        return {
            "game_id": self.game_id,
            "action_count": self.action_count,
            "levels_completed": self.levels_completed,
            "baseline_actions": self.baseline_actions,
            "state": self.current_frame.state if self.current_frame else None
        }


class ARCAGIAdapter:
    """Adaptateur principal pour charger et gérer les jeux ARC-AGI-3"""
    
    def __init__(self, data_dir: Path):
        self.data_dir = data_dir
        self.environment_dir = data_dir / "environment_files"
        self.games: Dict[str, ARCGameEnvironment] = {}
        
    def discover_games(self) -> List[str]:
        """Découvre tous les jeux disponibles"""
        game_ids = []
        
        if not self.environment_dir.exists():
            logger.error(f"Répertoire introuvable: {self.environment_dir}")
            return game_ids
        
        for game_dir in self.environment_dir.iterdir():
            if game_dir.is_dir():
                # Chaque jeu a un sous-répertoire avec hash
                for hash_dir in game_dir.iterdir():
                    if hash_dir.is_dir():
                        metadata_file = hash_dir / "metadata.json"
                        game_file = hash_dir / f"{game_dir.name}.py"
                        
                        if metadata_file.exists() and game_file.exists():
                            game_ids.append(game_dir.name)
                            break
        
        logger.info(f"🎮 {len(game_ids)} jeux découverts: {', '.join(sorted(game_ids))}")
        return sorted(game_ids)
    
    def load_game(self, game_id: str) -> Optional[ARCGameEnvironment]:
        """Charge un jeu spécifique"""
        if game_id in self.games:
            return self.games[game_id]
        
        # Trouver le répertoire du jeu
        game_dir = self.environment_dir / game_id
        if not game_dir.exists():
            logger.error(f"Jeu introuvable: {game_id}")
            return None
        
        # Trouver le sous-répertoire avec hash
        hash_dirs = [d for d in game_dir.iterdir() if d.is_dir()]
        if not hash_dirs:
            logger.error(f"Pas de sous-répertoire pour {game_id}")
            return None
        
        hash_dir = hash_dirs[0]
        metadata_file = hash_dir / "metadata.json"
        game_file = hash_dir / f"{game_id}.py"
        
        if not metadata_file.exists() or not game_file.exists():
            logger.error(f"Fichiers manquants pour {game_id}")
            return None
        
        # Charger metadata
        with open(metadata_file, 'r') as f:
            metadata = json.load(f)
        
        # Créer l'environnement
        env = ARCGameEnvironment(game_id, game_file, metadata)
        
        if env.load():
            self.games[game_id] = env
            return env
        
        return None
    
    def load_multiple_games(self, game_ids: List[str]) -> Dict[str, ARCGameEnvironment]:
        """Charge plusieurs jeux"""
        loaded = {}
        for game_id in game_ids:
            env = self.load_game(game_id)
            if env:
                loaded[game_id] = env
        return loaded
    
    def get_game_stats(self) -> Dict[str, Any]:
        """Statistiques sur les jeux chargés"""
        return {
            "total_games": len(self.games),
            "games": {
                game_id: {
                    "baseline_actions": len(env.baseline_actions),
                    "levels_completed": env.levels_completed,
                    "action_count": env.action_count
                }
                for game_id, env in self.games.items()
            }
        }


def create_adapter(magen_root: Optional[Path] = None) -> ARCAGIAdapter:
    """
    Crée un adaptateur ARC-AGI-3
    
    Args:
        magen_root: Racine du projet MAGEN (auto-détecté si None)
    
    Returns:
        ARCAGIAdapter configuré
    """
    if magen_root is None:
        magen_root = Path(__file__).parent.parent
    
    data_dir = magen_root / "data" / "arc_agi_3"
    
    if not data_dir.exists():
        raise FileNotFoundError(
            f"Dataset ARC-AGI-3 introuvable: {data_dir}\n"
            "Exécutez: python scripts/download_arc_dataset.py"
        )
    
    return ARCAGIAdapter(data_dir)


if __name__ == "__main__":
    # Test de l'adaptateur
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s | %(levelname)s | %(message)s'
    )
    
    print("🔧 Test ARCAGIAdapter")
    print("=" * 60)
    
    adapter = create_adapter()
    games = adapter.discover_games()
    
    print(f"\n✅ {len(games)} jeux disponibles")
    
    if games:
        # Tester le premier jeu
        test_game = games[0]
        print(f"\n🎮 Test chargement: {test_game}")
        
        env = adapter.load_game(test_game)
        if env:
            print(f"  ✅ Chargé: {env.game_id}")
            print(f"  📊 Baseline: {len(env.baseline_actions)} niveaux")
            print(f"  🎯 Actions baseline: {env.baseline_actions}")
            
            grid = env.get_grid()
            if grid is not None:
                print(f"  🔲 Grille: {grid.shape}")
                print(f"  📈 Valeurs: min={grid.min()}, max={grid.max()}")

# Made with Bob
