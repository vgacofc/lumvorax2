"""
MAGEN V32 - Module C9: SpatialKnowledgeGraph
Spatial Cognitive System - Graphe de Connaissances Spatiales

Mission: Construire graphe relationnel des connaissances spatiales
Relations: Adjacence, accessibilité, causalité, hiérarchie
Inférences: Chemins optimaux, contraintes, opportunités
"""

import time
from typing import Dict, List, Optional, Tuple, Set, Any
from dataclasses import dataclass, field
from collections import defaultdict
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class SpatialNode:
    """Nœud du graphe spatial."""
    node_id: str
    node_type: str  # "position", "region", "landmark", "goal"
    position: Optional[Tuple[int, int]]
    properties: Dict[str, Any] = field(default_factory=dict)
    visit_count: int = 0
    last_visited_step: int = 0


@dataclass
class SpatialEdge:
    """Arête du graphe spatial."""
    edge_id: str
    source_id: str
    target_id: str
    edge_type: str  # "adjacent", "accessible", "leads_to", "blocks", "contains"
    weight: float  # Coût ou distance
    properties: Dict[str, Any] = field(default_factory=dict)
    traversal_count: int = 0


@dataclass
class SpatialConstraint:
    """Contrainte spatiale."""
    constraint_id: str
    constraint_type: str  # "wall", "hazard", "one_way", "conditional"
    affected_nodes: List[str]
    description: str
    severity: float  # 0-1


@dataclass
class InferredPath:
    """Chemin inféré."""
    path_id: str
    source: str
    target: str
    nodes: List[str]
    total_cost: float
    confidence: float
    constraints: List[str]


class SpatialKnowledgeGraph:
    """
    Graphe de Connaissances Spatiales.
    
    Représentation relationnelle de l'espace permettant:
    - Navigation intelligente
    - Inférences topologiques
    - Planification de chemins
    - Détection d'opportunités
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Configuration
        self.max_nodes = 500  # Maximum de nœuds
        self.max_edges = 2000  # Maximum d'arêtes
        self.adjacency_threshold = 1.5  # Distance pour adjacence
        
        # Graphe
        self.nodes: Dict[str, SpatialNode] = {}
        self.edges: Dict[str, SpatialEdge] = {}
        self.adjacency: Dict[str, Set[str]] = defaultdict(set)
        
        # Contraintes
        self.constraints: Dict[str, SpatialConstraint] = {}
        
        # Chemins inférés
        self.inferred_paths: Dict[Tuple[str, str], InferredPath] = {}
        
        # Index spatiaux
        self.position_to_node: Dict[Tuple[int, int], str] = {}
        self.region_nodes: Dict[str, List[str]] = defaultdict(list)
        
        # Statistiques
        self.total_nodes_created: int = 0
        self.total_edges_created: int = 0
        self.total_constraints_added: int = 0
        self.total_paths_inferred: int = 0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/spatial_knowledge_graph.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "SpatialKnowledgeGraph")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "SpatialKnowledgeGraph",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "spatial_knowledge_graph.initialized",
                "spatial_knowledge_graph",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    def add_position_node(self,
                         position: Tuple[int, int],
                         properties: Optional[Dict] = None,
                         step: int = 0) -> str:
        """Ajouter un nœud de position."""
        # Vérifier si existe déjà
        if position in self.position_to_node:
            node_id = self.position_to_node[position]
            node = self.nodes[node_id]
            node.visit_count += 1
            node.last_visited_step = step
            return node_id
        
        # Créer nouveau nœud
        node_id = f"pos_{position[0]}_{position[1]}"
        node = SpatialNode(
            node_id=node_id,
            node_type="position",
            position=position,
            properties=properties or {},
            visit_count=1,
            last_visited_step=step
        )
        
        self.nodes[node_id] = node
        self.position_to_node[position] = node_id
        self.total_nodes_created += 1
        
        return node_id
    
    def add_region_node(self,
                       region_id: str,
                       positions: List[Tuple[int, int]],
                       properties: Optional[Dict] = None) -> str:
        """Ajouter un nœud de région."""
        node_id = f"region_{region_id}"
        
        if node_id in self.nodes:
            return node_id
        
        # Créer nœud région
        node = SpatialNode(
            node_id=node_id,
            node_type="region",
            position=None,  # Région n'a pas de position unique
            properties=properties or {}
        )
        
        self.nodes[node_id] = node
        self.total_nodes_created += 1
        
        # Lier positions à la région
        for pos in positions:
            pos_node_id = self.add_position_node(pos)
            self.region_nodes[node_id].append(pos_node_id)
            
            # Créer arête "contains"
            self.add_edge(node_id, pos_node_id, "contains", weight=0.0)
        
        return node_id
    
    def add_edge(self,
                source_id: str,
                target_id: str,
                edge_type: str,
                weight: float = 1.0,
                properties: Optional[Dict] = None) -> str:
        """Ajouter une arête."""
        edge_id = f"{source_id}_to_{target_id}_{edge_type}"
        
        if edge_id in self.edges:
            # Mettre à jour arête existante
            edge = self.edges[edge_id]
            edge.traversal_count += 1
            return edge_id
        
        # Créer nouvelle arête
        edge = SpatialEdge(
            edge_id=edge_id,
            source_id=source_id,
            target_id=target_id,
            edge_type=edge_type,
            weight=weight,
            properties=properties or {},
            traversal_count=1
        )
        
        self.edges[edge_id] = edge
        self.adjacency[source_id].add(target_id)
        self.total_edges_created += 1
        
        return edge_id
    
    def add_constraint(self,
                      constraint_type: str,
                      affected_nodes: List[str],
                      description: str,
                      severity: float = 0.5) -> str:
        """Ajouter une contrainte spatiale."""
        constraint_id = f"constraint_{len(self.constraints)}"
        
        constraint = SpatialConstraint(
            constraint_id=constraint_id,
            constraint_type=constraint_type,
            affected_nodes=affected_nodes,
            description=description,
            severity=severity
        )
        
        self.constraints[constraint_id] = constraint
        self.total_constraints_added += 1
        
        return constraint_id
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_nodes': len(self.nodes),
            'total_edges': len(self.edges),
            'total_constraints': len(self.constraints),
            'total_queries': self.total_paths_inferred,
            'position_nodes': sum(1 for n in self.nodes.values() if n.node_type == "position"),
            'region_nodes': sum(1 for n in self.nodes.values() if n.node_type == "region")
        }
    
    def update_from_observation(self,
                               position: Tuple[int, int],
                               observation: Dict,
                               mental_map_state: Dict,
                               step: int) -> None:
        """Mettre à jour le graphe depuis une observation."""
        start_ns = time.time_ns()
        
        # Log début update
        self.forensic.forensic_logger.log(
            "update_start",
            "SpatialKnowledgeGraph",
            {
                "step": step,
                "position": position,
                "state": self._forensic_state
            }
        )
        
        # 1. Ajouter nœud position actuelle
        current_node_id = self.add_position_node(position, step=step)
        
        # 2. Détecter positions adjacentes
        if 'grid' in observation:
            self._detect_adjacent_positions(position, observation['grid'], step)
        
        # 3. Intégrer régions depuis mental map
        if mental_map_state.get('num_regions', 0) > 0:
            self._integrate_regions(mental_map_state)
        
        # 4. Détecter contraintes (murs, obstacles)
        if 'grid' in observation:
            self._detect_constraints(position, observation['grid'])
        
        # 5. Mettre à jour accessibilité
        self._update_accessibility()
        
        # Forensic logging
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "update_complete",
            "SpatialKnowledgeGraph",
            {
                "step": step,
                "execution_time_ns": end_ns - start_ns,
                "success": True,
                "state": self._forensic_state
            }
        )
    
    def _detect_adjacent_positions(self,
                                   current_pos: Tuple[int, int],
                                   grid: List[List[str]],
                                   step: int) -> None:
        """Détecter positions adjacentes accessibles."""
        x, y = current_pos
        h, w = len(grid), len(grid[0]) if grid else 0
        
        # Directions cardinales
        directions = [
            (0, -1, 'north'),  # Nord
            (0, 1, 'south'),   # Sud
            (1, 0, 'east'),    # Est
            (-1, 0, 'west')    # Ouest
        ]
        
        current_node_id = self.position_to_node.get(current_pos)
        if not current_node_id:
            return
        
        for dx, dy, direction in directions:
            nx, ny = x + dx, y + dy
            
            # Vérifier limites
            if 0 <= nx < w and 0 <= ny < h:
                cell = grid[ny][nx]
                
                # Vérifier si accessible (pas un mur)
                if cell not in ['#', 'W']:
                    # Ajouter nœud adjacent
                    adjacent_node_id = self.add_position_node((nx, ny), step=step)
                    
                    # Créer arête bidirectionnelle
                    self.add_edge(
                        current_node_id,
                        adjacent_node_id,
                        "adjacent",
                        weight=1.0,
                        properties={"direction": direction}
                    )
                    self.add_edge(
                        adjacent_node_id,
                        current_node_id,
                        "adjacent",
                        weight=1.0,
                        properties={"direction": self._opposite_direction(direction)}
                    )
                else:
                    # Mur détecté - ajouter contrainte
                    wall_node_id = f"wall_{nx}_{ny}"
                    if wall_node_id not in self.nodes:
                        self.add_constraint(
                            "wall",
                            [current_node_id],
                            f"Wall at ({nx}, {ny}) blocks {direction}",
                            severity=1.0
                        )
    
    def _opposite_direction(self, direction: str) -> str:
        """Obtenir direction opposée."""
        opposites = {
            'north': 'south',
            'south': 'north',
            'east': 'west',
            'west': 'east'
        }
        return opposites.get(direction, direction)
    
    def _integrate_regions(self, mental_map_state: Dict) -> None:
        """Intégrer régions depuis mental map."""
        # Cette méthode serait appelée avec les régions du MentalMapBuilder
        # Pour l'instant, structure de base
        pass
    
    def _detect_constraints(self,
                           current_pos: Tuple[int, int],
                           grid: List[List[str]]) -> None:
        """Détecter contraintes spatiales."""
        x, y = current_pos
        h, w = len(grid), len(grid[0]) if grid else 0
        
        # Détecter dangers autour
        for dx in [-1, 0, 1]:
            for dy in [-1, 0, 1]:
                if dx == 0 and dy == 0:
                    continue
                
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    cell = grid[ny][nx]
                    
                    # Symboles de danger
                    if cell in ['H', 'X', '☠', '⚠']:
                        hazard_pos = (nx, ny)
                        if hazard_pos in self.position_to_node:
                            hazard_node_id = self.position_to_node[hazard_pos]
                            self.add_constraint(
                                "hazard",
                                [hazard_node_id],
                                f"Hazard at ({nx}, {ny})",
                                severity=0.8
                            )
    
    def _update_accessibility(self) -> None:
        """Mettre à jour l'accessibilité entre nœuds."""
        # Marquer nœuds accessibles depuis positions visitées
        for node_id, node in self.nodes.items():
            if node.node_type == "position" and node.visit_count > 0:
                node.properties['accessible'] = True
    
    def find_path(self,
                 source_id: str,
                 target_id: str,
                 avoid_constraints: bool = True) -> Optional[InferredPath]:
        """Trouver chemin entre deux nœuds (Dijkstra simplifié)."""
        if source_id not in self.nodes or target_id not in self.nodes:
            return None
        
        # Vérifier cache
        cache_key = (source_id, target_id)
        if cache_key in self.inferred_paths:
            return self.inferred_paths[cache_key]
        
        # Dijkstra
        distances = {source_id: 0.0}
        previous = {}
        unvisited = set(self.nodes.keys())
        
        while unvisited:
            # Trouver nœud non visité avec distance minimale
            current = min(
                unvisited,
                key=lambda n: distances.get(n, float('inf'))
            )
            
            if distances.get(current, float('inf')) == float('inf'):
                break
            
            if current == target_id:
                break
            
            unvisited.remove(current)
            
            # Examiner voisins
            for neighbor_id in self.adjacency.get(current, []):
                if neighbor_id not in unvisited:
                    continue
                
                # Trouver arête
                edge_id = f"{current}_to_{neighbor_id}_adjacent"
                if edge_id not in self.edges:
                    continue
                
                edge = self.edges[edge_id]
                
                # Vérifier contraintes si demandé
                if avoid_constraints:
                    if self._has_blocking_constraint(neighbor_id):
                        continue
                
                # Calculer distance
                alt_distance = distances[current] + edge.weight
                
                if alt_distance < distances.get(neighbor_id, float('inf')):
                    distances[neighbor_id] = alt_distance
                    previous[neighbor_id] = current
        
        # Reconstruire chemin
        if target_id not in previous and target_id != source_id:
            return None
        
        path_nodes = []
        current = target_id
        while current != source_id:
            path_nodes.append(current)
            if current not in previous:
                return None
            current = previous[current]
        path_nodes.append(source_id)
        path_nodes.reverse()
        
        # Créer chemin inféré
        path = InferredPath(
            path_id=f"path_{len(self.inferred_paths)}",
            source=source_id,
            target=target_id,
            nodes=path_nodes,
            total_cost=distances.get(target_id, 0.0),
            confidence=0.8,
            constraints=[]
        )
        
        self.inferred_paths[cache_key] = path
        self.total_paths_inferred += 1
        
        return path
    
    def _has_blocking_constraint(self, node_id: str) -> bool:
        """Vérifier si un nœud a une contrainte bloquante."""
        for constraint in self.constraints.values():
            if node_id in constraint.affected_nodes:
                if constraint.constraint_type in ['wall', 'hazard']:
                    if constraint.severity > 0.7:
                        return True
        return False
    
    def get_reachable_nodes(self, source_id: str, max_distance: int = 10) -> List[str]:
        """Obtenir nœuds accessibles depuis une source."""
        if source_id not in self.nodes:
            return []
        
        reachable = set()
        queue = [(source_id, 0)]
        visited = set()
        
        while queue:
            current, distance = queue.pop(0)
            
            if current in visited:
                continue
            
            visited.add(current)
            reachable.add(current)
            
            if distance >= max_distance:
                continue
            
            # Ajouter voisins
            for neighbor_id in self.adjacency.get(current, []):
                if neighbor_id not in visited:
                    queue.append((neighbor_id, distance + 1))
        
        return list(reachable)
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques."""
        return {
            "total_nodes": len(self.nodes),
            "total_edges": len(self.edges),
            "total_constraints": len(self.constraints),
            "total_paths_inferred": self.total_paths_inferred,
            "position_nodes": sum(1 for n in self.nodes.values() if n.node_type == "position"),
            "region_nodes": sum(1 for n in self.nodes.values() if n.node_type == "region"),
            "visited_nodes": sum(1 for n in self.nodes.values() if n.visit_count > 0)
        }
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter graphe en JSON."""
        data = {
            "statistics": self.get_statistics(),
            "nodes": [
                {
                    "id": n.node_id,
                    "type": n.node_type,
                    "position": list(n.position) if n.position else None,
                    "visit_count": n.visit_count,
                    "properties": n.properties
                }
                for n in list(self.nodes.values())[:100]  # Limiter export
            ],
            "edges": [
                {
                    "id": e.edge_id,
                    "source": e.source_id,
                    "target": e.target_id,
                    "type": e.edge_type,
                    "weight": e.weight,
                    "traversal_count": e.traversal_count
                }
                for e in list(self.edges.values())[:200]  # Limiter export
            ],
            "constraints": [
                {
                    "id": c.constraint_id,
                    "type": c.constraint_type,
                    "description": c.description,
                    "severity": c.severity
                }
                for c in self.constraints.values()
            ]
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "SpatialKnowledgeGraph",
            {
                "final_state": self._forensic_state,
                "total_nodes": len(self.nodes),
                "total_edges": len(self.edges),
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()


# Made with Bob
