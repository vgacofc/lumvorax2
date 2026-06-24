"""
PHASE V41: Predictive World Model
==================================

OBJECTIF CRITIQUE identifié par analyse utilisateur:
"Le vrai critère est: Le modèle interne permet-il de faire des prédictions correctes?"

Ce module implémente les capacités prédictives:
1. Prédire position après action
2. Prédire état grille après transformation
3. Prédire régions découvrables
4. Prédire stratégies efficaces

Citation utilisateur clé:
"Si MAGEN peut dire où il sera après une action, quelles régions sont déjà connues,
quelles régions sont probablement stériles, quelle stratégie a le plus de chances
d'aboutir, alors on pourra commencer à parler de compréhension."

Auteur: Bob (Assistant IA)
Date: 2026-06-16
Version: V41.0 (Predictive Capabilities)
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime
import json
from collections import defaultdict


@dataclass
class PositionPrediction:
    """Prédiction de position future."""
    current_position: Tuple[int, int]
    action: str
    predicted_position: Tuple[int, int]
    confidence: float
    reasoning: str
    
    def to_dict(self) -> Dict:
        return {
            'current': self.current_position,
            'action': self.action,
            'predicted': self.predicted_position,
            'confidence': float(self.confidence),
            'reasoning': self.reasoning
        }


@dataclass
class StatePrediction:
    """Prédiction d'état futur."""
    current_state: np.ndarray
    transformation: str
    predicted_state: np.ndarray
    confidence: float
    reasoning: str
    
    def to_dict(self) -> Dict:
        return {
            'transformation': self.transformation,
            'confidence': float(self.confidence),
            'reasoning': self.reasoning,
            'state_shape': self.predicted_state.shape
        }


@dataclass
class RegionPrediction:
    """Prédiction de régions découvrables."""
    known_regions: List[str]
    predicted_regions: List[Tuple[str, float]]  # (region_id, probability)
    sterile_regions: List[str]
    reasoning: str
    
    def to_dict(self) -> Dict:
        return {
            'known_count': len(self.known_regions),
            'predicted_count': len(self.predicted_regions),
            'sterile_count': len(self.sterile_regions),
            'reasoning': self.reasoning
        }


@dataclass
class StrategyPrediction:
    """Prédiction de stratégie efficace."""
    current_context: Dict
    predicted_strategy: str
    expected_success_rate: float
    alternative_strategies: List[Tuple[str, float]]  # (strategy, success_rate)
    reasoning: str
    
    def to_dict(self) -> Dict:
        return {
            'strategy': self.predicted_strategy,
            'expected_success': float(self.expected_success_rate),
            'alternatives_count': len(self.alternative_strategies),
            'reasoning': self.reasoning
        }


class PredictiveWorldModel:
    """
    Modèle du monde prédictif V41.
    
    Capacités prédictives:
    1. Position future après action
    2. État grille après transformation
    3. Régions découvrables
    4. Stratégies efficaces
    
    Permet de tester la COMPRÉHENSION vs simple représentation.
    """
    
    def __init__(self,
                 world_graph: Any,
                 agent_loc: Any,
                 causal_mem: Any,
                 learning_sys: Any):
        """
        Initialise le modèle prédictif.
        
        Args:
            world_graph: Graphe d'état du monde
            agent_loc: Système de localisation
            causal_mem: Mémoire causale
            learning_sys: Système d'apprentissage
        """
        self.world_graph = world_graph
        self.agent_loc = agent_loc
        self.causal_mem = causal_mem
        self.learning_sys = learning_sys
        
        # Historique prédictions
        self.position_predictions: List[PositionPrediction] = []
        self.state_predictions: List[StatePrediction] = []
        self.region_predictions: List[RegionPrediction] = []
        self.strategy_predictions: List[StrategyPrediction] = []
        
        # Statistiques prédictions
        self.stats = {
            'total_predictions': 0,
            'position_accuracy': 0.0,
            'state_accuracy': 0.0,
            'region_accuracy': 0.0,
            'strategy_accuracy': 0.0
        }
        
        # Forensic
        self.forensic_events: List[Dict] = []
    
    def predict_position_after_action(self,
                                     current_pos: Tuple[int, int],
                                     action: str) -> PositionPrediction:
        """
        Prédit la position après une action.
        
        Args:
            current_pos: Position actuelle
            action: Action à effectuer
        
        Returns:
            Prédiction de position
        """
        # Règles de mouvement basiques
        movement_map = {
            'translate_up': (0, -1),
            'translate_down': (0, 1),
            'translate_left': (-1, 0),
            'translate_right': (1, 0)
        }
        
        if action in movement_map:
            dx, dy = movement_map[action]
            predicted_pos = (current_pos[0] + dx, current_pos[1] + dy)
            confidence = 0.9  # Haute confiance pour mouvements directs
            reasoning = f"Direct movement: {action}"
        else:
            # Transformations sans mouvement
            predicted_pos = current_pos
            confidence = 0.95
            reasoning = f"No movement for transformation: {action}"
        
        # Vérifier limites grille
        if hasattr(self.world_graph, 'width') and hasattr(self.world_graph, 'height'):
            predicted_pos = (
                max(0, min(predicted_pos[0], self.world_graph.width - 1)),
                max(0, min(predicted_pos[1], self.world_graph.height - 1))
            )
        
        prediction = PositionPrediction(
            current_position=current_pos,
            action=action,
            predicted_position=predicted_pos,
            confidence=confidence,
            reasoning=reasoning
        )
        
        self.position_predictions.append(prediction)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'position_predicted',
            'prediction': prediction.to_dict()
        })
        
        return prediction
    
    def predict_state_after_transformation(self,
                                          current_state: np.ndarray,
                                          transformation: str) -> StatePrediction:
        """
        Prédit l'état de la grille après transformation.
        
        Args:
            current_state: État actuel
            transformation: Transformation à appliquer
        
        Returns:
            Prédiction d'état
        """
        # Appliquer transformation basique
        if transformation == 'rotate_90':
            predicted_state = np.rot90(current_state)
            confidence = 0.95
            reasoning = "Rotation 90° clockwise"
        elif transformation == 'rotate_180':
            predicted_state = np.rot90(current_state, 2)
            confidence = 0.95
            reasoning = "Rotation 180°"
        elif transformation == 'rotate_270':
            predicted_state = np.rot90(current_state, 3)
            confidence = 0.95
            reasoning = "Rotation 270° clockwise"
        elif transformation == 'flip_horizontal':
            predicted_state = np.fliplr(current_state)
            confidence = 0.95
            reasoning = "Horizontal flip"
        elif transformation == 'flip_vertical':
            predicted_state = np.flipud(current_state)
            confidence = 0.95
            reasoning = "Vertical flip"
        elif transformation == 'identity':
            predicted_state = current_state.copy()
            confidence = 1.0
            reasoning = "Identity (no change)"
        else:
            # Transformation inconnue
            predicted_state = current_state.copy()
            confidence = 0.5
            reasoning = f"Unknown transformation: {transformation}"
        
        prediction = StatePrediction(
            current_state=current_state,
            transformation=transformation,
            predicted_state=predicted_state,
            confidence=confidence,
            reasoning=reasoning
        )
        
        self.state_predictions.append(prediction)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'state_predicted',
            'prediction': prediction.to_dict()
        })
        
        return prediction
    
    def predict_discoverable_regions(self) -> RegionPrediction:
        """
        Prédit quelles régions sont découvrables.
        
        Returns:
            Prédiction de régions
        """
        # Régions connues
        known_regions = list(self.world_graph.regions.keys()) if hasattr(self.world_graph, 'regions') else []
        
        # Prédire nouvelles régions basé sur couverture
        coverage = self.world_graph.get_coverage() if hasattr(self.world_graph, 'get_coverage') else 0.0
        
        predicted_regions = []
        if coverage < 0.3:
            # Faible couverture: beaucoup à découvrir
            predicted_regions = [
                ('unexplored_north', 0.8),
                ('unexplored_south', 0.8),
                ('unexplored_east', 0.7),
                ('unexplored_west', 0.7)
            ]
            reasoning = "Low coverage (<30%), many regions likely undiscovered"
        elif coverage < 0.7:
            # Couverture moyenne
            predicted_regions = [
                ('unexplored_corners', 0.6),
                ('hidden_patterns', 0.5)
            ]
            reasoning = "Medium coverage (30-70%), some regions may remain"
        else:
            # Haute couverture
            predicted_regions = [
                ('edge_cases', 0.3)
            ]
            reasoning = "High coverage (>70%), most regions likely discovered"
        
        # Régions stériles (basé sur historique)
        sterile_regions = []
        if hasattr(self.causal_mem, 'get_failed_transformations'):
            failed = self.causal_mem.get_failed_transformations()
            if len(failed) > 10:
                sterile_regions = ['high_failure_zone']
        
        prediction = RegionPrediction(
            known_regions=known_regions,
            predicted_regions=predicted_regions,
            sterile_regions=sterile_regions,
            reasoning=reasoning
        )
        
        self.region_predictions.append(prediction)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'regions_predicted',
            'prediction': prediction.to_dict()
        })
        
        return prediction
    
    def predict_effective_strategy(self,
                                  context: Dict) -> StrategyPrediction:
        """
        Prédit quelle stratégie sera efficace.
        
        Args:
            context: Contexte actuel (coverage, patterns, etc.)
        
        Returns:
            Prédiction de stratégie
        """
        coverage = context.get('coverage', 0.0)
        patterns_count = context.get('patterns_count', 0)
        step_count = context.get('step_count', 0)
        
        # Décision stratégique basée sur contexte
        if coverage < 0.3:
            # Exploration prioritaire
            strategy = "aggressive_exploration"
            expected_success = 0.7
            alternatives = [
                ("balanced_exploration", 0.5),
                ("pattern_exploitation", 0.3)
            ]
            reasoning = "Low coverage requires exploration"
        elif patterns_count > 5:
            # Exploitation patterns
            strategy = "pattern_exploitation"
            expected_success = 0.8
            alternatives = [
                ("balanced_exploration", 0.6),
                ("aggressive_exploration", 0.4)
            ]
            reasoning = "Many patterns discovered, exploit them"
        elif step_count > 150:
            # Fin de partie: exploitation
            strategy = "focused_exploitation"
            expected_success = 0.6
            alternatives = [
                ("pattern_exploitation", 0.5),
                ("last_resort_exploration", 0.3)
            ]
            reasoning = "Late game, focus on known patterns"
        else:
            # Équilibré
            strategy = "balanced_exploration"
            expected_success = 0.65
            alternatives = [
                ("aggressive_exploration", 0.6),
                ("pattern_exploitation", 0.6)
            ]
            reasoning = "Balanced approach for current context"
        
        prediction = StrategyPrediction(
            current_context=context,
            predicted_strategy=strategy,
            expected_success_rate=expected_success,
            alternative_strategies=alternatives,
            reasoning=reasoning
        )
        
        self.strategy_predictions.append(prediction)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'strategy_predicted',
            'prediction': prediction.to_dict()
        })
        
        return prediction
    
    def get_prediction_statistics(self) -> Dict:
        """
        Retourne statistiques des prédictions.
        
        Returns:
            Statistiques complètes
        """
        return {
            'total_predictions': (
                len(self.position_predictions) +
                len(self.state_predictions) +
                len(self.region_predictions) +
                len(self.strategy_predictions)
            ),
            'by_type': {
                'position': len(self.position_predictions),
                'state': len(self.state_predictions),
                'region': len(self.region_predictions),
                'strategy': len(self.strategy_predictions)
            },
            'avg_confidence': {
                'position': np.mean([p.confidence for p in self.position_predictions]) if self.position_predictions else 0.0,
                'state': np.mean([p.confidence for p in self.state_predictions]) if self.state_predictions else 0.0
            }
        }
    
    def save_forensic_log(self, filepath: str):
        """Sauvegarde log forensique."""
        with open(filepath, 'w') as f:
            for event in self.forensic_events:
                f.write(json.dumps(event) + '\n')

# Made with Bob
