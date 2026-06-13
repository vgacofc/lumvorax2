"""
Tests pour Symbolic Execution Verifier
======================================

Valide que le verifier détecte correctement:
1. Exact matches
2. Violations de contraintes
3. Transformations valides vs invalides
"""

import numpy as np
import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from core.symbolic_execution_verifier import (
    SymbolicExecutionVerifier,
    VerificationResult,
    ConstraintType
)


def test_exact_match():
    """Test 1: Exact match doit être détecté"""
    print("\n" + "="*80)
    print("TEST 1: EXACT MATCH")
    print("="*80)
    
    verifier = SymbolicExecutionVerifier(strict_mode=True)
    
    # Grilles identiques
    input_grid = np.array([[1, 2], [3, 4]])
    predicted = np.array([[1, 2], [3, 4]])
    expected = np.array([[1, 2], [3, 4]])
    
    result = verifier.verify_transformation(input_grid, predicted, expected)
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    
    assert result.exact_match, "Should detect exact match"
    assert result.is_valid, "Exact match should be valid"
    assert result.confidence == 1.0, "Confidence should be 1.0"
    
    print("✅ TEST 1 RÉUSSI")
    return True


def test_shape_violation():
    """Test 2: Violation de shape doit être détectée"""
    print("\n" + "="*80)
    print("TEST 2: SHAPE VIOLATION")
    print("="*80)
    
    verifier = SymbolicExecutionVerifier(strict_mode=True)
    
    input_grid = np.array([[1, 2], [3, 4]])
    predicted = np.array([[1, 2, 3], [4, 5, 6]])  # Shape différente
    expected = np.array([[1, 2], [3, 4]])
    
    result = verifier.verify_transformation(
        input_grid, predicted, expected, transformation_type="geometric"
    )
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    print(f"✓ Violations: {result.constraints_violated}")
    
    assert not result.exact_match, "Should not be exact match"
    assert not result.is_valid, "Should be invalid (shape violation)"
    
    print("✅ TEST 2 RÉUSSI")
    return True


def test_color_mapping():
    """Test 3: Color mapping doit être vérifié"""
    print("\n" + "="*80)
    print("TEST 3: COLOR MAPPING")
    print("="*80)
    
    verifier = SymbolicExecutionVerifier(strict_mode=False)
    
    input_grid = np.array([[1, 2], [3, 4]])
    # Predicted a trop de couleurs différentes
    predicted = np.array([[1, 2], [5, 6]])
    expected = np.array([[1, 2], [3, 4]])
    
    result = verifier.verify_transformation(
        input_grid, predicted, expected, transformation_type="color"
    )
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    print(f"✓ Satisfied: {result.constraints_satisfied}")
    print(f"✓ Violated: {result.constraints_violated}")
    
    assert not result.exact_match, "Should not be exact match"
    
    print("✅ TEST 3 RÉUSSI")
    return True


def test_rotation_valid():
    """Test 4: Rotation valide doit être acceptée"""
    print("\n" + "="*80)
    print("TEST 4: ROTATION VALIDE")
    print("="*80)
    
    verifier = SymbolicExecutionVerifier(strict_mode=False)
    
    input_grid = np.array([[1, 2], [3, 4]])
    # Rotation 90° (shape inversée)
    predicted = np.array([[3, 1], [4, 2]])
    expected = np.array([[3, 1], [4, 2]])
    
    result = verifier.verify_transformation(
        input_grid, predicted, expected, transformation_type="geometric"
    )
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    
    assert result.exact_match, "Rotation should match expected"
    assert result.is_valid, "Valid rotation should be accepted"
    
    print("✅ TEST 4 RÉUSSI")
    return True


def test_program_verification():
    """Test 5: Vérification programme complet"""
    print("\n" + "="*80)
    print("TEST 5: PROGRAM VERIFICATION")
    print("="*80)
    
    from dsl.program import Program
    
    verifier = SymbolicExecutionVerifier(strict_mode=True)
    
    # Créer programme simple (identity)
    program = Program()
    program.add_operation("identity", lambda x: x, ())
    
    # Train pairs
    train_pairs = [
        (np.array([[1, 2], [3, 4]]), np.array([[1, 2], [3, 4]])),
        (np.array([[5, 6], [7, 8]]), np.array([[5, 6], [7, 8]]))
    ]
    
    result = verifier.verify_program_execution(program, train_pairs)
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    
    assert result.exact_match, "Identity program should match exactly"
    assert result.is_valid, "Identity program should be valid"
    assert result.confidence == 1.0, "Confidence should be 1.0"
    
    print("✅ TEST 5 RÉUSSI")
    return True


def test_spatial_relations():
    """Test 6: Relations spatiales"""
    print("\n" + "="*80)
    print("TEST 6: SPATIAL RELATIONS")
    print("="*80)
    
    verifier = SymbolicExecutionVerifier(strict_mode=False)
    
    # Grilles avec objets colorés
    input_grid = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]])
    # Predicted: objet déplacé légèrement
    predicted = np.array([[0, 0, 0], [0, 0, 1], [0, 0, 0]])
    expected = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]])
    
    result = verifier.verify_transformation(
        input_grid, predicted, expected, transformation_type="spatial"
    )
    
    print(f"✓ Exact match: {result.exact_match}")
    print(f"✓ Is valid: {result.is_valid}")
    print(f"✓ Confidence: {result.confidence:.2f}")
    print(f"✓ Satisfied: {len(result.constraints_satisfied)}")
    print(f"✓ Violated: {len(result.constraints_violated)}")
    
    assert not result.exact_match, "Should not be exact match"
    # Spatial relation devrait être violée (objet déplacé)
    
    print("✅ TEST 6 RÉUSSI")
    return True


def main():
    """Exécuter tous les tests"""
    print("="*80)
    print("TESTS SYMBOLIC EXECUTION VERIFIER")
    print("="*80)
    print("Objectif: Valider Exact Constraint Solver Layer")
    print("="*80)
    
    tests = [
        ("Exact Match", test_exact_match),
        ("Shape Violation", test_shape_violation),
        ("Color Mapping", test_color_mapping),
        ("Rotation Valide", test_rotation_valid),
        ("Program Verification", test_program_verification),
        ("Spatial Relations", test_spatial_relations)
    ]
    
    results = []
    for name, test_func in tests:
        try:
            success = test_func()
            results.append((name, success))
        except Exception as e:
            print(f"\n❌ TEST ÉCHOUÉ: {name}")
            print(f"   Erreur: {str(e)}")
            import traceback
            traceback.print_exc()
            results.append((name, False))
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ DES TESTS")
    print("="*80)
    
    passed = sum(1 for _, success in results if success)
    total = len(results)
    
    for name, success in results:
        status = "✅ RÉUSSI" if success else "❌ ÉCHOUÉ"
        print(f"{status}: {name}")
    
    print(f"\n📊 RÉSULTAT: {passed}/{total} tests réussis ({100*passed/total:.0f}%)")
    
    if passed == total:
        print("\n🎉 TOUS LES TESTS RÉUSSIS!")
        print("✅ Symbolic Execution Verifier VALIDÉ")
        return 0
    else:
        print(f"\n⚠️  {total - passed} test(s) échoué(s)")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
