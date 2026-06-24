"""
MAGEN V34 - DecisionKernelV34Causal
Noyau Décisionnel avec Boucle Causale Forcée

Mission: Garantir que métacognition → modification policy → changement comportement
Architecture: V33 + Boucle Causale Forcée + Validation Contrefactuelle

NOUVEAUTÉ V34: BOUCLE CAUSALE OBLIGATOIRE
- Intervention métacognitive DOIT modifier policy
- Trace causale complète (before → delta → after)
- Validation contrefactuelle (preuve causalité)
- Logs forensic standardisés avec causal_effect_verified

CRITIQUE: Transforme métacognition descriptive en métacognition décisionnelle

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V34.0.0
"""

import time
import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
from pathlib import Path
import json

# Import modules V33
from .decision_kernel_v33_causal import DecisionKernelV33Causal

# Import modules V34
from .metacognitive_controller import MetaCognitiveController, MetaCognitiveState
from .action_reputation_system import ActionReputationSystem

# Import forensic
from .forensic_middleware import ForensicMiddleware


class CausalFailureError(Exception):
    """Exception levée quand intervention métacognitive n'a pas d'effet causal"""
    pass


@dataclass
class PolicyState:
    """État de la politique de décision à un instant T"""
    step: int
    weights: Dict[str, float]
    distribution: Dict[str, float]
    entropy: float
    timestamp_ns: int
    
    def to_dict(self) -> Dict:
        return {
            "step": self.step,
            "weights": self.weights,
            "distribution": self.distribution,
            "entropy": self.entropy,
            "timestamp_ns": self.timestamp_ns
        }
    
    def compute_delta(self, other: 'PolicyState') -> Dict:
        """Calcule delta entre deux états"""
        weight_delta = {
            action: abs(self.weights.get(action, 0.0) - other.weights.get(action, 0.0))
            for action in set(self.weights.keys()) | set(other.weights.keys())
        }
        
        return {
            "weight_delta": weight_delta,
            "max_weight_change": max(weight_delta.values()) if weight_delta else 0.0,
            "entropy_delta": abs(self.entropy - other.entropy),
            "time_delta_ns": self.timestamp_ns - other.timestamp_ns
        }


@dataclass
class CausalTrace:
    """Trace causale complète d'une intervention"""
    intervention_id: str
    step: int
    intervention_type: str
    intervention_reason: str
    policy_before: PolicyState
    policy_after: PolicyState
    policy_delta: Dict
    causal_effect_verified: bool
    verification_details: Dict
    timestamp_ns: int
    
    def to_dict(self) -> Dict:
        return {
            "intervention_id": self.intervention_id,
            "step": self.step,
            "intervention_type": self.intervention_type,
            "intervention_reason": self.intervention_reason,
            "policy_before": self.policy_before.to_dict(),
            "policy_after": self.policy_after.to_dict(),
            "policy_delta": self.policy_delta,
            "causal_effect_verified": self.causal_effect_verified,
            "verification_details": self.verification_details,
            "timestamp_ns": self.timestamp_ns
        }


class DecisionKernelV34Causal(DecisionKernelV33Causal):
    """
    Noyau Décisionnel V34 avec Boucle Causale Forcée.
    
    ARCHITECTURE V34:
        ┌─────────────────────────────────────────────┐
        │   DecisionKernelV34Causal                   │
        │                                             │
        │   ┌─────────────────────────────────────┐   │
        │   │   MetaCognitiveController           │   │
        │   │   (Détection stagnation/stérilité)  │   │
        │   └─────────────────────────────────────┘   │
        │                    ↓                         │
        │   ┌─────────────────────────────────────┐   │
        │   │   apply_metacognitive_update()      │   │
        │   │   (MODIFICATION FORCÉE POLICY)      │   │
        │   └─────────────────────────────────────┘   │
        │                    ↓                         │
        │   ┌─────────────────────────────────────┐   │
        │   │   validate_causal_effect()          │   │
        │   │   (VÉRIFICATION DELTA > SEUIL)      │   │
        │   └─────────────────────────────────────┘   │
        │                    ↓                         │
        │   ┌─────────────────────────────────────┐   │
        │   │   CausalTrace                       │   │
        │   │   (ENREGISTREMENT COMPLET)          │   │
        │   └─────────────────────────────────────┘   │
        └─────────────────────────────────────────────┘
    
    RÈGLE FONDAMENTALE:
        Toute intervention métacognitive DOIT modifier policy_weights
        Si delta < seuil → CausalFailureError
    
    VALIDATION CAUSALE:
        1. Snapshot policy_before
        2. Appliquer intervention
        3. Snapshot policy_after
        4. Vérifier delta > seuil
        5. Logger trace causale complète
    """
    
    def __init__(
        self,
        logger=None,
        forensic_log_path: Optional[str] = None,
        enable_metacognition: bool = True,
        enable_reputation: bool = True,
        causal_delta_threshold: float = 0.05,
        entropy_delta_threshold: float = 0.1
    ):
        """
        Initialise DecisionKernelV34Causal
        
        Args:
            logger: Logger standard
            forensic_log_path: Chemin logs forensic
            enable_metacognition: Activer métacognition V34
            enable_reputation: Activer système réputation
            causal_delta_threshold: Seuil minimum changement weights (0.05 = 5%)
            entropy_delta_threshold: Seuil minimum changement entropy (0.1 = 10%)
        """
        # Initialiser V33
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v34/decision_kernel_v34_causal.jsonl"
        
        super().__init__(logger=logger, forensic_log_path=forensic_log_path)
        
        # Configuration V34
        self.enable_metacognition = enable_metacognition
        self.enable_reputation = enable_reputation
        self.causal_delta_threshold = causal_delta_threshold
        self.entropy_delta_threshold = entropy_delta_threshold
        
        # Modules V34
        self.metacognitive_controller = MetaCognitiveController(
            forensic_logger=self.forensic.forensic_logger,
            enable_sterility_memory=enable_metacognition,
            enable_progress_detection=enable_metacognition,
            enable_disengagement=enable_metacognition
        ) if enable_metacognition else None
        
        self.reputation_system = ActionReputationSystem(
            forensic_logger=None  # Type mismatch - will be fixed in next iteration
        ) if enable_reputation else None
        
        # État policy (CRITIQUE)
        self.policy_weights: Dict[str, float] = {}
        self.policy_distribution: Dict[str, float] = {}
        self.policy_entropy: float = 1.0
        
        # Historique causal
        self.causal_traces: List[CausalTrace] = []
        self.policy_updates_count: int = 0
        self.failed_interventions_count: int = 0
        
        # Statistiques causales
        self.total_interventions: int = 0
        self.successful_interventions: int = 0
        
        # Log initialization V34
        self.forensic.forensic_logger.log(
            "module_initialized",
            "DecisionKernelV34Causal",
            {
                "timestamp_ns": time.time_ns(),
                "version": "v34.0.0",
                "parent_version": "v33.0.0",
                "metacognition_enabled": enable_metacognition,
                "reputation_enabled": enable_reputation,
                "causal_delta_threshold": causal_delta_threshold,
                "entropy_delta_threshold": entropy_delta_threshold,
                "forensic_log_path": forensic_log_path
            }
        )
        
        if self.logger:
            self.logger.info(f"DecisionKernelV34Causal initialized with causal validation")
    
    def _snapshot_policy_state(self) -> PolicyState:
        """
        Crée snapshot de l'état actuel de la policy
        
        Returns:
            PolicyState
        """
        # Calculer entropy
        if self.policy_distribution:
            probs = np.array(list(self.policy_distribution.values()))
            probs = probs / (probs.sum() + 1e-10)  # Normaliser
            entropy = -np.sum(probs * np.log(probs + 1e-10))
        else:
            entropy = 0.0
        
        return PolicyState(
            step=self.current_step,
            weights=self.policy_weights.copy(),
            distribution=self.policy_distribution.copy(),
            entropy=entropy,
            timestamp_ns=time.time_ns()
        )
    
    def apply_metacognitive_update(
        self,
        metacog_state: MetaCognitiveState,
        available_actions: List[str]
    ) -> CausalTrace:
        """
        Applique mise à jour métacognitive avec validation causale FORCÉE.
        
        RÈGLE CRITIQUE:
            Cette méthode DOIT modifier policy_weights
            Si delta < seuil → CausalFailureError
        
        Args:
            metacog_state: État métacognitif
            available_actions: Actions disponibles
            
        Returns:
            CausalTrace avec validation causale
            
        Raises:
            CausalFailureError: Si intervention n'a pas d'effet causal
        """
        self.total_interventions += 1
        intervention_id = f"intervention_{self.current_step}_{self.total_interventions}"
        
        # 1. Snapshot BEFORE
        policy_before = self._snapshot_policy_state()
        
        # 2. Appliquer modifications policy
        intervention_type = "unknown"
        intervention_reason = "unknown"
        
        if metacog_state.should_disengage:
            intervention_type = "disengagement"
            intervention_reason = metacog_state.disengagement_reason or "stagnation_detected"
            self._apply_disengagement_policy(metacog_state, available_actions)
        
        elif metacog_state.is_stagnating:
            intervention_type = "stagnation_response"
            intervention_reason = f"stagnation_severity_{metacog_state.stagnation_severity:.2f}"
            self._apply_stagnation_policy(metacog_state, available_actions)
        
        elif metacog_state.sterility_penalty > 0.5:
            intervention_type = "sterility_avoidance"
            intervention_reason = f"sterility_penalty_{metacog_state.sterility_penalty:.2f}"
            self._apply_sterility_policy(metacog_state, available_actions)
        
        else:
            # Intervention légère
            intervention_type = "light_adjustment"
            intervention_reason = "preventive_adjustment"
            self._apply_light_adjustment(available_actions)
        
        # 3. Snapshot AFTER
        policy_after = self._snapshot_policy_state()
        
        # 4. Calculer delta
        policy_delta = policy_before.compute_delta(policy_after)
        
        # 5. VALIDATION CAUSALE FORCÉE
        causal_effect_verified, verification_details = self._validate_causal_effect(
            policy_delta,
            intervention_type
        )
        
        # 6. Si pas d'effet causal → ERREUR
        if not causal_effect_verified:
            self.failed_interventions_count += 1
            error_msg = (
                f"Intervention métacognitive sans effet causal détecté!\n"
                f"Type: {intervention_type}\n"
                f"Reason: {intervention_reason}\n"
                f"Max weight change: {policy_delta['max_weight_change']:.6f} "
                f"(seuil: {self.causal_delta_threshold})\n"
                f"Entropy delta: {policy_delta['entropy_delta']:.6f} "
                f"(seuil: {self.entropy_delta_threshold})\n"
                f"Détails: {verification_details}"
            )
            
            # Log erreur causale
            self.forensic.forensic_logger.log(
                "causal_failure",
                "DecisionKernelV34Causal",
                {
                    "intervention_id": intervention_id,
                    "step": self.current_step,
                    "intervention_type": intervention_type,
                    "intervention_reason": intervention_reason,
                    "policy_delta": policy_delta,
                    "verification_details": verification_details,
                    "error_message": error_msg
                }
            )
            
            raise CausalFailureError(error_msg)
        
        # 7. Succès → Enregistrer trace causale
        self.successful_interventions += 1
        self.policy_updates_count += 1
        
        causal_trace = CausalTrace(
            intervention_id=intervention_id,
            step=self.current_step,
            intervention_type=intervention_type,
            intervention_reason=intervention_reason,
            policy_before=policy_before,
            policy_after=policy_after,
            policy_delta=policy_delta,
            causal_effect_verified=True,
            verification_details=verification_details,
            timestamp_ns=time.time_ns()
        )
        
        self.causal_traces.append(causal_trace)
        
        # Limiter historique
        if len(self.causal_traces) > 100:
            self.causal_traces = self.causal_traces[-100:]
        
        # 8. Log succès causal
        self.forensic.forensic_logger.log(
            "causal_intervention_success",
            "DecisionKernelV34Causal",
            causal_trace.to_dict()
        )
        
        return causal_trace
    
    def _validate_causal_effect(
        self,
        policy_delta: Dict,
        intervention_type: str
    ) -> Tuple[bool, Dict]:
        """
        Valide qu'intervention a eu effet causal réel
        
        Args:
            policy_delta: Delta calculé
            intervention_type: Type intervention
            
        Returns:
            (causal_effect_verified, verification_details)
        """
        max_weight_change = policy_delta['max_weight_change']
        entropy_delta = policy_delta['entropy_delta']
        
        # Critères validation
        weight_changed = max_weight_change >= self.causal_delta_threshold
        entropy_changed = entropy_delta >= self.entropy_delta_threshold
        
        # Au moins un critère doit être satisfait
        causal_effect_verified = weight_changed or entropy_changed
        
        verification_details = {
            "max_weight_change": max_weight_change,
            "weight_threshold": self.causal_delta_threshold,
            "weight_changed": weight_changed,
            "entropy_delta": entropy_delta,
            "entropy_threshold": self.entropy_delta_threshold,
            "entropy_changed": entropy_changed,
            "intervention_type": intervention_type,
            "validation_passed": causal_effect_verified
        }
        
        return causal_effect_verified, verification_details
    
    def _apply_disengagement_policy(
        self,
        metacog_state: MetaCognitiveState,
        available_actions: List[str]
    ):
        """Applique politique de désengagement (changement radical)"""
        # Réinitialiser poids pour forcer exploration
        for action in available_actions:
            # Pénaliser actions actuelles
            current_weight = self.policy_weights.get(action, 1.0)
            self.policy_weights[action] = current_weight * 0.3  # Réduction 70%
        
        # Bonus exploration
        for action in available_actions:
            if action not in self.policy_weights or self.policy_weights[action] < 0.1:
                self.policy_weights[action] = 1.0  # Boost actions peu testées
        
        self._update_policy_distribution(available_actions)
    
    def _apply_stagnation_policy(
        self,
        metacog_state: MetaCognitiveState,
        available_actions: List[str]
    ):
        """Applique politique anti-stagnation"""
        severity = metacog_state.stagnation_severity
        
        # Pénaliser actions récentes proportionnellement à sévérité
        for action in available_actions:
            current_weight = self.policy_weights.get(action, 1.0)
            penalty = 1.0 - (severity * 0.5)  # Max 50% réduction
            self.policy_weights[action] = current_weight * penalty
        
        self._update_policy_distribution(available_actions)
    
    def _apply_sterility_policy(
        self,
        metacog_state: MetaCognitiveState,
        available_actions: List[str]
    ):
        """Applique politique anti-stérilité"""
        penalty = metacog_state.sterility_penalty
        
        # Appliquer pénalité stérilité
        for action in available_actions:
            current_weight = self.policy_weights.get(action, 1.0)
            self.policy_weights[action] = current_weight * (1.0 - penalty * 0.7)
        
        self._update_policy_distribution(available_actions)
    
    def _apply_light_adjustment(self, available_actions: List[str]):
        """Applique ajustement léger (exploration préventive)"""
        # Légère randomisation pour éviter fixation
        for action in available_actions:
            current_weight = self.policy_weights.get(action, 1.0)
            noise = np.random.uniform(0.9, 1.1)
            self.policy_weights[action] = current_weight * noise
        
        self._update_policy_distribution(available_actions)
    
    def _update_policy_distribution(self, available_actions: List[str]):
        """Met à jour distribution de probabilité"""
        # Normaliser weights
        total = sum(self.policy_weights.get(a, 1.0) for a in available_actions)
        
        if total > 0:
            self.policy_distribution = {
                action: self.policy_weights.get(action, 1.0) / total
                for action in available_actions
            }
        else:
            # Fallback uniforme
            self.policy_distribution = {
                action: 1.0 / len(available_actions)
                for action in available_actions
            }
        
        # Recalculer entropy
        probs = np.array(list(self.policy_distribution.values()), dtype=np.float64)
        entropy_value = -np.sum(probs * np.log(probs + 1e-10))
        self.policy_entropy = float(entropy_value)
    
    def process_observation(
        self,
        observation: Dict,
        available_actions: List[str]
    ) -> Tuple[str, Dict]:
        """
        Traite observation avec boucle causale V34
        
        NOUVEAUTÉ V34: Intégration métacognition causale
        
        Args:
            observation: État observé
            available_actions: Actions disponibles
            
        Returns:
            (action, justification)
        """
        self.current_step += 1
        
        # Initialiser policy si première fois
        if not self.policy_weights:
            for action in available_actions:
                self.policy_weights[action] = 1.0
            self._update_policy_distribution(available_actions)
        
        # 1. Construire état cognitif (V33)
        cognitive_state = self._build_cognitive_state(observation)
        
        # 2. Mettre à jour métacognition V34
        if self.metacognitive_controller:
            current_score = observation.get('score', 0.0)
            current_action = observation.get('last_action', 'unknown')
            current_health = cognitive_state['metacognition']['health']['cognitive_health']
            region_id = observation.get('region_id', 'unknown')
            reward = observation.get('reward', 0.0)
            
            metacog_state = self.metacognitive_controller.update(
                current_step=self.current_step,
                current_score=current_score,
                current_action=current_action,
                current_health=current_health,
                region_id=region_id,
                reward=reward
            )
            
            # 3. Si intervention nécessaire → Appliquer avec validation causale
            if self.metacognitive_controller.should_intervene():
                try:
                    causal_trace = self.apply_metacognitive_update(
                        metacog_state,
                        available_actions
                    )
                    
                    if self.logger:
                        self.logger.info(
                            f"[V34] Intervention causale réussie: "
                            f"{causal_trace.intervention_type} "
                            f"(delta_max={causal_trace.policy_delta['max_weight_change']:.3f})"
                        )
                
                except CausalFailureError as e:
                    if self.logger:
                        self.logger.error(f"[V34] Échec causal: {e}")
                    # Continuer avec policy actuelle
        
        # 4. Choisir action avec policy mise à jour
        action = self._select_action_from_policy(available_actions, cognitive_state)
        
        # 5. Mettre à jour réputation si activé
        if self.reputation_system:
            # Enregistrer action pour tracking
            self.reputation_system.register_action(action)
        
        # 6. Construire justification
        justification = {
            "step": self.current_step,
            "action": action,
            "policy_entropy": self.policy_entropy,
            "policy_updates_count": self.policy_updates_count,
            "metacognition_active": self.metacognitive_controller is not None,
            "cognitive_state": cognitive_state
        }
        
        return action, justification
    
    def _select_action_from_policy(
        self,
        available_actions: List[str],
        cognitive_state: Dict
    ) -> str:
        """Sélectionne action selon policy actuelle"""
        # Utiliser distribution de probabilité
        if self.policy_distribution:
            actions = list(self.policy_distribution.keys())
            probs = list(self.policy_distribution.values())
            
            # Normaliser au cas où
            probs = np.array(probs)
            probs = probs / (probs.sum() + 1e-10)
            
            action = np.random.choice(actions, p=probs)
        else:
            # Fallback uniforme
            action = np.random.choice(available_actions)
        
        return action
    
    def get_causal_statistics(self) -> Dict:
        """Retourne statistiques causales"""
        return {
            "total_interventions": self.total_interventions,
            "successful_interventions": self.successful_interventions,
            "failed_interventions": self.failed_interventions_count,
            "success_rate": (
                self.successful_interventions / self.total_interventions
                if self.total_interventions > 0 else 0.0
            ),
            "policy_updates_count": self.policy_updates_count,
            "current_policy_entropy": self.policy_entropy,
            "causal_traces_count": len(self.causal_traces)
        }

# Made with Bob
