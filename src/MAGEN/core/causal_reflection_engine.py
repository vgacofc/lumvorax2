"""
Causal Reflection Engine - MAGEN V33
====================================

Module critique qui transforme MAGEN d'un moteur transformationnel
en un agent cognitif apprenant de ses échecs.

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V33 Prototype Minimal (Phase 1)

Architecture:
    1. Failure Detector: Identifie patterns d'échec répétés
    2. Causal Hypothesis Generator: Génère hypothèses sur POURQUOI
    3. Model Update Engine: Modifie modèle interne basé sur hypothèses
    4. Forensic Integration: Logging complet pour traçabilité

Référence: ARCHITECTURE_CAUSAL_REFLECTION_ENGINE_V33.md
"""

import time
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, field
from collections import Counter
from enum import Enum


class FailureType(Enum):
    """Types de patterns d'échec détectables"""
    WAIT_LOOP = "wait_loop"
    ACTION_LOOP = "action_loop"
    STAGNATION = "stagnation"
    LOW_HEALTH = "low_health"
    NO_EXPLORATION = "no_exploration"


class CauseType(Enum):
    """Types de causes racines possibles"""
    WRONG_THRESHOLD = "wrong_threshold"
    ACTION_OVERVALUED = "action_overvalued"
    WRONG_GOAL = "wrong_goal_representation"
    MODEL_FROZEN = "model_frozen"
    HEALTH_MISCALCULATED = "health_miscalculated"


class UpdateType(Enum):
    """Types de mises à jour du modèle"""
    POLICY_SHIFT = "policy_shift"
    WEIGHT_ADJUSTMENT = "weight_adjustment"
    THRESHOLD_CHANGE = "threshold_change"
    GOAL_REFRAMING = "goal_reframing"
    EXPLORATION_FORCED = "exploration_forced"


@dataclass
class Step:
    """Représente un step d'exécution"""
    step_number: int
    action: str
    score: float
    health: float
    timestamp_ns: int
    context: Dict[str, Any] = field(default_factory=dict)


@dataclass
class FailurePattern:
    """Pattern d'échec détecté"""
    failure_type: FailureType
    severity: float  # 0.0 → 1.0
    repetition_count: int
    duration_steps: int
    context: Dict[str, Any] = field(default_factory=dict)
    detected_at_step: int = 0
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "failure_type": self.failure_type.value,
            "severity": self.severity,
            "repetition_count": self.repetition_count,
            "duration_steps": self.duration_steps,
            "context": self.context,
            "detected_at_step": self.detected_at_step
        }


@dataclass
class CausalHypothesis:
    """Hypothèse causale sur un échec"""
    hypothesis_id: str
    cause_type: CauseType
    explanation: str
    confidence: float  # 0.0 → 1.0
    testable_prediction: str
    suggested_update: 'ModelUpdate'
    created_at_step: int = 0
    validated: Optional[bool] = None
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "hypothesis_id": self.hypothesis_id,
            "cause_type": self.cause_type.value,
            "explanation": self.explanation,
            "confidence": self.confidence,
            "prediction": self.testable_prediction,
            "suggested_update": self.suggested_update.to_dict(),
            "created_at_step": self.created_at_step,
            "validated": self.validated
        }


@dataclass
class ModelUpdate:
    """Mise à jour du modèle interne"""
    update_type: UpdateType
    target_component: str
    old_value: Any
    new_value: Any
    justification: str
    applied_at_step: int = 0
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "update_type": self.update_type.value,
            "target": self.target_component,
            "old_value": str(self.old_value),
            "new_value": str(self.new_value),
            "justification": self.justification,
            "applied_at_step": self.applied_at_step
        }


class CausalReflectionEngine:
    """
    Moteur de réflexion causale pour MAGEN V33
    
    Transforme les échecs répétés en apprentissage structurel
    via génération d'hypothèses causales et mise à jour du modèle.
    
    Principe fondamental:
        Échec → Pourquoi? → Hypothèse → Test → Nouveau modèle
    """
    
    def __init__(self, forensic_logger=None):
        """
        Initialise le Causal Reflection Engine
        
        Args:
            forensic_logger: Logger forensic pour traçabilité complète
        """
        self.forensic_logger = forensic_logger
        
        # État interne
        self.failure_history: List[FailurePattern] = []
        self.hypothesis_cache: Dict[str, CausalHypothesis] = {}
        self.model_updates: List[ModelUpdate] = []
        
        # Compteurs pour détection
        self.consecutive_waits = 0
        self.action_repetition_counter: Dict[str, int] = {}
        self.stagnation_steps = 0
        self.last_score = 0.0
        
        # Configuration
        self.wait_loop_threshold = 10  # N waits consécutifs = loop
        self.action_loop_threshold = 15  # N répétitions = loop
        self.stagnation_threshold = 20  # N steps sans progrès = stagnation
        self.stagnation_score_delta = 0.05  # Δ score minimum
        
        # État de politique
        self.forced_exploration_enabled = False
        self.forced_exploration_counter = 0
        self.action_blacklist: Dict[str, int] = {}  # action → steps restants
        
        self._log_initialization()
    
    def _log_initialization(self) -> None:
        """Log l'initialisation du CRE"""
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_initialized",
                "CausalReflectionEngine",
                {
                    "version": "V33_prototype_minimal",
                    "wait_loop_threshold": self.wait_loop_threshold,
                    "action_loop_threshold": self.action_loop_threshold,
                    "stagnation_threshold": self.stagnation_threshold,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def detect_failure(self, history: List[Step]) -> Optional[FailurePattern]:
        """
        Détecte patterns d'échec dans l'historique récent
        
        Args:
            history: Liste des steps récents
            
        Returns:
            FailurePattern si détecté, None sinon
        """
        if len(history) < 5:
            return None
        
        current_step = history[-1].step_number
        
        # Détection 1: WAIT loop
        wait_pattern = self._detect_wait_loop(history)
        if wait_pattern:
            wait_pattern.detected_at_step = current_step
            if self.forensic_logger:
                self._log_failure_detected(wait_pattern)
            return wait_pattern
        
        # Détection 2: ACTION loop
        action_pattern = self._detect_action_loop(history)
        if action_pattern:
            action_pattern.detected_at_step = current_step
            if self.forensic_logger:
                self._log_failure_detected(action_pattern)
            return action_pattern
        
        # Détection 3: Stagnation
        stagnation_pattern = self._detect_stagnation(history)
        if stagnation_pattern:
            stagnation_pattern.detected_at_step = current_step
            if self.forensic_logger:
                self._log_failure_detected(stagnation_pattern)
            return stagnation_pattern
        
        # Détection 4: Low health persistant
        health_pattern = self._detect_low_health(history)
        if health_pattern:
            health_pattern.detected_at_step = current_step
            if self.forensic_logger:
                self._log_failure_detected(health_pattern)
            return health_pattern
        
        return None
    
    def _detect_wait_loop(self, history: List[Step]) -> Optional[FailurePattern]:
        """Détecte si les N dernières actions sont 'wait'"""
        recent = history[-self.wait_loop_threshold:]
        
        if len(recent) < self.wait_loop_threshold:
            return None
        
        wait_count = sum(1 for step in recent if step.action == "wait")
        
        if wait_count >= self.wait_loop_threshold:
            return FailurePattern(
                failure_type=FailureType.WAIT_LOOP,
                severity=0.9,
                repetition_count=wait_count,
                duration_steps=len(recent),
                context={
                    "wait_ratio": wait_count / len(recent),
                    "avg_health": sum(s.health for s in recent) / len(recent)
                }
            )
        
        return None
    
    def _detect_action_loop(self, history: List[Step]) -> Optional[FailurePattern]:
        """Détecte si une action domine sans progrès"""
        recent = history[-self.action_loop_threshold:]
        
        if len(recent) < self.action_loop_threshold:
            return None
        
        # Compter actions
        action_counts = Counter(step.action for step in recent)
        most_common_action, count = action_counts.most_common(1)[0]
        
        # Vérifier domination (> 70%)
        ratio = count / len(recent)
        if ratio < 0.7:
            return None
        
        # Vérifier absence de progrès
        scores = [step.score for step in recent]
        score_delta = max(scores) - min(scores)
        
        if score_delta < self.stagnation_score_delta:
            return FailurePattern(
                failure_type=FailureType.ACTION_LOOP,
                severity=0.8,
                repetition_count=count,
                duration_steps=len(recent),
                context={
                    "dominant_action": most_common_action,
                    "action_ratio": ratio,
                    "score_delta": score_delta
                }
            )
        
        return None
    
    def _detect_stagnation(self, history: List[Step]) -> Optional[FailurePattern]:
        """Détecte stagnation globale (aucun progrès)"""
        recent = history[-self.stagnation_threshold:]
        
        if len(recent) < self.stagnation_threshold:
            return None
        
        scores = [step.score for step in recent]
        score_delta = max(scores) - min(scores)
        
        if score_delta < self.stagnation_score_delta:
            return FailurePattern(
                failure_type=FailureType.STAGNATION,
                severity=0.7,
                repetition_count=0,
                duration_steps=len(recent),
                context={
                    "score_delta": score_delta,
                    "avg_score": sum(scores) / len(scores),
                    "action_diversity": len(set(s.action for s in recent))
                }
            )
        
        return None
    
    def _detect_low_health(self, history: List[Step]) -> Optional[FailurePattern]:
        """Détecte santé cognitive basse persistante"""
        recent = history[-10:]
        
        if len(recent) < 10:
            return None
        
        avg_health = sum(step.health for step in recent) / len(recent)
        
        if avg_health < 0.3:
            return FailurePattern(
                failure_type=FailureType.LOW_HEALTH,
                severity=0.85,
                repetition_count=len(recent),
                duration_steps=len(recent),
                context={
                    "avg_health": avg_health,
                    "min_health": min(s.health for s in recent),
                    "max_health": max(s.health for s in recent)
                }
            )
        
        return None
    
    def generate_hypothesis(self, failure: FailurePattern) -> CausalHypothesis:
        """
        Génère hypothèse causale basée sur le pattern d'échec
        
        Args:
            failure: Pattern d'échec détecté
            
        Returns:
            CausalHypothesis expliquant la cause probable
        """
        hypothesis_id = f"hyp_{failure.failure_type.value}_{time.time_ns()}"
        
        if failure.failure_type == FailureType.WAIT_LOOP:
            hypothesis = self._generate_wait_loop_hypothesis(failure, hypothesis_id)
        
        elif failure.failure_type == FailureType.ACTION_LOOP:
            hypothesis = self._generate_action_loop_hypothesis(failure, hypothesis_id)
        
        elif failure.failure_type == FailureType.STAGNATION:
            hypothesis = self._generate_stagnation_hypothesis(failure, hypothesis_id)
        
        elif failure.failure_type == FailureType.LOW_HEALTH:
            hypothesis = self._generate_low_health_hypothesis(failure, hypothesis_id)
        
        else:
            # Hypothèse générique
            hypothesis = self._generate_generic_hypothesis(failure, hypothesis_id)
        
        # Cacher hypothèse
        self.hypothesis_cache[hypothesis_id] = hypothesis
        
        # Logger
        if self.forensic_logger:
            self._log_hypothesis_generated(hypothesis)
        
        return hypothesis
    
    def _generate_wait_loop_hypothesis(
        self, 
        failure: FailurePattern, 
        hyp_id: str
    ) -> CausalHypothesis:
        """Génère hypothèse pour WAIT loop"""
        avg_health = failure.context.get("avg_health", 0.0)
        
        update = ModelUpdate(
            update_type=UpdateType.EXPLORATION_FORCED,
            target_component="exploration_policy",
            old_value="conservative_when_low_health",
            new_value="forced_after_10_waits",
            justification=f"Santé basse ({avg_health:.3f}) → wait permanent → paralysie"
        )
        
        return CausalHypothesis(
            hypothesis_id=hyp_id,
            cause_type=CauseType.WRONG_THRESHOLD,
            explanation=(
                f"Santé cognitive basse ({avg_health:.3f}) déclenche mode conservateur "
                f"qui choisit toujours 'wait', créant paralysie permanente sans récupération"
            ),
            confidence=0.9,
            testable_prediction=(
                "Si exploration forcée après 10 waits, diversité actions devrait augmenter "
                "et santé devrait se stabiliser ou augmenter"
            ),
            suggested_update=update
        )
    
    def _generate_action_loop_hypothesis(
        self,
        failure: FailurePattern,
        hyp_id: str
    ) -> CausalHypothesis:
        """Génère hypothèse pour ACTION loop"""
        dominant_action = failure.context.get("dominant_action", "UNKNOWN")
        action_ratio = failure.context.get("action_ratio", 0.0)
        
        update = ModelUpdate(
            update_type=UpdateType.WEIGHT_ADJUSTMENT,
            target_component=f"action_weights.{dominant_action}",
            old_value=1.0,
            new_value=0.5,
            justification=(
                f"{dominant_action} répété {failure.repetition_count}x "
                f"({action_ratio:.1%}) sans progrès"
            )
        )
        
        return CausalHypothesis(
            hypothesis_id=hyp_id,
            cause_type=CauseType.ACTION_OVERVALUED,
            explanation=(
                f"Action '{dominant_action}' surreprésentée ({action_ratio:.1%}) "
                f"sans produire de progrès réel (Δscore < {self.stagnation_score_delta})"
            ),
            confidence=0.85,
            testable_prediction=(
                f"Si poids de '{dominant_action}' réduit de 50%, "
                "diversité exploration devrait augmenter"
            ),
            suggested_update=update
        )
    
    def _generate_stagnation_hypothesis(
        self,
        failure: FailurePattern,
        hyp_id: str
    ) -> CausalHypothesis:
        """Génère hypothèse pour stagnation"""
        score_delta = failure.context.get("score_delta", 0.0)
        action_diversity = failure.context.get("action_diversity", 0)
        
        update = ModelUpdate(
            update_type=UpdateType.GOAL_REFRAMING,
            target_component="goal_hypotheses",
            old_value="current_goals",
            new_value="alternative_goals",
            justification=(
                f"Aucun progrès pendant {failure.duration_steps} steps "
                f"(Δscore={score_delta:.4f})"
            )
        )
        
        return CausalHypothesis(
            hypothesis_id=hyp_id,
            cause_type=CauseType.WRONG_GOAL,
            explanation=(
                f"Stagnation prolongée ({failure.duration_steps} steps) "
                f"malgré diversité actions ({action_diversity} types) "
                "suggère représentation de but incorrecte"
            ),
            confidence=0.7,
            testable_prediction=(
                "Si buts régénérés avec représentation alternative, "
                "nouvelles actions devraient émerger"
            ),
            suggested_update=update
        )
    
    def _generate_low_health_hypothesis(
        self,
        failure: FailurePattern,
        hyp_id: str
    ) -> CausalHypothesis:
        """Génère hypothèse pour santé basse persistante"""
        avg_health = failure.context.get("avg_health", 0.0)
        
        update = ModelUpdate(
            update_type=UpdateType.THRESHOLD_CHANGE,
            target_component="health_threshold",
            old_value=0.3,
            new_value=0.1,
            justification=(
                f"Santé moyenne {avg_health:.3f} < 0.3 pendant "
                f"{failure.duration_steps} steps"
            )
        )
        
        return CausalHypothesis(
            hypothesis_id=hyp_id,
            cause_type=CauseType.HEALTH_MISCALCULATED,
            explanation=(
                f"Santé cognitive persistante basse ({avg_health:.3f}) "
                "suggère calcul santé trop pessimiste ou seuil trop élevé"
            ),
            confidence=0.75,
            testable_prediction=(
                "Si seuil abaissé à 0.1, mode conservateur devrait se déclencher "
                "moins souvent et exploration devrait augmenter"
            ),
            suggested_update=update
        )
    
    def _generate_generic_hypothesis(
        self,
        failure: FailurePattern,
        hyp_id: str
    ) -> CausalHypothesis:
        """Génère hypothèse générique"""
        update = ModelUpdate(
            update_type=UpdateType.POLICY_SHIFT,
            target_component="exploration_policy",
            old_value="current",
            new_value="exploratory",
            justification=f"Échec type {failure.failure_type.value} détecté",
            applied_at_step=0
        )
        
        return CausalHypothesis(
            hypothesis_id=hyp_id,
            cause_type=CauseType.MODEL_FROZEN,
            explanation=f"Pattern d'échec {failure.failure_type.value} détecté",
            confidence=0.5,
            testable_prediction="Changement de politique devrait modifier comportement",
            suggested_update=update,
            created_at_step=0
        )
    
    def update_model(self, hypothesis: CausalHypothesis, current_step: int) -> None:
        """
        Applique mise à jour du modèle basée sur hypothèse
        
        Args:
            hypothesis: Hypothèse causale validée
            current_step: Numéro du step actuel
        """
        update = hypothesis.suggested_update
        update.applied_at_step = current_step
        
        if update.update_type == UpdateType.EXPLORATION_FORCED:
            self._apply_forced_exploration(update)
        
        elif update.update_type == UpdateType.WEIGHT_ADJUSTMENT:
            self._apply_weight_adjustment(update)
        
        elif update.update_type == UpdateType.THRESHOLD_CHANGE:
            self._apply_threshold_change(update)
        
        elif update.update_type == UpdateType.GOAL_REFRAMING:
            self._apply_goal_reframing(update)
        
        elif update.update_type == UpdateType.POLICY_SHIFT:
            self._apply_policy_shift(update)
        
        # Enregistrer mise à jour
        self.model_updates.append(update)
        
        # Logger
        self._log_model_updated(hypothesis, update)
    
    def _apply_forced_exploration(self, update: ModelUpdate) -> None:
        """Active exploration forcée"""
        self.forced_exploration_enabled = True
        self.forced_exploration_counter = 0
        
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="forced_exploration_enabled",
                component="CausalReflectionEngine",
                data={
                    "reason": update.justification,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _apply_weight_adjustment(self, update: ModelUpdate) -> None:
        """Ajuste poids d'action (blacklist temporaire)"""
        # Extraire nom d'action du target
        action_name = update.target_component.split(".")[-1]
        
        # Blacklist pour 50 steps
        self.action_blacklist[action_name] = 50
        
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="action_blacklisted",
                component="CausalReflectionEngine",
                data={
                    "action": action_name,
                    "duration_steps": 50,
                    "reason": update.justification,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _apply_threshold_change(self, update: ModelUpdate) -> None:
        """Change seuil (note: nécessite coordination avec DecisionKernel)"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="threshold_changed",
                component="CausalReflectionEngine",
                data={
                    "target": update.target_component,
                    "old_value": update.old_value,
                    "new_value": update.new_value,
                    "reason": update.justification,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _apply_goal_reframing(self, update: ModelUpdate) -> None:
        """Régénère hypothèses de buts (note: nécessite coordination)"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="goal_reframing_requested",
                component="CausalReflectionEngine",
                data={
                    "reason": update.justification,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _apply_policy_shift(self, update: ModelUpdate) -> None:
        """Change politique globale"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="policy_shifted",
                component="CausalReflectionEngine",
                data={
                    "old_policy": update.old_value,
                    "new_policy": update.new_value,
                    "reason": update.justification,
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def should_force_exploration(self) -> bool:
        """Vérifie si exploration forcée est nécessaire"""
        if not self.forced_exploration_enabled:
            return False
        
        self.forced_exploration_counter += 1
        
        # Forcer exploration tous les 3 steps pendant 15 steps
        if self.forced_exploration_counter <= 15:
            return self.forced_exploration_counter % 3 == 0
        
        # Désactiver après 15 steps
        self.forced_exploration_enabled = False
        return False
    
    def is_action_blacklisted(self, action: str) -> bool:
        """Vérifie si action est blacklistée"""
        if action not in self.action_blacklist:
            return False
        
        # Décrémenter compteur
        self.action_blacklist[action] -= 1
        
        # Retirer si expiré
        if self.action_blacklist[action] <= 0:
            del self.action_blacklist[action]
            return False
        
        return True
    
    def get_state(self) -> Dict[str, Any]:
        """Retourne état complet du CRE pour forensic"""
        return {
            "failures_detected": len(self.failure_history),
            "hypotheses_generated": len(self.hypothesis_cache),
            "model_updates_applied": len(self.model_updates),
            "forced_exploration_enabled": self.forced_exploration_enabled,
            "action_blacklist": dict(self.action_blacklist),
            "consecutive_waits": self.consecutive_waits,
            "stagnation_steps": self.stagnation_steps
        }
    
    def _log_failure_detected(self, failure: FailurePattern) -> None:
        """Log détection d'échec"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="failure_detected",
                component="CausalReflectionEngine",
                data={
                    "failure": failure.to_dict(),
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _log_hypothesis_generated(self, hypothesis: CausalHypothesis) -> None:
        """Log génération d'hypothèse"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="hypothesis_generated",
                component="CausalReflectionEngine",
                data={
                    "hypothesis": hypothesis.to_dict(),
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def _log_model_updated(
        self, 
        hypothesis: CausalHypothesis, 
        update: ModelUpdate
    ) -> None:
        """Log mise à jour du modèle"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="model_updated",
                component="CausalReflectionEngine",
                data={
                    "hypothesis_id": hypothesis.hypothesis_id,
                    "update": update.to_dict(),
                    "timestamp_ns": time.time_ns()
                }
            )
    
    def close(self) -> None:
        """Ferme le CRE et log statistiques finales"""
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event="module_closing",
                component="CausalReflectionEngine",
                data={
                    "final_state": self.get_state(),
                    "timestamp_ns": time.time_ns()
                }
            )


# Export public API
__all__ = [
    'CausalReflectionEngine',
    'FailureType',
    'CauseType',
    'UpdateType',
    'Step',
    'FailurePattern',
    'CausalHypothesis',
    'ModelUpdate'
]

# Made with Bob
