"""
PHASE 3 V39: World State Graph
Carte spatiale persistante du puzzle pour MAGEN.

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import numpy as np
from typing import Dict, List, Tuple, Set, Optional
from dataclasses import dataclass, field
from datetime import datetime
import json


@dataclass
class GridCell:
    """Cellule de la grille du puzzle."""
    x: int
    y: int
    value: Optional[int] = None
    visited_count: int = 0
    last_visited: Optional[float] = None
    transformations_applied: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'x': self.x,
            'y': self.y,
            'value': self.value,
            'visited_count': self.visited_count,
            'last_visited': self.last_visited,
            'transformations': self.transformations_applied
        }


@dataclass
class Region:
    """Région du puzzle (groupe de cellules connexes)."""
    region_id: str
    cells: Set[Tuple[int, int]] = field(default_factory=set)
    properties: Dict = field(default_factory=dict)
    discovered_at: Optional[float] = None
    
    def add_cell(self, x: int, y: int):
        """Ajoute une cellule à la région."""
        self.cells.add((x, y))
    
    def size(self) -> int:
        """Retourne la taille de la région."""
        return len(self.cells)
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'region_id': self.region_id,
            'size': self.size(),
            'cells': list(self.cells),
            'properties': self.properties,
            'discovered_at': self.discovered_at
        }


class WorldStateGraph:
    """
    Graphe d'état du monde pour MAGEN.
    Maintient une carte spatiale persistante du puzzle.
    """
    
    def __init__(self, width: int, height: int):
        """
        Initialise le graphe d'état du monde.
        
        Args:
            width: Largeur de la grille
            height: Hauteur de la grille
        """
        self.width = width
        self.height = height
        
        # Grille de cellules
        self.grid: Dict[Tuple[int, int], GridCell] = {}
        for x in range(width):
            for y in range(height):
                self.grid[(x, y)] = GridCell(x=x, y=y)
        
        # Régions découvertes
        self.regions: Dict[str, Region] = {}
        
        # Graphe de connectivité (adjacence)
        self.adjacency: Dict[Tuple[int, int], Set[Tuple[int, int]]] = {}
        self._build_adjacency_graph()
        
        # Statistiques
        self.stats = {
            'cells_visited': 0,
            'regions_discovered': 0,
            'total_transformations': 0,
            'coverage': 0.0
        }
        
        # Forensic logging
        self.forensic_events: List[Dict] = []
    
    def _build_adjacency_graph(self):
        """Construit le graphe d'adjacence (4-connectivité)."""
        for x in range(self.width):
            for y in range(self.height):
                neighbors = set()
                # Haut, Bas, Gauche, Droite
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < self.width and 0 <= ny < self.height:
                        neighbors.add((nx, ny))
                self.adjacency[(x, y)] = neighbors
    
    def visit_cell(self, x: int, y: int, value: Optional[int] = None,
                   transformation: Optional[str] = None) -> GridCell:
        """
        Visite une cellule et met à jour son état.
        
        Args:
            x: Coordonnée x
            y: Coordonnée y
            value: Valeur de la cellule (optionnel)
            transformation: Transformation appliquée (optionnel)
        
        Returns:
            Cellule visitée
        """
        cell = self.grid[(x, y)]
        
        # Première visite?
        if cell.visited_count == 0:
            self.stats['cells_visited'] += 1
        
        # Mettre à jour cellule
        cell.visited_count += 1
        cell.last_visited = datetime.now().timestamp()
        
        if value is not None:
            cell.value = value
        
        if transformation:
            cell.transformations_applied.append(transformation)
            self.stats['total_transformations'] += 1
        
        # Mettre à jour couverture
        self.stats['coverage'] = self.stats['cells_visited'] / (self.width * self.height)
        
        # Log forensic
        self._log_forensic_event({
            'event': 'cell_visit',
            'position': {'x': x, 'y': y},
            'value': value,
            'transformation': transformation,
            'visit_count': cell.visited_count
        })
        
        return cell
    
    def discover_region(self, region_id: str, seed_x: int, seed_y: int,
                       similarity_fn=None) -> Region:
        """
        Découvre une région à partir d'une cellule seed.
        Utilise flood-fill avec fonction de similarité.
        
        Args:
            region_id: ID de la région
            seed_x: Coordonnée x de départ
            seed_y: Coordonnée y de départ
            similarity_fn: Fonction de similarité (optionnel)
        
        Returns:
            Région découverte
        """
        if region_id in self.regions:
            return self.regions[region_id]
        
        # Créer nouvelle région
        region = Region(
            region_id=region_id,
            discovered_at=datetime.now().timestamp()
        )
        
        # Flood-fill pour découvrir cellules connexes
        to_visit = [(seed_x, seed_y)]
        visited = set()
        
        while to_visit:
            x, y = to_visit.pop(0)
            
            if (x, y) in visited:
                continue
            
            visited.add((x, y))
            region.add_cell(x, y)
            
            # Ajouter voisins si similaires
            for nx, ny in self.adjacency[(x, y)]:
                if (nx, ny) not in visited:
                    # Si pas de fonction de similarité, ajouter tous les voisins
                    if similarity_fn is None:
                        to_visit.append((nx, ny))
                    else:
                        # Sinon, vérifier similarité
                        if similarity_fn(self.grid[(x, y)], self.grid[(nx, ny)]):
                            to_visit.append((nx, ny))
        
        # Sauvegarder région
        self.regions[region_id] = region
        self.stats['regions_discovered'] += 1
        
        # Log forensic
        self._log_forensic_event({
            'event': 'region_discovered',
            'region_id': region_id,
            'size': region.size(),
            'seed': {'x': seed_x, 'y': seed_y}
        })
        
        return region
    
    def get_cell(self, x: int, y: int) -> Optional[GridCell]:
        """Retourne une cellule."""
        return self.grid.get((x, y))
    
    def get_region(self, region_id: str) -> Optional[Region]:
        """Retourne une région."""
        return self.regions.get(region_id)
    
    def get_neighbors(self, x: int, y: int) -> List[GridCell]:
        """Retourne les cellules voisines."""
        neighbors = []
        for nx, ny in self.adjacency.get((x, y), []):
            neighbors.append(self.grid[(nx, ny)])
        return neighbors
    
    def get_unvisited_cells(self) -> List[GridCell]:
        """Retourne les cellules non visitées."""
        return [cell for cell in self.grid.values() if cell.visited_count == 0]
    
    def get_coverage(self) -> float:
        """Retourne le pourcentage de couverture."""
        return self.stats['coverage']
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            **self.stats,
            'total_cells': self.width * self.height,
            'unvisited_cells': len(self.get_unvisited_cells())
        }
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'width': self.width,
            'height': self.height,
            'stats': self.get_stats(),
            'regions': {rid: r.to_dict() for rid, r in self.regions.items()},
            'cells_sample': [
                self.grid[(x, y)].to_dict()
                for x in range(min(5, self.width))
                for y in range(min(5, self.height))
            ]
        }
    
    def _log_forensic_event(self, event: Dict):
        """Log événement forensic."""
        event['timestamp'] = datetime.now().isoformat()
        self.forensic_events.append(event)
    
    def save_forensic_log(self, filepath: str):
        """Sauvegarde le log forensic."""
        with open(filepath, 'w') as f:
            for event in self.forensic_events:
                f.write(json.dumps(event) + '\n')


def test_world_state_graph():
    """Test unitaire du World State Graph."""
    print("="*80)
    print("TEST WORLD STATE GRAPH V39")
    print("="*80)
    
    # Créer graphe 10x10
    graph = WorldStateGraph(width=10, height=10)
    
    print(f"\nGraphe créé: {graph.width}x{graph.height}")
    print(f"Total cellules: {graph.width * graph.height}")
    
    # Simuler exploration
    print("\nSimulation exploration...")
    for i in range(50):
        x = np.random.randint(0, 10)
        y = np.random.randint(0, 10)
        value = np.random.randint(0, 10)
        transformation = f"transform_{i%5}"
        
        graph.visit_cell(x, y, value, transformation)
    
    # Découvrir régions
    print("\nDécouverte régions...")
    for i in range(3):
        x = np.random.randint(0, 10)
        y = np.random.randint(0, 10)
        region = graph.discover_region(f"region_{i}", x, y)
        print(f"  Région {i}: {region.size()} cellules")
    
    # Statistiques
    print("\n" + "="*80)
    print("STATISTIQUES")
    print("="*80)
    
    stats = graph.get_stats()
    print(f"Cellules visitées: {stats['cells_visited']}/{stats['total_cells']}")
    print(f"Couverture: {stats['coverage']:.1%}")
    print(f"Régions découvertes: {stats['regions_discovered']}")
    print(f"Transformations appliquées: {stats['total_transformations']}")
    print(f"Cellules non visitées: {stats['unvisited_cells']}")
    
    # Sauvegarder forensic
    graph.save_forensic_log('/tmp/world_state_graph_test.jsonl')
    print(f"\nForensic log sauvegardé: /tmp/world_state_graph_test.jsonl")
    
    # Verdict
    print("\n" + "="*80)
    if stats['coverage'] > 0.3 and stats['regions_discovered'] > 0:
        print("✅ TEST RÉUSSI - World State Graph fonctionnel")
    else:
        print("⚠️  TEST PARTIEL - Couverture ou régions insuffisantes")
    print("="*80)


if __name__ == '__main__':
    test_world_state_graph()

# Made with Bob
