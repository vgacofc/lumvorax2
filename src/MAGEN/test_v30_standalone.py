"""
Test MAGEN V30 (TVT) Standalone - Sans dépendances externes
============================================================

Test simplifié V30 vs V28 sur environnements synthétiques.

Date : 2026-06-15
"""

import sys
import os
import json
import time
from datetime import datetime
from typing import Dict, List, Tuple
import numpy as np

# Ajouter path MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'core'))

from decision_kernel_v30_tvt import DecisionKernelV30TVT
from decision_kernel_minimal import DecisionKernelMinimal


def create_test_environments() -> List[Dict]:
    """Crée 7 environnements synthétiques de test."""
    return [
        {
            'name': 'simple_direct',
            'grid_size': (10, 10),
            'agent_pos': (0, 0),
            'goal_pos': (9, 9),
            'obstacles': []
        },
        {
            'name': 'with_obstacles',
            'grid_size': (10, 10),
            'agent_pos': (0, 0),
            'goal_pos': (9, 9),
            'obstacles': [(5, i) for i in range(10)]  # Mur vertical
        },
        {
            'name': 'maze_simple',
            'grid_size': (12, 12),
            'agent_pos': (1, 1),
            'goal_pos': (10, 10),
            'obstacles': [
                (3, i) for i in range(8)
            ] + [
                (6, i) for i in range(4, 12)
            ] + [
                (9, i) for i in range(8)
            ]
        },
        {
            'name': 'long_corridor',
            'grid_size': (20, 5),
            'agent_pos': (0, 2),
            'goal_pos': (19, 2),
            'obstacles': []
        },
        {
            'name': 'u_shape',
            'grid_size': (10, 10),
            'agent_pos': (0, 0),
            'goal_pos': (0, 9),
            'obstacles': [(1, i) for i in range(1, 9)]
        },
        {
            'name': 'multi_goal',
            'grid_size': (12, 12),
            'agent_pos': (6, 6),
            'goal_positions': [(2, 2), (10, 2), (2, 10), (10, 10)],
            'obstacles': []
        },
        {
            'name': 'complex_maze',
            'grid_size': (15, 15),
            'agent_pos': (1, 1),
            'goal_pos': (13, 13),
            'obstacles': [
                (3, i) for i in range(10)
            ] + [
                (6, i) for i in range(5, 15)
            ] + [
                (9, i) for i in range(10)
            ] + [
                (12, i) for i in range(5, 15)
            ]
        }
    ]


def simulate_step(
    state: Dict,
    action: str,
    env: Dict
) -> Tuple[Dict, float, bool]:
    """Simule step dans environnement."""
    agent_pos = state['agent_pos']
    obstacles = state.get('obstacles', [])
    grid_size = state['grid_size']
    
    # Prédire nouvelle position
    x, y = agent_pos
    if action == 'ACTION1':  # up
        y = max(0, y - 1)
    elif action == 'ACTION2':  # down
        y = min(grid_size[1] - 1, y + 1)
    elif action == 'ACTION3':  # left
        x = max(0, x - 1)
    elif action == 'ACTION4':  # right
        x = min(grid_size[0] - 1, x + 1)
    
    new_pos = (x, y)
    
    # Collision obstacle
    if new_pos in obstacles:
        new_pos = agent_pos
    
    # Nouveau state
    next_state = state.copy()
    next_state['agent_pos'] = new_pos
    
    # Reward et done
    reward = 0.0
    done = False
    
    # Multi-goal
    if 'goal_positions' in state:
        goals_reached = set(state.get('goals_reached', []))
        
        if new_pos in state['goal_positions'] and new_pos not in goals_reached:
            reward = 1.0
            goals_reached.add(new_pos)
            next_state['goals_reached'] = list(goals_reached)
            
            # Tous goals atteints
            if len(goals_reached) == len(state['goal_positions']):
                done = True
    else:
        # Single goal
        if new_pos == state['goal_pos']:
            reward = 1.0
            done = True
    
    return next_state, reward, done


def run_single_test(
    env: Dict,
    kernel_type: str,
    max_steps: int = 100
) -> Dict:
    """
    Exécute test sur un environnement.
    
    Args:
        env: Configuration environnement
        kernel_type: 'v28' ou 'v30'
        max_steps: Steps maximum
    
    Returns:
        Résultats test
    """
    # Créer kernel
    if kernel_type == 'v30':
        kernel = DecisionKernelV30TVT(
            learning_rate=0.01,
            exploration_rate=0.2,
            temperature=1.0,
            stagnation_threshold=10,
            collapse_variance_threshold=0.02,
            enable_policy_breaking=True
        )
    else:  # v28
        kernel = DecisionKernelMinimal(
            learning_rate=0.01,
            exploration_rate=0.2,
            temperature=1.0
        )
    
    # État initial
    state = {
        'grid_size': env['grid_size'],
        'agent_pos': env['agent_pos'],
        'obstacles': env.get('obstacles', [])
    }
    
    # Multi-goal ou single goal
    if 'goal_positions' in env:
        state['goal_positions'] = env['goal_positions']
        state['goals_reached'] = []
    else:
        state['goal_pos'] = env['goal_pos']
    
    # Actions disponibles
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    
    # Exécution
    trajectory = []
    success = False
    total_reward = 0.0
    
    for step in range(max_steps):
        # Sélection action
        action, decision_info = kernel.select_action(
            state, actions
        )
        
        # Simuler step
        next_state, reward, done = simulate_step(state, action, env)
        
        # Update kernel
        if kernel_type == 'v30':
            kernel.update_after_step(state, action, next_state, reward, done)
        else:
            kernel.update_weights(reward)
        
        # Log trajectory
        trajectory.append({
            'step': step,
            'action': action,
            'reward': reward,
            'scores': decision_info.get('scores', {}),
            'policy_broken': decision_info.get('policy_broken', False)
        })
        
        total_reward += reward
        state = next_state
        
        if done:
            success = True
            break
    
    # Métriques finales
    if kernel_type == 'v30':
        metrics = kernel.get_extended_metrics()
    else:
        metrics = kernel.get_stats()
    
    return {
        'success': success,
        'steps': len(trajectory),
        'total_reward': total_reward,
        'metrics': metrics,
        'trajectory_sample': trajectory[:5]  # Premiers 5 steps seulement
    }


def run_comparison_test():
    """Exécute test comparatif V28 vs V30."""
    print("=" * 80)
    print("TEST COMPARATIF MAGEN V28.2.2 vs V30.0.0 (TVT)")
    print("=" * 80)
    print()
    
    # Environnements
    environments = create_test_environments()
    
    # Résultats
    results = {
        'v28': [],
        'v30': [],
        'timestamp': datetime.now().isoformat()
    }
    
    # Test chaque environnement
    for env in environments:
        print(f"\n{'='*80}")
        print(f"Environnement : {env['name']}")
        print(f"{'='*80}")
        
        # Test V28
        print("\n[V28.2.2] Exécution...")
        start = time.time()
        result_v28 = run_single_test(env, 'v28', max_steps=100)
        duration_v28 = time.time() - start
        result_v28['duration'] = duration_v28
        result_v28['env_name'] = env['name']
        results['v28'].append(result_v28)
        
        print(f"  Succès : {result_v28['success']}")
        print(f"  Steps : {result_v28['steps']}")
        print(f"  Reward : {result_v28['total_reward']:.2f}")
        print(f"  Durée : {duration_v28:.2f}s")
        
        # Test V30
        print("\n[V30.0.0] Exécution...")
        start = time.time()
        result_v30 = run_single_test(env, 'v30', max_steps=100)
        duration_v30 = time.time() - start
        result_v30['duration'] = duration_v30
        result_v30['env_name'] = env['name']
        results['v30'].append(result_v30)
        
        print(f"  Succès : {result_v30['success']}")
        print(f"  Steps : {result_v30['steps']}")
        print(f"  Reward : {result_v30['total_reward']:.2f}")
        print(f"  Durée : {duration_v30:.2f}s")
        print(f"  Policy breaks : {result_v30['metrics'].get('total_policy_breaks', 0)}")
        
        # Comparaison
        print("\n[COMPARAISON]")
        if result_v30['success'] and not result_v28['success']:
            print("  ✓ V30 réussit là où V28 échoue")
        elif result_v30['success'] and result_v28['success']:
            if result_v30['steps'] < result_v28['steps']:
                print(f"  ✓ V30 plus efficace ({result_v28['steps'] - result_v30['steps']} steps économisés)")
            else:
                print(f"  = Performances similaires")
        elif not result_v30['success'] and not result_v28['success']:
            print("  = Échec pour les deux versions")
        else:
            print("  ✗ V28 meilleur que V30 (régression)")
    
    # Statistiques globales
    print(f"\n{'='*80}")
    print("STATISTIQUES GLOBALES")
    print(f"{'='*80}")
    
    success_v28 = sum(1 for r in results['v28'] if r['success'])
    success_v30 = sum(1 for r in results['v30'] if r['success'])
    
    print(f"\nTaux succès :")
    print(f"  V28.2.2 : {success_v28}/{len(environments)} ({100*success_v28/len(environments):.1f}%)")
    print(f"  V30.0.0 : {success_v30}/{len(environments)} ({100*success_v30/len(environments):.1f}%)")
    
    if success_v28 > 0:
        avg_steps_v28 = sum(r['steps'] for r in results['v28'] if r['success']) / success_v28
        print(f"\nSteps moyens V28 (succès) : {avg_steps_v28:.1f}")
    
    if success_v30 > 0:
        avg_steps_v30 = sum(r['steps'] for r in results['v30'] if r['success']) / success_v30
        print(f"Steps moyens V30 (succès) : {avg_steps_v30:.1f}")
    
    total_policy_breaks = sum(
        r['metrics'].get('total_policy_breaks', 0)
        for r in results['v30']
    )
    print(f"\nPolicy breaks V30 : {total_policy_breaks}")
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"test_v30_results_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nRésultats sauvegardés : {output_file}")
    
    return results


if __name__ == '__main__':
    results = run_comparison_test()

# Made with Bob
