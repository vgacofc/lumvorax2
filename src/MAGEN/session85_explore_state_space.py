#!/usr/bin/env python3
"""
SESSION 85 - EXPLORATION ESPACE D'ÉTATS COMPLÈTE
Objectif: Exécuter actions EN BOUCLE jusqu'à arrêt du jeu
Hypothèse: ACTION1/3/4 = même opérateur T sur régions différentes
Date: 2026-06-18
Auteur: Bob (basé sur analyse révolutionnaire utilisateur)
"""

import numpy as np
import json
from datetime import datetime
from pathlib import Path
import sys
from collections import defaultdict
import hashlib

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class StateSpaceExplorer:
    """Explore l'espace d'états jusqu'à arrêt du jeu"""
    
    def __init__(self):
        self.env = None
        self.states_visited = {}  # hash -> (step, frame, action_taken)
        self.transitions = []  # (state_hash, action, new_state_hash, reward, done)
        self.step_count = 0
        self.max_steps = 10000  # Sécurité
        
    def setup_environment(self, game_id: str = "ls20-9607627b"):
        """Initialiser environnement"""
        print("\n" + "="*80)
        print("SESSION 85 - EXPLORATION ESPACE D'ÉTATS")
        print("="*80)
        print(f"\nObjectif: Exécuter actions EN BOUCLE jusqu'à arrêt du jeu")
        print(f"Hypothèse: ACTION1/3/4 = opérateur T sur régions différentes")
        print(f"Sécurité: Max {self.max_steps} steps")
        
        print("\n" + "="*80)
        print("INITIALISATION ENVIRONNEMENT")
        print("="*80)
        
        # Vérifier game_id
        if game_id != "ls20-9607627b":
            raise ValueError(f"❌ ERREUR: game_id={game_id} != ls20-9607627b")
        print(f"✅ Game ID vérifié: {game_id}")
        
        # Nouvelle API
        arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        self.env = arcade.make(game_id)
        print(f"✅ Environnement créé pour {game_id}")
        
        # Reset initial
        obs = self.env.reset()
        frame = self.extract_frame(obs)
        if frame is not None:
            print(f"✅ Frame shape: {frame.shape}")
        else:
            raise ValueError("Frame extraction retourne None")
        
        return obs
    
    def extract_frame(self, obs):
        """Extraire frame 2D depuis observation"""
        if hasattr(obs, 'frame'):
            frame_data = obs.frame
            if isinstance(frame_data, list):
                frame_array = np.asarray(frame_data)
                if frame_array.ndim == 3 and frame_array.shape[0] == 1:
                    return frame_array[0]
                return frame_array
            return frame_data
        return None
    
    def hash_frame(self, frame: np.ndarray) -> str:
        """Calculer hash unique d'une frame"""
        return hashlib.sha256(frame.tobytes()).hexdigest()[:16]
    
    def analyze_frame(self, frame: np.ndarray) -> dict:
        """Analyser distribution pixels dans frame"""
        unique, counts = np.unique(frame, return_counts=True)
        distribution = dict(zip(unique.tolist(), counts.tolist()))
        
        return {
            'shape': frame.shape,
            'distribution': distribution,
            'total_pixels': frame.size
        }
    
    def explore_until_done(self, strategy: str = "cycle_actions"):
        """
        Explorer espace d'états jusqu'à done=True
        
        Stratégies:
        - cycle_actions: ACTION1, ACTION2, ACTION3, ACTION4, répéter
        - random: Actions aléatoires
        - action2_only: Seulement ACTION2
        """
        print("\n" + "="*80)
        print(f"EXPLORATION STRATÉGIE: {strategy}")
        print("="*80)
        
        # Reset
        obs = self.env.reset()
        frame = self.extract_frame(obs)
        if frame is None:
            raise ValueError("Frame extraction retourne None")
        
        done = False
        self.step_count = 0
        
        # État initial
        initial_hash = self.hash_frame(frame)
        self.states_visited[initial_hash] = {
            'step': 0,
            'frame': frame.copy(),
            'action_taken': None,
            'reward': 0.0,
            'analysis': self.analyze_frame(frame)
        }
        
        print(f"\n📍 État initial:")
        print(f"   Hash: {initial_hash}")
        print(f"   Distribution: {self.states_visited[initial_hash]['analysis']['distribution']}")
        
        # Actions disponibles
        actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
        action_names = ["ACTION1", "ACTION2", "ACTION3", "ACTION4"]
        
        current_hash = initial_hash
        reward = 0.0  # Initialiser reward
        
        # Boucle exploration
        print("\n" + "="*80)
        print("BOUCLE EXPLORATION")
        print("="*80)
        
        while not done and self.step_count < self.max_steps:
            self.step_count += 1
            
            # Choisir action selon stratégie
            if strategy == "cycle_actions":
                action_idx = (self.step_count - 1) % len(actions)
                action = actions[action_idx]
                action_name = action_names[action_idx]
            elif strategy == "action2_only":
                action = GameAction.ACTION2
                action_name = "ACTION2"
            else:
                action_idx = np.random.randint(len(actions))
                action = actions[action_idx]
                action_name = action_names[action_idx]
            
            # Exécuter action
            obs = self.env.step(action)
            frame_after = self.extract_frame(obs)
            if frame_after is None:
                raise ValueError(f"Frame extraction retourne None après step {self.step_count}")
            
            # Utiliser levels_completed comme reward et state pour done
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Hash nouvel état
            new_hash = self.hash_frame(frame_after)
            
            # Enregistrer transition
            self.transitions.append({
                'step': self.step_count,
                'state_from': current_hash,
                'action': action_name,
                'state_to': new_hash,
                'reward': float(reward),
                'done': done
            })
            
            # Nouvel état?
            if new_hash not in self.states_visited:
                self.states_visited[new_hash] = {
                    'step': self.step_count,
                    'frame': frame_after.copy(),
                    'action_taken': action_name,
                    'reward': float(reward),
                    'analysis': self.analyze_frame(frame_after)
                }
                print(f"\n✨ NOUVEL ÉTAT découvert!")
                print(f"   Step: {self.step_count}")
                print(f"   Action: {action_name}")
                print(f"   Hash: {new_hash}")
                print(f"   Reward: {reward}")
                print(f"   Done: {done}")
                print(f"   Distribution: {self.states_visited[new_hash]['analysis']['distribution']}")
            else:
                # État déjà visité
                if self.step_count % 10 == 0:
                    print(f"   Step {self.step_count}: {action_name} -> État connu {new_hash[:8]}... (reward={reward}, done={done})")
            
            # Vérifier si done
            if done:
                print("\n" + "="*80)
                print("🎯 JEU TERMINÉ!")
                print("="*80)
                print(f"   Step final: {self.step_count}")
                print(f"   Reward final: {reward}")
                print(f"   Raison: done=True")
                break
            
            current_hash = new_hash
        
        # Fin boucle
        if self.step_count >= self.max_steps:
            print("\n" + "="*80)
            print("⚠️ LIMITE SÉCURITÉ ATTEINTE")
            print("="*80)
            print(f"   Max steps: {self.max_steps}")
            print(f"   Le jeu n'a pas signalé done=True")
        
        # Récupérer reward final (défaut 0.0 si boucle vide)
        final_reward = float(reward) if 'reward' in locals() else 0.0
        
        return {
            'total_steps': self.step_count,
            'states_discovered': len(self.states_visited),
            'final_reward': final_reward,
            'done': done
        }
    
    def analyze_results(self):
        """Analyser résultats exploration"""
        print("\n" + "="*80)
        print("ANALYSE RÉSULTATS")
        print("="*80)
        
        print(f"\n📊 Statistiques globales:")
        print(f"   Total steps: {self.step_count}")
        print(f"   États découverts: {len(self.states_visited)}")
        print(f"   Transitions enregistrées: {len(self.transitions)}")
        
        # Analyser transitions
        print(f"\n🔄 Analyse transitions:")
        
        # Compter transitions par action
        action_counts = defaultdict(int)
        action_rewards = defaultdict(list)
        
        for trans in self.transitions:
            action_counts[trans['action']] += 1
            action_rewards[trans['action']].append(trans['reward'])
        
        for action in sorted(action_counts.keys()):
            count = action_counts[action]
            rewards = action_rewards[action]
            avg_reward = np.mean(rewards) if rewards else 0.0
            print(f"   {action}: {count} fois (reward moyen: {avg_reward:.4f})")
        
        # Analyser cycles
        print(f"\n🔁 Recherche cycles:")
        state_sequence = [trans['state_to'] for trans in self.transitions]
        
        # Détecter premier cycle
        for i in range(len(state_sequence)):
            state = state_sequence[i]
            if state in state_sequence[:i]:
                first_occurrence = state_sequence.index(state)
                cycle_length = i - first_occurrence
                print(f"   Premier cycle détecté:")
                print(f"      État: {state[:8]}...")
                print(f"      Première occurrence: step {first_occurrence + 1}")
                print(f"      Répétition: step {i + 1}")
                print(f"      Longueur cycle: {cycle_length} steps")
                break
        else:
            print(f"   Aucun cycle détecté")
        
        # Analyser rewards
        print(f"\n💰 Analyse rewards:")
        all_rewards = [trans['reward'] for trans in self.transitions]
        unique_rewards = set(all_rewards)
        print(f"   Rewards uniques: {sorted(unique_rewards)}")
        print(f"   Reward min: {min(all_rewards)}")
        print(f"   Reward max: {max(all_rewards)}")
        print(f"   Reward moyen: {np.mean(all_rewards):.4f}")
        
        # Analyser états
        print(f"\n🗺️ Analyse états:")
        print(f"   États uniques: {len(self.states_visited)}")
        
        # Trouver états avec reward > 0
        rewarding_states = [
            (hash_val, data) 
            for hash_val, data in self.states_visited.items() 
            if data['reward'] > 0
        ]
        
        if rewarding_states:
            print(f"   États avec reward > 0: {len(rewarding_states)}")
            for hash_val, data in rewarding_states[:5]:
                print(f"      {hash_val[:8]}... (step {data['step']}, reward={data['reward']})")
        else:
            print(f"   Aucun état avec reward > 0")
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session85_state_exploration_{timestamp}.json"
        
        # Préparer données (sans frames numpy)
        results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session85_explore_state_space.py',
                'total_steps': self.step_count,
                'states_discovered': len(self.states_visited),
                'max_steps': self.max_steps
            },
            'states': {
                hash_val: {
                    'step': data['step'],
                    'action_taken': data['action_taken'],
                    'reward': data['reward'],
                    'analysis': data['analysis']
                }
                for hash_val, data in self.states_visited.items()
            },
            'transitions': self.transitions
        }
        
        filepath = Path(__file__).parent / filename
        with open(filepath, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename


def main():
    """Point d'entrée principal"""
    explorer = StateSpaceExplorer()
    
    try:
        # Setup
        explorer.setup_environment("ls20-9607627b")
        
        # Explorer avec stratégie cycle
        print("\n" + "="*80)
        print("STRATÉGIE: CYCLE ACTIONS (ACTION1→2→3→4→1...)")
        print("="*80)
        
        summary = explorer.explore_until_done(strategy="cycle_actions")
        
        # Analyser
        explorer.analyze_results()
        
        # Sauvegarder
        filename = explorer.save_results()
        
        print("\n" + "="*80)
        print("✅ SESSION 85 COMPLÉTÉE")
        print("="*80)
        print(f"   Total steps: {summary['total_steps']}")
        print(f"   États découverts: {summary['states_discovered']}")
        print(f"   Reward final: {summary['final_reward']}")
        print(f"   Done: {summary['done']}")
        print(f"   Fichier: {filename}")
        
        return 0
        
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
