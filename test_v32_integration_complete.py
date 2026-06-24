#!/usr/bin/env python3
"""
Test d'intégration complet V32 Spatial Cognitive System
Tests unitaires + intégration + validation ARC Arcade
"""

import sys
import json
import time
from pathlib import Path
from typing import Dict, List, Any, Tuple
from dataclasses import dataclass, asdict

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / "src"))

from MAGEN.core.mental_map_builder import MentalMapBuilder
from MAGEN.core.spatial_compressor import SpatialCompressor
from MAGEN.core.regional_strategist import RegionalStrategist
from MAGEN.core.cognitive_stabilizer import CognitiveStabilizer
from MAGEN.core.metacognitive_monitor import MetaCognitiveMonitor
from MAGEN.core.self_spatial_identity import SelfSpatialIdentity
from MAGEN.core.goal_hypothesis_engine import GoalHypothesisEngine
from MAGEN.core.trajectory_meaning_system import TrajectoryMeaningSystem
from MAGEN.core.spatial_knowledge_graph import SpatialKnowledgeGraph
from MAGEN.core.decision_kernel_v32_spatial import DecisionKernelV32Spatial

# Import environnements de test
from MAGEN.environments.arc_arcade_api import ARCEnvironment


@dataclass
class TestResult:
    """Résultat d'un test"""
    test_name: str
    module: str
    passed: bool
    duration_ms: float
    details: Dict[str, Any]
    error: str = ""


class V32IntegrationTester:
    """Testeur d'intégration V32"""
    
    def __init__(self):
        self.results: List[TestResult] = []
        self.start_time = time.time()
        
    def log(self, message: str):
        """Log avec timestamp"""
        elapsed = time.time() - self.start_time
        print(f"[{elapsed:8.3f}s] {message}")
        
    # ========================================
    # TESTS UNITAIRES MODULES
    # ========================================
    
    def test_c1_mental_map(self) -> TestResult:
        """Test C1: MentalMapBuilder"""
        self.log("TEST C1: MentalMapBuilder")
        start = time.time()
        
        try:
            builder = MentalMapBuilder()
            
            # Simuler exploration
            positions = [(0,0), (1,0), (2,0), (2,1), (2,2)]
            for i, pos in enumerate(positions):
                obs = {"grid": [[0]*5 for _ in range(5)]}
                builder.update(pos, "right", 0.0, obs, i)
            
            state = builder.get_state()
            
            # Vérifications
            assert len(state["visited_positions"]) == 5, "Positions visitées"
            assert state["total_steps"] == 5, "Steps comptés"
            assert len(state["regions"]) > 0, "Régions formées"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c1_mental_map",
                module="C1_MentalMapBuilder",
                passed=True,
                duration_ms=duration,
                details={
                    "positions_visited": len(state["visited_positions"]),
                    "regions_formed": len(state["regions"]),
                    "total_steps": state["total_steps"]
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c1_mental_map",
                module="C1_MentalMapBuilder",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c2_spatial_compressor(self) -> TestResult:
        """Test C2: SpatialCompressor"""
        self.log("TEST C2: SpatialCompressor")
        start = time.time()
        
        try:
            compressor = SpatialCompressor()
            
            # Trajectoire linéaire
            positions = [(i, 0) for i in range(5)]
            actions = ["right"] * 4
            rewards = [0.0] * 4
            
            pattern = compressor.compress_trajectory(positions, actions, rewards, 0)
            
            # Vérifications
            assert pattern is not None, "Pattern généré"
            assert pattern.pattern_type in ["linear_probe", "goal_approach"], "Type valide"
            
            state = compressor.get_state()
            assert state["total_patterns"] == 1, "Pattern enregistré"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c2_spatial_compressor",
                module="C2_SpatialCompressor",
                passed=True,
                duration_ms=duration,
                details={
                    "pattern_type": pattern.pattern_type,
                    "cognitive_label": pattern.cognitive_label,
                    "total_patterns": state["total_patterns"]
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c2_spatial_compressor",
                module="C2_SpatialCompressor",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c3_regional_strategist(self) -> TestResult:
        """Test C3: RegionalStrategist"""
        self.log("TEST C3: RegionalStrategist")
        start = time.time()
        
        try:
            strategist = RegionalStrategist()
            
            # Région test
            region_data = {
                "positions": {(0,0), (1,0), (2,0)},
                "visit_count": 3,
                "total_reward": 0.5,
                "region_type": "corridor"
            }
            
            assessment = strategist.evaluate_region("region_1", region_data, 10)
            
            # Vérifications
            assert 0.0 <= assessment.strategic_value <= 1.0, "Valeur valide"
            assert assessment.cognitive_status in ["sterile", "promising", "goal_candidate", "explored", "neutral"], "Statut valide"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c3_regional_strategist",
                module="C3_RegionalStrategist",
                passed=True,
                duration_ms=duration,
                details={
                    "strategic_value": assessment.strategic_value,
                    "cognitive_status": assessment.cognitive_status,
                    "exploration_priority": assessment.exploration_priority
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c3_regional_strategist",
                module="C3_RegionalStrategist",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c4_cognitive_stabilizer(self) -> TestResult:
        """Test C4: CognitiveStabilizer"""
        self.log("TEST C4: CognitiveStabilizer")
        start = time.time()
        
        try:
            stabilizer = CognitiveStabilizer()
            
            # Test changement stratégie
            evidence = {
                "collapse_detected": False,
                "stagnation_detected": False,
                "sterility_pressure": 0.3,
                "viability": 0.7,
                "contradiction_strength": 0.1
            }
            
            # Premier changement (devrait être autorisé)
            can_change_1 = stabilizer.should_change_strategy(evidence, 0)
            
            # Changement immédiat (devrait être refusé - cooldown)
            can_change_2 = stabilizer.should_change_strategy(evidence, 1)
            
            state = stabilizer.get_state()
            
            # Vérifications
            assert state["total_steps"] == 2, "Steps comptés"
            assert state["break_frequency"] < 0.10, "Contrainte respectée"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c4_cognitive_stabilizer",
                module="C4_CognitiveStabilizer",
                passed=True,
                duration_ms=duration,
                details={
                    "first_change_allowed": can_change_1,
                    "second_change_blocked": not can_change_2,
                    "break_frequency": state["break_frequency"],
                    "breaks_prevented": state["breaks_prevented"]
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c4_cognitive_stabilizer",
                module="C4_CognitiveStabilizer",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c5_metacognitive_monitor(self) -> TestResult:
        """Test C5: MetaCognitiveMonitor"""
        self.log("TEST C5: MetaCognitiveMonitor")
        start = time.time()
        
        try:
            monitor = MetaCognitiveMonitor()
            
            # États simulés
            mental_map_state = {
                "visited_positions": [(i,0) for i in range(10)],
                "regions": [{"region_id": "r1"}],
                "total_steps": 10
            }
            
            compression_state = {
                "total_patterns": 3,
                "pattern_diversity": 0.6
            }
            
            strategic_state = {
                "regions_evaluated": 1
            }
            
            stability_state = {
                "break_frequency": 0.05,
                "total_breaks": 1
            }
            
            health = monitor.evaluate_cognitive_health(
                mental_map_state,
                compression_state,
                strategic_state,
                stability_state,
                10
            )
            
            # Vérifications
            assert 0.0 <= health.cognitive_health <= 1.0, "Santé valide"
            assert 0.0 <= health.map_coherence <= 1.0, "Cohérence valide"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c5_metacognitive_monitor",
                module="C5_MetaCognitiveMonitor",
                passed=True,
                duration_ms=duration,
                details={
                    "cognitive_health": health.cognitive_health,
                    "map_coherence": health.map_coherence,
                    "strategic_clarity": health.strategic_clarity
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c5_metacognitive_monitor",
                module="C5_MetaCognitiveMonitor",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c6_self_spatial_identity(self) -> TestResult:
        """Test C6: SelfSpatialIdentity"""
        self.log("TEST C6: SelfSpatialIdentity")
        start = time.time()
        
        try:
            identity = SelfSpatialIdentity()
            
            # Observations cohérentes
            for i in range(5):
                obs = {"grid": [[0]*5 for _ in range(5)]}
                obs["grid"][i][0] = 1  # Avatar se déplace
                
                state = identity.update_from_observation(obs, "right", i)
            
            # Vérifications
            assert state.spatial_awareness > 0.0, "Awareness > 0"
            assert len(state.hypotheses) > 0, "Hypothèses formées"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c6_self_spatial_identity",
                module="C6_SelfSpatialIdentity",
                passed=True,
                duration_ms=duration,
                details={
                    "spatial_awareness": state.spatial_awareness,
                    "identity_stability": state.identity_stability,
                    "hypotheses_count": len(state.hypotheses),
                    "avatar_locked": state.avatar_locked
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c6_self_spatial_identity",
                module="C6_SelfSpatialIdentity",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c7_goal_hypothesis_engine(self) -> TestResult:
        """Test C7: GoalHypothesisEngine"""
        self.log("TEST C7: GoalHypothesisEngine")
        start = time.time()
        
        try:
            engine = GoalHypothesisEngine()
            
            # Observations avec récompense
            obs = {"grid": [[0]*5 for _ in range(5)]}
            mental_map_state = {"visited_positions": [(0,0)], "regions": []}
            strategic_state = {"regions_evaluated": 0}
            
            for i in range(5):
                reward = 1.0 if i == 4 else 0.0
                hypotheses = engine.update(obs, reward, mental_map_state, strategic_state, i)
            
            state = engine.get_state()
            
            # Vérifications
            assert state["total_hypotheses_generated"] > 0, "Hypothèses générées"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c7_goal_hypothesis_engine",
                module="C7_GoalHypothesisEngine",
                passed=True,
                duration_ms=duration,
                details={
                    "hypotheses_generated": state["total_hypotheses_generated"],
                    "hypotheses_validated": state["hypotheses_validated"],
                    "active_hypotheses": state["active_hypotheses"]
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c7_goal_hypothesis_engine",
                module="C7_GoalHypothesisEngine",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c8_trajectory_meaning(self) -> TestResult:
        """Test C8: TrajectoryMeaningSystem"""
        self.log("TEST C8: TrajectoryMeaningSystem")
        start = time.time()
        
        try:
            meaning = TrajectoryMeaningSystem()
            
            # Trajectoire avec pattern
            from MAGEN.core.spatial_compressor import CompressedPattern
            pattern = CompressedPattern(
                pattern_type="linear_probe",
                start_position=(0,0),
                end_position=(4,0),
                length=5,
                cognitive_label="productive_exploration",
                information_gain=0.8,
                strategic_value=0.7
            )
            
            for i in range(5):
                state = meaning.update("right", (i,0), 0.0, pattern, i)
            
            # Vérifications
            assert state["total_segments"] > 0, "Segments créés"
            assert state["total_steps"] == 5, "Steps comptés"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c8_trajectory_meaning",
                module="C8_TrajectoryMeaningSystem",
                passed=True,
                duration_ms=duration,
                details={
                    "total_segments": state["total_segments"],
                    "semantic_diversity": state["semantic_diversity"]
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c8_trajectory_meaning",
                module="C8_TrajectoryMeaningSystem",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    def test_c9_knowledge_graph(self) -> TestResult:
        """Test C9: SpatialKnowledgeGraph"""
        self.log("TEST C9: SpatialKnowledgeGraph")
        start = time.time()
        
        try:
            graph = SpatialKnowledgeGraph()
            
            # Construire graphe simple
            positions = [(0,0), (1,0), (2,0)]
            for pos in positions:
                graph.add_position_node(pos, {"visited": True})
            
            # Ajouter edges
            graph.add_edge((0,0), (1,0), "adjacent", {"distance": 1})
            graph.add_edge((1,0), (2,0), "adjacent", {"distance": 1})
            
            # Chercher chemin
            path = graph.find_path((0,0), (2,0))
            
            # Vérifications
            assert path is not None, "Chemin trouvé"
            assert len(path) == 3, "Chemin correct"
            
            state = graph.get_state()
            assert state["total_nodes"] == 3, "Nodes créés"
            assert state["total_edges"] == 2, "Edges créés"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_c9_knowledge_graph",
                module="C9_SpatialKnowledgeGraph",
                passed=True,
                duration_ms=duration,
                details={
                    "total_nodes": state["total_nodes"],
                    "total_edges": state["total_edges"],
                    "path_found": path is not None,
                    "path_length": len(path) if path else 0
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_c9_knowledge_graph",
                module="C9_SpatialKnowledgeGraph",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    # ========================================
    # TEST INTÉGRATION COMPLÈTE
    # ========================================
    
    def test_decision_kernel_integration(self) -> TestResult:
        """Test intégration DecisionKernelV32Spatial"""
        self.log("TEST INTÉGRATION: DecisionKernelV32Spatial")
        start = time.time()
        
        try:
            kernel = DecisionKernelV32Spatial()
            
            # Simuler épisode
            obs = {"grid": [[0]*5 for _ in range(5)]}
            
            for step in range(10):
                # Process observation
                cognitive_state = kernel.process_observation(obs, "right", 0.0, step)
                
                # Decide action
                action, justification = kernel.decide_action(
                    cognitive_state,
                    ["up", "down", "left", "right"]
                )
                
                # Vérifications
                assert action in ["up", "down", "left", "right"], "Action valide"
                assert len(justification) > 0, "Justification fournie"
            
            # Vérifier état final
            final_state = kernel.get_full_state()
            
            assert final_state["total_steps"] == 10, "Steps comptés"
            assert "cognitive_health" in final_state, "Santé cognitive"
            assert "spatial_awareness" in final_state, "Awareness spatiale"
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name="test_decision_kernel_integration",
                module="DecisionKernelV32Spatial",
                passed=True,
                duration_ms=duration,
                details={
                    "total_steps": final_state["total_steps"],
                    "cognitive_health": final_state.get("cognitive_health", 0.0),
                    "spatial_awareness": final_state.get("spatial_awareness", 0.0),
                    "policy_breaks": final_state.get("policy_breaks", 0)
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name="test_decision_kernel_integration",
                module="DecisionKernelV32Spatial",
                passed=False,
                duration_ms=duration,
                details={},
                error=str(e)
            )
    
    # ========================================
    # TESTS ARC ARCADE
    # ========================================
    
    def test_arc_arcade_game(self, game_id: str, max_steps: int = 1000) -> TestResult:
        """Test sur un jeu ARC Arcade réel"""
        self.log(f"TEST ARC ARCADE: {game_id}")
        start = time.time()
        
        try:
            # Créer environnement
            env = ARCEnvironment(game_id)
            kernel = DecisionKernelV32Spatial()
            
            # Reset
            obs = env.reset()
            done = False
            step = 0
            total_reward = 0.0
            
            # Épisode
            while not done and step < max_steps:
                # Process observation
                cognitive_state = kernel.process_observation(obs, "noop", 0.0, step)
                
                # Decide action
                action, justification = kernel.decide_action(
                    cognitive_state,
                    env.get_available_actions()
                )
                
                # Execute
                obs, reward, done, info = env.step(action)
                total_reward += reward
                step += 1
                
                if done:
                    break
            
            # État final
            final_state = kernel.get_full_state()
            victory = total_reward > 0.9
            
            duration = (time.time() - start) * 1000
            
            return TestResult(
                test_name=f"test_arc_arcade_{game_id}",
                module="ARC_Arcade",
                passed=victory,
                duration_ms=duration,
                details={
                    "game_id": game_id,
                    "victory": victory,
                    "total_reward": total_reward,
                    "steps_taken": step,
                    "cognitive_health": final_state.get("cognitive_health", 0.0),
                    "spatial_awareness": final_state.get("spatial_awareness", 0.0),
                    "avatar_identified": final_state.get("avatar_locked", False),
                    "policy_breaks": final_state.get("policy_breaks", 0),
                    "break_frequency": final_state.get("break_frequency", 0.0)
                }
            )
            
        except Exception as e:
            duration = (time.time() - start) * 1000
            return TestResult(
                test_name=f"test_arc_arcade_{game_id}",
                module="ARC_Arcade",
                passed=False,
                duration_ms=duration,
                details={"game_id": game_id},
                error=str(e)
            )
    
    # ========================================
    # EXÉCUTION COMPLÈTE
    # ========================================
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Exécuter tous les tests"""
        self.log("=" * 80)
        self.log("DÉBUT TESTS INTÉGRATION V32")
        self.log("=" * 80)
        
        # Tests unitaires modules
        self.log("\n### PHASE 1: TESTS UNITAIRES MODULES ###\n")
        self.results.append(self.test_c1_mental_map())
        self.results.append(self.test_c2_spatial_compressor())
        self.results.append(self.test_c3_regional_strategist())
        self.results.append(self.test_c4_cognitive_stabilizer())
        self.results.append(self.test_c5_metacognitive_monitor())
        self.results.append(self.test_c6_self_spatial_identity())
        self.results.append(self.test_c7_goal_hypothesis_engine())
        self.results.append(self.test_c8_trajectory_meaning())
        self.results.append(self.test_c9_knowledge_graph())
        
        # Test intégration
        self.log("\n### PHASE 2: TEST INTÉGRATION KERNEL ###\n")
        self.results.append(self.test_decision_kernel_integration())
        
        # Tests ARC Arcade
        self.log("\n### PHASE 3: TESTS ARC ARCADE RÉELS ###\n")
        arc_games = ["ls20", "cn04", "cn05"]
        for game_id in arc_games:
            self.results.append(self.test_arc_arcade_game(game_id))
        
        # Résumé
        return self.generate_summary()
    
    def generate_summary(self) -> Dict[str, Any]:
        """Générer résumé des tests"""
        total_tests = len(self.results)
        passed_tests = sum(1 for r in self.results if r.passed)
        failed_tests = total_tests - passed_tests
        
        total_duration = sum(r.duration_ms for r in self.results)
        
        # Par module
        by_module = {}
        for result in self.results:
            if result.module not in by_module:
                by_module[result.module] = {"passed": 0, "failed": 0, "duration_ms": 0.0}
            
            if result.passed:
                by_module[result.module]["passed"] += 1
            else:
                by_module[result.module]["failed"] += 1
            
            by_module[result.module]["duration_ms"] += result.duration_ms
        
        # ARC Arcade spécifique
        arc_results = [r for r in self.results if r.module == "ARC_Arcade"]
        arc_victories = sum(1 for r in arc_results if r.passed)
        
        summary = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "total_tests": total_tests,
            "passed": passed_tests,
            "failed": failed_tests,
            "success_rate": passed_tests / total_tests if total_tests > 0 else 0.0,
            "total_duration_ms": total_duration,
            "by_module": by_module,
            "arc_arcade": {
                "total_games": len(arc_results),
                "victories": arc_victories,
                "win_rate": arc_victories / len(arc_results) if arc_results else 0.0
            },
            "all_results": [asdict(r) for r in self.results]
        }
        
        return summary


def main():
    """Point d'entrée"""
    print("\n" + "=" * 80)
    print("TEST INTÉGRATION COMPLET V32 SPATIAL COGNITIVE SYSTEM")
    print("=" * 80 + "\n")
    
    tester = V32IntegrationTester()
    summary = tester.run_all_tests()
    
    # Afficher résumé
    print("\n" + "=" * 80)
    print("RÉSUMÉ DES TESTS")
    print("=" * 80)
    print(f"\nTotal tests: {summary['total_tests']}")
    print(f"Passed: {summary['passed']} ({summary['success_rate']*100:.1f}%)")
    print(f"Failed: {summary['failed']}")
    print(f"Duration: {summary['total_duration_ms']:.1f}ms")
    
    print("\n### Par module ###")
    for module, stats in summary["by_module"].items():
        print(f"{module}: {stats['passed']}/{stats['passed']+stats['failed']} passed ({stats['duration_ms']:.1f}ms)")
    
    print("\n### ARC Arcade ###")
    print(f"Victoires: {summary['arc_arcade']['victories']}/{summary['arc_arcade']['total_games']}")
    print(f"Win rate: {summary['arc_arcade']['win_rate']*100:.1f}%")
    
    # Sauvegarder résultats
    output_file = Path(__file__).parent / "test_results_v32_integration.json"
    with open(output_file, 'w') as f:
        json.dump(summary, f, indent=2)
    
    print(f"\nRésultats sauvegardés: {output_file}")
    
    # Exit code
    sys.exit(0 if summary['failed'] == 0 else 1)


if __name__ == "__main__":
    main()

# Made with Bob
