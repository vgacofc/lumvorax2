#!/usr/bin/env python3
"""
MAGEN Multi-Level Transfer Learning Script
==========================================

Entraînement progressif sur les 7 niveaux du jeu ls20-9607627b avec transfer learning.

Architecture:
- Niveau 1 (facile): Entraînement from scratch (100 épisodes)
- Niveaux 2-7: Transfer learning avec fine-tuning (50 épisodes chacun)
- Sauvegarde checkpoints après chaque niveau
- Métriques comparatives entre niveaux

Auteur: Bob (Assistant IA)
Date: 2026-06-19
"""

import sys
import os
import json
import pickle
import time
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass, asdict
import numpy as np

# Ajout chemin MAGEN
sys.path.insert(0, str(Path(__file__).parent.parent))

from arc_integration.ls20_real_wrapper import LS20RealWrapper
from arc_integration.magen_arc_agent_simple import MAGENSimpleAgent


@dataclass
class LevelMetrics:
    """Métriques pour un niveau spécifique."""
    level: int
    episodes: int
    total_steps: int
    unique_states: int
    success_rate: float
    avg_reward: float
    avg_steps_per_episode: float
    best_reward: float
    training_time: float
    transfer_from_level: int = 0  # 0 = from scratch


@dataclass
class TransferLearningReport:
    """Rapport complet du transfer learning."""
    game_id: str
    total_levels: int
    total_episodes: int
    total_training_time: float
    level_metrics: List[LevelMetrics]
    transfer_improvement: Dict[int, float]  # level -> improvement %
    generalization_score: float


class MultiLevelTrainer:
    """Entraîneur multi-niveaux avec transfer learning."""
    
    def __init__(
        self,
        game_id: str = "ls20-9607627b",
        episodes_per_level: List[int] = None,
        checkpoint_dir: str = "./checkpoints_multilevel",
        results_dir: str = "./results_multilevel"
    ):
        """
        Initialise l'entraîneur multi-niveaux.
        
        Args:
            game_id: ID du jeu ARC-AGI
            episodes_per_level: Nombre d'épisodes par niveau [100, 50, 50, ...]
            checkpoint_dir: Répertoire pour sauvegarder les checkpoints
            results_dir: Répertoire pour sauvegarder les résultats
        """
        self.game_id = game_id
        self.episodes_per_level = episodes_per_level or [100, 50, 50, 50, 50, 50, 50]
        self.checkpoint_dir = Path(checkpoint_dir)
        self.results_dir = Path(results_dir)
        
        # Créer répertoires
        self.checkpoint_dir.mkdir(parents=True, exist_ok=True)
        self.results_dir.mkdir(parents=True, exist_ok=True)
        
        # Métriques
        self.level_metrics: List[LevelMetrics] = []
        self.agent: MAGENSimpleAgent = None  # type: ignore
        
    def train_level(
        self,
        level: int,
        num_episodes: int,
        transfer_from_level: int = 0
    ) -> LevelMetrics:
        """
        Entraîne MAGEN sur un niveau spécifique.
        
        Args:
            level: Numéro du niveau (1-7)
            num_episodes: Nombre d'épisodes d'entraînement
            transfer_from_level: Niveau source pour transfer learning (0 = from scratch)
            
        Returns:
            LevelMetrics: Métriques d'entraînement du niveau
        """
        print(f"\n{'='*60}")
        print(f"NIVEAU {level}/7 - {num_episodes} épisodes")
        if transfer_from_level > 0:
            print(f"Transfer Learning depuis niveau {transfer_from_level}")
        else:
            print("Entraînement FROM SCRATCH")
        print(f"{'='*60}\n")
        
        # Créer environnement (level_index = level - 1)
        env = LS20RealWrapper(level_index=level - 1)
        
        # Créer ou charger agent
        if transfer_from_level == 0:
            # From scratch
            self.agent = MAGENSimpleAgent(
                n_actions=4,  # UP, DOWN, LEFT, RIGHT
                learning_rate=0.1,
                gamma=0.99,
                epsilon=0.995
            )
            print("✅ Agent créé FROM SCRATCH")
        else:
            # Transfer learning: charger checkpoint niveau précédent
            checkpoint_path = self.checkpoint_dir / f"level_{transfer_from_level}_final.pkl"
            if checkpoint_path.exists():
                with open(checkpoint_path, 'rb') as f:
                    checkpoint = pickle.load(f)
                self.agent.q_table = checkpoint['q_table']
                # Réinitialiser epsilon pour exploration du nouveau niveau
                self.agent.epsilon = 0.5  # Exploration modérée
                print(f"✅ Checkpoint niveau {transfer_from_level} chargé")
                print(f"   Q-table size: {len(self.agent.q_table)} états")
                print(f"   Epsilon réinitialisé: {self.agent.epsilon}")
            else:
                print(f"⚠️  Checkpoint niveau {transfer_from_level} introuvable, from scratch")
                self.agent = MAGENARCAgent(action_space=4)
        
        # Métriques niveau
        total_steps = 0
        total_reward = 0.0
        successes = 0
        best_reward = float('-inf')
        unique_states = set()
        
        start_time = time.time()
        
        # Boucle d'entraînement
        for episode in range(1, num_episodes + 1):
            obs = env.reset()
            done = False
            episode_reward = 0.0
            episode_steps = 0
            
            while not done and episode_steps < 50:
                # Sélectionner action
                action = self.agent.select_action(obs)
                
                # Exécuter action
                next_obs, reward, done, info = env.step(action)
                
                # Apprendre
                self.agent.learn(obs, action, reward, next_obs, done)
                
                # Tracking
                state_hash = self.agent.hash_state(obs)
                unique_states.add(state_hash)
                
                # Mise à jour
                obs = next_obs
                episode_reward += reward
                episode_steps += 1
                total_steps += 1
            
            # Statistiques épisode
            total_reward += episode_reward
            if episode_reward > 0:  # Victoire
                successes += 1
            if episode_reward > best_reward:
                best_reward = episode_reward
            
            # Logs périodiques
            if episode % 10 == 0:
                avg_reward = total_reward / episode
                success_rate = (successes / episode) * 100
                print(f"Episode {episode}/{num_episodes}: "
                      f"reward={episode_reward:.2f}, "
                      f"steps={episode_steps}, "
                      f"success_rate={success_rate:.1f}%, "
                      f"epsilon={self.agent.epsilon:.3f}, "
                      f"unique_states={len(unique_states)}")
        
        training_time = time.time() - start_time
        
        # Calculer métriques finales
        metrics = LevelMetrics(
            level=level,
            episodes=num_episodes,
            total_steps=total_steps,
            unique_states=len(unique_states),
            success_rate=(successes / num_episodes) * 100,
            avg_reward=total_reward / num_episodes,
            avg_steps_per_episode=total_steps / num_episodes,
            best_reward=best_reward,
            training_time=training_time,
            transfer_from_level=transfer_from_level
        )
        
        # Sauvegarder checkpoint
        checkpoint_path = self.checkpoint_dir / f"level_{level}_final.pkl"
        with open(checkpoint_path, 'wb') as f:
            pickle.dump({
                'level': level,
                'q_table': self.agent.q_table,
                'epsilon': self.agent.epsilon,
                'metrics': asdict(metrics)
            }, f)
        print(f"\n✅ Checkpoint sauvegardé: {checkpoint_path}")
        
        # Pas de méthode close() dans LS20RealWrapper
        # L'environnement sera garbage collected
        
        return metrics
    
    def run_full_training(self) -> TransferLearningReport:
        """
        Exécute l'entraînement complet sur tous les niveaux.
        
        Returns:
            TransferLearningReport: Rapport complet
        """
        print("\n" + "="*60)
        print("MAGEN MULTI-LEVEL TRANSFER LEARNING")
        print("="*60)
        print(f"Jeu: {self.game_id}")
        print(f"Niveaux: 7")
        print(f"Épisodes par niveau: {self.episodes_per_level}")
        print(f"Total épisodes: {sum(self.episodes_per_level)}")
        print("="*60 + "\n")
        
        total_start_time = time.time()
        
        # Niveau 1: From scratch
        metrics_level1 = self.train_level(
            level=1,
            num_episodes=self.episodes_per_level[0],
            transfer_from_level=0
        )
        self.level_metrics.append(metrics_level1)
        
        # Niveaux 2-7: Transfer learning
        for level in range(2, 8):
            metrics = self.train_level(
                level=level,
                num_episodes=self.episodes_per_level[level - 1],
                transfer_from_level=level - 1
            )
            self.level_metrics.append(metrics)
        
        total_training_time = time.time() - total_start_time
        
        # Calculer amélioration transfer learning
        transfer_improvement = {}
        for i in range(1, len(self.level_metrics)):
            prev_success = self.level_metrics[i-1].success_rate
            curr_success = self.level_metrics[i].success_rate
            if prev_success > 0:
                improvement = ((curr_success - prev_success) / prev_success) * 100
            else:
                improvement = 0.0
            transfer_improvement[self.level_metrics[i].level] = improvement
        
        # Score de généralisation (moyenne success rate tous niveaux)
        generalization_score = float(np.mean([m.success_rate for m in self.level_metrics]))
        
        # Créer rapport
        report = TransferLearningReport(
            game_id=self.game_id,
            total_levels=7,
            total_episodes=sum(self.episodes_per_level),
            total_training_time=total_training_time,
            level_metrics=self.level_metrics,
            transfer_improvement=transfer_improvement,
            generalization_score=generalization_score
        )
        
        # Sauvegarder rapport
        self._save_report(report)
        
        # Afficher résumé
        self._print_summary(report)
        
        return report
    
    def _save_report(self, report: TransferLearningReport):
        """Sauvegarde le rapport en JSON."""
        report_path = self.results_dir / "transfer_learning_report.json"
        
        # Convertir en dict
        report_dict = {
            'game_id': report.game_id,
            'total_levels': report.total_levels,
            'total_episodes': report.total_episodes,
            'total_training_time': report.total_training_time,
            'level_metrics': [asdict(m) for m in report.level_metrics],
            'transfer_improvement': report.transfer_improvement,
            'generalization_score': report.generalization_score
        }
        
        with open(report_path, 'w') as f:
            json.dump(report_dict, f, indent=2)
        
        print(f"\n✅ Rapport sauvegardé: {report_path}")
    
    def _print_summary(self, report: TransferLearningReport):
        """Affiche le résumé du rapport."""
        print("\n" + "="*60)
        print("RÉSUMÉ TRANSFER LEARNING MULTI-NIVEAUX")
        print("="*60)
        
        print(f"\n📊 Métriques Globales:")
        print(f"  - Total niveaux: {report.total_levels}")
        print(f"  - Total épisodes: {report.total_episodes}")
        print(f"  - Temps total: {report.total_training_time:.1f}s ({report.total_training_time/60:.1f}min)")
        print(f"  - Score généralisation: {report.generalization_score:.2f}%")
        
        print(f"\n📈 Métriques par Niveau:")
        print(f"{'Niveau':<8} {'Episodes':<10} {'Success%':<12} {'Avg Reward':<12} {'States':<10} {'Transfer':<15}")
        print("-" * 80)
        
        for metrics in report.level_metrics:
            transfer_info = "FROM SCRATCH" if metrics.transfer_from_level == 0 else f"From L{metrics.transfer_from_level}"
            print(f"{metrics.level:<8} {metrics.episodes:<10} "
                  f"{metrics.success_rate:<12.2f} {metrics.avg_reward:<12.2f} "
                  f"{metrics.unique_states:<10} {transfer_info:<15}")
        
        print(f"\n🚀 Amélioration Transfer Learning:")
        for level, improvement in report.transfer_improvement.items():
            symbol = "📈" if improvement > 0 else "📉" if improvement < 0 else "➡️"
            print(f"  {symbol} Niveau {level}: {improvement:+.1f}%")
        
        print("\n" + "="*60)


def main():
    """Point d'entrée principal."""
    # Configuration
    trainer = MultiLevelTrainer(
        game_id="ls20-9607627b",
        episodes_per_level=[100, 50, 50, 50, 50, 50, 50],  # Total: 400 épisodes
        checkpoint_dir="./checkpoints_multilevel",
        results_dir="./results_multilevel"
    )
    
    # Lancer entraînement complet
    report = trainer.run_full_training()
    
    print("\n✅ Entraînement multi-niveaux terminé avec succès!")
    print(f"📊 Score généralisation: {report.generalization_score:.2f}%")
    print(f"📁 Checkpoints: {trainer.checkpoint_dir}")
    print(f"📁 Résultats: {trainer.results_dir}")


if __name__ == "__main__":
    main()

# Made with Bob
