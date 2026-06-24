"""
Test Minimal - Decision Kernel + World Model
Validation architecture V28 sur environnement 5×5

Protocole MAGEN:
1. Exécution test 20 steps
2. Génération logs forensic temps réel
3. Lecture logs immédiate
4. Rapport basé données empiriques

Date: 2026-06-14
"""

import sys
import os
import json
import time
from pathlib import Path

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

from decision_kernel_minimal import DecisionKernelMinimal
from world_model_minimal import WorldModelMinimal
import numpy as np


class SimpleGridEnv:
    """Environnement 5×5 minimal pour test."""
    
    def __init__(self):
        self.grid_size = (5, 5)
        self.agent_pos = (2, 2)  # Centre
        self.goal_pos = (4, 4)   # Coin
        self.step_count = 0
        self.max_steps = 20
        
    def reset(self):
        """Réinitialise environnement."""
        self.agent_pos = (2, 2)
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
            'step': self.step_count
        }
    
    def step(self, action):
        """Exécute action et retourne (next_state, reward, done)."""
        # Sauvegarder position avant
        old_pos = self.agent_pos
        
        # Appliquer action
        x, y = self.agent_pos
        if action == 'ACTION1' or action == 'move_up':
            y = max(0, y - 1)
        elif action == 'ACTION2' or action == 'move_down':
            y = min(self.grid_size[0] - 1, y + 1)
        elif action == 'ACTION3' or action == 'move_left':
            x = max(0, x - 1)
        elif action == 'ACTION4' or action == 'move_right':
            x = min(self.grid_size[1] - 1, x + 1)
        
        self.agent_pos = (x, y)
        self.step_count += 1
        
        # Calculer reward
        dist_before = abs(old_pos[0] - self.goal_pos[0]) + abs(old_pos[1] - self.goal_pos[1])
        dist_after = abs(self.agent_pos[0] - self.goal_pos[0]) + abs(self.agent_pos[1] - self.goal_pos[1])
        
        # Reward = réduction distance
        reward = dist_before - dist_after
        
        # Bonus si atteint goal
        if self.agent_pos == self.goal_pos:
            reward += 10.0
        
        # Done si goal atteint ou max steps
        done = (self.agent_pos == self.goal_pos) or (self.step_count >= self.max_steps)
        
        return self.get_state(), reward, done


def run_test():
    """Exécute test et génère logs forensic."""
    
    print("=" * 80)
    print("TEST DECISION KERNEL + WORLD MODEL - MAGEN V28")
    print("=" * 80)
    print()
    
    # Créer logs directory
    logs_dir = Path(__file__).parent / 'logs_forensic_v28_test'
    logs_dir.mkdir(exist_ok=True)
    
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    log_file = logs_dir / f"test_kernel_world_model_{timestamp}.jsonl"
    
    # Initialiser composants
    env = SimpleGridEnv()
    kernel = DecisionKernelMinimal(
        learning_rate=0.05,
        exploration_rate=0.2,
        temperature=1.0
    )
    world_model = WorldModelMinimal(grid_size=(5, 5))
    
    # Actions disponibles
    actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    
    # Logs forensic
    forensic_logs = []
    
    # Reset
    state = env.reset()
    
    print(f"État initial:")
    print(f"  Agent: {state['agent_pos']}")
    print(f"  Goal: {state['goal_pos']}")
    print(f"  Distance: {abs(state['agent_pos'][0] - state['goal_pos'][0]) + abs(state['agent_pos'][1] - state['goal_pos'][1])}")
    print()
    
    # Boucle principale
    total_reward = 0.0
    done = False
    step = 0
    
    # CORRECTION #3: Détection stagnation
    stagnation_counter = 0
    last_position = state['agent_pos']
    MAX_STAGNATION = 3  # Force exploration après 3 steps sans mouvement
    
    while not done and step < 20:
        step += 1
        
        print(f"--- Step {step} ---")
        
        # Détecter stagnation
        if state['agent_pos'] == last_position:
            stagnation_counter += 1
        else:
            stagnation_counter = 0
            last_position = state['agent_pos']
        
        # Forcer exploration si stagnation
        if stagnation_counter >= MAX_STAGNATION:
            print(f"  ⚠️  STAGNATION DÉTECTÉE ({stagnation_counter} steps) - Force exploration")
            kernel.exploration_rate = 1.0  # 100% exploration
        else:
            kernel.exploration_rate = 0.2  # Normal
        
        # CRITIQUE: Prédire états futurs pour chaque action
        future_states = {}
        for action in actions:
            future_state = world_model.predict_next_state(state, action)
            future_states[action] = future_state
            
            # Log prédiction
            forensic_logs.append({
                'timestamp': time.time(),
                'step': step,
                'component': 'world_model',
                'event': 'prediction',
                'action': action,
                'current_pos': state['agent_pos'],
                'predicted_pos': future_state.get('agent_pos'),
                'confidence': world_model.get_action_confidence(action)
            })
        
        # Décision basée sur états futurs prédits
        # CORRECTION #1: Passer world_model au kernel
        action, decision_info = kernel.select_action(
            state, actions,
            reward_shaper=None,  # Pas encore intégré
            reputation_system=None,
            grid_analyzer=None,
            causal_graph=None,
            world_model=world_model  # NOUVEAU V28.1
        )
        
        # Log décision
        forensic_logs.append({
            'timestamp': time.time(),
            'step': step,
            'component': 'decision_kernel',
            'event': 'decision',
            'action': action,
            'scores': decision_info['scores'],
            'probs': decision_info['probs'],
            'is_exploration': decision_info['is_exploration'],
            'weights': decision_info['weights']
        })
        
        print(f"  Action: {action} ({'exploration' if decision_info['is_exploration'] else 'exploitation'})")
        print(f"  Scores: {decision_info['scores']}")
        print(f"  Weights: {decision_info['weights']}")
        
        # Exécuter action
        next_state, reward, done = env.step(action)
        total_reward += reward
        
        print(f"  Position: {state['agent_pos']} → {next_state['agent_pos']}")
        print(f"  Reward: {reward:.2f} (total: {total_reward:.2f})")
        
        # Observer transition (World Model apprend)
        world_model.observe_transition(state, action, next_state)
        
        # Log transition
        forensic_logs.append({
            'timestamp': time.time(),
            'step': step,
            'component': 'environment',
            'event': 'transition',
            'action': action,
            'pos_before': state['agent_pos'],
            'pos_after': next_state['agent_pos'],
            'reward': reward,
            'done': done
        })
        
        # Apprentissage (Decision Kernel ajuste poids)
        kernel.update_weights(reward)
        
        # Log apprentissage
        forensic_logs.append({
            'timestamp': time.time(),
            'step': step,
            'component': 'decision_kernel',
            'event': 'learning',
            'reward': reward,
            'weights_after': kernel.weights.copy()
        })
        
        # Prochaine itération
        state = next_state
        print()
    
    # Résultats finaux
    print("=" * 80)
    print("RÉSULTATS FINAUX")
    print("=" * 80)
    print(f"Steps: {step}")
    print(f"Total reward: {total_reward:.2f}")
    print(f"Goal atteint: {'OUI' if state['agent_pos'] == state['goal_pos'] else 'NON'}")
    print(f"Distance finale: {abs(state['agent_pos'][0] - state['goal_pos'][0]) + abs(state['agent_pos'][1] - state['goal_pos'][1])}")
    print()
    print(f"Decision Kernel stats: {kernel.get_stats()}")
    print(f"World Model stats: {world_model.get_stats()}")
    print()
    
    # Sauvegarder logs forensic
    with open(log_file, 'w') as f:
        for log in forensic_logs:
            f.write(json.dumps(log) + '\n')
    
    print(f"Logs forensic sauvegardés: {log_file}")
    print(f"Total logs: {len(forensic_logs)}")
    print()
    
    return {
        'success': state['agent_pos'] == state['goal_pos'],
        'steps': step,
        'total_reward': total_reward,
        'kernel_stats': kernel.get_stats(),
        'world_model_stats': world_model.get_stats(),
        'log_file': str(log_file),
        'log_count': len(forensic_logs)
    }


if __name__ == '__main__':
    result = run_test()
    
    print("=" * 80)
    print("TEST TERMINÉ")
    print("=" * 80)
    print(json.dumps(result, indent=2))

# Made with Bob
