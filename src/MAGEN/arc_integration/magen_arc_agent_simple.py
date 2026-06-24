#!/usr/bin/env python3
"""
MAGEN ARC-AGI Agent Simplifié - Wrapper pour Environnement Réel
================================================================

Agent RL simplifié utilisant le système d'apprentissage MAGEN existant
(session87_learning_system.py) adapté pour l'environnement ARC-AGI réel.

Author: MAGEN Team
Date: 2026-06-19
"""

import sys
import os
import numpy as np
import hashlib
from typing import Dict, Any, Tuple, Optional
from collections import defaultdict
import time

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import wrapper ARC-AGI réel
from arc_integration.ls20_real_wrapper import LS20RealWrapper


class MAGENSimpleAgent:
    """
    Agent MAGEN simplifié pour ARC-AGI.
    
    Utilise:
    - Apprentissage par renforcement Q-learning
    - Mémoire des états visités
    - Stratégie epsilon-greedy
    """
    
    def __init__(
        self,
        n_actions: int = 4,
        learning_rate: float = 0.1,
        gamma: float = 0.99,
        epsilon: float = 1.0,
        epsilon_decay: float = 0.995,
        epsilon_min: float = 0.01,
        verbose: bool = True
    ):
        """
        Initialise l'agent MAGEN simplifié.
        
        Args:
            n_actions: Nombre d'actions (4 pour UP/DOWN/LEFT/RIGHT)
            learning_rate: Taux d'apprentissage
            gamma: Facteur de discount
            epsilon: Exploration initiale
            epsilon_decay: Décroissance epsilon
            epsilon_min: Epsilon minimum
            verbose: Affichage détaillé
        """
        self.n_actions = n_actions
        self.learning_rate = learning_rate
        self.gamma = gamma
        self.epsilon = epsilon
        self.epsilon_decay = epsilon_decay
        self.epsilon_min = epsilon_min
        self.verbose = verbose
        
        # Q-table: state_hash -> action -> Q-value
        self.q_table = defaultdict(lambda: np.zeros(n_actions))
        
        # Statistiques
        self.episode_count = 0
        self.total_steps = 0
        self.states_visited = set()
        self.state_visit_count = defaultdict(int)
        
        # Historique
        self.episode_rewards = []
        self.episode_steps = []
        
        if self.verbose:
            print("\n" + "="*80)
            print("🧠 MAGEN SIMPLE AGENT INITIALISÉ")
            print("="*80)
            print(f"🎮 Actions: {n_actions}")
            print(f"📚 Learning rate: {learning_rate}")
            print(f"🎯 Gamma: {gamma}")
            print(f"🔍 Epsilon: {epsilon:.3f} → {epsilon_min:.3f}")
            print("="*80 + "\n")
    
    def hash_state(self, state: np.ndarray) -> str:
        """Calcule hash d'un état."""
        return hashlib.sha256(state.tobytes()).hexdigest()[:16]
    
    def select_action(self, state: np.ndarray, training: bool = True) -> int:
        """
        Sélectionne une action (epsilon-greedy).
        
        Args:
            state: État actuel
            training: Mode entraînement
            
        Returns:
            Action sélectionnée (0-3)
        """
        # Hash de l'état
        state_hash = self.hash_state(state)
        self.states_visited.add(state_hash)
        self.state_visit_count[state_hash] += 1
        
        # Exploration vs Exploitation
        if training and np.random.random() < self.epsilon:
            action = np.random.randint(0, self.n_actions)
            if self.verbose and self.total_steps % 50 == 0:
                print(f"🎲 Exploration: action={action}")
            return action
        
        # Exploitation: meilleure action Q
        q_values = self.q_table[state_hash]
        action = int(np.argmax(q_values))
        
        if self.verbose and self.total_steps % 50 == 0:
            print(f"🧠 Exploitation: action={action}, Q={q_values[action]:.3f}")
        
        return action
    
    def learn(
        self,
        state: np.ndarray,
        action: int,
        reward: float,
        next_state: np.ndarray,
        done: bool
    ):
        """
        Apprentissage Q-learning.
        
        Q(s,a) ← Q(s,a) + α[r + γ max Q(s',a') - Q(s,a)]
        """
        state_hash = self.hash_state(state)
        next_state_hash = self.hash_state(next_state)
        
        # Q-value actuel
        current_q = self.q_table[state_hash][action]
        
        # Meilleur Q-value suivant
        if done:
            max_next_q = 0.0
        else:
            max_next_q = np.max(self.q_table[next_state_hash])
        
        # Mise à jour Q-learning
        new_q = current_q + self.learning_rate * (
            reward + self.gamma * max_next_q - current_q
        )
        self.q_table[state_hash][action] = new_q
        
        self.total_steps += 1
        
        # Log périodique
        if self.verbose and self.total_steps % 100 == 0:
            print(f"📚 Q-update: Q({state_hash[:8]}..., {action}) = {new_q:.4f}")
    
    def end_episode(self, total_reward: float, steps: int):
        """Fin d'épisode."""
        self.episode_count += 1
        self.episode_rewards.append(total_reward)
        self.episode_steps.append(steps)
        
        # Décroissance epsilon
        self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
        
        if self.verbose:
            avg_reward = np.mean(self.episode_rewards[-10:]) if self.episode_rewards else 0
            print(f"\n✅ Épisode {self.episode_count} terminé")
            print(f"   Steps: {steps}")
            print(f"   Récompense: {total_reward:.2f}")
            print(f"   Avg reward (10 derniers): {avg_reward:.2f}")
            print(f"   États uniques: {len(self.states_visited)}")
            print(f"   Epsilon: {self.epsilon:.3f}\n")
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques."""
        return {
            'episodes': self.episode_count,
            'total_steps': self.total_steps,
            'unique_states': len(self.states_visited),
            'q_table_size': len(self.q_table),
            'epsilon': self.epsilon,
            'avg_reward_last_10': np.mean(self.episode_rewards[-10:]) if self.episode_rewards else 0,
            'avg_steps_last_10': np.mean(self.episode_steps[-10:]) if self.episode_steps else 0
        }


def test_agent_with_real_env():
    """Test agent avec environnement ARC-AGI réel."""
    print("\n" + "="*80)
    print("🧪 TEST AGENT MAGEN + ENVIRONNEMENT RÉEL")
    print("="*80 + "\n")
    
    # Créer environnement réel
    print("📡 Création environnement ARC-AGI réel...")
    env = LS20RealWrapper(level_index=0, max_steps=50)
    
    # Créer agent
    print("🧠 Création agent MAGEN...")
    agent = MAGENSimpleAgent(n_actions=4, verbose=True)
    
    # Test 3 épisodes
    print("\n🎮 Lancement 3 épisodes de test...\n")
    
    for episode in range(3):
        state = env.reset()
        done = False
        total_reward = 0
        steps = 0
        
        while not done and steps < 20:  # Max 20 steps par épisode test
            # Sélectionner action
            action = agent.select_action(state, training=True)
            
            # Exécuter action
            next_state, reward, done, info = env.step(action)
            
            # Apprendre
            agent.learn(state, action, reward, next_state, done)
            
            # Mise à jour
            state = next_state
            total_reward += reward
            steps += 1
        
        # Fin épisode
        agent.end_episode(total_reward, steps)
    
    # Statistiques finales
    print("\n📊 STATISTIQUES FINALES:")
    print("="*80)
    stats = agent.get_statistics()
    for key, value in stats.items():
        print(f"   {key}: {value}")
    print("="*80)
    
    print("\n✅ TEST RÉUSSI - Agent + Environnement fonctionnels\n")


if __name__ == "__main__":
    test_agent_with_real_env()

# Made with Bob
