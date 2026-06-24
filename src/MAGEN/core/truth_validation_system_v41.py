"""
PHASE V41: Truth Validation System
===================================

PRIORITÉ CRITIQUE identifiée par analyse utilisateur:
"Le vrai critère est: Le modèle interne permet-il de faire des prédictions correctes?"

Ordre optimal de validation:
1. Vérifier que la représentation existe (✅ V40.3)
2. Vérifier qu'elle correspond au monde réel (🎯 V41 - CE MODULE)
3. Vérifier qu'elle est réutilisable (V41.2)
4. Vérifier qu'elle généralise (V41.3)

Ce module implémente les métriques de VÉRITÉ:
- Précision localisation: distance(position_estimée, position_réelle)
- Stabilité carte: corrélation(carte_t, carte_t+100)
- Cohérence régions: overlap(régions_découvertes, régions_réelles)
- Prédiction états: précision(état_prédit, état_réel)

Citation utilisateur clé:
"Un agent peut devenir extrêmement cohérent avec lui-même tout en étant faux."

Auteur: Bob (Assistant IA)
Date: 2026-06-16
Version: V41.0 (Truth Validation)
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime
import json
from collections import defaultdict


@dataclass
class GroundTruth:
    """Vérité terrain pour validation."""
    puzzle_id: str
    true_grid: np.ndarray  # Grille réelle
    true_agent_position: Tuple[int, int]  # Position réelle agent
    true_regions: Dict[str, List[Tuple[int, int]]]  # Régions réelles
    true_transformations: List[str]  # Transformations réelles applicables
    
    def to_dict(self) -> Dict:
        return {
            'puzzle_id': self.puzzle_id,
            'grid_shape': self.true_grid.shape,
            'agent_position': self.true_agent_position,
            'regions_count': len(self.true_regions),
            'transformations_count': len(self.true_transformations)
        }


@dataclass
class LocalizationAccuracy:
    """Précision de localisation de l'agent."""
    estimated_position: Tuple[int, int]
    true_position: Tuple[int, int]
    euclidean_distance: float
    manhattan_distance: int
    is_correct: bool  # Distance < seuil
    confidence: float
    
    def to_dict(self) -> Dict:
        return {
            'estimated': self.estimated_position,
            'true': self.true_position,
            'euclidean_distance': float(self.euclidean_distance),
            'manhattan_distance': self.manhattan_distance,
            'is_correct': self.is_correct,
            'confidence': float(self.confidence)
        }


@dataclass
class MapStability:
    """Stabilité de la carte dans le temps."""
    correlation_t0_t100: float
    correlation_t0_t200: float
    cells_changed: int
    cells_stable: int
    stability_score: float  # 0-1
    
    def to_dict(self) -> Dict:
        return {
            'correlation_t0_t100': float(self.correlation_t0_t100),
            'correlation_t0_t200': float(self.correlation_t0_t200),
            'cells_changed': self.cells_changed,
            'cells_stable': self.cells_stable,
            'stability_score': float(self.stability_score)
        }


@dataclass
class RegionCoherence:
    """Cohérence des régions découvertes."""
    discovered_regions: Dict[str, List[Tuple[int, int]]]
    true_regions: Dict[str, List[Tuple[int, int]]]
    overlap_score: float  # 0-1
    false_positives: int
    false_negatives: int
    precision: float
    recall: float
    
    def to_dict(self) -> Dict:
        return {
            'discovered_count': len(self.discovered_regions),
            'true_count': len(self.true_regions),
            'overlap_score': float(self.overlap_score),
            'false_positives': self.false_positives,
            'false_negatives': self.false_negatives,
            'precision': float(self.precision),
            'recall': float(self.recall)
        }


@dataclass
class PredictionAccuracy:
    """Précision des prédictions d'états futurs."""
    predicted_state: Any
    true_state: Any
    prediction_error: float
    is_correct: bool
    prediction_type: str  # "position", "transformation", "state"
    
    def to_dict(self) -> Dict:
        return {
            'prediction_error': float(self.prediction_error),
            'is_correct': self.is_correct,
            'prediction_type': self.prediction_type
        }


class TruthValidationSystem:
    """
    Système de validation de vérité V41.
    
    Répond à la question critique:
    "Le modèle interne correspond-il au monde réel?"
    
    Métriques implémentées:
    1. Précision localisation (<10% erreur)
    2. Stabilité carte (>0.9 corrélation)
    3. Cohérence régions (>80% overlap)
    4. Précision prédiction (>70% correct)
    """
    
    def __init__(self,
                 localization_threshold: float = 0.1,  # 10% erreur max
                 stability_threshold: float = 0.9,     # 90% corrélation min
                 coherence_threshold: float = 0.8,     # 80% overlap min
                 prediction_threshold: float = 0.7):   # 70% précision min
        """
        Initialise le système de validation.
        
        Args:
            localization_threshold: Seuil erreur localisation (fraction grid)
            stability_threshold: Seuil corrélation stabilité
            coherence_threshold: Seuil overlap régions
            prediction_threshold: Seuil précision prédictions
        """
        self.localization_threshold = localization_threshold
        self.stability_threshold = stability_threshold
        self.coherence_threshold = coherence_threshold
        self.prediction_threshold = prediction_threshold
        
        # Historique validations
        self.localization_history: List[LocalizationAccuracy] = []
        self.stability_history: List[MapStability] = []
        self.coherence_history: List[RegionCoherence] = []
        self.prediction_history: List[PredictionAccuracy] = []
        
        # Statistiques globales
        self.stats = {
            'total_validations': 0,
            'localization_accuracy': 0.0,
            'map_stability': 0.0,
            'region_coherence': 0.0,
            'prediction_accuracy': 0.0,
            'truth_score': 0.0  # Score global vérité
        }
        
        # Forensic
        self.forensic_events: List[Dict] = []
    
    def validate_localization(self,
                            estimated_pos: Tuple[int, int],
                            true_pos: Tuple[int, int],
                            confidence: float,
                            grid_size: Tuple[int, int]) -> LocalizationAccuracy:
        """
        Valide la précision de localisation de l'agent.
        
        Args:
            estimated_pos: Position estimée par l'agent
            true_pos: Position réelle
            confidence: Confiance de l'agent
            grid_size: Taille de la grille (pour normalisation)
        
        Returns:
            Résultat validation localisation
        """
        # Distance euclidienne
        euclidean = np.sqrt(
            (estimated_pos[0] - true_pos[0])**2 +
            (estimated_pos[1] - true_pos[1])**2
        )
        
        # Distance Manhattan
        manhattan = abs(estimated_pos[0] - true_pos[0]) + abs(estimated_pos[1] - true_pos[1])
        
        # Normaliser par taille grille
        max_distance = np.sqrt(grid_size[0]**2 + grid_size[1]**2)
        normalized_error = euclidean / max_distance
        
        # Vérifier si correct
        is_correct = normalized_error < self.localization_threshold
        
        result = LocalizationAccuracy(
            estimated_position=estimated_pos,
            true_position=true_pos,
            euclidean_distance=euclidean,
            manhattan_distance=manhattan,
            is_correct=is_correct,
            confidence=confidence
        )
        
        self.localization_history.append(result)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'localization_validated',
            'result': result.to_dict(),
            'normalized_error': float(normalized_error),
            'threshold': self.localization_threshold
        })
        
        return result
    
    def validate_map_stability(self,
                              map_t0: np.ndarray,
                              map_t100: np.ndarray,
                              map_t200: Optional[np.ndarray] = None) -> MapStability:
        """
        Valide la stabilité de la carte dans le temps.
        
        Args:
            map_t0: Carte au temps t=0
            map_t100: Carte au temps t=100
            map_t200: Carte au temps t=200 (optionnel)
        
        Returns:
            Résultat validation stabilité
        """
        # Corrélation t0-t100
        flat_t0 = map_t0.flatten()
        flat_t100 = map_t100.flatten()
        
        if len(flat_t0) > 0 and len(flat_t100) > 0:
            corr_t100 = np.corrcoef(flat_t0, flat_t100)[0, 1]
        else:
            corr_t100 = 0.0
        
        # Corrélation t0-t200 si disponible
        corr_t200 = 0.0
        if map_t200 is not None:
            flat_t200 = map_t200.flatten()
            if len(flat_t0) > 0 and len(flat_t200) > 0:
                corr_t200 = np.corrcoef(flat_t0, flat_t200)[0, 1]
        
        # Compter cellules changées
        cells_changed = np.sum(map_t0 != map_t100)
        cells_stable = np.sum(map_t0 == map_t100)
        total_cells = map_t0.size
        
        # Score stabilité
        stability_score = cells_stable / total_cells if total_cells > 0 else 0.0
        
        result = MapStability(
            correlation_t0_t100=corr_t100,
            correlation_t0_t200=corr_t200,
            cells_changed=int(cells_changed),
            cells_stable=int(cells_stable),
            stability_score=stability_score
        )
        
        self.stability_history.append(result)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'stability_validated',
            'result': result.to_dict(),
            'threshold': self.stability_threshold
        })
        
        return result
    
    def validate_region_coherence(self,
                                 discovered_regions: Dict[str, List[Tuple[int, int]]],
                                 true_regions: Dict[str, List[Tuple[int, int]]]) -> RegionCoherence:
        """
        Valide la cohérence des régions découvertes.
        
        Args:
            discovered_regions: Régions découvertes par l'agent
            true_regions: Régions réelles
        
        Returns:
            Résultat validation cohérence
        """
        # Convertir en sets pour calcul overlap
        discovered_cells = set()
        for cells in discovered_regions.values():
            discovered_cells.update(cells)
        
        true_cells = set()
        for cells in true_regions.values():
            true_cells.update(cells)
        
        # Calcul overlap
        intersection = discovered_cells & true_cells
        union = discovered_cells | true_cells
        
        overlap_score = len(intersection) / len(union) if len(union) > 0 else 0.0
        
        # False positives/negatives
        false_positives = len(discovered_cells - true_cells)
        false_negatives = len(true_cells - discovered_cells)
        
        # Precision/Recall
        precision = len(intersection) / len(discovered_cells) if len(discovered_cells) > 0 else 0.0
        recall = len(intersection) / len(true_cells) if len(true_cells) > 0 else 0.0
        
        result = RegionCoherence(
            discovered_regions=discovered_regions,
            true_regions=true_regions,
            overlap_score=overlap_score,
            false_positives=false_positives,
            false_negatives=false_negatives,
            precision=precision,
            recall=recall
        )
        
        self.coherence_history.append(result)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'coherence_validated',
            'result': result.to_dict(),
            'threshold': self.coherence_threshold
        })
        
        return result
    
    def validate_prediction(self,
                          predicted_state: Any,
                          true_state: Any,
                          prediction_type: str = "state") -> PredictionAccuracy:
        """
        Valide la précision d'une prédiction.
        
        Args:
            predicted_state: État prédit
            true_state: État réel
            prediction_type: Type de prédiction
        
        Returns:
            Résultat validation prédiction
        """
        # Calcul erreur selon type
        if isinstance(predicted_state, (tuple, list)) and isinstance(true_state, (tuple, list)):
            # Position
            error = np.sqrt(sum((p - t)**2 for p, t in zip(predicted_state, true_state)))
            is_correct = error < 2.0  # Seuil 2 cellules
        elif isinstance(predicted_state, np.ndarray) and isinstance(true_state, np.ndarray):
            # État grille
            error = np.mean(np.abs(predicted_state - true_state))
            is_correct = error < 0.1
        else:
            # Autre (transformation, etc.)
            error = 0.0 if predicted_state == true_state else 1.0
            is_correct = predicted_state == true_state
        
        result = PredictionAccuracy(
            predicted_state=predicted_state,
            true_state=true_state,
            prediction_error=float(error),
            is_correct=bool(is_correct),
            prediction_type=prediction_type
        )
        
        self.prediction_history.append(result)
        
        # Forensic
        self.forensic_events.append({
            'timestamp': datetime.now().timestamp(),
            'event_type': 'prediction_validated',
            'result': result.to_dict(),
            'threshold': self.prediction_threshold
        })
        
        return result
    
    def compute_truth_score(self) -> float:
        """
        Calcule le score global de vérité.
        
        Combine les 4 métriques:
        - 25% localisation
        - 25% stabilité
        - 25% cohérence
        - 25% prédiction
        
        Returns:
            Score vérité 0-1
        """
        # Localisation
        if self.localization_history:
            loc_score = sum(1 for r in self.localization_history if r.is_correct) / len(self.localization_history)
        else:
            loc_score = 0.0
        
        # Stabilité
        if self.stability_history:
            stab_score = np.mean([r.stability_score for r in self.stability_history])
        else:
            stab_score = 0.0
        
        # Cohérence
        if self.coherence_history:
            coh_score = np.mean([r.overlap_score for r in self.coherence_history])
        else:
            coh_score = 0.0
        
        # Prédiction
        if self.prediction_history:
            pred_score = sum(1 for r in self.prediction_history if r.is_correct) / len(self.prediction_history)
        else:
            pred_score = 0.0
        
        # Score global
        truth_score = 0.25 * loc_score + 0.25 * stab_score + 0.25 * coh_score + 0.25 * pred_score
        
        # Mise à jour stats
        self.stats['localization_accuracy'] = loc_score
        self.stats['map_stability'] = stab_score
        self.stats['region_coherence'] = coh_score
        self.stats['prediction_accuracy'] = pred_score
        self.stats['truth_score'] = truth_score
        self.stats['total_validations'] = (
            len(self.localization_history) +
            len(self.stability_history) +
            len(self.coherence_history) +
            len(self.prediction_history)
        )
        
        return float(truth_score)
    
    def get_validation_report(self) -> Dict:
        """
        Génère rapport de validation complet.
        
        Returns:
            Rapport avec toutes les métriques
        """
        truth_score = self.compute_truth_score()
        
        return {
            'truth_score': float(truth_score),
            'metrics': {
                'localization': {
                    'accuracy': float(self.stats['localization_accuracy']),
                    'threshold': self.localization_threshold,
                    'passed': self.stats['localization_accuracy'] >= (1 - self.localization_threshold),
                    'samples': len(self.localization_history)
                },
                'stability': {
                    'score': float(self.stats['map_stability']),
                    'threshold': self.stability_threshold,
                    'passed': self.stats['map_stability'] >= self.stability_threshold,
                    'samples': len(self.stability_history)
                },
                'coherence': {
                    'score': float(self.stats['region_coherence']),
                    'threshold': self.coherence_threshold,
                    'passed': self.stats['region_coherence'] >= self.coherence_threshold,
                    'samples': len(self.coherence_history)
                },
                'prediction': {
                    'accuracy': float(self.stats['prediction_accuracy']),
                    'threshold': self.prediction_threshold,
                    'passed': self.stats['prediction_accuracy'] >= self.prediction_threshold,
                    'samples': len(self.prediction_history)
                }
            },
            'total_validations': self.stats['total_validations'],
            'verdict': 'TRUTH_VALIDATED' if truth_score >= 0.7 else 'TRUTH_FAILED'
        }
    
    def save_forensic_log(self, filepath: str):
        """Sauvegarde log forensique."""
        with open(filepath, 'w') as f:
            for event in self.forensic_events:
                f.write(json.dumps(event) + '\n')

# Made with Bob
