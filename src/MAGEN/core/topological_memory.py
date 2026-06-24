"""
MAGEN V31 - Topological Memory (C3)
====================================

**Objectif**: Créer mémoire spatiale globale avec détection loops/dead-ends

**Principe**: Le système doit savoir "Où suis-je dans la structure globale du monde ?"
et pas seulement "Quelle action locale semble bonne ?"

**Session**: 73 - Jour 8 - Phase 1
**Date**: 2026-06-15
**Version**: 1.0.0
"""

import time
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from collections import defaultdict, deque
import numpy as np


@dataclass
class TopologicalNode:
    """Nœud dans graphe topologique"""
    position: Tuple[int, int]
    node_type: str = "unknown"  # hub, corridor, dead_end, frontier, central
    connectivity: int = 0
    visit_count: int = 0
    is_loop_member: bool = False
    is_dead_end: bool = False
    exploration_pressure: float = 0.0


@dataclass
class Loop:
    """Boucle détectée dans trajectoire"""
    loop_id: str
    positions: List[Tuple[int, int]]
    entry_step: int
    exit_step: Optional[int] = None
    repetition_count: int = 0
    total_cost: float = 0.0


class TopologicalMemory:
    """
    Mémoire topologique globale du monde
    
    Construit et maintient:
    - Graphe connectivité régions
    - Détection loops répétitifs
    - Identification dead-ends
    - Calcul pression exploration
    - Estimation position globale
    """
    
    def __init__(self, forensic_logger=None):
        """
        Initialise mémoire topologique
        
        Args:
            forensic_logger: Logger forensique pour traçabilité
        """
        # Graphe topologique
        self.nodes: Dict[Tuple[int, int], TopologicalNode] = {}
        self.connectivity_graph: Dict[Tuple[int, int], Set[Tuple[int, int]]] = defaultdict(set)
        
        # Détection loops
        self.active_loops: Dict[str, Loop] = {}
        self.completed_loops: List[Loop] = []
        self.loop_detection_window: int = 20  # Fenêtre détection
        
        # Dead-ends
        self.dead_ends: Set[Tuple[int, int]] = set()
        self.dead_end_corridors: List[List[Tuple[int, int]]] = []
        
        # Hubs et corridors
        self.hubs: Set[Tuple[int, int]] = set()
        self.corridors: List[List[Tuple[int, int]]] = []
        
        # Historique trajectoire
        self.trajectory_history: List[Tuple[int, int]] = []
        self.position_sequence: deque = deque(maxlen=self.loop_detection_window)
        
        # Métriques globales
        self.total_loops_detected: int = 0
        self.total_dead_ends_detected: int = 0
        self.current_step: int = 0
        
        # Forensic
        self.forensic_logger = forensic_logger
        self.events: List[Dict] = []
        
        self._log_event("topological_memory_initialized", None, {
            "timestamp": time.perf_counter_ns()
        })
    
    def build_connectivity_graph(
        self,
        position: Tuple[int, int],
        neighbors: List[Tuple[int, int]]
    ) -> None:
        """
        Construit graphe connectivité
        
        Args:
            position: Position courante
            neighbors: Positions voisines accessibles
        """
        # Créer nœud si nécessaire
        if position not in self.nodes:
            self.nodes[position] = TopologicalNode(position=position)
        
        node = self.nodes[position]
        node.visit_count += 1
        
        # Mettre à jour connectivité
        for neighbor in neighbors:
            self.connectivity_graph[position].add(neighbor)
            
            # Créer nœud voisin si nécessaire
            if neighbor not in self.nodes:
                self.nodes[neighbor] = TopologicalNode(position=neighbor)
        
        # Calculer connectivité
        node.connectivity = len(self.connectivity_graph[position])
        
        # Classifier type nœud
        self._classify_node_type(position)
        
        self._log_event("connectivity_updated", position, {
            "position": position,
            "connectivity": node.connectivity,
            "node_type": node.node_type,
            "neighbors_count": len(neighbors)
        })
    
    def detect_loops(
        self,
        current_position: Tuple[int, int]
    ) -> Optional[Loop]:
        """
        Détecte boucles répétitives dans trajectoire
        
        Args:
            current_position: Position courante
            
        Returns:
            Loop détectée ou None
        """
        self.current_step += 1
        self.trajectory_history.append(current_position)
        self.position_sequence.append(current_position)
        
        # Chercher répétitions dans fenêtre
        sequence = list(self.position_sequence)
        
        # Détecter pattern répété
        for pattern_len in range(3, len(sequence) // 2 + 1):
            pattern = sequence[-pattern_len:]
            
            # Vérifier si pattern se répète
            if len(sequence) >= 2 * pattern_len:
                prev_pattern = sequence[-2*pattern_len:-pattern_len]
                
                if pattern == prev_pattern:
                    # Loop détectée !
                    loop_id = f"loop_{self.total_loops_detected}"
                    
                    loop = Loop(
                        loop_id=loop_id,
                        positions=pattern,
                        entry_step=self.current_step - 2*pattern_len,
                        repetition_count=1
                    )
                    
                    # Vérifier si loop déjà active
                    existing = False
                    for active_loop in self.active_loops.values():
                        if active_loop.positions == pattern:
                            active_loop.repetition_count += 1
                            existing = True
                            loop = active_loop
                            break
                    
                    if not existing:
                        self.active_loops[loop_id] = loop
                        self.total_loops_detected += 1
                        
                        # Marquer nœuds comme membres loop
                        for pos in pattern:
                            if pos in self.nodes:
                                self.nodes[pos].is_loop_member = True
                    
                    self._log_event("loop_detected", current_position, {
                        "loop_id": loop_id,
                        "pattern_length": pattern_len,
                        "repetition_count": loop.repetition_count,
                        "positions": pattern,
                        "step": self.current_step
                    })
                    
                    return loop
        
        return None
    
    def identify_dead_ends(
        self,
        position: Tuple[int, int],
        max_connectivity: int = 1
    ) -> bool:
        """
        Identifie dead-ends (impasses)
        
        Args:
            position: Position à vérifier
            max_connectivity: Connectivité max pour dead-end
            
        Returns:
            True si dead-end détecté
        """
        if position not in self.nodes:
            return False
        
        node = self.nodes[position]
        
        # Dead-end = faible connectivité + visité plusieurs fois
        is_dead_end = (
            node.connectivity <= max_connectivity and
            node.visit_count >= 2
        )
        
        if is_dead_end and position not in self.dead_ends:
            self.dead_ends.add(position)
            node.is_dead_end = True
            self.total_dead_ends_detected += 1
            
            # Identifier corridor menant au dead-end
            corridor = self._trace_corridor_to_dead_end(position)
            if corridor:
                self.dead_end_corridors.append(corridor)
            
            self._log_event("dead_end_detected", position, {
                "position": position,
                "connectivity": node.connectivity,
                "visit_count": node.visit_count,
                "corridor_length": len(corridor) if corridor else 0,
                "total_dead_ends": self.total_dead_ends_detected
            })
            
            return True
        
        return False
    
    def compute_exploration_pressure(
        self,
        position: Tuple[int, int],
        visited_positions: Set[Tuple[int, int]]
    ) -> float:
        """
        Calcule pression exploration pour position
        
        Args:
            position: Position à évaluer
            visited_positions: Ensemble positions visitées
            
        Returns:
            Score pression exploration
        """
        if position not in self.nodes:
            return 0.0
        
        node = self.nodes[position]
        
        # Compter voisins inconnus
        unknown_neighbors = 0
        for neighbor in self.connectivity_graph.get(position, set()):
            if neighbor not in visited_positions:
                unknown_neighbors += 1
        
        # Densité revisites locales
        local_positions = self._get_local_region(position, radius=3)
        revisit_density = sum(
            self.nodes[p].visit_count 
            for p in local_positions 
            if p in self.nodes
        ) / max(1, len(local_positions))
        
        # Stérilité (dead-end ou loop)
        sterility = 1.0 if (node.is_dead_end or node.is_loop_member) else 0.0
        
        # Formule pression
        pressure = (
            unknown_neighbors * 0.5 -
            revisit_density * 0.3 -
            sterility * 0.2
        )
        
        node.exploration_pressure = max(0.0, pressure)
        
        return node.exploration_pressure
    
    def estimate_global_position(
        self,
        position: Tuple[int, int]
    ) -> str:
        """
        Estime position globale dans structure monde
        
        Args:
            position: Position à classifier
            
        Returns:
            Type position: "central", "corridor", "dead_end", "frontier", "hub"
        """
        if position not in self.nodes:
            return "unknown"
        
        node = self.nodes[position]
        
        # Dead-end
        if node.is_dead_end:
            return "dead_end"
        
        # Hub (haute connectivité)
        if node.connectivity >= 4:
            if position not in self.hubs:
                self.hubs.add(position)
            return "hub"
        
        # Corridor (connectivité 2)
        if node.connectivity == 2:
            return "corridor"
        
        # Frontier (faible visite, connectivité moyenne)
        if node.visit_count <= 2 and node.connectivity >= 2:
            return "frontier"
        
        # Central (haute visite, connectivité moyenne)
        if node.visit_count >= 5:
            return "central"
        
        return "unknown"
    
    def get_topology_statistics(self) -> Dict:
        """
        Retourne statistiques topologiques
        
        Returns:
            Dict avec métriques
        """
        # Classifier tous les nœuds
        node_types = defaultdict(int)
        for pos, node in self.nodes.items():
            node_type = self.estimate_global_position(pos)
            node_types[node_type] += 1
        
        # Loops actives
        active_loops_count = len(self.active_loops)
        total_loop_repetitions = sum(
            loop.repetition_count 
            for loop in self.active_loops.values()
        )
        
        return {
            "total_nodes": len(self.nodes),
            "total_loops_detected": self.total_loops_detected,
            "active_loops": active_loops_count,
            "total_loop_repetitions": total_loop_repetitions,
            "total_dead_ends": self.total_dead_ends_detected,
            "hubs_count": len(self.hubs),
            "corridors_count": len(self.corridors),
            "node_types": dict(node_types),
            "current_step": self.current_step
        }
    
    def _classify_node_type(
        self,
        position: Tuple[int, int]
    ) -> None:
        """Classifie type nœud"""
        if position not in self.nodes:
            return
        
        node = self.nodes[position]
        node.node_type = self.estimate_global_position(position)
    
    def _trace_corridor_to_dead_end(
        self,
        dead_end_pos: Tuple[int, int]
    ) -> Optional[List[Tuple[int, int]]]:
        """Trace corridor menant à dead-end"""
        corridor = [dead_end_pos]
        current = dead_end_pos
        
        # Remonter corridor
        for _ in range(20):  # Max 20 steps
            # Trouver voisin avec connectivité 2
            neighbors = list(self.connectivity_graph.get(current, set()))
            
            corridor_neighbor = None
            for neighbor in neighbors:
                if neighbor in self.nodes:
                    if self.nodes[neighbor].connectivity == 2:
                        if neighbor not in corridor:
                            corridor_neighbor = neighbor
                            break
            
            if corridor_neighbor is None:
                break
            
            corridor.append(corridor_neighbor)
            current = corridor_neighbor
        
        return corridor if len(corridor) > 1 else None
    
    def _get_local_region(
        self,
        position: Tuple[int, int],
        radius: int = 3
    ) -> List[Tuple[int, int]]:
        """Retourne positions dans région locale"""
        x, y = position
        region = []
        
        for dx in range(-radius, radius + 1):
            for dy in range(-radius, radius + 1):
                pos = (x + dx, y + dy)
                if pos in self.nodes:
                    region.append(pos)
        
        return region
    
    def _log_event(
        self,
        event_type: str,
        position: Optional[Tuple[int, int]],
        data: Dict
    ) -> None:
        """Log événement forensique"""
        event = {
            "timestamp_ns": time.perf_counter_ns(),
            "event_type": f"topology.{event_type}",
            "position": position,
            "data": data
        }
        
        self.events.append(event)
        
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type=event["event_type"],
                component="topological_memory",
                data={"position": position, **data}
            )
    
    def reset(self) -> None:
        """Reset complet mémoire"""
        self.nodes.clear()
        self.connectivity_graph.clear()
        self.active_loops.clear()
        self.completed_loops.clear()
        self.dead_ends.clear()
        self.dead_end_corridors.clear()
        self.hubs.clear()
        self.corridors.clear()
        self.trajectory_history.clear()
        self.position_sequence.clear()
        self.total_loops_detected = 0
        self.total_dead_ends_detected = 0
        self.current_step = 0
        self.events.clear()
        
        self._log_event("topological_memory_reset", None, {
            "reason": "manual_reset"
        })

# Made with Bob
