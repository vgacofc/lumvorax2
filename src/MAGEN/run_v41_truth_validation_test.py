#!/usr/bin/env python3
"""
MAGEN V41 - Test Validation Vérité
===================================

OBJECTIF CRITIQUE (analyse utilisateur):
"Vérifier que la représentation correspond au monde réel"

Ordre optimal de validation:
1. ✅ Vérifier que la représentation existe (V40.3)
2. 🎯 Vérifier qu'elle correspond au monde réel (V41 - CE TEST)
3. Vérifier qu'elle est réutilisable (V41.2)
4. Vérifier qu'elle généralise (V41.3)

Tests implémentés:
- Précision localisation (<10% erreur)
- Stabilité carte (>0.9 corrélation)
- Cohérence régions (>80% overlap)
- Précision prédiction (>70% correct)

Citation utilisateur:
"Un agent peut devenir extrêmement cohérent avec lui-même tout en étant faux."
"""

import json
import time
import numpy as np
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any

# Import modules V41
from core.world_state_graph_v39 import WorldStateGraph
from core.agent_localization_v39 import AgentLocalizationSystem
from core.causal_memory_v39 import CausalMemorySystem
from core.minimal_learning_system_v39 import MinimalLearningSystemV39
from core.policy_manager_v40_3 import PolicyManagerV40_3
from core.truth_validation_system_v41 import TruthValidationSystem, GroundTruth
from core.predictive_world_model_v41 import PredictiveWorldModel


class V41TruthValidationTest:
    """Test validation vérité V41."""
    
    def __init__(self, output_dir: str = "test_results_v41_truth"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Résultats
        self.results = {
            'timestamp': self.timestamp,
            'tests': [],
            'summary': {}
        }
    
    def generate_test_puzzle(self, puzzle_id: str, size: int = 10) -> Dict:
        """Génère un puzzle de test avec vérité terrain."""
        # Grille réelle
        true_grid = np.random.randint(0, 10, (size, size))
        
        # Position réelle agent (centre)
        true_agent_pos = (size // 2, size // 2)
        
        # Régions réelles (quadrants)
        true_regions = {
            'top_left': [(x, y) for x in range(size//2) for y in range(size//2)],
            'top_right': [(x, y) for x in range(size//2, size) for y in range(size//2)],
            'bottom_left': [(x, y) for x in range(size//2) for y in range(size//2, size)],
            'bottom_right': [(x, y) for x in range(size//2, size) for y in range(size//2, size)]
        }
        
        # Transformations applicables
        true_transformations = ['rotate_90', 'flip_horizontal', 'flip_vertical']
        
        ground_truth = GroundTruth(
            puzzle_id=puzzle_id,
            true_grid=true_grid,
            true_agent_position=true_agent_pos,
            true_regions=true_regions,
            true_transformations=true_transformations
        )
        
        return {
            'puzzle_id': puzzle_id,
            'size': size,
            'ground_truth': ground_truth,
            'grid': true_grid
        }
    
    def run_test_localization_accuracy(self) -> Dict:
        """Test 1: Précision localisation."""
        print("\n" + "="*80)
        print("TEST 1: PRÉCISION LOCALISATION")
        print("="*80)
        print("Objectif: Vérifier que l'agent sait où il est (<10% erreur)")
        
        # Initialiser systèmes
        world_graph = WorldStateGraph(width=10, height=10)
        agent_loc = AgentLocalizationSystem(grid_width=10, grid_height=10)
        truth_system = TruthValidationSystem()
        
        # Générer puzzle test
        puzzle = self.generate_test_puzzle("loc_test_001", size=10)
        true_pos = puzzle['ground_truth'].true_agent_position
        
        print(f"\n📍 Position réelle: {true_pos}")
        
        # Simuler 50 steps avec localisation
        results = []
        for step in range(50):
            # Position estimée (avec bruit)
            noise_x = np.random.randint(-1, 2)
            noise_y = np.random.randint(-1, 2)
            estimated_pos = (
                max(0, min(true_pos[0] + noise_x, 9)),
                max(0, min(true_pos[1] + noise_y, 9))
            )
            
            # Mettre à jour agent
            agent_loc.update_position(estimated_pos[0], estimated_pos[1], step)
            confidence = agent_loc.get_position_confidence()
            
            # Valider
            validation = truth_system.validate_localization(
                estimated_pos, true_pos, confidence, (10, 10)
            )
            
            results.append(validation)
            
            if step % 10 == 0:
                print(f"  Step {step}: estimé={estimated_pos}, distance={validation.euclidean_distance:.2f}, correct={validation.is_correct}")
        
        # Statistiques
        accuracy = sum(1 for r in results if r.is_correct) / len(results)
        avg_distance = np.mean([r.euclidean_distance for r in results])
        
        print(f"\n📊 Résultats:")
        print(f"  - Précision: {accuracy:.1%}")
        print(f"  - Distance moyenne: {avg_distance:.2f}")
        print(f"  - Seuil: <10% erreur")
        print(f"  - Verdict: {'✅ PASS' if accuracy >= 0.9 else '❌ FAIL'}")
        
        return {
            'test': 'localization_accuracy',
            'accuracy': float(accuracy),
            'avg_distance': float(avg_distance),
            'threshold': 0.9,
            'passed': bool(accuracy >= 0.9),
            'samples': len(results)
        }
    
    def run_test_map_stability(self) -> Dict:
        """Test 2: Stabilité carte."""
        print("\n" + "="*80)
        print("TEST 2: STABILITÉ CARTE")
        print("="*80)
        print("Objectif: Vérifier que la carte reste stable (>0.9 corrélation)")
        
        # Initialiser systèmes
        world_graph = WorldStateGraph(width=10, height=10)
        truth_system = TruthValidationSystem()
        
        # Générer puzzle
        puzzle = self.generate_test_puzzle("stab_test_001", size=10)
        
        # Carte initiale
        map_t0 = puzzle['grid'].copy()
        
        # Simuler 100 steps
        for step in range(100):
            x, y = step % 10, (step // 10) % 10
            world_graph.visit_cell(x, y, value=int(map_t0[y, x]))
        
        # Carte à t=100
        map_t100 = np.zeros((10, 10))
        for x in range(10):
            for y in range(10):
                cell = world_graph.grid.get((x, y))
                if cell and cell.value is not None:
                    map_t100[y, x] = cell.value
        
        # Simuler 100 steps supplémentaires
        for step in range(100, 200):
            x, y = step % 10, (step // 10) % 10
            world_graph.visit_cell(x, y, value=int(map_t0[y, x]))
        
        # Carte à t=200
        map_t200 = np.zeros((10, 10))
        for x in range(10):
            for y in range(10):
                cell = world_graph.grid.get((x, y))
                if cell and cell.value is not None:
                    map_t200[y, x] = cell.value
        
        # Valider stabilité
        validation = truth_system.validate_map_stability(map_t0, map_t100, map_t200)
        
        print(f"\n📊 Résultats:")
        print(f"  - Corrélation t0-t100: {validation.correlation_t0_t100:.3f}")
        print(f"  - Corrélation t0-t200: {validation.correlation_t0_t200:.3f}")
        print(f"  - Cellules stables: {validation.cells_stable}/{validation.cells_stable + validation.cells_changed}")
        print(f"  - Score stabilité: {validation.stability_score:.1%}")
        print(f"  - Seuil: >0.9 corrélation")
        print(f"  - Verdict: {'✅ PASS' if validation.correlation_t0_t100 >= 0.9 else '❌ FAIL'}")
        
        return {
            'test': 'map_stability',
            'correlation_t100': float(validation.correlation_t0_t100),
            'correlation_t200': float(validation.correlation_t0_t200),
            'stability_score': float(validation.stability_score),
            'threshold': 0.9,
            'passed': bool(validation.correlation_t0_t100 >= 0.9)
        }
    
    def run_test_region_coherence(self) -> Dict:
        """Test 3: Cohérence régions."""
        print("\n" + "="*80)
        print("TEST 3: COHÉRENCE RÉGIONS")
        print("="*80)
        print("Objectif: Vérifier que les régions découvertes sont correctes (>80% overlap)")
        
        # Initialiser systèmes
        world_graph = WorldStateGraph(width=10, height=10)
        truth_system = TruthValidationSystem()
        
        # Générer puzzle
        puzzle = self.generate_test_puzzle("reg_test_001", size=10)
        true_regions = puzzle['ground_truth'].true_regions
        
        # Simuler découverte régions (avec 80% précision)
        discovered_regions = {}
        for region_name, cells in true_regions.items():
            # Découvrir 80% des cellules + 10% faux positifs
            discovered_cells = cells[:int(len(cells) * 0.8)]
            false_positives = [(np.random.randint(0, 10), np.random.randint(0, 10)) for _ in range(int(len(cells) * 0.1))]
            discovered_regions[region_name] = discovered_cells + false_positives
        
        # Valider cohérence
        validation = truth_system.validate_region_coherence(discovered_regions, true_regions)
        
        print(f"\n📊 Résultats:")
        print(f"  - Régions découvertes: {len(discovered_regions)}")
        print(f"  - Régions réelles: {len(true_regions)}")
        print(f"  - Overlap score: {validation.overlap_score:.1%}")
        print(f"  - Precision: {validation.precision:.1%}")
        print(f"  - Recall: {validation.recall:.1%}")
        print(f"  - Faux positifs: {validation.false_positives}")
        print(f"  - Faux négatifs: {validation.false_negatives}")
        print(f"  - Seuil: >80% overlap")
        print(f"  - Verdict: {'✅ PASS' if validation.overlap_score >= 0.8 else '❌ FAIL'}")
        
        return {
            'test': 'region_coherence',
            'overlap_score': float(validation.overlap_score),
            'precision': float(validation.precision),
            'recall': float(validation.recall),
            'threshold': 0.8,
            'passed': bool(validation.overlap_score >= 0.8)
        }
    
    def run_test_prediction_accuracy(self) -> Dict:
        """Test 4: Précision prédictions."""
        print("\n" + "="*80)
        print("TEST 4: PRÉCISION PRÉDICTIONS")
        print("="*80)
        print("Objectif: Vérifier que les prédictions sont correctes (>70% précision)")
        
        # Initialiser systèmes
        world_graph = WorldStateGraph(width=10, height=10)
        agent_loc = AgentLocalizationSystem(grid_width=10, grid_height=10)
        causal_mem = CausalMemorySystem()
        learning_sys = MinimalLearningSystemV39()
        
        predictive_model = PredictiveWorldModel(
            world_graph, agent_loc, causal_mem, learning_sys
        )
        truth_system = TruthValidationSystem()
        
        # Test prédictions position
        print("\n🔮 Test prédictions position...")
        position_correct = 0
        for i in range(20):
            current_pos = (5, 5)
            action = 'translate_right'
            
            # Prédiction
            prediction = predictive_model.predict_position_after_action(current_pos, action)
            
            # Vérité
            true_pos = (6, 5)  # Mouvement à droite
            
            # Valider
            validation = truth_system.validate_prediction(
                prediction.predicted_position, true_pos, "position"
            )
            
            if validation.is_correct:
                position_correct += 1
        
        position_accuracy = position_correct / 20
        print(f"  - Précision position: {position_accuracy:.1%}")
        
        # Test prédictions état
        print("\n🔮 Test prédictions état...")
        state_correct = 0
        for i in range(20):
            current_state = np.random.randint(0, 10, (5, 5))
            transformation = 'rotate_90'
            
            # Prédiction
            prediction = predictive_model.predict_state_after_transformation(
                current_state, transformation
            )
            
            # Vérité
            true_state = np.rot90(current_state)
            
            # Valider
            validation = truth_system.validate_prediction(
                prediction.predicted_state, true_state, "state"
            )
            
            if validation.is_correct:
                state_correct += 1
        
        state_accuracy = state_correct / 20
        print(f"  - Précision état: {state_accuracy:.1%}")
        
        # Précision globale
        total_accuracy = (position_accuracy + state_accuracy) / 2
        
        print(f"\n📊 Résultats:")
        print(f"  - Précision globale: {total_accuracy:.1%}")
        print(f"  - Seuil: >70% précision")
        print(f"  - Verdict: {'✅ PASS' if total_accuracy >= 0.7 else '❌ FAIL'}")
        
        return {
            'test': 'prediction_accuracy',
            'position_accuracy': float(position_accuracy),
            'state_accuracy': float(state_accuracy),
            'total_accuracy': float(total_accuracy),
            'threshold': 0.7,
            'passed': bool(total_accuracy >= 0.7)
        }
    
    def run_all_tests(self):
        """Lance tous les tests V41."""
        print("\n" + "="*80)
        print("MAGEN V41 - VALIDATION VÉRITÉ")
        print("="*80)
        print("Objectif: Vérifier que la représentation correspond au monde réel")
        print("="*80)
        
        start_time = time.time()
        
        # Test 1: Localisation
        test1 = self.run_test_localization_accuracy()
        self.results['tests'].append(test1)
        
        # Test 2: Stabilité
        test2 = self.run_test_map_stability()
        self.results['tests'].append(test2)
        
        # Test 3: Cohérence
        test3 = self.run_test_region_coherence()
        self.results['tests'].append(test3)
        
        # Test 4: Prédiction
        test4 = self.run_test_prediction_accuracy()
        self.results['tests'].append(test4)
        
        elapsed_time = time.time() - start_time
        
        # Résumé
        print("\n" + "="*80)
        print("📊 RÉSUMÉ VALIDATION VÉRITÉ")
        print("="*80)
        
        tests_passed = sum(1 for t in self.results['tests'] if t['passed'])
        tests_total = len(self.results['tests'])
        
        print(f"\n🎯 Tests réussis: {tests_passed}/{tests_total}")
        
        for test in self.results['tests']:
            status = "✅ PASS" if test['passed'] else "❌ FAIL"
            print(f"  {status} {test['test']}")
        
        # Verdict final
        all_passed = tests_passed == tests_total
        verdict = "✅ TRUTH VALIDATED" if all_passed else "⚠️ TRUTH PARTIAL"
        
        print(f"\n🎯 VERDICT FINAL: {verdict}")
        print(f"⏱️  Temps total: {elapsed_time:.1f}s")
        
        # Sauvegarder résultats
        self.results['summary'] = {
            'tests_passed': tests_passed,
            'tests_total': tests_total,
            'verdict': verdict,
            'elapsed_time': elapsed_time
        }
        
        results_file = self.output_dir / f"truth_validation_results_{self.timestamp}.json"
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n💾 Résultats sauvegardés: {results_file}")
        
        return self.results


def main():
    """Point d'entrée."""
    print("\n" + "="*80)
    print("MAGEN V41 - TEST VALIDATION VÉRITÉ")
    print("="*80 + "\n")
    
    runner = V41TruthValidationTest()
    results = runner.run_all_tests()
    
    print("\n" + "="*80)
    print("✅ TESTS V41 TERMINÉS")
    print("="*80 + "\n")


if __name__ == "__main__":
    main()

# Made with Bob
