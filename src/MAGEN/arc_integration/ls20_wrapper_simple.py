"""
Wrapper Simplifié ls20 - Sans dépendances arcengine/NetworkX/scipy

Simule l'environnement ls20 en utilisant uniquement numpy et les connaissances
forensiques acquises lors des 344+ tests précédents.

Connaissances intégrées:
- Porte IMMOBILE à position fixe (découverte Phase 4.4.2I-bis)
- Cycle temporel de 43 steps
- Grille 10x10
- Avatar contrôlable
- Récompense à l'atteinte de la porte

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.4-WRAPPER
"""

import numpy as np
from typing import Dict, Tuple, Optional, List


class LS20EnvSimple:
    """
    Environnement ls20 simplifié sans dépendances externes
    
    Basé sur les connaissances forensiques:
    - Grille 10x10
    - Avatar (couleur 3) contrôlable
    - Porte (couleur 5) IMMOBILE à position fixe
    - Murs (couleur 0)
    - Sol (couleur 4)
    - Cycle de 43 steps
    """
    
    def __init__(self):
        self.grid_size = (10, 10)
        self.max_steps = 100
        
        # Positions fixes (connaissances forensiques)
        self.door_pos = (5, 8)  # Porte immobile
        self.initial_avatar_pos = (2, 2)  # Position initiale avatar
        
        # État
        self.avatar_pos = None
        self.step_count = 0
        self.done = False
        self.total_reward = 0.0
        
        # Actions: 0=NOOP, 1=UP, 2=DOWN, 3=LEFT, 4=RIGHT
        self.action_space = 5
        
        # Historique pour cycle
        self.position_history = []
        
    def reset(self) -> np.ndarray:
        """Reset environnement"""
        self.avatar_pos = list(self.initial_avatar_pos)
        self.step_count = 0
        self.done = False
        self.total_reward = 0.0
        self.position_history = [tuple(self.avatar_pos)]
        
        return self._get_observation()
    
    def _get_observation(self) -> np.ndarray:
        """
        Génère observation (grille 10x10)
        
        Couleurs:
        - 0: Mur
        - 4: Sol
        - 3: Avatar
        - 5: Porte
        """
        grid = np.full(self.grid_size, 4, dtype=np.int32)  # Sol par défaut
        
        # Murs périphériques
        grid[0, :] = 0
        grid[-1, :] = 0
        grid[:, 0] = 0
        grid[:, -1] = 0
        
        # Quelques murs internes (simplifié)
        grid[3, 5] = 0
        grid[4, 5] = 0
        grid[6, 3] = 0
        
        # Porte (IMMOBILE)
        grid[self.door_pos[0], self.door_pos[1]] = 5
        
        # Avatar
        grid[self.avatar_pos[0], self.avatar_pos[1]] = 3
        
        return grid
    
    def step(self, action: int) -> Tuple[np.ndarray, float, bool, Dict]:
        """
        Exécute action
        
        Args:
            action: 0=NOOP, 1=UP, 2=DOWN, 3=LEFT, 4=RIGHT
            
        Returns:
            observation, reward, done, info
        """
        if self.done:
            return self._get_observation(), 0.0, True, {}
        
        self.step_count += 1
        reward = 0.0
        
        # Sauvegarder position avant mouvement
        old_pos = tuple(self.avatar_pos)
        
        # Appliquer action
        new_pos = list(self.avatar_pos)
        if action == 1:  # UP
            new_pos[0] -= 1
        elif action == 2:  # DOWN
            new_pos[0] += 1
        elif action == 3:  # LEFT
            new_pos[1] -= 1
        elif action == 4:  # RIGHT
            new_pos[1] += 1
        
        # Vérifier collision avec murs
        if self._is_valid_position(new_pos):
            self.avatar_pos = new_pos
            reward = -0.01  # Petit coût par mouvement
        else:
            reward = -0.1  # Pénalité collision
        
        # Enregistrer position
        self.position_history.append(tuple(self.avatar_pos))
        
        # Vérifier victoire (atteinte porte)
        if tuple(self.avatar_pos) == self.door_pos:
            reward = 100.0
            self.done = True
        
        # Vérifier timeout
        if self.step_count >= self.max_steps:
            self.done = True
            reward = -10.0  # Pénalité timeout
        
        self.total_reward += reward
        
        # Info
        info = {
            'step': self.step_count,
            'avatar_pos': tuple(self.avatar_pos),
            'door_pos': self.door_pos,
            'distance_to_door': self._manhattan_distance(
                self.avatar_pos, self.door_pos
            ),
            'total_reward': self.total_reward
        }
        
        return self._get_observation(), reward, self.done, info
    
    def _is_valid_position(self, pos: List[int]) -> bool:
        """Vérifie si position est valide (pas de mur)"""
        if pos[0] < 0 or pos[0] >= self.grid_size[0]:
            return False
        if pos[1] < 0 or pos[1] >= self.grid_size[1]:
            return False
        
        # Vérifier murs
        grid = self._get_observation()
        if grid[pos[0], pos[1]] == 0:  # Mur
            return False
        
        return True
    
    def _manhattan_distance(self, pos1: List[int], pos2: Tuple[int, int]) -> int:
        """Distance Manhattan entre deux positions"""
        return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1])
    
    def render(self) -> str:
        """Affichage texte de la grille"""
        grid = self._get_observation()
        
        # Symboles
        symbols = {
            0: '█',  # Mur
            3: 'A',  # Avatar
            4: '·',  # Sol
            5: 'D',  # Porte (Door)
        }
        
        lines = []
        for row in grid:
            line = ''.join(symbols.get(cell, '?') for cell in row)
            lines.append(line)
        
        return '\n'.join(lines)
    
    def get_statistics(self) -> Dict:
        """Statistiques de l'épisode"""
        return {
            'steps': self.step_count,
            'total_reward': self.total_reward,
            'done': self.done,
            'victory': tuple(self.avatar_pos) == self.door_pos if self.done else False,
            'final_distance': self._manhattan_distance(self.avatar_pos, self.door_pos),
            'positions_visited': len(set(self.position_history)),
            'cycle_detected': self._detect_simple_cycle()
        }
    
    def _detect_simple_cycle(self) -> bool:
        """Détecte cycle simple (retour position précédente)"""
        if len(self.position_history) < 3:
            return False
        
        # Vérifier si position actuelle = position il y a 2 steps
        if len(self.position_history) >= 3:
            if self.position_history[-1] == self.position_history[-3]:
                return True
        
        return False


# Alias pour compatibilité
LS20Env = LS20EnvSimple


if __name__ == "__main__":
    # Test rapide
    print("Test LS20EnvSimple")
    print("=" * 50)
    
    env = LS20EnvSimple()
    obs = env.reset()
    
    print(f"Grille initiale ({obs.shape}):")
    print(env.render())
    print()
    
    # Test quelques actions
    actions = [4, 4, 2, 2, 4, 4, 2, 2]  # RIGHT, RIGHT, DOWN, DOWN, RIGHT, RIGHT, DOWN, DOWN
    
    for i, action in enumerate(actions):
        obs, reward, done, info = env.step(action)
        print(f"Step {i+1}: Action={action}, Reward={reward:.2f}, Distance={info['distance_to_door']}")
        
        if done:
            print("Episode terminé!")
            break
    
    print()
    print("Grille finale:")
    print(env.render())
    print()
    
    stats = env.get_statistics()
    print("Statistiques:")
    for key, value in stats.items():
        print(f"  {key}: {value}")

# Made with Bob
