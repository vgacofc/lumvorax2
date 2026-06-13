#!/usr/bin/env python3
"""
MAGEN - Transformation Space Engine
====================================

SESSION 49 - CORRECTION #3: Intégration Continuité Compositionnelle

Basé sur analyse ChatGPT:
"Une identité n'est pas une forme — c'est une continuité de transformations"

Implémente:
1. Espace de transformation (entités = vecteurs latents)
2. Règles de composition (comme couleurs primaires)
3. Continuité vs rupture d'identité
4. Transformations réversibles/irréversibles

Intégré avec DynamicIdentityTracker pour tracking entités.
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass
from enum import Enum


class TransformationType(Enum):
    """Types de transformations"""
    REVERSIBLE = "reversible"           # Rotation, translation
    SEMI_REVERSIBLE = "semi_reversible" # Scaling avec perte info
    IRREVERSIBLE = "irreversible"       # Fusion, destruction


@dataclass
class TransformationRule:
    """Règle de transformation compositionnelle"""
    name: str
    base_elements: List[str]  # Éléments de base (comme couleurs primaires)
    composition_fn: callable  # Fonction de composition
    reversible: TransformationType
    

class TransformationSpace:
    """
    Espace de transformation pour entités
    
    Principe: Chaque entité vit dans un espace latent de transformations
    possibles, défini par des règles de composition déterministes.
    
    Analogie couleurs:
    - Rouge, Bleu, Jaune = bases
    - Violet = Rouge + Bleu (composition stable)
    - Noir/Blanc = modulations
    """
    
    def __init__(self):
        # Éléments de base (invariants fondamentaux)
        self.base_elements = {
            'position': ['x', 'y'],
            'shape': ['rectangle', 'L_shape', 'line', 'point'],
            'color': ['primary', 'secondary', 'background'],
            'size': ['small', 'medium', 'large'],
            'orientation': ['0', '90', '180', '270']
        }
        
        # Règles de composition
        self.composition_rules: List[TransformationRule] = []
        self._init_composition_rules()
        
        # Historique transformations
        self.transformation_history: Dict[str, List[Dict]] = {}
        
    def _init_composition_rules(self):
        """Initialise règles de composition (comme couleurs primaires)"""
        # Règle 1: Composition spatiale
        self.composition_rules.append(TransformationRule(
            name="spatial_composition",
            base_elements=['position', 'size'],
            composition_fn=self._compose_spatial,
            reversible=TransformationType.REVERSIBLE
        ))
        
        # Règle 2: Composition forme
        self.composition_rules.append(TransformationRule(
            name="shape_composition",
            base_elements=['shape', 'orientation'],
            composition_fn=self._compose_shape,
            reversible=TransformationType.SEMI_REVERSIBLE
        ))
        
        # Règle 3: Modulation couleur
        self.composition_rules.append(TransformationRule(
            name="color_modulation",
            base_elements=['color'],
            composition_fn=self._modulate_color,
            reversible=TransformationType.REVERSIBLE
        ))
    
    def _compose_spatial(self, entity_state: Dict) -> Dict:
        """Composition spatiale (position + size)"""
        return {
            'bounding_box': (
                entity_state.get('position', (0, 0)),
                entity_state.get('size', (1, 1))
            ),
            'center': self._compute_center(entity_state)
        }
    
    def _compose_shape(self, entity_state: Dict) -> Dict:
        """Composition forme (shape + orientation)"""
        base_shape = entity_state.get('shape', 'rectangle')
        orientation = entity_state.get('orientation', 0)
        
        return {
            'oriented_shape': f"{base_shape}_{orientation}",
            'canonical_form': base_shape
        }
    
    def _modulate_color(self, entity_state: Dict) -> Dict:
        """Modulation couleur (comme +noir/+blanc)"""
        base_color = entity_state.get('color', 'primary')
        
        return {
            'display_color': base_color,
            'color_family': self._get_color_family(base_color)
        }
    
    def _compute_center(self, entity_state: Dict) -> Tuple[float, float]:
        """Calcule centre entité"""
        pos = entity_state.get('position', (0, 0))
        size = entity_state.get('size', (1, 1))
        return (pos[0] + size[0]/2, pos[1] + size[1]/2)
    
    def _get_color_family(self, color: str) -> str:
        """Détermine famille couleur"""
        if color in ['red', 'blue', 'yellow']:
            return 'primary'
        elif color in ['purple', 'orange', 'green']:
            return 'secondary'
        else:
            return 'other'
    
    def detect_continuity_threshold(self,
                                    state1: Dict,
                                    state2: Dict) -> Tuple[bool, float]:
        """
        Détecte si transformation préserve continuité identité
        
        Returns:
            (is_continuous, similarity_score)
        """
        # Calculer similarité sur chaque dimension
        similarities = []
        
        # Position
        if 'position' in state1 and 'position' in state2:
            pos_dist = np.linalg.norm(
                np.array(state1['position']) - np.array(state2['position'])
            )
            pos_sim = 1.0 / (1.0 + pos_dist)
            similarities.append(pos_sim)
        
        # Shape
        if 'shape' in state1 and 'shape' in state2:
            shape_sim = 1.0 if state1['shape'] == state2['shape'] else 0.3
            similarities.append(shape_sim)
        
        # Color
        if 'color' in state1 and 'color' in state2:
            color_sim = 1.0 if state1['color'] == state2['color'] else 0.5
            similarities.append(color_sim)
        
        # Score global
        overall_sim = np.mean(similarities) if similarities else 0.0
        
        # Seuil continuité: >0.6 = même entité
        is_continuous = overall_sim > 0.6
        
        return is_continuous, float(overall_sim)
    
    def classify_transformation(self,
                               state1: Dict,
                               state2: Dict) -> TransformationType:
        """
        Classifie type de transformation
        
        Returns:
            TransformationType (REVERSIBLE, SEMI_REVERSIBLE, IRREVERSIBLE)
        """
        # Vérifier préservation information
        info_preserved = True
        
        # Shape change = semi-reversible
        if state1.get('shape') != state2.get('shape'):
            info_preserved = False
        
        # Size change significatif = semi-reversible
        if 'size' in state1 and 'size' in state2:
            size1 = np.prod(state1['size'])
            size2 = np.prod(state2['size'])
            if abs(size1 - size2) / max(size1, size2) > 0.5:
                info_preserved = False
        
        # Color change = reversible (juste modulation)
        # Position change = reversible (translation)
        
        if not info_preserved:
            return TransformationType.SEMI_REVERSIBLE
        else:
            return TransformationType.REVERSIBLE
    
    def simulate_future_states(self,
                              current_state: Dict,
                              num_steps: int = 3) -> List[Dict]:
        """
        Simule états futurs possibles (Monte-Carlo)
        
        Utilisé par CognitiveStrategyMap pour élimination précoce impasses
        """
        future_states = []
        
        for step in range(num_steps):
            # Appliquer transformations possibles
            for rule in self.composition_rules:
                # Simuler application règle
                new_state = current_state.copy()
                
                # Exemple: translation aléatoire
                if 'position' in new_state:
                    pos = new_state['position']
                    new_state['position'] = (
                        pos[0] + np.random.randint(-1, 2),
                        pos[1] + np.random.randint(-1, 2)
                    )
                
                future_states.append(new_state)
        
        return future_states
    
    def get_transformation_trajectory(self,
                                     entity_id: str) -> List[Dict]:
        """Retourne trajectoire complète transformations entité"""
        return self.transformation_history.get(entity_id, [])
    
    def record_transformation(self,
                            entity_id: str,
                            from_state: Dict,
                            to_state: Dict,
                            transformation_type: str):
        """Enregistre transformation dans historique"""
        if entity_id not in self.transformation_history:
            self.transformation_history[entity_id] = []
        
        self.transformation_history[entity_id].append({
            'from': from_state,
            'to': to_state,
            'type': transformation_type,
            'continuity': self.detect_continuity_threshold(from_state, to_state)
        })


# Made with Bob - Session 49 - Correction #3