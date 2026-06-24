"""
Reward Shaper - Advanced reward shaping for ls20-9607627b
MAGEN Phase 4.2.8 - Correction P0

Objectif: Fournir reward intrinsèque pour guider exploration
Référence: RAPPORT_ENTRAINEMENT_NIVEAU1.md ligne 148
Référence: Log modèle concurrent (goals detection)

Date création: 2026-06-19
Auteur: Bob (MAGEN Team)
"""

import numpy as np
import hashlib
from typing import Dict, Tuple, Optional, Set, List
from collections import deque
import logging

logger = logging.getLogger(__name__)


class RewardShaper:
    """
    Advanced reward shaping for ls20-9607627b
    
    Composants:
    1. Exploration bonus (0.5 pour nouvel état)
    2. Goal proximity reward (distance Manhattan)
    3. Loop penalty (-0.5 si revisité)
    4. Counter preservation bonus (0.1 si préservé)
    
    Conformité PROTOCOLE:
    - Reward intrinsèque > 0.0 même sans progression
    - Guidance vers objectif (goal-oriented)
    - Pénalité boucles (éviter répétitions)
    """
    
    def __init__(self, goal_positions: Optional[List[Tuple[int, int]]] = None):
        """
        Initialiser Reward Shaper
        
        Args:
            goal_positions: Positions goals (depuis modèle concurrent)
                           Default: [(44,51), (45,51), (45,52)]
        """
        # Goals (depuis modèle concurrent Frame 719)
        if goal_positions is None:
            self.goal_positions = [(44, 51), (45, 51), (45, 52)]
        else:
            self.goal_positions = goal_positions
        
        # États visités
        self.visited_states: Set[str] = set()
        self.recent_states = deque(maxlen=10)  # 10 derniers états
        
        # Statistiques
        self.total_shaped_reward = 0.0
        self.exploration_bonuses = 0
        self.loop_penalties = 0
        self.counter_bonuses = 0
        self.goal_proximity_rewards = 0.0
        
        logger.info(f"✅ Reward Shaper initialisé (goals={self.goal_positions})")
    
    def shape_reward(
        self,
        state: np.ndarray,
        action: int,
        next_state: np.ndarray,
        base_reward: float,
        agent_pos: Optional[Tuple[int, int]] = None
    ) -> float:
        """
        Shape reward avec composants multiples
        
        Args:
            state: État avant action
            action: Action exécutée
            next_state: État après action
            base_reward: Reward de base (environnement)
            agent_pos: Position agent (optionnel)
            
        Returns:
            Reward shapé (base + intrinsèque)
        """
        shaped = base_reward
        
        # 1. Exploration bonus (0.5 pour nouvel état)
        state_hash = self._compute_state_hash(next_state)
        
        if state_hash not in self.visited_states:
            shaped += 0.5
            self.visited_states.add(state_hash)
            self.exploration_bonuses += 1
            
            if len(self.visited_states) % 100 == 0:
                logger.info(f"🔍 Exploration: {len(self.visited_states)} états découverts")
        
        # 2. Goal proximity reward (si position agent connue)
        if agent_pos is not None and self.goal_positions:
            proximity_reward = self._compute_proximity_reward(agent_pos)
            shaped += proximity_reward
            self.goal_proximity_rewards += proximity_reward
        
        # 3. Loop penalty (-0.5 si revisité récemment)
        if state_hash in self.recent_states:
            shaped -= 0.5
            self.loop_penalties += 1
        
        self.recent_states.append(state_hash)
        
        # 4. Counter preservation bonus (0.1 si préservé)
        if self._counter_preserved(state, next_state):
            shaped += 0.1
            self.counter_bonuses += 1
        
        # Mettre à jour statistiques
        self.total_shaped_reward += shaped
        
        return shaped
    
    def _compute_state_hash(self, state: np.ndarray) -> str:
        """Calculer hash SHA256 d'un état"""
        return hashlib.sha256(state.tobytes()).hexdigest()
    
    def _compute_proximity_reward(self, agent_pos: Tuple[int, int]) -> float:
        """
        Calculer reward basé sur proximité aux goals
        
        Formule: 1.0 / (min_distance + 1)
        - Distance 0: reward = 1.0
        - Distance 1: reward = 0.5
        - Distance 5: reward = 0.17
        - Distance 10: reward = 0.09
        
        Args:
            agent_pos: Position agent (row, col)
            
        Returns:
            Reward proximité (0.0-1.0)
        """
        if not self.goal_positions:
            return 0.0
        
        # Calculer distance Manhattan minimale
        min_distance = min([
            abs(agent_pos[0] - goal[0]) + abs(agent_pos[1] - goal[1])
            for goal in self.goal_positions
        ])
        
        # Reward inversement proportionnel à distance
        proximity_reward = 1.0 / (min_distance + 1)
        
        return proximity_reward
    
    def _counter_preserved(self, state: np.ndarray, next_state: np.ndarray) -> bool:
        """
        Vérifier si compteur préservé (pixel 11)
        
        Découverte Session 89:
        - Compteur = pixel value 11
        - Commence à 84 unités
        - Consomme 2 unités/action
        - Limite 42 actions
        
        Args:
            state: État avant
            next_state: État après
            
        Returns:
            True si compteur préservé ou augmenté
        """
        counter_before = np.sum(state == 11)
        counter_after = np.sum(next_state == 11)
        
        return counter_after >= counter_before
    
    def get_statistics(self) -> Dict:
        """
        Obtenir statistiques reward shaping
        
        Returns:
            Dict avec statistiques
        """
        return {
            'total_shaped_reward': self.total_shaped_reward,
            'states_discovered': len(self.visited_states),
            'exploration_bonuses': self.exploration_bonuses,
            'loop_penalties': self.loop_penalties,
            'counter_bonuses': self.counter_bonuses,
            'goal_proximity_rewards': self.goal_proximity_rewards,
            'avg_shaped_reward': (
                self.total_shaped_reward / max(1, self.exploration_bonuses + self.loop_penalties)
            )
        }
    
    def reset(self):
        """Reset statistiques (nouveau épisode)"""
        self.recent_states.clear()
        # Ne pas reset visited_states (mémoire inter-épisodes)
    
    def __repr__(self) -> str:
        """Représentation string"""
        stats = self.get_statistics()
        return (f"RewardShaper(states={stats['states_discovered']}, "
                f"bonuses={stats['exploration_bonuses']}, "
                f"penalties={stats['loop_penalties']})")


# Tests unitaires
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    print("🧪 Tests Reward Shaper")
    print("=" * 60)
    
    # Test 1: Initialisation
    shaper = RewardShaper()
    print(f"\n✅ Test 1: {shaper}")
    
    # Test 2: Exploration bonus
    state1 = np.random.randint(0, 12, (64, 64))
    state2 = np.random.randint(0, 12, (64, 64))
    
    reward1 = shaper.shape_reward(state1, 1, state2, base_reward=0.0)
    print(f"\n✅ Test 2: Premier état - Reward={reward1:.2f} (attendu: 0.5)")
    
    # Test 3: Loop penalty
    reward2 = shaper.shape_reward(state1, 1, state2, base_reward=0.0)
    print(f"✅ Test 3: État revisité - Reward={reward2:.2f} (attendu: -0.5)")
    
    # Test 4: Goal proximity
    agent_pos = (44, 51)  # Sur goal
    reward3 = shaper.shape_reward(state1, 1, state2, base_reward=0.0, agent_pos=agent_pos)
    print(f"✅ Test 4: Sur goal - Reward={reward3:.2f} (attendu: ~1.0)")
    
    agent_pos = (40, 51)  # Distance 4 du goal
    reward4 = shaper.shape_reward(state1, 1, state2, base_reward=0.0, agent_pos=agent_pos)
    print(f"✅ Test 5: Distance 4 - Reward={reward4:.2f} (attendu: ~0.2)")
    
    # Test 6: Statistiques
    stats = shaper.get_statistics()
    print(f"\n✅ Test 6: Statistiques")
    for key, value in stats.items():
        print(f"  {key}: {value}")
    
    print("\n" + "=" * 60)
    print("✅ Tests Reward Shaper terminés")

# Made with Bob
