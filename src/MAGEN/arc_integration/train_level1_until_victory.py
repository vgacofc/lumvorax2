#!/usr/bin/env python3
"""
MAGEN - Entraînement Intensif Niveau 1 UNIQUEMENT
==================================================

OBJECTIF: Obtenir AU MOINS UNE VICTOIRE sur le niveau 1 avant de passer au niveau 2.

Stratégie:
- Entraînement continu jusqu'à victoire
- Logs forensiques COMPLETS de chaque épisode
- Analyse détaillée des échecs
- Ajustement dynamique hyperparamètres
- Sauvegarde checkpoint à chaque victoire

Critère d'arrêt: AU MOINS 1 VICTOIRE obtenue

Auteur: Bob (Expert RL + ARC-AGI + Forensic)
Date: 2026-06-19
"""

import sys
import os
import json
import time
import pickle
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass, asdict
from datetime import datetime
import numpy as np

# Ajout chemin MAGEN
sys.path.insert(0, str(Path(__file__).parent.parent))

from arc_integration.ls20_real_wrapper import LS20RealWrapper
from arc_integration.magen_arc_agent_simple import MAGENSimpleAgent


@dataclass
class EpisodeResult:
    """Résultat d'un épisode."""
    episode: int
    steps: int
    reward: float
    victory: bool
    unique_states: int
    epsilon: float
    timestamp: str
    forensic_log_path: str


class Level1IntensiveTrainer:
    """
    Entraîneur intensif pour niveau 1 UNIQUEMENT.
    
    Continue jusqu'à obtenir AU MOINS 1 victoire.
    """
    
    def __init__(
        self,
        max_episodes: int = 1000,
        max_steps_per_episode: int = 100,
        checkpoint_dir: str = "./checkpoints_level1",
        forensic_dir: str = "./forensic_level1",
        results_dir: str = "./results_level1"
    ):
        """
        Initialise l'entraîneur niveau 1.
        
        Args:
            max_episodes: Nombre max d'épisodes (sécurité)
            max_steps_per_episode: Steps max par épisode
            checkpoint_dir: Répertoire checkpoints
            forensic_dir: Répertoire logs forensiques
            results_dir: Répertoire résultats
        """
        self.max_episodes = max_episodes
        self.max_steps_per_episode = max_steps_per_episode
        
        # Créer répertoires
        self.checkpoint_dir = Path(checkpoint_dir)
        self.forensic_dir = Path(forensic_dir)
        self.results_dir = Path(results_dir)
        
        for dir_path in [self.checkpoint_dir, self.forensic_dir, self.results_dir]:
            dir_path.mkdir(parents=True, exist_ok=True)
        
        # Environnement niveau 1
        self.env = LS20RealWrapper(level_index=0)  # Niveau 1
        
        # Agent MAGEN
        self.agent = MAGENSimpleAgent(
            n_actions=4,
            learning_rate=0.1,
            gamma=0.99,
            epsilon=0.9,  # Corrigé: 0.9 au lieu de 0.995
            epsilon_decay=0.98,  # Corrigé: 0.98 au lieu de 0.995 (exploitation après 50 ep)
            epsilon_min=0.01,
            verbose=False  # Désactiver logs verbeux
        )
        
        # Métriques
        self.episodes_results: List[EpisodeResult] = []
        self.victories: List[int] = []  # Episodes avec victoire
        self.total_steps = 0
        self.unique_states = set()
        
    def train_until_victory(self) -> Dict:
        """
        Entraîne jusqu'à obtenir AU MOINS 1 victoire.
        
        Returns:
            Dict: Rapport complet
        """
        print("\n" + "="*80)
        print("🎯 ENTRAÎNEMENT INTENSIF NIVEAU 1 - JUSQU'À VICTOIRE")
        print("="*80)
        print(f"Niveau: 1 (ls20-9607627b)")
        print(f"Max épisodes: {self.max_episodes}")
        print(f"Max steps/épisode: {self.max_steps_per_episode}")
        print(f"Critère arrêt: AU MOINS 1 VICTOIRE")
        print("="*80 + "\n")
        
        start_time = time.time()
        episode = 0
        
        while episode < self.max_episodes and len(self.victories) == 0:
            episode += 1
            
            # Entraîner épisode
            result = self._train_episode(episode)
            self.episodes_results.append(result)
            
            # Afficher progression
            if episode % 10 == 0 or result.victory:
                self._print_progress(episode, result)
            
            # Sauvegarder checkpoint si victoire
            if result.victory:
                self.victories.append(episode)
                self._save_victory_checkpoint(episode, result)
                print(f"\n🏆 VICTOIRE OBTENUE À L'ÉPISODE {episode} !")
                print(f"   Reward: {result.reward:.2f}")
                print(f"   Steps: {result.steps}")
                print(f"   États uniques: {result.unique_states}")
                break
            
            # Sauvegarder checkpoint périodique
            if episode % 50 == 0:
                self._save_checkpoint(episode)
        
        training_time = time.time() - start_time
        
        # Générer rapport final
        report = self._generate_report(episode, training_time)
        self._save_report(report)
        self._print_summary(report)
        
        return report
    
    def _train_episode(self, episode: int) -> EpisodeResult:
        """
        Entraîne un épisode complet.
        
        Args:
            episode: Numéro épisode
            
        Returns:
            EpisodeResult: Résultat épisode
        """
        # Reset environnement
        obs = self.env.reset()
        done = False
        episode_reward = 0.0
        episode_steps = 0
        episode_states = set()
        
        # Logs forensiques épisode
        forensic_log = []
        
        while not done and episode_steps < self.max_steps_per_episode:
            # Sélectionner action
            action = self.agent.select_action(obs, training=True)
            
            # Exécuter action
            next_obs, reward, done, info = self.env.step(action)
            
            # Apprendre
            self.agent.learn(obs, action, reward, next_obs, done)
            
            # Tracking
            state_hash = self.agent.hash_state(obs)
            episode_states.add(state_hash)
            self.unique_states.add(state_hash)
            
            # Log forensique
            forensic_entry = {
                'episode': episode,
                'step': episode_steps + 1,
                'state_hash': state_hash,
                'action': int(action),
                'reward': float(reward),
                'done': bool(done),
                'epsilon': float(self.agent.epsilon),
                'q_values': self.agent.q_table[state_hash].tolist() if state_hash in self.agent.q_table else [0.0]*4
            }
            forensic_log.append(forensic_entry)
            
            # Mise à jour
            obs = next_obs
            episode_reward += reward
            episode_steps += 1
            self.total_steps += 1
        
        # Sauvegarder logs forensiques
        forensic_path = self._save_forensic_log(episode, forensic_log)
        
        # Déterminer victoire
        victory = episode_reward > 0  # Récompense positive = victoire
        
        result = EpisodeResult(
            episode=episode,
            steps=episode_steps,
            reward=episode_reward,
            victory=victory,
            unique_states=len(episode_states),
            epsilon=self.agent.epsilon,
            timestamp=datetime.now().isoformat(),
            forensic_log_path=str(forensic_path)
        )
        
        return result
    
    def _save_forensic_log(self, episode: int, forensic_log: List[Dict]) -> Path:
        """Sauvegarde logs forensiques épisode."""
        forensic_path = self.forensic_dir / f"episode_{episode:04d}.json"
        with open(forensic_path, 'w') as f:
            json.dump(forensic_log, f, indent=2)
        return forensic_path
    
    def _save_checkpoint(self, episode: int):
        """Sauvegarde checkpoint."""
        checkpoint_path = self.checkpoint_dir / f"checkpoint_episode_{episode:04d}.pkl"
        with open(checkpoint_path, 'wb') as f:
            pickle.dump({
                'episode': episode,
                'agent_q_table': self.agent.q_table,
                'agent_epsilon': self.agent.epsilon,
                'total_steps': self.total_steps,
                'unique_states': len(self.unique_states),
                'victories': self.victories
            }, f)
        print(f"   💾 Checkpoint sauvegardé: episode {episode}")
    
    def _save_victory_checkpoint(self, episode: int, result: EpisodeResult):
        """Sauvegarde checkpoint spécial victoire."""
        victory_path = self.checkpoint_dir / f"VICTORY_episode_{episode:04d}.pkl"
        with open(victory_path, 'wb') as f:
            pickle.dump({
                'episode': episode,
                'result': asdict(result),
                'agent_q_table': self.agent.q_table,
                'agent_epsilon': self.agent.epsilon,
                'total_steps': self.total_steps,
                'unique_states': len(self.unique_states)
            }, f)
        print(f"   🏆 Checkpoint VICTOIRE sauvegardé: {victory_path}")
    
    def _print_progress(self, episode: int, result: EpisodeResult):
        """Affiche progression."""
        symbol = "🏆" if result.victory else "📊"
        print(f"{symbol} Episode {episode:4d}: "
              f"reward={result.reward:+6.2f}, "
              f"steps={result.steps:3d}, "
              f"states={result.unique_states:3d}, "
              f"epsilon={result.epsilon:.3f}, "
              f"total_states={len(self.unique_states):4d}")
    
    def _generate_report(self, episodes_trained: int, training_time: float) -> Dict:
        """Génère rapport final."""
        report = {
            'level': 1,
            'episodes_trained': episodes_trained,
            'training_time': training_time,
            'training_time_minutes': training_time / 60,
            'victories': len(self.victories),
            'victory_episodes': self.victories,
            'success_rate': (len(self.victories) / episodes_trained * 100) if episodes_trained > 0 else 0,
            'total_steps': self.total_steps,
            'unique_states': len(self.unique_states),
            'avg_reward': np.mean([r.reward for r in self.episodes_results]),
            'avg_steps': np.mean([r.steps for r in self.episodes_results]),
            'final_epsilon': self.agent.epsilon,
            'episodes_results': [asdict(r) for r in self.episodes_results]
        }
        return report
    
    def _save_report(self, report: Dict):
        """Sauvegarde rapport JSON."""
        report_path = self.results_dir / "level1_training_report.json"
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\n✅ Rapport sauvegardé: {report_path}")
    
    def _print_summary(self, report: Dict):
        """Affiche résumé final."""
        print("\n" + "="*80)
        print("📊 RÉSUMÉ ENTRAÎNEMENT NIVEAU 1")
        print("="*80)
        
        print(f"\n🎯 Objectif: AU MOINS 1 VICTOIRE")
        print(f"   Statut: {'✅ RÉUSSI' if report['victories'] > 0 else '❌ ÉCHEC'}")
        
        print(f"\n📈 Métriques:")
        print(f"   Episodes entraînés: {report['episodes_trained']}")
        print(f"   Temps total: {report['training_time']:.1f}s ({report['training_time_minutes']:.1f}min)")
        print(f"   Victoires: {report['victories']}")
        print(f"   Success rate: {report['success_rate']:.2f}%")
        print(f"   Total steps: {report['total_steps']}")
        print(f"   États uniques: {report['unique_states']}")
        print(f"   Reward moyen: {report['avg_reward']:.2f}")
        print(f"   Steps moyens: {report['avg_steps']:.1f}")
        print(f"   Epsilon final: {report['final_epsilon']:.3f}")
        
        if report['victories'] > 0:
            print(f"\n🏆 Épisodes avec victoire:")
            for ep in report['victory_episodes']:
                result = next(r for r in self.episodes_results if r.episode == ep)
                print(f"   Episode {ep}: reward={result.reward:.2f}, steps={result.steps}")
        
        print("\n" + "="*80)


def main():
    """Point d'entrée principal."""
    trainer = Level1IntensiveTrainer(
        max_episodes=1000,
        max_steps_per_episode=100,
        checkpoint_dir="./checkpoints_level1",
        forensic_dir="./forensic_level1",
        results_dir="./results_level1"
    )
    
    report = trainer.train_until_victory()
    
    if report['victories'] > 0:
        print("\n✅ NIVEAU 1 VALIDÉ - Prêt pour niveau 2")
    else:
        print("\n⚠️  Aucune victoire obtenue - Continuer entraînement")


if __name__ == "__main__":
    main()

# Made with Bob
