#!/usr/bin/env python3
"""
MAGEN ARC-AGI Agent Enhanced - Avec Layer 0 + Reward Shaper
============================================================

Agent RL amélioré intégrant:
- Layer 0 Sensorimotor (identification avatar + tracking position)
- Reward Shaper (exploration + goal proximity + loop penalty)
- Epsilon decay optimisé (0.98)

Author: MAGEN Team
Date: 2026-06-19
Version: 2.0 (Enhanced)
"""

import sys
import os
import numpy as np
import hashlib
from typing import Dict, Any, Tuple, Optional, List
from collections import defaultdict
import time
import logging

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Imports MAGEN
try:
    from layers.layer0_sensorimotor import SensorimotorLayer
except ImportError:
    from ..layers.layer0_sensorimotor import SensorimotorLayer

try:
    from arc_integration.reward_shaper import RewardShaper
except ImportError:
    from .reward_shaper import RewardShaper

try:
    from arc_integration.ls20_real_wrapper import LS20RealWrapper
except ImportError:
    from .ls20_real_wrapper import LS20RealWrapper

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class MAGENEnhancedAgent:
    """
    Agent MAGEN Enhanced avec Layer 0 + Reward Shaper.
    
    Améliorations vs version simple:
    - Layer 0: Identification avatar + tracking position
    - Reward Shaper: Guidance vers goals + exploration bonus
    - Epsilon decay optimisé: 0.98 (exploitation après 50 ep)
    - Métriques monde: Conformité PROTOCOLE RÈGLE #11
    """
    
    def __init__(
        self,
        env: Optional[LS20RealWrapper] = None,
        n_actions: int = 4,
        learning_rate: float = 0.1,
        gamma: float = 0.99,
        epsilon: float = 0.9,
        epsilon_decay: float = 0.995,  # Augmenté: 0.995 pour exploration plus longue
        epsilon_min: float = 0.01,
        use_layer0: bool = True,
        use_reward_shaper: bool = True,
        verbose: bool = True
    ):
        """
        Initialise l'agent MAGEN Enhanced.
        
        Args:
            env: Environnement LS20 (optionnel)
            n_actions: Nombre d'actions (4 pour UP/DOWN/LEFT/RIGHT)
            learning_rate: Taux d'apprentissage
            gamma: Facteur de discount
            epsilon: Exploration initiale (0.9 optimal)
            epsilon_decay: Décroissance epsilon (0.98 optimal)
            epsilon_min: Epsilon minimum
            use_layer0: Activer Layer 0 Sensorimotor
            use_reward_shaper: Activer Reward Shaper
            verbose: Affichage détaillé
        """
        self.env = env
        self.n_actions = n_actions
        self.learning_rate = learning_rate
        self.gamma = gamma
        self.epsilon = epsilon
        self.epsilon_decay = epsilon_decay
        self.epsilon_min = epsilon_min
        self.use_layer0 = use_layer0
        self.use_reward_shaper = use_reward_shaper
        self.verbose = verbose
        
        # Q-table: state_hash -> action -> Q-value
        self.q_table = defaultdict(lambda: np.zeros(n_actions))
        
        # Layer 0 Sensorimotor
        if self.use_layer0:
            self.layer0 = SensorimotorLayer(env=env, verbose=verbose)
            logger.info("✅ Layer 0 Sensorimotor activé")
        else:
            self.layer0 = None
        
        # Reward Shaper
        if self.use_reward_shaper:
            # Goals depuis modèle concurrent Frame 719
            goal_positions = [(44, 51), (45, 51), (45, 52)]
            self.reward_shaper = RewardShaper(goal_positions=goal_positions)
            logger.info("✅ Reward Shaper activé")
        else:
            self.reward_shaper = None
        
        # Statistiques
        self.episode_count = 0
        self.total_steps = 0
        self.states_visited = set()
        self.state_visit_count = defaultdict(int)
        
        # Historique
        self.episode_rewards = []
        self.episode_steps = []
        self.episode_victories = []
        
        # Métriques monde (PROTOCOLE RÈGLE #11)
        self.world_metrics_history = []
        
        if self.verbose:
            print("\n" + "="*80)
            print("🧠 MAGEN ENHANCED AGENT INITIALISÉ")
            print("="*80)
            print(f"🎮 Actions: {n_actions}")
            print(f"📚 Learning rate: {learning_rate}")
            print(f"🎯 Gamma: {gamma}")
            print(f"🔍 Epsilon: {epsilon:.3f} → {epsilon_min:.3f} (decay={epsilon_decay})")
            print(f"🧬 Layer 0: {'✅ ACTIVÉ' if use_layer0 else '❌ DÉSACTIVÉ'}")
            print(f"🎁 Reward Shaper: {'✅ ACTIVÉ' if use_reward_shaper else '❌ DÉSACTIVÉ'}")
            print("="*80 + "\n")
    
    def hash_state(self, state: np.ndarray) -> str:
        """Calcule hash d'un état."""
        return hashlib.sha256(state.tobytes()).hexdigest()[:16]
    
    def select_action(self, state: np.ndarray, training: bool = True) -> int:
        """
        Sélectionne une action (epsilon-greedy).
        
        Args:
            state: État actuel
            training: Mode entraînement (exploration) ou test (exploitation)
            
        Returns:
            Action sélectionnée (0-3)
        """
        state_hash = self.hash_state(state)
        
        # Exploration vs Exploitation
        if training and np.random.random() < self.epsilon:
            # Exploration: action aléatoire
            action = np.random.randint(0, self.n_actions)
        else:
            # Exploitation: meilleure action Q-table
            q_values = self.q_table[state_hash]
            action = int(np.argmax(q_values))
        
        return action
    
    def learn(
        self,
        state: np.ndarray,
        action: int,
        reward: float,
        next_state: np.ndarray,
        done: bool,
        agent_pos: Optional[Tuple[int, int]] = None
    ):
        """
        Apprentissage Q-learning avec reward shaping.
        
        Args:
            state: État avant action
            action: Action exécutée
            reward: Reward de base
            next_state: État après action
            done: Épisode terminé
            agent_pos: Position agent (pour reward shaper)
        """
        state_hash = self.hash_state(state)
        next_state_hash = self.hash_state(next_state)
        
        # Reward shaping (si activé)
        if self.reward_shaper is not None:
            shaped_reward = self.reward_shaper.shape_reward(
                state, action, next_state, reward, agent_pos
            )
        else:
            shaped_reward = reward
        
        # Q-learning update
        current_q = self.q_table[state_hash][action]
        
        if done:
            target_q = shaped_reward
        else:
            next_q_values = self.q_table[next_state_hash]
            max_next_q = np.max(next_q_values)
            target_q = shaped_reward + self.gamma * max_next_q
        
        # Mise à jour Q-value
        self.q_table[state_hash][action] += self.learning_rate * (target_q - current_q)
        
        # Statistiques
        self.states_visited.add(state_hash)
        self.state_visit_count[state_hash] += 1
        self.total_steps += 1
    
    def train_episode(self, max_steps: int = 100) -> Dict[str, Any]:
        """
        Entraîne l'agent sur un épisode.
        
        Args:
            max_steps: Nombre maximum de steps
            
        Returns:
            Dict avec résultats épisode
        """
        if self.env is None:
            raise ValueError("Environnement non fourni")
        
        # Reset environnement
        obs = self.env.reset()
        
        # Extraire state (gérer tuple ou objet)
        if isinstance(obs, tuple):
            state = obs[0]  # (frame, reward, done, info)
        elif hasattr(obs, 'frame'):
            state = obs.frame  # type: ignore
        else:
            state = obs
        
        # Reset Layer 0 (nouveau épisode)
        if self.layer0 is not None:
            self.layer0.update_spatial_coverage(state)
        
        # Reset Reward Shaper (nouveau épisode)
        if self.reward_shaper is not None:
            self.reward_shaper.reset()
        
        episode_reward = 0.0
        episode_steps = 0
        done = False
        victory = False
        
        for step in range(max_steps):
            # Tracker position agent (Layer 0)
            agent_pos = None
            if self.layer0 is not None:
                agent_pos = self.layer0.track_position(state)
            
            # Sélectionner action
            action = self.select_action(state, training=True)
            
            # Exécuter action
            next_obs = self.env.step(action)
            
            # Extraire next_state, reward, done (gérer tuple ou objet)
            if isinstance(next_obs, tuple):
                next_state, reward, done, info = next_obs
            elif hasattr(next_obs, 'frame'):
                next_state = next_obs.frame
                reward = next_obs.reward if hasattr(next_obs, 'reward') else 0.0
                done = next_obs.done if hasattr(next_obs, 'done') else False
            else:
                next_state = next_obs
                reward = 0.0
                done = False
            
            # Mesurer causalité (Layer 0)
            if self.layer0 is not None:
                self.layer0.measure_causal_effects(action, state, next_state)
                self.layer0.update_spatial_coverage(next_state)
            
            # Apprentissage
            self.learn(state, action, reward, next_state, done, agent_pos)
            
            # Mise à jour
            state = next_state
            episode_reward += reward
            episode_steps += 1
            
            # Vérifier victoire
            if reward > 0:
                victory = True
            
            if done:
                break
        
        # Decay epsilon
        self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
        
        # Statistiques épisode
        self.episode_count += 1
        self.episode_rewards.append(episode_reward)
        self.episode_steps.append(episode_steps)
        self.episode_victories.append(victory)
        
        # Métriques monde (Layer 0)
        world_metrics = {}
        if self.layer0 is not None:
            world_metrics = self.layer0.get_world_understanding()
            self.world_metrics_history.append(world_metrics)
        
        # Statistiques Reward Shaper
        reward_shaper_stats = {}
        if self.reward_shaper is not None:
            reward_shaper_stats = self.reward_shaper.get_statistics()
        
        result = {
            'episode': self.episode_count,
            'reward': episode_reward,
            'steps': episode_steps,
            'victory': victory,
            'epsilon': self.epsilon,
            'states_discovered': len(self.states_visited),
            'world_metrics': world_metrics,
            'reward_shaper_stats': reward_shaper_stats
        }
        
        if self.verbose and self.episode_count % 10 == 0:
            logger.info(f"Episode {self.episode_count}: "
                       f"reward={episode_reward:.2f}, "
                       f"steps={episode_steps}, "
                       f"victory={'✅' if victory else '❌'}, "
                       f"epsilon={self.epsilon:.3f}, "
                       f"states={len(self.states_visited)}")
            
            if world_metrics:
                logger.info(f"  World: positions={world_metrics.get('positions_tracked', 0)}, "
                           f"avatar={'✅' if world_metrics.get('avatar_identified') else '❌'}, "
                           f"confidence={world_metrics.get('avatar_confidence', 0):.2%}")
        
        return result
    
    def get_statistics(self) -> Dict[str, Any]:
        """
        Obtenir statistiques complètes.
        
        Returns:
            Dict avec statistiques
        """
        victories = sum(self.episode_victories)
        victory_rate = victories / max(1, self.episode_count)
        
        stats = {
            'episodes': self.episode_count,
            'total_steps': self.total_steps,
            'victories': victories,
            'victory_rate': victory_rate,
            'avg_reward': np.mean(self.episode_rewards) if self.episode_rewards else 0.0,
            'avg_steps': np.mean(self.episode_steps) if self.episode_steps else 0.0,
            'states_discovered': len(self.states_visited),
            'epsilon': self.epsilon
        }
        
        # Métriques monde (Layer 0)
        if self.layer0 is not None:
            stats['world_understanding'] = self.layer0.get_world_understanding()
            stats['can_answer_questions'] = self.layer0.can_answer_questions()
        
        # Statistiques Reward Shaper
        if self.reward_shaper is not None:
            stats['reward_shaper'] = self.reward_shaper.get_statistics()
        
        return stats
    
    def __repr__(self) -> str:
        """Représentation string"""
        return (f"MAGENEnhancedAgent(episodes={self.episode_count}, "
                f"victories={sum(self.episode_victories)}, "
                f"states={len(self.states_visited)}, "
                f"layer0={'✅' if self.use_layer0 else '❌'}, "
                f"reward_shaper={'✅' if self.use_reward_shaper else '❌'})")


# Tests unitaires
if __name__ == "__main__":
    print("🧪 Tests MAGEN Enhanced Agent")
    print("=" * 60)
    
    # Test 1: Initialisation
    agent = MAGENEnhancedAgent(verbose=True)
    print(f"\n✅ Test 1: {agent}")
    
    # Test 2: Statistiques
    stats = agent.get_statistics()
    print(f"\n✅ Test 2: Statistiques")
    for key, value in stats.items():
        if isinstance(value, dict):
            print(f"  {key}:")
            for k, v in value.items():
                print(f"    {k}: {v}")
        else:
            print(f"  {key}: {value}")
    
    print("\n" + "=" * 60)
    print("✅ Tests MAGEN Enhanced Agent terminés")

# Made with Bob
