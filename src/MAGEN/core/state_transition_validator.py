"""
State Transition Validator - Validateur de Transitions d'États pour MAGEN
Utilisé par Layer 5 (World Model) et Layer 9 (Prediction)

Objectif:
- Valider la cohérence des transitions d'états
- Détecter les transitions impossibles/anormales
- Maintenir les contraintes du monde
- Supporter l'apprentissage de modèles de transition

Auteur: Bob (Assistant IA)
Date: 2026-06-18
Version: 39
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from collections import defaultdict
from enum import Enum


class TransitionType(Enum):
    """Types de transitions."""
    VALID = "valid"
    INVALID = "invalid"
    ANOMALOUS = "anomalous"
    UNKNOWN = "unknown"


@dataclass
class TransitionRule:
    """
    Règle de transition.
    
    Attributes:
        rule_id: Identifiant de la règle
        preconditions: Conditions préalables
        postconditions: Conditions résultantes
        confidence: Confiance dans la règle [0, 1]
        support: Nombre d'observations
    """
    rule_id: str
    preconditions: Dict[str, any]
    postconditions: Dict[str, any]
    confidence: float = 1.0
    support: int = 0
    
    def matches_preconditions(self, state: Dict) -> bool:
        """Vérifie si l'état satisfait les préconditions."""
        for key, value in self.preconditions.items():
            if key not in state:
                return False
            if state[key] != value:
                return False
        return True
    
    def matches_postconditions(self, state: Dict) -> bool:
        """Vérifie si l'état satisfait les postconditions."""
        for key, value in self.postconditions.items():
            if key not in state:
                return False
            if state[key] != value:
                return False
        return True


@dataclass
class TransitionValidation:
    """
    Résultat de validation d'une transition.
    
    Attributes:
        is_valid: Transition valide
        transition_type: Type de transition
        confidence: Confiance dans la validation
        violated_rules: Règles violées
        matched_rules: Règles satisfaites
        anomaly_score: Score d'anomalie [0, 1]
    """
    is_valid: bool
    transition_type: TransitionType
    confidence: float
    violated_rules: List[str] = field(default_factory=list)
    matched_rules: List[str] = field(default_factory=list)
    anomaly_score: float = 0.0


class StateTransitionValidator:
    """
    Validateur de transitions d'états pour MAGEN.
    
    Fonctionnalités:
    - Validation de cohérence
    - Détection d'anomalies
    - Apprentissage de règles
    - Prédiction de transitions
    """
    
    def __init__(self,
                 min_confidence: float = 0.7,
                 min_support: int = 3,
                 anomaly_threshold: float = 0.8):
        """
        Initialise le validateur.
        
        Args:
            min_confidence: Confiance minimale pour règles
            min_support: Support minimal pour règles
            anomaly_threshold: Seuil de détection d'anomalies
        """
        self.min_confidence = min_confidence
        self.min_support = min_support
        self.anomaly_threshold = anomaly_threshold
        
        # Règles de transition
        self.rules: Dict[str, TransitionRule] = {}
        
        # Historique des transitions
        self.transition_history: List[Tuple[Dict, str, Dict]] = []
        
        # Statistiques de transitions
        self.transition_counts: Dict[Tuple[str, str], int] = defaultdict(int)
        
        # Contraintes du monde
        self.world_constraints: List[callable] = []
        
        # Statistiques
        self.total_validations = 0
        self.valid_transitions = 0
        self.invalid_transitions = 0
        self.anomalous_transitions = 0
    
    def validate_transition(self,
                          state_before: Dict,
                          action: str,
                          state_after: Dict) -> TransitionValidation:
        """
        Valide une transition d'état.
        
        Args:
            state_before: État avant l'action
            action: Action effectuée
            state_after: État après l'action
            
        Returns:
            Résultat de validation
        """
        self.total_validations += 1
        
        # Vérifier les contraintes du monde
        constraint_violations = []
        for constraint in self.world_constraints:
            if not constraint(state_before, action, state_after):
                constraint_violations.append(constraint.__name__)
        
        # Chercher des règles applicables
        matched_rules = []
        violated_rules = []
        
        for rule_id, rule in self.rules.items():
            if rule.matches_preconditions(state_before):
                if rule.matches_postconditions(state_after):
                    matched_rules.append(rule_id)
                else:
                    violated_rules.append(rule_id)
        
        # Calculer score d'anomalie
        anomaly_score = self._compute_anomaly_score(
            state_before, action, state_after
        )
        
        # Déterminer type de transition
        if constraint_violations:
            transition_type = TransitionType.INVALID
            is_valid = False
            confidence = 0.0
            self.invalid_transitions += 1
        elif anomaly_score > self.anomaly_threshold:
            transition_type = TransitionType.ANOMALOUS
            is_valid = False
            confidence = 1.0 - anomaly_score
            self.anomalous_transitions += 1
        elif matched_rules:
            transition_type = TransitionType.VALID
            is_valid = True
            # Confiance = moyenne des règles matchées
            confidence = np.mean([
                self.rules[rid].confidence
                for rid in matched_rules
            ])
            self.valid_transitions += 1
        else:
            transition_type = TransitionType.UNKNOWN
            is_valid = True  # Accepter par défaut
            confidence = 0.5
            self.valid_transitions += 1
        
        # Enregistrer la transition
        self._record_transition(state_before, action, state_after)
        
        return TransitionValidation(
            is_valid=is_valid,
            transition_type=transition_type,
            confidence=confidence,
            violated_rules=violated_rules + constraint_violations,
            matched_rules=matched_rules,
            anomaly_score=anomaly_score
        )
    
    def learn_rule(self,
                  state_before: Dict,
                  action: str,
                  state_after: Dict,
                  rule_id: Optional[str] = None) -> str:
        """
        Apprend une règle de transition depuis un exemple.
        
        Args:
            state_before: État avant
            action: Action
            state_after: État après
            rule_id: ID de règle (généré si None)
            
        Returns:
            ID de la règle créée/mise à jour
        """
        if rule_id is None:
            rule_id = f"rule_{len(self.rules)}"
        
        # Extraire les changements significatifs
        preconditions = {}
        postconditions = {}
        
        for key in state_before:
            if key in state_after:
                if state_before[key] != state_after[key]:
                    preconditions[key] = state_before[key]
                    postconditions[key] = state_after[key]
        
        if rule_id in self.rules:
            # Mettre à jour règle existante
            rule = self.rules[rule_id]
            rule.support += 1
            # Recalculer confiance
            rule.confidence = min(1.0, rule.support / (rule.support + 1))
        else:
            # Créer nouvelle règle
            rule = TransitionRule(
                rule_id=rule_id,
                preconditions=preconditions,
                postconditions=postconditions,
                confidence=0.5,
                support=1
            )
            self.rules[rule_id] = rule
        
        return rule_id
    
    def add_constraint(self, constraint: callable):
        """
        Ajoute une contrainte du monde.
        
        Args:
            constraint: Fonction (state_before, action, state_after) -> bool
        """
        self.world_constraints.append(constraint)
    
    def predict_next_state(self,
                          current_state: Dict,
                          action: str) -> Optional[Dict]:
        """
        Prédit l'état suivant après une action.
        
        Args:
            current_state: État actuel
            action: Action à effectuer
            
        Returns:
            État prédit (ou None si impossible)
        """
        # Chercher règles applicables
        applicable_rules = [
            rule for rule in self.rules.values()
            if rule.matches_preconditions(current_state)
            and rule.confidence >= self.min_confidence
        ]
        
        if not applicable_rules:
            return None
        
        # Utiliser la règle avec la plus haute confiance
        best_rule = max(applicable_rules, key=lambda r: r.confidence)
        
        # Construire l'état prédit
        predicted_state = current_state.copy()
        predicted_state.update(best_rule.postconditions)
        
        return predicted_state
    
    def get_valid_actions(self, current_state: Dict) -> List[str]:
        """
        Retourne les actions valides depuis un état.
        
        Args:
            current_state: État actuel
            
        Returns:
            Liste d'actions valides
        """
        valid_actions = set()
        
        for rule in self.rules.values():
            if rule.matches_preconditions(current_state):
                if rule.confidence >= self.min_confidence:
                    # Extraire l'action depuis l'ID de règle
                    # Format attendu: "rule_action_X"
                    parts = rule.rule_id.split('_')
                    if len(parts) >= 2:
                        valid_actions.add(parts[1])
        
        return list(valid_actions)
    
    def _compute_anomaly_score(self,
                              state_before: Dict,
                              action: str,
                              state_after: Dict) -> float:
        """Calcule le score d'anomalie d'une transition."""
        # Compter les transitions similaires dans l'historique
        similar_count = 0
        total_count = len(self.transition_history)
        
        if total_count == 0:
            return 0.0
        
        for hist_before, hist_action, hist_after in self.transition_history[-100:]:
            if hist_action == action:
                # Calculer similarité des états
                similarity = self._state_similarity(state_before, hist_before)
                if similarity > 0.8:
                    similar_count += 1
        
        # Score d'anomalie = 1 - fréquence relative
        frequency = similar_count / min(100, total_count)
        anomaly_score = 1.0 - frequency
        
        return anomaly_score
    
    def _state_similarity(self, state1: Dict, state2: Dict) -> float:
        """Calcule la similarité entre deux états."""
        if not state1 or not state2:
            return 0.0
        
        common_keys = set(state1.keys()) & set(state2.keys())
        if not common_keys:
            return 0.0
        
        matches = sum(
            1 for key in common_keys
            if state1[key] == state2[key]
        )
        
        return matches / len(common_keys)
    
    def _record_transition(self,
                          state_before: Dict,
                          action: str,
                          state_after: Dict):
        """Enregistre une transition dans l'historique."""
        self.transition_history.append((
            state_before.copy(),
            action,
            state_after.copy()
        ))
        
        # Limiter la taille de l'historique
        if len(self.transition_history) > 1000:
            self.transition_history = self.transition_history[-1000:]
        
        # Mettre à jour les compteurs
        state_key = str(sorted(state_before.items()))
        action_key = (state_key, action)
        self.transition_counts[action_key] += 1
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            'total_validations': self.total_validations,
            'valid_transitions': self.valid_transitions,
            'invalid_transitions': self.invalid_transitions,
            'anomalous_transitions': self.anomalous_transitions,
            'num_rules': len(self.rules),
            'num_constraints': len(self.world_constraints),
            'history_size': len(self.transition_history),
            'valid_rate': self.valid_transitions / max(1, self.total_validations)
        }


def test_state_transition_validator():
    """Test du validateur de transitions."""
    print("=== Test State Transition Validator ===\n")
    
    validator = StateTransitionValidator()
    
    # Définir une contrainte simple
    def no_negative_values(before, action, after):
        """Les valeurs ne peuvent pas être négatives."""
        return all(v >= 0 for v in after.values() if isinstance(v, (int, float)))
    
    validator.add_constraint(no_negative_values)
    print("✓ Contrainte ajoutée: no_negative_values")
    
    # Apprendre quelques règles
    validator.learn_rule(
        {'x': 0, 'y': 0},
        'move_right',
        {'x': 1, 'y': 0}
    )
    validator.learn_rule(
        {'x': 0, 'y': 0},
        'move_up',
        {'x': 0, 'y': 1}
    )
    print(f"\n✓ {len(validator.rules)} règles apprises")
    
    # Test validation transition valide
    result = validator.validate_transition(
        {'x': 0, 'y': 0},
        'move_right',
        {'x': 1, 'y': 0}
    )
    print(f"\n✓ Transition valide:")
    print(f"  - Type: {result.transition_type.value}")
    print(f"  - Confiance: {result.confidence:.2f}")
    print(f"  - Anomalie: {result.anomaly_score:.2f}")
    
    # Test transition invalide
    result = validator.validate_transition(
        {'x': 0, 'y': 0},
        'teleport',
        {'x': -5, 'y': 10}
    )
    print(f"\n✓ Transition invalide:")
    print(f"  - Type: {result.transition_type.value}")
    print(f"  - Violations: {result.violated_rules}")
    
    # Test prédiction
    predicted = validator.predict_next_state({'x': 0, 'y': 0}, 'move_right')
    print(f"\n✓ Prédiction: {predicted}")
    
    # Stats
    stats = validator.get_stats()
    print(f"\n✓ Statistiques:")
    print(f"  - Validations: {stats['total_validations']}")
    print(f"  - Taux valide: {stats['valid_rate']:.2%}")
    print(f"  - Règles: {stats['num_rules']}")


if __name__ == '__main__':
    test_state_transition_validator()

# Made with Bob
