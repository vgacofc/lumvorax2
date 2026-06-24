"""
Test Robustesse Multi-Environnements - MAGEN V28.1
Validation sur 10 configurations différentes (grids 3×3 à 10×10)

Objectif: Mesurer fragilités identifiées par feedback critique:
1. Spatial dominance collapse (grids larges)
2. World model lag (environnements complexes)
3. Reward sparsity (goals éloignés)

Protocole MAGEN:
- Exécution 10 environnements
- Logs forensic par environnement
- Métriques comparatives
- Identification fragilités empiriques

Date: 2026-06-15
"""

import sys
import os
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple
import numpy as np

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

from decision_kernel_minimal import DecisionKernelMinimal
from world_model_minimal import WorldModelMinimal


class ConfigurableGridEnv:
    """Environnement grid configurable pour tests robustesse."""
    
    def __init__(self, grid_size: Tuple[int, int], agent_pos: Tuple[int, int], goal_pos: Tuple[int, int]):
        """
        Args:
            grid_size: (width, height)
            agent_pos: Position initiale agent
            goal_pos: Position goal
        """
        self.grid_size = grid_size
        self.initial_agent_pos = agent_pos
        self.initial_goal_pos = goal_pos
        self.agent_pos = agent_pos
        self.goal_pos = goal_pos
        self.step_count = 0
        self.max_steps = max(50, grid_size[0] * grid_size[1])  # Adaptatif
        
    def reset(self):
        """Réinitialise environnement."""
        self.agent_pos = self.initial_agent_pos
        self.step_count = 0
        return self.get_state()
    
    def get_state(self):
        """Retourne état actuel."""
        grid = np.zeros(self.grid_size, dtype=int)
        grid[self.agent_pos[1], self.agent_pos[0]] = 1  # Agent
        grid[self.goal_pos[1], self.goal_pos[0]] = 2    # Goal
        
        return {
            'grid': grid,
            'agent_pos': self.agent_pos,
            'goal_pos': self.goal_pos,
            'step': self.step_count,
            'grid_size': self.grid_size
        }
    
    def step(self, action):
        """Exécute action et retourne (next_state, reward, done)."""
        old_pos = self.agent_pos
        
        # Appliquer action
        x, y = self.agent_pos
        if action == 'ACTION1' or action == 'move_up':
            y = max(0, y - 1)
        elif action == 'ACTION2' or action == 'move_down':
            y = min(self.grid_size[1] - 1, y + 1)
        elif action == 'ACTION3' or action == 'move_left':
            x = max(0, x - 1)
        elif action == 'ACTION4' or action == 'move_right':
            x = min(self.grid_size[0] - 1, x + 1)
        
        self.agent_pos = (x, y)
        self.step_count += 1
        
        # Calculer reward (distance Manhattan)
        dist_before = abs(old_pos[0] - self.goal_pos[0]) + abs(old_pos[1] - self.goal_pos[1])
        dist_after = abs(self.agent_pos[0] - self.goal_pos[0]) + abs(self.agent_pos[1] - self.goal_pos[1])
        
        reward = dist_before - dist_after
        
        # Bonus si atteint goal
        if self.agent_pos == self.goal_pos:
            reward += 10.0
        
        # Done si goal atteint ou max steps
        done = (self.agent_pos == self.goal_pos) or (self.step_count >= self.max_steps)
        
        return self.get_state(), reward, done


def create_test_environments() -> List[Dict]:
    """
    Crée 10 environnements de test avec difficultés croissantes.
    
    Returns:
        Liste de configs: {name, grid_size, agent_pos, goal_pos, difficulty}
    """
    environments = [
        # Facile (3×3 à 5×5)
        {
            'name': 'ENV1_3x3_corner',
            'grid_size': (3, 3),
            'agent_pos': (0, 0),
            'goal_pos': (2, 2),
            'difficulty': 'easy',
            'expected_steps': 4
        },
        {
            'name': 'ENV2_5x5_center',
            'grid_size': (5, 5),
            'agent_pos': (2, 2),
            'goal_pos': (4, 4),
            'difficulty': 'easy',
            'expected_steps': 4
        },
        {
            'name': 'ENV3_5x5_opposite',
            'grid_size': (5, 5),
            'agent_pos': (0, 0),
            'goal_pos': (4, 4),
            'difficulty': 'easy',
            'expected_steps': 8
        },
        
        # Moyen (7×7 à 8×8)
        {
            'name': 'ENV4_7x7_diagonal',
            'grid_size': (7, 7),
            'agent_pos': (1, 1),
            'goal_pos': (5, 5),
            'difficulty': 'medium',
            'expected_steps': 8
        },
        {
            'name': 'ENV5_8x8_far',
            'grid_size': (8, 8),
            'agent_pos': (0, 0),
            'goal_pos': (7, 7),
            'difficulty': 'medium',
            'expected_steps': 14
        },
        {
            'name': 'ENV6_8x8_asymmetric',
            'grid_size': (8, 8),
            'agent_pos': (2, 6),
            'goal_pos': (6, 1),
            'difficulty': 'medium',
            'expected_steps': 9
        },
        
        # Difficile (10×10)
        {
            'name': 'ENV7_10x10_corner',
            'grid_size': (10, 10),
            'agent_pos': (0, 0),
            'goal_pos': (9, 9),
            'difficulty': 'hard',
            'expected_steps': 18
        },
        {
            'name': 'ENV8_10x10_center_far',
            'grid_size': (10, 10),
            'agent_pos': (5, 5),
            'goal_pos': (0, 0),
            'difficulty': 'hard',
            'expected_steps': 10
        },
        {
            'name': 'ENV9_10x10_sparse',
            'grid_size': (10, 10),
            'agent_pos': (1, 8),
            'goal_pos': (8, 1),
            'difficulty': 'hard',
            'expected_steps': 14
        },
        {
            'name': 'ENV10_10x10_extreme',
            'grid_size': (10, 10),
            'agent_pos': (0, 9),
            'goal_pos': (9, 0),
            'difficulty': 'hard',
            'expected_steps': 18
        }
    ]
    
    return environments


def run_single_environment(env_config: Dict, kernel: DecisionKernelMinimal, log_file) -> Dict:
    """
    Exécute test sur un environnement et retourne métriques.
    
    Args:
        env_config: Configuration environnement
        kernel: Decision Kernel (réutilisé entre envs)
        log_file: Fichier logs forensic
    
    Returns:
        dict: Métriques performance
    """
    print(f"\n{'='*80}")
    print(f"TEST: {env_config['name']}")
    print(f"Grid: {env_config['grid_size']}, Difficulty: {env_config['difficulty']}")
    print(f"Agent: {env_config['agent_pos']} → Goal: {env_config['goal_pos']}")
    print(f"Expected steps: {env_config['expected_steps']}")
    print(f"{'='*80}\n")
    
    # Créer environnement
    env = ConfigurableGridEnv(
        env_config['grid_size'],
        env_config['agent_pos'],
        env_config['goal_pos']
    )
    
    # Créer World Model adapté à la taille
    world_model = WorldModelMinimal(grid_size=env_config['grid_size'])
    
    # Actions disponibles
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    
    # Reset
    state = env.reset()
    
    # Métriques
    total_reward = 0.0
    done = False
    step = 0
    trajectory = [state['agent_pos']]
    scores_history = []
    
    # Détection stagnation
    stagnation_counter = 0
    last_position = state['agent_pos']
    MAX_STAGNATION = 3
    
    while not done and step < env.max_steps:
        step += 1
        
        # Détecter stagnation
        if state['agent_pos'] == last_position:
            stagnation_counter += 1
        else:
            stagnation_counter = 0
            last_position = state['agent_pos']
        
        # Forcer exploration si stagnation
        if stagnation_counter >= MAX_STAGNATION:
            kernel.exploration_rate = 1.0
        else:
            kernel.exploration_rate = 0.2
        
        # Prédictions World Model
        for action in actions:
            future_state = world_model.predict_next_state(state, action)
            
            # Log prédiction
            log_file.write(json.dumps({
                'timestamp': time.time(),
                'env': env_config['name'],
                'step': step,
                'component': 'world_model',
                'event': 'prediction',
                'action': action,
                'current_pos': list(state['agent_pos']),
                'predicted_pos': list(future_state.get('agent_pos', state['agent_pos'])),
                'confidence': world_model.get_action_confidence(action)
            }) + '\n')
        
        # Décision
        action, decision_info = kernel.select_action(
            state, actions,
            world_model=world_model
        )
        
        scores_history.append(decision_info['scores'])
        
        # Log décision
        log_file.write(json.dumps({
            'timestamp': time.time(),
            'env': env_config['name'],
            'step': step,
            'component': 'decision_kernel',
            'event': 'decision',
            'action': action,
            'scores': decision_info['scores'],
            'is_exploration': decision_info['is_exploration'],
            'stagnation_counter': stagnation_counter
        }) + '\n')
        
        # Exécuter action
        next_state, reward, done = env.step(action)
        total_reward += reward
        trajectory.append(next_state['agent_pos'])
        
        # Observer transition
        world_model.observe_transition(state, action, next_state)
        
        # Log transition
        log_file.write(json.dumps({
            'timestamp': time.time(),
            'env': env_config['name'],
            'step': step,
            'component': 'environment',
            'event': 'transition',
            'action': action,
            'pos_before': list(state['agent_pos']),
            'pos_after': list(next_state['agent_pos']),
            'reward': float(reward),
            'done': done
        }) + '\n')
        
        # Apprentissage kernel
        kernel.update_weights(reward)
        
        state = next_state
    
    # Calculer métriques
    goal_reached = (state['agent_pos'] == env_config['goal_pos'])
    dist_final = abs(state['agent_pos'][0] - env_config['goal_pos'][0]) + \
                 abs(state['agent_pos'][1] - env_config['goal_pos'][1])
    
    # Calculer variance scores
    if scores_history:
        all_scores = [list(s.values()) for s in scores_history]
        scores_variance = float(np.var(all_scores))
    else:
        scores_variance = 0.0
    
    # Efficacité (steps réels vs attendus)
    efficiency = env_config['expected_steps'] / step if step > 0 else 0.0
    
    metrics = {
        'env_name': env_config['name'],
        'difficulty': env_config['difficulty'],
        'grid_size': env_config['grid_size'],
        'goal_reached': goal_reached,
        'steps': step,
        'expected_steps': env_config['expected_steps'],
        'efficiency': efficiency,
        'total_reward': total_reward,
        'dist_final': dist_final,
        'scores_variance': scores_variance,
        'trajectory_length': len(trajectory),
        'stagnation_detected': stagnation_counter >= MAX_STAGNATION
    }
    
    print(f"\nRésultats:")
    print(f"  Goal atteint: {'✅ OUI' if goal_reached else '❌ NON'}")
    print(f"  Steps: {step} (attendu: {env_config['expected_steps']}, efficacité: {efficiency:.2%})")
    print(f"  Reward total: {total_reward:.2f}")
    print(f"  Distance finale: {dist_final}")
    print(f"  Scores variance: {scores_variance:.4f}")
    
    return metrics


def run_robustesse_test():
    """Exécute test robustesse complet sur 10 environnements."""
    
    print("=" * 80)
    print("TEST ROBUSTESSE MULTI-ENVIRONNEMENTS - MAGEN V28.1")
    print("=" * 80)
    print()
    
    # Créer logs directory
    logs_dir = Path(__file__).parent / 'logs_forensic_v28_robustesse'
    logs_dir.mkdir(exist_ok=True)
    
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    log_file_path = logs_dir / f"robustesse_test_{timestamp}.jsonl"
    
    # Créer Decision Kernel (partagé entre tous les environnements)
    kernel = DecisionKernelMinimal(
        learning_rate=0.05,
        exploration_rate=0.2,
        temperature=1.0
    )
    
    # Créer environnements
    environments = create_test_environments()
    
    # Exécuter tests
    all_metrics = []
    
    with open(log_file_path, 'w') as log_file:
        for env_config in environments:
            metrics = run_single_environment(env_config, kernel, log_file)
            all_metrics.append(metrics)
    
    # Analyse globale
    print(f"\n{'='*80}")
    print("ANALYSE GLOBALE")
    print(f"{'='*80}\n")
    
    # Métriques par difficulté
    by_difficulty = {'easy': [], 'medium': [], 'hard': []}
    for m in all_metrics:
        by_difficulty[m['difficulty']].append(m)
    
    print("Taux succès par difficulté:")
    for diff in ['easy', 'medium', 'hard']:
        metrics = by_difficulty[diff]
        success_rate = sum(1 for m in metrics if m['goal_reached']) / len(metrics) * 100
        avg_steps = np.mean([m['steps'] for m in metrics])
        avg_efficiency = np.mean([m['efficiency'] for m in metrics])
        print(f"  {diff.upper():8s}: {success_rate:5.1f}% succès, {avg_steps:5.1f} steps moy, {avg_efficiency:5.1%} efficacité")
    
    # Métriques globales
    total_success = sum(1 for m in all_metrics if m['goal_reached'])
    print(f"\nGlobal: {total_success}/{len(all_metrics)} succès ({total_success/len(all_metrics)*100:.1f}%)")
    
    # Sauvegarder résultats
    results_file = logs_dir / f"robustesse_results_{timestamp}.json"
    with open(results_file, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'total_environments': len(all_metrics),
            'success_rate': total_success / len(all_metrics),
            'metrics_by_env': all_metrics,
            'metrics_by_difficulty': {
                diff: {
                    'success_rate': sum(1 for m in metrics if m['goal_reached']) / len(metrics),
                    'avg_steps': float(np.mean([m['steps'] for m in metrics])),
                    'avg_efficiency': float(np.mean([m['efficiency'] for m in metrics]))
                }
                for diff, metrics in by_difficulty.items()
            }
        }, f, indent=2)
    
    print(f"\nRésultats sauvegardés: {results_file}")
    print(f"Logs forensic: {log_file_path}")
    print(f"\n{'='*80}")
    print("TEST TERMINÉ")
    print(f"{'='*80}\n")


if __name__ == '__main__':
    run_robustesse_test()

# Made with Bob
