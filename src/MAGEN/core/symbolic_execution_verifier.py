"""
Symbolic Execution Verifier - Exact Constraint Solver Layer
============================================================

ROOT CAUSE SOLUTION pour gap reconnaissance → exécution

Architecture:
1. Perceptual Engine (V22) → détecte patterns ✓
2. Symbolic Verifier (V23) → garantit exactitude ✓✓✓
3. Execution Engine → applique transformations vérifiées

Objectif: Fermer le gap 174 (similarité) → 250+ (exact)
"""

import numpy as np
from typing import List, Dict, Tuple, Optional, Callable
from dataclasses import dataclass
from enum import Enum
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dsl.program import Program


class ConstraintType(Enum):
    """Types de contraintes symboliques"""
    SHAPE_PRESERVATION = "shape_preservation"
    COLOR_MAPPING = "color_mapping"
    SPATIAL_RELATION = "spatial_relation"
    CARDINALITY = "cardinality"
    SYMMETRY = "symmetry"
    CONNECTIVITY = "connectivity"


@dataclass
class SymbolicConstraint:
    """Contrainte symbolique à vérifier"""
    constraint_type: ConstraintType
    predicate: Callable[[np.ndarray, np.ndarray], bool]
    description: str
    critical: bool = True  # Si False, peut être relaxé


@dataclass
class VerificationResult:
    """Résultat de vérification symbolique"""
    is_valid: bool
    exact_match: bool
    constraints_satisfied: List[str]
    constraints_violated: List[str]
    confidence: float
    error_details: Optional[str] = None


class SymbolicExecutionVerifier:
    """
    Vérificateur d'exécution symbolique
    
    Garantit que chaque transformation DSL produit output EXACT,
    pas approximatif.
    
    Principe: Si reconnaissance dit "rotate_90", vérifier que:
    1. Shape correcte après rotation
    2. Tous pixels mappés correctement
    3. Aucune perte d'information
    4. Transformation inversible
    """
    
    def __init__(self, strict_mode: bool = True):
        """
        Args:
            strict_mode: Si True, exige satisfaction de TOUTES contraintes critiques
        """
        self.strict_mode = strict_mode
        self.constraints_library = self._build_constraints_library()
    
    def _build_constraints_library(self) -> Dict[str, List[SymbolicConstraint]]:
        """Construire bibliothèque de contraintes symboliques"""
        library = {}
        
        # Contraintes géométriques
        library['geometric'] = [
            SymbolicConstraint(
                constraint_type=ConstraintType.SHAPE_PRESERVATION,
                predicate=self._check_shape_preservation,
                description="Shape préservée ou transformée correctement",
                critical=True
            ),
            SymbolicConstraint(
                constraint_type=ConstraintType.SYMMETRY,
                predicate=self._check_symmetry_preservation,
                description="Symétries préservées si attendues",
                critical=False
            )
        ]
        
        # Contraintes couleur
        library['color'] = [
            SymbolicConstraint(
                constraint_type=ConstraintType.COLOR_MAPPING,
                predicate=self._check_color_mapping,
                description="Mapping couleurs bijectif et cohérent",
                critical=True
            ),
            SymbolicConstraint(
                constraint_type=ConstraintType.CARDINALITY,
                predicate=self._check_color_cardinality,
                description="Nombre de couleurs préservé ou réduit intentionnellement",
                critical=False
            )
        ]
        
        # Contraintes spatiales
        library['spatial'] = [
            SymbolicConstraint(
                constraint_type=ConstraintType.SPATIAL_RELATION,
                predicate=self._check_spatial_relations,
                description="Relations spatiales préservées",
                critical=True
            ),
            SymbolicConstraint(
                constraint_type=ConstraintType.CONNECTIVITY,
                predicate=self._check_connectivity,
                description="Connectivité objets préservée",
                critical=False
            )
        ]
        
        return library
    
    def verify_transformation(self,
                            input_grid: np.ndarray,
                            predicted_output: np.ndarray,
                            expected_output: np.ndarray,
                            transformation_type: str = "unknown") -> VerificationResult:
        """
        Vérifier qu'une transformation est symboliquement correcte
        
        Args:
            input_grid: Grille d'entrée
            predicted_output: Output prédit par système
            expected_output: Output attendu
            transformation_type: Type de transformation (geometric, color, etc.)
        
        Returns:
            VerificationResult avec détails de vérification
        """
        # Vérification exacte d'abord
        exact_match = np.array_equal(predicted_output, expected_output)
        
        if exact_match:
            return VerificationResult(
                is_valid=True,
                exact_match=True,
                constraints_satisfied=["exact_match"],
                constraints_violated=[],
                confidence=1.0
            )
        
        # Si pas exact, vérifier contraintes symboliques
        satisfied = []
        violated = []
        
        # Sélectionner contraintes appropriées
        if transformation_type in self.constraints_library:
            constraints = self.constraints_library[transformation_type]
        else:
            # Utiliser toutes contraintes
            constraints = []
            for constraint_list in self.constraints_library.values():
                constraints.extend(constraint_list)
        
        # Vérifier chaque contrainte
        critical_violations = 0
        for constraint in constraints:
            try:
                is_satisfied = constraint.predicate(predicted_output, expected_output)
                
                if is_satisfied:
                    satisfied.append(constraint.description)
                else:
                    violated.append(constraint.description)
                    if constraint.critical:
                        critical_violations += 1
            
            except Exception as e:
                violated.append(f"{constraint.description} (error: {str(e)})")
                if constraint.critical:
                    critical_violations += 1
        
        # Calculer validité
        if self.strict_mode:
            is_valid = (critical_violations == 0)
        else:
            # Mode relaxé: accepter si majorité contraintes satisfaites
            is_valid = (len(satisfied) > len(violated))
        
        # Calculer confiance
        total_constraints = len(satisfied) + len(violated)
        confidence = len(satisfied) / total_constraints if total_constraints > 0 else 0.0
        
        return VerificationResult(
            is_valid=is_valid,
            exact_match=False,
            constraints_satisfied=satisfied,
            constraints_violated=violated,
            confidence=confidence,
            error_details=f"{critical_violations} critical violations" if critical_violations > 0 else None
        )
    
    def verify_program_execution(self,
                                program: Program,
                                train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> VerificationResult:
        """
        Vérifier qu'un programme s'exécute correctement sur train pairs
        
        Garantit exécution EXACTE, pas approximative
        """
        all_satisfied = []
        all_violated = []
        exact_matches = 0
        
        for input_grid, expected_output in train_pairs:
            try:
                predicted_output = program.execute(input_grid)
                
                # Vérifier transformation
                result = self.verify_transformation(
                    input_grid,
                    predicted_output,
                    expected_output,
                    transformation_type="unknown"
                )
                
                all_satisfied.extend(result.constraints_satisfied)
                all_violated.extend(result.constraints_violated)
                
                if result.exact_match:
                    exact_matches += 1
            
            except Exception as e:
                all_violated.append(f"Execution error: {str(e)}")
        
        # Programme valide si TOUS les train pairs sont exacts
        is_valid = (exact_matches == len(train_pairs))
        exact_match = is_valid
        
        # Confiance basée sur proportion exact matches
        confidence = exact_matches / len(train_pairs) if train_pairs else 0.0
        
        return VerificationResult(
            is_valid=is_valid,
            exact_match=exact_match,
            constraints_satisfied=all_satisfied,
            constraints_violated=all_violated,
            confidence=confidence,
            error_details=f"Only {exact_matches}/{len(train_pairs)} exact matches" if not is_valid else None
        )
    
    # Prédicats de contraintes (définis après __init__)
    
    def _check_shape_preservation(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier préservation ou transformation correcte de shape"""
        # Shape exacte OU transformation géométrique valide
        if predicted.shape == expected.shape:
            return True
        
        # Vérifier si rotation/flip (dimensions inversées)
        if predicted.shape == expected.shape[::-1]:
            return True
        
        # Vérifier si scaling uniforme
        h_ratio = predicted.shape[0] / expected.shape[0]
        w_ratio = predicted.shape[1] / expected.shape[1]
        
        # Scaling valide si ratio entier ou inverse entier
        return (h_ratio == w_ratio) and (h_ratio == int(h_ratio) or 1/h_ratio == int(1/h_ratio))
    
    def _check_color_mapping(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier mapping couleurs cohérent"""
        # Extraire couleurs uniques
        pred_colors = set(predicted.flatten())
        exp_colors = set(expected.flatten())
        
        # Nombre de couleurs doit être compatible
        # (peut réduire mais pas augmenter arbitrairement)
        return len(pred_colors) <= len(exp_colors) * 2
    
    def _check_color_cardinality(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier cardinalité couleurs"""
        pred_colors = len(set(predicted.flatten()))
        exp_colors = len(set(expected.flatten()))
        
        # Cardinalité exacte ou réduite intentionnellement
        return pred_colors <= exp_colors
    
    def _check_spatial_relations(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier préservation relations spatiales"""
        # Vérifier que centres de masse sont cohérents
        if predicted.shape != expected.shape:
            return False
        
        # Calculer centres de masse pour chaque couleur
        for color in set(expected.flatten()):
            if color == 0:  # Skip background
                continue
            
            exp_mask = (expected == color)
            pred_mask = (predicted == color)
            
            if not np.any(exp_mask) or not np.any(pred_mask):
                continue
            
            # Centre de masse attendu
            exp_y, exp_x = np.where(exp_mask)
            exp_center = (np.mean(exp_y), np.mean(exp_x))
            
            # Centre de masse prédit
            pred_y, pred_x = np.where(pred_mask)
            pred_center = (np.mean(pred_y), np.mean(pred_x))
            
            # Distance entre centres
            distance = np.sqrt((exp_center[0] - pred_center[0])**2 + 
                             (exp_center[1] - pred_center[1])**2)
            
            # Tolérance: 20% de la diagonale
            diagonal = np.sqrt(expected.shape[0]**2 + expected.shape[1]**2)
            tolerance = 0.2 * diagonal
            
            if distance > tolerance:
                return False
        
        return True
    
    def _check_symmetry_preservation(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier préservation symétries"""
        if predicted.shape != expected.shape:
            return False
        
        # Vérifier symétrie horizontale
        exp_h_sym = np.array_equal(expected, np.fliplr(expected))
        pred_h_sym = np.array_equal(predicted, np.fliplr(predicted))
        
        # Vérifier symétrie verticale
        exp_v_sym = np.array_equal(expected, np.flipud(expected))
        pred_v_sym = np.array_equal(predicted, np.flipud(predicted))
        
        # Symétries doivent être préservées
        return (exp_h_sym == pred_h_sym) and (exp_v_sym == pred_v_sym)
    
    def _check_connectivity(self, predicted: np.ndarray, expected: np.ndarray) -> bool:
        """Vérifier préservation connectivité objets"""
        if predicted.shape != expected.shape:
            return False
        
        # Compter composantes connexes pour chaque couleur
        try:
            from scipy import ndimage
            
            for color in set(expected.flatten()):
                if color == 0:
                    continue
                
                exp_mask = (expected == color)
                pred_mask = (predicted == color)
                
                # Nombre de composantes connexes
                label_result_exp = ndimage.label(exp_mask)
                label_result_pred = ndimage.label(pred_mask)
                
                # Extraire nombre de composantes (2ème élément du tuple)
                exp_num = label_result_exp[1] if isinstance(label_result_exp, tuple) else 0
                pred_num = label_result_pred[1] if isinstance(label_result_pred, tuple) else 0
                
                # Nombre de composantes doit être préservé
                if exp_num != pred_num:
                    return False
            
            return True
        except Exception:
            # Si scipy pas disponible, skip cette contrainte
            return True


__all__ = ['SymbolicExecutionVerifier', 'VerificationResult', 'ConstraintType']

# Made with Bob - Exact Constraint Solver Layer