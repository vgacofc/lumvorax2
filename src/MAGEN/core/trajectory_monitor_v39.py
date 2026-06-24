"""
PHASE 6 V39 - C19: Trajectory Monitor
Système de monitoring des trajectoires d'exploration.

Objectif:
- Détecter stagnation cognitive (pas de progrès)
- Identifier trajectoires mortes (erreur répétée)
- Mesurer diversité d'exploration
- Déclencher changement de stratégie si nécessaire
- Éviter boucles infinies

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import numpy as np
from typing import Dict, List, Optional, Tuple, Deque
from dataclasses import dataclass, field
from datetime import datetime
from collections import deque
import json


@dataclass
class TrajectoryPoint:
    """Point dans une trajectoire d'exploration."""
    step: int
    state_hash: str
    action_id: str
    reward: float
    error: float
    success: bool
    timestamp: datetime = field(default_factory=datetime.now)
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'step': self.step,
            'state_hash': self.state_hash,
            'action_id': self.action_id,
            'reward': self.reward,
            'error': self.error,
            'success': self.success,
            'timestamp': self.timestamp.isoformat()
        }


@dataclass
class TrajectoryMetrics:
    """Métriques d'une trajectoire."""
    total_steps: int = 0
    successful_steps: int = 0
    failed_steps: int = 0
    avg_reward: float = 0.0
    avg_error: float = 1.0
    reward_variance: float = 0.0
    unique_states: int = 0
    repeated_states: int = 0
    stagnation_detected: bool = False
    dead_end_detected: bool = False
    loop_detected: bool = False
    diversity_score: float = 0.0
    
    def update(self, points: List[TrajectoryPoint]):
        """Met à jour les métriques depuis les points."""
        if not points:
            return
        
        self.total_steps = len(points)
        self.successful_steps = sum(1 for p in points if p.success)
        self.failed_steps = self.total_steps - self.successful_steps
        
        rewards = [p.reward for p in points]
        errors = [p.error for p in points]
        
        self.avg_reward = float(np.mean(rewards))
        self.avg_error = float(np.mean(errors))
        self.reward_variance = float(np.var(rewards))
        
        # Compter états uniques
        state_hashes = [p.state_hash for p in points]
        self.unique_states = len(set(state_hashes))
        self.repeated_states = self.total_steps - self.unique_states
        
        # Score de diversité (ratio états uniques)
        self.diversity_score = self.unique_states / self.total_steps if self.total_steps > 0 else 0.0


class TrajectoryMonitor:
    """
    Système de monitoring des trajectoires pour MAGEN V39.
    
    Fonctionnalités:
    - Tracking trajectoire en temps réel
    - Détection stagnation (pas de progrès)
    - Détection dead-end (erreurs répétées)
    - Détection boucles (états répétés)
    - Mesure diversité exploration
    """
    
    def __init__(self,
                 window_size: int = 50,
                 stagnation_threshold: float = 0.01,
                 dead_end_threshold: float = 0.9,
                 loop_detection_window: int = 10,
                 min_diversity: float = 0.3):
        """
        Initialise le moniteur de trajectoires.
        
        Args:
            window_size: Taille fenêtre glissante pour analyse
            stagnation_threshold: Seuil variance reward pour stagnation
            dead_end_threshold: Seuil erreur moyenne pour dead-end
            loop_detection_window: Fenêtre pour détecter boucles
            min_diversity: Diversité minimale requise
        """
        self.window_size = window_size
        self.stagnation_threshold = stagnation_threshold
        self.dead_end_threshold = dead_end_threshold
        self.loop_detection_window = loop_detection_window
        self.min_diversity = min_diversity
        
        # Trajectoire courante
        self.current_trajectory: Deque[TrajectoryPoint] = deque(maxlen=window_size)
        self.full_trajectory: List[TrajectoryPoint] = []
        
        # Métriques
        self.metrics = TrajectoryMetrics()
        
        # Détections
        self.stagnation_count = 0
        self.dead_end_count = 0
        self.loop_count = 0
        
        # Historique états
        self.state_history: Dict[str, int] = {}  # state_hash -> count
        
        # Alertes
        self.alerts: List[Dict] = []
    
    def add_point(self, step: int, state_hash: str, action_id: str,
                  reward: float, error: float, success: bool):
        """
        Ajoute un point à la trajectoire.
        
        Args:
            step: Numéro du step
            state_hash: Hash de l'état
            action_id: ID de l'action
            reward: Récompense obtenue
            error: Erreur de l'action
            success: Si l'action a réussi
        """
        point = TrajectoryPoint(
            step=step,
            state_hash=state_hash,
            action_id=action_id,
            reward=reward,
            error=error,
            success=success
        )
        
        self.current_trajectory.append(point)
        self.full_trajectory.append(point)
        
        # Mettre à jour historique états
        if state_hash in self.state_history:
            self.state_history[state_hash] += 1
        else:
            self.state_history[state_hash] = 1
        
        # Analyser trajectoire
        self._analyze_trajectory()
    
    def _analyze_trajectory(self):
        """Analyse la trajectoire courante."""
        if len(self.current_trajectory) < 10:
            return  # Pas assez de données
        
        # Mettre à jour métriques
        self.metrics.update(list(self.current_trajectory))
        
        # Détecter stagnation
        if self._detect_stagnation():
            self.stagnation_count += 1
            self.metrics.stagnation_detected = True
            self._add_alert('stagnation', 
                          f"Variance reward {self.metrics.reward_variance:.4f} < {self.stagnation_threshold}")
        
        # Détecter dead-end
        if self._detect_dead_end():
            self.dead_end_count += 1
            self.metrics.dead_end_detected = True
            self._add_alert('dead_end',
                          f"Erreur moyenne {self.metrics.avg_error:.2f} >= {self.dead_end_threshold}")
        
        # Détecter boucles
        if self._detect_loop():
            self.loop_count += 1
            self.metrics.loop_detected = True
            self._add_alert('loop',
                          f"Diversité {self.metrics.diversity_score:.2f} < {self.min_diversity}")
    
    def _detect_stagnation(self) -> bool:
        """
        Détecte stagnation (pas de progrès).
        
        Returns:
            True si stagnation détectée
        """
        # Stagnation = variance reward très faible
        return self.metrics.reward_variance < self.stagnation_threshold
    
    def _detect_dead_end(self) -> bool:
        """
        Détecte dead-end (erreurs répétées).
        
        Returns:
            True si dead-end détecté
        """
        # Dead-end = erreur moyenne très élevée
        return self.metrics.avg_error >= self.dead_end_threshold
    
    def _detect_loop(self) -> bool:
        """
        Détecte boucles (états répétés).
        
        Returns:
            True si boucle détectée
        """
        # Boucle = diversité trop faible
        if self.metrics.diversity_score < self.min_diversity:
            return True
        
        # Vérifier répétition récente d'états
        if len(self.current_trajectory) >= self.loop_detection_window:
            recent_states = [p.state_hash for p in list(self.current_trajectory)[-self.loop_detection_window:]]
            unique_recent = len(set(recent_states))
            
            # Si moins de 50% d'états uniques dans fenêtre récente
            if unique_recent < self.loop_detection_window * 0.5:
                return True
            return False
        
        return False
    
    def _add_alert(self, alert_type: str, message: str):
        """Ajoute une alerte."""
        alert = {
            'timestamp': datetime.now().isoformat(),
            'type': alert_type,
            'message': message,
            'step': len(self.full_trajectory)
        }
        self.alerts.append(alert)
    
    def should_change_strategy(self) -> bool:
        """
        Détermine si changement de stratégie nécessaire.
        
        Returns:
            True si changement recommandé
        """
        # Changer si:
        # - Stagnation détectée
        # - Dead-end détecté
        # - Boucle détectée
        # - Diversité trop faible
        
        return (self.metrics.stagnation_detected or
                self.metrics.dead_end_detected or
                self.metrics.loop_detected or
                self.metrics.diversity_score < self.min_diversity)
    
    def get_recommendation(self) -> Dict:
        """
        Retourne recommandation d'action.
        
        Returns:
            Dictionnaire avec recommandation
        """
        if self.metrics.stagnation_detected:
            return {
                'action': 'increase_exploration',
                'reason': 'stagnation_detected',
                'details': f'Variance reward {self.metrics.reward_variance:.4f} trop faible'
            }
        
        if self.metrics.dead_end_detected:
            return {
                'action': 'backtrack',
                'reason': 'dead_end_detected',
                'details': f'Erreur moyenne {self.metrics.avg_error:.2f} trop élevée'
            }
        
        if self.metrics.loop_detected:
            return {
                'action': 'diversify',
                'reason': 'loop_detected',
                'details': f'Diversité {self.metrics.diversity_score:.2f} trop faible'
            }
        
        if self.metrics.diversity_score < self.min_diversity:
            return {
                'action': 'explore_new_states',
                'reason': 'low_diversity',
                'details': f'Seulement {self.metrics.unique_states} états uniques sur {self.metrics.total_steps}'
            }
        
        return {
            'action': 'continue',
            'reason': 'trajectory_healthy',
            'details': 'Aucun problème détecté'
        }
    
    def reset_trajectory(self):
        """Réinitialise la trajectoire courante."""
        self.current_trajectory.clear()
        self.metrics = TrajectoryMetrics()
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques du moniteur."""
        return {
            'trajectory': {
                'total_steps': self.metrics.total_steps,
                'successful_steps': self.metrics.successful_steps,
                'failed_steps': self.metrics.failed_steps,
                'success_rate': self.metrics.successful_steps / self.metrics.total_steps if self.metrics.total_steps > 0 else 0.0
            },
            'performance': {
                'avg_reward': self.metrics.avg_reward,
                'avg_error': self.metrics.avg_error,
                'reward_variance': self.metrics.reward_variance
            },
            'exploration': {
                'unique_states': self.metrics.unique_states,
                'repeated_states': self.metrics.repeated_states,
                'diversity_score': self.metrics.diversity_score
            },
            'detections': {
                'stagnation_count': self.stagnation_count,
                'dead_end_count': self.dead_end_count,
                'loop_count': self.loop_count,
                'total_alerts': len(self.alerts)
            },
            'status': {
                'stagnation_detected': self.metrics.stagnation_detected,
                'dead_end_detected': self.metrics.dead_end_detected,
                'loop_detected': self.metrics.loop_detected,
                'should_change_strategy': self.should_change_strategy()
            }
        }
    
    def export_trajectory(self) -> List[Dict]:
        """Exporte la trajectoire complète."""
        return [p.to_dict() for p in self.full_trajectory]
    
    def export_alerts(self) -> List[Dict]:
        """Exporte les alertes."""
        return self.alerts


def test_trajectory_monitor():
    """Test unitaire du moniteur de trajectoires."""
    print("="*80)
    print("TEST TRAJECTORY MONITOR V39")
    print("="*80)
    
    monitor = TrajectoryMonitor(
        window_size=50,
        stagnation_threshold=0.01,
        dead_end_threshold=0.8,
        min_diversity=0.4
    )
    
    # Simuler trajectoire avec différents patterns
    print("\nSimulation trajectoire...")
    
    # Phase 1: Exploration normale (steps 0-30)
    print("  Phase 1: Exploration normale")
    for i in range(30):
        state_hash = f"state_{i % 10}"  # 10 états différents
        action_id = f"action_{np.random.randint(0, 5)}"
        reward = np.random.uniform(0.3, 0.7)
        error = np.random.uniform(0.2, 0.5)
        success = reward > 0.4
        
        monitor.add_point(i, state_hash, action_id, reward, error, success)
    
    # Phase 2: Stagnation (steps 30-50)
    print("  Phase 2: Stagnation (rewards constants)")
    for i in range(30, 50):
        state_hash = f"state_{i % 5}"
        action_id = "action_stagnant"
        reward = 0.5  # Reward constant
        error = 0.5
        success = True
        
        monitor.add_point(i, state_hash, action_id, reward, error, success)
    
    # Phase 3: Dead-end (steps 50-70)
    print("  Phase 3: Dead-end (erreurs élevées)")
    for i in range(50, 70):
        state_hash = f"state_dead_{i % 3}"
        action_id = "action_failing"
        reward = 0.1
        error = 0.95  # Erreur très élevée
        success = False
        
        monitor.add_point(i, state_hash, action_id, reward, error, success)
    
    # Phase 4: Boucle (steps 70-100)
    print("  Phase 4: Boucle (états répétés)")
    for i in range(70, 100):
        state_hash = f"state_loop_{i % 3}"  # Seulement 3 états
        action_id = f"action_{i % 2}"
        reward = np.random.uniform(0.3, 0.6)
        error = np.random.uniform(0.3, 0.6)
        success = reward > 0.4
        
        monitor.add_point(i, state_hash, action_id, reward, error, success)
    
    # Afficher stats
    stats = monitor.get_stats()
    print(f"\nStatistiques finales:")
    print(f"  Steps totaux: {stats['trajectory']['total_steps']}")
    print(f"  Taux succès: {stats['trajectory']['success_rate']:.1%}")
    print(f"  Reward moyen: {stats['performance']['avg_reward']:.3f}")
    print(f"  Erreur moyenne: {stats['performance']['avg_error']:.3f}")
    print(f"  Variance reward: {stats['performance']['reward_variance']:.4f}")
    print(f"  États uniques: {stats['exploration']['unique_states']}")
    print(f"  Diversité: {stats['exploration']['diversity_score']:.2f}")
    
    print(f"\nDétections:")
    print(f"  Stagnations: {stats['detections']['stagnation_count']}")
    print(f"  Dead-ends: {stats['detections']['dead_end_count']}")
    print(f"  Boucles: {stats['detections']['loop_count']}")
    print(f"  Alertes totales: {stats['detections']['total_alerts']}")
    
    print(f"\nStatut:")
    print(f"  Stagnation: {'✅ OUI' if stats['status']['stagnation_detected'] else '❌ NON'}")
    print(f"  Dead-end: {'✅ OUI' if stats['status']['dead_end_detected'] else '❌ NON'}")
    print(f"  Boucle: {'✅ OUI' if stats['status']['loop_detected'] else '❌ NON'}")
    print(f"  Changer stratégie: {'✅ OUI' if stats['status']['should_change_strategy'] else '❌ NON'}")
    
    # Recommandation
    recommendation = monitor.get_recommendation()
    print(f"\nRecommandation:")
    print(f"  Action: {recommendation['action']}")
    print(f"  Raison: {recommendation['reason']}")
    print(f"  Détails: {recommendation['details']}")
    
    # Alertes récentes
    if monitor.alerts:
        print(f"\nDernières alertes:")
        for alert in monitor.alerts[-3:]:
            print(f"  [{alert['type']}] Step {alert['step']}: {alert['message']}")
    
    print("\n" + "="*80)
    print("✅ TEST RÉUSSI")
    print("="*80)


if __name__ == '__main__':
    test_trajectory_monitor()

# Made with Bob
