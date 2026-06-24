#!/usr/bin/env python3
"""
Test Local MAGEN V26 GRID-AWARE
================================

Test l'agent V26 avec GridAnalyzer sur jeux ARC-AGI-3 en mode LOCAL
AVANT toute soumission publique.

Objectif: Valider que score passe de 0/3 à >0/3

Protocole: CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN v2.0
- Test local uniquement
- Logs forensiques complets
- Validation avant soumission

Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 71
"""

import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent))

# Import agent V26
from magen_arc_agi_agent_v26_grid_aware import MAGENArcAgiAgentV26GridAware

# Import modules ARC-AGI (si disponibles)
try:
    import arc_agi
    from arcengine import GameAction, GameState
    ARC_AVAILABLE = True
except ImportError:
    print("[WARNING] arc_agi non disponible, mode simulation")
    ARC_AVAILABLE = False
    
    # Mock classes pour tests sans arc_agi
    class GameAction:
        ACTION1 = "ACTION1"
        ACTION2 = "ACTION2"
        ACTION3 = "ACTION3"
        ACTION4 = "ACTION4"
        ACTION5 = "ACTION5"
        ACTION6 = "ACTION6"
        ACTION7 = "ACTION7"
        RESET = "RESET"
    
    class GameState:
        pass


class V26LocalTester:
    """
    Testeur local pour MAGEN V26 GRID-AWARE
    
    Tests:
    1. Initialisation agent
    2. Perception grille (GridAnalyzer)
    3. Décision grid-aware
    4. Apprentissage avec feedback spatial
    5. Statistiques complètes
    """
    
    def __init__(self):
        self.session_id = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.results_dir = Path(__file__).parent / 'test_results_v26'
        self.results_dir.mkdir(exist_ok=True)
        
        # Forensic log path
        self.forensic_log_path = self.results_dir / f'forensic_v26_{self.session_id}.jsonl'
        
        # Agent V26
        self.agent = MAGENArcAgiAgentV26GridAware(
            forensic_log_path=str(self.forensic_log_path)
        )
        
        # Résultats
        self.results = {
            'session_id': self.session_id,
            'version': 'V26_GRID_AWARE',
            'tests': [],
            'summary': {},
            'start_time': datetime.now().isoformat(),
            'end_time': None
        }
        
        print("=" * 80)
        print("🧪 TEST LOCAL MAGEN V26 GRID-AWARE")
        print("=" * 80)
        print(f"Session ID: {self.session_id}")
        print(f"Results dir: {self.results_dir}")
        print(f"Forensic log: {self.forensic_log_path}")
        print(f"ARC-AGI disponible: {ARC_AVAILABLE}")
        print("=" * 80)
    
    def test_initialization(self) -> Dict:
        """Test 1: Initialisation agent"""
        print("\n[TEST 1] Initialisation agent V26...")
        
        test_result = {
            'test_name': 'initialization',
            'success': False,
            'details': {}
        }
        
        try:
            # Vérifier modules chargés
            assert self.agent.grid_analyzer is not None, "GridAnalyzer non initialisé"
            assert self.agent.grid_aware_enabled, "Grid-aware non activé"
            assert self.agent.state_encoder is not None, "StateEncoder non initialisé"
            
            test_result['success'] = True
            test_result['details'] = {
                'grid_analyzer': 'OK',
                'grid_aware_enabled': True,
                'state_encoder': 'OK',
                'forensic_enabled': self.agent.forensic_enabled
            }
            
            print("✅ Initialisation réussie")
            
        except Exception as e:
            test_result['details']['error'] = str(e)
            print(f"❌ Erreur initialisation: {e}")
        
        self.results['tests'].append(test_result)
        return test_result
    
    def test_grid_perception(self) -> Dict:
        """Test 2: Perception grille avec GridAnalyzer"""
        print("\n[TEST 2] Perception grille + analyse spatiale...")
        
        test_result = {
            'test_name': 'grid_perception',
            'success': False,
            'details': {}
        }
        
        try:
            # Créer grille test simple
            # Joueur (1) en (5,5), Objectif (4) en (8,8), Obstacles (7) en ligne
            grid = np.zeros((10, 10), dtype=np.uint8)
            grid[5, 5] = 1  # Joueur
            grid[8, 8] = 4  # Objectif
            grid[6, :] = 7  # Ligne obstacles
            grid[6, 5] = 0  # Passage
            
            # Mock observation
            class MockObs:
                def __init__(self, frame):
                    self.frame = frame
                    self.levels_completed = 0
            
            obs_mock = MockObs(grid)
            
            # Percevoir
            obs = self.agent.perceive(obs_mock)
            
            # Vérifier perception
            assert obs.grid is not None, "Grille non extraite"
            assert obs.embedding is not None, "Embedding non généré"
            assert obs.spatial_analysis is not None, "Analyse spatiale non effectuée"
            
            # Vérifier détection entités
            spatial = obs.spatial_analysis
            assert spatial.player_position is not None, "Joueur non détecté"
            assert spatial.goal_position is not None, "Objectif non détecté"
            assert len(spatial.obstacles) > 0, "Obstacles non détectés"
            
            # Vérifier pathfinding
            assert spatial.path_to_goal is not None, "Chemin non trouvé"
            assert spatial.distance_to_goal < float('inf'), "Distance invalide"
            
            test_result['success'] = True
            test_result['details'] = {
                'grid_shape': list(obs.grid.shape),
                'player_position': spatial.player_position,
                'goal_position': spatial.goal_position,
                'obstacles_count': len(spatial.obstacles),
                'path_found': spatial.path_to_goal is not None,
                'path_length': len(spatial.path_to_goal) if spatial.path_to_goal else 0,
                'distance_to_goal': float(spatial.distance_to_goal)
            }
            
            print("✅ Perception réussie")
            print(f"   Joueur: {spatial.player_position}")
            print(f"   Objectif: {spatial.goal_position}")
            print(f"   Obstacles: {len(spatial.obstacles)}")
            print(f"   Chemin: {len(spatial.path_to_goal) if spatial.path_to_goal else 0} steps")
            print(f"   Distance: {spatial.distance_to_goal:.1f}")
            
        except Exception as e:
            test_result['details']['error'] = str(e)
            print(f"❌ Erreur perception: {e}")
            import traceback
            traceback.print_exc()
        
        self.results['tests'].append(test_result)
        return test_result
    
    def test_grid_aware_decision(self) -> Dict:
        """Test 3: Décision grid-aware"""
        print("\n[TEST 3] Décision grid-aware...")
        
        test_result = {
            'test_name': 'grid_aware_decision',
            'success': False,
            'details': {}
        }
        
        try:
            # Créer grille test
            grid = np.zeros((10, 10), dtype=np.uint8)
            grid[5, 5] = 1  # Joueur
            grid[8, 8] = 4  # Objectif
            grid[6, :] = 7  # Obstacles
            grid[6, 5] = 0  # Passage
            
            class MockObs:
                def __init__(self, frame):
                    self.frame = frame
                    self.levels_completed = 0
            
            obs_mock = MockObs(grid)
            obs = self.agent.perceive(obs_mock)
            
            # Actions disponibles
            available_actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4', 'ACTION5']
            
            # Décider
            decision = self.agent.decide(obs, available_actions)
            
            # Vérifier décision
            assert decision is not None, "Décision non générée"
            assert decision.action_name in available_actions, "Action invalide"
            assert decision.strategy in ['grid_aware', 'exploration'], "Stratégie invalide"
            
            test_result['success'] = True
            test_result['details'] = {
                'action': decision.action_name,
                'semantic_action': decision.semantic_name,
                'strategy': decision.strategy,
                'score': float(decision.score),
                'justification': decision.justification,
                'grid_suggestion_used': decision.grid_suggestion is not None
            }
            
            print("✅ Décision réussie")
            print(f"   Action: {decision.action_name} ({decision.semantic_name})")
            print(f"   Stratégie: {decision.strategy}")
            print(f"   Score: {decision.score:.3f}")
            print(f"   Justification: {decision.justification}")
            
        except Exception as e:
            test_result['details']['error'] = str(e)
            print(f"❌ Erreur décision: {e}")
            import traceback
            traceback.print_exc()
        
        self.results['tests'].append(test_result)
        return test_result
    
    def test_learning_with_spatial_feedback(self) -> Dict:
        """Test 4: Apprentissage avec feedback spatial"""
        print("\n[TEST 4] Apprentissage avec feedback spatial...")
        
        test_result = {
            'test_name': 'learning_spatial_feedback',
            'success': False,
            'details': {}
        }
        
        try:
            # Créer deux grilles (avant/après action)
            grid_before = np.zeros((10, 10), dtype=np.uint8)
            grid_before[5, 5] = 1  # Joueur
            grid_before[8, 8] = 4  # Objectif
            
            grid_after = np.zeros((10, 10), dtype=np.uint8)
            grid_after[6, 5] = 1  # Joueur déplacé (progression)
            grid_after[8, 8] = 4  # Objectif
            
            class MockObs:
                def __init__(self, frame):
                    self.frame = frame
                    self.levels_completed = 0
            
            obs_before = self.agent.perceive(MockObs(grid_before))
            obs_after = self.agent.perceive(MockObs(grid_after))
            
            # Apprendre
            action = 'ACTION2'  # move_down
            reward = 0.0  # Reward environnement neutre
            done = False
            
            self.agent.learn(obs_before, action, obs_after, reward, done)
            
            # Vérifier apprentissage
            assert len(self.agent.recent_actions) > 0, "Historique non mis à jour"
            assert self.agent.stats['learning_updates'] > 0, "Stats non mises à jour"
            
            # Vérifier reward intrinsèque (doit être positif car progression)
            intrinsic_reward = self.agent.stats['intrinsic_rewards_total']
            assert intrinsic_reward > 0, "Reward intrinsèque non calculé"
            
            test_result['success'] = True
            test_result['details'] = {
                'action': action,
                'env_reward': float(reward),
                'intrinsic_reward': float(intrinsic_reward),
                'learning_updates': self.agent.stats['learning_updates'],
                'graph_nodes': len(self.agent.causal_graph.nodes),
                'distance_before': float(obs_before.spatial_analysis.distance_to_goal),
                'distance_after': float(obs_after.spatial_analysis.distance_to_goal),
                'progression': float(obs_before.spatial_analysis.distance_to_goal - obs_after.spatial_analysis.distance_to_goal)
            }
            
            print("✅ Apprentissage réussi")
            print(f"   Reward environnement: {reward}")
            print(f"   Reward intrinsèque: {intrinsic_reward:.3f}")
            print(f"   Progression: {test_result['details']['progression']:.1f}")
            
        except Exception as e:
            test_result['details']['error'] = str(e)
            print(f"❌ Erreur apprentissage: {e}")
            import traceback
            traceback.print_exc()
        
        self.results['tests'].append(test_result)
        return test_result
    
    def test_statistics(self) -> Dict:
        """Test 5: Statistiques complètes"""
        print("\n[TEST 5] Statistiques complètes...")
        
        test_result = {
            'test_name': 'statistics',
            'success': False,
            'details': {}
        }
        
        try:
            stats = self.agent.get_stats()
            
            # Vérifier stats V26
            assert 'grids_analyzed' in stats, "Stats grids_analyzed manquante"
            assert 'players_detected' in stats, "Stats players_detected manquante"
            assert 'goals_detected' in stats, "Stats goals_detected manquante"
            assert 'paths_found' in stats, "Stats paths_found manquante"
            assert 'grid_aware_actions' in stats, "Stats grid_aware_actions manquante"
            assert 'grid_analyzer_stats' in stats, "Stats GridAnalyzer manquantes"
            
            test_result['success'] = True
            test_result['details'] = {
                'grids_analyzed': stats['grids_analyzed'],
                'players_detected': stats['players_detected'],
                'goals_detected': stats['goals_detected'],
                'paths_found': stats['paths_found'],
                'grid_aware_actions': stats['grid_aware_actions'],
                'grid_suggestions_used': stats['grid_suggestions_used'],
                'total_actions': stats['total_actions'],
                'learning_updates': stats['learning_updates'],
                'forensic_logs': stats['forensic_logs_generated']
            }
            
            print("✅ Statistiques OK")
            print(f"   Grilles analysées: {stats['grids_analyzed']}")
            print(f"   Joueurs détectés: {stats['players_detected']}")
            print(f"   Objectifs détectés: {stats['goals_detected']}")
            print(f"   Chemins trouvés: {stats['paths_found']}")
            print(f"   Actions grid-aware: {stats['grid_aware_actions']}")
            print(f"   Logs forensiques: {stats['forensic_logs_generated']}")
            
        except Exception as e:
            test_result['details']['error'] = str(e)
            print(f"❌ Erreur statistiques: {e}")
            import traceback
            traceback.print_exc()
        
        self.results['tests'].append(test_result)
        return test_result
    
    def run_all_tests(self):
        """Exécuter tous les tests"""
        print("\n" + "=" * 80)
        print("EXÉCUTION TESTS V26 GRID-AWARE")
        print("=" * 80)
        
        # Tests
        self.test_initialization()
        self.test_grid_perception()
        self.test_grid_aware_decision()
        self.test_learning_with_spatial_feedback()
        self.test_statistics()
        
        # Résumé
        total_tests = len(self.results['tests'])
        passed_tests = sum(1 for t in self.results['tests'] if t['success'])
        
        self.results['summary'] = {
            'total_tests': total_tests,
            'passed_tests': passed_tests,
            'failed_tests': total_tests - passed_tests,
            'success_rate': passed_tests / total_tests if total_tests > 0 else 0.0
        }
        
        self.results['end_time'] = datetime.now().isoformat()
        
        # Sauvegarder résultats
        results_file = self.results_dir / f'test_results_v26_{self.session_id}.json'
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        # Afficher résumé
        print("\n" + "=" * 80)
        print("RÉSUMÉ TESTS V26 GRID-AWARE")
        print("=" * 80)
        print(f"Tests exécutés: {total_tests}")
        print(f"Tests réussis: {passed_tests}")
        print(f"Tests échoués: {total_tests - passed_tests}")
        print(f"Taux de réussite: {self.results['summary']['success_rate']*100:.1f}%")
        print(f"\nRésultats sauvegardés: {results_file}")
        print(f"Logs forensiques: {self.forensic_log_path}")
        print("=" * 80)
        
        # Verdict
        if passed_tests == total_tests:
            print("\n✅ TOUS LES TESTS PASSÉS - V26 PRÊT POUR VALIDATION")
        else:
            print(f"\n⚠️  {total_tests - passed_tests} TEST(S) ÉCHOUÉ(S) - CORRECTIONS NÉCESSAIRES")
        
        return self.results


def main():
    """Point d'entrée principal"""
    tester = V26LocalTester()
    results = tester.run_all_tests()
    
    # Exit code basé sur résultats
    if results['summary']['success_rate'] == 1.0:
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == '__main__':
    main()

# Made with Bob
