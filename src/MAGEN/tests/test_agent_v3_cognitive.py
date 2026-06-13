"""
Test complet Agent V3 Cognitive
Validation RuleExtractor + HypothesisTester + Agent V3
"""

import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent))

import numpy as np
from agent.arc_agent_v3_cognitive import ARCAgentV3Cognitive
from core.magen_memory import MAGENMemory
from perception.arc_perception import ARCPerception


def test_rotation_90():
    """Test avec rotation 90°"""
    print("\n" + "="*70)
    print("TEST 1: Rotation 90°")
    print("="*70)
    
    # Créer composants
    save_dir = Path("/tmp/magen_test_v3")
    save_dir.mkdir(exist_ok=True)
    
    memory = MAGENMemory(save_dir=str(save_dir))
    perception = ARCPerception(max_grid_size=64)
    
    # Créer agent V3
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        verbose=True
    )
    
    # Démarrer jeu
    agent.start_new_game("test_rotation_90")
    
    # Training pairs (rotation 90°)
    inp1 = np.array([[1, 2], [3, 4]])
    out1 = np.rot90(inp1, k=1)
    
    inp2 = np.array([[5, 6], [7, 8]])
    out2 = np.rot90(inp2, k=1)
    
    training_pairs = [(inp1, out1), (inp2, out2)]
    
    # Test input
    test_input = np.array([[9, 10], [11, 12]])
    expected = np.rot90(test_input, k=1)
    
    # Analyser et prédire
    agent.analyze_training_pairs(training_pairs)
    predicted = agent.predict_test_output(test_input)
    
    # Vérifier
    if predicted is not None:
        print(f"\n✅ Prédiction générée:")
        print(f"Input:\n{test_input}")
        print(f"\nPredicted:\n{predicted}")
        print(f"\nExpected:\n{expected}")
        
        if np.array_equal(predicted, expected):
            print(f"\n🎉 TEST RÉUSSI - Prédiction correcte!")
            return True
        else:
            print(f"\n❌ TEST ÉCHOUÉ - Prédiction incorrecte")
            return False
    else:
        print(f"\n❌ TEST ÉCHOUÉ - Pas de prédiction")
        return False


def test_flip_horizontal():
    """Test avec flip horizontal"""
    print("\n" + "="*70)
    print("TEST 2: Flip Horizontal")
    print("="*70)
    
    # Créer composants
    save_dir = Path("/tmp/magen_test_v3")
    save_dir.mkdir(exist_ok=True)
    
    memory = MAGENMemory(save_dir=str(save_dir))
    perception = ARCPerception(max_grid_size=64)
    
    # Créer agent V3
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        verbose=True
    )
    
    # Démarrer jeu
    agent.start_new_game("test_flip_h")
    
    # Training pairs (flip horizontal)
    inp1 = np.array([[1, 2, 3], [4, 5, 6]])
    out1 = np.fliplr(inp1)
    
    inp2 = np.array([[7, 8, 9], [10, 11, 12]])
    out2 = np.fliplr(inp2)
    
    training_pairs = [(inp1, out1), (inp2, out2)]
    
    # Test input
    test_input = np.array([[13, 14, 15], [16, 17, 18]])
    expected = np.fliplr(test_input)
    
    # Analyser et prédire
    agent.analyze_training_pairs(training_pairs)
    predicted = agent.predict_test_output(test_input)
    
    # Vérifier
    if predicted is not None:
        print(f"\n✅ Prédiction générée:")
        print(f"Input:\n{test_input}")
        print(f"\nPredicted:\n{predicted}")
        print(f"\nExpected:\n{expected}")
        
        if np.array_equal(predicted, expected):
            print(f"\n🎉 TEST RÉUSSI - Prédiction correcte!")
            return True
        else:
            print(f"\n❌ TEST ÉCHOUÉ - Prédiction incorrecte")
            return False
    else:
        print(f"\n❌ TEST ÉCHOUÉ - Pas de prédiction")
        return False


def test_color_mapping():
    """Test avec mapping de couleurs"""
    print("\n" + "="*70)
    print("TEST 3: Color Mapping")
    print("="*70)
    
    # Créer composants
    save_dir = Path("/tmp/magen_test_v3")
    save_dir.mkdir(exist_ok=True)
    
    memory = MAGENMemory(save_dir=str(save_dir))
    perception = ARCPerception(max_grid_size=64)
    
    # Créer agent V3
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        verbose=True
    )
    
    # Démarrer jeu
    agent.start_new_game("test_color_map")
    
    # Training pairs (mapping: 1→2, 2→3, 3→1)
    inp1 = np.array([[1, 2], [3, 1]])
    out1 = np.array([[2, 3], [1, 2]])
    
    inp2 = np.array([[2, 3], [1, 2]])
    out2 = np.array([[3, 1], [2, 3]])
    
    training_pairs = [(inp1, out1), (inp2, out2)]
    
    # Test input
    test_input = np.array([[3, 1], [2, 3]])
    expected = np.array([[1, 2], [3, 1]])
    
    # Analyser et prédire
    agent.analyze_training_pairs(training_pairs)
    predicted = agent.predict_test_output(test_input)
    
    # Vérifier
    if predicted is not None:
        print(f"\n✅ Prédiction générée:")
        print(f"Input:\n{test_input}")
        print(f"\nPredicted:\n{predicted}")
        print(f"\nExpected:\n{expected}")
        
        if np.array_equal(predicted, expected):
            print(f"\n🎉 TEST RÉUSSI - Prédiction correcte!")
            return True
        else:
            print(f"\n❌ TEST ÉCHOUÉ - Prédiction incorrecte")
            return False
    else:
        print(f"\n❌ TEST ÉCHOUÉ - Pas de prédiction")
        return False


if __name__ == "__main__":
    print("🧪 TESTS AGENT V3 COGNITIVE")
    print("="*70)
    
    results = []
    
    # Test 1: Rotation 90°
    results.append(("Rotation 90°", test_rotation_90()))
    
    # Test 2: Flip horizontal
    results.append(("Flip Horizontal", test_flip_horizontal()))
    
    # Test 3: Color mapping
    results.append(("Color Mapping", test_color_mapping()))
    
    # Résumé
    print("\n" + "="*70)
    print("📊 RÉSUMÉ DES TESTS")
    print("="*70)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{status} - {name}")
    
    print(f"\n🎯 Score: {passed}/{total} ({passed/total*100:.0f}%)")
    
    if passed == total:
        print("\n🎉 TOUS LES TESTS RÉUSSIS!")
        print("✅ Agent V3 Cognitive opérationnel")
        print("✅ RuleExtractor fonctionnel")
        print("✅ HypothesisTester fonctionnel")
        print("\n🚀 Prêt pour LOOP 2 avec jeux ARC-AGI-3 réels")
    else:
        print(f"\n⚠️ {total - passed} test(s) échoué(s)")
        print("🔧 Corrections nécessaires avant LOOP 2")

# Made with Bob
