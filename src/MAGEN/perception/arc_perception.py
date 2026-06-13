"""
MAGEN - Module de Perception ARC-AGI-3
Traitement des grilles et extraction de features

Inspiré de GEN23/GEN24: Perception réelle avec extraction features
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass
import hashlib


@dataclass
class GridFeatures:
    """Features extraites d'une grille ARC"""
    # Statistiques basiques
    width: int
    height: int
    total_cells: int
    unique_colors: int
    color_distribution: Dict[int, int]  # {color: count}
    
    # Features spatiales
    density: float  # Proportion cellules non-vides (!=0)
    symmetry_h: float  # Symétrie horizontale [0-1]
    symmetry_v: float  # Symétrie verticale [0-1]
    
    # Features de patterns
    has_border: bool  # Bordure détectée
    has_clusters: bool  # Clusters de couleurs
    cluster_count: int
    
    # Features de complexité
    entropy: float  # Entropie de Shannon
    complexity: float  # Mesure de complexité visuelle
    
    # Signature vectorielle (pour similarité)
    vector: np.ndarray  # Vecteur 64D pour comparaison
    
    def to_dict(self) -> Dict:
        """Sérialisation"""
        return {
            'width': self.width,
            'height': self.height,
            'total_cells': self.total_cells,
            'unique_colors': self.unique_colors,
            'color_distribution': self.color_distribution,
            'density': self.density,
            'symmetry_h': self.symmetry_h,
            'symmetry_v': self.symmetry_v,
            'has_border': self.has_border,
            'has_clusters': self.has_clusters,
            'cluster_count': self.cluster_count,
            'entropy': self.entropy,
            'complexity': self.complexity,
            'vector_shape': self.vector.shape
        }


class ARCPerception:
    """
    Système de perception pour grilles ARC-AGI-3
    
    Fonctionnalités:
    - Extraction features spatiales
    - Détection patterns (bordures, clusters, symétries)
    - Calcul similarité entre grilles
    - Vectorisation pour apprentissage
    """
    
    def __init__(self, max_grid_size: int = 64):
        """
        Initialisation perception ARC
        
        Args:
            max_grid_size: Taille maximale grille (64x64 pour ARC-AGI-3)
        """
        self.max_grid_size = max_grid_size
        self.total_processed = 0
        
        print(f"[ARC Perception] Initialisé (max_size={max_grid_size}x{max_grid_size})")
    
    def extract_features(self, grid: np.ndarray) -> GridFeatures:
        """
        Extraire features complètes d'une grille
        
        Args:
            grid: Grille ARC (HxW, valeurs 0-15) ou (C, H, W)
            
        Returns:
            Features extraites
        """
        # Normaliser grille en 2D (extraire premier canal si 3D)
        if len(grid.shape) == 3:
            # Format (channels, height, width) - prendre premier canal
            if grid.shape[0] <= 10:  # Probablement channels first
                grid = grid[0]  # Extraire premier canal
            else:  # Probablement channels last
                grid = grid[:, :, 0]  # Extraire premier canal
        elif len(grid.shape) != 2:
            raise ValueError(f"Format de grille non supporté: {grid.shape}")
        
        # Dimensions de la grille (maintenant forcément 2D)
        height, width = grid.shape
        total_cells = height * width
        
        # 1. Statistiques basiques
        unique_colors = len(np.unique(grid))
        color_distribution = {
            int(color): int(count) 
            for color, count in zip(*np.unique(grid, return_counts=True))
        }
        
        # 2. Features spatiales
        density = float(np.sum(grid != 0) / total_cells)
        symmetry_h = self._compute_symmetry_horizontal(grid)
        symmetry_v = self._compute_symmetry_vertical(grid)
        
        # 3. Features de patterns
        has_border = self._detect_border(grid)
        clusters = self._detect_clusters(grid)
        has_clusters = len(clusters) > 0
        cluster_count = len(clusters)
        
        # 4. Features de complexité
        entropy = self._compute_entropy(grid)
        complexity = self._compute_complexity(grid)
        
        # 5. Vectorisation (64D)
        vector = self._vectorize_grid(grid)
        
        # Statistiques
        self.total_processed += 1
        
        return GridFeatures(
            width=width,
            height=height,
            total_cells=total_cells,
            unique_colors=unique_colors,
            color_distribution=color_distribution,
            density=density,
            symmetry_h=symmetry_h,
            symmetry_v=symmetry_v,
            has_border=has_border,
            has_clusters=has_clusters,
            cluster_count=cluster_count,
            entropy=entropy,
            complexity=complexity,
            vector=vector
        )
    
    def _compute_symmetry_horizontal(self, grid: np.ndarray) -> float:
        """Calculer symétrie horizontale [0-1]"""
        flipped = np.fliplr(grid)
        matches = np.sum(grid == flipped)
        total = grid.size
        return float(matches / total)
    
    def _compute_symmetry_vertical(self, grid: np.ndarray) -> float:
        """Calculer symétrie verticale [0-1]"""
        flipped = np.flipud(grid)
        matches = np.sum(grid == flipped)
        total = grid.size
        return float(matches / total)
    
    def _detect_border(self, grid: np.ndarray) -> bool:
        """Détecter si grille a une bordure"""
        if grid.shape[0] < 3 or grid.shape[1] < 3:
            return False
        
        # Vérifier bordure (première/dernière ligne/colonne)
        top = grid[0, :]
        bottom = grid[-1, :]
        left = grid[:, 0]
        right = grid[:, -1]
        
        # Bordure = au moins 50% des cellules bordure non-vides
        border_cells = np.concatenate([top, bottom, left, right])
        non_empty = np.sum(border_cells != 0)
        
        return non_empty / len(border_cells) > 0.5
    
    def _detect_clusters(self, grid: np.ndarray) -> List[Tuple[int, int, int]]:
        """
        Détecter clusters de couleurs (flood fill simple)
        
        Returns:
            Liste de (color, size, centroid_x, centroid_y)
        """
        clusters = []
        visited = np.zeros_like(grid, dtype=bool)
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if not visited[i, j] and grid[i, j] != 0:
                    # Nouveau cluster
                    color = grid[i, j]
                    cluster_cells = self._flood_fill(grid, visited, i, j, color)
                    
                    if len(cluster_cells) >= 2:  # Au moins 2 cellules
                        # Calculer centroid
                        centroid_i = int(np.mean([c[0] for c in cluster_cells]))
                        centroid_j = int(np.mean([c[1] for c in cluster_cells]))
                        
                        clusters.append((int(color), len(cluster_cells), centroid_i, centroid_j))
        
        return clusters
    
    def _flood_fill(self, grid: np.ndarray, visited: np.ndarray,
                    i: int, j: int, color: int) -> List[Tuple[int, int]]:
        """
        Flood fill ITÉRATIF pour détecter cluster (évite récursion infinie)
        Utilise une pile pour parcours en profondeur
        """
        # Vérifications initiales
        if i < 0 or i >= grid.shape[0] or j < 0 or j >= grid.shape[1]:
            return []
        if visited[i, j] or grid[i, j] != color:
            return []
        
        # Pile pour parcours itératif
        stack = [(i, j)]
        cells = []
        
        # Directions 4-connectivité (haut, bas, gauche, droite)
        directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
        
        while stack:
            ci, cj = stack.pop()
            
            # Vérifier si déjà visité
            if visited[ci, cj]:
                continue
            
            # Marquer comme visité
            visited[ci, cj] = True
            cells.append((ci, cj))
            
            # Explorer voisins
            for di, dj in directions:
                ni, nj = ci + di, cj + dj
                
                # Vérifier limites
                if 0 <= ni < grid.shape[0] and 0 <= nj < grid.shape[1]:
                    # Vérifier couleur et non visité
                    if not visited[ni, nj] and grid[ni, nj] == color:
                        stack.append((ni, nj))
        
        return cells
    
    def _compute_entropy(self, grid: np.ndarray) -> float:
        """Calculer entropie de Shannon"""
        _, counts = np.unique(grid, return_counts=True)
        probabilities = counts / grid.size
        
        # Entropie: -sum(p * log2(p))
        entropy = -np.sum(probabilities * np.log2(probabilities + 1e-10))
        return float(entropy)
    
    def _compute_complexity(self, grid: np.ndarray) -> float:
        """
        Calculer complexité visuelle (transitions entre cellules)
        
        Complexité = nombre de transitions / transitions_max
        """
        # Transitions horizontales
        h_transitions = np.sum(grid[:, :-1] != grid[:, 1:])
        
        # Transitions verticales
        v_transitions = np.sum(grid[:-1, :] != grid[1:, :])
        
        # Total transitions
        total_transitions = h_transitions + v_transitions
        
        # Max transitions possible
        max_transitions = (grid.shape[0] * (grid.shape[1] - 1) + 
                          grid.shape[1] * (grid.shape[0] - 1))
        
        complexity = total_transitions / max(1, max_transitions)
        return float(complexity)
    
    def _vectorize_grid(self, grid: np.ndarray) -> np.ndarray:
        """
        Vectoriser grille en vecteur 64D pour comparaison
        
        Vecteur contient:
        - 16 bins pour distribution couleurs (0-15)
        - 16 bins pour distribution spatiale (quadrants 4x4)
        - 16 bins pour gradients horizontaux
        - 16 bins pour gradients verticaux
        """
        vector = np.zeros(64, dtype=np.float32)
        
        # 1. Distribution couleurs (bins 0-15)
        for color in range(16):
            vector[color] = float(np.sum(grid == color) / grid.size)
        
        # 2. Distribution spatiale (bins 16-31)
        # Diviser grille en 4x4 quadrants
        h_step = max(1, grid.shape[0] // 4)
        w_step = max(1, grid.shape[1] // 4)
        
        idx = 16
        for i in range(4):
            for j in range(4):
                quadrant = grid[i*h_step:(i+1)*h_step, j*w_step:(j+1)*w_step]
                vector[idx] = float(np.sum(quadrant != 0) / max(1, quadrant.size))
                idx += 1
        
        # 3. Gradients horizontaux (bins 32-47)
        if grid.shape[1] > 1:
            h_grad = np.abs(grid[:, 1:].astype(float) - grid[:, :-1].astype(float))
            for i in range(16):
                vector[32 + i] = float(np.sum(h_grad == i) / max(1, h_grad.size))
        
        # 4. Gradients verticaux (bins 48-63)
        if grid.shape[0] > 1:
            v_grad = np.abs(grid[1:, :].astype(float) - grid[:-1, :].astype(float))
            for i in range(16):
                vector[48 + i] = float(np.sum(v_grad == i) / max(1, v_grad.size))
        
        return vector
    
    def compute_similarity(self, grid1: np.ndarray, grid2: np.ndarray) -> float:
        """
        Calculer similarité entre deux grilles [0-1]
        
        Args:
            grid1: Première grille
            grid2: Deuxième grille
            
        Returns:
            Score similarité (1.0 = identiques, 0.0 = très différentes)
        """
        # Extraire features
        features1 = self.extract_features(grid1)
        features2 = self.extract_features(grid2)
        
        # Similarité basée sur distance cosine des vecteurs
        dot_product = np.dot(features1.vector, features2.vector)
        norm1 = np.linalg.norm(features1.vector)
        norm2 = np.linalg.norm(features2.vector)
        
        if norm1 == 0 or norm2 == 0:
            return 0.0
        
        cosine_similarity = dot_product / (norm1 * norm2)
        
        # Normaliser [0-1]
        similarity = (cosine_similarity + 1.0) / 2.0
        
        return float(similarity)
    
    def detect_transformation(self, grid_before: np.ndarray,
                            grid_after: np.ndarray) -> Dict[str, Any]:
        """
        Détecter type de transformation entre deux grilles
        
        Args:
            grid_before: Grille avant action
            grid_after: Grille après action
            
        Returns:
            Dictionnaire décrivant transformation
        """
        # Vérifier si grilles identiques
        if np.array_equal(grid_before, grid_after):
            return {
                'type': 'no_change',
                'cells_changed': 0,
                'change_ratio': 0.0
            }
        
        # Compter cellules changées
        diff = grid_before != grid_after
        cells_changed = int(np.sum(diff))
        change_ratio = float(cells_changed / grid_before.size)
        
        # Détecter type transformation
        transformation_type = 'unknown'
        
        # Rotation?
        if self._is_rotation(grid_before, grid_after):
            transformation_type = 'rotation'
        # Flip?
        elif self._is_flip(grid_before, grid_after):
            transformation_type = 'flip'
        # Ajout/suppression?
        elif change_ratio < 0.1:
            transformation_type = 'local_change'
        # Transformation globale?
        elif change_ratio > 0.5:
            transformation_type = 'global_change'
        else:
            transformation_type = 'partial_change'
        
        return {
            'type': transformation_type,
            'cells_changed': cells_changed,
            'change_ratio': change_ratio,
            'similarity': self.compute_similarity(grid_before, grid_after)
        }
    
    def _is_rotation(self, grid1: np.ndarray, grid2: np.ndarray) -> bool:
        """Vérifier si grid2 est rotation de grid1"""
        if grid1.shape != grid2.shape:
            return False
        
        # Tester rotations 90°, 180°, 270°
        for k in [1, 2, 3]:
            rotated = np.rot90(grid1, k)
            if np.array_equal(rotated, grid2):
                return True
        
        return False
    
    def _is_flip(self, grid1: np.ndarray, grid2: np.ndarray) -> bool:
        """Vérifier si grid2 est flip de grid1"""
        if grid1.shape != grid2.shape:
            return False
        
        # Tester flip horizontal et vertical
        if np.array_equal(np.fliplr(grid1), grid2):
            return True
        if np.array_equal(np.flipud(grid1), grid2):
            return True
        
        return False
    
    def __repr__(self) -> str:
        """Représentation string"""
        return (
            f"ARCPerception(\n"
            f"  Max grid size: {self.max_grid_size}x{self.max_grid_size}\n"
            f"  Total processed: {self.total_processed}\n"
            f")"
        )

# Made with Bob
