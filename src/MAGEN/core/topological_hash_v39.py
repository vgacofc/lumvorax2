"""
Topological Hash V39 - Hashing Topologique pour MAGEN
Utilisé par Layer 2 (Objects) et Layer 5 (World Model)

Objectif:
- Créer des signatures topologiques invariantes
- Détecter les structures isomorphes
- Permettre la comparaison rapide de configurations
- Supporter l'invariance aux transformations géométriques

Auteur: Bob (Assistant IA)
Date: 2026-06-18
Version: 39
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass
import hashlib
from collections import defaultdict


@dataclass
class TopologicalHash:
    """
    Hash topologique d'une structure.
    
    Attributes:
        hash_value: Valeur du hash (hex)
        structure_type: Type de structure détecté
        num_components: Nombre de composantes connexes
        euler_characteristic: Caractéristique d'Euler
        invariants: Invariants topologiques
    """
    hash_value: str
    structure_type: str
    num_components: int
    euler_characteristic: int
    invariants: Dict[str, float]
    
    def __eq__(self, other) -> bool:
        return isinstance(other, TopologicalHash) and self.hash_value == other.hash_value
    
    def __hash__(self) -> int:
        return int(self.hash_value[:16], 16)


class TopologicalHashV39:
    """
    Générateur de hash topologique pour MAGEN.
    
    Fonctionnalités:
    - Hash invariant aux rotations/réflexions
    - Détection de composantes connexes
    - Calcul d'invariants topologiques
    - Comparaison de structures
    """
    
    def __init__(self,
                 rotation_invariant: bool = True,
                 reflection_invariant: bool = True):
        """
        Initialise le générateur de hash.
        
        Args:
            rotation_invariant: Invariance aux rotations
            reflection_invariant: Invariance aux réflexions
        """
        self.rotation_invariant = rotation_invariant
        self.reflection_invariant = reflection_invariant
        
        # Cache de hash
        self.hash_cache: Dict[str, TopologicalHash] = {}
        
        # Statistiques
        self.total_hashes = 0
        self.cache_hits = 0
    
    def compute_hash(self, grid: np.ndarray) -> TopologicalHash:
        """
        Calcule le hash topologique d'une grille.
        
        Args:
            grid: Grille 2D binaire ou multi-valeurs
            
        Returns:
            Hash topologique
        """
        self.total_hashes += 1
        
        # Normaliser la grille
        if grid.dtype != bool:
            grid = grid > 0
        
        # Calculer un hash rapide pour le cache
        quick_hash = hashlib.md5(grid.tobytes()).hexdigest()
        
        if quick_hash in self.hash_cache:
            self.cache_hits += 1
            return self.hash_cache[quick_hash]
        
        # Extraire les composantes connexes
        components = self._find_connected_components(grid)
        num_components = len(components)
        
        # Calculer la caractéristique d'Euler
        euler_char = self._compute_euler_characteristic(grid)
        
        # Calculer les invariants topologiques
        invariants = self._compute_invariants(grid, components)
        
        # Déterminer le type de structure
        structure_type = self._classify_structure(grid, components, invariants)
        
        # Générer le hash canonique
        if self.rotation_invariant or self.reflection_invariant:
            canonical_grid = self._canonicalize(grid)
        else:
            canonical_grid = grid
        
        hash_value = hashlib.sha256(canonical_grid.tobytes()).hexdigest()
        
        # Créer l'objet hash
        topo_hash = TopologicalHash(
            hash_value=hash_value,
            structure_type=structure_type,
            num_components=num_components,
            euler_characteristic=euler_char,
            invariants=invariants
        )
        
        # Mettre en cache
        self.hash_cache[quick_hash] = topo_hash
        
        return topo_hash
    
    def are_isomorphic(self,
                      grid1: np.ndarray,
                      grid2: np.ndarray) -> bool:
        """
        Vérifie si deux grilles sont isomorphes.
        
        Args:
            grid1: Première grille
            grid2: Seconde grille
            
        Returns:
            True si isomorphes
        """
        hash1 = self.compute_hash(grid1)
        hash2 = self.compute_hash(grid2)
        
        return hash1.hash_value == hash2.hash_value
    
    def find_symmetries(self, grid: np.ndarray) -> List[str]:
        """
        Trouve les symétries d'une grille.
        
        Args:
            grid: Grille à analyser
            
        Returns:
            Liste des symétries détectées
        """
        symmetries = []
        
        # Test rotation 90°
        if np.array_equal(grid, np.rot90(grid, k=1)):
            symmetries.append('rotation_90')
        
        # Test rotation 180°
        if np.array_equal(grid, np.rot90(grid, k=2)):
            symmetries.append('rotation_180')
        
        # Test réflexion horizontale
        if np.array_equal(grid, np.fliplr(grid)):
            symmetries.append('reflection_horizontal')
        
        # Test réflexion verticale
        if np.array_equal(grid, np.flipud(grid)):
            symmetries.append('reflection_vertical')
        
        # Test réflexion diagonale
        if grid.shape[0] == grid.shape[1]:
            if np.array_equal(grid, grid.T):
                symmetries.append('reflection_diagonal')
        
        return symmetries
    
    def _find_connected_components(self, grid: np.ndarray) -> List[Set[Tuple[int, int]]]:
        """Trouve les composantes connexes."""
        h, w = grid.shape
        visited = np.zeros_like(grid, dtype=bool)
        components = []
        
        def dfs(y: int, x: int, component: Set):
            """Parcours en profondeur."""
            if y < 0 or y >= h or x < 0 or x >= w:
                return
            if visited[y, x] or not grid[y, x]:
                return
            
            visited[y, x] = True
            component.add((y, x))
            
            # 4-connectivité
            dfs(y-1, x, component)
            dfs(y+1, x, component)
            dfs(y, x-1, component)
            dfs(y, x+1, component)
        
        for y in range(h):
            for x in range(w):
                if grid[y, x] and not visited[y, x]:
                    component = set()
                    dfs(y, x, component)
                    if component:
                        components.append(component)
        
        return components
    
    def _compute_euler_characteristic(self, grid: np.ndarray) -> int:
        """
        Calcule la caractéristique d'Euler.
        χ = V - E + F (sommets - arêtes + faces)
        """
        # Pour une grille 2D: χ = composantes - trous
        components = self._find_connected_components(grid)
        
        # Approximation simple: χ = nombre de composantes
        # (une analyse plus fine nécessiterait la détection de trous)
        return len(components)
    
    def _compute_invariants(self,
                           grid: np.ndarray,
                           components: List[Set[Tuple[int, int]]]) -> Dict[str, float]:
        """Calcule les invariants topologiques."""
        h, w = grid.shape
        
        invariants = {}
        
        # Densité globale
        invariants['density'] = float(grid.sum() / (h * w))
        
        # Taille moyenne des composantes
        if components:
            sizes = [len(c) for c in components]
            invariants['avg_component_size'] = float(np.mean(sizes))
            invariants['max_component_size'] = float(max(sizes))
            invariants['min_component_size'] = float(min(sizes))
        else:
            invariants['avg_component_size'] = 0.0
            invariants['max_component_size'] = 0.0
            invariants['min_component_size'] = 0.0
        
        # Compacité (périmètre / aire)
        total_perimeter = 0
        total_area = grid.sum()
        
        for y in range(h):
            for x in range(w):
                if grid[y, x]:
                    # Compter les voisins vides
                    neighbors = 0
                    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < h and 0 <= nx < w:
                            if not grid[ny, nx]:
                                neighbors += 1
                        else:
                            neighbors += 1
                    total_perimeter += neighbors
        
        if total_area > 0:
            invariants['compactness'] = float(total_perimeter / total_area)
        else:
            invariants['compactness'] = 0.0
        
        # Moments géométriques (invariants aux transformations)
        if total_area > 0:
            y_coords, x_coords = np.where(grid)
            
            # Centre de masse
            cy = y_coords.mean()
            cx = x_coords.mean()
            
            # Moments centrés
            mu20 = ((x_coords - cx) ** 2).sum() / total_area
            mu02 = ((y_coords - cy) ** 2).sum() / total_area
            mu11 = ((x_coords - cx) * (y_coords - cy)).sum() / total_area
            
            # Invariants de Hu (simplifiés)
            invariants['moment_1'] = float(mu20 + mu02)
            invariants['moment_2'] = float((mu20 - mu02) ** 2 + 4 * mu11 ** 2)
        else:
            invariants['moment_1'] = 0.0
            invariants['moment_2'] = 0.0
        
        return invariants
    
    def _classify_structure(self,
                           grid: np.ndarray,
                           components: List[Set[Tuple[int, int]]],
                           invariants: Dict[str, float]) -> str:
        """Classifie le type de structure."""
        num_comp = len(components)
        density = invariants['density']
        compactness = invariants.get('compactness', 0)
        
        if num_comp == 0:
            return 'empty'
        elif num_comp == 1:
            if compactness < 2.5:
                return 'blob'
            elif compactness < 4.0:
                return 'shape'
            else:
                return 'line'
        elif num_comp <= 3:
            return 'sparse'
        else:
            if density > 0.5:
                return 'dense'
            else:
                return 'scattered'
    
    def _canonicalize(self, grid: np.ndarray) -> np.ndarray:
        """
        Canonicalise une grille pour invariance.
        Retourne la représentation canonique minimale.
        """
        candidates = [grid]
        
        if self.rotation_invariant:
            # Ajouter les 4 rotations
            for k in range(1, 4):
                candidates.append(np.rot90(grid, k=k))
        
        if self.reflection_invariant:
            # Ajouter les réflexions
            candidates.append(np.fliplr(grid))
            candidates.append(np.flipud(grid))
            if grid.shape[0] == grid.shape[1]:
                candidates.append(grid.T)
        
        # Choisir la représentation lexicographiquement minimale
        canonical = min(candidates, key=lambda g: g.tobytes())
        
        return canonical
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            'total_hashes': self.total_hashes,
            'cache_hits': self.cache_hits,
            'cache_hit_rate': self.cache_hits / max(1, self.total_hashes),
            'cache_size': len(self.hash_cache)
        }


def test_topological_hash():
    """Test du hashing topologique."""
    print("=== Test Topological Hash V39 ===\n")
    
    hasher = TopologicalHashV39(rotation_invariant=True, reflection_invariant=True)
    
    # Créer une structure simple
    grid1 = np.array([
        [0, 1, 1, 0],
        [0, 1, 1, 0],
        [0, 0, 0, 0],
        [1, 0, 0, 1]
    ], dtype=bool)
    
    print("Grille originale:")
    print(grid1.astype(int))
    
    # Calculer le hash
    hash1 = hasher.compute_hash(grid1)
    print(f"\n✓ Hash calculé:")
    print(f"  - Hash: {hash1.hash_value[:16]}...")
    print(f"  - Type: {hash1.structure_type}")
    print(f"  - Composantes: {hash1.num_components}")
    print(f"  - Euler: {hash1.euler_characteristic}")
    
    # Invariants
    print(f"\n✓ Invariants:")
    for key, value in hash1.invariants.items():
        print(f"  - {key}: {value:.3f}")
    
    # Test rotation
    grid2 = np.rot90(grid1)
    hash2 = hasher.compute_hash(grid2)
    print(f"\n✓ Test rotation 90°:")
    print(f"  - Isomorphe: {hasher.are_isomorphic(grid1, grid2)}")
    
    # Test symétries
    symmetries = hasher.find_symmetries(grid1)
    print(f"\n✓ Symétries détectées: {symmetries if symmetries else 'aucune'}")
    
    # Stats
    stats = hasher.get_stats()
    print(f"\n✓ Statistiques:")
    print(f"  - Hash calculés: {stats['total_hashes']}")
    print(f"  - Cache hits: {stats['cache_hits']}")
    print(f"  - Taux cache: {stats['cache_hit_rate']:.2%}")


if __name__ == '__main__':
    test_topological_hash()

# Made with Bob
