"""
Test Fragilités Réelles - MAGEN V28.1
Environnements avec obstacles, multi-goals, stochastique

Objectif: Révéler les 3 fragilités prédites par feedback critique:
1. Spatial dominance collapse (obstacles bloquent chemin direct)
2. World model lag (transitions non-déterministes)
3. Reward sparsity (goals multiples, rewards rares)

Protocole MAGEN:
- 5 environnements complexes
- Logs forensic détaillés
- Identification fragilités empiriques

Date: 2026-06-15
"""

import sys
import os
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple, Set
import numpy as np
import random

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

from decision_kernel_minimal import DecisionKernelMinimal
from world_model_minimal import WorldModelMinimal


class ComplexGridEnv:
    """Environnement grid avec obstacles, multi-goals, stochastique."""
    
    def __init__(
        self,
        grid_size: Tuple[int, int],
        agent_pos: Tuple[int, int],
        goal_positions: List[Tuple[int, int]],
        obstacles: Set[Tuple[int, int]] = None,
        stochastic: bool = False
    ):
        """
        Args:
            grid_size: (width, height)
            agent_pos: Position initiale agent
            goal_positions: Liste positions goals (multi-goal)
            obstacles: Set positions obstacles
            stochastic: Si True, actions ont 20% chance d'échouer
        """
        self.grid_size = grid_size
        self.initial_agent_pos = agent_pos
        self.agent_pos = agent_pos
        self.goal_positions = goal_positions
        self.goals_reached = set()
        self.obstacles = obstacles or set()
        self.stochastic = stochastic
        self.step_count = 0
        self.max_steps = grid_size[0] * grid_size[1] * 2
        
    def reset(self):
        """Réinitialise environnement."""
        self.agent_pos = self.initial_agent_pos
        self.goals_reached = set()
        self.step_count = 0
        return self.get_state()
    
    def get_state(self):
        """Retourne état actuel."""
        grid = np.zeros(self.grid_size, dtype=int)
        
        # Agent
        grid[self.agent_pos[1], self.agent_pos[0]] = 1
        
        # Goals non atteints
        for goal in self.goal_positions:
            if goal not in self.goals_reached:
                grid[goal[1], goal[0]] = 2
        
        # Obstacles
        for obs in self.obstacles:
            grid[obs[1], obs[0]] = -1
        
        return {
            'grid': grid,
            'agent_pos': self.agent_pos,
            'goal_positions': self.goal_positions,
            'goals_reached': list(self.goals_reached),
            'obstacles': list(self.obstacles),
            'step': self.step_count,
            'grid_size': self.grid_size
        }
    
    def step(self, action):
        """Exécute action et retourne (next_state, reward, done)."""
        old_pos = self.agent_pos
        
        # Stochastique: 20% chance action échoue
        if self.stochastic and random.random() < 0.2:
            # Action échoue, agent ne bouge pas
            new_pos = old_pos
        else:
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
            
            new_pos = (x, y)
            
            # Vérifier collision obstacle
            if new_pos in self.obstacles:
                new_pos = old_pos  # Reste en place
        
        self.agent_pos = new_pos
        self.step_count += 1
        
        # Calculer reward
        reward = 0.0
        
        # Pénalité collision obstacle
        if new_pos == old_pos and new_pos not in self.obstacles:
            reward -= 0.1  # Petite pénalité si bloqué
        
        # Reward si atteint nouveau goal
        if new_pos in self.goal_positions and new_pos not in self.goals_reached:
            self.goals_reached.add(new_pos)
            reward += 10.0  # Gros reward pour goal
        
        # Reward distance au goal le plus proche non atteint
        remaining_goals = [g for g in self.goal_positions if g not in self.goals_reached]
        if remaining_goals:
            dist_before = min(abs(old_pos[0] - g[0]) + abs(old_pos[1] - g[1]) for g in remaining_goals)
            dist_after = min(abs(new_pos[0] - g[0]) + abs(new_pos[1] - g[1]) for g in remaining_goals)
            reward += (dist_before - dist_after) * 0.5  # Reward réduit pour approche
        
        # Done si tous goals atteints ou max steps
        done = (len(self.goals_reached) == len(self.goal_positions)) or (self.step_count >= self.max_steps)
        
        return self.get_state(), reward, done


def create_complex_environments() -> List[Dict]:
    """
    Crée 5 environnements complexes pour révéler fragilités.
    
    Returns:
        Liste configs environnements
    """
    environments = [
        # ENV1: Obstacle simple (mur vertical)
        {
            'name': 'ENV1_obstacle_mur',
            'grid_size': (7, 7),
            'agent_pos': (1, 3),
            'goal_positions': [(5, 3)],
            'obstacles': {(3, 0), (3, 1), (3, 2), (3, 3), (3, 4), (3, 5), (3, 6)},  # Mur vertical
            'stochastic': False,
            'difficulty': 'medium',
            'expected_behavior': 'Doit contourner mur (spatial dominance test)'
        },
        
        # ENV2: Labyrinthe simple
        {
            'name': 'ENV2_labyrinthe',
            'grid_size': (9, 9),
            'agent_pos': (1, 1),
            'goal_positions': [(7, 7)],
            'obstacles': {
                # Murs formant labyrinthe
                (3, 0), (3, 1), (3, 2), (3, 3), (3, 4),
                (5, 4), (5, 5), (5, 6), (5, 7), (5, 8),
                (1, 5), (2, 5), (3, 5), (4, 5)
            },
            'stochastic': False,
            'difficulty': 'hard',
            'expected_behavior': 'Doit trouver chemin non-direct'
        },
        
        # ENV3: Multi-goals (2 goals)
        {
            'name': 'ENV3_multi_goals',
            'grid_size': (8, 8),
            'agent_pos': (4, 4),
            'goal_positions': [(1, 1), (6, 6)],  # 2 goals
            'obstacles': set(),
            'stochastic': False,
            'difficulty': 'medium',
            'expected_behavior': 'Doit atteindre les 2 goals (reward sparsity test)'
        },
        
        # ENV4: Stochastique (actions échouent 20%)
        {
            'name': 'ENV4_stochastique',
            'grid_size': (6, 6),
            'agent_pos': (1, 1),
            'goal_positions': [(4, 4)],
            'obstacles': set(),
            'stochastic': True,  # Actions échouent 20%
            'difficulty': 'medium',
            'expected_behavior': 'Doit gérer échecs actions (world model lag test)'
        },
        
        # ENV5: Combiné (obstacles + multi-goals + stochastique)
        {
            'name': 'ENV5_extreme',
            'grid_size': (10, 10),
            'agent_pos': (1, 1),
            'goal_positions': [(3, 8), (8, 3)],  # 2 goals éloignés
            'obstacles': {
                (5, 0), (5, 1), (5, 2), (5, 3), (5, 4),  # Mur vertical
                (0, 5), (1, 5), (2, 5), (3, 5), (4, 5)   # Mur horizontal
            },
            'stochastic': True,
            'difficulty': 'extreme',
            'expected_behavior': 'Test ultime: tous les challenges combinés'
        }
    ]
    
    return environments


def run_complex_environment(env_config: Dict, kernel: DecisionKernelMinimal, log_file) -> Dict:
    """
    Exécute test sur environnement complexe.
    
    Args:
        env_config: Configuration environnement
        kernel: Decision Kernel
        log_file: Fichier logs forensic
    
    Returns:
        dict: Métriques + fragilités détectées
    """
    print(f"\n{'='*80}")
    print(f"TEST: {env_config['name']}")
    print(f"Grid: {env_config['grid_size']}, Difficulty: {env_config['difficulty']}")
    print(f"Agent: {env_config['agent_pos']}")
    print(f"Goals: {env_config['goal_positions']} ({len(env_config['goal_positions'])} goals)")
    print(f"Obstacles: {len(env_config['obstacles'])} positions")
    print(f"Stochastic: {env_config['stochastic']}")
    print(f"Expected: {env_config['expected_behavior']}")
    print(f"{'='*80}\n")
    
    # Créer environnement
    env = ComplexGridEnv(
        env_config['grid_size'],
        env_config['agent_pos'],
        env_config['goal_positions'],
        env_config['obstacles'],
        env_config['stochastic']
    )
    
    # Créer World Model
    world_model = WorldModelMinimal(grid_size=env_config['grid_size'])
    
    # Actions
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    
    # Reset
    state = env.reset()
    
    # Métriques
    total_reward = 0.0
    done = False
    step = 0
    trajectory = [state['agent_pos']]
    collision_count = 0
    stagnation_count = 0
    scores_history = []
    
    # Détection fragilités
    fragilites = {
        'spatial_dominance_collapse': False,
        'world_model_lag': False,
        'reward_sparsity': False,
        'details': []
    }
    
    # Détection stagnation
    stagnation_counter = 0
    last_position = state['agent_pos']
    MAX_STAGNATION = 5
    
    while not done and step < env.max_steps:
        step += 1
        
        # Détecter stagnation
        if state['agent_pos'] == last_position:
            stagnation_counter += 1
            stagnation_count += 1
        else:
            stagnation_counter = 0
            last_position = state['agent_pos']
        
        # Forcer exploration si stagnation
        if stagnation_counter >= MAX_STAGNATION:
            kernel.exploration_rate = 1.0
            fragilites['spatial_dominance_collapse'] = True
            fragilites['details'].append(f"Step {step}: Stagnation {stagnation_counter} steps (spatial dominance?)")
        else:
            kernel.exploration_rate = 0.2
        
        # Prédictions World Model
        for action in actions:
            future_state = world_model.predict_next_state(state, action)
            
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
        
        # Détecter collision
        if next_state['agent_pos'] == state['agent_pos'] and next_state['agent_pos'] in env_config['obstacles']:
            collision_count += 1
        
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
            'done': done,
            'goals_reached': len(next_state['goals_reached'])
        }) + '\n')
        
        # Apprentissage
        kernel.update_weights(reward)
        
        state = next_state
    
    # Analyser fragilités
    goals_reached = len(state['goals_reached'])
    goals_total = len(env_config['goal_positions'])
    
    # Fragilité reward sparsity
    if goals_reached < goals_total:
        fragilites['reward_sparsity'] = True
        fragilites['details'].append(f"Goals: {goals_reached}/{goals_total} atteints (reward sparsity)")
    
    # Fragilité world model lag (stochastique)
    if env_config['stochastic'] and stagnation_count > step * 0.3:
        fragilites['world_model_lag'] = True
        fragilites['details'].append(f"Stagnation: {stagnation_count}/{step} steps (world model lag?)")
    
    # Calculer métriques
    if scores_history:
        all_scores = [list(s.values()) for s in scores_history]
        scores_variance = float(np.var(all_scores))
    else:
        scores_variance = 0.0
    
    metrics = {
        'env_name': env_config['name'],
        'difficulty': env_config['difficulty'],
        'goals_reached': goals_reached,
        'goals_total': goals_total,
        'success_rate': goals_reached / goals_total,
        'steps': step,
        'total_reward': total_reward,
        'collision_count': collision_count,
        'stagnation_count': stagnation_count,
        'scores_variance': scores_variance,
        'fragilites': fragilites
    }
    
    print(f"\nRésultats:")
    print(f"  Goals: {goals_reached}/{goals_total} ({'✅' if goals_reached == goals_total else '❌'})")
    print(f"  Steps: {step}")
    print(f"  Reward: {total_reward:.2f}")
    print(f"  Collisions: {collision_count}")
    print(f"  Stagnation: {stagnation_count} steps")
    print(f"  Scores variance: {scores_variance:.4f}")
    print(f"\nFragilités détectées:")
    for key, value in fragilites.items():
        if key != 'details' and value:
            print(f"  ⚠️  {key}: OUI")
    for detail in fragilites['details']:
        print(f"    - {detail}")
    
    return metrics


def run_fragilites_test():
    """Exécute test fragilités sur 5 environnements complexes."""
    
    print("=" * 80)
    print("TEST FRAGILITÉS RÉELLES - MAGEN V28.1")
    print("=" * 80)
    print()
    
    # Créer logs directory
    logs_dir = Path(__file__).parent / 'logs_forensic_v28_fragilites'
    logs_dir.mkdir(exist_ok=True)
    
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    log_file_path = logs_dir / f"fragilites_test_{timestamp}.jsonl"
    
    # Créer Decision Kernel
    kernel = DecisionKernelMinimal(
        learning_rate=0.05,
        exploration_rate=0.2,
        temperature=1.0
    )
    
    # Créer environnements
    environments = create_complex_environments()
    
    # Exécuter tests
    all_metrics = []
    
    with open(log_file_path, 'w') as log_file:
        for env_config in environments:
            metrics = run_complex_environment(env_config, kernel, log_file)
            all_metrics.append(metrics)
    
    # Analyse globale
    print(f"\n{'='*80}")
    print("ANALYSE GLOBALE FRAGILITÉS")
    print(f"{'='*80}\n")
    
    # Compter fragilités
    fragilites_count = {
        'spatial_dominance_collapse': 0,
        'world_model_lag': 0,
        'reward_sparsity': 0
    }
    
    for m in all_metrics:
        for key in fragilites_count:
            if m['fragilites'][key]:
                fragilites_count[key] += 1
    
    print("Fragilités observées:")
    for key, count in fragilites_count.items():
        print(f"  {key}: {count}/5 environnements ({count/5*100:.0f}%)")
    
    # Taux succès global
    total_goals = sum(m['goals_total'] for m in all_metrics)
    reached_goals = sum(m['goals_reached'] for m in all_metrics)
    print(f"\nGoals atteints: {reached_goals}/{total_goals} ({reached_goals/total_goals*100:.1f}%)")
    
    # Sauvegarder résultats
    results_file = logs_dir / f"fragilites_results_{timestamp}.json"
    with open(results_file, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'total_environments': len(all_metrics),
            'fragilites_count': fragilites_count,
            'metrics_by_env': all_metrics
        }, f, indent=2)
    
    print(f"\nRésultats sauvegardés: {results_file}")
    print(f"Logs forensic: {log_file_path}")
    print(f"\n{'='*80}")
    print("TEST TERMINÉ")
    print(f"{'='*80}\n")


if __name__ == '__main__':
    run_fragilites_test()

# Made with Bob
