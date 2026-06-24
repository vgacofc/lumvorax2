#!/usr/bin/env python3
"""
Tests Unitaires Complets - Toutes les Couches MAGEN V2
=======================================================

Suite de tests exhaustive pour valider les 10 layers de l'architecture V2.

Couverture:
- Layer 0: Sensorimotor (contingences sensorimotrices)
- Layer 1: Vision (traitement visuel)
- Layer 2: Objects (détection objets)
- Layer 3: Attention (mécanisme attention)
- Layer 4: Causality (raisonnement causal)
- Layer 5: World Model (modèle monde)
- Layer 6: Semantic Memory (mémoire sémantique)
- Layer 7: Reasoning (raisonnement stratégique)
- Layer 8: Meta-Learning (méta-apprentissage)
- Layer 9: Prediction (moteur prédiction)

Méthodologie:
- Tests isolés par layer
- Validation inputs/outputs
- Vérification états internes
- Tests edge cases
- Performance benchmarks

Auteur: Bob (Mode Code)
Date: 2026-06-19
"""

import sys
from pathlib import Path
import numpy as np
import time
from typing import Dict, List, Tuple

# Setup paths
sys.path.insert(0, str(Path(__file__).parent.parent))

# Import all layers
from core.sensorimotor_layer import SensorimotorLayer
from core.vision_layer import VisionLayer
from core.objects_layer import ObjectsLayer
from core.attention_mechanism import AttentionMechanism
from core.causality_layer import CausalityLayer
from core.world_layer import WorldLayer
from core.semantic_layer import SemanticLayer
from core.reasoning_layer import ReasoningLayer
from core.metalearning_layer import MetaLearningLayer
from core.prediction_engine import PredictionEngine


class TestResults:
    """Collecteur de résultats de tests"""
    
    def __init__(self):
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.errors: List[str] = []
        self.timings: Dict[str, float] = {}
    
    def add_test(self, name: str, passed: bool, error: str = None, duration: float = 0.0):
        """Ajoute un résultat de test"""
        self.total_tests += 1
        if passed:
            self.passed_tests += 1
            print(f"  ✅ {name} ({duration:.3f}s)")
        else:
            self.failed_tests += 1
            print(f"  ❌ {name}: {error}")
            self.errors.append(f"{name}: {error}")
        self.timings[name] = duration
    
    def print_summary(self):
        """Affiche résumé des tests"""
        print("\n" + "=" * 70)
        print("RÉSUMÉ DES TESTS")
        print("=" * 70)
        print(f"Total: {self.total_tests} tests")
        print(f"✅ Réussis: {self.passed_tests} ({self.passed_tests/self.total_tests*100:.1f}%)")
        print(f"❌ Échoués: {self.failed_tests}")
        
        if self.errors:
            print("\nERREURS:")
            for error in self.errors:
                print(f"  - {error}")
        
        print(f"\nTemps total: {sum(self.timings.values()):.3f}s")
        print("=" * 70)


def test_layer0_sensorimotor(results: TestResults):
    """Tests Layer 0: Sensorimotor"""
    print("\n[LAYER 0] Sensorimotor Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = SensorimotorLayer()
        assert layer is not None
        results.add_test("L0.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L0.1 Initialisation", False, str(e))
    
    # Test 2: Process grid
    start = time.time()
    try:
        grid = np.random.randint(0, 10, (30, 30))
        output = layer.process(grid)
        assert output is not None
        assert hasattr(output, 'contingencies')
        results.add_test("L0.2 Process grid", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L0.2 Process grid", False, str(e))
    
    # Test 3: Contingencies detection
    start = time.time()
    try:
        grid = np.array([[1, 2], [3, 4]])
        output = layer.process(grid)
        assert len(output.contingencies) > 0
        results.add_test("L0.3 Contingencies detection", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L0.3 Contingencies detection", False, str(e))


def test_layer1_vision(results: TestResults):
    """Tests Layer 1: Vision"""
    print("\n[LAYER 1] Vision Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = VisionLayer()
        assert layer is not None
        results.add_test("L1.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L1.1 Initialisation", False, str(e))
    
    # Test 2: Analyze grid
    start = time.time()
    try:
        grid = np.random.randint(0, 10, (30, 30))
        output = layer.analyze(grid)
        assert output is not None
        assert hasattr(output, 'features')
        results.add_test("L1.2 Analyze grid", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L1.2 Analyze grid", False, str(e))
    
    # Test 3: Feature extraction
    start = time.time()
    try:
        grid = np.ones((10, 10))
        output = layer.analyze(grid)
        assert output.features is not None
        results.add_test("L1.3 Feature extraction", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L1.3 Feature extraction", False, str(e))


def test_layer2_objects(results: TestResults):
    """Tests Layer 2: Objects"""
    print("\n[LAYER 2] Objects Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = ObjectsLayer()
        assert layer is not None
        results.add_test("L2.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L2.1 Initialisation", False, str(e))
    
    # Test 2: Detect objects
    start = time.time()
    try:
        grid = np.random.randint(0, 10, (30, 30))
        output = layer.detect(grid)
        assert output is not None
        assert hasattr(output, 'objects')
        results.add_test("L2.2 Detect objects", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L2.2 Detect objects", False, str(e))
    
    # Test 3: Object properties
    start = time.time()
    try:
        grid = np.array([[1, 1], [1, 1]])
        output = layer.detect(grid)
        assert isinstance(output.objects, list)
        results.add_test("L2.3 Object properties", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L2.3 Object properties", False, str(e))


def test_layer3_attention(results: TestResults):
    """Tests Layer 3: Attention"""
    print("\n[LAYER 3] Attention Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = AttentionMechanism()
        assert layer is not None
        results.add_test("L3.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L3.1 Initialisation", False, str(e))
    
    # Test 2: Focus attention
    start = time.time()
    try:
        grid = np.random.randint(0, 10, (30, 30))
        output = layer.focus(grid)
        assert output is not None
        assert hasattr(output, 'attention_map')
        results.add_test("L3.2 Focus attention", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L3.2 Focus attention", False, str(e))
    
    # Test 3: Attention weights
    start = time.time()
    try:
        grid = np.ones((10, 10))
        output = layer.focus(grid)
        assert output.attention_map is not None
        results.add_test("L3.3 Attention weights", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L3.3 Attention weights", False, str(e))


def test_layer4_causality(results: TestResults):
    """Tests Layer 4: Causality"""
    print("\n[LAYER 4] Causality Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = CausalityLayer()
        assert layer is not None
        results.add_test("L4.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L4.1 Initialisation", False, str(e))
    
    # Test 2: Infer causality
    start = time.time()
    try:
        state = np.random.randint(0, 10, (30, 30))
        action = "move_right"
        output = layer.infer(state, action)
        assert output is not None
        assert hasattr(output, 'causal_links')
        results.add_test("L4.2 Infer causality", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L4.2 Infer causality", False, str(e))
    
    # Test 3: Causal graph
    start = time.time()
    try:
        state = np.ones((10, 10))
        action = "test"
        output = layer.infer(state, action)
        assert output.causal_links is not None
        results.add_test("L4.3 Causal graph", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L4.3 Causal graph", False, str(e))


def test_layer5_world(results: TestResults):
    """Tests Layer 5: World Model"""
    print("\n[LAYER 5] World Model Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = WorldLayer()
        assert layer is not None
        results.add_test("L5.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L5.1 Initialisation", False, str(e))
    
    # Test 2: Update world model
    start = time.time()
    try:
        state = np.random.randint(0, 10, (30, 30))
        output = layer.update(state)
        assert output is not None
        assert hasattr(output, 'world_state')
        results.add_test("L5.2 Update world model", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L5.2 Update world model", False, str(e))
    
    # Test 3: World representation
    start = time.time()
    try:
        state = np.ones((10, 10))
        output = layer.update(state)
        assert output.world_state is not None
        results.add_test("L5.3 World representation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L5.3 World representation", False, str(e))


def test_layer6_semantic(results: TestResults):
    """Tests Layer 6: Semantic Memory"""
    print("\n[LAYER 6] Semantic Memory Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = SemanticLayer()
        assert layer is not None
        results.add_test("L6.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L6.1 Initialisation", False, str(e))
    
    # Test 2: Store concept
    start = time.time()
    try:
        concept = {"type": "pattern", "data": [1, 2, 3]}
        output = layer.store(concept)
        assert output is not None
        results.add_test("L6.2 Store concept", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L6.2 Store concept", False, str(e))
    
    # Test 3: Retrieve concept
    start = time.time()
    try:
        query = {"type": "pattern"}
        output = layer.retrieve(query)
        assert output is not None
        results.add_test("L6.3 Retrieve concept", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L6.3 Retrieve concept", False, str(e))


def test_layer7_reasoning(results: TestResults):
    """Tests Layer 7: Reasoning"""
    print("\n[LAYER 7] Reasoning Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = ReasoningLayer()
        assert layer is not None
        results.add_test("L7.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L7.1 Initialisation", False, str(e))
    
    # Test 2: Reason about state
    start = time.time()
    try:
        state = np.random.randint(0, 10, (30, 30))
        context = {"goal": "explore"}
        output = layer.reason(state, context)
        assert output is not None
        assert hasattr(output, 'strategy')
        results.add_test("L7.2 Reason about state", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L7.2 Reason about state", False, str(e))
    
    # Test 3: Strategy selection
    start = time.time()
    try:
        state = np.ones((10, 10))
        context = {"urgency": "high"}
        output = layer.reason(state, context)
        assert output.strategy is not None
        results.add_test("L7.3 Strategy selection", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L7.3 Strategy selection", False, str(e))


def test_layer8_metalearning(results: TestResults):
    """Tests Layer 8: Meta-Learning"""
    print("\n[LAYER 8] Meta-Learning Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = MetaLearningLayer()
        assert layer is not None
        results.add_test("L8.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L8.1 Initialisation", False, str(e))
    
    # Test 2: Learn from experience
    start = time.time()
    try:
        action = "test_action"
        success = True
        reward = 0.8
        output = layer.learn(action, success, reward)
        assert output is not None
        results.add_test("L8.2 Learn from experience", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L8.2 Learn from experience", False, str(e))
    
    # Test 3: Curriculum progression
    start = time.time()
    try:
        for i in range(10):
            layer.learn(f"action_{i}", True, 0.9)
        assert layer.learning_system.curriculum_phase != "exploration"
        results.add_test("L8.3 Curriculum progression", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L8.3 Curriculum progression", False, str(e))


def test_layer9_prediction(results: TestResults):
    """Tests Layer 9: Prediction"""
    print("\n[LAYER 9] Prediction Tests...")
    
    # Test 1: Initialisation
    start = time.time()
    try:
        layer = PredictionEngine()
        assert layer is not None
        results.add_test("L9.1 Initialisation", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L9.1 Initialisation", False, str(e))
    
    # Test 2: Predict next state
    start = time.time()
    try:
        state = np.random.randint(0, 10, (30, 30))
        action = "move_right"
        output = layer.predict(state, action)
        assert output is not None
        assert hasattr(output, 'predicted_state')
        results.add_test("L9.2 Predict next state", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L9.2 Predict next state", False, str(e))
    
    # Test 3: Prediction confidence
    start = time.time()
    try:
        state = np.ones((10, 10))
        action = "test"
        output = layer.predict(state, action)
        assert hasattr(output, 'confidence')
        assert 0 <= output.confidence <= 1
        results.add_test("L9.3 Prediction confidence", True, duration=time.time()-start)
    except Exception as e:
        results.add_test("L9.3 Prediction confidence", False, str(e))


def run_all_tests():
    """Exécute tous les tests"""
    print("=" * 70)
    print("TESTS UNITAIRES - TOUTES LES COUCHES MAGEN V2")
    print("=" * 70)
    
    results = TestResults()
    
    # Tests par layer
    test_layer0_sensorimotor(results)
    test_layer1_vision(results)
    test_layer2_objects(results)
    test_layer3_attention(results)
    test_layer4_causality(results)
    test_layer5_world(results)
    test_layer6_semantic(results)
    test_layer7_reasoning(results)
    test_layer8_metalearning(results)
    test_layer9_prediction(results)
    
    # Résumé
    results.print_summary()
    
    return results.passed_tests == results.total_tests


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)

# Made with Bob
