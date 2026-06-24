"""
MAGEN V31 - Persistent World Model (C2)
========================================

**Objectif**: Construire carte persistante du monde avec mémoire topologique

**Principe**: Le système doit mémoriser régions explorées, transitions, zones stériles,
frontières inconnues, et trajectoires historiques

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
class WorldRegion:
    """Région du monde avec métriques"""
    region_id: str
    center: Tuple[int, int]
    positions: Set[Tuple[int, int]] = field(default_factory=set)
    visit_count: int = 0
    total_reward: float = 0.0
    sterility_score: float = 0.0
    novelty: float = 1.0
    last_visit_step: int = 0
    is_frontier: bool = False
    is_sterile: bool = False
    is_high_value: bool = False


@dataclass
class Transition:
    """Transition entre positions"""
    from_pos: Tuple[int, int]
    to_pos: Tuple[int, int]
    action: str
    count: int = 0
    total_reward: float = 0.0
    avg_reward: float = 0.0
    cognitive_cost: float = 0.0


class PersistentWorldModel:
    """
    Modèle persistant du monde avec mémoire topologique
    
    Construit et maintient:
    - Carte positions visitées
    - Graphe transitions
    - Régions identifiées
    - Zones stériles
    - Frontières inconnues
    - Historique trajectoires
    """
    
    def __init__(self, forensic_logger=None):
        """
        Initialise le modèle monde persistant
        
        Args:
            forensic_logger: Logger forensique pour traçabilité
        """
        # Positions et régions
        self.visited_positions: Set[Tuple[int, int]] = set()
        self.position_visits: Dict[Tuple[int, int], int] = defaultdict(int)
        self.position_rewards: Dict[Tuple[int, int], List[float]] = defaultdict(list)
        
        # Régions
        self.regions: Dict[str, WorldRegion] = {}
        self.position_to_region: Dict[Tuple[int, int], str] = {}
        
        # Transitions
        self.transitions: Dict[Tuple[Tuple[int, int], str], Transition] = {}
        self.transition_graph: Dict[Tuple[int, int], List[Tuple[int, int]]] = defaultdict(list)
        
        # Historique
        self.position_history: List[Tuple[int, int]] = []
        self.trajectory_history: List[List[Tuple[int, int]]] = []
        self.current_trajectory: List[Tuple[int, int]] = []
        
        # Heatmap exploration
        self.exploration_heatmap: Dict[Tuple[int, int], float] = defaultdict(float)
        
        # Frontières et zones spéciales
        self.frontier_positions: Set[Tuple[int, int]] = set()
        self.sterile_positions: Set[Tuple[int, int]] = set()
        self.high_value_positions: Set[Tuple[int, int]] = set()
        
        # Métriques globales
        self.total_coverage: float = 0.0
        self.exploration_entropy: float = 0.0
        self.current_step: int = 0
        
        # Forensic
        self.forensic_logger = forensic_logger
        self.events: List[Dict] = []
        
        self._log_event("world_model_initialized", None, {
            "timestamp": time.perf_counter_ns()
        })
    
    def update_position(
        self,
        position: Tuple[int, int],
        reward: float = 0.0,
        action: Optional[str] = None
    ) -> None:
        """
        Met à jour position courante et historique
        
        Args:
            position: Position (x, y)
            reward: Reward obtenu
            action: Action exécutée
        """
        self.current_step += 1
        
        # Mettre à jour visites
        is_new = position not in self.visited_positions
        self.visited_positions.add(position)
        self.position_visits[position] += 1
        self.position_rewards[position].append(reward)
        
        # Mettre à jour historique
        self.position_history.append(position)
        self.current_trajectory.append(position)
        
        # Mettre à jour heatmap
        self.exploration_heatmap[position] += 1.0
        
        # Enregistrer transition si action fournie
        if action and len(self.position_history) > 1:
            prev_pos = self.position_history[-2]
            self.register_transition(prev_pos, position, action, reward)
        
        # Mettre à jour région
        self._update_region_membership(position, reward)
        
        # Détecter frontières si nouvelle position
        if is_new:
            self._identify_frontiers_around(position)
        
        # Log événement
        self._log_event("position_updated", position, {
            "step": self.current_step,
            "reward": reward,
            "action": action,
            "visit_count": self.position_visits[position],
            "is_new": is_new,
            "total_positions": len(self.visited_positions)
        })
    
    def register_transition(
        self,
        from_pos: Tuple[int, int],
        to_pos: Tuple[int, int],
        action: str,
        reward: float
    ) -> None:
        """
        Enregistre transition dans graphe topologique
        
        Args:
            from_pos: Position départ
            to_pos: Position arrivée
            action: Action exécutée
            reward: Reward obtenu
        """
        key = (from_pos, action)
        
        if key not in self.transitions:
            self.transitions[key] = Transition(
                from_pos=from_pos,
                to_pos=to_pos,
                action=action
            )
        
        trans = self.transitions[key]
        trans.count += 1
        trans.total_reward += reward
        trans.avg_reward = trans.total_reward / trans.count
        trans.cognitive_cost += 1.0  # Coût unitaire par transition
        
        # Mettre à jour graphe
        if to_pos not in self.transition_graph[from_pos]:
            self.transition_graph[from_pos].append(to_pos)
        
        self._log_event("transition_registered", from_pos, {
            "from": from_pos,
            "to": to_pos,
            "action": action,
            "count": trans.count,
            "avg_reward": trans.avg_reward
        })
    
    def mark_sterile_region(
        self,
        position: Tuple[int, int],
        reason: str = "low_reward"
    ) -> None:
        """
        Marque région comme stérile
        
        Args:
            position: Position centrale
            reason: Raison stérilité
        """
        self.sterile_positions.add(position)
        
        # Mettre à jour région si existe
        region_id = self.position_to_region.get(position)
        if region_id and region_id in self.regions:
            region = self.regions[region_id]
            region.is_sterile = True
            region.sterility_score = 1.0
        
        self._log_event("region_marked_sterile", position, {
            "position": position,
            "reason": reason,
            "total_sterile": len(self.sterile_positions)
        })
    
    def identify_frontiers(self) -> List[Tuple[int, int]]:
        """
        Identifie frontières non explorées
        
        Returns:
            Liste positions frontière
        """
        frontiers = []
        
        for pos in self.visited_positions:
            # Vérifier voisins
            neighbors = self._get_neighbors(pos)
            for neighbor in neighbors:
                if neighbor not in self.visited_positions:
                    if neighbor not in self.frontier_positions:
                        self.frontier_positions.add(neighbor)
                        frontiers.append(neighbor)
        
        if frontiers:
            self._log_event("frontiers_identified", None, {
                "count": len(frontiers),
                "total_frontiers": len(self.frontier_positions)
            })
        
        return frontiers
    
    def compute_region_value(
        self,
        position: Tuple[int, int]
    ) -> float:
        """
        Calcule valeur d'une région
        
        Args:
            position: Position à évaluer
            
        Returns:
            Score valeur région
        """
        # Composantes
        novelty = 1.0 / (1.0 + self.position_visits.get(position, 0))
        
        rewards = self.position_rewards.get(position, [0.0])
        reward_density = np.mean(rewards) if rewards else 0.0
        
        is_frontier = position in self.frontier_positions
        frontier_potential = 1.0 if is_frontier else 0.0
        
        is_sterile = position in self.sterile_positions
        sterility = 1.0 if is_sterile else 0.0
        
        revisit_penalty = min(1.0, self.position_visits.get(position, 0) / 10.0)
        
        # Formule pondérée
        value = (
            0.3 * novelty +
            0.3 * reward_density +
            0.2 * frontier_potential -
            0.1 * sterility -
            0.1 * revisit_penalty
        )
        
        return float(max(0.0, value))
    
    def get_exploration_statistics(self) -> Dict:
        """
        Retourne statistiques exploration
        
        Returns:
            Dict avec métriques
        """
        total_visits = sum(self.position_visits.values())
        unique_positions = len(self.visited_positions)
        
        # Entropie exploration
        if total_visits > 0:
            probs = [v / total_visits for v in self.position_visits.values()]
            self.exploration_entropy = -sum(p * np.log(p + 1e-10) for p in probs)
        
        # Coverage
        if unique_positions > 0:
            self.total_coverage = unique_positions / (unique_positions + len(self.frontier_positions) + 1)
        
        return {
            "unique_positions": unique_positions,
            "total_visits": total_visits,
            "avg_visits_per_position": total_visits / unique_positions if unique_positions > 0 else 0,
            "frontier_count": len(self.frontier_positions),
            "sterile_count": len(self.sterile_positions),
            "high_value_count": len(self.high_value_positions),
            "regions_count": len(self.regions),
            "exploration_entropy": self.exploration_entropy,
            "coverage": self.total_coverage,
            "current_step": self.current_step
        }
    
    def _update_region_membership(
        self,
        position: Tuple[int, int],
        reward: float
    ) -> None:
        """Met à jour appartenance région"""
        # Chercher région proche
        region_id = None
        min_dist = float('inf')
        
        for rid, region in self.regions.items():
            dist = self._manhattan_distance(position, region.center)
            if dist < 5 and dist < min_dist:  # Seuil proximité
                region_id = rid
                min_dist = dist
        
        # Créer nouvelle région si nécessaire
        if region_id is None:
            region_id = f"region_{len(self.regions)}"
            self.regions[region_id] = WorldRegion(
                region_id=region_id,
                center=position
            )
            self._log_event("region_discovered", position, {
                "region_id": region_id,
                "center": position
            })
        
        # Mettre à jour région
        region = self.regions[region_id]
        region.positions.add(position)
        region.visit_count += 1
        region.total_reward += reward
        region.last_visit_step = self.current_step
        region.novelty = 1.0 / (1.0 + region.visit_count)
        
        self.position_to_region[position] = region_id
    
    def _identify_frontiers_around(
        self,
        position: Tuple[int, int]
    ) -> None:
        """Identifie frontières autour position"""
        neighbors = self._get_neighbors(position)
        for neighbor in neighbors:
            if neighbor not in self.visited_positions:
                self.frontier_positions.add(neighbor)
    
    def _get_neighbors(
        self,
        position: Tuple[int, int],
        distance: int = 1
    ) -> List[Tuple[int, int]]:
        """Retourne voisins d'une position"""
        x, y = position
        neighbors = []
        for dx in range(-distance, distance + 1):
            for dy in range(-distance, distance + 1):
                if dx == 0 and dy == 0:
                    continue
                neighbors.append((x + dx, y + dy))
        return neighbors
    
    def _manhattan_distance(
        self,
        pos1: Tuple[int, int],
        pos2: Tuple[int, int]
    ) -> int:
        """Distance Manhattan entre positions"""
        return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1])
    
    def _log_event(
        self,
        event_type: str,
        position: Optional[Tuple[int, int]],
        data: Dict
    ) -> None:
        """Log événement forensique"""
        event = {
            "timestamp_ns": time.perf_counter_ns(),
            "event_type": f"world_model.{event_type}",
            "position": position,
            "data": data
        }
        
        self.events.append(event)
        
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type=event["event_type"],
                component="persistent_world_model",
                data={"position": position, **data}
            )
    
    def reset(self) -> None:
        """Reset complet du modèle"""
        self.visited_positions.clear()
        self.position_visits.clear()
        self.position_rewards.clear()
        self.regions.clear()
        self.position_to_region.clear()
        self.transitions.clear()
        self.transition_graph.clear()
        self.position_history.clear()
        self.trajectory_history.clear()
        self.current_trajectory.clear()
        self.exploration_heatmap.clear()
        self.frontier_positions.clear()
        self.sterile_positions.clear()
        self.high_value_positions.clear()
        self.current_step = 0
        self.events.clear()
        
        self._log_event("world_model_reset", None, {
            "reason": "manual_reset"
        })

# Made with Bob
