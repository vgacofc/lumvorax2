#!/usr/bin/env python3
"""
MAGEN - Entraînement Niveau 1 JUSQU'À VICTOIRE
===============================================

Entraîne l'agent sur niveau 1 jusqu'à obtenir AU MOINS 1 victoire.
Conformité PROTOCOLE MAGEN: Pas de passage niveau suivant sans victoire.

Author: MAGEN Team
Date: 2026-06-19
Version: 1.0 (Until Victory)
"""

import sys
import os
import time
import json
from datetime import datetime
from typing import Dict, Any

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from arc_integration.ls20_real_wrapper import LS20RealWrapper
from arc_integration.magen_arc_agent_simple import MAGENSimpleAgent


class UntilVictoryTrainer:
    """Entraîneur jusqu'à victoire niveau 1"""
    
    def __init__(
        self,
        level: int = 1,
        checkpoint_every: int = 100,
        max_episodes: int = 10000,
        target_victories: int = 1
    ):
        """
        Initialiser entraîneur
        
        Args:
            level: Niveau à entraîner
            checkpoint_every: Sauvegarder tous les N épisodes
            max_episodes: Limite sécurité (10,000 par défaut)
            target_victories: Nombre victoires cible
        """
        self.level = level
        self.checkpoint_every = checkpoint_every
        self.max_episodes = max_episodes
        self.target_victories = target_victories
        
        # Créer environnement
        self.env = LS20RealWrapper()
        
        # Créer agent
        self.agent = MAGENSimpleAgent(
            epsilon=0.9,  # Start high
            epsilon_decay=0.9995,  # Très lent
            epsilon_min=0.01,
            verbose=False
        )
        
        # Statistiques
        self.total_episodes = 0
        self.total_victories = 0
        self.episode_rewards = []
        self.episode_steps = []
        self.victory_episodes = []
        
        print("="*60)
        print("MAGEN - ENTRAÎNEMENT JUSQU'À VICTOIRE")
        print("="*60)
        print(f"Niveau: {level}")
        print(f"Victoires cibles: {target_victories}")
        print(f"Checkpoint tous les: {checkpoint_every} épisodes")
        print(f"Limite sécurité: {max_episodes} épisodes")
        print("="*60)
        print()
    
    def train_episode(self) -> Dict[str, Any]:
        """
        Entraîner un épisode
        
        Returns:
            Résultats épisode
        """
        # Reset
        obs = self.env.reset()
        if isinstance(obs, tuple):
            state = obs[0]
        else:
            state = obs
        
        episode_reward = 0.0
        episode_steps = 0
        done = False
        victory = False
        
        max_steps = 100
        
        for step in range(max_steps):
            # Sélectionner action
            action = self.agent.select_action(state, training=True)
            
            # Exécuter
            next_obs = self.env.step(action)
            
            # Extraire
            if isinstance(next_obs, tuple):
                next_state, reward, done, info = next_obs
            else:
                next_state = next_obs
                reward = 0.0
                done = False
            
            # Apprendre
            self.agent.learn(state, action, reward, next_state, done)
            
            # Mise à jour
            state = next_state
            episode_reward += reward
            episode_steps += 1
            
            # Victoire?
            if reward > 0:
                victory = True
                break
            
            if done:
                break
        
        # Fin épisode
        self.agent.end_episode(episode_reward, episode_steps)
        
        return {
            'episode': self.total_episodes + 1,
            'reward': episode_reward,
            'steps': episode_steps,
            'victory': victory,
            'epsilon': self.agent.epsilon,
            'unique_states': len(self.agent.q_table)
        }
    
    def train_until_victory(self) -> Dict[str, Any]:
        """
        Entraîner jusqu'à victoire
        
        Returns:
            Rapport complet
        """
        start_time = time.time()
        
        print(f"🎯 Objectif: {self.target_victories} victoire(s)")
        print(f"⏱️  Démarrage: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print()
        
        while self.total_victories < self.target_victories:
            # Vérifier limite
            if self.total_episodes >= self.max_episodes:
                print(f"\n⚠️  LIMITE ATTEINTE: {self.max_episodes} épisodes")
                print(f"   Victoires: {self.total_victories}/{self.target_victories}")
                break
            
            # Entraîner épisode
            result = self.train_episode()
            
            # Mise à jour stats
            self.total_episodes += 1
            self.episode_rewards.append(result['reward'])
            self.episode_steps.append(result['steps'])
            
            if result['victory']:
                self.total_victories += 1
                self.victory_episodes.append(self.total_episodes)
                print(f"\n🎉 VICTOIRE #{self.total_victories} à l'épisode {self.total_episodes}!")
                print(f"   Reward: {result['reward']:.2f}")
                print(f"   Steps: {result['steps']}")
                print(f"   Epsilon: {result['epsilon']:.3f}")
                print(f"   États: {result['unique_states']}")
                print()
            
            # Checkpoint
            if self.total_episodes % self.checkpoint_every == 0:
                elapsed = time.time() - start_time
                avg_reward = sum(self.episode_rewards[-100:]) / min(100, len(self.episode_rewards))
                
                print(f"📊 Épisode {self.total_episodes}/{self.max_episodes}")
                print(f"   Victoires: {self.total_victories}/{self.target_victories}")
                print(f"   Reward moyen (100 derniers): {avg_reward:.2f}")
                print(f"   Epsilon: {result['epsilon']:.3f}")
                print(f"   États uniques: {result['unique_states']}")
                print(f"   Temps écoulé: {elapsed/60:.1f} min")
                print(f"   Progression: {(self.total_victories/self.target_victories)*100:.1f}%")
                print()
        
        # Fin entraînement
        end_time = time.time()
        duration = end_time - start_time
        
        # Rapport
        report = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'level': self.level,
                'target_victories': self.target_victories,
                'max_episodes': self.max_episodes
            },
            'results': {
                'total_episodes': self.total_episodes,
                'total_victories': self.total_victories,
                'victory_rate': (self.total_victories / self.total_episodes * 100) if self.total_episodes > 0 else 0,
                'victory_episodes': self.victory_episodes,
                'final_epsilon': self.agent.epsilon,
                'unique_states': len(self.agent.q_table),
                'duration_seconds': duration,
                'duration_minutes': duration / 60
            },
            'statistics': {
                'mean_reward': sum(self.episode_rewards) / len(self.episode_rewards) if self.episode_rewards else 0,
                'mean_steps': sum(self.episode_steps) / len(self.episode_steps) if self.episode_steps else 0,
                'min_reward': min(self.episode_rewards) if self.episode_rewards else 0,
                'max_reward': max(self.episode_rewards) if self.episode_rewards else 0
            }
        }
        
        return report
    
    def save_report(self, report: Dict[str, Any], filepath: str):
        """Sauvegarder rapport"""
        with open(filepath, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\n💾 Rapport sauvegardé: {filepath}")
    
    def print_final_report(self, report: Dict[str, Any]):
        """Afficher rapport final"""
        print("\n" + "="*60)
        print("RAPPORT FINAL - ENTRAÎNEMENT NIVEAU 1")
        print("="*60)
        
        results = report['results']
        stats = report['statistics']
        
        print(f"\n🎯 OBJECTIF:")
        print(f"   Victoires cibles: {report['metadata']['target_victories']}")
        print(f"   Victoires obtenues: {results['total_victories']}")
        print(f"   Statut: {'✅ RÉUSSI' if results['total_victories'] >= report['metadata']['target_victories'] else '❌ ÉCHEC'}")
        
        print(f"\n📊 STATISTIQUES:")
        print(f"   Épisodes totaux: {results['total_episodes']}")
        print(f"   Taux victoire: {results['victory_rate']:.2f}%")
        print(f"   Épisodes victoire: {results['victory_episodes']}")
        print(f"   États uniques: {results['unique_states']}")
        print(f"   Epsilon final: {results['final_epsilon']:.3f}")
        
        print(f"\n📈 PERFORMANCE:")
        print(f"   Reward moyen: {stats['mean_reward']:.2f}")
        print(f"   Steps moyen: {stats['mean_steps']:.1f}")
        print(f"   Reward min/max: {stats['min_reward']:.2f} / {stats['max_reward']:.2f}")
        
        print(f"\n⏱️  DURÉE:")
        print(f"   Temps total: {results['duration_minutes']:.1f} minutes")
        print(f"   Temps/épisode: {results['duration_seconds']/results['total_episodes']:.2f} secondes")
        
        print("\n" + "="*60)


def main():
    """Fonction principale"""
    
    # Créer entraîneur
    trainer = UntilVictoryTrainer(
        level=1,
        checkpoint_every=100,
        max_episodes=10000,
        target_victories=1
    )
    
    # Entraîner jusqu'à victoire
    report = trainer.train_until_victory()
    
    # Afficher rapport
    trainer.print_final_report(report)
    
    # Sauvegarder
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = f"training_until_victory_level1_{timestamp}.json"
    trainer.save_report(report, report_path)
    
    # Statut final
    if report['results']['total_victories'] >= trainer.target_victories:
        print("\n✅ NIVEAU 1 RÉUSSI - Prêt pour niveau 2")
        return 0
    else:
        print("\n❌ NIVEAU 1 NON RÉUSSI - Continuer entraînement")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
