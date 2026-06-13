"""
MAGEN DSL - Structural Point Detection for Relational Reasoning

Détecte points structurels (coins, extrémités, jonctions) et génère
programmes de marquage avec auto-détection de couleur.

Basé sur analyse forensique 3aa6fb7a.
"""

import numpy as np
from typing import List, Tuple, Set, Optional, Dict
from .program import Program


class StructuralDetector:
    """
    Détecte points structurels dans grilles et génère programmes de marquage
    
    Points structurels:
    - Corners (coins externes)
    - Interior angles (coins internes)
    - Endpoints (extrémités)
    - Junctions (intersections)
    - Cavities (creux)
    """
    
    def detect_introduced_colors(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Set[int]:
        """
        Détecte couleurs introduites dans output (absentes de input)
        
        Args:
            train_pairs: Liste de (input, output)
            
        Returns:
            Set de couleurs introduites
        """
        introduced = set()
        
        for inp, out in train_pairs:
            input_colors = set(np.unique(inp))
            output_colors = set(np.unique(out))
            new_colors = output_colors - input_colors
            introduced.update(new_colors)
        
        return introduced
    
    def detect_L_shapes(self, grid: np.ndarray, target_color: Optional[int] = None) -> List[Tuple[int, int, str]]:
        """
        Détecte formes L et retourne positions des coins
        
        Args:
            grid: Grille d'entrée
            target_color: Couleur à chercher (None = toutes sauf 0)
            
        Returns:
            Liste de (y, x, orientation) pour chaque coin L
            Orientations: 'top_left', 'top_right', 'bottom_left', 'bottom_right'
        """
        h, w = grid.shape
        l_corners = []
        
        # Patterns de formes L (relatif au coin)
        # Format: [(dy1, dx1), (dy2, dx2)] - les 2 autres cellules du L
        l_patterns = {
            'top_left': [(0, 1), (1, 0)],      # ┌
            'top_right': [(0, -1), (1, 0)],    # ┐
            'bottom_left': [(0, 1), (-1, 0)],  # └
            'bottom_right': [(0, -1), (-1, 0)] # ┘
        }
        
        for y in range(h):
            for x in range(w):
                cell_color = safe_get_cell(grid, y, x, default=0)
                
                # Ignorer fond (0) ou vérifier target_color
                if cell_color == 0:
                    continue
                if target_color is not None and cell_color != target_color:
                    continue
                
                # Tester chaque pattern de L
                for orientation, pattern in l_patterns.items():
                    is_l_shape = True
                    
                    for dy, dx in pattern:
                        ny, nx = y + dy, x + dx
                        
                        # Vérifier limites avec validate_bounds
                        if not validate_bounds(grid, ny, nx):
                            is_l_shape = False
                            break
                        
                        # Vérifier même couleur avec safe_get_cell
                        neighbor_color = safe_get_cell(grid, ny, nx, default=-1)
                        if neighbor_color != cell_color:
                            is_l_shape = False
                            break
                    
                    if is_l_shape:
                        l_corners.append((y, x, orientation))
                        break  # Un seul pattern par position
        
        return l_corners
    
    def detect_corners(self, grid: np.ndarray, target_color: Optional[int] = None) -> List[Tuple[int, int, str]]:
        """
        Détecte coins externes (changements de direction)
        
        Returns:
            Liste de (y, x, type) où type = 'convex' ou 'concave'
        """
        corners = []
        h, w = grid.shape
        
        for y in range(1, h-1):
            for x in range(1, w-1):
                cell = safe_get_cell(grid, y, x, default=0)
                
                if cell == 0:
                    continue
                if target_color is not None and cell != target_color:
                    continue
                
                # Vérifier voisins 8-connectés avec safe_get_cell
                neighbors = [
                    safe_get_cell(grid, y-1, x-1, default=0), safe_get_cell(grid, y-1, x, default=0), safe_get_cell(grid, y-1, x+1, default=0),
                    safe_get_cell(grid, y, x-1, default=0),                                            safe_get_cell(grid, y, x+1, default=0),
                    safe_get_cell(grid, y+1, x-1, default=0), safe_get_cell(grid, y+1, x, default=0), safe_get_cell(grid, y+1, x+1, default=0)
                ]
                
                # Coin = cellule avec exactement 2 voisins adjacents de même couleur
                same_color_neighbors = sum(1 for n in neighbors if n == cell)
                
                if same_color_neighbors == 2:
                    corners.append((y, x, 'convex'))
                elif same_color_neighbors >= 6:
                    corners.append((y, x, 'concave'))
        
        return corners
    
    def detect_endpoints(self, grid: np.ndarray, target_color: Optional[int] = None) -> List[Tuple[int, int]]:
        """
        Détecte extrémités (cellules avec 1 seul voisin)
        
        Returns:
            Liste de (y, x)
        """
        endpoints = []
        h, w = grid.shape
        
        for y in range(h):
            for x in range(w):
                cell = safe_get_cell(grid, y, x, default=0)
                
                if cell == 0:
                    continue
                if target_color is not None and cell != target_color:
                    continue
                
                # Compter voisins 4-connectés de même couleur avec safe_get_cell
                neighbors = 0
                for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    ny, nx = y + dy, x + dx
                    if validate_bounds(grid, ny, nx):
                        neighbor_color = safe_get_cell(grid, ny, nx, default=-1)
                        if neighbor_color == cell:
                            neighbors += 1
                
                if neighbors == 1:
                    endpoints.append((y, x))
        
        return endpoints
    
    def generate_marking_programs(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]], 
                                  test_input: np.ndarray) -> List[Program]:
        """
        Génère programmes de marquage de points structurels
        
        Stratégie:
        1. Détecter couleurs introduites
        2. Détecter types de points structurels
        3. Générer programmes avec couleur correcte
        
        Returns:
            Liste de programmes candidats
        """
        programs = []
        
        # 1. Détecter couleurs introduites
        introduced_colors = self.detect_introduced_colors(train_pairs)
        
        if not introduced_colors:
            return programs  # Pas de marquage si pas de nouvelle couleur
        
        # Utiliser première couleur introduite (généralement unique)
        mark_color = list(introduced_colors)[0]
        
        # 2. Détecter couleurs cibles dans input
        input_colors = set()
        for inp, _ in train_pairs:
            input_colors.update(np.unique(inp))
        input_colors.discard(0)  # Ignorer fond
        
        # 3. Générer programmes pour chaque couleur cible
        from .primitives_advanced import detect_and_mark_L_shapes
        
        for target_color in input_colors:
            # Programme: Marquer coins L de target_color avec mark_color
            prog = Program()
            prog.add_operation('detect_and_mark_L_shapes', 
                             detect_and_mark_L_shapes,
                             args=(int(target_color), int(mark_color)))
            programs.append(prog)
        
        # Programme: Marquer tous les coins L (toutes couleurs)
        prog = Program()
        prog.add_operation('detect_and_mark_L_shapes',
                         detect_and_mark_L_shapes,
                         args=(None, int(mark_color)))
        programs.append(prog)
        
        return programs


# Made with Bob - Session 32