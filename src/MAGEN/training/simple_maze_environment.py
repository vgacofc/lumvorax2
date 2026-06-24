#!/usr/bin/env python3
"""
Environnement Labyrinthe Simplifié pour Entraînement MAGEN
==========================================================

Environnement de test pour Phase 4.2 avant intégration ARC complète.
Basé sur le labyrinthe ls20-9607627b (baseline 2% réussite).

Architecture:
- Grille 2D avec murs, agent, objectif
- Actions: UP, DOWN, LEFT, RIGHT
- Récompenses: +1.0 objectif, -0.01 step, -0.5 mur
- Terminaison: Objectif atteint ou max_steps

Expertises Activées:
- Game Development (Grid-based Environment)
- Reinforcement Learning (Reward Shaping, State Representation)
- Python Programming (OOP, Type Hints)

Auteur: Bob (Expert RL + Game Dev)
Date: 2026-06-19
Version: V42.1
"""

import numpy as np
from typing import Tuple, Dict, Any, List, Optional
from dataclasses import dataclass
from enum import IntEnum


class Action(IntEnum):
    """Actions possibles dans le labyrinthe."""
    UP = 0
    DOWN = 1
    LEFT = 2
    RIGHT = 3


@dataclass
class MazeConfig:
    """Configuration du labyrinthe."""
    width: int = 10
    height: int = 10
    wall_density: float = 0.2  # 20% de murs
    max_steps: int = 100
    reward_goal: float = 1.0
    reward_step: float = -0.01
    reward_wall: float = -0.5


class SimpleMazeEnvironment:
    """
    Environnement labyrinthe simplifié pour entraînement.
    
    Capacités:
    - Génération labyrinthe aléatoire
    - Gestion collisions murs
    - Calcul récompenses
    - État observable (grille + position)
    - Reset/Step standard RL
    """
    
    def __init__(self, config: Optional[MazeConfig] = None):
        self.config = config or MazeConfig()
        
        # Grille (0=vide, 1=mur, 2=agent, 3=objectif)
        self.grid: np.ndarray = None
        
        # Positions
        self.agent_pos: Tuple[int, int] = None
        self.goal_pos: Tuple[int, int] = None
        self.start_pos: Tuple[int, int] = None
        
        # État
        self.steps: int = 0
        self.done: bool = False
        self.total_reward: float = 0.0
        
        # Statistiques
        self.episode_count: int = 0
        self.success_count: int = 0
        
        # Initialiser
        self.reset()
    
    def reset(self) -> np.ndarray:
        """
        Réinitialise l'environnement.
        
        Returns:
            État initial (grille observable)
        """
        # Générer nouveau labyrinthe
        self._generate_maze()
        
        # Réinitialiser état
        self.steps = 0
        self.done = False
        self.total_reward = 0.0
        self.episode_count += 1
        
        return self._get_state()
    
    def step(self, action: int) -> Tuple[np.ndarray, float, bool, Dict[str, Any]]:
        """
        Exécute une action.
        
        Args:
            action: Action à exécuter (0-3)
            
        Returns:
            (next_state, reward, done, info)
        """
        if self.done:
            raise RuntimeError("Épisode terminé, appelez reset()")
        
        # Calculer nouvelle position
        new_pos = self._get_new_position(self.agent_pos, action)
        
        # Vérifier collision mur
        reward = 0.0
        if self._is_wall(new_pos):
            # Collision mur
            reward = self.config.reward_wall
        else:
            # Déplacement valide
            self.agent_pos = new_pos
            reward = self.config.reward_step
            
            # Vérifier objectif atteint
            if self.agent_pos == self.goal_pos:
                reward = self.config.reward_goal
                self.done = True
                self.success_count += 1
        
        # Incrémenter steps
        self.steps += 1
        self.total_reward += reward
        
        # Vérifier max steps
        if self.steps >= self.config.max_steps:
            self.done = True
        
        # Info
        info = {
            'success': self.agent_pos == self.goal_pos,
            'steps': self.steps,
            'total_reward': self.total_reward,
            'agent_pos': self.agent_pos,
            'goal_pos': self.goal_pos
        }
        
        return self._get_state(), reward, self.done, info
    
    def sample_action(self) -> int:
        """Échantillonne une action aléatoire."""
        return np.random.randint(0, 4)
    
    def _generate_maze(self):
        """Génère un labyrinthe aléatoire."""
        # Grille vide
        self.grid = np.zeros((self.config.height, self.config.width), dtype=np.int32)
        
        # Ajouter murs aléatoires
        num_walls = int(self.config.width * self.config.height * self.config.wall_density)
        for _ in range(num_walls):
            x = np.random.randint(0, self.config.width)
            y = np.random.randint(0, self.config.height)
            self.grid[y, x] = 1
        
        # Placer agent (position aléatoire vide)
        while True:
            x = np.random.randint(0, self.config.width)
            y = np.random.randint(0, self.config.height)
            if self.grid[y, x] == 0:
                self.agent_pos = (x, y)
                self.start_pos = (x, y)
                self.grid[y, x] = 2
                break
        
        # Placer objectif (loin de l'agent)
        while True:
            x = np.random.randint(0, self.config.width)
            y = np.random.randint(0, self.config.height)
            if self.grid[y, x] == 0:
                # Vérifier distance minimale
                dist = abs(x - self.agent_pos[0]) + abs(y - self.agent_pos[1])
                if dist >= self.config.width // 2:
                    self.goal_pos = (x, y)
                    self.grid[y, x] = 3
                    break
    
    def _get_new_position(self, pos: Tuple[int, int], action: int) -> Tuple[int, int]:
        """Calcule nouvelle position après action."""
        x, y = pos
        
        if action == Action.UP:
            y = max(0, y - 1)
        elif action == Action.DOWN:
            y = min(self.config.height - 1, y + 1)
        elif action == Action.LEFT:
            x = max(0, x - 1)
        elif action == Action.RIGHT:
            x = min(self.config.width - 1, x + 1)
        
        return (x, y)
    
    def _is_wall(self, pos: Tuple[int, int]) -> bool:
        """Vérifie si position est un mur."""
        x, y = pos
        return self.grid[y, x] == 1
    
    def _get_state(self) -> np.ndarray:
        """
        Retourne l'état observable.
        
        Returns:
            Grille avec positions agent/objectif
        """
        # Copier grille
        state = self.grid.copy()
        
        # Marquer agent et objectif
        state[self.agent_pos[1], self.agent_pos[0]] = 2
        state[self.goal_pos[1], self.goal_pos[0]] = 3
        
        return state
    
    def render(self) -> str:
        """
        Affiche le labyrinthe en ASCII.
        
        Returns:
            Représentation ASCII
        """
        symbols = {
            0: '.',  # Vide
            1: '#',  # Mur
            2: 'A',  # Agent
            3: 'G'   # Goal
        }
        
        lines = []
        lines.append(f"\n{'='*40}")
        lines.append(f"Épisode {self.episode_count} | Steps: {self.steps}/{self.config.max_steps}")
        lines.append(f"Récompense: {self.total_reward:.2f}")
        lines.append(f"Position: {self.agent_pos} → Objectif: {self.goal_pos}")
        lines.append(f"{'='*40}")
        
        state = self._get_state()
        for row in state:
            line = ' '.join(symbols[cell] for cell in row)
            lines.append(line)
        
        lines.append(f"{'='*40}\n")
        
        return '\n'.join(lines)
    
    def get_stats(self) -> Dict[str, Any]:
        """Retourne statistiques environnement."""
        return {
            'episodes': self.episode_count,
            'successes': self.success_count,
            'success_rate': self.success_count / self.episode_count if self.episode_count > 0 else 0.0,
            'current_steps': self.steps,
            'current_reward': self.total_reward
        }


def test_environment():
    """Test rapide de l'environnement."""
    print("\n🧪 TEST ENVIRONNEMENT LABYRINTHE\n")
    
    # Créer environnement
    env = SimpleMazeEnvironment()
    
    # Afficher état initial
    print(env.render())
    
    # Exécuter quelques actions aléatoires
    for i in range(10):
        action = env.sample_action()
        action_name = Action(action).name
        
        state, reward, done, info = env.step(action)
        
        print(f"Action {i+1}: {action_name} | Récompense: {reward:.2f}")
        
        if done:
            print(f"\n✅ Épisode terminé!")
            print(f"   Succès: {info['success']}")
            print(f"   Steps: {info['steps']}")
            print(f"   Récompense totale: {info['total_reward']:.2f}")
            break
    
    # Afficher état final
    print(env.render())
    
    # Statistiques
    stats = env.get_stats()
    print(f"📊 Statistiques:")
    print(f"   Épisodes: {stats['episodes']}")
    print(f"   Succès: {stats['successes']}")
    print(f"   Taux réussite: {stats['success_rate']*100:.1f}%\n")


if __name__ == "__main__":
    test_environment()

# Made with Bob
