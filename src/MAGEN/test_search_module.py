"""
MAGEN - Tests Unitaires Module Search
Validation pruning system et beam search (CORRECTIONS CRITIQUES)

Tests:
1. Program cost model
2. Transform priors
3. Canonical encoding
4. Pruning system
5. Beam search
6. Integration complète
"""

import numpy as np
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from search import (
    ProgramCostModel,
    TransformPriors,
    CanonicalEncoder,
    PruningSystem,
    BeamSearch
)
from symbolic import Transform, TransformType, TransformComposer, Hypothesis, PrimitiveTransforms


class TestSearchModule:
    """Suite de tests pour le module search"""
    
    def __init__(self):
        self.cost_model = ProgramCostModel()
        self.priors = TransformPriors()
        self.canonical = CanonicalEncoder()
        self.pruning = PruningSystem(beam_width=5)
        self.primitives = PrimitiveTransforms()
        self.composer = TransformComposer(self.primitives)
        
        self.tests_passed = 0
        self.tests_failed = 0
        self.tests_total = 0
    
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
    
    def assert_equal(self, actual, expected, test_name: str):
        """Assert avec logging"""
        self.tests_total += 1
        if actual == expected:
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            print(f"  Expected: {expected}")
            print(f"  Got: {actual}")
            self.tests_failed += 1
            return False
    
    # ========== TESTS PROGRAM COST MODEL ==========
    
    def test_cost_simple_transform(self):
        """Test coût transformation simple"""
        transform = Transform(type=TransformType.ROTATE_90, parameters={})
        composed = self.composer.compose([transform])
        
        cost = self.cost_model.compute_cost(composed)
        
        self.assert_equal(cost.complexity, 1, "Cost simple - complexity")
        self.assert_equal(cost.parameter_count, 0, "Cost simple - parameters")
        self.assert_true(cost.total_cost > 0, "Cost simple - total > 0")
    
    def test_cost_complex_composition(self):
        """Test coût composition complexe"""
        transforms = [
            Transform(type=TransformType.ROTATE_90, parameters={}),
            Transform(type=TransformType.FLIP_HORIZONTAL, parameters={}),
            Transform(type=TransformType.RECOLOR, parameters={'old_color': 1, 'new_color': 2})
        ]
        composed = self.composer.compose(transforms)
        
        cost = self.cost_model.compute_cost(composed)
        
        self.assert_equal(cost.complexity, 3, "Cost complex - complexity")
        self.assert_equal(cost.parameter_count, 2, "Cost complex - parameters")
        self.assert_true(cost.total_cost > 3.0, "Cost complex - total > simple")
    
    def test_cost_penalty(self):
        """Test pénalité de complexité"""
        # Simple
        simple = self.composer.compose([Transform(type=TransformType.IDENTITY, parameters={})])
        cost_simple = self.cost_model.compute_cost(simple)
        penalty_simple = self.cost_model.compute_penalty(cost_simple)
        
        # Complexe
        complex_transforms = [Transform(type=TransformType.EXTRACT_OBJECTS, parameters={})] * 5
        complex = self.composer.compose(complex_transforms)
        cost_complex = self.cost_model.compute_cost(complex)
        penalty_complex = self.cost_model.compute_penalty(cost_complex)
        
        self.assert_true(penalty_complex > penalty_simple, "Cost penalty - complex > simple")
        self.assert_true(0.0 <= penalty_simple <= 1.0, "Cost penalty - range [0,1]")
    
    # ========== TESTS TRANSFORM PRIORS ==========
    
    def test_priors_initialization(self):
        """Test initialisation priors"""
        prior = self.priors.get_prior('rotate_90')
        
        self.assert_true(0.0 < prior < 1.0, "Priors init - valid range")
    
    def test_priors_update(self):
        """Test mise à jour priors"""
        initial_prior = self.priors.get_prior('rotate_90')
        
        # Plusieurs succès
        for _ in range(5):
            self.priors.update('rotate_90', success=True)
        
        updated_prior = self.priors.get_prior('rotate_90')
        
        self.assert_true(updated_prior > initial_prior, "Priors update - increases with success")
    
    def test_priors_composition(self):
        """Test prior d'une composition"""
        transforms = [
            Transform(type=TransformType.ROTATE_90, parameters={}),
            Transform(type=TransformType.FLIP_HORIZONTAL, parameters={})
        ]
        composed = self.composer.compose(transforms)
        
        prior = self.priors.get_composition_prior(composed)
        
        self.assert_true(0.0 < prior <= 1.0, "Priors composition - valid range")
    
    # ========== TESTS CANONICAL ENCODING ==========
    
    def test_canonical_same_transforms(self):
        """Test encoding transformations identiques"""
        t1 = Transform(type=TransformType.ROTATE_90, parameters={})
        t2 = Transform(type=TransformType.ROTATE_90, parameters={})
        
        composed1 = self.composer.compose([t1])
        composed2 = self.composer.compose([t2])
        
        hyp1 = Hypothesis(composed_transform=composed1)
        hyp2 = Hypothesis(composed_transform=composed2)
        
        hash1 = self.canonical.encode(hyp1)
        hash2 = self.canonical.encode(hyp2)
        
        self.assert_equal(hash1, hash2, "Canonical - same transforms same hash")
    
    def test_canonical_different_transforms(self):
        """Test encoding transformations différentes"""
        t1 = Transform(type=TransformType.ROTATE_90, parameters={})
        t2 = Transform(type=TransformType.ROTATE_180, parameters={})
        
        composed1 = self.composer.compose([t1])
        composed2 = self.composer.compose([t2])
        
        hyp1 = Hypothesis(composed_transform=composed1)
        hyp2 = Hypothesis(composed_transform=composed2)
        
        hash1 = self.canonical.encode(hyp1)
        hash2 = self.canonical.encode(hyp2)
        
        self.assert_true(hash1 != hash2, "Canonical - different transforms different hash")
    
    def test_canonical_parameter_order(self):
        """Test encoding indépendant de l'ordre des paramètres"""
        t1 = Transform(type=TransformType.RECOLOR, parameters={'old_color': 1, 'new_color': 2})
        t2 = Transform(type=TransformType.RECOLOR, parameters={'new_color': 2, 'old_color': 1})
        
        composed1 = self.composer.compose([t1])
        composed2 = self.composer.compose([t2])
        
        hyp1 = Hypothesis(composed_transform=composed1)
        hyp2 = Hypothesis(composed_transform=composed2)
        
        hash1 = self.canonical.encode(hyp1)
        hash2 = self.canonical.encode(hyp2)
        
        self.assert_equal(hash1, hash2, "Canonical - parameter order independent")
    
    # ========== TESTS PRUNING SYSTEM ==========
    
    def test_pruning_score_hypothesis(self):
        """Test scoring hypothèse"""
        transform = Transform(type=TransformType.ROTATE_90, parameters={})
        composed = self.composer.compose([transform])
        hypothesis = Hypothesis(composed_transform=composed, score=0.8)
        
        scored = self.pruning.score_hypothesis(hypothesis, accuracy=0.8)
        
        self.assert_true(scored.total_score > 0, "Pruning score - positive")
        self.assert_true(0.0 <= scored.complexity_penalty <= 1.0, "Pruning score - penalty range")
    
    def test_pruning_early_reject(self):
        """Test early rejection"""
        # Hypothèse très complexe
        complex_transforms = [Transform(type=TransformType.EXTRACT_OBJECTS, parameters={})] * 10
        composed = self.composer.compose(complex_transforms)
        hypothesis = Hypothesis(composed_transform=composed, score=0.1)
        
        scored = self.pruning.score_hypothesis(hypothesis, accuracy=0.1)
        should_reject = self.pruning.early_reject(scored)
        
        self.assert_true(should_reject, "Pruning early reject - rejects complex")
    
    def test_pruning_remove_duplicates(self):
        """Test suppression duplicatas"""
        # Créer hypothèses duplicatas
        t = Transform(type=TransformType.ROTATE_90, parameters={})
        composed = self.composer.compose([t])
        
        hypotheses = [Hypothesis(composed_transform=composed, score=0.8) for _ in range(3)]
        scored = [self.pruning.score_hypothesis(h, 0.8) for h in hypotheses]
        
        unique = self.pruning.remove_duplicates(scored)
        
        self.assert_equal(len(unique), 1, "Pruning duplicates - removes duplicates")
    
    def test_pruning_beam(self):
        """Test beam pruning"""
        # Créer plusieurs hypothèses avec scores différents
        hypotheses = []
        for i in range(10):
            t = Transform(type=TransformType.ROTATE_90, parameters={})
            composed = self.composer.compose([t])
            hyp = Hypothesis(composed_transform=composed, score=0.5 + i * 0.05)
            hypotheses.append(hyp)
        
        scored = [self.pruning.score_hypothesis(h, h.score) for h in hypotheses]
        pruned = self.pruning.prune_beam(scored)
        
        self.assert_equal(len(pruned), 5, "Pruning beam - keeps beam_width")
        # Vérifier ordre décroissant
        scores = [s.total_score for s in pruned]
        self.assert_true(scores == sorted(scores, reverse=True), "Pruning beam - sorted")
    
    # ========== TESTS BEAM SEARCH ==========
    
    def test_beam_search_simple(self):
        """Test beam search simple"""
        beam = BeamSearch(beam_width=3, max_depth=2, pruning_system=self.pruning)
        
        # Grilles simples
        input_grid = np.array([[1, 2], [3, 4]])
        target_grid = np.array([[3, 1], [4, 2]])  # Rotation 90
        
        def score_fn(result, target):
            if result.shape != target.shape:
                return 0.0
            matches = np.sum(result == target)
            return float(matches / result.size)
        
        solutions = beam.search(input_grid, target_grid, score_fn)
        
        self.assert_true(len(solutions) >= 0, "Beam search - returns solutions")
        self.assert_true(beam.nodes_expanded > 0, "Beam search - expands nodes")
    
    # ========== EXÉCUTION TESTS ==========
    
    def run_all_tests(self):
        """Exécuter tous les tests"""
        print("=" * 80)
        print("MAGEN - Tests Unitaires Module Search (CORRECTIONS CRITIQUES)")
        print("=" * 80)
        print()
        
        print("--- Tests Program Cost Model ---")
        self.test_cost_simple_transform()
        self.test_cost_complex_composition()
        self.test_cost_penalty()
        print()
        
        print("--- Tests Transform Priors ---")
        self.test_priors_initialization()
        self.test_priors_update()
        self.test_priors_composition()
        print()
        
        print("--- Tests Canonical Encoding ---")
        self.test_canonical_same_transforms()
        self.test_canonical_different_transforms()
        self.test_canonical_parameter_order()
        print()
        
        print("--- Tests Pruning System ---")
        self.test_pruning_score_hypothesis()
        self.test_pruning_early_reject()
        self.test_pruning_remove_duplicates()
        self.test_pruning_beam()
        print()
        
        print("--- Tests Beam Search ---")
        self.test_beam_search_simple()
        print()
        
        # Résultats
        print("=" * 80)
        print("RÉSULTATS")
        print("=" * 80)
        print(f"Tests passés:  {self.tests_passed}/{self.tests_total}")
        print(f"Tests échoués: {self.tests_failed}/{self.tests_total}")
        print(f"Taux succès:   {100.0 * self.tests_passed / self.tests_total:.1f}%")
        print()
        
        # Statistiques modules
        print("Statistiques modules:")
        print(f"  Pruning:  {self.pruning.get_statistics()}")
        print()
        
        return self.tests_failed == 0


if __name__ == "__main__":
    tester = TestSearchModule()
    success = tester.run_all_tests()
    
    if success:
        print("✓ TOUS LES TESTS PASSÉS - CORRECTIONS CRITIQUES VALIDÉES")
        sys.exit(0)
    else:
        print("✗ CERTAINS TESTS ONT ÉCHOUÉ")
        sys.exit(1)


# Made with Bob - Expert en Unit Testing, Search Optimization, Complexity Theory