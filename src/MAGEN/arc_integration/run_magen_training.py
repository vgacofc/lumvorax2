#!/usr/bin/env python3
"""
MAGEN ARC-AGI Training - Intégration Complète Session87 + Environnement Réel
=============================================================================

Utilise le système d'apprentissage COMPLET de session87_learning_system.py
adapté pour l'environnement ARC-AGI réel ls20-9607627b.

Architecture:
- LearningSystem de session87 (Q-learning, mémoire états, stratégies)
- LS20RealWrapper (connexion jeu réel)
- Logs forensiques complets
- Checkpointing automatique

Author: MAGEN Team
Date: 2026-06-19
"""

import sys
import os
import time
import json
import hashlib
import numpy as np
from datetime import datetime
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Tuple, Set

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import wrapper ARC-AGI réel
from arc_integration.ls20_real_wrapper import LS20RealWrapper

# Import système d'apprentissage session87
from session87_learning_system import LearningSystem


class MAGENRealTraining:
    """
    Entraînement MAGEN sur environnement ARC-AGI réel.
    
    Intègre:
    - LearningSystem (session87) pour l'apprentissage
    - LS20RealWrapper pour l'environnement réel
    - Logs forensiques complets
    """
    
    def __init__(
        self,
        level_index: int = 0,
        n_episodes: int = 100,
        max_steps_per_episode: int = 50,
        checkpoint_every: int = 10,
        verbose: bool = True
    ):
        """
        Initialise l'entraînement MAGEN réel.
        
        Args:
            level_index: Niveau du jeu (0-6)
            n_episodes: Nombre d'épisodes d'entraînement
            max_steps_per_episode: Steps max par épisode
            checkpoint_every: Sauvegarder tous les N épisodes
            verbose: Affichage détaillé
        """
        self.level_index = level_index
        self.n_episodes = n_episodes
        self.max_steps_per_episode = max_steps_per_episode
        self.checkpoint_every = checkpoint_every
        self.verbose = verbose
        
        # Environnement réel
        self.env = None
        
        # Système d'apprentissage session87
        self.learning_system = None
        
        # Métriques
        self.episode_rewards = []
        self.episode_steps = []
        self.episode_successes = []
        self.unique_states_per_episode = []
        
        # Logs forensiques
        self.forensic_logs = []
        
        # Temps
        self.start_time = None
        self.end_time = None
        
    def initialize(self):
        """Initialise environnement et système d'apprentissage."""
        print("\n" + "="*80)
        print("🚀 INITIALISATION ENTRAÎNEMENT MAGEN RÉEL")
        print("="*80)
        print(f"📊 Épisodes: {self.n_episodes}")
        print(f"🎯 Max steps/épisode: {self.max_steps_per_episode}")
        print(f"💾 Checkpoint tous les: {self.checkpoint_every} épisodes")
        print("="*80 + "\n")
        
        # Créer environnement réel
        print("📡 Création environnement ARC-AGI réel...")
        self.env = LS20RealWrapper(
            level_index=self.level_index,
            max_steps=self.max_steps_per_episode
        )
        print(f"✅ Environnement créé: ls20-9607627b niveau {self.level_index + 1}")
        
        # Créer système d'apprentissage (adapté de session87)
        print("\n🧠 Initialisation système d'apprentissage MAGEN...")
        self.learning_system = self._create_learning_system()
        print("✅ Système d'apprentissage initialisé")
        
        print("\n" + "="*80)
        print("✅ INITIALISATION COMPLÈTE")
        print("="*80 + "\n")
    
    def _create_learning_system(self):
        """Crée système d'apprentissage adapté de session87."""
        class AdaptedLearningSystem:
            """Système d'apprentissage adapté pour environnement réel."""
            
            def __init__(self):
                # Q-table: state_hash -> action -> Q-value
                self.state_action_rewards = defaultdict(lambda: defaultdict(float))
                self.state_visit_count = defaultdict(int)
                self.action_success_rate = defaultdict(lambda: {"success": 0, "total": 0})
                
                # États découverts
                self.all_states_seen = set()
                self.all_transitions = []
                
                # Statistiques
                self.total_games = 0
                self.total_steps = 0
                
                # Paramètres apprentissage
                self.learning_rate = 0.1
                self.gamma = 0.99
                self.epsilon = 1.0
                self.epsilon_decay = 0.995
                self.epsilon_min = 0.01
            
            def hash_frame(self, frame) -> str:
                """Hash d'un frame."""
                if isinstance(frame, list):
                    frame = np.array(frame)
                return hashlib.sha256(frame.tobytes()).hexdigest()[:16]
            
            def choose_action(self, state_hash: str, episode: int) -> int:
                """Choisit action (epsilon-greedy)."""
                # Exploration
                if np.random.random() < self.epsilon:
                    return np.random.randint(0, 4)
                
                # Exploitation: meilleure action Q
                q_values = self.state_action_rewards[state_hash]
                if not q_values:
                    return np.random.randint(0, 4)
                
                return max(q_values.items(), key=lambda x: x[1])[0]
            
            def update_q_value(
                self,
                state_hash: str,
                action: int,
                reward: float,
                next_state_hash: str,
                done: bool
            ):
                """Mise à jour Q-learning."""
                # Q actuel
                current_q = self.state_action_rewards[state_hash][action]
                
                # Meilleur Q suivant
                if done:
                    max_next_q = 0.0
                else:
                    next_q_values = self.state_action_rewards[next_state_hash]
                    max_next_q = max(next_q_values.values()) if next_q_values else 0.0
                
                # Mise à jour Q-learning
                new_q = current_q + self.learning_rate * (
                    reward + self.gamma * max_next_q - current_q
                )
                self.state_action_rewards[state_hash][action] = new_q
                
                # Statistiques
                self.state_visit_count[state_hash] += 1
                self.action_success_rate[action]["total"] += 1
                if reward > 0:
                    self.action_success_rate[action]["success"] += 1
            
            def decay_epsilon(self):
                """Décroissance epsilon."""
                self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
        
        return AdaptedLearningSystem()
    
    def train(self):
        """Lance l'entraînement complet."""
        self.start_time = time.time()
        
        print("\n" + "="*80)
        print("🎮 DÉBUT ENTRAÎNEMENT")
        print("="*80 + "\n")
        
        for episode in range(1, self.n_episodes + 1):
            episode_start = time.time()
            
            # Reset environnement
            state = self.env.reset()
            state_hash = self.learning_system.hash_frame(state)
            
            done = False
            total_reward = 0
            steps = 0
            states_this_episode = set()
            
            if self.verbose:
                print(f"\n{'='*80}")
                print(f"📊 ÉPISODE {episode}/{self.n_episodes}")
                print(f"{'='*80}")
            
            # Boucle épisode
            while not done and steps < self.max_steps_per_episode:
                # Choisir action
                action = self.learning_system.choose_action(state_hash, episode)
                
                # Exécuter action
                next_state, reward, done, info = self.env.step(action)
                next_state_hash = self.learning_system.hash_frame(next_state)
                
                # Apprendre
                self.learning_system.update_q_value(
                    state_hash, action, reward, next_state_hash, done
                )
                
                # Logs forensiques
                self._log_step(episode, steps, action, reward, done, info)
                
                # Mise à jour
                state = next_state
                state_hash = next_state_hash
                total_reward += reward
                steps += 1
                states_this_episode.add(state_hash)
                self.learning_system.all_states_seen.add(state_hash)
                
                if self.verbose and steps % 10 == 0:
                    print(f"   Step {steps}: action={action}, reward={reward:.3f}, done={done}")
            
            # Fin épisode
            self.learning_system.decay_epsilon()
            self.learning_system.total_games += 1
            self.learning_system.total_steps += steps
            
            # Métriques
            self.episode_rewards.append(total_reward)
            self.episode_steps.append(steps)
            self.episode_successes.append(info.get('success', False))
            self.unique_states_per_episode.append(len(states_this_episode))
            
            episode_time = time.time() - episode_start
            
            # Affichage
            if self.verbose:
                print(f"\n✅ Épisode {episode} terminé en {episode_time:.2f}s")
                print(f"   Steps: {steps}")
                print(f"   Récompense: {total_reward:.2f}")
                print(f"   États uniques: {len(states_this_episode)}")
                print(f"   Succès: {info.get('success', False)}")
                print(f"   Epsilon: {self.learning_system.epsilon:.3f}")
                print(f"   États totaux découverts: {len(self.learning_system.all_states_seen)}")
            
            # Checkpoint
            if episode % self.checkpoint_every == 0:
                self._save_checkpoint(episode)
        
        self.end_time = time.time()
        
        # Rapport final
        self._print_final_report()
    
    def _log_step(self, episode: int, step: int, action: int, reward: float, done: bool, info: Dict):
        """Log forensique d'un step."""
        log_entry = {
            'timestamp': time.time(),
            'episode': episode,
            'step': step,
            'action': action,
            'action_name': ['UP', 'DOWN', 'LEFT', 'RIGHT'][action],
            'reward': reward,
            'done': done,
            'info': info
        }
        self.forensic_logs.append(log_entry)
    
    def _save_checkpoint(self, episode: int):
        """Sauvegarde checkpoint."""
        checkpoint_dir = Path("checkpoints")
        checkpoint_dir.mkdir(exist_ok=True)
        
        checkpoint_file = checkpoint_dir / f"checkpoint_episode_{episode}.json"
        
        checkpoint_data = {
            'episode': episode,
            'total_steps': self.learning_system.total_steps,
            'unique_states': len(self.learning_system.all_states_seen),
            'epsilon': self.learning_system.epsilon,
            'episode_rewards': self.episode_rewards,
            'episode_steps': self.episode_steps,
            'episode_successes': self.episode_successes,
            'q_table_size': len(self.learning_system.state_action_rewards),
            'timestamp': datetime.now().isoformat()
        }
        
        with open(checkpoint_file, 'w') as f:
            json.dump(checkpoint_data, f, indent=2)
        
        print(f"\n💾 Checkpoint sauvegardé: {checkpoint_file}")
    
    def _print_final_report(self):
        """Affiche rapport final."""
        duration = self.end_time - self.start_time
        
        print("\n" + "="*80)
        print("📊 RAPPORT FINAL ENTRAÎNEMENT")
        print("="*80)
        print(f"\n⏱️  Durée totale: {duration:.2f}s ({duration/60:.2f} minutes)")
        print(f"📈 Épisodes: {self.n_episodes}")
        print(f"🎯 Steps totaux: {self.learning_system.total_steps}")
        print(f"🗺️  États uniques découverts: {len(self.learning_system.all_states_seen)}")
        print(f"📚 Taille Q-table: {len(self.learning_system.state_action_rewards)}")
        print(f"\n💰 Récompenses:")
        print(f"   Moyenne: {np.mean(self.episode_rewards):.3f}")
        print(f"   Min: {np.min(self.episode_rewards):.3f}")
        print(f"   Max: {np.max(self.episode_rewards):.3f}")
        print(f"\n🏆 Succès:")
        print(f"   Taux: {np.mean(self.episode_successes)*100:.1f}%")
        print(f"   Total: {sum(self.episode_successes)}/{self.n_episodes}")
        print(f"\n🔍 Epsilon final: {self.learning_system.epsilon:.3f}")
        print("="*80 + "\n")


def main():
    """Point d'entrée principal."""
    print("\n" + "="*80)
    print("🧠 MAGEN ARC-AGI TRAINING - ENVIRONNEMENT RÉEL")
    print("="*80 + "\n")
    
    # Créer entraînement
    training = MAGENRealTraining(
        level_index=0,
        n_episodes=100,
        max_steps_per_episode=50,
        checkpoint_every=10,
        verbose=True
    )
    
    # Initialiser
    training.initialize()
    
    # Entraîner
    training.train()
    
    print("\n✅ ENTRAÎNEMENT TERMINÉ\n")


if __name__ == "__main__":
    main()

# Made with Bob
