"""
ARC Arcade API Environment
Wrapper pour tester MAGEN sur ARC Arcade via API
"""

from typing import Dict, List, Tuple, Any, Optional
import numpy as np


class ARCEnvironment:
    """Environnement ARC Arcade pour tests"""
    
    def __init__(self, game_id: str):
        """
        Initialiser environnement ARC Arcade
        
        Args:
            game_id: ID du jeu (ex: "ls20", "cn04", "cn05")
        """
        self.game_id = game_id
        self.grid_size = 10
        self.current_step = 0
        self.max_steps = 1000
        self.done = False
        
        # Grille initiale (simulée pour tests)
        self.grid = np.zeros((self.grid_size, self.grid_size), dtype=int)
        self.avatar_pos = (0, 0)
        self.goal_pos = (self.grid_size - 1, self.grid_size - 1)
        
    def reset(self) -> Dict[str, Any]:
        """
        Reset environnement
        
        Returns:
            Observation initiale
        """
        self.current_step = 0
        self.done = False
        self.grid = np.zeros((self.grid_size, self.grid_size), dtype=int)
        self.avatar_pos = (0, 0)
        
        # Marquer avatar et goal
        self.grid[self.avatar_pos] = 1
        self.grid[self.goal_pos] = 2
        
        return self._get_observation()
    
    def step(self, action: str) -> Tuple[Dict[str, Any], float, bool, Dict[str, Any]]:
        """
        Exécuter action
        
        Args:
            action: Action à exécuter ("up", "down", "left", "right", "noop")
            
        Returns:
            (observation, reward, done, info)
        """
        if self.done:
            return self._get_observation(), 0.0, True, {}
        
        # Effacer ancienne position
        self.grid[self.avatar_pos] = 0
        
        # Calculer nouvelle position
        new_pos = self._apply_action(self.avatar_pos, action)
        
        # Vérifier limites
        if self._is_valid_position(new_pos):
            self.avatar_pos = new_pos
        
        # Marquer nouvelle position
        self.grid[self.avatar_pos] = 1
        
        # Calculer récompense
        reward = self._calculate_reward()
        
        # Vérifier victoire
        if self.avatar_pos == self.goal_pos:
            self.done = True
            reward = 1.0
        
        self.current_step += 1
        
        # Vérifier timeout
        if self.current_step >= self.max_steps:
            self.done = True
        
        return self._get_observation(), reward, self.done, {}
    
    def _apply_action(self, pos: Tuple[int, int], action: str) -> Tuple[int, int]:
        """Appliquer action à position"""
        x, y = pos
        
        if action == "up":
            return (max(0, x - 1), y)
        elif action == "down":
            return (min(self.grid_size - 1, x + 1), y)
        elif action == "left":
            return (x, max(0, y - 1))
        elif action == "right":
            return (x, min(self.grid_size - 1, y + 1))
        else:  # noop
            return pos
    
    def _is_valid_position(self, pos: Tuple[int, int]) -> bool:
        """Vérifier si position valide"""
        x, y = pos
        return 0 <= x < self.grid_size and 0 <= y < self.grid_size
    
    def _calculate_reward(self) -> float:
        """Calculer récompense basée sur distance au goal"""
        # Distance Manhattan au goal
        dist = abs(self.avatar_pos[0] - self.goal_pos[0]) + abs(self.avatar_pos[1] - self.goal_pos[1])
        max_dist = 2 * (self.grid_size - 1)
        
        # Récompense inversement proportionnelle à distance
        return 1.0 - (dist / max_dist)
    
    def _get_observation(self) -> Dict[str, Any]:
        """Obtenir observation courante"""
        return {
            "grid": self.grid.tolist(),
            "avatar_pos": self.avatar_pos,
            "goal_pos": self.goal_pos,
            "step": self.current_step
        }
    
    def get_available_actions(self) -> List[str]:
        """Obtenir actions disponibles"""
        return ["up", "down", "left", "right", "noop"]
    
    def render(self) -> str:
        """Afficher grille"""
        lines = []
        for i in range(self.grid_size):
            row = []
            for j in range(self.grid_size):
                if (i, j) == self.avatar_pos:
                    row.append("A")
                elif (i, j) == self.goal_pos:
                    row.append("G")
                else:
                    row.append(".")
            lines.append(" ".join(row))
        return "\n".join(lines)

# Made with Bob
