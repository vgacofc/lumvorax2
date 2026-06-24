"""
GOAL GROUNDER - MAGEN V24
Hiérarchisation conséquences utiles pour progression objectifs

PROBLÈME V23: Information Gain récompense nouveauté, pas progression
SOLUTION V24: Goal Progress priorise actions augmentant reward

Architecture:
- Goal Detection: Identifier objectifs via changements reward
- Action Prioritization: Classer actions par utilité
- Abandonment Policy: Éliminer actions inutiles (>20 essais, reward=0)
- Strategy Learning: Mémoriser séquences gagnantes

FORENSIC: Intégré via ForensicMiddleware (PROTOCOLE_MAGEN V3.0)
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass, field
from collections import defaultdict, deque
import time

from .forensic_middleware import ForensicMiddleware, CausalIDManager

@dataclass
class GoalProgress:
    """Progression vers objectif"""
    action: str
    reward_before: float
    reward_after: float
    reward_delta: float
    levels_before: int
    levels_after: int
    levels_delta: int
    timestamp_ns: int
    success: bool  # True si progression positive
    
    def __post_init__(self):
        """Calcul automatique success"""
        self.success = self.reward_delta > 0 or self.levels_delta > 0


@dataclass
class ActionPriority:
    """Priorité action basée sur historique"""
    action: str
    total_attempts: int = 0
    successful_attempts: int = 0
    total_reward_gained: float = 0.0
    avg_reward_per_attempt: float = 0.0
    last_success_timestamp_ns: int = 0
    consecutive_failures: int = 0
    priority_score: float = 0.0
    status: str = "ACTIVE"  # ACTIVE, PROMISING, ABANDONED
    
    def update(self, progress: GoalProgress):
        """Mise à jour priorité après tentative"""
        self.total_attempts += 1
        
        if progress.success:
            self.successful_attempts += 1
            self.total_reward_gained += progress.reward_delta
            self.last_success_timestamp_ns = progress.timestamp_ns
            self.consecutive_failures = 0
        else:
            self.consecutive_failures += 1
        
        # Calcul avg reward
        if self.total_attempts > 0:
            self.avg_reward_per_attempt = self.total_reward_gained / self.total_attempts
        
        # Calcul priority score
        self._compute_priority_score()
        
        # Mise à jour status
        self._update_status()
    
    def _compute_priority_score(self):
        """
        Score priorité combinant:
        - Success rate (40%)
        - Avg reward (40%)
        - Recency (20%)
        """
        if self.total_attempts == 0:
            self.priority_score = 0.5  # Neutre pour actions non testées
            return
        
        # Success rate component
        success_rate = self.successful_attempts / self.total_attempts
        success_component = success_rate * 0.4
        
        # Reward component (normalisé [0, 1])
        reward_component = min(1.0, self.avg_reward_per_attempt) * 0.4
        
        # Recency component (décroissance exponentielle)
        time_since_success_ns = time.perf_counter_ns() - self.last_success_timestamp_ns
        time_since_success_s = time_since_success_ns / 1e9
        recency_component = np.exp(-time_since_success_s / 10.0) * 0.2  # Decay 10s
        
        self.priority_score = success_component + reward_component + recency_component
    
    def _update_status(self):
        """Mise à jour status basé sur performance"""
        if self.consecutive_failures > 20:
            self.status = "ABANDONED"
        elif self.avg_reward_per_attempt > 0.1:
            self.status = "PROMISING"
        else:
            self.status = "ACTIVE"


class GoalGrounder:
    """
    Grounding actions → objectifs environnementaux
    
    Remplace exploration aveugle (V23) par exploration guidée objectif (V24)
    """
    
    def __init__(
        self,
        abandonment_threshold: int = 20,
        promising_threshold: float = 0.1,
        history_size: int = 100,
        forensic_log_path: Optional[str] = None,
        id_manager: Optional[CausalIDManager] = None
    ):
        self.abandonment_threshold = abandonment_threshold
        self.promising_threshold = promising_threshold
        self.history_size = history_size
        
        # Priorités actions
        self.action_priorities: Dict[str, ActionPriority] = {}
        
        # Historique progression
        self.progress_history: deque = deque(maxlen=history_size)
        
        # Objectifs détectés
        self.detected_goals: Set[str] = set()
        
        # Séquences gagnantes
        self.winning_sequences: List[List[str]] = []
        self.current_sequence: List[str] = []
        
        # Statistiques forensiques
        self.stats = {
            'total_progressions': 0,
            'successful_progressions': 0,
            'total_reward_gained': 0.0,
            'total_levels_completed': 0,
            'actions_abandoned': 0,
            'actions_promising': 0,
            'winning_sequences_found': 0,
            'avg_priority_score': 0.0
        }
        
        # FORENSIC: Middleware standardisé (PROTOCOLE_MAGEN V3.0)
        self.forensic: Optional[ForensicMiddleware] = None
        if forensic_log_path:
            self.forensic = ForensicMiddleware(
                forensic_log_path,
                "GoalGrounder",
                id_manager
            )
        
        print("[GOAL GROUNDER] Initialisé")
        print(f"  Abandonment threshold: {abandonment_threshold} échecs consécutifs")
        print(f"  Promising threshold: {promising_threshold} reward/attempt")
        print(f"  History size: {history_size}")
    
    def record_progress(
        self,
        action: str,
        reward_before: float,
        reward_after: float,
        levels_before: int,
        levels_after: int
    ) -> GoalProgress:
        """
        Enregistre progression après action
        
        Args:
            action: Action exécutée
            reward_before: Reward avant action
            reward_after: Reward après action
            levels_before: Niveaux avant action
            levels_after: Niveaux après action
        
        Returns:
            GoalProgress avec métadonnées
        """
        # Créer progression
        progress = GoalProgress(
            action=action,
            reward_before=reward_before,
            reward_after=reward_after,
            reward_delta=reward_after - reward_before,
            levels_before=levels_before,
            levels_after=levels_after,
            levels_delta=levels_after - levels_before,
            timestamp_ns=time.perf_counter_ns(),
            success=False  # Calculé dans __post_init__
        )
        
        # Ajouter à historique
        self.progress_history.append(progress)
        
        # Mise à jour priorité action
        if action not in self.action_priorities:
            self.action_priorities[action] = ActionPriority(action=action)
        
        self.action_priorities[action].update(progress)
        
        # Mise à jour séquence courante
        self.current_sequence.append(action)
        
        # Détecter objectif si progression
        if progress.success:
            self._detect_goal(progress)
            
            # Séquence gagnante si niveau complété
            if progress.levels_delta > 0:
                self.winning_sequences.append(self.current_sequence.copy())
                self.stats['winning_sequences_found'] += 1
                
                # FORENSIC: Log winning sequence
                if self.forensic:
                    self.forensic.log_event('winning_sequence_found', {
                        'timestamp_ns': int(time.perf_counter_ns()),
                        'sequence': self.current_sequence.copy(),
                        'sequence_length': len(self.current_sequence),
                        'levels_completed': int(progress.levels_delta),
                        'total_sequences': self.stats['winning_sequences_found']
                    })
                
                self.current_sequence = []  # Reset séquence
        
        # Statistiques forensiques
        self.stats['total_progressions'] += 1
        if progress.success:
            self.stats['successful_progressions'] += 1
            self.stats['total_reward_gained'] += progress.reward_delta
            self.stats['total_levels_completed'] += progress.levels_delta
        
        # FORENSIC: Log progress recorded
        if self.forensic:
            self.forensic.log_event('progress_recorded', {
                'timestamp_ns': int(progress.timestamp_ns),
                'action': action,
                'reward_delta': float(progress.reward_delta),
                'levels_delta': int(progress.levels_delta),
                'success': bool(progress.success),
                'total_progressions': self.stats['total_progressions']
            })
        
        # Log forensique
        if self.stats['total_progressions'] % 10 == 0:
            self._log_progress_stats()
        
        return progress
    
    def get_prioritized_actions(self, available_actions: List[str]) -> List[Tuple[str, float]]:
        """
        Retourne actions triées par priorité
        
        Args:
            available_actions: Actions disponibles
        
        Returns:
            Liste (action, priority_score) triée décroissant
        """
        prioritized = []
        
        for action in available_actions:
            if action not in self.action_priorities:
                # Action non testée: priorité neutre
                prioritized.append((action, 0.5))
            else:
                priority = self.action_priorities[action]
                
                # Ignorer actions abandonnées
                if priority.status == "ABANDONED":
                    # FORENSIC: Log abandoned action skipped
                    if self.forensic:
                        self.forensic.log_event('action_abandoned_skipped', {
                            'timestamp_ns': int(time.perf_counter_ns()),
                            'action': action,
                            'consecutive_failures': int(priority.consecutive_failures),
                            'total_attempts': int(priority.total_attempts)
                        })
                    continue
                
                prioritized.append((action, priority.priority_score))
        
        # Trier par score décroissant
        prioritized.sort(key=lambda x: x[1], reverse=True)
        
        return prioritized
    
    def should_abandon_action(self, action: str) -> bool:
        """
        Vérifie si action doit être abandonnée
        
        Critères:
        - >20 échecs consécutifs
        - Reward moyen < 0
        - Aucun succès depuis >100 tentatives
        """
        if action not in self.action_priorities:
            return False
        
        priority = self.action_priorities[action]
        
        return priority.status == "ABANDONED"
    
    def get_promising_actions(self) -> List[str]:
        """Retourne actions prometteuses (reward > threshold)"""
        promising = []
        
        for action, priority in self.action_priorities.items():
            if priority.status == "PROMISING":
                promising.append(action)
        
        return promising
    
    def get_abandoned_actions(self) -> List[str]:
        """Retourne actions abandonnées"""
        abandoned = []
        
        for action, priority in self.action_priorities.items():
            if priority.status == "ABANDONED":
                abandoned.append(action)
        
        return abandoned
    
    def _detect_goal(self, progress: GoalProgress):
        """
        Détecte objectif basé sur progression
        
        Heuristiques:
        - Augmentation levels → objectif "complete_level"
        - Augmentation reward sans level → objectif "collect_points"
        """
        goal_detected = None
        
        if progress.levels_delta > 0:
            self.detected_goals.add("complete_level")
            goal_detected = "complete_level"
        elif progress.reward_delta > 0:
            self.detected_goals.add("collect_points")
            goal_detected = "collect_points"
        
        # FORENSIC: Log goal detection
        if goal_detected and self.forensic:
            self.forensic.log_event('goal_detected', {
                'timestamp_ns': int(time.perf_counter_ns()),
                'goal_type': goal_detected,
                'action': progress.action,
                'reward_delta': float(progress.reward_delta),
                'levels_delta': int(progress.levels_delta),
                'total_goals': len(self.detected_goals)
            })
    
    def _log_progress_stats(self):
        """Log statistiques progression"""
        if self.stats['total_progressions'] == 0:
            return
        
        success_rate = self.stats['successful_progressions'] / self.stats['total_progressions']
        
        # Calcul avg priority score
        if self.action_priorities:
            avg_priority = np.mean([p.priority_score for p in self.action_priorities.values()])
            self.stats['avg_priority_score'] = avg_priority
        
        # Compter statuts
        self.stats['actions_abandoned'] = sum(
            1 for p in self.action_priorities.values() if p.status == "ABANDONED"
        )
        self.stats['actions_promising'] = sum(
            1 for p in self.action_priorities.values() if p.status == "PROMISING"
        )
        
        print(f"[GOAL GROUNDER] Progression #{self.stats['total_progressions']}: "
              f"success_rate={success_rate:.2%}, "
              f"reward_gained={self.stats['total_reward_gained']:.2f}, "
              f"levels={self.stats['total_levels_completed']}, "
              f"promising={self.stats['actions_promising']}, "
              f"abandoned={self.stats['actions_abandoned']}")
    
    def get_stats(self) -> Dict:
        """Statistiques forensiques"""
        stats = self.stats.copy()
        
        # Ajouter détails actions
        stats['action_details'] = {}
        for action, priority in self.action_priorities.items():
            stats['action_details'][action] = {
                'attempts': priority.total_attempts,
                'successes': priority.successful_attempts,
                'reward': priority.total_reward_gained,
                'avg_reward': priority.avg_reward_per_attempt,
                'priority_score': priority.priority_score,
                'status': priority.status,
                'consecutive_failures': priority.consecutive_failures
            }
        
        # Ajouter objectifs détectés
        stats['detected_goals'] = list(self.detected_goals)
        
        # Ajouter séquences gagnantes
        stats['winning_sequences'] = self.winning_sequences
        
        return stats
    
    def print_stats(self):
        """Affiche statistiques forensiques"""
        stats = self.get_stats()
        
        print("\n[GOAL GROUNDER] Statistiques:")
        print(f"  Total progressions: {stats['total_progressions']}")
        print(f"  Successful progressions: {stats['successful_progressions']}")
        print(f"  Total reward gained: {stats['total_reward_gained']:.2f}")
        print(f"  Total levels completed: {stats['total_levels_completed']}")
        print(f"  Actions promising: {stats['actions_promising']}")
        print(f"  Actions abandoned: {stats['actions_abandoned']}")
        print(f"  Winning sequences found: {stats['winning_sequences_found']}")
        print(f"  Avg priority score: {stats['avg_priority_score']:.3f}")
        
        if stats['detected_goals']:
            print(f"  Detected goals: {', '.join(stats['detected_goals'])}")
        
        if stats['action_details']:
            print(f"\n  Action details:")
            for action, details in sorted(
                stats['action_details'].items(),
                key=lambda x: x[1]['priority_score'],
                reverse=True
            ):
                print(f"    {action}: priority={details['priority_score']:.3f}, "
                      f"attempts={details['attempts']}, "
                      f"successes={details['successes']}, "
                      f"status={details['status']}")

# Made with Bob
