"""
World Model Persistent - MAGEN V36
Carte topologique stable + Self-Anchor + Mémoire Stratégique

PROBLÈME IDENTIFIÉ (V35):
- V35 possède causalité fonctionnelle (arrêter/continuer)
- MAIS couche métacognitive "tourne dans le vide"
- CAUSE: Pas de représentation interne persistante du monde

SOLUTION (V36):
- Construire carte mentale stable du monde exploré
- Maintenir identité spatiale ("je suis ici")
- Consolider expérience en modèle topologique
- Séparer "état observé" vs "monde inféré"

ARCHITECTURE:
┌─────────────────────────────────────────┐
│ WORLD MODEL PERSISTENT                  │
├─────────────────────────────────────────┤
│ 1. TopologicalMap (carte stable)       │
│    - Régions découvertes                │
│    - Corridors/passages                 │
│    - Landmarks (points remarquables)    │
│                                         │
│ 2. SelfAnchor (identité spatiale)      │
│    - Position actuelle                  │
│    - Historique positions               │
│    - Trajectoire récente                │
│                                         │
│ 3. StrategicMemory (consolidation)     │
│    - Zones explorées                    │
│    - Zones prometteuses                 │
│    - Zones mortes (dead-ends)           │
│                                         │
│ 4. WorldInference (séparation)         │
│    - État observé (raw)                 │
│    - Monde inféré (modèle)              │
│    - Confiance par région               │
└─────────────────────────────────────────┘

DIFFÉRENCE CRITIQUE:
- V28 WorldModelMinimal: state + action → future_state (prédiction locale)
- V36 WorldModelPersistent: expérience → carte mentale stable (représentation globale)

Date: 2026-06-15
Conformité: LEÇON-73.1 (Forensic Middleware obligatoire)
"""

from typing import Dict, List, Any, Tuple, Optional, Set
import numpy as np
from dataclasses import dataclass, field
from collections import defaultdict
from copy import deepcopy
import json
from datetime import datetime

# Import forensic middleware (LEÇON-73.1)
try:
    from .forensic_middleware import ForensicMiddleware
except ImportError:
    ForensicMiddleware = None


@dataclass
class Region:
    """Région topologique du monde."""
    id: str
    center: Tuple[int, int]
    bounds: Tuple[int, int, int, int]  # (x_min, y_min, x_max, y_max)
    landmarks: List[Tuple[int, int]] = field(default_factory=list)
    visit_count: int = 0
    last_visit_step: int = 0
    properties: Dict[str, Any] = field(default_factory=dict)
    confidence: float = 0.0  # 0.0-1.0


@dataclass
class Corridor:
    """Corridor/passage entre régions."""
    id: str
    start_region: str
    end_region: str
    path: List[Tuple[int, int]] = field(default_factory=list)
    traversal_count: int = 0
    cost: float = 1.0  # Coût de traversée


@dataclass
class Landmark:
    """Point remarquable du monde."""
    id: str
    position: Tuple[int, int]
    type: str  # 'goal', 'obstacle', 'resource', 'danger', 'unknown'
    properties: Dict[str, Any] = field(default_factory=dict)
    discovery_step: int = 0


class TopologicalMap:
    """
    Carte topologique stable du monde exploré.
    
    Principe:
    - Découpe le monde en régions
    - Identifie corridors entre régions
    - Marque landmarks importants
    - Maintient confiance par région
    """
    
    def __init__(self, grid_size: Tuple[int, int] = (64, 64)):
        self.grid_size = grid_size
        self.regions: Dict[str, Region] = {}
        self.corridors: Dict[str, Corridor] = {}
        self.landmarks: Dict[str, Landmark] = {}
        
        # Grille de régions (pour lookup rapide)
        self.region_grid = np.full(grid_size, "", dtype=object)
        
        # Statistiques
        self.stats = {
            'regions_discovered': 0,
            'corridors_found': 0,
            'landmarks_identified': 0,
            'total_coverage': 0.0
        }
    
    def add_region(self, region: Region):
        """Ajoute une région à la carte."""
        self.regions[region.id] = region
        self.stats['regions_discovered'] = len(self.regions)
        
        # Mettre à jour grille
        x_min, y_min, x_max, y_max = region.bounds
        self.region_grid[y_min:y_max+1, x_min:x_max+1] = region.id
    
    def add_corridor(self, corridor: Corridor):
        """Ajoute un corridor entre régions."""
        self.corridors[corridor.id] = corridor
        self.stats['corridors_found'] = len(self.corridors)
    
    def add_landmark(self, landmark: Landmark):
        """Ajoute un landmark."""
        self.landmarks[landmark.id] = landmark
        self.stats['landmarks_identified'] = len(self.landmarks)
    
    def get_region_at(self, pos: Tuple[int, int]) -> Optional[Region]:
        """Retourne la région à une position."""
        x, y = pos
        if 0 <= y < self.grid_size[0] and 0 <= x < self.grid_size[1]:
            region_id = self.region_grid[y, x]
            if region_id and region_id in self.regions:
                return self.regions[region_id]
        return None
    
    def update_coverage(self):
        """Met à jour le taux de couverture."""
        covered = np.sum(self.region_grid != "")
        total = self.grid_size[0] * self.grid_size[1]
        self.stats['total_coverage'] = covered / total if total > 0 else 0.0


class SelfAnchor:
    """
    Identité spatiale stable ("je suis ici").
    
    Principe:
    - Maintient position actuelle
    - Historique positions récentes
    - Trajectoire et direction
    - Conscience de localisation
    """
    
    def __init__(self, max_history: int = 100):
        self.current_position: Optional[Tuple[int, int]] = None
        self.position_history: List[Tuple[int, int, int]] = []  # (x, y, step)
        self.max_history = max_history
        
        # Trajectoire récente (10 dernières positions)
        self.recent_trajectory: List[Tuple[int, int]] = []
        
        # Direction actuelle (vecteur moyen)
        self.current_direction: Optional[Tuple[float, float]] = None
        
        # Statistiques
        self.stats = {
            'total_moves': 0,
            'unique_positions': 0,
            'avg_speed': 0.0
        }
    
    def update_position(self, pos: Tuple[int, int], step: int):
        """Met à jour la position actuelle."""
        self.current_position = pos
        self.position_history.append((pos[0], pos[1], step))
        self.stats['total_moves'] += 1
        
        # Limiter historique
        if len(self.position_history) > self.max_history:
            self.position_history = self.position_history[-self.max_history:]
        
        # Mettre à jour trajectoire récente
        self.recent_trajectory.append(pos)
        if len(self.recent_trajectory) > 10:
            self.recent_trajectory = self.recent_trajectory[-10:]
        
        # Calculer direction
        self._update_direction()
        
        # Calculer positions uniques
        unique_pos = set((x, y) for x, y, _ in self.position_history)
        self.stats['unique_positions'] = len(unique_pos)
    
    def _update_direction(self):
        """Calcule la direction actuelle (vecteur moyen)."""
        if len(self.recent_trajectory) < 2:
            self.current_direction = None
            return
        
        # Calculer vecteurs entre positions consécutives
        vectors = []
        for i in range(1, len(self.recent_trajectory)):
            prev = self.recent_trajectory[i-1]
            curr = self.recent_trajectory[i]
            vectors.append((curr[0] - prev[0], curr[1] - prev[1]))
        
        # Moyenne
        if vectors:
            avg_x = sum(v[0] for v in vectors) / len(vectors)
            avg_y = sum(v[1] for v in vectors) / len(vectors)
            self.current_direction = (avg_x, avg_y)
    
    def get_distance_traveled(self) -> float:
        """Calcule distance totale parcourue."""
        if len(self.position_history) < 2:
            return 0.0
        
        total_dist = 0.0
        for i in range(1, len(self.position_history)):
            prev = self.position_history[i-1]
            curr = self.position_history[i]
            dist = np.sqrt((curr[0] - prev[0])**2 + (curr[1] - prev[1])**2)
            total_dist += dist
        
        return total_dist


class StrategicMemory:
    """
    Mémoire stratégique consolidée.
    
    Principe:
    - Zones explorées (déjà visitées)
    - Zones prometteuses (à explorer)
    - Zones mortes (dead-ends, inutiles)
    - Compression géométrique
    - Loop detection (V36.1)
    """
    
    def __init__(self):
        self.explored_zones: Set[Tuple[int, int]] = set()
        self.promising_zones: Set[Tuple[int, int]] = set()
        self.dead_zones: Set[Tuple[int, int]] = set()
        
        # Métriques par zone (peut contenir float ou str)
        self.zone_metrics: Dict[Tuple[int, int], Dict[str, Any]] = defaultdict(dict)
        
        # Historique positions récentes (pour loop detection)
        self.recent_positions: List[Tuple[int, int]] = []
        self.max_history = 20
        
        # Statistiques
        self.stats = {
            'explored_count': 0,
            'promising_count': 0,
            'dead_count': 0,
            'exploration_ratio': 0.0,
            'loops_detected': 0
        }
    
    def mark_explored(self, pos: Tuple[int, int], metrics: Optional[Dict[str, float]] = None):
        """Marque une zone comme explorée."""
        self.explored_zones.add(pos)
        if metrics:
            self.zone_metrics[pos].update(metrics)
        
        # Ajouter à historique positions (V36.1)
        self.recent_positions.append(pos)
        if len(self.recent_positions) > self.max_history:
            self.recent_positions = self.recent_positions[-self.max_history:]
        
        # Auto-générer zones prometteuses aux frontières (V36.1)
        self._auto_generate_promising_zones(pos)
        
        self._update_stats()
    
    def mark_promising(self, pos: Tuple[int, int], reason: str = ""):
        """Marque une zone comme prometteuse."""
        self.promising_zones.add(pos)
        self.zone_metrics[pos]['promising_reason'] = reason
        self._update_stats()
    
    def mark_dead(self, pos: Tuple[int, int], reason: str = ""):
        """Marque une zone comme morte (dead-end)."""
        self.dead_zones.add(pos)
        self.zone_metrics[pos]['dead_reason'] = reason
        # Retirer des zones prometteuses si présent
        self.promising_zones.discard(pos)
        self._update_stats()
    
    def is_explored(self, pos: Tuple[int, int]) -> bool:
        """Vérifie si une zone est explorée."""
        return pos in self.explored_zones
    
    def is_promising(self, pos: Tuple[int, int]) -> bool:
        """Vérifie si une zone est prometteuse."""
        return pos in self.promising_zones
    
    def is_dead(self, pos: Tuple[int, int]) -> bool:
        """Vérifie si une zone est morte."""
        return pos in self.dead_zones
    
    def get_zone_value(self, pos: Tuple[int, int]) -> float:
        """
        Calcule la valeur stratégique d'une zone.
        
        Returns:
            float: Valeur (-1.0 = dead, 0.0 = neutre, 1.0 = très prometteuse)
        """
        if pos in self.dead_zones:
            return -1.0
        elif pos in self.promising_zones:
            return 1.0
        elif pos in self.explored_zones:
            return 0.0
        else:
            return 0.5  # Inexploré = moyennement intéressant
    
    def detect_loop(self, pattern_size: int = 4) -> bool:
        """
        Détecte si agent en boucle (V36.1).
        
        Args:
            pattern_size: Taille du pattern à détecter
        
        Returns:
            bool: True si boucle détectée
        """
        if len(self.recent_positions) < pattern_size * 2:
            return False
        
        # Vérifier si pattern se répète
        pattern = self.recent_positions[-pattern_size:]
        
        # Chercher pattern dans historique
        for i in range(len(self.recent_positions) - pattern_size * 2, -1, -1):
            if self.recent_positions[i:i+pattern_size] == pattern:
                self.stats['loops_detected'] += 1
                return True
        
        return False
    
    def _auto_generate_promising_zones(self, current_pos: Tuple[int, int]):
        """
        Génère automatiquement zones prometteuses aux frontières (V36.1).
        
        Args:
            current_pos: Position actuelle
        """
        # Positions adjacentes (4-connectivité)
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            new_pos = (current_pos[0] + dx, current_pos[1] + dy)
            
            # Si non explorée et pas déjà prometteuse
            if (not self.is_explored(new_pos) and
                not self.is_promising(new_pos) and
                not self.is_dead(new_pos)):
                self.mark_promising(new_pos, reason="auto_frontier")
    
    def _update_stats(self):
        """Met à jour les statistiques."""
        self.stats['explored_count'] = len(self.explored_zones)
        self.stats['promising_count'] = len(self.promising_zones)
        self.stats['dead_count'] = len(self.dead_zones)
        
        total = len(self.explored_zones) + len(self.promising_zones) + len(self.dead_zones)
        if total > 0:
            self.stats['exploration_ratio'] = len(self.explored_zones) / total


class WorldInference:
    """
    Séparation "état observé" vs "monde inféré".
    
    Principe:
    - État observé = données brutes
    - Monde inféré = modèle construit
    - Confiance par région
    - Prédictions basées sur modèle
    """
    
    def __init__(self):
        self.observed_states: List[Dict[str, Any]] = []
        self.inferred_world: Dict[str, Any] = {
            'structure': {},
            'dynamics': {},
            'constraints': {}
        }
        
        # Confiance par région
        self.confidence_map: Dict[Tuple[int, int], float] = {}
        
        # Statistiques
        self.stats = {
            'observations': 0,
            'inferences_made': 0,
            'avg_confidence': 0.0
        }
    
    def add_observation(self, state: Dict[str, Any], step: int):
        """Ajoute une observation brute."""
        obs = {
            'state': deepcopy(state),
            'step': step,
            'timestamp': datetime.now().isoformat()
        }
        self.observed_states.append(obs)
        self.stats['observations'] += 1
        
        # Limiter historique (garder 200 dernières)
        if len(self.observed_states) > 200:
            self.observed_states = self.observed_states[-200:]
    
    def infer_structure(self) -> Dict[str, Any]:
        """
        Infère la structure du monde à partir des observations.
        
        Returns:
            dict: Structure inférée (obstacles, passages, etc.)
        """
        if not self.observed_states:
            return {}
        
        # Analyser observations pour détecter structure
        structure = {
            'static_obstacles': [],
            'dynamic_entities': [],
            'boundaries': None
        }
        
        # TODO: Implémenter inférence réelle
        # Pour l'instant: placeholder
        
        self.inferred_world['structure'] = structure
        self.stats['inferences_made'] += 1
        
        return structure
    
    def get_confidence(self, pos: Tuple[int, int]) -> float:
        """Retourne la confiance du modèle à une position."""
        return self.confidence_map.get(pos, 0.0)
    
    def update_confidence(self, pos: Tuple[int, int], confidence: float):
        """Met à jour la confiance à une position."""
        self.confidence_map[pos] = max(0.0, min(1.0, confidence))
        
        # Calculer confiance moyenne
        if self.confidence_map:
            self.stats['avg_confidence'] = sum(self.confidence_map.values()) / len(self.confidence_map)


class WorldModelPersistent:
    """
    World Model Persistant V36 - Carte mentale stable du monde.
    
    ARCHITECTURE COMPLÈTE:
    - TopologicalMap: Carte topologique (régions, corridors, landmarks)
    - SelfAnchor: Identité spatiale ("je suis ici")
    - StrategicMemory: Mémoire consolidée (zones explorées/prometteuses/mortes)
    - WorldInference: Séparation observé/inféré + confiance
    
    DIFFÉRENCE AVEC V28 WorldModelMinimal:
    - V28: Prédiction locale (state → future_state)
    - V36: Représentation globale (expérience → carte mentale)
    
    CONFORMITÉ: LEÇON-73.1 (Forensic Middleware)
    """
    
    def __init__(
        self,
        grid_size: Tuple[int, int] = (64, 64),
        forensic_log_path: Optional[str] = None
    ):
        """
        Args:
            grid_size: Taille de la grille (H, W)
            forensic_log_path: Chemin vers fichier de logs forensic
        """
        self.grid_size = grid_size
        
        # Composants principaux
        self.topological_map = TopologicalMap(grid_size)
        self.self_anchor = SelfAnchor()
        self.strategic_memory = StrategicMemory()
        self.world_inference = WorldInference()
        
        # Forensic middleware (LEÇON-73.1)
        self._forensic_state = {
            'module': 'WorldModelPersistent',
            'version': 'V36',
            'initialized': datetime.now().isoformat()
        }
        
        if ForensicMiddleware and forensic_log_path:
            self.forensic = ForensicMiddleware(forensic_log_path, 'WorldModelPersistent')
            self.forensic.log_event('module_initialized', self._forensic_state)
        else:
            self.forensic = None
        
        # Statistiques globales
        self.stats = {
            'total_updates': 0,
            'current_step': 0,
            'world_coherence': 0.0
        }
        
        # Historique cohérence (pour progression - V36.1)
        self.coherence_history: List[float] = []
        self.max_coherence_history = 10
    
    def update(
        self,
        state: Dict[str, Any],
        action: Optional[str] = None,
        step: int = 0
    ):
        """
        Met à jour le modèle du monde avec nouvelle observation.
        
        Args:
            state: État observé
            action: Action exécutée (optionnel)
            step: Numéro de step
        """
        self.stats['total_updates'] += 1
        self.stats['current_step'] = step
        
        # 1. Ajouter observation brute
        self.world_inference.add_observation(state, step)
        
        # 2. Mettre à jour position (self-anchor)
        if 'agent_pos' in state and state['agent_pos'] is not None:
            pos = state['agent_pos']
            self.self_anchor.update_position(pos, step)
            
            # Marquer zone comme explorée
            self.strategic_memory.mark_explored(pos, {'step': step})
            
            # Mettre à jour confiance à cette position
            self.world_inference.update_confidence(pos, 0.8)
        
        # 3. Découvrir/mettre à jour régions
        self._update_regions(state, step)
        
        # 4. Calculer cohérence du monde
        self._update_world_coherence()
        
        # Log forensic
        if self.forensic:
            self.forensic.log_event('world_model_updated', {
                'step': step,
                'position': self.self_anchor.current_position,
                'regions_discovered': len(self.topological_map.regions),
                'explored_zones': len(self.strategic_memory.explored_zones),
                'world_coherence': self.stats['world_coherence']
            })
    
    def _update_regions(self, state: Dict[str, Any], step: int):
        """Découvre et met à jour les régions."""
        if 'agent_pos' not in state or state['agent_pos'] is None:
            return
        
        pos = state['agent_pos']
        
        # Vérifier si position dans région existante
        region = self.topological_map.get_region_at(pos)
        
        if region:
            # Mettre à jour région existante
            region.visit_count += 1
            region.last_visit_step = step
            region.confidence = min(1.0, region.confidence + 0.1)
        else:
            # Créer nouvelle région (simple: 5x5 autour de position)
            region_id = f"region_{len(self.topological_map.regions)}"
            x, y = pos
            bounds = (
                max(0, x - 2),
                max(0, y - 2),
                min(self.grid_size[1] - 1, x + 2),
                min(self.grid_size[0] - 1, y + 2)
            )
            
            new_region = Region(
                id=region_id,
                center=pos,
                bounds=bounds,
                visit_count=1,
                last_visit_step=step,
                confidence=0.3
            )
            
            self.topological_map.add_region(new_region)
            
            # Log forensic
            if self.forensic:
                self.forensic.log_event('region_discovered', {
                    'region_id': region_id,
                    'center': pos,
                    'step': step
                })
        
        # Mettre à jour couverture
        self.topological_map.update_coverage()
    
    def _update_world_coherence(self):
        """
        Calcule la cohérence du modèle du monde.
        
        Cohérence = moyenne de:
        - Couverture topologique
        - Confiance moyenne
        - Ratio exploration
        """
        coverage = self.topological_map.stats['total_coverage']
        confidence = self.world_inference.stats['avg_confidence']
        exploration = self.strategic_memory.stats['exploration_ratio']
        
        self.stats['world_coherence'] = (coverage + confidence + exploration) / 3.0
    
    def get_strategic_value(self, pos: Tuple[int, int]) -> float:
        """
        Calcule la valeur stratégique d'une position.
        
        Combine:
        - Valeur mémoire stratégique
        - Confiance du modèle
        - Proximité régions connues
        
        Returns:
            float: Valeur stratégique (-1.0 à 1.0)
        """
        # Valeur de base (mémoire stratégique)
        base_value = self.strategic_memory.get_zone_value(pos)
        
        # Confiance du modèle
        confidence = self.world_inference.get_confidence(pos)
        
        # Bonus si dans région connue
        region = self.topological_map.get_region_at(pos)
        region_bonus = 0.2 if region and region.confidence > 0.5 else 0.0
        
        # Valeur finale
        strategic_value = base_value * confidence + region_bonus
        
        return max(-1.0, min(1.0, strategic_value))
    
    def get_exploration_frontier(self) -> List[Tuple[int, int]]:
        """
        Retourne les positions à la frontière de l'exploration.
        
        Returns:
            list: Positions prometteuses à explorer
        """
        frontier = []
        
        # Positions prometteuses non explorées
        for pos in self.strategic_memory.promising_zones:
            if not self.strategic_memory.is_explored(pos):
                frontier.append(pos)
        
        # Si pas de zones prometteuses, chercher autour des régions connues
        if not frontier:
            for region in self.topological_map.regions.values():
                x, y = region.center
                # Positions adjacentes
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    new_pos = (x + dx, y + dy)
                    if (0 <= new_pos[0] < self.grid_size[1] and
                        0 <= new_pos[1] < self.grid_size[0] and
                        not self.strategic_memory.is_explored(new_pos)):
                        frontier.append(new_pos)
        
        return frontier[:10]  # Limiter à 10 positions
    
    def get_state(self) -> Dict[str, Any]:
        """Retourne l'état complet du World Model (LEÇON-73.1)."""
        return {
            **self._forensic_state,
            'grid_size': self.grid_size,
            'topological_map': {
                'regions': len(self.topological_map.regions),
                'corridors': len(self.topological_map.corridors),
                'landmarks': len(self.topological_map.landmarks),
                'coverage': self.topological_map.stats['total_coverage']
            },
            'self_anchor': {
                'current_position': self.self_anchor.current_position,
                'total_moves': self.self_anchor.stats['total_moves'],
                'unique_positions': self.self_anchor.stats['unique_positions']
            },
            'strategic_memory': {
                'explored': len(self.strategic_memory.explored_zones),
                'promising': len(self.strategic_memory.promising_zones),
                'dead': len(self.strategic_memory.dead_zones)
            },
            'world_inference': {
                'observations': self.world_inference.stats['observations'],
                'avg_confidence': self.world_inference.stats['avg_confidence']
            },
            'stats': self.stats
        }
    
    def close(self):
        """Ferme le module et log événement final (LEÇON-73.1)."""
        if self.forensic:
            self.forensic.log_event('module_closing', {
                'total_updates': self.stats['total_updates'],
                'final_coherence': self.stats['world_coherence'],
                'regions_discovered': len(self.topological_map.regions)
            })
            self.forensic.close()
    
    def reset(self):
        """Réinitialise le World Model (nouveau puzzle)."""
        self.topological_map = TopologicalMap(self.grid_size)
        self.self_anchor = SelfAnchor()
        self.strategic_memory = StrategicMemory()
        self.world_inference = WorldInference()
        
        self.stats = {
            'total_updates': 0,
            'current_step': 0,
            'world_coherence': 0.0
        }
        
        if self.forensic:
            self.forensic.log_event('world_model_reset', {
                'timestamp': datetime.now().isoformat()
            })


# Made with Bob