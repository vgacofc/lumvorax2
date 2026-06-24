"""
MAGEN V32 - Module C1: MentalMapBuilder
Spatial Cognitive System - Représentation Persistante du Monde

Mission: Construire une carte mentale persistante et exploitable

FORENSIC: Conforme LEÇON-60.1, 60.2, 61.1, 72.2 (Standard LumVorax)
"""

import time
from typing import Dict, List, Tuple, Set, Optional, Any
from dataclasses import dataclass, field
from collections import defaultdict
import json
from pathlib import Path

# Forensic imports (Standard LumVorax)
from .forensic_middleware import ForensicMiddleware


@dataclass
class Region:
    """Représentation cognitive d'une région spatiale."""
    id: str
    positions: Set[Tuple[int, int]] = field(default_factory=set)
    region_type: str = "unknown"  # corridor, loop, cluster, frontier, dead_end
    orientation: Optional[str] = None  # north, south, east, west, none
    termination: Optional[str] = None  # wall, loop, frontier, unknown
    strategic_value: float = 0.5
    confidence: float = 0.0
    visit_count: int = 0
    discovery_step: int = 0
    last_visit_step: int = 0
    rewards_collected: List[float] = field(default_factory=list)
    failed_actions: int = 0
    successful_actions: int = 0
    
    def to_dict(self) -> Dict:
        """Sérialisation pour forensic."""
        return {
            'id': self.id,
            'positions': [list(p) for p in self.positions],
            'type': self.region_type,
            'orientation': self.orientation,
            'termination': self.termination,
            'strategic_value': self.strategic_value,
            'confidence': self.confidence,
            'visit_count': self.visit_count,
            'discovery_step': self.discovery_step,
            'last_visit_step': self.last_visit_step,
            'avg_reward': sum(self.rewards_collected) / len(self.rewards_collected) if self.rewards_collected else 0.0,
            'success_rate': self.successful_actions / (self.successful_actions + self.failed_actions) if (self.successful_actions + self.failed_actions) > 0 else 0.0
        }


class MentalMapBuilder:
    """
    Constructeur de Carte Mentale Persistante.
    
    Transforme l'expérience brute en représentation cognitive structurée.
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Setup Forensic Middleware (Standard LumVorax)
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/mental_map_builder.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(
            forensic_log_path,
            "MentalMapBuilder"
        )
        
        # Spatial Persistence
        self.all_positions: Set[Tuple[int, int]] = set()
        self.position_visits: Dict[Tuple[int, int], int] = defaultdict(int)
        self.position_rewards: Dict[Tuple[int, int], List[float]] = defaultdict(list)
        self.transitions: Dict[Tuple[int, int], Dict[str, Tuple[int, int]]] = defaultdict(dict)
        self.walls: Set[Tuple[int, int]] = set()
        
        # Region Formation
        self.regions: Dict[str, Region] = {}
        self.position_to_region: Dict[Tuple[int, int], str] = {}
        self.region_counter: int = 0
        
        # Trajectory Tracking
        self.trajectory_history: List[Tuple[int, int]] = []
        self.action_history: List[str] = []
        self.reward_history: List[float] = []
        
        # Spatial Compression
        self.compressed_patterns: List[Dict] = []
        
        # Metrics
        self.step: int = 0
        self.total_regions_created: int = 0
        self.total_compressions: int = 0
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization (forensic)
        self.forensic.forensic_logger.log(
            "module_initialized",
            "MentalMapBuilder",
            {
                "timestamp_ns": self.creation_time,
                "forensic_enabled": True,
                "forensic_log_path": forensic_log_path
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "mental_map.initialized",
                "mental_map_builder",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic pour instrumentation."""
        return {
            'step': self.step,
            'total_positions': len(self.all_positions),
            'total_regions': len(self.regions),
            'total_regions_created': self.total_regions_created,
            'total_compressions': self.total_compressions,
            'trajectory_length': len(self.trajectory_history)
        }
    
    def update(self, avatar_pos: Optional[Tuple[int, int]], 
               action: str, 
               reward: float,
               observation: Dict,
               step: int) -> None:
        """
        Mise à jour de la carte mentale avec nouvelle observation.
        
        Args:
            avatar_pos: Position actuelle de l'avatar
            action: Action exécutée
            reward: Reward reçu
            observation: Observation complète
            step: Numéro du step
        """
        self.step = step
        
        if avatar_pos is None:
            return
        
        # 1. Spatial Persistence
        self._update_spatial_persistence(avatar_pos, action, reward)
        
        # 2. Region Formation
        self._update_region_formation(avatar_pos, action, reward)
        
        # 3. Trajectory Tracking
        self._update_trajectory_tracking(avatar_pos, action, reward)
        
        # 4. Spatial Compression (tous les 10 steps)
        if step % 10 == 0:
            self._perform_spatial_compression()
        
        # 5. Forensic Logging
        if self.logger and step % 20 == 0:
            self._log_mental_map_state()
    
    def _update_spatial_persistence(self, pos: Tuple[int, int], 
                                    action: str, 
                                    reward: float) -> None:
        """Mise à jour de la persistance spatiale."""
        # Enregistrer position
        self.all_positions.add(pos)
        self.position_visits[pos] += 1
        self.position_rewards[pos].append(reward)
        
        # Détecter murs (action échoue = mur)
        if len(self.trajectory_history) > 0:
            prev_pos = self.trajectory_history[-1]
            expected_pos = self._simulate_action(prev_pos, action)
            
            if expected_pos != pos:
                # Action a échoué, probablement un mur
                self.walls.add(expected_pos)
                
                # Enregistrer transition échouée
                self.transitions[prev_pos][action] = prev_pos
            else:
                # Action réussie
                self.transitions[prev_pos][action] = pos
    
    def _update_region_formation(self, pos: Tuple[int, int], 
                                 action: str, 
                                 reward: float) -> None:
        """Formation automatique de régions."""
        # Si position déjà dans une région, mettre à jour
        if pos in self.position_to_region:
            region_id = self.position_to_region[pos]
            region = self.regions[region_id]
            region.visit_count += 1
            region.last_visit_step = self.step
            region.rewards_collected.append(reward)
            
            if reward > 0:
                region.successful_actions += 1
            else:
                region.failed_actions += 1
        else:
            # Nouvelle position: créer ou étendre région
            self._assign_to_region(pos, action, reward)
    
    def _assign_to_region(self, pos: Tuple[int, int], 
                         action: str, 
                         reward: float) -> None:
        """Assigner position à une région (nouvelle ou existante)."""
        # Chercher région voisine
        neighbors = self._get_neighbors(pos)
        neighbor_regions = set()
        
        for neighbor in neighbors:
            if neighbor in self.position_to_region:
                neighbor_regions.add(self.position_to_region[neighbor])
        
        if neighbor_regions:
            # Étendre région existante
            region_id = list(neighbor_regions)[0]  # Prendre la première
            region = self.regions[region_id]
            region.positions.add(pos)
            self.position_to_region[pos] = region_id
            
            # Mettre à jour type de région
            self._update_region_type(region)
        else:
            # Créer nouvelle région
            region_id = f"region_{self.region_counter}"
            self.region_counter += 1
            self.total_regions_created += 1
            
            region = Region(
                id=region_id,
                positions={pos},
                discovery_step=self.step,
                last_visit_step=self.step
            )
            region.rewards_collected.append(reward)
            
            self.regions[region_id] = region
            self.position_to_region[pos] = region_id
            
            if self.logger:
                self.logger.log_event(
                    "mental_map.region_created",
                    "mental_map_builder",
                    {
                        "region_id": region_id,
                        "position": list(pos),
                        "step": self.step
                    }
                )
    
    def _update_region_type(self, region: Region) -> None:
        """Mettre à jour le type de région basé sur sa structure."""
        positions = list(region.positions)
        
        if len(positions) < 2:
            region.region_type = "single"
            return
        
        # Analyser la géométrie
        xs = [p[0] for p in positions]
        ys = [p[1] for p in positions]
        
        x_range = max(xs) - min(xs)
        y_range = max(ys) - min(ys)
        
        # Corridor: étendu dans une direction
        if x_range > y_range * 2:
            region.region_type = "corridor"
            region.orientation = "horizontal"
        elif y_range > x_range * 2:
            region.region_type = "corridor"
            region.orientation = "vertical"
        # Cluster: étendu dans les deux directions
        elif len(positions) > 5:
            region.region_type = "cluster"
        else:
            region.region_type = "area"
        
        # Détecter termination
        if region.failed_actions > region.successful_actions:
            region.termination = "wall"
        elif self._is_loop_region(region):
            region.termination = "loop"
        
        # Calculer confiance
        region.confidence = min(1.0, region.visit_count / 10.0)
    
    def _is_loop_region(self, region: Region) -> bool:
        """Détecter si région forme une boucle."""
        # Simplification: si beaucoup de revisites
        return region.visit_count > len(region.positions) * 2
    
    def _update_trajectory_tracking(self, pos: Tuple[int, int], 
                                   action: str, 
                                   reward: float) -> None:
        """Mise à jour de l'historique de trajectoire."""
        self.trajectory_history.append(pos)
        self.action_history.append(action)
        self.reward_history.append(reward)
        
        # Garder seulement les 100 derniers
        if len(self.trajectory_history) > 100:
            self.trajectory_history = self.trajectory_history[-100:]
            self.action_history = self.action_history[-100:]
            self.reward_history = self.reward_history[-100:]
    
    def _perform_spatial_compression(self) -> None:
        """Compression spatiale: identifier patterns dans trajectoire."""
        if len(self.trajectory_history) < 5:
            return
        
        # Analyser les 20 dernières positions
        recent = self.trajectory_history[-20:]
        
        # Détecter pattern linéaire
        if self._is_linear_trajectory(recent):
            pattern = {
                "type": "linear_probe",
                "length": len(recent),
                "start": recent[0],
                "end": recent[-1],
                "step": self.step
            }
            self.compressed_patterns.append(pattern)
            self.total_compressions += 1
            
            if self.logger:
                self.logger.log_event(
                    "mental_map.pattern_compressed",
                    "mental_map_builder",
                    {
                        "pattern": pattern,
                        "step": self.step
                    }
                )
    
    def _is_linear_trajectory(self, positions: List[Tuple[int, int]]) -> bool:
        """Vérifier si trajectoire est linéaire."""
        if len(positions) < 3:
            return False
        
        # Vérifier si mouvement dans une seule direction
        xs = [p[0] for p in positions]
        ys = [p[1] for p in positions]
        
        x_constant = len(set(xs)) == 1
        y_constant = len(set(ys)) == 1
        
        return x_constant or y_constant
    
    def _get_neighbors(self, pos: Tuple[int, int]) -> List[Tuple[int, int]]:
        """Obtenir positions voisines (4-connectivité)."""
        x, y = pos
        return [
            (x-1, y),
            (x+1, y),
            (x, y-1),
            (x, y+1)
        ]
    
    def _simulate_action(self, pos: Tuple[int, int], action: str) -> Tuple[int, int]:
        """Simuler résultat d'une action."""
        x, y = pos
        if action == 'up':
            return (x, y-1)
        elif action == 'down':
            return (x, y+1)
        elif action == 'left':
            return (x-1, y)
        elif action == 'right':
            return (x+1, y)
        else:
            return pos
    
    def get_regions(self) -> Dict[str, Region]:
        """Obtenir toutes les régions."""
        return self.regions
    
    def get_region_at(self, pos: Tuple[int, int]) -> Optional[Region]:
        """Obtenir région à une position."""
        if pos in self.position_to_region:
            region_id = self.position_to_region[pos]
            return self.regions[region_id]
        return None
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_mental_map_state()
    
    def get_mental_map_state(self) -> Dict:
        """Obtenir état complet de la carte mentale."""
        return {
            "timestamp_ns": time.perf_counter_ns(),
            "step": self.step,
            "total_positions": len(self.all_positions),
            "total_regions": len(self.regions),
            "total_walls": len(self.walls),
            "total_compressions": self.total_compressions,
            "regions": {
                rid: region.to_dict()
                for rid, region in self.regions.items()
            },
            "compressed_patterns": self.compressed_patterns[-10:],
            "trajectory_length": len(self.trajectory_history)
        }
    
    def _log_mental_map_state(self) -> None:
        """Logger l'état de la carte mentale."""
        if not self.logger:
            return
        
        state = self.get_mental_map_state()
        
        self.logger.log_event(
            "mental_map.state_snapshot",
            "mental_map_builder",
            state
        )
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter carte mentale en JSON."""
        state = self.get_mental_map_state()
        
        with open(filepath, 'w') as f:
            json.dump(state, f, indent=2)
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques de la carte mentale."""
        return {
            "total_positions_discovered": len(self.all_positions),
            "total_regions_created": self.total_regions_created,
            "current_regions": len(self.regions),
            "total_walls_detected": len(self.walls),
            "total_compressions": self.total_compressions,
            "avg_region_size": sum(len(r.positions) for r in self.regions.values()) / len(self.regions) if self.regions else 0,
            "most_visited_region": max(
                self.regions.values(),
                key=lambda r: r.visit_count
            ).id if self.regions else None
        }
    
    def close(self) -> None:
        """
        Fermer proprement le module et le forensic logger.
        
        FORENSIC: Conforme LEÇON-60.1, 60.2, 61.1, 72.2
        """
        # Log fermeture (forensic)
        self.forensic.forensic_logger.log(
            "module_closing",
            "MentalMapBuilder",
            {
                "step": self.step,
                "total_positions": len(self.all_positions),
                "total_regions": len(self.regions),
                "total_regions_created": self.total_regions_created,
                "total_compressions": self.total_compressions,
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        
        # Fermer forensic logger
        self.forensic.forensic_logger.close()


# Made with Bob - FORENSIC INTEGRATED (LEÇON-60.1, 60.2, 61.1, 72.2)
