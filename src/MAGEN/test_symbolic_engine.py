"""
MAGEN - Tests Unitaires Moteur Symbolique
Validation complète du système de program synthesis

Tests:
1. Transformations primitives individuelles
2. Composition de transformations
3. Génération d'hypothèses
4. Vérification symbolique
5. Cas réels ARC-like
"""

import numpy as np
import sys
from pathlib import Path

# Ajouter le chemin du module
sys.path.insert(0, str(Path(__file__).parent))

from symbolic import (
    PrimitiveTransforms,
    TransformComposer,
    HypothesisGenerator,
    SymbolicVerifier,
    Transform,
    TransformType
)


class TestSymbolicEngine:
    """Suite de tests pour le moteur symbolique"""
    
    def __init__(self):
        self.primitives = PrimitiveTransforms(verbose=True)
        self.composer = TransformComposer(self.primitives)
        self.generator = HypothesisGenerator(self.primitives, self.composer, max_composition_length=3)
        self.verifier = SymbolicVerifier(self.primitives, self.composer)
        
        self.tests_passed = 0
        self.tests_failed = 0
        self.tests_total = 0
    
    def assert_equal(self, actual, expected, test_name: str):
        """Assert avec logging"""
        self.tests_total += 1
        if np.array_equal(actual, expected):
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            print(f"  Expected:\n{expected}")
            print(f"  Got:\n{actual}")
            self.tests_failed += 1
            return False
    
    def assert_true(self, condition: bool, test_name: str):
        """Assert boolean avec logging"""
        self.tests_total += 1
        if condition:
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            self.tests_failed += 1
            return False
    
    # ========== TESTS TRANSFORMATIONS PRIMITIVES ==========
    
    def test_rotate_90(self):
        """Test rotation 90°"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[3, 1], [4, 2]])
        result = self.primitives.rotate_90(grid)
        self.assert_equal(result, expected, "Rotate 90°")
    
    def test_rotate_180(self):
        """Test rotation 180°"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[4, 3], [2, 1]])
        result = self.primitives.rotate_180(grid)
        self.assert_equal(result, expected, "Rotate 180°")
    
    def test_flip_horizontal(self):
        """Test flip horizontal"""
        grid = np.array([[1, 2, 3], [4, 5, 6]])
        expected = np.array([[3, 2, 1], [6, 5, 4]])
        result = self.primitives.flip_horizontal(grid)
        self.assert_equal(result, expected, "Flip horizontal")
    
    def test_flip_vertical(self):
        """Test flip vertical"""
        grid = np.array([[1, 2, 3], [4, 5, 6]])
        expected = np.array([[4, 5, 6], [1, 2, 3]])
        result = self.primitives.flip_vertical(grid)
        self.assert_equal(result, expected, "Flip vertical")
    
    def test_transpose(self):
        """Test transposition"""
        grid = np.array([[1, 2, 3], [4, 5, 6]])
        expected = np.array([[1, 4], [2, 5], [3, 6]])
        result = self.primitives.transpose(grid)
        self.assert_equal(result, expected, "Transpose")
    
    def test_crop(self):
        """Test crop"""
        grid = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
        expected = np.array([[5, 6], [8, 9]])
        result = self.primitives.crop(grid, x=1, y=1, width=2, height=2)
        self.assert_equal(result, expected, "Crop")
    
    def test_expand(self):
        """Test expand"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[0, 0, 0, 0], [0, 1, 2, 0], [0, 3, 4, 0], [0, 0, 0, 0]])
        result = self.primitives.expand(grid, top=1, bottom=1, left=1, right=1, fill_value=0)
        self.assert_equal(result, expected, "Expand")
    
    def test_translate(self):
        """Test translation"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[0, 0], [0, 1]])
        result = self.primitives.translate(grid, dx=1, dy=1, fill_value=0)
        self.assert_equal(result, expected, "Translate")
    
    def test_scale_up(self):
        """Test scale up"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[1, 1, 2, 2], [1, 1, 2, 2], [3, 3, 4, 4], [3, 3, 4, 4]])
        result = self.primitives.scale_up(grid, factor=2)
        self.assert_equal(result, expected, "Scale up")
    
    def test_recolor(self):
        """Test recolor"""
        grid = np.array([[1, 2, 1], [2, 1, 2]])
        expected = np.array([[9, 2, 9], [2, 9, 2]])
        result = self.primitives.recolor(grid, old_color=1, new_color=9)
        self.assert_equal(result, expected, "Recolor")
    
    def test_color_map(self):
        """Test color mapping"""
        grid = np.array([[1, 2, 3], [2, 3, 1]])
        mapping = {1: 7, 2: 8, 3: 9}
        expected = np.array([[7, 8, 9], [8, 9, 7]])
        result = self.primitives.color_map(grid, mapping=mapping)
        self.assert_equal(result, expected, "Color map")
    
    def test_flood_fill(self):
        """Test flood fill"""
        grid = np.array([[1, 1, 2], [1, 1, 2], [2, 2, 2]])
        expected = np.array([[9, 9, 2], [9, 9, 2], [2, 2, 2]])
        result = self.primitives.flood_fill(grid, x=0, y=0, new_color=9)
        self.assert_equal(result, expected, "Flood fill")
    
    def test_detect_symmetry(self):
        """Test détection symétrie"""
        # Grille avec symétrie horizontale
        grid = np.array([[1, 2, 1], [3, 4, 3]])
        symmetries = self.primitives.detect_symmetry(grid)
        self.assert_true(symmetries['horizontal'], "Detect horizontal symmetry")
    
    def test_complete_symmetry(self):
        """Test complétion symétrie"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[1, 2, 2, 1], [3, 4, 4, 3]])
        result = self.primitives.complete_symmetry(grid, axis='horizontal')
        self.assert_equal(result, expected, "Complete symmetry")
    
    def test_repeat_pattern(self):
        """Test répétition pattern"""
        grid = np.array([[1, 2], [3, 4]])
        expected = np.array([[1, 2, 1, 2], [3, 4, 3, 4], [1, 2, 1, 2], [3, 4, 3, 4]])
        result = self.primitives.repeat_pattern(grid, repeat_h=2, repeat_v=2)
        self.assert_equal(result, expected, "Repeat pattern")
    
    # ========== TESTS COMPOSITION ==========
    
    def test_compose_two_transforms(self):
        """Test composition de 2 transformations"""
        grid = np.array([[1, 2], [3, 4]])
        
        # Rotate 90 puis flip horizontal
        t1 = Transform(type=TransformType.ROTATE_90, parameters={})
        t2 = Transform(type=TransformType.FLIP_HORIZONTAL, parameters={})
        composed = self.composer.compose([t1, t2])
        
        result = self.composer.apply(grid, composed)
        
        # Vérifier résultat
        intermediate = self.primitives.rotate_90(grid)  # [[3, 1], [4, 2]]
        expected = self.primitives.flip_horizontal(intermediate)  # [[1, 3], [2, 4]]
        
        self.assert_equal(result, expected, "Compose rotate+flip")
    
    def test_validate_composition(self):
        """Test validation composition"""
        input_grid = np.array([[1, 2], [3, 4]])
        output_grid = np.array([[4, 3], [2, 1]])  # Rotation 180
        
        t = Transform(type=TransformType.ROTATE_180, parameters={})
        composed = self.composer.compose([t])
        
        is_valid = self.composer.validate(input_grid, output_grid, composed)
        self.assert_true(is_valid, "Validate composition")
    
    # ========== TESTS GÉNÉRATION HYPOTHÈSES ==========
    
    def test_generate_simple_hypotheses(self):
        """Test génération hypothèses simples"""
        # Cas simple: rotation 90
        input_grids = [np.array([[1, 2], [3, 4]])]
        output_grids = [np.array([[3, 1], [4, 2]])]
        
        hypotheses = self.generator.generate_candidates(input_grids, output_grids, max_candidates=20)
        
        # Vérifier qu'on a des hypothèses
        self.assert_true(len(hypotheses) > 0, "Generate hypotheses")
        
        # Vérifier qu'une hypothèse a score parfait
        perfect_hypotheses = [h for h in hypotheses if h.score == 1.0]
        self.assert_true(len(perfect_hypotheses) > 0, "Find perfect hypothesis")
    
    def test_generate_color_hypotheses(self):
        """Test génération hypothèses couleurs"""
        # Cas: color mapping
        input_grids = [np.array([[1, 2], [2, 1]])]
        output_grids = [np.array([[7, 8], [8, 7]])]
        
        hypotheses = self.generator.generate_candidates(input_grids, output_grids, max_candidates=50)
        
        # Vérifier qu'on trouve l'hypothèse correcte
        perfect = [h for h in hypotheses if h.score == 1.0]
        self.assert_true(len(perfect) > 0, "Find color mapping hypothesis")
    
    def test_generate_symmetry_hypotheses(self):
        """Test génération hypothèses symétrie"""
        # Cas: complétion symétrie
        input_grids = [np.array([[1, 2], [3, 4]])]
        output_grids = [np.array([[1, 2, 2, 1], [3, 4, 4, 3]])]
        
        hypotheses = self.generator.generate_candidates(input_grids, output_grids, max_candidates=50)
        
        # Vérifier qu'on trouve l'hypothèse
        perfect = [h for h in hypotheses if h.score == 1.0]
        self.assert_true(len(perfect) > 0, "Find symmetry hypothesis")
    
    # ========== TESTS VÉRIFICATION ==========
    
    def test_verify_exact_match(self):
        """Test vérification exacte"""
        input_grid = np.array([[1, 2], [3, 4]])
        output_grid = np.array([[3, 1], [4, 2]])
        
        t = Transform(type=TransformType.ROTATE_90, parameters={})
        composed = self.composer.compose([t])
        
        from symbolic.transform_composer import Hypothesis
        hypothesis = Hypothesis(composed_transform=composed, score=1.0)
        
        is_valid = self.verifier.verify_exact_match(hypothesis, [(input_grid, output_grid)])
        self.assert_true(is_valid, "Verify exact match")
    
    def test_verify_with_details(self):
        """Test vérification détaillée"""
        input_grid = np.array([[1, 2], [3, 4]])
        output_grid = np.array([[3, 1], [4, 2]])
        
        t = Transform(type=TransformType.ROTATE_90, parameters={})
        composed = self.composer.compose([t])
        
        result = self.verifier.verify_with_details(input_grid, output_grid, composed)
        
        self.assert_true(result.exact_match, "Verify details - exact match")
        self.assert_true(result.similarity == 1.0, "Verify details - similarity")
    
    def test_find_best_hypothesis(self):
        """Test recherche meilleure hypothèse"""
        input_grids = [np.array([[1, 2], [3, 4]])]
        output_grids = [np.array([[3, 1], [4, 2]])]
        
        hypotheses = self.generator.generate_candidates(input_grids, output_grids, max_candidates=20)
        
        best = self.verifier.find_best_hypothesis(hypotheses, list(zip(input_grids, output_grids)))
        
        self.assert_true(best is not None, "Find best hypothesis")
        if best:
            self.assert_true(best.score == 1.0, "Best hypothesis has perfect score")
    
    # ========== TESTS CAS RÉELS ARC-LIKE ==========
    
    def test_arc_like_pattern_1(self):
        """Test pattern ARC-like: répétition"""
        # Input: petit pattern
        # Output: pattern répété 2x2
        input_grid = np.array([[1, 2], [3, 4]])
        output_grid = np.array([[1, 2, 1, 2], [3, 4, 3, 4], [1, 2, 1, 2], [3, 4, 3, 4]])
        
        hypotheses = self.generator.generate_candidates([input_grid], [output_grid], max_candidates=50)
        best = self.verifier.find_best_hypothesis(hypotheses, [(input_grid, output_grid)])
        
        self.assert_true(best is not None, "ARC pattern: repetition")
    
    def test_arc_like_pattern_2(self):
        """Test pattern ARC-like: symétrie"""
        input_grid = np.array([[1, 2], [3, 4]])
        output_grid = np.array([[1, 2, 2, 1], [3, 4, 4, 3]])
        
        hypotheses = self.generator.generate_candidates([input_grid], [output_grid], max_candidates=50)
        best = self.verifier.find_best_hypothesis(hypotheses, [(input_grid, output_grid)])
        
        self.assert_true(best is not None, "ARC pattern: symmetry")
    
    def test_arc_like_pattern_3(self):
        """Test pattern ARC-like: rotation + flip"""
        input_grid = np.array([[1, 2, 3], [4, 5, 6]])
        # Rotate 90 puis flip horizontal
        intermediate = np.rot90(input_grid, k=-1)
        output_grid = np.fliplr(intermediate)
        
        hypotheses = self.generator.generate_candidates([input_grid], [output_grid], max_candidates=100)
        best = self.verifier.find_best_hypothesis(hypotheses, [(input_grid, output_grid)])
        
        self.assert_true(best is not None, "ARC pattern: rotate+flip")
    
    # ========== EXÉCUTION TESTS ==========
    
    def run_all_tests(self):
        """Exécuter tous les tests"""
        print("=" * 80)
        print("MAGEN - Tests Unitaires Moteur Symbolique")
        print("=" * 80)
        print()
        
        print("--- Tests Transformations Primitives ---")
        self.test_rotate_90()
        self.test_rotate_180()
        self.test_flip_horizontal()
        self.test_flip_vertical()
        self.test_transpose()
        self.test_crop()
        self.test_expand()
        self.test_translate()
        self.test_scale_up()
        self.test_recolor()
        self.test_color_map()
        self.test_flood_fill()
        self.test_detect_symmetry()
        self.test_complete_symmetry()
        self.test_repeat_pattern()
        print()
        
        print("--- Tests Composition ---")
        self.test_compose_two_transforms()
        self.test_validate_composition()
        print()
        
        print("--- Tests Génération Hypothèses ---")
        self.test_generate_simple_hypotheses()
        self.test_generate_color_hypotheses()
        self.test_generate_symmetry_hypotheses()
        print()
        
        print("--- Tests Vérification ---")
        self.test_verify_exact_match()
        self.test_verify_with_details()
        self.test_find_best_hypothesis()
        print()
        
        print("--- Tests Cas Réels ARC-like ---")
        self.test_arc_like_pattern_1()
        self.test_arc_like_pattern_2()
        self.test_arc_like_pattern_3()
        print()
        
        # Statistiques
        print("=" * 80)
        print("RÉSULTATS")
        print("=" * 80)
        print(f"Tests passés:  {self.tests_passed}/{self.tests_total}")
        print(f"Tests échoués: {self.tests_failed}/{self.tests_total}")
        print(f"Taux succès:   {100.0 * self.tests_passed / self.tests_total:.1f}%")
        print()
        
        # Statistiques modules
        print("Statistiques modules:")
        print(f"  Primitives:  {self.primitives.get_statistics()}")
        print(f"  Composer:    {self.composer.get_statistics()}")
        print(f"  Generator:   {self.generator.get_statistics()}")
        print(f"  Verifier:    {self.verifier.get_statistics()}")
        print()
        
        return self.tests_failed == 0


if __name__ == "__main__":
    tester = TestSymbolicEngine()
    success = tester.run_all_tests()
    
    if success:
        print("✓ TOUS LES TESTS PASSÉS")
        sys.exit(0)
    else:
        print("✗ CERTAINS TESTS ONT ÉCHOUÉ")
        sys.exit(1)


# Made with Bob - Expert en Unit Testing, Test-Driven Development, Quality Assurance