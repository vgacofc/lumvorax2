#!/usr/bin/env python3
"""
MAGEN V21 - Test TLE avec Monitoring Temps Réel
================================================

Test du Transformation Learning Engine sur quelques puzzles
avec affichage progression temps réel.

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
"""

import sys
import time
import numpy as np
from pathlib import Path

# Import TLE
from core.transformation_learning_engine import TransformationLearningEngine

def test_tle_on_simple_cases():
    """Test TLE sur cas simples avec monitoring"""
    
    print("="*70)
    print("🧪 MAGEN V21 - TEST TRANSFORMATION LEARNING ENGINE")
    print("="*70)
    print()
    
    # Créer engine
    print("📦 Initialisation TLE...")
    engine = TransformationLearningEngine(verbose=True)
    print(f"   ✅ {len(engine.actions)} actions disponibles")
    print()
    
    # Test Case 1: Rotation 90°
    print("="*70)
    print("TEST 1: ROTATION 90°")
    print("="*70)
    
    train_pairs_rot90 = [
        (np.array([[1, 2], [3, 4]]), np.array([[2, 4], [1, 3]])),
        (np.array([[5, 6], [7, 8]]), np.array([[6, 8], [5, 7]])),
        (np.array([[9, 10], [11, 12]]), np.array([[10, 12], [9, 11]])),
    ]
    
    print("\n📚 Exemples d'entraînement:")
    for i, (inp, out) in enumerate(train_pairs_rot90, 1):
        print(f"   Exemple {i}: {inp.shape} → {out.shape}")
    
    print("\n🔍 Phase 1: Action Discovery...")
    start = time.time()
    discovered = engine.discover_actions(train_pairs_rot90)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   ✅ Actions découvertes: {discovered}")
    
    print("\n🎓 Phase 2: Learning Loop...")
    start = time.time()
    learning_result = engine.learn_from_examples(train_pairs_rot90, max_iterations=5)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   ✅ Learned: {learning_result['learned']}")
    print(f"   🎯 Best action: {learning_result.get('best_action')}")
    print(f"   📊 Best error: {learning_result.get('best_error', 1.0):.3f}")
    print(f"   🔄 Iterations: {learning_result.get('iterations', 0)}")
    
    print("\n🔮 Phase 3: Prediction...")
    test_input = np.array([[13, 14], [15, 16]])
    test_expected = np.array([[14, 16], [13, 15]])
    
    start = time.time()
    predicted, action, confidence = engine.predict(test_input, use_best_action=True)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   🎯 Action: {action}")
    print(f"   📊 Confidence: {confidence:.2f}")
    print(f"   ✅ Correct: {np.array_equal(predicted, test_expected)}")
    
    # Test Case 2: Miroir Horizontal
    print("\n" + "="*70)
    print("TEST 2: MIROIR HORIZONTAL")
    print("="*70)
    
    train_pairs_mirror = [
        (np.array([[1, 2, 3], [4, 5, 6]]), np.array([[3, 2, 1], [6, 5, 4]])),
        (np.array([[7, 8, 9], [10, 11, 12]]), np.array([[9, 8, 7], [12, 11, 10]])),
    ]
    
    print("\n📚 Exemples d'entraînement:")
    for i, (inp, out) in enumerate(train_pairs_mirror, 1):
        print(f"   Exemple {i}: {inp.shape} → {out.shape}")
    
    # Créer nouveau engine pour test indépendant
    engine2 = TransformationLearningEngine(verbose=False)
    
    print("\n🔍 Phase 1: Action Discovery...")
    start = time.time()
    discovered2 = engine2.discover_actions(train_pairs_mirror)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   ✅ Actions découvertes: {discovered2}")
    
    print("\n🎓 Phase 2: Learning Loop...")
    start = time.time()
    learning_result2 = engine2.learn_from_examples(train_pairs_mirror, max_iterations=5)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   ✅ Learned: {learning_result2['learned']}")
    print(f"   🎯 Best action: {learning_result2.get('best_action')}")
    print(f"   📊 Best error: {learning_result2.get('best_error', 1.0):.3f}")
    
    print("\n🔮 Phase 3: Prediction...")
    test_input2 = np.array([[13, 14, 15], [16, 17, 18]])
    test_expected2 = np.array([[15, 14, 13], [18, 17, 16]])
    
    start = time.time()
    predicted2, action2, confidence2 = engine2.predict(test_input2, use_best_action=True)
    print(f"   ⏱️  Durée: {time.time()-start:.3f}s")
    print(f"   🎯 Action: {action2}")
    print(f"   📊 Confidence: {confidence2:.2f}")
    print(f"   ✅ Correct: {np.array_equal(predicted2, test_expected2)}")
    
    # Statistiques finales
    print("\n" + "="*70)
    print("📊 STATISTIQUES FINALES")
    print("="*70)
    
    stats1 = engine.get_statistics()
    print("\n🔬 Engine 1 (Rotation):")
    print(f"   Total transformations: {stats1['metrics']['total_transformations']}")
    print(f"   Succès: {stats1['metrics']['successful_transformations']}")
    print(f"   Échecs: {stats1['metrics']['failed_transformations']}")
    print(f"   Erreur moyenne: {stats1['metrics']['avg_error']:.3f}")
    
    stats2 = engine2.get_statistics()
    print("\n🔬 Engine 2 (Miroir):")
    print(f"   Total transformations: {stats2['metrics']['total_transformations']}")
    print(f"   Succès: {stats2['metrics']['successful_transformations']}")
    print(f"   Échecs: {stats2['metrics']['failed_transformations']}")
    print(f"   Erreur moyenne: {stats2['metrics']['avg_error']:.3f}")
    
    print("\n" + "="*70)
    print("✅ TESTS TERMINÉS")
    print("="*70)


if __name__ == "__main__":
    test_tle_on_simple_cases()

# Made with Bob
