"""
Progressive Reward Shaper - MAGEN V27.1
========================================

Principe:
    Reward dense multi-couches au lieu de binaire
    
    Reward = découverte + compression + causalité +
             nouveauté_utile + cohérence + réduction_incertitude

Objectif:
    Fournir signal d'apprentissage exploitable à C17
    Différencier actions utiles vs inutiles

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass
from collections import defaultdict
import time
import json


@dataclass
class RewardBreakdown:
    """Décomposition détaillée du reward"""
    exploration_reward: float = 0.0  # Nouvelle zone visitée
    novelty_reward: float = 0.0  # État jamais vu
    topology_reward: float = 0.0  # Chemin vers goal plus court
    causality_reward: float = 0.0  # Action modifie environnement
    progress_reward: float = 0.0  # Distance à goal réduite
    completion_reward: float = 0.0  # Niveau complété
    total_reward: float = 0.0


class ProgressiveRewardShaper:
    """
    Reward shaper dense multi-critères.
    
    Capacités:
    1. Reward exploration (nouvelles zones)
    2. Reward nouveauté (états inédits)
    3. Reward topologique (progression spatiale)
    4. Reward causal (modifications environnement)
    5. Reward progression (distance objectif)
    6. Reward completion (succès final)
    """
    
    def __init__(
        self,
        exploration_weight: float = 0.05,
        novelty_weight: float = 0.10,
        topology_weight: float = 0.20,
        causality_weight: float = 0.30,
        progress_weight: float = 0.50,
        completion_weight: float = 10.0,
        novelty_decay: float = 0.95
    ):
        """
        Args:
            exploration_weight: Poids reward exploration
            novelty_weight: Poids reward nouveauté
            topology_weight: Poids reward topologique
            causality_weight: Poids reward causal
            progress_weight: Poids reward progression
            completion_weight: Poids reward completion
            novelty_decay: Décroissance nouveauté (0-1)
        """
        self.exploration_weight = exploration_weight
        self.novelty_weight = novelty_weight
        self.topology_weight = topology_weight
        self.causality_weight = causality_weight
        self.progress_weight = progress_weight
        self.completion_weight = completion_weight
        self.novelty_decay = novelty_decay
        
        # État interne
        self.visited_positions: Set[Tuple[int, int]] = set()
        self.seen_states: Set[int] = set()  # Hash états
        self.state_visit_counts: Dict[int, int] = defaultdict(int)
        
        # Historique
        self.distance_history: List[float] = []
        self.reward_history: List[RewardBreakdown] = []
        
        # Statistiques
        self.total_rewards_computed = 0
        self.max_reward_seen = 0.0
        self.min_reward_seen = 0.0
        
        # Forensic bit-level nanoseconde
        self.forensic_enabled = True
        self.forensic_log_path: Optional[str] = None
    
    def compute_reward(
        self,
        grid_before: np.ndarray,
        grid_after: np.ndarray,
        action: str,
        agent_pos_before: Optional[Tuple[int, int]],
        agent_pos_after: Optional[Tuple[int, int]],
        goal_pos: Optional[Tuple[int, int]],
        state_name: str
    ) -> RewardBreakdown:
        """
        Calcule reward dense multi-couches.
        
        Args:
            grid_before: Grille avant action
            grid_after: Grille après action
            action: Action exécutée
            agent_pos_before: Position agent avant
            agent_pos_after: Position agent après
            goal_pos: Position objectif
            state_name: État jeu ('LEVEL_COMPLETE', 'NOT_FINISHED', etc.)
        
        Returns:
            breakdown: Décomposition complète reward
        """
        timestamp_start = int(time.time_ns())
        breakdown = RewardBreakdown()
        
        # Forensic: Log reward computation start
        if self.forensic_enabled:
            self._log_forensic('reward_computation_start', {
                'timestamp_ns': timestamp_start,
                'action': action,
                'state_name': state_name,
                'agent_pos_before': agent_pos_before,
                'agent_pos_after': agent_pos_after,
                'goal_pos': goal_pos
            })
        
        # 1. Exploration Reward
        if agent_pos_after:
            if agent_pos_after not in self.visited_positions:
                breakdown.exploration_reward = self.exploration_weight
                self.visited_positions.add(agent_pos_after)
        
        # 2. Novelty Reward
        state_hash = self._hash_state(grid_after)
        visit_count = self.state_visit_counts[state_hash]
        
        if visit_count == 0:
            # État complètement nouveau
            breakdown.novelty_reward = self.novelty_weight
        else:
            # Décroissance avec visites
            breakdown.novelty_reward = self.novelty_weight * (self.novelty_decay ** visit_count)
        
        self.state_visit_counts[state_hash] += 1
        self.seen_states.add(state_hash)
        
        # 3. Topology Reward
        if agent_pos_before and agent_pos_after and goal_pos:
            dist_before = self._manhattan_distance(agent_pos_before, goal_pos)
            dist_after = self._manhattan_distance(agent_pos_after, goal_pos)
            
            if dist_after < dist_before:
                # Se rapproche de l'objectif
                improvement = (dist_before - dist_after) / max(dist_before, 1.0)
                breakdown.topology_reward = self.topology_weight * improvement
            
            # Enregistrer distance
            self.distance_history.append(dist_after)
        
        # 4. Causality Reward
        grid_changed = not np.array_equal(grid_before, grid_after)
        
        if grid_changed:
            # Mesurer ampleur changement
            diff = np.abs(grid_after.astype(float) - grid_before.astype(float))
            change_magnitude = np.sum(diff) / (grid_after.size + 1e-6)
            
            # Reward proportionnel au changement
            breakdown.causality_reward = self.causality_weight * min(1.0, change_magnitude)
        
        # 5. Progress Reward
        if agent_pos_after and goal_pos:
            current_dist = self._manhattan_distance(agent_pos_after, goal_pos)
            
            # Reward inversement proportionnel à distance
            if current_dist == 0:
                # Sur l'objectif
                breakdown.progress_reward = self.progress_weight
            else:
                # Plus proche = plus de reward
                max_dist = grid_after.shape[0] + grid_after.shape[1]  # Distance max possible
                normalized_dist = current_dist / max_dist
                breakdown.progress_reward = self.progress_weight * (1.0 - normalized_dist)
        
        # 6. Completion Reward
        if state_name in ['LEVEL_COMPLETE', 'GAME_OVER']:
            if state_name == 'LEVEL_COMPLETE':
                breakdown.completion_reward = self.completion_weight
            # GAME_OVER = 0 reward (déjà implicite)
        
        # Total
        breakdown.total_reward = (
            breakdown.exploration_reward +
            breakdown.novelty_reward +
            breakdown.topology_reward +
            breakdown.causality_reward +
            breakdown.progress_reward +
            breakdown.completion_reward
        )
        
        # Statistiques
        self.total_rewards_computed += 1
        self.max_reward_seen = max(self.max_reward_seen, breakdown.total_reward)
        self.min_reward_seen = min(self.min_reward_seen, breakdown.total_reward)
        
        # Historique
        self.reward_history.append(breakdown)
        if len(self.reward_history) > 1000:
            self.reward_history.pop(0)
        
        # Forensic: Log reward computation complete
        timestamp_end = int(time.time_ns())
        if self.forensic_enabled:
            self._log_forensic('reward_computation_complete', {
                'timestamp_ns': timestamp_end,
                'duration_ns': timestamp_end - timestamp_start,
                'total_reward': breakdown.total_reward,
                'exploration_reward': breakdown.exploration_reward,
                'novelty_reward': breakdown.novelty_reward,
                'topology_reward': breakdown.topology_reward,
                'causality_reward': breakdown.causality_reward,
                'progress_reward': breakdown.progress_reward,
                'completion_reward': breakdown.completion_reward,
                'state_name': state_name
            })
        
        return breakdown
    
    def _hash_state(self, grid: np.ndarray) -> int:
        """Hash rapide d'un état"""
        # Simple hash basé sur somme pondérée
        flat = grid.flatten()
        weights = np.arange(len(flat), dtype=np.int64)
        return int(np.sum(flat * weights) % (2**31))
    
    def _manhattan_distance(self, pos1: Tuple[int, int], pos2: Tuple[int, int]) -> float:
        """Distance Manhattan entre deux positions"""
        return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1])
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques reward"""
        if not self.reward_history:
            return {
                'total_computed': 0,
                'avg_reward': 0.0,
                'max_reward': 0.0,
                'min_reward': 0.0
            }
        
        recent = self.reward_history[-100:]  # 100 derniers
        
        return {
            'total_computed': self.total_rewards_computed,
            'avg_reward': np.mean([r.total_reward for r in recent]),
            'max_reward': self.max_reward_seen,
            'min_reward': self.min_reward_seen,
            'avg_exploration': np.mean([r.exploration_reward for r in recent]),
            'avg_novelty': np.mean([r.novelty_reward for r in recent]),
            'avg_topology': np.mean([r.topology_reward for r in recent]),
            'avg_causality': np.mean([r.causality_reward for r in recent]),
            'avg_progress': np.mean([r.progress_reward for r in recent]),
            'avg_completion': np.mean([r.completion_reward for r in recent]),
            'unique_states_seen': len(self.seen_states),
            'positions_explored': len(self.visited_positions)
        }
    
    def reset(self):
        """Réinitialise pour nouveau jeu"""
        self.visited_positions.clear()
        self.seen_states.clear()
        self.state_visit_counts.clear()
        self.distance_history.clear()
        self.reward_history.clear()
        self.total_rewards_computed = 0
        self.max_reward_seen = 0.0
        self.min_reward_seen = 0.0
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensic bit-level nanoseconde"""
        if not self.forensic_enabled:
            return
        
        event = {
            'timestamp_ns': int(time.time_ns()),
            'component': 'ProgressiveRewardShaper',
            'event_type': event_type,
            'data': data
        }
        
        # Écrire dans fichier si path défini
        if self.forensic_log_path:
            try:
                with open(self.forensic_log_path, 'a') as f:
                    f.write(json.dumps(event) + '\n')
            except Exception as e:
                print(f"[FORENSIC ERROR] {e}")
    
    def set_forensic_log_path(self, path: str):
        """Configure chemin fichier log forensic"""
        self.forensic_log_path = path

# Made with Bob
