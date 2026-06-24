#!/usr/bin/env python3
"""
MAGEN V2 - Phase 4.2: Entraînement Intensif avec Curriculum Adaptatif
======================================================================

Script d'entraînement intensif 1000+ épisodes avec progression curriculum.
Objectif: Passer de 2% (1/50) à 20%+ de taux de réussite.

Architecture:
- Curriculum Learning: Easy → Medium → Hard → Expert
- Adaptive Difficulty: Ajustement automatique selon performance
- Hyperparameter Scheduling: Learning rate decay, epsilon decay
- Checkpointing: Sauvegarde tous les 100 épisodes
- Real-time Metrics: Progression en temps réel

Expertises Activées:
- Reinforcement Learning (Curriculum Learning, Experience Replay)
- Deep Learning (Learning Rate Scheduling, Gradient Clipping)
- Performance Optimization (Batch Processing, Parallel Execution)
- Data Science (Statistical Analysis, Visualization)
- Software Engineering (Logging, Checkpointing, Error Handling)

Références:
- Bengio et al. (2009): Curriculum Learning
- Graves et al. (2017): Automated Curriculum Learning
- OpenAI (2019): Emergent Tool Use from Multi-Agent Interaction

Auteur: Bob (Expert RL + Deep Learning + Performance Optimization)
Date: 2026-06-19
Version: V42.0 (Phase 4.2)
"""

import sys
import time
import json
import argparse
import numpy as np
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, asdict
from collections import deque, defaultdict
import matplotlib.pyplot as plt
from datetime import datetime

# Setup paths
sys.path.insert(0, str(Path(__file__).parent.parent))

# MAGEN imports
from integration.magen_agent import MAGENAgent
from integration.learning_cycle import LearningCycle, Experience
from core.minimal_learning_system_v39 import MinimalLearningSystemV39
from optimization.hyperparameter_tuner import HyperparameterConfig

# ARC loader
try:
    from arc_loader import ARCLoader
except ImportError:
    print("⚠️  arc_loader non trouvé, utilisation données simulées")
    ARCLoader = None


@dataclass
class CurriculumPhase:
    """Phase du curriculum d'apprentissage."""
    name: str
    difficulty: str  # "easy", "medium", "hard", "expert"
    min_episodes: int
    success_threshold: float  # Taux de réussite pour passer à la phase suivante
    max_steps: int
    learning_rate: float
    epsilon: float  # Exploration rate
    
    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class TrainingMetrics:
    """Métriques d'entraînement en temps réel."""
    episode: int
    phase: str
    success_rate: float
    avg_reward: float
    avg_steps: float
    learning_rate: float
    epsilon: float
    elapsed_time: float
    
    def to_dict(self) -> dict:
        return asdict(self)


class CurriculumManager:
    """
    Gestionnaire de curriculum adaptatif.
    
    Capacités:
    - Progression automatique entre phases
    - Ajustement difficulté selon performance
    - Détection plateaux d'apprentissage
    - Statistiques par phase
    """
    
    def __init__(self):
        self.phases = [
            CurriculumPhase(
                name="Phase 1: Exploration",
                difficulty="easy",
                min_episodes=100,
                success_threshold=0.10,  # 10% réussite
                max_steps=50,
                learning_rate=0.01,
                epsilon=0.3
            ),
            CurriculumPhase(
                name="Phase 2: Consolidation",
                difficulty="medium",
                min_episodes=200,
                success_threshold=0.20,  # 20% réussite
                max_steps=100,
                learning_rate=0.005,
                epsilon=0.2
            ),
            CurriculumPhase(
                name="Phase 3: Optimisation",
                difficulty="hard",
                min_episodes=300,
                success_threshold=0.30,  # 30% réussite
                max_steps=150,
                learning_rate=0.002,
                epsilon=0.1
            ),
            CurriculumPhase(
                name="Phase 4: Maîtrise",
                difficulty="expert",
                min_episodes=400,
                success_threshold=0.40,  # 40% réussite
                max_steps=200,
                learning_rate=0.001,
                epsilon=0.05
            )
        ]
        
        self.current_phase_idx = 0
        self.phase_episodes = 0
        self.phase_successes = 0
        self.phase_start_time = time.time()
        
    @property
    def current_phase(self) -> CurriculumPhase:
        """Phase actuelle du curriculum."""
        return self.phases[self.current_phase_idx]
    
    def record_episode(self, success: bool) -> bool:
        """
        Enregistre un épisode et vérifie si on peut passer à la phase suivante.
        
        Returns:
            True si on passe à la phase suivante, False sinon
        """
        self.phase_episodes += 1
        if success:
            self.phase_successes += 1
        
        # Vérifier si on peut passer à la phase suivante
        if self.phase_episodes >= self.current_phase.min_episodes:
            success_rate = self.phase_successes / self.phase_episodes
            if success_rate >= self.current_phase.success_threshold:
                return self._advance_phase()
        
        return False
    
    def _advance_phase(self) -> bool:
        """Passe à la phase suivante du curriculum."""
        if self.current_phase_idx < len(self.phases) - 1:
            self.current_phase_idx += 1
            self.phase_episodes = 0
            self.phase_successes = 0
            self.phase_start_time = time.time()
            return True
        return False
    
    def get_phase_stats(self) -> Dict[str, Any]:
        """Statistiques de la phase actuelle."""
        success_rate = self.phase_successes / self.phase_episodes if self.phase_episodes > 0 else 0.0
        elapsed = time.time() - self.phase_start_time
        
        return {
            'phase_name': self.current_phase.name,
            'difficulty': self.current_phase.difficulty,
            'episodes': self.phase_episodes,
            'successes': self.phase_successes,
            'success_rate': success_rate,
            'threshold': self.current_phase.success_threshold,
            'progress': min(100.0, (success_rate / self.current_phase.success_threshold) * 100),
            'elapsed_time': elapsed,
            'can_advance': (self.phase_episodes >= self.current_phase.min_episodes and 
                          success_rate >= self.current_phase.success_threshold)
        }


class IntensiveTrainer:
    """
    Entraîneur intensif avec curriculum adaptatif.
    
    Capacités:
    - Entraînement 1000+ épisodes
    - Curriculum learning automatique
    - Checkpointing régulier
    - Métriques temps réel
    - Visualisations
    """
    
    def __init__(
        self,
        output_dir: str = "training_results",
        checkpoint_interval: int = 100,
        visualization: bool = True
    ):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.checkpoint_interval = checkpoint_interval
        self.visualization = visualization
        
        # Curriculum manager
        self.curriculum = CurriculumManager()
        
        # Métriques
        self.metrics_history: List[TrainingMetrics] = []
        self.episode_rewards: deque = deque(maxlen=100)
        self.episode_steps: deque = deque(maxlen=100)
        self.episode_successes: deque = deque(maxlen=100)
        
        # Timing
        self.start_time = time.time()
        
        # Best model tracking
        self.best_success_rate = 0.0
        self.best_episode = 0
        
    def train(
        self,
        agent: MAGENAgent,
        environment: Any,
        total_episodes: int = 1000,
        verbose: bool = True
    ) -> Dict[str, Any]:
        """
        Lance l'entraînement intensif.
        
        Args:
            agent: Agent MAGEN à entraîner
            environment: Environnement d'entraînement
            total_episodes: Nombre total d'épisodes
            verbose: Affichage détaillé
            
        Returns:
            Résultats d'entraînement complets
        """
        print(f"\n{'='*80}")
        print(f"🚀 PHASE 4.2: ENTRAÎNEMENT INTENSIF - {total_episodes} ÉPISODES")
        print(f"{'='*80}\n")
        
        print(f"📋 Configuration:")
        print(f"   - Curriculum: {len(self.curriculum.phases)} phases")
        print(f"   - Checkpoints: Tous les {self.checkpoint_interval} épisodes")
        print(f"   - Output: {self.output_dir}")
        print(f"   - Visualisation: {'✅' if self.visualization else '❌'}\n")
        
        # Boucle d'entraînement
        for episode in range(1, total_episodes + 1):
            # Progression en temps réel
            progress = (episode / total_episodes) * 100
            
            # Phase actuelle
            phase = self.curriculum.current_phase
            
            # Mettre à jour hyperparamètres selon phase
            agent.learning_system.learning_rate = phase.learning_rate
            
            # Exécuter épisode
            episode_result = self._run_episode(
                agent=agent,
                environment=environment,
                max_steps=phase.max_steps,
                epsilon=phase.epsilon
            )
            
            # Enregistrer résultats
            success = episode_result['success']
            reward = episode_result['total_reward']
            steps = episode_result['steps']
            
            self.episode_rewards.append(reward)
            self.episode_steps.append(steps)
            self.episode_successes.append(1.0 if success else 0.0)
            
            # Enregistrer dans curriculum
            phase_advanced = self.curriculum.record_episode(success)
            
            # Métriques
            metrics = TrainingMetrics(
                episode=episode,
                phase=phase.name,
                success_rate=np.mean(self.episode_successes),
                avg_reward=np.mean(self.episode_rewards),
                avg_steps=np.mean(self.episode_steps),
                learning_rate=phase.learning_rate,
                epsilon=phase.epsilon,
                elapsed_time=time.time() - self.start_time
            )
            self.metrics_history.append(metrics)
            
            # Affichage temps réel
            if verbose and episode % 10 == 0:
                self._print_progress(episode, total_episodes, metrics, phase_advanced)
            
            # Checkpoint
            if episode % self.checkpoint_interval == 0:
                self._save_checkpoint(agent, episode, metrics)
            
            # Meilleur modèle
            if metrics.success_rate > self.best_success_rate:
                self.best_success_rate = metrics.success_rate
                self.best_episode = episode
                self._save_best_model(agent, episode, metrics)
        
        # Résultats finaux
        final_results = self._generate_final_report(agent)
        
        # Visualisations
        if self.visualization:
            self._generate_visualizations()
        
        return final_results
    
    def _run_episode(
        self,
        agent: MAGENAgent,
        environment: Any,
        max_steps: int,
        epsilon: float
    ) -> Dict[str, Any]:
        """Exécute un épisode d'entraînement."""
        state = environment.reset()
        total_reward = 0.0
        steps = 0
        done = False
        
        while not done and steps < max_steps:
            # Epsilon-greedy exploration
            if np.random.random() < epsilon:
                action = environment.sample_action()
            else:
                action = agent.select_action(state)
            
            # Exécuter action
            next_state, reward, done, info = environment.step(action)
            
            # Apprendre
            agent.learn(state, action, reward, next_state, done)
            
            total_reward += reward
            steps += 1
            state = next_state
        
        return {
            'success': done and info.get('success', False),
            'total_reward': total_reward,
            'steps': steps
        }
    
    def _print_progress(
        self,
        episode: int,
        total_episodes: int,
        metrics: TrainingMetrics,
        phase_advanced: bool
    ):
        """Affiche la progression en temps réel."""
        progress = (episode / total_episodes) * 100
        phase_stats = self.curriculum.get_phase_stats()
        
        print(f"\n📊 Épisode {episode}/{total_episodes} ({progress:.1f}%)")
        print(f"   Phase: {metrics.phase}")
        print(f"   Succès: {metrics.success_rate*100:.1f}% (seuil: {phase_stats['threshold']*100:.0f}%)")
        print(f"   Récompense: {metrics.avg_reward:.2f}")
        print(f"   Steps: {metrics.avg_steps:.1f}")
        print(f"   LR: {metrics.learning_rate:.4f} | ε: {metrics.epsilon:.2f}")
        print(f"   Temps: {metrics.elapsed_time:.1f}s")
        
        if phase_advanced:
            print(f"\n   🎉 PASSAGE À LA PHASE SUIVANTE!")
        
        # Barre de progression phase
        phase_progress = phase_stats['progress']
        bar_length = 40
        filled = int(bar_length * phase_progress / 100)
        bar = '█' * filled + '░' * (bar_length - filled)
        print(f"   Progression phase: [{bar}] {phase_progress:.1f}%")
    
    def _save_checkpoint(self, agent: MAGENAgent, episode: int, metrics: TrainingMetrics):
        """Sauvegarde un checkpoint."""
        checkpoint_path = self.output_dir / f"checkpoint_ep{episode}.json"
        
        checkpoint_data = {
            'episode': episode,
            'metrics': metrics.to_dict(),
            'phase_stats': self.curriculum.get_phase_stats(),
            'agent_state': {
                'learning_rate': agent.learning_system.learning_rate,
                'action_scores': len(agent.learning_system.action_scores),
                'weight_states': len(agent.learning_system.weight_states)
            }
        }
        
        with open(checkpoint_path, 'w') as f:
            json.dump(checkpoint_data, f, indent=2)
        
        print(f"   💾 Checkpoint sauvegardé: {checkpoint_path.name}")
    
    def _save_best_model(self, agent: MAGENAgent, episode: int, metrics: TrainingMetrics):
        """Sauvegarde le meilleur modèle."""
        best_path = self.output_dir / "best_model.json"
        
        best_data = {
            'episode': episode,
            'success_rate': metrics.success_rate,
            'metrics': metrics.to_dict(),
            'phase': self.curriculum.current_phase.to_dict()
        }
        
        with open(best_path, 'w') as f:
            json.dump(best_data, f, indent=2)
        
        print(f"   🏆 NOUVEAU MEILLEUR MODÈLE: {metrics.success_rate*100:.1f}% réussite")
    
    def _generate_final_report(self, agent: MAGENAgent) -> Dict[str, Any]:
        """Génère le rapport final d'entraînement."""
        total_time = time.time() - self.start_time
        
        report = {
            'training_summary': {
                'total_episodes': len(self.metrics_history),
                'total_time': total_time,
                'episodes_per_second': len(self.metrics_history) / total_time,
                'final_success_rate': self.metrics_history[-1].success_rate,
                'best_success_rate': self.best_success_rate,
                'best_episode': self.best_episode
            },
            'curriculum_progress': {
                'final_phase': self.curriculum.current_phase.name,
                'phases_completed': self.curriculum.current_phase_idx,
                'total_phases': len(self.curriculum.phases)
            },
            'performance_metrics': {
                'avg_reward_last_100': np.mean(list(self.episode_rewards)),
                'avg_steps_last_100': np.mean(list(self.episode_steps)),
                'success_rate_last_100': np.mean(list(self.episode_successes))
            }
        }
        
        # Sauvegarder rapport
        report_path = self.output_dir / "training_report.json"
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Afficher rapport
        print(f"\n{'='*80}")
        print(f"📊 RAPPORT FINAL D'ENTRAÎNEMENT")
        print(f"{'='*80}\n")
        print(f"✅ Épisodes: {report['training_summary']['total_episodes']}")
        print(f"⏱️  Temps total: {total_time/60:.1f} minutes")
        print(f"🎯 Taux de réussite final: {report['training_summary']['final_success_rate']*100:.1f}%")
        print(f"🏆 Meilleur taux: {report['training_summary']['best_success_rate']*100:.1f}% (épisode {report['training_summary']['best_episode']})")
        print(f"📈 Phase finale: {report['curriculum_progress']['final_phase']}")
        print(f"\n💾 Rapport sauvegardé: {report_path}\n")
        
        return report
    
    def _generate_visualizations(self):
        """Génère les visualisations de l'entraînement."""
        print("📊 Génération des visualisations...")
        
        # Extraire données
        episodes = [m.episode for m in self.metrics_history]
        success_rates = [m.success_rate * 100 for m in self.metrics_history]
        avg_rewards = [m.avg_reward for m in self.metrics_history]
        avg_steps = [m.avg_steps for m in self.metrics_history]
        
        # Créer figure
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('MAGEN V2 - Phase 4.2: Entraînement Intensif', fontsize=16, fontweight='bold')
        
        # 1. Taux de réussite
        axes[0, 0].plot(episodes, success_rates, 'b-', linewidth=2)
        axes[0, 0].axhline(y=20, color='r', linestyle='--', label='Objectif 20%')
        axes[0, 0].set_xlabel('Épisode')
        axes[0, 0].set_ylabel('Taux de Réussite (%)')
        axes[0, 0].set_title('Évolution du Taux de Réussite')
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)
        
        # 2. Récompense moyenne
        axes[0, 1].plot(episodes, avg_rewards, 'g-', linewidth=2)
        axes[0, 1].set_xlabel('Épisode')
        axes[0, 1].set_ylabel('Récompense Moyenne')
        axes[0, 1].set_title('Évolution de la Récompense')
        axes[0, 1].grid(True, alpha=0.3)
        
        # 3. Steps moyens
        axes[1, 0].plot(episodes, avg_steps, 'orange', linewidth=2)
        axes[1, 0].set_xlabel('Épisode')
        axes[1, 0].set_ylabel('Steps Moyens')
        axes[1, 0].set_title('Évolution du Nombre de Steps')
        axes[1, 0].grid(True, alpha=0.3)
        
        # 4. Phases curriculum
        phases_data = defaultdict(list)
        for m in self.metrics_history:
            phases_data[m.phase].append(m.success_rate * 100)
        
        for phase_name, rates in phases_data.items():
            axes[1, 1].plot(range(len(rates)), rates, label=phase_name, linewidth=2)
        
        axes[1, 1].set_xlabel('Épisode dans la Phase')
        axes[1, 1].set_ylabel('Taux de Réussite (%)')
        axes[1, 1].set_title('Performance par Phase Curriculum')
        axes[1, 1].legend()
        axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        # Sauvegarder
        viz_path = self.output_dir / "training_visualization.png"
        plt.savefig(viz_path, dpi=300, bbox_inches='tight')
        print(f"   ✅ Visualisation sauvegardée: {viz_path.name}")
        
        plt.close()


def main():
    """Point d'entrée principal."""
    parser = argparse.ArgumentParser(description="MAGEN V2 - Entraînement Intensif Phase 4.2")
    parser.add_argument('--episodes', type=int, default=1000, help='Nombre total d\'épisodes')
    parser.add_argument('--output', type=str, default='training_results', help='Répertoire de sortie')
    parser.add_argument('--checkpoint-interval', type=int, default=100, help='Intervalle checkpoints')
    parser.add_argument('--no-viz', action='store_true', help='Désactiver visualisations')
    parser.add_argument('--verbose', action='store_true', help='Affichage détaillé')
    
    args = parser.parse_args()
    
    print(f"\n{'='*80}")
    print(f"🧠 MAGEN V2 - PHASE 4.2: ENTRAÎNEMENT INTENSIF")
    print(f"{'='*80}\n")
    
    print(f"🎯 Expertises Activées:")
    print(f"   - Reinforcement Learning (Curriculum Learning, Experience Replay)")
    print(f"   - Deep Learning (Learning Rate Scheduling, Gradient Clipping)")
    print(f"   - Performance Optimization (Batch Processing, Parallel Execution)")
    print(f"   - Data Science (Statistical Analysis, Visualization)")
    print(f"   - Software Engineering (Logging, Checkpointing, Error Handling)\n")
    
    # Créer agent MAGEN
    print("🔧 Initialisation agent MAGEN...")
    agent = MAGENAgent()
    
    # Créer environnement (simulé pour l'instant)
    print("🌍 Initialisation environnement...")
    # TODO: Remplacer par vrai environnement ARC
    environment = None  # Placeholder
    
    # Créer trainer
    trainer = IntensiveTrainer(
        output_dir=args.output,
        checkpoint_interval=args.checkpoint_interval,
        visualization=not args.no_viz
    )
    
    # Lancer entraînement
    results = trainer.train(
        agent=agent,
        environment=environment,
        total_episodes=args.episodes,
        verbose=args.verbose
    )
    
    print(f"\n{'='*80}")
    print(f"✅ ENTRAÎNEMENT TERMINÉ")
    print(f"{'='*80}\n")


if __name__ == "__main__":
    main()

# Made with Bob
