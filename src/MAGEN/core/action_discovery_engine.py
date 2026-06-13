#!/usr/bin/env python3
"""
MAGEN - Action Discovery Engine (ADE)
=====================================

SESSION 55 - INSIGHT UTILISATEUR RÉVOLUTIONNAIRE

Problème identifié:
Le système suppose quelles actions sont pertinentes AVANT d'avoir exploré
ce que chaque action FAIT réellement dans le contexte du puzzle.

Solution:
Phase 0 d'exploration naïve pour découvrir les affordances disponibles.

Inspiration:
Expérience utilisateur avec jeu arcade - bloqué jusqu'à découverte que
CLICK supprime des objets (action jamais testée auparavant).

Architecture:
1. ActionInventory: Catalogue toutes les primitives DSL disponibles
2. ExperimentalProbe: Teste chaque action sur exemples d'entraînement
3. EffectObserver: Mesure transformations produites
4. AffordanceMapper: Catégorise action → effet → contexte
5. SemanticMemory: Mémorise "dans ce type de puzzle, cette action fait ça"
"""

import numpy as np
from typing import List, Dict, Set, Tuple, Any, Optional
from dataclasses import dataclass, field
from enum import Enum
import time

class ActionType(Enum):
    """Types d'actions découvertes"""
    SPATIAL = "spatial"              # Déplacement, rotation
    MANIPULATION = "manipulation"     # Modification, suppression
    TEMPORAL = "temporal"            # Undo, reset
    MODAL = "modal"                  # Changement de mode/contexte
    COMPOSITIONAL = "compositional"  # Combinaisons d'actions
    META = "meta"                    # Actions sur actions


class EffectType(Enum):
    """Types d'effets observés"""
    OBJECT_CREATED = "object_created"
    OBJECT_DELETED = "object_deleted"
    OBJECT_MOVED = "object_moved"
    OBJECT_TRANSFORMED = "object_transformed"
    COLOR_CHANGED = "color_changed"
    SHAPE_CHANGED = "shape_changed"
    SIZE_CHANGED = "size_changed"
    PATTERN_EMERGED = "pattern_emerged"
    GRID_RESIZED = "grid_resized"
    NO_EFFECT = "no_effect"
    UNKNOWN_EFFECT = "unknown_effect"


@dataclass
class ActionPrimitive:
    """Primitive d'action DSL"""
    name: str
    function: Any
    parameters: List[str]
    action_type: ActionType
    description: str
    tested: bool = False
    success_count: int = 0
    failure_count: int = 0


@dataclass
class ObservedEffect:
    """Effet observé d'une action"""
    action_name: str
    effect_type: EffectType
    before_state: np.ndarray
    after_state: np.ndarray
    diff_magnitude: float
    diff_locations: List[Tuple[int, int]]
    context: Dict[str, Any]
    timestamp: float = field(default_factory=time.time)


@dataclass
class Affordance:
    """Affordance = Action possible dans un contexte donné"""
    action_name: str
    context_signature: str  # Hash du type de puzzle
    effect_types: Set[EffectType]
    success_rate: float
    avg_diff_magnitude: float
    examples: List[ObservedEffect]
    confidence: float


class ActionInventory:
    """
    Catalogue de toutes les primitives d'action disponibles
    
    Inspiré par: "Quels boutons sont disponibles dans ce jeu?"
    """
    
    def __init__(self):
        self.primitives: Dict[str, ActionPrimitive] = {}
        self._initialize_dsl_primitives()
    
    def _initialize_dsl_primitives(self):
        """Initialise catalogue des primitives DSL MAGEN"""
        
        # SPATIAL - Actions de déplacement/transformation spatiale
        spatial_actions = [
            ("move_up", ActionType.SPATIAL, "Déplace objet vers le haut"),
            ("move_down", ActionType.SPATIAL, "Déplace objet vers le bas"),
            ("move_left", ActionType.SPATIAL, "Déplace objet vers la gauche"),
            ("move_right", ActionType.SPATIAL, "Déplace objet vers la droite"),
            ("rotate_90", ActionType.SPATIAL, "Rotation 90° horaire"),
            ("rotate_180", ActionType.SPATIAL, "Rotation 180°"),
            ("rotate_270", ActionType.SPATIAL, "Rotation 270° horaire"),
            ("flip_horizontal", ActionType.SPATIAL, "Miroir horizontal"),
            ("flip_vertical", ActionType.SPATIAL, "Miroir vertical"),
            ("translate", ActionType.SPATIAL, "Translation par vecteur"),
        ]
        
        # MANIPULATION - Actions de modification d'objets
        manipulation_actions = [
            ("fill_color", ActionType.MANIPULATION, "Remplit zone avec couleur"),
            ("change_color", ActionType.MANIPULATION, "Change couleur objet"),
            ("delete_object", ActionType.MANIPULATION, "Supprime objet"),
            ("duplicate_object", ActionType.MANIPULATION, "Duplique objet"),
            ("merge_objects", ActionType.MANIPULATION, "Fusionne objets"),
            ("split_object", ActionType.MANIPULATION, "Divise objet"),
            ("resize_object", ActionType.MANIPULATION, "Redimensionne objet"),
            ("extract_region", ActionType.MANIPULATION, "Extrait région"),
            ("mask_apply", ActionType.MANIPULATION, "Applique masque"),
        ]
        
        # TEMPORAL - Actions temporelles
        temporal_actions = [
            ("undo", ActionType.TEMPORAL, "Annule dernière action"),
            ("reset", ActionType.TEMPORAL, "Réinitialise état"),
            ("replay", ActionType.TEMPORAL, "Rejoue séquence"),
        ]
        
        # MODAL - Actions de changement de contexte
        modal_actions = [
            ("select_object", ActionType.MODAL, "Sélectionne objet"),
            ("change_mode", ActionType.MODAL, "Change mode d'interaction"),
            ("set_focus", ActionType.MODAL, "Définit focus"),
        ]
        
        # COMPOSITIONAL - Combinaisons d'actions
        compositional_actions = [
            ("compose_sequence", ActionType.COMPOSITIONAL, "Séquence d'actions"),
            ("parallel_apply", ActionType.COMPOSITIONAL, "Actions parallèles"),
            ("conditional_apply", ActionType.COMPOSITIONAL, "Action conditionnelle"),
        ]
        
        all_actions = (
            spatial_actions + 
            manipulation_actions + 
            temporal_actions + 
            modal_actions + 
            compositional_actions
        )
        
        for name, action_type, desc in all_actions:
            self.primitives[name] = ActionPrimitive(
                name=name,
                function=None,  # Sera lié au DSL réel
                parameters=[],
                action_type=action_type,
                description=desc
            )
    
    def get_untested_actions(self) -> List[ActionPrimitive]:
        """Retourne actions jamais testées"""
        return [p for p in self.primitives.values() if not p.tested]
    
    def get_actions_by_type(self, action_type: ActionType) -> List[ActionPrimitive]:
        """Retourne actions d'un type donné"""
        return [p for p in self.primitives.values() if p.action_type == action_type]


class ExperimentalProbe:
    """
    Teste systématiquement chaque action sur exemples d'entraînement
    
    Inspiré par: "J'ai appuyé sur tous les boutons pour voir ce qu'ils font"
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.experiments_run: int = 0
    
    def probe_action(self,
                    action: ActionPrimitive,
                    train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                    max_attempts: int = 3) -> List[ObservedEffect]:
        """
        Teste une action sur exemples d'entraînement
        
        Stratégie:
        1. Applique action sur input
        2. Observe changements
        3. Compare avec output attendu
        4. Catégorise effet
        """
        effects = []
        
        for attempt in range(min(max_attempts, len(train_pairs))):
            input_grid, expected_output = train_pairs[attempt]
            
            try:
                # Simuler application action (à implémenter avec DSL réel)
                # Pour l'instant, placeholder
                after_state = input_grid.copy()
                
                # Observer effet
                effect = self._observe_effect(
                    action_name=action.name,
                    before=input_grid,
                    after=after_state,
                    expected=expected_output
                )
                
                effects.append(effect)
                self.experiments_run += 1
                
                if self.verbose:
                    print(f"  🧪 Testé {action.name}: {effect.effect_type.value}")
            
            except Exception as e:
                if self.verbose:
                    print(f"  ⚠️  Erreur test {action.name}: {e}")
                continue
        
        return effects
    
    def _observe_effect(self,
                       action_name: str,
                       before: np.ndarray,
                       after: np.ndarray,
                       expected: np.ndarray) -> ObservedEffect:
        """Observe et catégorise effet d'une action"""
        
        # Calculer différence
        diff = np.sum(before != after)
        diff_magnitude = diff / before.size if before.size > 0 else 0.0
        
        # Localiser changements
        diff_locations = list(zip(*np.where(before != after)))
        
        # Catégoriser effet
        effect_type = self._categorize_effect(before, after, expected)
        
        return ObservedEffect(
            action_name=action_name,
            effect_type=effect_type,
            before_state=before,
            after_state=after,
            diff_magnitude=diff_magnitude,
            diff_locations=diff_locations,
            context={"expected_match": np.array_equal(after, expected)}
        )
    
    def _categorize_effect(self,
                          before: np.ndarray,
                          after: np.ndarray,
                          expected: np.ndarray) -> EffectType:
        """Catégorise type d'effet observé"""
        
        if np.array_equal(before, after):
            return EffectType.NO_EFFECT
        
        # Compter objets (zones non-zéro)
        before_objects = np.count_nonzero(before)
        after_objects = np.count_nonzero(after)
        
        if after_objects > before_objects:
            return EffectType.OBJECT_CREATED
        elif after_objects < before_objects:
            return EffectType.OBJECT_DELETED
        elif before.shape != after.shape:
            return EffectType.GRID_RESIZED
        else:
            # Analyser type de transformation
            if self._is_color_change(before, after):
                return EffectType.COLOR_CHANGED
            elif self._is_spatial_transform(before, after):
                return EffectType.OBJECT_MOVED
            else:
                return EffectType.OBJECT_TRANSFORMED
    
    def _is_color_change(self, before: np.ndarray, after: np.ndarray) -> bool:
        """Détecte si changement = modification couleurs"""
        # Même positions, couleurs différentes
        return (before != 0).sum() == (after != 0).sum()
    
    def _is_spatial_transform(self, before: np.ndarray, after: np.ndarray) -> bool:
        """Détecte si changement = transformation spatiale"""
        # Même couleurs, positions différentes
        return sorted(before.flatten()) == sorted(after.flatten())


class AffordanceMapper:
    """
    Cartographie action → effet → contexte
    
    Inspiré par: "Dans ce type de jeu, ce bouton fait ça"
    """
    
    def __init__(self):
        self.affordances: Dict[str, List[Affordance]] = {}
    
    def learn_affordance(self,
                        action_name: str,
                        effects: List[ObservedEffect],
                        context_signature: str):
        """Apprend affordance d'une action dans un contexte"""
        
        if not effects:
            return
        
        # Agréger effets
        effect_types = set(e.effect_type for e in effects)
        success_count = sum(1 for e in effects if e.context.get("expected_match", False))
        success_rate = success_count / len(effects) if effects else 0.0
        avg_magnitude = float(np.mean([e.diff_magnitude for e in effects]))
        
        # Calculer confiance
        confidence = self._compute_confidence(effects, success_rate)
        
        affordance = Affordance(
            action_name=action_name,
            context_signature=context_signature,
            effect_types=effect_types,
            success_rate=success_rate,
            avg_diff_magnitude=avg_magnitude,
            examples=effects[:5],  # Garder 5 exemples max
            confidence=confidence
        )
        
        if action_name not in self.affordances:
            self.affordances[action_name] = []
        
        self.affordances[action_name].append(affordance)
    
    def _compute_confidence(self, effects: List[ObservedEffect], success_rate: float) -> float:
        """Calcule confiance dans l'affordance"""
        # Confiance = f(nombre exemples, cohérence effets, taux succès)
        n_examples = len(effects)
        consistency = 1.0 if len(set(e.effect_type for e in effects)) == 1 else 0.5
        
        confidence = (
            min(n_examples / 10.0, 1.0) * 0.4 +  # Plus d'exemples = plus confiant
            consistency * 0.3 +                   # Effets cohérents = plus confiant
            success_rate * 0.3                    # Succès = plus confiant
        )
        
        return confidence
    
    def get_best_actions_for_context(self, context_signature: str, top_k: int = 5) -> List[str]:
        """Retourne meilleures actions pour un contexte donné"""
        candidates = []
        
        for action_name, affordances in self.affordances.items():
            for aff in affordances:
                if aff.context_signature == context_signature:
                    score = aff.confidence * aff.success_rate
                    candidates.append((action_name, score))
        
        # Trier par score décroissant
        candidates.sort(key=lambda x: x[1], reverse=True)
        
        return [name for name, score in candidates[:top_k]]


class ActionDiscoveryEngine:
    """
    Moteur principal de découverte d'actions
    
    Phase 0 avant résolution: Explorer ce que chaque action FAIT
    """
    
    def __init__(self, verbose: bool = False):
        self.inventory = ActionInventory()
        self.probe = ExperimentalProbe(verbose=verbose)
        self.mapper = AffordanceMapper()
        self.verbose = verbose
        
        self.discovery_complete = False
        self.total_experiments = 0
    
    def discover_affordances(self,
                           train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                           context_signature: str) -> Dict[str, Any]:
        """
        Phase 0: Découverte systématique des affordances
        
        Processus:
        1. Inventorier actions disponibles
        2. Tester chaque action sur exemples
        3. Observer effets produits
        4. Cartographier action → effet → contexte
        5. Mémoriser affordances découvertes
        """
        if self.verbose:
            print(f"\n🔍 PHASE 0: DÉCOUVERTE AFFORDANCES")
            print(f"{'='*70}")
        
        start_time = time.time()
        
        # Obtenir actions non testées
        untested = self.inventory.get_untested_actions()
        
        if self.verbose:
            print(f"📋 Actions à tester: {len(untested)}")
        
        discovered_affordances = []
        
        for action in untested:
            if self.verbose:
                print(f"\n🧪 Test action: {action.name} ({action.action_type.value})")
            
            # Tester action
            effects = self.probe.probe_action(action, train_pairs)
            
            # Apprendre affordance
            if effects:
                self.mapper.learn_affordance(action.name, effects, context_signature)
                discovered_affordances.append(action.name)
            
            # Marquer comme testé
            action.tested = True
            action.success_count = sum(1 for e in effects if e.context.get("expected_match", False))
            action.failure_count = len(effects) - action.success_count
        
        self.discovery_complete = True
        self.total_experiments = self.probe.experiments_run
        
        elapsed = time.time() - start_time
        
        if self.verbose:
            print(f"\n{'='*70}")
            print(f"✅ DÉCOUVERTE COMPLÈTE")
            print(f"   Temps: {elapsed:.2f}s")
            print(f"   Expériences: {self.total_experiments}")
            print(f"   Affordances découvertes: {len(discovered_affordances)}")
            print(f"{'='*70}")
        
        return {
            "discovered_actions": discovered_affordances,
            "total_experiments": self.total_experiments,
            "elapsed_time": elapsed,
            "affordances": self.mapper.affordances
        }
    
    def get_recommended_actions(self, context_signature: str, top_k: int = 10) -> List[str]:
        """Retourne actions recommandées pour un contexte"""
        if not self.discovery_complete:
            # Si découverte pas faite, retourner toutes actions
            return list(self.inventory.primitives.keys())
        
        return self.mapper.get_best_actions_for_context(context_signature, top_k)
    
    def get_action_semantics(self, action_name: str) -> Optional[List[Affordance]]:
        """Retourne sémantique apprise d'une action"""
        return self.mapper.affordances.get(action_name)

# Made with Bob
