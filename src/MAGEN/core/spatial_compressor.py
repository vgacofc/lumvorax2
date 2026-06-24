"""
MAGEN V32 - Module C2: SpatialCompressor
Spatial Cognitive System - Compression Cognitive des Trajectoires

Mission: Transformer expérience brute en structures cognitives
"""

import time
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field
from collections import Counter
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class CompressedPattern:
    """Pattern spatial compressé."""
    pattern_id: str
    pattern_type: str  # linear_probe, loop, zigzag, cluster_exploration, dead_end_probe
    positions: List[Tuple[int, int]]
    actions: List[str]
    rewards: List[float]
    
    # Caractéristiques géométriques
    length: int
    orientation: Optional[str] = None  # horizontal, vertical, diagonal, mixed
    termination_type: Optional[str] = None  # wall, loop, goal, frontier
    
    # Caractéristiques cognitives
    cognitive_label: str = "unknown"  # sterile_probe, productive_exploration, goal_approach
    information_gained: float = 0.0
    strategic_value: float = 0.0
    
    # Métadonnées
    start_step: int = 0
    end_step: int = 0
    revisits: int = 0
    
    def to_dict(self) -> Dict:
        """Sérialisation pour forensic."""
        return {
            'pattern_id': self.pattern_id,
            'type': self.pattern_type,
            'length': self.length,
            'orientation': self.orientation,
            'termination': self.termination_type,
            'cognitive_label': self.cognitive_label,
            'information_gained': self.information_gained,
            'strategic_value': self.strategic_value,
            'start_step': self.start_step,
            'end_step': self.end_step,
            'revisits': self.revisits,
            'avg_reward': sum(self.rewards) / len(self.rewards) if self.rewards else 0.0,
            'positions_count': len(self.positions)
        }


class SpatialCompressor:
    """
    Compresseur Spatial Cognitif.
    
    Transforme séquences de positions/actions en patterns cognitifs.
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Patterns compressés
        self.compressed_patterns: List[CompressedPattern] = []
        self.pattern_counter: int = 0
        
        # Statistiques
        self.total_compressions: int = 0
        self.compression_ratio: float = 0.0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/spatial_compressor.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "SpatialCompressor")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "SpatialCompressor",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "spatial_compressor.initialized",
                "spatial_compressor",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_patterns': len(self.compressed_patterns),
            'pattern_counter': self.pattern_counter,
            'total_compressions': self.total_compressions,
            'compression_ratio': self.compression_ratio,
            'pattern_diversity': len(set(p.pattern_type for p in self.compressed_patterns)),
            'avg_strategic_value': sum(p.strategic_value for p in self.compressed_patterns) / len(self.compressed_patterns) if self.compressed_patterns else 0.0
        }
    
    def compress_trajectory(self,
                           positions: List[Tuple[int, int]],
                           actions: List[str],
                           rewards: List[float],
                           start_step: int) -> Optional[CompressedPattern]:
        """
        Compresser une trajectoire en pattern cognitif.
        
        Args:
            positions: Séquence de positions
            actions: Séquence d'actions
            rewards: Séquence de rewards
            start_step: Step de début
            
        Returns:
            Pattern compressé ou None si pas de pattern détecté
        """
        start_ns = time.time_ns()
        
        # Log début compression
        self.forensic.forensic_logger.log(
            "compression_start",
            "SpatialCompressor",
            {
                "positions_count": len(positions),
                "actions_count": len(actions),
                "rewards_count": len(rewards),
                "start_step": start_step,
                "state": self._forensic_state
            }
        )
        
        if len(positions) < 3:
            end_ns = time.time_ns()
            self.forensic.forensic_logger.log(
                "compression_rejected",
                "SpatialCompressor",
                {
                    "reason": "trajectory_too_short",
                    "positions_count": len(positions),
                    "execution_time_ns": end_ns - start_ns
                }
            )
            return None
        
        # 1. Identifier le type de pattern
        pattern_type = self._identify_pattern_type(positions, actions)
        
        # 2. Analyser géométrie
        orientation = self._analyze_orientation(positions)
        termination = self._analyze_termination(positions, actions, rewards)
        
        # 3. Calculer caractéristiques cognitives
        cognitive_label = self._assign_cognitive_label(
            pattern_type, rewards, termination
        )
        information_gained = self._calculate_information_gain(
            positions, rewards
        )
        strategic_value = self._calculate_strategic_value(
            pattern_type, rewards, information_gained
        )
        
        # 4. Détecter revisites
        revisits = len(positions) - len(set(positions))
        
        # 5. Créer pattern compressé
        pattern_id = f"pattern_{self.pattern_counter}"
        self.pattern_counter += 1
        
        pattern = CompressedPattern(
            pattern_id=pattern_id,
            pattern_type=pattern_type,
            positions=positions,
            actions=actions,
            rewards=rewards,
            length=len(positions),
            orientation=orientation,
            termination_type=termination,
            cognitive_label=cognitive_label,
            information_gained=information_gained,
            strategic_value=strategic_value,
            start_step=start_step,
            end_step=start_step + len(positions) - 1,
            revisits=revisits
        )
        
        self.compressed_patterns.append(pattern)
        self.total_compressions += 1
        
        # 6. Calculer ratio de compression
        self._update_compression_ratio(pattern)
        
        # 7. Forensic logging standard
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "compression_complete",
            "SpatialCompressor",
            {
                "pattern": pattern.to_dict(),
                "execution_time_ns": end_ns - start_ns,
                "success": True,
                "state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "spatial_compressor.pattern_compressed",
                "spatial_compressor",
                pattern.to_dict()
            )
        
        return pattern
    
    def _identify_pattern_type(self, 
                               positions: List[Tuple[int, int]],
                               actions: List[str]) -> str:
        """Identifier le type de pattern géométrique."""
        # Linear probe: mouvement dans une direction
        if self._is_linear(positions):
            return "linear_probe"
        
        # Loop: retour au point de départ
        if self._is_loop(positions):
            return "loop"
        
        # Zigzag: alternance de directions
        if self._is_zigzag(actions):
            return "zigzag"
        
        # Cluster exploration: exploration locale
        if self._is_cluster_exploration(positions):
            return "cluster_exploration"
        
        # Dead-end probe: aller-retour
        if self._is_dead_end_probe(positions):
            return "dead_end_probe"
        
        return "mixed"
    
    def _is_linear(self, positions: List[Tuple[int, int]]) -> bool:
        """Vérifier si trajectoire est linéaire."""
        if len(positions) < 3:
            return False
        
        xs = [p[0] for p in positions]
        ys = [p[1] for p in positions]
        
        # Linéaire si une coordonnée constante
        x_constant = len(set(xs)) == 1
        y_constant = len(set(ys)) == 1
        
        # Ou si mouvement monotone dans une direction
        x_monotone = all(xs[i] <= xs[i+1] for i in range(len(xs)-1)) or \
                     all(xs[i] >= xs[i+1] for i in range(len(xs)-1))
        y_monotone = all(ys[i] <= ys[i+1] for i in range(len(ys)-1)) or \
                     all(ys[i] >= ys[i+1] for i in range(len(ys)-1))
        
        return (x_constant or y_constant) or (x_monotone and y_constant) or (y_monotone and x_constant)
    
    def _is_loop(self, positions: List[Tuple[int, int]]) -> bool:
        """Vérifier si trajectoire forme une boucle."""
        if len(positions) < 4:
            return False
        
        # Boucle si retour au point de départ ou proche
        start = positions[0]
        end = positions[-1]
        
        distance = abs(start[0] - end[0]) + abs(start[1] - end[1])
        return distance <= 1
    
    def _is_zigzag(self, actions: List[str]) -> bool:
        """Vérifier si pattern zigzag (alternance de directions)."""
        if len(actions) < 4:
            return False
        
        # Compter alternances
        alternances = 0
        for i in range(len(actions) - 1):
            if actions[i] != actions[i+1]:
                alternances += 1
        
        # Zigzag si beaucoup d'alternances
        return alternances > len(actions) * 0.6
    
    def _is_cluster_exploration(self, positions: List[Tuple[int, int]]) -> bool:
        """Vérifier si exploration locale d'un cluster."""
        if len(positions) < 5:
            return False
        
        # Calculer bounding box
        xs = [p[0] for p in positions]
        ys = [p[1] for p in positions]
        
        x_range = max(xs) - min(xs)
        y_range = max(ys) - min(ys)
        
        # Cluster si zone compacte
        return x_range <= 3 and y_range <= 3
    
    def _is_dead_end_probe(self, positions: List[Tuple[int, int]]) -> bool:
        """Vérifier si aller-retour (dead-end probe)."""
        if len(positions) < 4:
            return False
        
        # Dead-end si première moitié linéaire et retour
        mid = len(positions) // 2
        first_half = positions[:mid]
        second_half = positions[mid:]
        
        if not self._is_linear(first_half):
            return False
        
        # Vérifier si retour approximatif
        start = positions[0]
        end = positions[-1]
        distance = abs(start[0] - end[0]) + abs(start[1] - end[1])
        
        return distance < len(positions) * 0.3
    
    def _analyze_orientation(self, positions: List[Tuple[int, int]]) -> str:
        """Analyser l'orientation du mouvement."""
        if len(positions) < 2:
            return "none"
        
        xs = [p[0] for p in positions]
        ys = [p[1] for p in positions]
        
        x_range = max(xs) - min(xs)
        y_range = max(ys) - min(ys)
        
        if x_range > y_range * 2:
            return "horizontal"
        elif y_range > x_range * 2:
            return "vertical"
        elif x_range > 0 and y_range > 0:
            return "diagonal"
        
        return "none"
    
    def _analyze_termination(self, 
                            positions: List[Tuple[int, int]],
                            actions: List[str],
                            rewards: List[float]) -> str:
        """Analyser comment la trajectoire se termine."""
        if len(positions) < 2:
            return "unknown"
        
        # Vérifier si dernière action a échoué (position identique)
        if len(positions) >= 2 and positions[-1] == positions[-2]:
            return "wall"
        
        # Vérifier si boucle
        if self._is_loop(positions):
            return "loop"
        
        # Vérifier si reward positif à la fin
        if rewards and rewards[-1] > 0:
            return "goal"
        
        # Sinon, probablement frontière
        return "frontier"
    
    def _assign_cognitive_label(self, 
                                pattern_type: str,
                                rewards: List[float],
                                termination: str) -> str:
        """Assigner un label cognitif au pattern."""
        avg_reward = sum(rewards) / len(rewards) if rewards else 0.0
        
        # Sterile probe: pas de reward, terminaison mur
        if avg_reward <= 0 and termination == "wall":
            return "sterile_probe"
        
        # Goal approach: reward positif
        if avg_reward > 0:
            return "goal_approach"
        
        # Loop: exploration circulaire
        if pattern_type == "loop":
            return "circular_exploration"
        
        # Productive exploration: découverte sans reward immédiat
        if termination == "frontier":
            return "productive_exploration"
        
        return "neutral_exploration"
    
    def _calculate_information_gain(self, 
                                    positions: List[Tuple[int, int]],
                                    rewards: List[float]) -> float:
        """Calculer le gain d'information de la trajectoire."""
        # Gain basé sur:
        # 1. Nouvelles positions découvertes
        unique_positions = len(set(positions))
        novelty_score = unique_positions / len(positions) if positions else 0.0
        
        # 2. Variance des rewards (information sur l'environnement)
        if len(rewards) > 1:
            mean_reward = sum(rewards) / len(rewards)
            variance = sum((r - mean_reward) ** 2 for r in rewards) / len(rewards)
            reward_info = min(1.0, variance)
        else:
            reward_info = 0.0
        
        # Combinaison
        information_gain = 0.7 * novelty_score + 0.3 * reward_info
        
        return information_gain
    
    def _calculate_strategic_value(self, 
                                   pattern_type: str,
                                   rewards: List[float],
                                   information_gained: float) -> float:
        """Calculer la valeur stratégique du pattern."""
        avg_reward = sum(rewards) / len(rewards) if rewards else 0.0
        
        # Valeur basée sur:
        # 1. Reward moyen (40%)
        reward_component = max(0.0, min(1.0, (avg_reward + 1.0) / 2.0))
        
        # 2. Information gagnée (40%)
        info_component = information_gained
        
        # 3. Type de pattern (20%)
        type_values = {
            "goal_approach": 1.0,
            "productive_exploration": 0.7,
            "cluster_exploration": 0.5,
            "linear_probe": 0.4,
            "dead_end_probe": 0.2,
            "sterile_probe": 0.0,
            "loop": 0.1
        }
        type_component = type_values.get(pattern_type, 0.3)
        
        strategic_value = (
            0.4 * reward_component +
            0.4 * info_component +
            0.2 * type_component
        )
        
        return strategic_value
    
    def _update_compression_ratio(self, pattern: CompressedPattern) -> None:
        """Mettre à jour le ratio de compression."""
        # Ratio = taille compressée / taille originale
        # Taille originale = nombre de positions
        # Taille compressée = nombre de champs dans le pattern
        
        original_size = len(pattern.positions)
        compressed_size = 10  # Nombre approximatif de champs significatifs
        
        if original_size > 0:
            ratio = compressed_size / original_size
            
            # Moyenne mobile
            if self.compression_ratio == 0.0:
                self.compression_ratio = ratio
            else:
                self.compression_ratio = 0.9 * self.compression_ratio + 0.1 * ratio
    
    def get_recent_patterns(self, n: int = 10) -> List[CompressedPattern]:
        """Obtenir les n patterns les plus récents."""
        return self.compressed_patterns[-n:]
    
    def get_patterns_by_type(self, pattern_type: str) -> List[CompressedPattern]:
        """Obtenir tous les patterns d'un type donné."""
        return [p for p in self.compressed_patterns if p.pattern_type == pattern_type]
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques de compression."""
        if not self.compressed_patterns:
            return {
                "total_compressions": 0,
                "compression_ratio": 0.0,
                "pattern_types": {}
            }
        
        # Compter types de patterns
        type_counts = Counter(p.pattern_type for p in self.compressed_patterns)
        
        # Calculer valeur stratégique moyenne par type
        type_values = {}
        for ptype in type_counts:
            patterns = self.get_patterns_by_type(ptype)
            avg_value = sum(p.strategic_value for p in patterns) / len(patterns)
            type_values[ptype] = avg_value
        
        return {
            "total_compressions": self.total_compressions,
            "compression_ratio": self.compression_ratio,
            "pattern_types": dict(type_counts),
            "avg_strategic_values": type_values,
            "avg_information_gain": sum(p.information_gained for p in self.compressed_patterns) / len(self.compressed_patterns),
            "most_common_pattern": type_counts.most_common(1)[0][0] if type_counts else None
        }
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter patterns compressés en JSON."""
        data = {
            "statistics": self.get_statistics(),
            "patterns": [p.to_dict() for p in self.compressed_patterns]
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "SpatialCompressor",
            {
                "final_state": self._forensic_state,
                "total_patterns": len(self.compressed_patterns),
                "total_compressions": self.total_compressions,
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()

# Made with Bob
