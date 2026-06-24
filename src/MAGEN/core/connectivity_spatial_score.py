"""
Connectivity-Aware Spatial Score - MAGEN V28.2.1
Correction du collapse de variance en environnements complexes

V28.2.1: BFS + Manhattan fallback (Option B)
- BFS local (rayon 10) pour détection obstacles
- Manhattan fallback si goal hors rayon
- Compromis performance/efficacité


Problème identifié:
- Spatial score binaire (0.0/1.0) basé sur distance Manhattan
- Collapse variance = 0.0000 quand obstacles bloquent chemin direct
- Toutes actions "vers goal" mais bloquées → scores identiques

Solution:
- BFS local (rayon 3-5) pour détecter accessibilité réelle
- Score basé sur "distance de chemin" (pas distance Manhattan)
- Différenciation actions même avec obstacles

Date: 2026-06-15
"""

from typing import Dict, Tuple, Set, Optional, List
from collections import deque
import numpy as np


class ConnectivitySpatialScore:
    """
    Calcule spatial score basé sur connectivité réelle (BFS local).
    
    Principe:
    - BFS rayon limité (3-5 steps) depuis position actuelle
    - Détecte quelles positions sont accessibles
    - Score action = réduction "distance de chemin" (pas Manhattan)
    """
    
    def __init__(self, bfs_radius: int = 10):
        """
        Args:
            bfs_radius: Rayon BFS local (10 recommandé pour compromis)
        """
        self.bfs_radius = bfs_radius
        self.cache = {}  # Cache BFS pour performance
    
    def compute_score(
        self,
        state: Dict,
        action: str,
        goal_pos: Tuple[int, int]
    ) -> float:
        """
        Calcule score spatial avec awareness connectivité.
        
        Args:
            state: État actuel (doit contenir 'agent_pos', 'grid' ou 'obstacles')
            action: Action candidate
            goal_pos: Position goal
        
        Returns:
            float: Score 0.0-1.0 (0.0=éloigne, 0.5=neutre, 1.0=rapproche)
        """
        agent_pos = state.get('agent_pos')
        if not agent_pos or not goal_pos:
            return 0.5  # Neutre si info manquante
        
        # Extraire obstacles
        obstacles = self._extract_obstacles(state)
        
        # Position après action
        next_pos = self._predict_position(agent_pos, action, obstacles, state.get('grid_size', (10, 10)))
        
        # Distance de chemin AVANT action (BFS depuis agent_pos)
        dist_before = self._path_distance(agent_pos, goal_pos, obstacles, state.get('grid_size', (10, 10)))
        
        # Distance de chemin APRÈS action (BFS depuis next_pos)
        dist_after = self._path_distance(next_pos, goal_pos, obstacles, state.get('grid_size', (10, 10)))
        
        # V28.2.1: Manhattan fallback si BFS retourne None
        if dist_before is None:
            # Goal hors rayon BFS → fallback Manhattan
            dist_before = abs(agent_pos[0] - goal_pos[0]) + abs(agent_pos[1] - goal_pos[1])
        
        if dist_after is None:
            # Goal hors rayon BFS → fallback Manhattan
            dist_after = abs(next_pos[0] - goal_pos[0]) + abs(next_pos[1] - goal_pos[1])
        
        # Score basé sur réduction distance
        if dist_after < dist_before:
            return 1.0  # Rapproche
        elif dist_after > dist_before:
            return 0.0  # Éloigne
        else:
            return 0.5  # Neutre
    
    def _extract_obstacles(self, state: Dict) -> Set[Tuple[int, int]]:
        """Extrait obstacles depuis état."""
        # Méthode 1: obstacles explicites
        if 'obstacles' in state:
            return set(state['obstacles'])
        
        # Méthode 2: grid avec valeurs -1
        if 'grid' in state:
            grid = state['grid']
            obstacles = set()
            for y in range(grid.shape[0]):
                for x in range(grid.shape[1]):
                    if grid[y, x] == -1:
                        obstacles.add((x, y))
            return obstacles
        
        return set()
    
    def _predict_position(
        self,
        pos: Tuple[int, int],
        action: str,
        obstacles: Set[Tuple[int, int]],
        grid_size: Tuple[int, int]
    ) -> Tuple[int, int]:
        """Prédit position après action (avec collision obstacles)."""
        x, y = pos
        
        if action == 'ACTION1' or action == 'move_up':
            y = max(0, y - 1)
        elif action == 'ACTION2' or action == 'move_down':
            y = min(grid_size[1] - 1, y + 1)
        elif action == 'ACTION3' or action == 'move_left':
            x = max(0, x - 1)
        elif action == 'ACTION4' or action == 'move_right':
            x = min(grid_size[0] - 1, x + 1)
        
        new_pos = (x, y)
        
        # Collision obstacle → reste en place
        if new_pos in obstacles:
            return pos
        
        return new_pos
    
    def _path_distance(
        self,
        start: Tuple[int, int],
        goal: Tuple[int, int],
        obstacles: Set[Tuple[int, int]],
        grid_size: Tuple[int, int]
    ) -> Optional[int]:
        """
        Calcule distance de chemin via BFS local (rayon limité).
        
        Returns:
            int: Distance de chemin (nombre steps), ou None si inaccessible
        """
        # Cache key
        cache_key = (start, goal, frozenset(obstacles))
        if cache_key in self.cache:
            return self.cache[cache_key]
        
        # BFS
        queue = deque([(start, 0)])  # (position, distance)
        visited = {start}
        
        while queue:
            pos, dist = queue.popleft()
            
            # Goal atteint
            if pos == goal:
                self.cache[cache_key] = dist
                return dist
            
            # Rayon limité
            if dist >= self.bfs_radius:
                continue
            
            # Explorer voisins (4 directions)
            x, y = pos
            for dx, dy in [(0, -1), (0, 1), (-1, 0), (1, 0)]:
                nx, ny = x + dx, y + dy
                
                # Vérifier limites grid
                if not (0 <= nx < grid_size[0] and 0 <= ny < grid_size[1]):
                    continue
                
                neighbor = (nx, ny)
                
                # Skip obstacles et déjà visités
                if neighbor in obstacles or neighbor in visited:
                    continue
                
                visited.add(neighbor)
                queue.append((neighbor, dist + 1))
        
        # Goal inaccessible dans rayon BFS
        self.cache[cache_key] = None
        return None
    
    def clear_cache(self):
        """Vide cache BFS (appeler si environnement change)."""
        self.cache.clear()


# Fonction helper pour intégration facile
def compute_connectivity_spatial_score(
    state: Dict,
    action: str,
    goal_pos: Tuple[int, int],
    bfs_radius: int = 5
) -> float:
    """
    Helper function pour calcul rapide sans instancier classe.
    
    Args:
        state: État actuel
        action: Action candidate
        goal_pos: Position goal
        bfs_radius: Rayon BFS (default: 5)
    
    Returns:
        float: Score 0.0-1.0
    """
    scorer = ConnectivitySpatialScore(bfs_radius=bfs_radius)
    return scorer.compute_score(state, action, goal_pos)

# Made with Bob
