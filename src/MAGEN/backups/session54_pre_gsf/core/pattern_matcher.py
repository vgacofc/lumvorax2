"""
MAGEN - Pattern Matcher Exact
==============================

Détecte patterns EXACTS et génère transformations complètes
pour éviter conflits entre règles génériques.

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot + LumVorax
"""

import numpy as np
from typing import Optional, Dict, Any, List, Tuple, Callable
from dataclasses import dataclass

# Import safe operations pour validation bounds
from .safe_operations import safe_access, safe_write, safe_slice


@dataclass
class ExactPattern:
    """Pattern exact détecté"""
    name: str
    confidence: float
    transform_func: Callable[[np.ndarray], np.ndarray]
    description: str


class PatternMatcher:
    """
    Détecteur de patterns exacts pour ARC-AGI-3
    
    Stratégie:
    1. Détecter pattern EXACT depuis train examples
    2. Générer transformation COMPLÈTE
    3. Appliquer directement sans règles intermédiaires
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.patterns_detected = []
    
    def detect_exact_pattern(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    ) -> Optional[ExactPattern]:
        """
        Détecter pattern exact depuis exemples training
        
        Returns:
            ExactPattern si détecté, None sinon
        """
        if len(train_pairs) == 0:
            return None
        
        # Tester chaque pattern dans l'ordre de spécificité (PLUS SPÉCIFIQUE D'ABORD)
        patterns = [
            self._detect_bounding_rectangle,      # Plus spécifique: exactement 2 pixels
            self._detect_l_shape_corner_fill,     # Spécifique: objets 3-pixels
            self._detect_spaced_row_duplication,  # Plus générique: lignes espacées
        ]
        
        for pattern_detector in patterns:
            pattern = pattern_detector(train_pairs)
            if pattern:
                self.patterns_detected.append(pattern)
                if self.verbose:
                    print(f"[PatternMatcher] Détecté: {pattern.name} (conf={pattern.confidence})")
                return pattern
        
        return None
    
    def _detect_spaced_row_duplication(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    ) -> Optional[ExactPattern]:
        """
        Pattern: Lignes non-vides espacées avec duplication adjacente
        
        Exemple: Puzzle 00d62c1b
        - Input: Lignes 2 et 4 avec pattern
        - Output: Ajouter lignes au-dessus/dessous avec nouvelle couleur
        - Contrainte: Lignes doivent être espacées (pas adjacentes)
        """
        inp, out = train_pairs[0]
        
        # Trouver lignes non-vides dans input
        non_empty_rows = []
        for i in range(inp.shape[0]):
            if np.any(inp[i, :] > 0):
                non_empty_rows.append(i)
        
        if len(non_empty_rows) < 2:
            return None
        
        # Vérifier espacement (au moins 1 ligne vide entre)
        min_spacing = min(non_empty_rows[i+1] - non_empty_rows[i] 
                         for i in range(len(non_empty_rows)-1))
        
        if min_spacing < 2:
            return None  # Lignes trop proches
        
        # Vérifier si output a lignes ajoutées au-dessus/dessous
        added_above = 0
        added_below = 0
        new_color = None
        
        for row_idx in non_empty_rows:
            # CORRECTION: Vérifier bounds pour output aussi
            # Ligne au-dessus
            if row_idx > 0 and row_idx - 1 < out.shape[0]:
                if np.sum(inp[row_idx-1, :]) == 0 and np.sum(out[row_idx-1, :]) > 0:
                    added_above += 1
                    if new_color is None:
                        new_color = int(np.max(out[row_idx-1, :]))
            
            # Ligne en-dessous
            if row_idx < inp.shape[0] - 1 and row_idx + 1 < out.shape[0]:
                if np.sum(inp[row_idx+1, :]) == 0 and np.sum(out[row_idx+1, :]) > 0:
                    added_below += 1
                    if new_color is None:
                        new_color = int(np.max(out[row_idx+1, :]))
        
        if added_above > 0 and added_below > 0 and new_color is not None:
            # Pattern détecté!
            def transform(grid: np.ndarray) -> np.ndarray:
                result = grid.copy()
                
                # Trouver lignes non-vides (lignes avec couleur source)
                rows = []
                for i in range(grid.shape[0]):
                    if np.any(grid[i, :] > 0):
                        rows.append(i)
                
                # CORRECTION FINALE: Ajouter lignes UNIQUEMENT si pas ENTRE 2 lignes sources
                for row_idx in rows:
                    # Au-dessus: ajouter SI pas de ligne source au-dessus
                    # ET ligne au-dessus n'est pas entre 2 sources
                    if row_idx > 0 and np.sum(result[row_idx-1, :]) == 0:
                        # Vérifier si ligne au-dessus est entre 2 sources
                        is_between = False
                        if row_idx > 1 and (row_idx-2) in rows:
                            is_between = True  # Entre row_idx-2 et row_idx
                        
                        if not is_between:
                            for j in range(grid.shape[1]):
                                if grid[row_idx, j] > 0:
                                    result[row_idx-1, j] = new_color
                    
                    # En-dessous: ajouter SI pas de ligne source en-dessous
                    # ET ligne en-dessous n'est pas entre 2 sources
                    if row_idx < grid.shape[0] - 1 and np.sum(result[row_idx+1, :]) == 0:
                        # Vérifier si ligne en-dessous est entre 2 sources
                        is_between = False
                        if row_idx < grid.shape[0] - 2 and (row_idx+2) in rows:
                            is_between = True  # Entre row_idx et row_idx+2
                        
                        if not is_between:
                            for j in range(grid.shape[1]):
                                if grid[row_idx, j] > 0:
                                    result[row_idx+1, j] = new_color
                
                return result
            
            return ExactPattern(
                name="spaced_row_duplication",
                confidence=0.98,
                transform_func=transform,
                description=f"Dupliquer lignes espacées avec couleur {new_color}"
            )
        
        return None
    
    def _detect_bounding_rectangle(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    ) -> Optional[ExactPattern]:
        """
        Pattern: Rectangle englobant entre pixels isolés
        
        Exemple: Puzzle 025d127b
        - Input: 2 pixels isolés
        - Output: Rectangle creux englobant avec couleur du premier pixel
        """
        inp, out = train_pairs[0]
        
        # Trouver pixels non-nuls dans input
        pixels = []
        for i in range(inp.shape[0]):
            for j in range(inp.shape[1]):
                if inp[i, j] > 0:
                    pixels.append((i, j, inp[i, j]))
        
        if len(pixels) != 2:
            return None  # Doit avoir exactement 2 pixels
        
        # CORRECTION: Vérifier shapes compatibles avant broadcast
        if inp.shape != out.shape:
            return None  # Shapes incompatibles
        
        # Vérifier si output contient rectangle
        diff = out != inp
        if np.sum(diff) < 10:
            return None  # Pas assez de pixels ajoutés
        
        # Calculer bounding box
        rows = [p[0] for p in pixels]
        cols = [p[1] for p in pixels]
        
        y_min, y_max = min(rows), max(rows)
        x_min, x_max = min(cols), max(cols)
        
        # Vérifier si c'est un rectangle dans output
        # Compter pixels sur les bords
        border_pixels = 0
        rect_color = None
        
        # Haut et bas
        for x in range(x_min, x_max + 1):
            if out[y_min, x] > 0 and inp[y_min, x] == 0:
                border_pixels += 1
                if rect_color is None:
                    rect_color = int(out[y_min, x])
            if out[y_max, x] > 0 and inp[y_max, x] == 0:
                border_pixels += 1
        
        # Gauche et droite
        for y in range(y_min, y_max + 1):
            if out[y, x_min] > 0 and inp[y, x_min] == 0:
                border_pixels += 1
                if rect_color is None:
                    rect_color = int(out[y, x_min])
            if out[y, x_max] > 0 and inp[y, x_max] == 0:
                border_pixels += 1
        
        if border_pixels >= 8 and rect_color is not None:
            # Pattern détecté!
            def transform(grid: np.ndarray) -> np.ndarray:
                result = grid.copy()
                
                # Trouver pixels non-nuls
                pxs = []
                for i in range(grid.shape[0]):
                    for j in range(grid.shape[1]):
                        if grid[i, j] > 0:
                            pxs.append((i, j, grid[i, j]))
                
                if len(pxs) < 2:
                    return result
                
                # CORRECTION: Bounding box doit inclure TOUS les pixels
                # Ajouter +1 pour avoir un rectangle qui englobe complètement
                ys = [p[0] for p in pxs]
                xs = [p[1] for p in pxs]
                
                y_min, y_max = min(ys), max(ys) + 1
                x_min, x_max = min(xs), max(xs) + 1
                
                # CORRECTION: Couleur = premier pixel en position (y_min, x_min)
                # Chercher quel pixel est à cette position
                color = None
                for p in pxs:
                    if p[0] == y_min and p[1] == x_min:
                        color = p[2]
                        break
                
                if color is None:
                    # Fallback: premier pixel de la liste
                    color = pxs[0][2]
                
                # Dessiner rectangle creux
                # Haut et bas
                for x in range(x_min, x_max + 1):
                    if result[y_min, x] == 0:  # Ne pas écraser pixels existants
                        result[y_min, x] = color
                    if result[y_max, x] == 0:
                        result[y_max, x] = color
                
                # Gauche et droite
                for y in range(y_min, y_max + 1):
                    if result[y, x_min] == 0:
                        result[y, x_min] = color
                    if result[y, x_max] == 0:
                        result[y, x_max] = color
                
                return result
            
            return ExactPattern(
                name="bounding_rectangle",
                confidence=0.99,
                transform_func=transform,
                description=f"Rectangle englobant avec couleur {rect_color}"
            )
        
        return None
    
    def _detect_l_shape_corner_fill(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    ) -> Optional[ExactPattern]:
        """
        Pattern: Ajouter pixel au coin intérieur des L-shapes
        
        Exemple: Puzzle 007bbfb7
        - Input: Objets en forme de L (3 pixels)
        - Output: Pixel ajouté au coin intérieur
        """
        inp, out = train_pairs[0]
        
        # Compter objets 3-pixels dans input
        objects_3px = []
        visited = set()
        
        for i in range(inp.shape[0]):
            for j in range(inp.shape[1]):
                if inp[i, j] > 0 and (i, j) not in visited:
                    # Extraire objet connecté
                    obj_pixels = self._extract_connected_component(inp, i, j, visited)
                    if len(obj_pixels) == 3:
                        objects_3px.append(obj_pixels)
        
        if len(objects_3px) == 0:
            return None
        
        # Vérifier si output a pixels ajoutés aux coins
        pixels_added = 0
        corner_color = None
        
        for obj in objects_3px:
            ys = [p[0] for p in obj]
            xs = [p[1] for p in obj]
            
            y_min, y_max = min(ys), max(ys)
            x_min, x_max = min(xs), max(xs)
            
            # Tester coins
            corners = [
                (y_min, x_min), (y_min, x_max),
                (y_max, x_min), (y_max, x_max)
            ]
            
            # CORRECTION: Vérifier bounds pour output aussi
            for y, x in corners:
                if (0 <= y < inp.shape[0] and 0 <= x < inp.shape[1] and
                    0 <= y < out.shape[0] and 0 <= x < out.shape[1] and
                    inp[y, x] == 0 and out[y, x] > 0):
                    pixels_added += 1
                    if corner_color is None:
                        corner_color = int(out[y, x])
        
        if pixels_added > 0 and corner_color is not None:
            # Pattern détecté!
            def transform(grid: np.ndarray) -> np.ndarray:
                result = grid.copy()
                visited = set()
                
                # Trouver objets 3-pixels
                for i in range(grid.shape[0]):
                    for j in range(grid.shape[1]):
                        if grid[i, j] > 0 and (i, j) not in visited:
                            obj = self._extract_connected_component(grid, i, j, visited)
                            
                            if len(obj) == 3:
                                # Trouver coin intérieur
                                ys = [p[0] for p in obj]
                                xs = [p[1] for p in obj]
                                
                                y_min, y_max = min(ys), max(ys)
                                x_min, x_max = min(xs), max(xs)
                                
                                corners = [
                                    (y_min, x_min), (y_min, x_max),
                                    (y_max, x_min), (y_max, x_max)
                                ]
                                
                                for y, x in corners:
                                    if (0 <= y < result.shape[0] and 
                                        0 <= x < result.shape[1] and
                                        result[y, x] == 0):
                                        
                                        # Vérifier si adjacent à 2 pixels de l'objet
                                        adj_count = 0
                                        for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                                            ny, nx = y + dy, x + dx
                                            if (ny, nx) in obj:
                                                adj_count += 1
                                        
                                        if adj_count >= 2:
                                            result[y, x] = corner_color
                                            break
                
                return result
            
            return ExactPattern(
                name="l_shape_corner_fill",
                confidence=0.97,
                transform_func=transform,
                description=f"Remplir coins L-shapes avec couleur {corner_color}"
            )
        
        return None
    
    def _extract_connected_component(
        self,
        grid: np.ndarray,
        start_y: int,
        start_x: int,
        visited: set
    ) -> List[Tuple[int, int]]:
        """Extraire composante connectée (4-connectivity)"""
        pixels = []
        stack = [(start_y, start_x)]
        color = grid[start_y, start_x]
        
        while stack:
            y, x = stack.pop()
            
            if (y, x) in visited:
                continue
            
            if (0 <= y < grid.shape[0] and 
                0 <= x < grid.shape[1] and
                grid[y, x] == color):
                
                visited.add((y, x))
                pixels.append((y, x))
                
                # 4-connectivity
                for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    stack.append((y + dy, x + dx))
        
        return pixels

# Made with Bob
