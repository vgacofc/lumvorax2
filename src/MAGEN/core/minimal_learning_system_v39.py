on"""
Minimal Learning System V39 - Système d'Apprentissage Minimal FONCTIONNEL
==========================================================================

Ce module implémente un système d'apprentissage minimal qui FONCTIONNE réellement.
Il corrige le problème identifié en PHASE 1 V39: scores figés à 0.5.

Architecture:
- Scores d'actions mis à jour après chaque tentative
- Poids du decision kernel ajustés par gradient descent
- Mémoire des transformations réussies
- Forensic logging complet

Formule d'apprentissage:
    new_score = old_score + learning_rate * (reward - old_score)
    new_weight = old_weight + learning_rate * gradient

Auteur: Bob (IA)
Date: 2026-06-16
Version: V39.0 (Correction PHASE 1)
"""

import json
import time
import numpy as np
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from collections import defaultdict, deque
from dataclasses import dataclass, asdict


@dataclass
class ActionScore:
    """Score d'une action avec historique."""
    action_id: str
    score: float = 0.5  # Score initial neutre
    attempts: int = 0
    successes: int = 0
    total_reward: float = 0.0
    history: List[float] = None  # Historique scores
    
    def __post_init__(self):
        if self.history is None:
            self.history = [self.score]
    
    def to_dict(self) -> dict:
        return {
            'action_id': self.action_id,
            'score': float(self.score),
            'attempts': self.attempts,
            'successes': self.successes,
            'total_reward': float(self.total_reward),
            'success_rate': self.successes / self.attempts if self.attempts > 0 else 0.0,
            'avg_reward': self.total_reward / self.attempts if self.attempts > 0 else 0.0,
            'history_length': len(self.history)
        }


@dataclass
class WeightState:
    """État des poids du decision kernel."""
    weight_id: str
    value: float = 0.5  # Valeur initiale neutre
    gradient_sum: float = 0.0
    update_count: int = 0
    history: List[float] = None
    
    def __post_init__(self):
        if self.history is None:
            self.history = [self.value]
    
    def to_dict(self) -> dict:
        return {
            'weight_id': self.weight_id,
            'value': float(self.value),
            'gradient_sum': float(self.gradient_sum),
            'update_count': self.update_count,
            'history_length': len(self.history)
        }


class MinimalLearningSystemV39:
    """
    Système d'apprentissage minimal qui FONCTIONNE.
    
    Capacités:
    - Mise à jour scores actions (gradient descent simple)
    - Mise à jour poids decision kernel
    - Mémoire transformations réussies
    - Forensic logging complet
    - Vérification changements (assert)
    """
    
    def __init__(
        self,
        learning_rate: float = 0.01,
        weight_learning_rate: float = 0.005,
        forensic_log_path: Optional[str] = None,
        history_size: int = 100
    ):
        """
        Initialise le système d'apprentissage.
        
        Args:
            learning_rate: Taux d'apprentissage pour scores actions
            weight_learning_rate: Taux d'apprentissage pour poids
            forensic_log_path: Chemin du log forensic
            history_size: Taille historique scores/poids
        """
        self.learning_rate = learning_rate
        self.weight_learning_rate = weight_learning_rate
        self.history_size = history_size
        
        # Scores actions (mémoire décisionnelle)
        self.action_scores: Dict[str, ActionScore] = {}
        
        # Poids decision kernel
        self.weights: Dict[str, WeightState] = {
            'pattern_match': WeightState('pattern_match', 0.5),
            'color_similarity': WeightState('color_similarity', 0.5),
            'shape_match': WeightState('shape_match', 0.5)
        }
        
        # Mémoire transformations réussies
        self.successful_transforms: Dict[str, List[dict]] = defaultdict(list)
        
        # Statistiques
        self.total_updates = 0
        self.total_score_changes = 0
        self.total_weight_changes = 0
        
        # Forensic
        self.forensic_log_path = forensic_log_path
        if forensic_log_path:
            Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
                
        # Curriculum learning phase
        self.curriculum_phase = "exploration"  # Phases: exploration, exploitation, mastery
        
        self._log_forensic('system_initialized', {
            'learning_rate': learning_rate,
            'weight_learning_rate': weight_learning_rate,
            'history_size': history_size
        })
    
    def _log_forensic(self, event_type: str, data: dict):
        """Log événement forensic."""
        if not self.forensic_log_path:
            return
        
        event = {
            'timestamp': time.time(),
            'module': 'MinimalLearningSystemV39',
            'event_type': event_type,
            'data': data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def update_action_score(
        self,
        action_id: str,
        reward: float,
        success: bool = False
    ) -> float:
        """
        Met à jour le score d'une action (APPRENTISSAGE RÉEL).
        
        Formule: new_score = old_score + lr * (reward - old_score)
        
        Args:
            action_id: Identifiant de l'action
            reward: Récompense obtenue (0.0 à 1.0)
            success: Si l'action a réussi
            
        Returns:
            Nouveau score
        """
        # Créer score si première fois
        if action_id not in self.action_scores:
            self.action_scores[action_id] = ActionScore(action_id=action_id)
        
        score_obj = self.action_scores[action_id]
        old_score = score_obj.score
        
        # Gradient descent simple
        error = reward - old_score
        new_score = old_score + self.learning_rate * error
        
        # Clipper entre 0 et 1
        new_score = np.clip(new_score, 0.0, 1.0)
        
        # Mettre à jour
        score_obj.score = new_score
        score_obj.attempts += 1
        if success:
            score_obj.successes += 1
        score_obj.total_reward += reward
        score_obj.history.append(new_score)
        
        # Limiter taille historique
        if len(score_obj.history) > self.history_size:
            score_obj.history.pop(0)
        
        # Statistiques
        self.total_updates += 1
        if abs(new_score - old_score) > 1e-6:
            self.total_score_changes += 1
        
        # VÉRIFICATION CRITIQUE: Le score DOIT changer
        assert new_score != old_score or reward == old_score, \
            f"Score n'a pas changé! old={old_score}, new={new_score}, reward={reward}"
        
        # Forensic
        self._log_forensic('action_score_updated', {
            'action_id': action_id,
            'old_score': float(old_score),
            'new_score': float(new_score),
            'reward': float(reward),
            'delta': float(new_score - old_score),
            'success': success,
            'attempts': score_obj.attempts
        })
        
        return new_score
    
    def update_weight(
        self,
        weight_id: str,
        gradient: float
    ) -> float:
        """
        Met à jour un poids du decision kernel.
        
        Formule: new_weight = old_weight + lr * gradient
        
        Args:
            weight_id: Identifiant du poids
            gradient: Gradient calculé
            
        Returns:
            Nouvelle valeur du poids
        """
        if weight_id not in self.weights:
            self.weights[weight_id] = WeightState(weight_id=weight_id)
        
        weight_obj = self.weights[weight_id]
        old_value = weight_obj.value
        
        # Gradient descent
        new_value = old_value + self.weight_learning_rate * gradient
        
        # Clipper entre 0 et 1
        new_value = np.clip(new_value, 0.0, 1.0)
        
        # Mettre à jour
        weight_obj.value = new_value
        weight_obj.gradient_sum += gradient
        weight_obj.update_count += 1
        weight_obj.history.append(new_value)
        
        # Limiter taille historique
        if len(weight_obj.history) > self.history_size:
            weight_obj.history.pop(0)
        
        # Statistiques
        if abs(new_value - old_value) > 1e-6:
            self.total_weight_changes += 1
        
        # VÉRIFICATION CRITIQUE: Le poids DOIT changer
        assert new_value != old_value or gradient == 0.0, \
            f"Poids n'a pas changé! old={old_value}, new={new_value}, gradient={gradient}"
        
        # Forensic
        self._log_forensic('weight_updated', {
            'weight_id': weight_id,
            'old_value': float(old_value),
            'new_value': float(new_value),
            'gradient': float(gradient),
            'delta': float(new_value - old_value),
            'update_count': weight_obj.update_count
        })
        
        return new_value
    
    def record_successful_transform(
        self,
        transform_name: str,
        input_state: np.ndarray,
        output_state: np.ndarray,
        parameters: Optional[dict] = None
    ):
        """
        Enregistre une transformation réussie (mémoire causale).
        
        Args:
            transform_name: Nom de la transformation
            input_state: État avant transformation
            output_state: État après transformation
            parameters: Paramètres de la transformation
        """
        record = {
            'timestamp': time.time(),
            'transform_name': transform_name,
            'input_shape': input_state.shape,
            'output_shape': output_state.shape,
            'parameters': parameters or {}
        }
        
        self.successful_transforms[transform_name].append(record)
        
        # Limiter taille mémoire (garder 50 dernières)
        if len(self.successful_transforms[transform_name]) > 50:
            self.successful_transforms[transform_name].pop(0)
        
        # Forensic
        self._log_forensic('successful_transform_recorded', {
            'transform_name': transform_name,
            'total_successes': len(self.successful_transforms[transform_name])
        })
    
    def get_action_score(self, action_id: str) -> float:
        """Obtenir le score actuel d'une action."""
        if action_id not in self.action_scores:
            return 0.5  # Score neutre par défaut
        return self.action_scores[action_id].score
    
    def get_weight(self, weight_id: str) -> float:
        """Obtenir la valeur actuelle d'un poids."""
        if weight_id not in self.weights:
            return 0.5  # Valeur neutre par défaut
        return self.weights[weight_id].value
    
    def get_statistics(self) -> dict:
        """Obtenir statistiques d'apprentissage."""
        total_actions = len(self.action_scores)
        total_weights = len(self.weights)
        
        # Calculer taux de changement
        change_rate_scores = (
            self.total_score_changes / self.total_updates
            if self.total_updates > 0 else 0.0
        )
        
        change_rate_weights = (
            self.total_weight_changes / self.total_updates
            if self.total_updates > 0 else 0.0
        )
        
        # Calculer variance scores (preuve d'apprentissage)
        score_values = [s.score for s in self.action_scores.values()]
        score_variance = float(np.var(score_values)) if score_values else 0.0
        
        # Calculer variance poids
        weight_values = [w.value for w in self.weights.values()]
        weight_variance = float(np.var(weight_values)) if weight_values else 0.0
        
        stats = {
            'total_actions': total_actions,
            'total_weights': total_weights,
            'total_updates': self.total_updates,
            'total_score_changes': self.total_score_changes,
            'total_weight_changes': self.total_weight_changes,
            'change_rate_scores': change_rate_scores,
            'change_rate_weights': change_rate_weights,
            'score_variance': score_variance,
            'weight_variance': weight_variance,
            'successful_transforms': len(self.successful_transforms),
            'learning_functional': (
                self.total_score_changes > 0 or
                self.total_weight_changes > 0
            )
        }
        
        # Forensic
        self._log_forensic('statistics_computed', stats)
        
        return stats
    
    def get_action_scores_dict(self) -> Dict[str, float]:
        """Obtenir dictionnaire scores actions (pour audit)."""
        return {
            action_id: score_obj.score
            for action_id, score_obj in self.action_scores.items()
        }
    
    def get_weights_dict(self) -> Dict[str, float]:
        """Obtenir dictionnaire poids (pour audit)."""
        return {
            weight_id: weight_obj.value
            for weight_id, weight_obj in self.weights.items()
        }
    
    def reset(self):
        """Réinitialiser le système (pour tests)."""
        self.action_scores.clear()
        for weight_id in self.weights:
            self.weights[weight_id] = WeightState(weight_id=weight_id, value=0.5)
        self.successful_transforms.clear()
        self.total_updates = 0
        self.total_score_changes = 0
        self.total_weight_changes = 0
        
        self._log_forensic('system_reset', {})


# Tests unitaires
if __name__ == "__main__":
    import tempfile
    
    print("=== Test MinimalLearningSystemV39 ===\n")
    
    # Créer système
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    
    system = MinimalLearningSystemV39(
        learning_rate=0.1,  # Plus élevé pour tests
        weight_learning_rate=0.05,
        forensic_log_path=log_path
    )
    
    # Test 1: Mise à jour scores
    print("Test 1: Mise à jour scores actions")
    print(f"Score initial rotate_90: {system.get_action_score('rotate_90')}")
    
    new_score = system.update_action_score('rotate_90', reward=0.8, success=True)
    print(f"Score après reward=0.8: {new_score}")
    assert new_score != 0.5, "Score n'a pas changé!"
    
    new_score = system.update_action_score('rotate_90', reward=0.9, success=True)
    print(f"Score après reward=0.9: {new_score}")
    print("✓ Scores changent correctement\n")
    
    # Test 2: Mise à jour poids
    print("Test 2: Mise à jour poids")
    print(f"Poids initial pattern_match: {system.get_weight('pattern_match')}")
    
    new_weight = system.update_weight('pattern_match', gradient=0.2)
    print(f"Poids après gradient=0.2: {new_weight}")
    assert new_weight != 0.5, "Poids n'a pas changé!"
    
    new_weight = system.update_weight('pattern_match', gradient=-0.1)
    print(f"Poids après gradient=-0.1: {new_weight}")
    print("✓ Poids changent correctement\n")
    
    # Test 3: Mémoire transformations
    print("Test 3: Mémoire transformations")
    input_grid = np.array([[1, 0], [0, 1]])
    output_grid = np.array([[0, 1], [1, 0]])
    system.record_successful_transform('flip_h', input_grid, output_grid)
    print(f"Transformations réussies: {len(system.successful_transforms)}")
    print("✓ Mémoire transformations fonctionne\n")
    
    # Test 4: Statistiques
    print("Test 4: Statistiques")
    stats = system.get_statistics()
    print(f"Total updates: {stats['total_updates']}")
    print(f"Score changes: {stats['total_score_changes']}")
    print(f"Weight changes: {stats['total_weight_changes']}")
    print(f"Learning functional: {stats['learning_functional']}")
    assert stats['learning_functional'], "Apprentissage non fonctionnel!"
    print("✓ Apprentissage fonctionnel\n")
    
    # Test 5: Vérifier variance (preuve d'apprentissage)
    print("Test 5: Variance scores (preuve d'apprentissage)")
    system.update_action_score('identity', reward=0.2, success=False)
    system.update_action_score('flip_v', reward=0.7, success=True)
    stats = system.get_statistics()
    print(f"Score variance: {stats['score_variance']:.6f}")
    assert stats['score_variance'] > 0.0, "Aucune variance = pas d'apprentissage!"
    print("✓ Variance > 0 = apprentissage actif\n")
    
    print("=== TOUS LES TESTS RÉUSSIS ===")
    print(f"Forensic log: {log_path}")

# Made with Bob