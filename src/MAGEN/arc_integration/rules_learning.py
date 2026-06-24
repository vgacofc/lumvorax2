"""
Rules Learning - Apprentissage Règles Causales (Phase 4.5.2)

Objectif: Apprendre règles causales ACTION → EFFET

Méthode:
- Observer ACTION effectuée
- Comparer état AVANT et APRÈS
- Détecter CHANGEMENTS (effets)
- Créer/mettre à jour RÈGLE causale
- Augmenter CONFIANCE avec répétitions

Exemples règles:
- ACTION4 + Avatar adjacent Bloc → Bloc bouge
- ACTION1 + Avatar sur Goal → Victoire
- ACTION2 + Timer=0 → Timer reset (84 pixels)
- ACTION2 + Step=42 → Événement majeur (88 pixels)

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.2
"""

import numpy as np
from collections import defaultdict
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass, field
from enum import IntEnum


class EffectType(IntEnum):
    """Types d'effets observables"""
    ENTITY_MOVED = 1          # Entité a bougé
    ENTITY_CREATED = 2        # Nouvelle entité apparue
    ENTITY_DELETED = 3        # Entité disparue
    ENTITY_TRANSFORMED = 4    # Entité changée (taille/valeur)
    PIXELS_MODIFIED = 5       # Pixels modifiés
    VICTORY = 6               # Victoire obtenue
    DEFEAT = 7                # Défaite
    NO_EFFECT = 8             # Aucun effet


@dataclass
class Effect:
    """Effet observé"""
    type: EffectType
    entity_id: Optional[str] = None
    details: Dict = field(default_factory=dict)
    magnitude: float = 0.0
    
    def __repr__(self):
        if self.entity_id:
            return f"Effect({self.type.name}, entity={self.entity_id}, mag={self.magnitude:.1f})"
        return f"Effect({self.type.name}, mag={self.magnitude:.1f})"


@dataclass
class Precondition:
    """Précondition pour règle"""
    type: str  # "entity_exists", "entity_adjacent", "entity_at_position", etc.
    entity_id: Optional[str] = None
    details: Dict = field(default_factory=dict)
    
    def matches(self, state: Dict) -> bool:
        """Vérifier si précondition satisfaite dans état"""
        # À implémenter selon type
        return True


@dataclass
class CausalRule:
    """
    Règle causale ACTION → EFFET
    
    Structure:
    - Condition: ACTION effectuée
    - Préconditions: État requis avant action
    - Effets: Changements observés après action
    - Confiance: Probabilité règle vraie (0.0-1.0)
    - Observations: Nombre fois règle observée
    """
    rule_id: str
    action: int
    preconditions: List[Precondition] = field(default_factory=list)
    effects: List[Effect] = field(default_factory=list)
    confidence: float = 0.5
    observations: int = 0
    successes: int = 0
    failures: int = 0
    
    def update_confidence(self, success: bool):
        """Mettre à jour confiance selon observation"""
        self.observations += 1
        
        if success:
            self.successes += 1
        else:
            self.failures += 1
        
        # Confiance = ratio succès / total
        if self.observations > 0:
            self.confidence = self.successes / self.observations
    
    def __repr__(self):
        return (f"Rule(action={self.action}, "
                f"effects={len(self.effects)}, "
                f"conf={self.confidence:.2%}, "
                f"obs={self.observations})")


class RulesLearning:
    """
    Apprentissage règles causales ACTION → EFFET
    
    Principe:
    1. Observer action + états avant/après
    2. Détecter changements (effets)
    3. Créer ou mettre à jour règle
    4. Augmenter confiance avec répétitions
    5. Prédire résultats futurs
    """
    
    def __init__(self):
        # Règles apprises
        self.rules: Dict[str, CausalRule] = {}
        self.rules_by_action: Dict[int, List[CausalRule]] = defaultdict(list)
        
        # Historique observations
        self.observations: List[Dict] = []
        
        # Statistiques
        self.total_observations = 0
        self.total_rules = 0
        self.prediction_accuracy = 0.0
        
        # Seuils
        self.min_observations_for_rule = 2
        self.confidence_threshold = 0.6
    
    def learn_rule(self, action: int, entities_before: Dict, 
                   entities_after: Dict, reward: float = 0.0):
        """
        Apprendre règle depuis observation
        
        Args:
            action: Action effectuée
            entities_before: Entités avant action
            entities_after: Entités après action
            reward: Récompense reçue
        """
        self.total_observations += 1
        
        # Détecter effets
        effects = self.detect_effects(entities_before, entities_after, reward)
        
        # Extraire préconditions
        preconditions = self.extract_preconditions(entities_before)
        
        # Créer signature règle
        rule_signature = self.create_rule_signature(action, effects)
        
        # Trouver ou créer règle
        if rule_signature in self.rules:
            # Mettre à jour règle existante
            rule = self.rules[rule_signature]
            rule.update_confidence(success=len(effects) > 0)
        else:
            # Créer nouvelle règle
            rule = CausalRule(
                rule_id=rule_signature,
                action=action,
                preconditions=preconditions,
                effects=effects,
                confidence=0.5,
                observations=1,
                successes=1 if len(effects) > 0 else 0,
                failures=0 if len(effects) > 0 else 1
            )
            
            self.rules[rule_signature] = rule
            self.rules_by_action[action].append(rule)
            self.total_rules += 1
        
        # Enregistrer observation
        self.observations.append({
            'action': action,
            'effects': effects,
            'reward': reward,
            'rule_id': rule_signature
        })
    
    def detect_effects(self, entities_before: Dict, entities_after: Dict, 
                      reward: float) -> List[Effect]:
        """
        Détecter effets entre deux états
        
        Compare entités avant/après pour identifier changements
        """
        effects = []
        
        # Victoire/Défaite
        if reward > 0:
            effects.append(Effect(
                type=EffectType.VICTORY,
                magnitude=reward
            ))
            return effects
        elif reward < 0:
            effects.append(Effect(
                type=EffectType.DEFEAT,
                magnitude=abs(reward)
            ))
            return effects
        
        # Entités créées
        new_entities = set(entities_after.keys()) - set(entities_before.keys())
        for entity_id in new_entities:
            effects.append(Effect(
                type=EffectType.ENTITY_CREATED,
                entity_id=entity_id,
                details={'size': entities_after[entity_id].size},
                magnitude=float(entities_after[entity_id].size)
            ))
        
        # Entités supprimées
        deleted_entities = set(entities_before.keys()) - set(entities_after.keys())
        for entity_id in deleted_entities:
            effects.append(Effect(
                type=EffectType.ENTITY_DELETED,
                entity_id=entity_id,
                details={'size': entities_before[entity_id].size},
                magnitude=float(entities_before[entity_id].size)
            ))
        
        # Entités modifiées
        common_entities = set(entities_before.keys()) & set(entities_after.keys())
        for entity_id in common_entities:
            entity_before = entities_before[entity_id]
            entity_after = entities_after[entity_id]
            
            # Mouvement
            pos_before = entity_before.centroid
            pos_after = entity_after.centroid
            distance = np.sqrt((pos_after[0] - pos_before[0])**2 + 
                             (pos_after[1] - pos_before[1])**2)
            
            if distance > 0.5:
                effects.append(Effect(
                    type=EffectType.ENTITY_MOVED,
                    entity_id=entity_id,
                    details={
                        'from': pos_before,
                        'to': pos_after,
                        'distance': distance
                    },
                    magnitude=distance
                ))
            
            # Transformation (taille changée)
            size_diff = abs(entity_after.size - entity_before.size)
            if size_diff > 1:
                effects.append(Effect(
                    type=EffectType.ENTITY_TRANSFORMED,
                    entity_id=entity_id,
                    details={
                        'size_before': entity_before.size,
                        'size_after': entity_after.size,
                        'diff': size_diff
                    },
                    magnitude=float(size_diff)
                ))
        
        # Si aucun effet détecté
        if not effects:
            effects.append(Effect(type=EffectType.NO_EFFECT))
        
        return effects
    
    def extract_preconditions(self, entities: Dict) -> List[Precondition]:
        """
        Extraire préconditions depuis état
        
        Identifie conditions importantes dans état actuel
        """
        preconditions = []
        
        # Pour chaque entité
        for entity_id, entity in entities.items():
            # Précondition: entité existe
            preconditions.append(Precondition(
                type="entity_exists",
                entity_id=entity_id,
                details={
                    'value': entity.value,
                    'size': entity.size,
                    'category': entity.category
                }
            ))
        
        return preconditions
    
    def create_rule_signature(self, action: int, effects: List[Effect]) -> str:
        """
        Créer signature unique pour règle
        
        Signature = action + types effets
        """
        effect_types = sorted([e.type.value for e in effects])
        signature = f"action_{action}_effects_{'_'.join(map(str, effect_types))}"
        return signature
    
    def predict_outcome(self, action: int, current_entities: Dict) -> List[Effect]:
        """
        Prédire résultat d'une action
        
        Utilise règles apprises pour prédire effets
        """
        # Trouver règles pour cette action
        candidate_rules = self.rules_by_action.get(action, [])
        
        if not candidate_rules:
            return [Effect(type=EffectType.NO_EFFECT)]
        
        # Filtrer règles avec confiance suffisante
        confident_rules = [r for r in candidate_rules 
                          if r.confidence >= self.confidence_threshold]
        
        if not confident_rules:
            # Utiliser règle la plus observée
            confident_rules = sorted(candidate_rules, 
                                   key=lambda r: r.observations, 
                                   reverse=True)[:1]
        
        # Agréger prédictions
        predicted_effects = []
        for rule in confident_rules:
            # Vérifier préconditions
            if self.check_preconditions(rule.preconditions, current_entities):
                predicted_effects.extend(rule.effects)
        
        return predicted_effects if predicted_effects else [Effect(type=EffectType.NO_EFFECT)]
    
    def check_preconditions(self, preconditions: List[Precondition], 
                           entities: Dict) -> bool:
        """
        Vérifier si préconditions satisfaites
        
        Returns:
            True si toutes préconditions satisfaites
        """
        for precond in preconditions:
            if precond.type == "entity_exists":
                if precond.entity_id not in entities:
                    return False
        
        return True
    
    def get_rules_for_action(self, action: int, min_confidence: float = 0.5) -> List[CausalRule]:
        """
        Obtenir règles pour action donnée
        
        Args:
            action: Action
            min_confidence: Confiance minimale
            
        Returns:
            Liste règles filtrées
        """
        rules = self.rules_by_action.get(action, [])
        return [r for r in rules if r.confidence >= min_confidence]
    
    def get_most_confident_rules(self, top_n: int = 10) -> List[CausalRule]:
        """
        Obtenir règles avec meilleure confiance
        
        Args:
            top_n: Nombre de règles à retourner
            
        Returns:
            Liste règles triées par confiance
        """
        all_rules = list(self.rules.values())
        sorted_rules = sorted(all_rules, key=lambda r: r.confidence, reverse=True)
        return sorted_rules[:top_n]
    
    def get_statistics(self) -> Dict:
        """
        Obtenir statistiques apprentissage
        
        Returns:
            Dict avec statistiques complètes
        """
        if not self.rules:
            return {
                'total_rules': 0,
                'total_observations': self.total_observations,
                'avg_confidence': 0.0,
                'rules_by_action': {}
            }
        
        # Calculer confiance moyenne
        avg_confidence = np.mean([r.confidence for r in self.rules.values()])
        
        # Règles par action
        rules_by_action = {
            action: len(rules) 
            for action, rules in self.rules_by_action.items()
        }
        
        # Top règles
        top_rules = self.get_most_confident_rules(5)
        
        return {
            'total_rules': len(self.rules),
            'total_observations': self.total_observations,
            'avg_confidence': float(avg_confidence),
            'rules_by_action': rules_by_action,
            'top_rules': [
                {
                    'action': r.action,
                    'confidence': r.confidence,
                    'observations': r.observations,
                    'effects': len(r.effects)
                }
                for r in top_rules
            ]
        }
    
    def __repr__(self):
        stats = self.get_statistics()
        return (f"RulesLearning(rules={stats['total_rules']}, "
                f"obs={stats['total_observations']}, "
                f"avg_conf={stats['avg_confidence']:.2%})")

# Made with Bob
