"""
World Dynamics - Modélisation Dynamiques Temporelles (Phase 4.5.3)

Objectif: Comprendre comment le monde ÉVOLUE dans le temps

Méthode:
- Analyser séquences d'états
- Détecter INVARIANTS (ce qui ne change jamais)
- Détecter CYCLES (patterns répétitifs)
- Modéliser TRANSITIONS d'états
- Prédire ÉTATS FUTURS

Découvertes attendues:
- Porte immobile (invariant)
- Timer décroît (dynamique)
- Cycle 43 steps (pattern temporel)
- Avatar contrôlable (règle)

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.3
"""

import numpy as np
from collections import defaultdict, deque
from typing import Dict, List, Tuple, Optional, Set, Any
from dataclasses import dataclass, field


@dataclass
class Invariant:
    """Invariant du monde (propriété qui ne change jamais)"""
    type: str  # "stable_position", "stable_size", "stable_value"
    entity_id: str
    property_name: str
    value: Any
    confidence: float = 1.0
    observations: int = 0
    
    def __repr__(self):
        return f"Invariant({self.type}, {self.entity_id}, {self.property_name}={self.value}, conf={self.confidence:.2%})"


@dataclass
class Cycle:
    """Cycle temporel (pattern répétitif)"""
    period: int  # Nombre de steps dans cycle
    events: List[int]  # Steps où événements se produisent
    confidence: float = 0.5
    observations: int = 0
    pattern_type: str = "unknown"  # "timer_reset", "periodic_event", etc.
    
    def __repr__(self):
        return f"Cycle(period={self.period}, events={len(self.events)}, conf={self.confidence:.2%})"


@dataclass
class StateTransition:
    """Transition entre deux états"""
    from_state_hash: str
    to_state_hash: str
    action: int
    changes: Dict  # Changements observés
    probability: float = 1.0
    observations: int = 1
    
    def __repr__(self):
        return f"Transition(action={self.action}, changes={len(self.changes)}, prob={self.probability:.2%})"


class WorldDynamics:
    """
    Modélisation dynamiques temporelles du monde
    
    Principe:
    1. Observer séquences d'états
    2. Détecter ce qui change / ne change pas
    3. Identifier patterns temporels (cycles)
    4. Modéliser transitions d'états
    5. Prédire états futurs
    """
    
    def __init__(self):
        # Invariants détectés
        self.invariants: List[Invariant] = []
        self.invariants_by_entity: Dict[str, List[Invariant]] = defaultdict(list)
        
        # Cycles détectés
        self.cycles: List[Cycle] = []
        
        # Transitions d'états
        self.transitions: List[StateTransition] = []
        self.transition_graph: Dict[str, List[StateTransition]] = defaultdict(list)
        
        # Historique états
        self.state_history: List[Dict] = []
        self.state_hashes: List[str] = []
        
        # Statistiques
        self.total_observations = 0
        self.major_events: List[Dict] = []
        
        # Seuils
        self.invariant_threshold = 0.1  # Variance max pour invariant
        self.cycle_confidence_threshold = 0.7
        self.min_observations_for_invariant = 5
    
    def learn_dynamics(self, state_sequence: List[Dict], actions: Optional[List[int]] = None):
        """
        Apprendre dynamiques depuis séquence d'états
        
        Args:
            state_sequence: Liste d'états (dicts avec entités)
            actions: Liste d'actions correspondantes (optionnel)
        """
        self.total_observations += len(state_sequence)
        
        # Enregistrer historique
        for state in state_sequence:
            self.state_history.append(state)
            state_hash = self.hash_state(state)
            self.state_hashes.append(state_hash)
        
        # Détecter invariants
        self.detect_invariants(state_sequence)
        
        # Détecter cycles
        self.detect_cycles(state_sequence)
        
        # Modéliser transitions
        if actions:
            self.model_transitions(state_sequence, actions)
        
        # Détecter événements majeurs
        self.detect_major_events(state_sequence)
    
    def detect_invariants(self, state_sequence: List[Dict]):
        """
        Détecter invariants (propriétés stables)
        
        Analyse variance des propriétés sur séquence
        """
        if len(state_sequence) < self.min_observations_for_invariant:
            return
        
        # Collecter propriétés par entité
        entity_properties = defaultdict(lambda: defaultdict(list))
        
        for state in state_sequence:
            entities = state.get('entities', {})
            for entity_id, entity in entities.items():
                # Position
                entity_properties[entity_id]['position'].append(entity.centroid)
                # Taille
                entity_properties[entity_id]['size'].append(entity.size)
                # Valeur
                entity_properties[entity_id]['value'].append(entity.value)
        
        # Analyser variance
        for entity_id, properties in entity_properties.items():
            # Position stable ?
            if len(properties['position']) >= self.min_observations_for_invariant:
                positions = np.array(properties['position'])
                variance = np.var(positions, axis=0).sum()
                
                if variance < self.invariant_threshold:
                    # Position invariante
                    mean_position = tuple(np.mean(positions, axis=0))
                    variance_float = float(variance) if hasattr(variance, 'item') else float(variance)
                    confidence = float(1.0 - min(variance_float, 1.0))
                    
                    invariant = Invariant(
                        type="stable_position",
                        entity_id=entity_id,
                        property_name="position",
                        value=mean_position,
                        confidence=confidence,
                        observations=len(properties['position'])
                    )
                    
                    # Ajouter si pas déjà présent
                    if not self.has_invariant(entity_id, "stable_position"):
                        self.invariants.append(invariant)
                        self.invariants_by_entity[entity_id].append(invariant)
            
            # Taille stable ?
            if len(properties['size']) >= self.min_observations_for_invariant:
                sizes = np.array(properties['size'])
                variance = np.var(sizes)
                
                if variance < self.invariant_threshold:
                    # Taille invariante
                    mean_size = int(np.mean(sizes))
                    variance_float = float(variance) if hasattr(variance, 'item') else float(variance)
                    confidence = float(1.0 - min(variance_float, 1.0))
                    
                    invariant = Invariant(
                        type="stable_size",
                        entity_id=entity_id,
                        property_name="size",
                        value=mean_size,
                        confidence=confidence,
                        observations=len(properties['size'])
                    )
                    
                    if not self.has_invariant(entity_id, "stable_size"):
                        self.invariants.append(invariant)
                        self.invariants_by_entity[entity_id].append(invariant)
    
    def has_invariant(self, entity_id: str, invariant_type: str) -> bool:
        """Vérifier si invariant existe déjà"""
        return any(inv.entity_id == entity_id and inv.type == invariant_type 
                  for inv in self.invariants)
    
    def detect_cycles(self, state_sequence: List[Dict]):
        """
        Détecter cycles temporels (patterns répétitifs)
        
        Analyse événements majeurs pour trouver périodicité
        """
        if len(state_sequence) < 10:
            return
        
        # Détecter événements majeurs (changements importants)
        event_steps = []
        all_changes = []
        
        for i in range(1, len(state_sequence)):
            prev_state = state_sequence[i-1]
            curr_state = state_sequence[i]
            
            # Compter changements
            changes = self.count_changes(prev_state, curr_state)
            all_changes.append(changes)
            
            # Si changement significatif (seuil adaptatif)
            if changes > 5:  # Seuil plus bas pour détecter plus d'événements
                event_steps.append(i)
        
        # Analyser intervalles entre événements
        if len(event_steps) >= 3:  # Au moins 3 événements pour détecter pattern
            intervals = [event_steps[i+1] - event_steps[i]
                        for i in range(len(event_steps)-1)]
            
            if len(intervals) >= 2:
                mean_interval = np.mean(intervals)
                std_interval = np.std(intervals)
                
                # Si intervalles réguliers = cycle (tolérance augmentée)
                if mean_interval > 0 and std_interval < mean_interval * 0.3:  # 30% tolérance
                    period = int(mean_interval)
                    confidence = float(1.0 - min(std_interval / mean_interval, 0.5))
                    
                    cycle = Cycle(
                        period=period,
                        events=event_steps,
                        confidence=max(confidence, 0.5),  # Confiance minimum 50%
                        observations=len(intervals),
                        pattern_type="periodic_event"
                    )
                    
                    # Ajouter si pas déjà présent
                    if not self.has_cycle(period):
                        self.cycles.append(cycle)
        
        # Méthode alternative: analyser tous les changements pour trouver périodicité
        if len(all_changes) >= 20 and len(self.cycles) == 0:
            # Chercher périodicité dans les pics de changements
            changes_array = np.array(all_changes)
            mean_change = np.mean(changes_array)
            
            # Identifier pics (changements > moyenne)
            peaks = [i for i, c in enumerate(all_changes) if c > mean_change * 1.5]
            
            if len(peaks) >= 3:
                peak_intervals = [peaks[i+1] - peaks[i] for i in range(len(peaks)-1)]
                if len(peak_intervals) >= 2:
                    mean_peak_interval = np.mean(peak_intervals)
                    std_peak_interval = np.std(peak_intervals)
                    
                    if mean_peak_interval > 0 and std_peak_interval < mean_peak_interval * 0.4:
                        period = int(mean_peak_interval)
                        confidence = float(1.0 - min(std_peak_interval / mean_peak_interval, 0.5))
                        
                        cycle = Cycle(
                            period=period,
                            events=peaks,
                            confidence=max(confidence, 0.5),
                            observations=len(peak_intervals),
                            pattern_type="peak_pattern"
                        )
                        
                        if not self.has_cycle(period):
                            self.cycles.append(cycle)
    
    def has_cycle(self, period: int, tolerance: int = 2) -> bool:
        """Vérifier si cycle existe déjà"""
        return any(abs(cycle.period - period) <= tolerance for cycle in self.cycles)
    
    def count_changes(self, state1: Dict, state2: Dict) -> int:
        """Compter changements entre deux états"""
        entities1 = state1.get('entities', {})
        entities2 = state2.get('entities', {})
        
        changes = 0
        
        # Entités créées/supprimées
        changes += len(set(entities2.keys()) - set(entities1.keys())) * 10
        changes += len(set(entities1.keys()) - set(entities2.keys())) * 10
        
        # Entités modifiées
        for entity_id in set(entities1.keys()) & set(entities2.keys()):
            entity1 = entities1[entity_id]
            entity2 = entities2[entity_id]
            
            # Position changée
            distance = np.sqrt((entity2.centroid[0] - entity1.centroid[0])**2 +
                             (entity2.centroid[1] - entity1.centroid[1])**2)
            if distance > 0.5:
                changes += int(distance)
            
            # Taille changée
            size_diff = abs(entity2.size - entity1.size)
            if size_diff > 1:
                changes += size_diff
        
        return changes
    
    def model_transitions(self, state_sequence: List[Dict], actions: List[int]):
        """
        Modéliser transitions d'états
        
        Crée graphe de transitions STATE + ACTION → NEXT_STATE
        """
        for i in range(len(state_sequence) - 1):
            state = state_sequence[i]
            next_state = state_sequence[i + 1]
            action = actions[i] if i < len(actions) else 0
            
            # Hash états
            state_hash = self.hash_state(state)
            next_state_hash = self.hash_state(next_state)
            
            # Détecter changements
            changes = self.detect_changes(state, next_state)
            
            # Créer transition
            transition = StateTransition(
                from_state_hash=state_hash,
                to_state_hash=next_state_hash,
                action=action,
                changes=changes,
                probability=1.0,
                observations=1
            )
            
            # Ajouter au graphe
            self.transitions.append(transition)
            self.transition_graph[state_hash].append(transition)
    
    def hash_state(self, state: Dict) -> str:
        """Créer hash d'un état"""
        entities = state.get('entities', {})
        # Hash simple basé sur nombre et positions entités
        entity_info = []
        for entity_id in sorted(entities.keys()):
            entity = entities[entity_id]
            entity_info.append(f"{entity_id}:{entity.centroid[0]:.1f},{entity.centroid[1]:.1f}")
        return "_".join(entity_info)[:32]
    
    def detect_changes(self, state1: Dict, state2: Dict) -> Dict:
        """Détecter changements détaillés entre états"""
        changes = {
            'entities_created': [],
            'entities_deleted': [],
            'entities_moved': [],
            'entities_transformed': []
        }
        
        entities1 = state1.get('entities', {})
        entities2 = state2.get('entities', {})
        
        # Créées
        for entity_id in set(entities2.keys()) - set(entities1.keys()):
            changes['entities_created'].append(entity_id)
        
        # Supprimées
        for entity_id in set(entities1.keys()) - set(entities2.keys()):
            changes['entities_deleted'].append(entity_id)
        
        # Modifiées
        for entity_id in set(entities1.keys()) & set(entities2.keys()):
            entity1 = entities1[entity_id]
            entity2 = entities2[entity_id]
            
            # Mouvement
            distance = np.sqrt((entity2.centroid[0] - entity1.centroid[0])**2 +
                             (entity2.centroid[1] - entity1.centroid[1])**2)
            if distance > 0.5:
                changes['entities_moved'].append({
                    'entity_id': entity_id,
                    'distance': distance
                })
            
            # Transformation
            size_diff = abs(entity2.size - entity1.size)
            if size_diff > 1:
                changes['entities_transformed'].append({
                    'entity_id': entity_id,
                    'size_diff': size_diff
                })
        
        return changes
    
    def detect_major_events(self, state_sequence: List[Dict]):
        """Détecter événements majeurs dans séquence"""
        all_changes = []
        
        # Calculer tous les changements
        for i in range(1, len(state_sequence)):
            prev_state = state_sequence[i-1]
            curr_state = state_sequence[i]
            changes = self.count_changes(prev_state, curr_state)
            all_changes.append((i, changes))
        
        if not all_changes:
            return
        
        # Seuil adaptatif basé sur moyenne
        changes_values = [c for _, c in all_changes]
        mean_change = np.mean(changes_values)
        std_change = np.std(changes_values)
        
        # Événement majeur si changement > moyenne + 1.5 * écart-type
        threshold = mean_change + 1.5 * std_change
        
        for step, changes in all_changes:
            if changes > threshold and changes > 10:  # Minimum absolu
                event = {
                    'step': step,
                    'magnitude': changes,
                    'type': 'major_change'
                }
                self.major_events.append(event)
    
    def predict_next_state(self, current_state: Dict, action: int) -> Optional[Dict]:
        """
        Prédire état futur
        
        Utilise graphe transitions pour prédire
        """
        state_hash = self.hash_state(current_state)
        
        # Trouver transitions depuis cet état
        possible_transitions = self.transition_graph.get(state_hash, [])
        
        # Filtrer par action
        matching_transitions = [t for t in possible_transitions if t.action == action]
        
        if not matching_transitions:
            return None
        
        # Utiliser transition la plus probable
        best_transition = max(matching_transitions, key=lambda t: t.probability)
        
        # Retourner hash état suivant (simplifié)
        return {'next_state_hash': best_transition.to_state_hash}
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques dynamiques"""
        return {
            'total_observations': self.total_observations,
            'invariants_detected': len(self.invariants),
            'cycles_detected': len(self.cycles),
            'transitions_modeled': len(self.transitions),
            'major_events': len(self.major_events),
            'invariants_by_type': {
                inv_type: len([inv for inv in self.invariants if inv.type == inv_type])
                for inv_type in set(inv.type for inv in self.invariants)
            },
            'cycles_info': [
                {
                    'period': cycle.period,
                    'confidence': cycle.confidence,
                    'events': len(cycle.events)
                }
                for cycle in self.cycles
            ]
        }
    
    def __repr__(self):
        stats = self.get_statistics()
        return (f"WorldDynamics(obs={stats['total_observations']}, "
                f"invariants={stats['invariants_detected']}, "
                f"cycles={stats['cycles_detected']}, "
                f"transitions={stats['transitions_modeled']})")

# Made with Bob
