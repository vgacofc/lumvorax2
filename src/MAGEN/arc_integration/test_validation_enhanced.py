#!/usr/bin/env python3
"""
MAGEN - Test Validation Agent Enhanced vs Simple
=================================================

Compare les performances de l'agent enhanced (Layer 0 + Reward Shaper)
avec l'agent simple sur 20 épisodes.

Author: MAGEN Team
Date: 2026-06-19
Version: 1.0
"""

import sys
import os
import numpy as np
import json
from datetime import datetime
from typing import Dict, Any, List
import logging

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Imports MAGEN
from arc_integration.ls20_real_wrapper import LS20RealWrapper
from arc_integration.magen_arc_agent_simple import MAGENSimpleAgent
from arc_integration.magen_arc_agent_enhanced import MAGENEnhancedAgent

# Configuration logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class ValidationTester:
    """Testeur validation comparatif"""
    
    def __init__(self, num_episodes: int = 20, max_steps: int = 100):
        """
        Initialiser testeur
        
        Args:
            num_episodes: Nombre d'épisodes de test
            max_steps: Nombre max de steps par épisode
        """
        self.num_episodes = num_episodes
        self.max_steps = max_steps
        
        # Créer environnement (niveau 1 par défaut)
        self.env = LS20RealWrapper()
        
        # Créer agents
        self.agent_simple = MAGENSimpleAgent(
            epsilon=0.1,  # Exploitation (peu d'exploration)
            epsilon_decay=0.99
        )
        
        self.agent_enhanced = MAGENEnhancedAgent(
            epsilon=0.1,  # Exploitation (peu d'exploration)
            epsilon_decay=0.99,
            use_layer0=True,
            use_reward_shaper=True
        )
        
        # Résultats
        self.results_simple: List[Dict[str, Any]] = []
        self.results_enhanced: List[Dict[str, Any]] = []
    
    def test_agent(self, agent, agent_name: str) -> List[Dict[str, Any]]:
        """
        Tester un agent sur num_episodes épisodes
        
        Args:
            agent: Agent à tester
            agent_name: Nom de l'agent
            
        Returns:
            Liste des résultats par épisode
        """
        logger.info(f"\n{'='*60}")
        logger.info(f"TEST AGENT: {agent_name}")
        logger.info(f"{'='*60}")
        
        results = []
        
        for episode in range(self.num_episodes):
            logger.info(f"\nÉpisode {episode + 1}/{self.num_episodes}")
            
            # Exécuter épisode selon type d'agent
            if agent_name == "Enhanced":
                # Agent Enhanced a train_episode()
                agent.env = self.env
                result = agent.train_episode(max_steps=self.max_steps)
            else:
                # Agent Simple nécessite boucle manuelle
                result = self._run_simple_episode(agent, episode)
            
            # Ajouter métadonnées
            result['agent'] = agent_name
            result['episode_num'] = episode + 1
            
            results.append(result)
            
            # Log résumé
            logger.info(f"  Reward: {result['reward']:.2f}")
            logger.info(f"  Steps: {result['steps']}")
            logger.info(f"  Victory: {result['victory']}")
            
            if agent_name == "Enhanced":
                logger.info(f"  Avatar identifié: {result.get('world_metrics', {}).get('avatar_identified', False)}")
                logger.info(f"  Positions trackées: {result.get('world_metrics', {}).get('positions_tracked', 0)}")
        
        return results
    
    def _run_simple_episode(self, agent, episode_num: int) -> Dict[str, Any]:
        """
        Exécuter un épisode avec agent simple (boucle manuelle)
        
        Args:
            agent: Agent simple
            episode_num: Numéro épisode
            
        Returns:
            Résultats épisode
        """
        # Reset environnement
        obs = self.env.reset()
        
        # Extraire state
        if isinstance(obs, tuple):
            state = obs[0]
        elif hasattr(obs, 'frame'):
            state = obs.frame  # type: ignore
        else:
            state = obs
        
        episode_reward = 0.0
        episode_steps = 0
        done = False
        victory = False
        
        for step in range(self.max_steps):
            # Sélectionner action
            action = agent.select_action(state, training=True)
            
            # Exécuter action
            next_obs = self.env.step(action)
            
            # Extraire next_state, reward, done
            if isinstance(next_obs, tuple):
                next_state, reward, done, info = next_obs
            elif hasattr(next_obs, 'frame'):
                next_state = next_obs.frame  # type: ignore
                reward = next_obs.reward if hasattr(next_obs, 'reward') else 0.0
                done = next_obs.done if hasattr(next_obs, 'done') else False
            else:
                next_state = next_obs
                reward = 0.0
                done = False
            
            # Apprentissage
            agent.learn(state, action, reward, next_state, done)
            
            # Mise à jour
            state = next_state
            episode_reward += reward
            episode_steps += 1
            
            # Vérifier victoire
            if reward > 0:
                victory = True
                break
            
            if done:
                break
        
        # Fin épisode
        agent.end_episode(episode_reward, episode_steps)
        
        return {
            'episode': episode_num + 1,
            'reward': episode_reward,
            'steps': episode_steps,
            'victory': victory
        }
    
    def compute_statistics(self, results: List[Dict[str, Any]]) -> Dict[str, Any]:
        """
        Calculer statistiques sur résultats
        
        Args:
            results: Liste des résultats
            
        Returns:
            Dictionnaire de statistiques
        """
        rewards = [r['reward'] for r in results]
        steps = [r['steps'] for r in results]
        victories = [r['victory'] for r in results]
        
        stats = {
            'num_episodes': len(results),
            'total_victories': sum(victories),
            'victory_rate': sum(victories) / len(victories) * 100,
            'mean_reward': np.mean(rewards),
            'std_reward': np.std(rewards),
            'min_reward': np.min(rewards),
            'max_reward': np.max(rewards),
            'mean_steps': np.mean(steps),
            'std_steps': np.std(steps),
            'min_steps': np.min(steps),
            'max_steps': np.max(steps)
        }
        
        # Statistiques spécifiques agent enhanced
        if results and 'world_metrics' in results[0]:
            world_metrics = [r.get('world_metrics', {}) for r in results]
            stats['avatar_identified_rate'] = sum(
                m.get('avatar_identified', False) for m in world_metrics
            ) / len(world_metrics) * 100
            stats['mean_positions_tracked'] = np.mean([
                m.get('positions_tracked', 0) for m in world_metrics
            ])
            stats['mean_avatar_confidence'] = np.mean([
                m.get('avatar_confidence', 0.0) for m in world_metrics
            ])
        
        return stats
    
    def compare_agents(self) -> Dict[str, Any]:
        """
        Comparer les deux agents
        
        Returns:
            Dictionnaire de comparaison
        """
        stats_simple = self.compute_statistics(self.results_simple)
        stats_enhanced = self.compute_statistics(self.results_enhanced)
        
        comparison = {
            'simple': stats_simple,
            'enhanced': stats_enhanced,
            'improvements': {
                'victory_rate': stats_enhanced['victory_rate'] - stats_simple['victory_rate'],
                'mean_reward': stats_enhanced['mean_reward'] - stats_simple['mean_reward'],
                'mean_steps': stats_enhanced['mean_steps'] - stats_simple['mean_steps']
            }
        }
        
        return comparison
    
    def run_validation(self) -> Dict[str, Any]:
        """
        Exécuter validation complète
        
        Returns:
            Résultats complets
        """
        logger.info("\n" + "="*60)
        logger.info("DÉMARRAGE VALIDATION COMPARATIVE")
        logger.info("="*60)
        logger.info(f"Nombre d'épisodes: {self.num_episodes}")
        logger.info(f"Max steps par épisode: {self.max_steps}")
        
        # Tester agent simple
        self.results_simple = self.test_agent(self.agent_simple, "Simple")
        
        # Tester agent enhanced
        self.results_enhanced = self.test_agent(self.agent_enhanced, "Enhanced")
        
        # Comparer
        comparison = self.compare_agents()
        
        # Créer rapport
        report = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'num_episodes': self.num_episodes,
                'max_steps': self.max_steps,
                'level': 1
            },
            'results_simple': self.results_simple,
            'results_enhanced': self.results_enhanced,
            'comparison': comparison
        }
        
        return report
    
    def save_report(self, report: Dict[str, Any], filepath: str):
        """
        Sauvegarder rapport JSON
        
        Args:
            report: Rapport à sauvegarder
            filepath: Chemin fichier
        """
        # Convertir numpy types en types Python natifs
        def convert_numpy(obj):
            if isinstance(obj, np.integer):
                return int(obj)
            elif isinstance(obj, np.floating):
                return float(obj)
            elif isinstance(obj, np.ndarray):
                return obj.tolist()
            elif isinstance(obj, dict):
                return {k: convert_numpy(v) for k, v in obj.items()}
            elif isinstance(obj, list):
                return [convert_numpy(item) for item in obj]
            return obj
        
        report_converted = convert_numpy(report)
        
        with open(filepath, 'w') as f:
            json.dump(report_converted, f, indent=2)
        logger.info(f"\nRapport sauvegardé: {filepath}")
    
    def print_summary(self, comparison: Dict[str, Any]):
        """
        Afficher résumé comparaison
        
        Args:
            comparison: Dictionnaire de comparaison
        """
        logger.info("\n" + "="*60)
        logger.info("RÉSUMÉ COMPARAISON")
        logger.info("="*60)
        
        simple = comparison['simple']
        enhanced = comparison['enhanced']
        improvements = comparison['improvements']
        
        logger.info("\n📊 AGENT SIMPLE:")
        logger.info(f"  Victoires: {simple['total_victories']}/{simple['num_episodes']} ({simple['victory_rate']:.1f}%)")
        logger.info(f"  Reward moyen: {simple['mean_reward']:.2f} ± {simple['std_reward']:.2f}")
        logger.info(f"  Steps moyen: {simple['mean_steps']:.1f} ± {simple['std_steps']:.1f}")
        
        logger.info("\n🚀 AGENT ENHANCED:")
        logger.info(f"  Victoires: {enhanced['total_victories']}/{enhanced['num_episodes']} ({enhanced['victory_rate']:.1f}%)")
        logger.info(f"  Reward moyen: {enhanced['mean_reward']:.2f} ± {enhanced['std_reward']:.2f}")
        logger.info(f"  Steps moyen: {enhanced['mean_steps']:.1f} ± {enhanced['std_steps']:.1f}")
        
        if 'avatar_identified_rate' in enhanced:
            logger.info(f"  Avatar identifié: {enhanced['avatar_identified_rate']:.1f}%")
            logger.info(f"  Positions trackées: {enhanced['mean_positions_tracked']:.1f}")
            logger.info(f"  Confiance avatar: {enhanced['mean_avatar_confidence']:.2f}")
        
        logger.info("\n📈 AMÉLIORATIONS:")
        logger.info(f"  Victory rate: {improvements['victory_rate']:+.1f}%")
        logger.info(f"  Reward moyen: {improvements['mean_reward']:+.2f}")
        logger.info(f"  Steps moyen: {improvements['mean_steps']:+.1f}")
        
        # Verdict
        logger.info("\n🎯 VERDICT:")
        if improvements['victory_rate'] > 0:
            logger.info("  ✅ Agent Enhanced SUPÉRIEUR (plus de victoires)")
        elif improvements['mean_reward'] > 0.1:
            logger.info("  ✅ Agent Enhanced SUPÉRIEUR (meilleur reward)")
        elif abs(improvements['victory_rate']) < 0.1 and abs(improvements['mean_reward']) < 0.1:
            logger.info("  ⚖️  Agents ÉQUIVALENTS (performances similaires)")
        else:
            logger.info("  ❌ Agent Enhanced INFÉRIEUR (nécessite ajustements)")


def main():
    """Fonction principale"""
    
    # Créer testeur
    tester = ValidationTester(num_episodes=20, max_steps=100)
    
    # Exécuter validation
    report = tester.run_validation()
    
    # Afficher résumé
    tester.print_summary(report['comparison'])
    
    # Sauvegarder rapport
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = f"validation_report_{timestamp}.json"
    tester.save_report(report, report_path)
    
    logger.info("\n✅ VALIDATION TERMINÉE")


if __name__ == "__main__":
    main()

# Made with Bob
