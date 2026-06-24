#!/usr/bin/env python3
"""
Layer 9: Prediction Engine
==========================

CŒUR du système d'apprentissage MAGEN V2.
Implémente le cycle complet: Prédire → Agir → Observer → Apprendre

Principe fondamental:
L'apprentissage émerge de la minimisation de l'erreur de prédiction.
Un agent qui prédit mieux comprend mieux, et agit mieux.

Basé sur:
- Free Energy Principle (Friston, 2010)
- Predictive Coding (Rao & Ballard, 1999)
- Temporal Difference Learning (Sutton & Barto, 2018)

Author: Bob (Assistant IA)
Date: 2026-06-18
Version: V2.0
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Any, Callable
from dataclasses import dataclass, field
from collections import deque
import json
import time

# Imports modules existants
try:
    from .state_transition_validator import StateTransitionValidator
    from .minimal_learning_system_v39 import MinimalLearningSystemV39
except ImportError:
    StateTransitionValidator = None
    MinimalLearningSystemV39 = None


@dataclass
class PredictionResult:
    """
    Résultat d'une prédiction.
    
    Contient l'état prédit, la confiance, et les métriques associées.
    """
    predicted_state: np.ndarray
    confidence: float  # 0-1
    prediction_time_ms: float
    method_used: str  # 'model', 'heuristic', 'random'
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict:
        return {
            'predicted_state_shape': self.predicted_state.shape,
            'confidence': self.confidence,
            'prediction_time_ms': self.prediction_time_ms,
            'method_used': self.method_used,
            'metadata': self.metadata
        }


@dataclass
class PredictionError:
    """
    Erreur de prédiction.
    
    Quantifie la différence entre prédit et observé.
    C'est le SIGNAL D'APPRENTISSAGE fondamental.
    """
    error_magnitude: float  # Magnitude totale
    error_map: np.ndarray  # Carte spatiale erreurs
    prediction_accuracy: float  # 0-1 (1 = parfait)
    surprise: float  # Surprise bayésienne
    timestamp: float
    
    def to_dict(self) -> Dict:
        return {
            'error_magnitude': float(self.error_magnitude),
            'error_map_shape': self.error_map.shape,
            'prediction_accuracy': float(self.prediction_accuracy),
            'surprise': float(self.surprise),
            'timestamp': self.timestamp
        }


@dataclass
class LearningSignal:
    """
    Signal d'apprentissage généré par erreur prédiction.
    
    Indique COMMENT et COMBIEN apprendre.
    """
    learning_rate: float  # Taux apprentissage adaptatif
    update_magnitude: float  # Magnitude mise à jour
    focus_regions: List[Tuple[int, int]]  # Régions à focaliser
    confidence_adjustment: float  # Ajustement confiance
    action_id: str  # Action concernée
    
    def to_dict(self) -> Dict:
        return {
            'learning_rate': self.learning_rate,
            'update_magnitude': self.update_magnitude,
            'num_focus_regions': len(self.focus_regions),
            'confidence_adjustment': self.confidence_adjustment,
            'action_id': self.action_id
        }


class PredictionEngine:
    """
    Layer 9: Prediction Engine
    
    CŒUR du système d'apprentissage MAGEN V2.
    
    Cycle complet:
    1. PRÉDIRE: Prédit état futur pour action donnée
    2. AGIR: Exécute action (externe)
    3. OBSERVER: Observe état réel
    4. COMPARER: Calcule erreur prédiction
    5. APPRENDRE: Met à jour modèle interne
    6. RÉPÉTER: Retour à étape 1
    
    Principe: Minimisation erreur prédiction = apprentissage
    """
    
    def __init__(
        self,
        base_learning_rate: float = 0.1,
        surprise_threshold: float = 0.5,
        confidence_decay: float = 0.95,
        history_size: int = 100,
        forensic_logger=None
    ):
        """
        Initialise le moteur de prédiction.
        
        Args:
            base_learning_rate: Taux apprentissage de base
            surprise_threshold: Seuil surprise pour adaptation
            confidence_decay: Decay confiance prédictions
            history_size: Taille historique prédictions
            forensic_logger: Logger forensique
        """
        self.base_learning_rate = base_learning_rate
        self.surprise_threshold = surprise_threshold
        self.confidence_decay = confidence_decay
        self.forensic_logger = forensic_logger
        
        # Modules intégrés
        if StateTransitionValidator:
            self.transition_validator = StateTransitionValidator()
        else:
            self.transition_validator = None
            
        if MinimalLearningSystemV39:
            self.learning_system = MinimalLearningSystemV39()
        else:
            self.learning_system = None
        
        # Modèle interne (transitions apprises)
        self.transition_model: Dict[str, List[Dict]] = {}
        
        # Historique prédictions
        self.prediction_history = deque(maxlen=history_size)
        self.error_history = deque(maxlen=history_size)
        
        # Statistiques
        self.total_predictions = 0
        self.correct_predictions = 0
        self.total_learning_updates = 0
        self.cumulative_error = 0.0
        
        # Confiance modèle par action
        self.action_confidence: Dict[str, float] = {}
        
        self._log_forensic("PREDICTION_ENGINE_INIT", {
            "base_learning_rate": base_learning_rate,
            "surprise_threshold": surprise_threshold,
            "confidence_decay": confidence_decay
        })
    
    def predict(
        self,
        state: np.ndarray,
        action: str,
        context: Optional[Dict] = None
    ) -> PredictionResult:
        """
        Prédit l'état futur après exécution d'une action.
        
        C'est la fonction CENTRALE du moteur.
        
        Args:
            state: État actuel
            action: Action à prédire
            context: Contexte additionnel
            
        Returns:
            PredictionResult avec état prédit et confiance
        """
        start_time = time.time()
        
        # Vérifier si modèle existe pour cette action
        if action in self.transition_model and len(self.transition_model[action]) > 0:
            # Prédiction basée modèle
            predicted_state, confidence = self._model_based_prediction(state, action)
            method = 'model'
        else:
            # Prédiction heuristique (fallback)
            predicted_state, confidence = self._heuristic_prediction(state, action)
            method = 'heuristic'
        
        prediction_time = (time.time() - start_time) * 1000
        
        result = PredictionResult(
            predicted_state=predicted_state,
            confidence=confidence,
            prediction_time_ms=prediction_time,
            method_used=method,
            metadata={'action': action, 'state_shape': state.shape}
        )
        
        # Enregistrer prédiction
        self.prediction_history.append({
            'state': state.copy(),
            'action': action,
            'predicted': predicted_state.copy(),
            'confidence': confidence,
            'timestamp': time.time()
        })
        
        self.total_predictions += 1
        
        self._log_forensic("PREDICTION_MADE", {
            "action": action,
            "confidence": confidence,
            "method": method,
            "prediction_time_ms": prediction_time
        })
        
        return result
    
    def compute_error(
        self,
        predicted_state: np.ndarray,
        observed_state: np.ndarray
    ) -> PredictionError:
        """
        Calcule l'erreur de prédiction.
        
        L'erreur est le SIGNAL D'APPRENTISSAGE fondamental.
        Plus l'erreur est grande, plus il faut apprendre.
        
        Args:
            predicted_state: État prédit
            observed_state: État observé (réel)
            
        Returns:
            PredictionError avec métriques détaillées
        """
        # Carte erreurs (différence absolue)
        error_map = np.abs(predicted_state - observed_state)
        
        # Magnitude erreur (moyenne)
        error_magnitude = np.mean(error_map)
        
        # Précision prédiction (1 - erreur normalisée)
        max_possible_error = np.max(np.abs(predicted_state)) + np.max(np.abs(observed_state))
        if max_possible_error > 0:
            prediction_accuracy = 1.0 - (error_magnitude / max_possible_error)
        else:
            prediction_accuracy = 1.0
        
        # Surprise bayésienne (erreur relative à confiance)
        # Si confiance haute et erreur haute → surprise haute
        recent_confidence = self.prediction_history[-1]['confidence'] if self.prediction_history else 0.5
        surprise = error_magnitude * (1.0 + recent_confidence)
        
        error = PredictionError(
            error_magnitude=error_magnitude,
            error_map=error_map,
            prediction_accuracy=max(0.0, prediction_accuracy),
            surprise=surprise,
            timestamp=time.time()
        )
        
        # Enregistrer erreur
        self.error_history.append(error)
        self.cumulative_error += error_magnitude
        
        # Mettre à jour statistiques
        if prediction_accuracy > 0.9:  # Seuil "correct"
            self.correct_predictions += 1
        
        self._log_forensic("PREDICTION_ERROR_COMPUTED", {
            "error_magnitude": error_magnitude,
            "prediction_accuracy": prediction_accuracy,
            "surprise": surprise
        })
        
        return error
    
    def learn(
        self,
        state_before: np.ndarray,
        action: str,
        state_after: np.ndarray,
        prediction_error: PredictionError
    ) -> LearningSignal:
        """
        Apprend de l'erreur de prédiction.
        
        Met à jour le modèle interne pour réduire erreur future.
        
        Args:
            state_before: État avant action
            action: Action exécutée
            state_after: État après action (observé)
            prediction_error: Erreur de prédiction
            
        Returns:
            LearningSignal indiquant mise à jour effectuée
        """
        # Calculer taux apprentissage adaptatif
        # Plus l'erreur est grande, plus on apprend vite
        adaptive_lr = self.base_learning_rate * (1.0 + prediction_error.surprise)
        adaptive_lr = min(adaptive_lr, 1.0)  # Cap à 1.0
        
        # Identifier régions à focaliser (erreur > moyenne)
        error_threshold = np.mean(prediction_error.error_map)
        focus_regions = []
        rows, cols = np.where(prediction_error.error_map > error_threshold)
        for r, c in zip(rows[:10], cols[:10]):  # Max 10 régions
            focus_regions.append((int(r), int(c)))
        
        # Mettre à jour modèle transition
        self._update_transition_model(state_before, action, state_after, adaptive_lr)
        
        # Ajuster confiance action
        confidence_adjustment = -prediction_error.error_magnitude * 0.1
        current_confidence = self.action_confidence.get(action, 0.5)
        self.action_confidence[action] = np.clip(
            current_confidence + confidence_adjustment,
            0.0, 1.0
        )
        
        # Mettre à jour learning system si disponible
        if self.learning_system:
            self.learning_system.update_action_score(
                action_id=action,
                reward=prediction_error.prediction_accuracy,
                success=prediction_error.prediction_accuracy > 0.8
            )
        
        self.total_learning_updates += 1
        
        signal = LearningSignal(
            learning_rate=adaptive_lr,
            update_magnitude=prediction_error.error_magnitude,
            focus_regions=focus_regions,
            confidence_adjustment=confidence_adjustment,
            action_id=action
        )
        
        self._log_forensic("LEARNING_UPDATE", {
            "action": action,
            "learning_rate": adaptive_lr,
            "update_magnitude": prediction_error.error_magnitude,
            "num_focus_regions": len(focus_regions),
            "new_confidence": self.action_confidence[action]
        })
        
        return signal
    
    def closed_loop_cycle(
        self,
        state: np.ndarray,
        action: str,
        execute_action: Callable[[np.ndarray, str], np.ndarray]
    ) -> Dict[str, Any]:
        """
        Exécute un cycle complet de boucle fermée.
        
        CYCLE COMPLET:
        1. Prédire état futur
        2. Exécuter action (via callback)
        3. Observer état réel
        4. Calculer erreur
        5. Apprendre
        
        Args:
            state: État actuel
            action: Action à exécuter
            execute_action: Callback pour exécuter action
            
        Returns:
            Dict avec tous les résultats du cycle
        """
        # 1. PRÉDIRE
        prediction = self.predict(state, action)
        
        # 2. AGIR (via callback externe)
        observed_state = execute_action(state, action)
        
        # 3. COMPARER
        error = self.compute_error(prediction.predicted_state, observed_state)
        
        # 4. APPRENDRE
        learning_signal = self.learn(state, action, observed_state, error)
        
        # 5. RETOURNER RÉSULTATS
        return {
            'prediction': prediction.to_dict(),
            'error': error.to_dict(),
            'learning_signal': learning_signal.to_dict(),
            'observed_state': observed_state,
            'cycle_complete': True
        }
    
    def _model_based_prediction(
        self,
        state: np.ndarray,
        action: str
    ) -> Tuple[np.ndarray, float]:
        """Prédiction basée sur modèle appris."""
        transitions = self.transition_model[action]
        
        # Trouver transition la plus similaire
        best_match = None
        best_similarity = -1.0
        
        for trans in transitions:
            similarity = self._compute_similarity(state, trans['state_before'])
            if similarity > best_similarity:
                best_similarity = similarity
                best_match = trans
        
        if best_match:
            predicted = best_match['state_after'].copy()
            confidence = best_similarity * self.action_confidence.get(action, 0.5)
            return predicted, confidence
        
        # Fallback
        return self._heuristic_prediction(state, action)
    
    def _heuristic_prediction(
        self,
        state: np.ndarray,
        action: str
    ) -> Tuple[np.ndarray, float]:
        """Prédiction heuristique (fallback)."""
        # Prédiction simple: état inchangé
        predicted = state.copy()
        confidence = 0.3  # Faible confiance
        return predicted, confidence
    
    def _update_transition_model(
        self,
        state_before: np.ndarray,
        action: str,
        state_after: np.ndarray,
        learning_rate: float
    ):
        """Met à jour le modèle de transitions."""
        if action not in self.transition_model:
            self.transition_model[action] = []
        
        # Ajouter nouvelle transition
        transition = {
            'state_before': state_before.copy(),
            'state_after': state_after.copy(),
            'learning_rate': learning_rate,
            'timestamp': time.time()
        }
        
        self.transition_model[action].append(transition)
        
        # Limiter taille (garder 50 plus récentes)
        if len(self.transition_model[action]) > 50:
            self.transition_model[action] = self.transition_model[action][-50:]
    
    def _compute_similarity(
        self,
        state1: np.ndarray,
        state2: np.ndarray
    ) -> float:
        """Calcule similarité entre deux états."""
        if state1.shape != state2.shape:
            return 0.0
        
        # Similarité cosinus
        flat1 = state1.flatten()
        flat2 = state2.flatten()
        
        norm1 = np.linalg.norm(flat1)
        norm2 = np.linalg.norm(flat2)
        
        if norm1 == 0 or norm2 == 0:
            return 0.0
        
        similarity = np.dot(flat1, flat2) / (norm1 * norm2)
        return max(0.0, similarity)
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensique."""
        if self.forensic_logger:
            try:
                self.forensic_logger.log(
                    event=event_type,
                    component="PredictionEngine",
                    data=data
                )
            except Exception:
                pass
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques du moteur."""
        accuracy = 0.0
        if self.total_predictions > 0:
            accuracy = self.correct_predictions / self.total_predictions
        
        avg_error = 0.0
        if len(self.error_history) > 0:
            avg_error = np.mean([e.error_magnitude for e in self.error_history])
        
        return {
            'total_predictions': self.total_predictions,
            'correct_predictions': self.correct_predictions,
            'prediction_accuracy': accuracy,
            'total_learning_updates': self.total_learning_updates,
            'average_error': avg_error,
            'num_actions_learned': len(self.transition_model),
            'action_confidences': dict(self.action_confidence),
            'history_size': len(self.prediction_history)
        }
    
    def reset(self):
        """Réinitialise le moteur."""
        self.transition_model.clear()
        self.prediction_history.clear()
        self.error_history.clear()
        self.action_confidence.clear()
        self.total_predictions = 0
        self.correct_predictions = 0
        self.total_learning_updates = 0
        self.cumulative_error = 0.0
        
        self._log_forensic("PREDICTION_ENGINE_RESET", {})


def test_prediction_engine():
    """Test basique du moteur de prédiction."""
    print("🧪 Test PredictionEngine...")
    
    # Créer moteur
    engine = PredictionEngine(
        base_learning_rate=0.1,
        surprise_threshold=0.5
    )
    
    # Test 1: Prédiction simple
    print("\n1. Test prédiction...")
    state = np.random.rand(10, 10)
    action = 'move_right'
    
    prediction = engine.predict(state, action)
    print(f"   ✓ Prédiction: confiance={prediction.confidence:.2f}, méthode={prediction.method_used}")
    
    # Test 2: Calcul erreur
    print("\n2. Test calcul erreur...")
    observed = state + np.random.rand(10, 10) * 0.1
    error = engine.compute_error(prediction.predicted_state, observed)
    print(f"   ✓ Erreur: magnitude={error.error_magnitude:.4f}, précision={error.prediction_accuracy:.2%}")
    
    # Test 3: Apprentissage
    print("\n3. Test apprentissage...")
    signal = engine.learn(state, action, observed, error)
    print(f"   ✓ Signal: lr={signal.learning_rate:.3f}, régions focus={len(signal.focus_regions)}")
    
    # Test 4: Cycle complet
    print("\n4. Test cycle complet...")
    def mock_execute(s, a):
        return s + np.random.rand(*s.shape) * 0.05
    
    result = engine.closed_loop_cycle(state, action, mock_execute)
    print(f"   ✓ Cycle: complet={result['cycle_complete']}")
    
    # Statistiques
    print("\n📊 Statistiques:")
    stats = engine.get_statistics()
    print(f"   - Prédictions: {stats['total_predictions']}")
    print(f"   - Précision: {stats['prediction_accuracy']:.2%}")
    print(f"   - Mises à jour: {stats['total_learning_updates']}")
    print(f"   - Actions apprises: {stats['num_actions_learned']}")
    
    print("\n✅ Test PredictionEngine réussi!")


if __name__ == "__main__":
    test_prediction_engine()

# Made with Bob
