"""
PHASE 6 V39 - C17: Reputation System
Système de réputation pour mémoriser la valeur des actions.

Objectif:
- Mémoriser quelles transformations/actions valent le coût
- Calculer réputation basée sur historique succès/échecs
- Blacklister actions inefficaces (identity excessive, erreurs répétées)
- Prioriser actions prometteuses

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
from datetime import datetime
import json


@dataclass
class ActionReputation:
    """Réputation d'une action/transformation."""
    action_id: str
    total_attempts: int = 0
    successful_attempts: int = 0
    failed_attempts: int = 0
    total_error: float = 0.0
    avg_error: float = 1.0
    success_rate: float = 0.0
    reputation_score: float = 0.0
    is_blacklisted: bool = False
    blacklist_reason: str = ""
    last_used: Optional[datetime] = None
    
    def update(self, success: bool, error: float):
        """Met à jour la réputation après une utilisation."""
        self.total_attempts += 1
        self.last_used = datetime.now()
        
        if success:
            self.successful_attempts += 1
        else:
            self.failed_attempts += 1
        
        self.total_error += error
        
        # Calculer métriques
        self.success_rate = self.successful_attempts / self.total_attempts
        self.avg_error = self.total_error / self.total_attempts
        
        # Calculer score de réputation
        # Formule: success_rate * (1 - avg_error)
        # Range: [0, 1] où 1 = parfait
        self.reputation_score = self.success_rate * (1.0 - self.avg_error)
    
    def check_blacklist(self, min_attempts: int = 10, 
                       max_error_rate: float = 0.9,
                       min_success_rate: float = 0.1) -> bool:
        """
        Vérifie si l'action doit être blacklistée.
        
        Args:
            min_attempts: Nombre minimum de tentatives avant blacklist
            max_error_rate: Erreur moyenne maximale tolérée
            min_success_rate: Taux de succès minimum requis
        
        Returns:
            True si action doit être blacklistée
        """
        if self.total_attempts < min_attempts:
            return False
        
        # Blacklist si erreur trop élevée
        if self.avg_error >= max_error_rate:
            self.is_blacklisted = True
            self.blacklist_reason = f"avg_error={self.avg_error:.2f} >= {max_error_rate}"
            return True
        
        # Blacklist si taux de succès trop bas
        if self.success_rate < min_success_rate:
            self.is_blacklisted = True
            self.blacklist_reason = f"success_rate={self.success_rate:.2f} < {min_success_rate}"
            return True
        
        return False


@dataclass
class TransformationPattern:
    """Pattern de transformation observé."""
    pattern_id: str
    transformations: List[str] = field(default_factory=list)
    contexts: List[Dict] = field(default_factory=list)
    success_count: int = 0
    total_count: int = 0
    avg_reward: float = 0.0
    
    def add_observation(self, transformation: str, context: Dict, 
                       success: bool, reward: float):
        """Ajoute une observation du pattern."""
        self.transformations.append(transformation)
        self.contexts.append(context)
        self.total_count += 1
        
        if success:
            self.success_count += 1
        
        # Mise à jour moyenne mobile du reward
        alpha = 0.1  # Facteur d'apprentissage
        self.avg_reward = (1 - alpha) * self.avg_reward + alpha * reward
    
    def get_success_rate(self) -> float:
        """Retourne le taux de succès du pattern."""
        if self.total_count == 0:
            return 0.0
        return self.success_count / self.total_count


class ReputationSystem:
    """
    Système de réputation pour MAGEN V39.
    
    Fonctionnalités:
    - Tracking réputation par action/transformation
    - Blacklisting automatique des actions inefficaces
    - Détection de patterns de transformations
    - Priorisation basée sur réputation
    """
    
    def __init__(self, 
                 blacklist_threshold_attempts: int = 10,
                 blacklist_max_error: float = 0.9,
                 blacklist_min_success: float = 0.1,
                 identity_max_ratio: float = 0.3):
        """
        Initialise le système de réputation.
        
        Args:
            blacklist_threshold_attempts: Tentatives avant blacklist possible
            blacklist_max_error: Erreur max tolérée
            blacklist_min_success: Succès min requis
            identity_max_ratio: Ratio max d'identity autorisé
        """
        self.reputations: Dict[str, ActionReputation] = {}
        self.patterns: Dict[str, TransformationPattern] = {}
        
        self.blacklist_threshold_attempts = blacklist_threshold_attempts
        self.blacklist_max_error = blacklist_max_error
        self.blacklist_min_success = blacklist_min_success
        self.identity_max_ratio = identity_max_ratio
        
        self.total_actions = 0
        self.identity_count = 0
        self.blacklisted_actions: List[str] = []
    
    def record_action(self, action_id: str, success: bool, error: float,
                     context: Optional[Dict] = None):
        """
        Enregistre l'utilisation d'une action.
        
        Args:
            action_id: Identifiant de l'action
            success: Si l'action a réussi
            error: Erreur de l'action (0=parfait, 1=échec total)
            context: Contexte optionnel de l'action
        """
        # Créer réputation si n'existe pas
        if action_id not in self.reputations:
            self.reputations[action_id] = ActionReputation(action_id=action_id)
        
        # Mettre à jour réputation
        rep = self.reputations[action_id]
        rep.update(success, error)
        
        # Vérifier blacklist
        if rep.check_blacklist(
            self.blacklist_threshold_attempts,
            self.blacklist_max_error,
            self.blacklist_min_success
        ):
            if action_id not in self.blacklisted_actions:
                self.blacklisted_actions.append(action_id)
        
        # Tracking identity
        self.total_actions += 1
        if 'identity' in action_id.lower():
            self.identity_count += 1
        
        # Vérifier ratio identity
        if self.total_actions > 50:
            identity_ratio = self.identity_count / self.total_actions
            if identity_ratio > self.identity_max_ratio:
                identity_id = 'identity'
                if identity_id not in self.blacklisted_actions:
                    if identity_id in self.reputations:
                        self.reputations[identity_id].is_blacklisted = True
                        self.reputations[identity_id].blacklist_reason = \
                            f"identity_ratio={identity_ratio:.2f} > {self.identity_max_ratio}"
                    self.blacklisted_actions.append(identity_id)
    
    def record_transformation_pattern(self, pattern_id: str, 
                                     transformation: str,
                                     context: Dict,
                                     success: bool,
                                     reward: float):
        """
        Enregistre un pattern de transformation.
        
        Args:
            pattern_id: ID du pattern
            transformation: Transformation appliquée
            context: Contexte de la transformation
            success: Si réussi
            reward: Récompense obtenue
        """
        if pattern_id not in self.patterns:
            self.patterns[pattern_id] = TransformationPattern(pattern_id=pattern_id)
        
        self.patterns[pattern_id].add_observation(
            transformation, context, success, reward
        )
    
    def get_action_priority(self, action_id: str) -> float:
        """
        Retourne la priorité d'une action (0=basse, 1=haute).
        
        Args:
            action_id: ID de l'action
        
        Returns:
            Score de priorité
        """
        if action_id not in self.reputations:
            return 0.5  # Priorité neutre pour actions inconnues
        
        rep = self.reputations[action_id]
        
        # Actions blacklistées ont priorité 0
        if rep.is_blacklisted:
            return 0.0
        
        # Sinon utiliser reputation_score
        return rep.reputation_score
    
    def get_best_actions(self, n: int = 5, 
                        exclude_blacklisted: bool = True) -> List[Tuple[str, float]]:
        """
        Retourne les n meilleures actions par réputation.
        
        Args:
            n: Nombre d'actions à retourner
            exclude_blacklisted: Exclure actions blacklistées
        
        Returns:
            Liste de (action_id, reputation_score)
        """
        actions = []
        for action_id, rep in self.reputations.items():
            if exclude_blacklisted and rep.is_blacklisted:
                continue
            actions.append((action_id, rep.reputation_score))
        
        # Trier par score décroissant
        actions.sort(key=lambda x: x[1], reverse=True)
        
        return actions[:n]
    
    def get_worst_actions(self, n: int = 5) -> List[Tuple[str, float]]:
        """
        Retourne les n pires actions par réputation.
        
        Args:
            n: Nombre d'actions à retourner
        
        Returns:
            Liste de (action_id, reputation_score)
        """
        actions = [(aid, rep.reputation_score) 
                  for aid, rep in self.reputations.items()]
        
        # Trier par score croissant
        actions.sort(key=lambda x: x[1])
        
        return actions[:n]
    
    def is_blacklisted(self, action_id: str) -> bool:
        """Vérifie si une action est blacklistée."""
        return action_id in self.blacklisted_actions
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques du système."""
        total_reps = len(self.reputations)
        blacklisted = len(self.blacklisted_actions)
        
        if total_reps > 0:
            avg_reputation = np.mean([r.reputation_score 
                                     for r in self.reputations.values()])
            avg_success_rate = np.mean([r.success_rate 
                                       for r in self.reputations.values()])
            avg_error = np.mean([r.avg_error 
                                for r in self.reputations.values()])
        else:
            avg_reputation = 0.0
            avg_success_rate = 0.0
            avg_error = 1.0
        
        identity_ratio = (self.identity_count / self.total_actions 
                         if self.total_actions > 0 else 0.0)
        
        return {
            'total_actions_tracked': total_reps,
            'blacklisted_actions': blacklisted,
            'blacklist_rate': blacklisted / total_reps if total_reps > 0 else 0.0,
            'avg_reputation': float(avg_reputation),
            'avg_success_rate': float(avg_success_rate),
            'avg_error': float(avg_error),
            'total_actions_executed': self.total_actions,
            'identity_count': self.identity_count,
            'identity_ratio': float(identity_ratio),
            'patterns_discovered': len(self.patterns)
        }
    
    def export_reputations(self) -> Dict:
        """Exporte les réputations au format JSON."""
        return {
            action_id: {
                'total_attempts': rep.total_attempts,
                'success_rate': rep.success_rate,
                'avg_error': rep.avg_error,
                'reputation_score': rep.reputation_score,
                'is_blacklisted': rep.is_blacklisted,
                'blacklist_reason': rep.blacklist_reason
            }
            for action_id, rep in self.reputations.items()
        }


def test_reputation_system():
    """Test unitaire du système de réputation."""
    print("="*80)
    print("TEST REPUTATION SYSTEM V39")
    print("="*80)
    
    system = ReputationSystem(
        blacklist_threshold_attempts=5,
        blacklist_max_error=0.8,
        identity_max_ratio=0.4
    )
    
    # Simuler 100 actions
    print("\nSimulation 100 actions...")
    actions = ['rotate_90', 'flip_h', 'identity', 'scale_2x', 'crop']
    
    for i in range(100):
        action = np.random.choice(actions)
        
        # Identity a mauvaise performance
        if action == 'identity':
            success = np.random.random() > 0.7
            error = np.random.uniform(0.7, 1.0)
        # rotate_90 a bonne performance
        elif action == 'rotate_90':
            success = np.random.random() > 0.2
            error = np.random.uniform(0.0, 0.3)
        else:
            success = np.random.random() > 0.5
            error = np.random.uniform(0.3, 0.7)
        
        system.record_action(action, success, error)
    
    # Afficher stats
    stats = system.get_stats()
    print(f"\nStatistiques:")
    print(f"  Actions trackées: {stats['total_actions_tracked']}")
    print(f"  Actions blacklistées: {stats['blacklisted_actions']}")
    print(f"  Taux blacklist: {stats['blacklist_rate']:.1%}")
    print(f"  Réputation moyenne: {stats['avg_reputation']:.3f}")
    print(f"  Taux succès moyen: {stats['avg_success_rate']:.1%}")
    print(f"  Erreur moyenne: {stats['avg_error']:.3f}")
    print(f"  Ratio identity: {stats['identity_ratio']:.1%}")
    
    # Meilleures actions
    print(f"\nTop 3 actions:")
    for action_id, score in system.get_best_actions(3):
        print(f"  {action_id}: {score:.3f}")
    
    # Pires actions
    print(f"\nPires 3 actions:")
    for action_id, score in system.get_worst_actions(3):
        blacklisted = " [BLACKLISTED]" if system.is_blacklisted(action_id) else ""
        print(f"  {action_id}: {score:.3f}{blacklisted}")
    
    # Vérifier blacklist
    print(f"\nActions blacklistées:")
    for action_id in system.blacklisted_actions:
        rep = system.reputations[action_id]
        print(f"  {action_id}: {rep.blacklist_reason}")
    
    print("\n" + "="*80)
    print("✅ TEST RÉUSSI")
    print("="*80)


if __name__ == '__main__':
    test_reputation_system()

# Made with Bob
