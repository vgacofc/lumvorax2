"""
MAGEN - Symbolic Verifier
Vérification symbolique EXACTE des transformations

Principe:
- Vérification EXACTE (pas approximative)
- Explication des échecs
- Détection de patterns d'erreurs
- Métriques de similarité pour debugging
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional
from dataclasses import dataclass

from .primitive_transforms import Transform, PrimitiveTransforms
from .transform_composer import ComposedTransform, Hypothesis, TransformComposer


@dataclass
class VerificationResult:
    """Résultat de vérification"""
    success: bool
    exact_match: bool
    similarity: float  # 0.0 à 1.0
    explanation: str
    diff_pixels: int
    total_pixels: int
    shape_match: bool
    color_distribution_match: bool
    
    def __repr__(self):
        status = "✓ SUCCESS" if self.success else "✗ FAILED"
        return f"{status} (sim={self.similarity:.3f}, {self.explanation})"


class SymbolicVerifier:
    """
    Vérificateur symbolique de transformations
    
    Fonctionnalités:
    - Vérification EXACTE (array_equal)
    - Métriques de similarité pour debugging
    - Explication détaillée des échecs
    - Détection de patterns d'erreurs
    """
    
    def __init__(self, primitives: PrimitiveTransforms, composer: TransformComposer):
        self.primitives = primitives
        self.composer = composer
        self.verifications_count = 0
        self.exact_matches = 0
    
    def verify_exact_match(self, hypothesis: Hypothesis, 
                          examples: List[Tuple[np.ndarray, np.ndarray]]) -> bool:
        """
        Vérification EXACTE sur tous les exemples
        
        Returns:
            True si TOUS les exemples matchent EXACTEMENT
        """
        self.verifications_count += 1
        
        for inp, out in examples:
            if not self.composer.validate(inp, out, hypothesis.composed_transform):
                return False
        
        self.exact_matches += 1
        return True
    
    def verify_with_details(self, input_grid: np.ndarray, output_grid: np.ndarray,
                           composed: ComposedTransform) -> VerificationResult:
        """
        Vérification détaillée avec métriques
        
        Returns:
            VerificationResult avec métriques complètes
        """
        self.verifications_count += 1
        
        # Appliquer transformation
        result = self.composer.apply(input_grid, composed)
        
        # Vérification exacte
        exact_match = self.primitives.verify_exact_match(result, output_grid)
        
        # Shape match
        shape_match = (result.shape == output_grid.shape)
        
        if not shape_match:
            return VerificationResult(
                success=False,
                exact_match=False,
                similarity=0.0,
                explanation=f"Shape mismatch: {result.shape} vs {output_grid.shape}",
                diff_pixels=result.size,
                total_pixels=result.size,
                shape_match=False,
                color_distribution_match=False
            )
        
        # Calculer différences pixel par pixel
        diff_mask = (result != output_grid)
        diff_pixels = int(np.sum(diff_mask))
        total_pixels = result.size
        
        # Similarité = proportion de pixels corrects
        similarity = 1.0 - (diff_pixels / total_pixels) if total_pixels > 0 else 0.0
        
        # Vérifier distribution de couleurs
        result_colors = np.bincount(result.flatten(), minlength=10)
        output_colors = np.bincount(output_grid.flatten(), minlength=10)
        color_distribution_match = np.array_equal(result_colors, output_colors)
        
        # Générer explication
        if exact_match:
            explanation = "Perfect exact match ✓"
            success = True
        else:
            explanation = self._generate_mismatch_explanation(
                result, output_grid, diff_pixels, total_pixels
            )
            success = False
        
        if exact_match:
            self.exact_matches += 1
        
        return VerificationResult(
            success=success,
            exact_match=exact_match,
            similarity=similarity,
            explanation=explanation,
            diff_pixels=diff_pixels,
            total_pixels=total_pixels,
            shape_match=shape_match,
            color_distribution_match=color_distribution_match
        )
    
    def _generate_mismatch_explanation(self, result: np.ndarray, expected: np.ndarray,
                                      diff_pixels: int, total_pixels: int) -> str:
        """Générer explication détaillée d'un échec"""
        diff_percent = 100.0 * diff_pixels / total_pixels
        
        explanation = f"{diff_pixels}/{total_pixels} pixels differ ({diff_percent:.1f}%)"
        
        # Analyser couleurs
        result_colors = set(result.flatten())
        expected_colors = set(expected.flatten())
        
        missing_colors = expected_colors - result_colors
        extra_colors = result_colors - expected_colors
        
        if missing_colors:
            explanation += f" | Missing colors: {sorted(missing_colors)}"
        if extra_colors:
            explanation += f" | Extra colors: {sorted(extra_colors)}"
        
        # Analyser distribution spatiale des erreurs
        diff_mask = (result != expected)
        if diff_pixels > 0:
            # Trouver région avec plus d'erreurs
            h, w = result.shape
            errors_by_quadrant = []
            for i in range(2):
                for j in range(2):
                    y_start = i * h // 2
                    y_end = (i + 1) * h // 2
                    x_start = j * w // 2
                    x_end = (j + 1) * w // 2
                    quadrant_errors = np.sum(diff_mask[y_start:y_end, x_start:x_end])
                    errors_by_quadrant.append(quadrant_errors)
            
            max_errors = max(errors_by_quadrant)
            if max_errors > diff_pixels * 0.5:  # Plus de 50% dans un quadrant
                quadrant_idx = errors_by_quadrant.index(max_errors)
                explanation += f" | Errors concentrated in quadrant {quadrant_idx}"
        
        return explanation
    
    def explain_mismatch(self, input_grid: np.ndarray, output_grid: np.ndarray,
                        hypothesis: Hypothesis) -> str:
        """Expliquer pourquoi une hypothèse échoue"""
        result = self.verify_with_details(
            input_grid, output_grid, hypothesis.composed_transform
        )
        return result.explanation
    
    def batch_verify(self, hypotheses: List[Hypothesis],
                    examples: List[Tuple[np.ndarray, np.ndarray]]) -> List[Tuple[Hypothesis, bool]]:
        """
        Vérifier batch d'hypothèses
        
        Returns:
            Liste de (hypothesis, success) triée par succès
        """
        results = []
        for hypothesis in hypotheses:
            success = self.verify_exact_match(hypothesis, examples)
            results.append((hypothesis, success))
        
        # Trier: succès d'abord, puis par score
        results.sort(key=lambda x: (not x[1], -x[0].score))
        return results
    
    def find_best_hypothesis(self, hypotheses: List[Hypothesis],
                            examples: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[Hypothesis]:
        """
        Trouver meilleure hypothèse (vérification exacte)
        
        Returns:
            Première hypothèse qui matche EXACTEMENT tous les exemples, ou None
        """
        for hypothesis in hypotheses:
            if self.verify_exact_match(hypothesis, examples):
                return hypothesis
        return None
    
    def compute_similarity_matrix(self, grids1: List[np.ndarray],
                                 grids2: List[np.ndarray]) -> np.ndarray:
        """
        Calculer matrice de similarité entre deux ensembles de grilles
        
        Returns:
            Matrice (len(grids1), len(grids2)) de similarités [0, 1]
        """
        n1 = len(grids1)
        n2 = len(grids2)
        matrix = np.zeros((n1, n2))
        
        for i, g1 in enumerate(grids1):
            for j, g2 in enumerate(grids2):
                if g1.shape == g2.shape:
                    diff = np.sum(g1 != g2)
                    total = g1.size
                    matrix[i, j] = 1.0 - (diff / total) if total > 0 else 0.0
                else:
                    matrix[i, j] = 0.0
        
        return matrix
    
    def detect_error_patterns(self, results: List[VerificationResult]) -> Dict[str, Any]:
        """
        Détecter patterns d'erreurs récurrents
        
        Returns:
            Dictionnaire avec statistiques d'erreurs
        """
        if not results:
            return {}
        
        total = len(results)
        exact_matches = sum(1 for r in results if r.exact_match)
        shape_mismatches = sum(1 for r in results if not r.shape_match)
        color_mismatches = sum(1 for r in results if not r.color_distribution_match)
        
        avg_similarity = np.mean([r.similarity for r in results])
        
        return {
            'total_verifications': total,
            'exact_matches': exact_matches,
            'exact_match_rate': exact_matches / total if total > 0 else 0.0,
            'shape_mismatches': shape_mismatches,
            'color_mismatches': color_mismatches,
            'avg_similarity': float(avg_similarity),
            'min_similarity': float(min(r.similarity for r in results)),
            'max_similarity': float(max(r.similarity for r in results)),
        }
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques globales"""
        return {
            'verifications': self.verifications_count,
            'exact_matches': self.exact_matches,
            'match_rate': self.exact_matches / self.verifications_count if self.verifications_count > 0 else 0.0
        }


# Made with Bob - Expert en Symbolic Verification, Formal Methods, Error Analysis