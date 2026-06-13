#!/usr/bin/env python3
"""
Trajectory Divergence Analyzer - Détecte OÙ et POURQUOI programmes divergent
Inspiré par jeux arcade: identifier le moment où la trajectoire devient fausse
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass
from enum import Enum


class DivergenceType(Enum):
    """Types de divergence détectés"""
    SHAPE_MISMATCH = "shape_mismatch"
    COLOR_WRONG = "color_wrong"
    POSITION_WRONG = "position_wrong"
    OBJECT_MISSING = "object_missing"
    OBJECT_EXTRA = "object_extra"
    PARTIAL_CORRECT = "partial_correct"
    LOGIC_ERROR = "logic_error"
    UNKNOWN = "unknown"


@dataclass
class DivergencePoint:
    """Point de divergence identifié"""
    location: Tuple[int, int]  # (row, col)
    divergence_type: DivergenceType
    expected_value: int
    actual_value: int
    context: str
    severity: float  # 0.0 (minor) to 1.0 (critical)


@dataclass
class TrajectoryAnalysis:
    """Analyse complète de la trajectoire d'un programme"""
    program: Any
    train_pair_id: int
    success: bool
    
    # Métriques globales
    accuracy: float
    partial_correctness: float
    
    # Points de divergence
    divergence_points: List[DivergencePoint]
    first_divergence: Optional[DivergencePoint]
    
    # Analyse spatiale
    divergence_regions: List[Tuple[int, int, int, int]]  # (row_start, row_end, col_start, col_end)
    
    # Analyse temporelle (si applicable)
    divergence_step: Optional[int]  # À quelle étape la divergence commence
    
    # Diagnostic
    root_cause: str
    suggested_fix: str


class TrajectoryDivergenceAnalyzer:
    """
    Analyseur de divergence de trajectoire
    
    Inspiré par perception humaine dans jeux arcade:
    - Identifier le MOMENT où la trajectoire diverge
    - Comprendre POURQUOI (pas juste "échec")
    - Suggérer corrections basées sur analyse
    """
    
    def __init__(self):
        self.debug = False
    
    def analyze(self,
                program: Any,
                input_grid: np.ndarray,
                expected_output: np.ndarray,
                actual_output: np.ndarray,
                train_pair_id: int = 0) -> TrajectoryAnalysis:
        """
        Analyse la divergence entre sortie attendue et sortie réelle
        
        Args:
            program: Programme exécuté
            input_grid: Grille d'entrée
            expected_output: Sortie attendue
            actual_output: Sortie obtenue
            train_pair_id: ID du train pair
            
        Returns:
            TrajectoryAnalysis avec diagnostic complet
        """
        # Métriques globales
        success = np.array_equal(expected_output, actual_output)
        
        if success:
            return TrajectoryAnalysis(
                program=program,
                train_pair_id=train_pair_id,
                success=True,
                accuracy=1.0,
                partial_correctness=1.0,
                divergence_points=[],
                first_divergence=None,
                divergence_regions=[],
                divergence_step=None,
                root_cause="none",
                suggested_fix="none"
            )
        
        # Calculer métriques
        accuracy, partial_correctness = self._compute_metrics(expected_output, actual_output)
        
        # Détecter points de divergence
        divergence_points = self._detect_divergence_points(expected_output, actual_output)
        
        # Identifier première divergence
        first_divergence = divergence_points[0] if divergence_points else None
        
        # Identifier régions de divergence
        divergence_regions = self._identify_divergence_regions(expected_output, actual_output)
        
        # Diagnostiquer root cause
        root_cause = self._diagnose_root_cause(
            input_grid, expected_output, actual_output, divergence_points
        )
        
        # Suggérer fix
        suggested_fix = self._suggest_fix(root_cause, divergence_points, program)
        
        return TrajectoryAnalysis(
            program=program,
            train_pair_id=train_pair_id,
            success=False,
            accuracy=accuracy,
            partial_correctness=partial_correctness,
            divergence_points=divergence_points,
            first_divergence=first_divergence,
            divergence_regions=divergence_regions,
            divergence_step=None,  # TODO: Implémenter analyse temporelle
            root_cause=root_cause,
            suggested_fix=suggested_fix
        )
    
    def compare_trajectories(self,
                            analyses: List[TrajectoryAnalysis]) -> Dict[str, Any]:
        """
        Compare les trajectoires sur plusieurs train pairs
        
        Args:
            analyses: Liste d'analyses de trajectoire
            
        Returns:
            Dictionnaire avec comparaison et patterns communs
        """
        if not analyses:
            return {}
        
        comparison = {
            'total_pairs': len(analyses),
            'successes': sum(1 for a in analyses if a.success),
            'failures': sum(1 for a in analyses if not a.success),
            'avg_accuracy': np.mean([a.accuracy for a in analyses]),
            'avg_partial_correctness': np.mean([a.partial_correctness for a in analyses])
        }
        
        # Analyser patterns de divergence
        failed_analyses = [a for a in analyses if not a.success]
        
        if failed_analyses:
            # Root causes communes
            root_causes = [a.root_cause for a in failed_analyses]
            from collections import Counter
            root_cause_counter = Counter(root_causes)
            comparison['common_root_causes'] = root_cause_counter.most_common()
            
            # Types de divergence communs
            all_divergence_types = []
            for analysis in failed_analyses:
                all_divergence_types.extend([dp.divergence_type for dp in analysis.divergence_points])
            
            divergence_type_counter = Counter(all_divergence_types)
            comparison['common_divergence_types'] = divergence_type_counter.most_common()
            
            # Première divergence moyenne
            first_divergences = [a.first_divergence for a in failed_analyses if a.first_divergence]
            if first_divergences:
                avg_first_row = np.mean([dp.location[0] for dp in first_divergences])
                avg_first_col = np.mean([dp.location[1] for dp in first_divergences])
                comparison['avg_first_divergence_location'] = (avg_first_row, avg_first_col)
        
        return comparison
    
    def _compute_metrics(self,
                        expected: np.ndarray,
                        actual: np.ndarray) -> Tuple[float, float]:
        """
        Calcule métriques de précision
        
        Returns:
            (accuracy, partial_correctness)
        """
        # Accuracy: ratio pixels corrects
        if expected.shape != actual.shape:
            # Shapes différentes → accuracy basée sur intersection
            min_h = min(expected.shape[0], actual.shape[0])
            min_w = min(expected.shape[1], actual.shape[1])
            
            expected_crop = expected[:min_h, :min_w]
            actual_crop = actual[:min_h, :min_w]
            
            matches = np.sum(expected_crop == actual_crop)
            total = expected.size
            accuracy = matches / total if total > 0 else 0.0
        else:
            matches = np.sum(expected == actual)
            total = expected.size
            accuracy = matches / total if total > 0 else 0.0
        
        # Partial correctness: ratio régions correctes
        # (version simplifiée: même que accuracy pour l'instant)
        partial_correctness = accuracy
        
        return accuracy, partial_correctness
    
    def _detect_divergence_points(self,
                                  expected: np.ndarray,
                                  actual: np.ndarray) -> List[DivergencePoint]:
        """
        Détecte tous les points de divergence
        
        Returns:
            Liste de DivergencePoint triée par sévérité
        """
        divergence_points = []
        
        # Vérifier shape mismatch
        if expected.shape != actual.shape:
            divergence_points.append(DivergencePoint(
                location=(0, 0),
                divergence_type=DivergenceType.SHAPE_MISMATCH,
                expected_value=-1,
                actual_value=-1,
                context=f"Expected shape {expected.shape}, got {actual.shape}",
                severity=1.0
            ))
            return divergence_points
        
        # Détecter divergences pixel par pixel
        diff_mask = (expected != actual)
        diff_positions = np.argwhere(diff_mask)
        
        # Limiter à 100 premiers points pour performance
        for pos in diff_positions[:100]:
            row, col = int(pos[0]), int(pos[1])
            
            divergence_points.append(DivergencePoint(
                location=(row, col),
                divergence_type=DivergenceType.COLOR_WRONG,
                expected_value=int(expected[row, col]),
                actual_value=int(actual[row, col]),
                context=f"Pixel ({row},{col}): expected {expected[row, col]}, got {actual[row, col]}",
                severity=0.5
            ))
        
        # Trier par position (top-left first)
        divergence_points.sort(key=lambda dp: (dp.location[0], dp.location[1]))
        
        return divergence_points
    
    def _identify_divergence_regions(self,
                                    expected: np.ndarray,
                                    actual: np.ndarray) -> List[Tuple[int, int, int, int]]:
        """
        Identifie les régions de divergence (bounding boxes)
        
        Returns:
            Liste de (row_start, row_end, col_start, col_end)
        """
        if expected.shape != actual.shape:
            return [(0, expected.shape[0], 0, expected.shape[1])]
        
        diff_mask = (expected != actual)
        diff_positions = np.argwhere(diff_mask)
        
        if len(diff_positions) == 0:
            return []
        
        # Bounding box global
        row_min = int(np.min(diff_positions[:, 0]))
        row_max = int(np.max(diff_positions[:, 0]))
        col_min = int(np.min(diff_positions[:, 1]))
        col_max = int(np.max(diff_positions[:, 1]))
        
        return [(row_min, row_max + 1, col_min, col_max + 1)]
    
    def _diagnose_root_cause(self,
                            input_grid: np.ndarray,
                            expected: np.ndarray,
                            actual: np.ndarray,
                            divergence_points: List[DivergencePoint]) -> str:
        """
        Diagnostique la root cause de la divergence
        
        Returns:
            Description de la root cause
        """
        if not divergence_points:
            return "none"
        
        # Shape mismatch
        if expected.shape != actual.shape:
            return f"shape_mismatch: expected {expected.shape}, got {actual.shape}"
        
        # Analyser pattern de divergence
        num_divergences = len(divergence_points)
        total_pixels = expected.size
        divergence_ratio = num_divergences / total_pixels
        
        if divergence_ratio < 0.1:
            return "local_error: <10% pixels wrong"
        elif divergence_ratio < 0.5:
            return "partial_error: 10-50% pixels wrong"
        else:
            return "global_error: >50% pixels wrong"
    
    def _suggest_fix(self,
                    root_cause: str,
                    divergence_points: List[DivergencePoint],
                    program: Any) -> str:
        """
        Suggère un fix basé sur l'analyse
        
        Returns:
            Description du fix suggéré
        """
        if root_cause == "none":
            return "none"
        
        if "shape_mismatch" in root_cause:
            return "Add resize/crop operation to match expected shape"
        
        if "local_error" in root_cause:
            return "Adjust local transformation parameters"
        
        if "partial_error" in root_cause:
            return "Review selection/filtering logic"
        
        if "global_error" in root_cause:
            return "Reconsider entire transformation approach"
        
        return "unknown"

# Made with Bob
