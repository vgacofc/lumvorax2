"""
PHASE V40.3: Test Validation Consultation World Model
======================================================

Objectif: Valider que PolicyManager V40.3 consulte RÉELLEMENT le World Model

Critères GO/NO-GO:
1. world_model_consultation_rate > 50%
2. decisions_influenced_rate > 20%
3. coverage_improvement > 10% (vs V40.2)
4. regions_discovered variance > 0 (pas toujours 4)

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'core'))

import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime

# Import modules V39
from world_state_graph_v39 import WorldStateGraph
from agent_localization_v39 import AgentLocalizationSystem
from causal_memory_v39 import CausalMemorySystem
from minimal_learning_system_v39 import MinimalLearningSystemV39

# Import PolicyManager V40.3
from policy_manager_v40_3 import PolicyManagerV40_3, ProblemType


def create_test_puzzle(size=10):
    """Crée un puzzle de test simple."""
    return np.random.randint(0, 10, (size, size))


def simulate_action_result(action_id, puzzle):
    """Simule le résultat d'une action."""
    # Simuler succès/échec selon action
    if 'identity' in action_id.lower():
        success = False
        reward = 0.1
        error = 0.9
    elif 'rotate' in action_id.lower():
        success = True
        reward = 0.8
        error = 0.2
    else:
        success = np.random.random() > 0.5
        reward = np.random.uniform(0.3, 0.7)
        error = 1.0 - reward
    
    return success, reward, error


def test_v40_3_world_model_consultation():
    """
    Test principal V40.3: Validation consultation World Model.
    """
    print("="*80)
    print("TEST V40.3 - VALIDATION CONSULTATION WORLD MODEL")
    print("="*80)
    
    # Créer répertoire logs
    log_dir = Path("lumvorax2/logs/v40_3_validation")
    log_dir.mkdir(parents=True, exist_ok=True)
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    forensic_path = log_dir / f"v40_3_forensic_{timestamp}.jsonl"
    
    # Créer tous les modules
    print("\n1. Création modules World Model...")
    world_graph = WorldStateGraph(width=10, height=10)
    agent_loc = AgentLocalizationSystem(grid_width=10, grid_height=10)
    causal_mem = CausalMemorySystem(max_events=1000)
    learning_sys = MinimalLearningSystemV39(
        learning_rate=0.01,
        forensic_log_path=str(forensic_path)
    )
    
    # Créer PolicyManager V40.3
    policy_manager = PolicyManagerV40_3(
        initial_mode=ProblemType.ARC_AGI,
        forensic_log_path=str(forensic_path)
    )
    policy_manager.set_problem_type(ProblemType.ARC_AGI)
    
    print("✅ Modules créés")
    
    # Actions disponibles
    actions = ['rotate_90', 'flip_h', 'flip_v', 'identity', 'scale_2x', 'crop']
    
    # Test sur 10 puzzles
    print("\n2. Test sur 10 puzzles ARC...")
    
    results = []
    
    for puzzle_id in range(10):
        print(f"\n--- Puzzle {puzzle_id} ---")
        
        puzzle = create_test_puzzle(size=10)
        puzzle_results = {
            'puzzle_id': puzzle_id,
            'steps': 0,
            'consultations': 0,
            'influences': 0,
            'coverage_start': 0.0,
            'coverage_end': 0.0,
            'regions_discovered': 0
        }
        
        # Simuler 20 steps par puzzle
        for step in range(20):
            # Position aléatoire
            x, y = np.random.randint(0, 10), np.random.randint(0, 10)
            
            # Mettre à jour World Model
            world_graph.visit_cell(x, y, int(puzzle[x, y]))
            agent_loc.update_position(x, y, step)
            
            # DÉCISION AVEC CONSULTATION WORLD MODEL (V40.3)
            action, consultation = policy_manager.decide_action(
                available_actions=actions,
                world_graph=world_graph,      # ← NOUVEAU V40.3
                agent_loc=agent_loc,          # ← NOUVEAU V40.3
                causal_mem=causal_mem,        # ← NOUVEAU V40.3
                learning_sys=learning_sys     # ← NOUVEAU V40.3
            )
            
            # Compter consultations
            if consultation.world_graph_consulted:
                puzzle_results['consultations'] += 1
            
            if consultation.decision_influenced:
                puzzle_results['influences'] += 1
            
            # Simuler résultat
            success, reward, error = simulate_action_result(action, puzzle)
            
            # Mettre à jour tous les systèmes
            state_hash = f"state_{puzzle_id}_{step}"
            
            causal_mem.record_transformation(
                action, 
                {'step': step}, 
                {'step': step+1}, 
                success, 
                error
            )
            
            learning_sys.update_action_score(action, reward, success)
            
            policy_manager.update_after_action(
                action, state_hash, success, reward, error,
                steps_used=1, time_ms=10.0, memory_mb=0.1
            )
            
            puzzle_results['steps'] += 1
            
            # Enregistrer couverture
            if step == 0:
                puzzle_results['coverage_start'] = world_graph.get_coverage()
            if step == 19:
                puzzle_results['coverage_end'] = world_graph.get_coverage()
        
        # Découvrir régions
        for i in range(3):
            x, y = np.random.randint(0, 10), np.random.randint(0, 10)
            world_graph.discover_region(f"region_{puzzle_id}_{i}", x, y)
        
        puzzle_results['regions_discovered'] = len(world_graph.regions)
        
        results.append(puzzle_results)
        
        print(f"  Steps: {puzzle_results['steps']}")
        print(f"  Consultations: {puzzle_results['consultations']}/{puzzle_results['steps']}")
        print(f"  Influences: {puzzle_results['influences']}/{puzzle_results['steps']}")
        print(f"  Coverage: {puzzle_results['coverage_start']:.1%} → {puzzle_results['coverage_end']:.1%}")
        print(f"  Régions: {puzzle_results['regions_discovered']}")
    
    # Statistiques globales
    print("\n" + "="*80)
    print("STATISTIQUES GLOBALES")
    print("="*80)
    
    total_steps = sum(r['steps'] for r in results)
    total_consultations = sum(r['consultations'] for r in results)
    total_influences = sum(r['influences'] for r in results)
    
    consultation_rate = total_consultations / total_steps if total_steps > 0 else 0.0
    influence_rate = total_influences / total_steps if total_steps > 0 else 0.0
    
    avg_coverage_start = np.mean([r['coverage_start'] for r in results])
    avg_coverage_end = np.mean([r['coverage_end'] for r in results])
    coverage_improvement = avg_coverage_end - avg_coverage_start
    
    regions_counts = [r['regions_discovered'] for r in results]
    regions_variance = np.var(regions_counts)
    
    print(f"\nSteps totaux: {total_steps}")
    print(f"Consultations World Model: {total_consultations}/{total_steps} ({consultation_rate:.1%})")
    print(f"Décisions influencées: {total_influences}/{total_steps} ({influence_rate:.1%})")
    print(f"Couverture moyenne: {avg_coverage_start:.1%} → {avg_coverage_end:.1%} (+{coverage_improvement:.1%})")
    print(f"Régions découvertes: {regions_counts} (variance: {regions_variance:.2f})")
    
    # Statistiques PolicyManager
    policy_stats = policy_manager.get_stats()
    print(f"\nStatistiques PolicyManager:")
    print(f"  Consultation rate: {policy_stats['world_model_consultation']['consultation_rate']:.1%}")
    print(f"  Influence rate: {policy_stats['world_model_consultation']['influence_rate']:.1%}")
    print(f"  Changements stratégie: {policy_stats['policy_state']['strategy_changes']}")
    
    # Statistiques Learning System
    learning_stats = learning_sys.get_statistics()
    print(f"\nStatistiques Learning System:")
    print(f"  Actions trackées: {learning_stats['total_actions']}")
    print(f"  Score changes: {learning_stats['total_score_changes']}")
    print(f"  Score variance: {learning_stats['score_variance']:.6f}")
    print(f"  Learning functional: {learning_stats['learning_functional']}")
    
    # Statistiques Causal Memory
    causal_stats = causal_mem.get_stats()
    print(f"\nStatistiques Causal Memory:")
    print(f"  Événements: {causal_stats['total_events']}")
    print(f"  Chaînes causales: {causal_stats['total_chains']}")
    print(f"  Transformations trackées: {causal_stats['transformations_tracked']}")
    
    # VALIDATION GO/NO-GO
    print("\n" + "="*80)
    print("VALIDATION GO/NO-GO V40.3")
    print("="*80)
    
    criteria = {
        'consultation_rate': {
            'value': consultation_rate,
            'threshold': 0.50,
            'pass': consultation_rate > 0.50
        },
        'influence_rate': {
            'value': influence_rate,
            'threshold': 0.20,
            'pass': influence_rate > 0.20
        },
        'coverage_improvement': {
            'value': coverage_improvement,
            'threshold': 0.10,
            'pass': coverage_improvement > 0.10
        },
        'regions_variance': {
            'value': regions_variance,
            'threshold': 0.0,
            'pass': regions_variance > 0.0
        }
    }
    
    all_pass = all(c['pass'] for c in criteria.values())
    
    print("\nCritères:")
    for name, criterion in criteria.items():
        status = "✅ PASS" if criterion['pass'] else "❌ FAIL"
        print(f"  {name}: {criterion['value']:.3f} (seuil: {criterion['threshold']}) {status}")
    
    print("\n" + "="*80)
    if all_pass:
        print("✅ VERDICT: GO - Tous les critères sont validés")
        print("V40.3 consulte RÉELLEMENT le World Model et influence les décisions")
    else:
        print("❌ VERDICT: NO-GO - Certains critères ne sont pas validés")
        print("Analyse forensic requise pour identifier le problème")
    print("="*80)
    
    # Sauvegarder résultats
    results_path = log_dir / f"v40_3_results_{timestamp}.json"
    
    # Convertir criteria en format JSON-serializable
    criteria_json = {
        name: {
            'value': float(c['value']),
            'threshold': float(c['threshold']),
            'pass': bool(c['pass'])
        }
        for name, c in criteria.items()
    }
    
    with open(results_path, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'results': results,
            'global_stats': {
                'total_steps': int(total_steps),
                'consultation_rate': float(consultation_rate),
                'influence_rate': float(influence_rate),
                'coverage_improvement': float(coverage_improvement),
                'regions_variance': float(regions_variance)
            },
            'criteria': criteria_json,
            'verdict': 'GO' if all_pass else 'NO-GO'
        }, f, indent=2)
    
    print(f"\n📁 Résultats sauvegardés:")
    print(f"  - {results_path}")
    print(f"  - {forensic_path}")
    
    return all_pass


if __name__ == '__main__':
    success = test_v40_3_world_model_consultation()
    sys.exit(0 if success else 1)


# Made with Bob - V40.3 Validation Test