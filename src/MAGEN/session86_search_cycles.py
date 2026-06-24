#!/usr/bin/env python3
"""
SESSION 86 - RECHERCHE CYCLES COMPLETS
Objectif: Tester si on peut revenir à l'état initial (créer un cycle)
Méthode: Explorer toutes les actions jusqu'à trouver un cycle ou épuisement
Date: 2026-06-18
Auteur: Bob
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


class CycleSearcher:
    """Recherche cycles complets dans l'espace d'états"""
    
    def __init__(self):
        self.env = None
        self.states_visited = {}  # hash -> (step, frame, action_taken)
        self.transitions = []
        self.step_count = 0
        self.max_steps = 10000
        
    def setup_environment(self, game_id: str = "ls20-9607627b"):
        """Initialiser environnement"""
        print("\n" + "="*80)
        print("SESSION 86 - RECHERCHE CYCLES COMPLETS")
        print("="*80)
        print(f"\nObjectif: Trouver un cycle (revenir à l'état initial)")
        print(f"Méthode: Explorer TOUTES les actions possibles")
        print(f"Sécurité: Max {self.max_steps} steps")
        
        print("\n" + "="*80)
        print("INITIALISATION ENVIRONNEMENT")
        print("="*80)
        
        if game_id != "ls20-9607627b":
            raise ValueError(f"❌ ERREUR: game_id={game_id} != ls20-9607627b")
        print(f"✅ Game ID vérifié: {game_id}")
        
        arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        self.env = arcade.make(game_id)
        print(f"✅ Environnement créé pour {game_id}")
        
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
    
    def search_cycle_exhaustive(self):
        """
        Recherche exhaustive de cycles
        Teste TOUTES les actions à chaque état jusqu'à trouver un cycle
        """
        print("\n" + "="*80)
        print("RECHERCHE EXHAUSTIVE DE CYCLES")
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
        actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, 
                   GameAction.ACTION4, GameAction.ACTION5, GameAction.ACTION6, GameAction.ACTION7]
        action_names = ["ACTION1", "ACTION2", "ACTION3", "ACTION4", "ACTION5", "ACTION6", "ACTION7"]
        
        # File d'exploration (BFS)
        to_explore = [(initial_hash, [])]  # (state_hash, path_to_reach)
        explored = set([initial_hash])
        
        print("\n" + "="*80)
        print("EXPLORATION EXHAUSTIVE (BFS)")
        print("="*80)
        
        cycle_found = False
        
        while to_explore and not cycle_found and self.step_count < self.max_steps:
            current_hash, path = to_explore.pop(0)
            
            # Pour chaque action possible
            for action_idx, action in enumerate(actions):
                self.step_count += 1
                action_name = action_names[action_idx]
                
                # Reset au début puis rejouer le chemin
                obs = self.env.reset()
                for prev_action_name in path:
                    prev_action = actions[action_names.index(prev_action_name)]
                    obs = self.env.step(prev_action)
                
                # Exécuter l'action actuelle
                obs = self.env.step(action)
                frame_after = self.extract_frame(obs)
                if frame_after is None:
                    continue
                
                reward = float(obs.levels_completed)
                done = (obs.state != GameState.NOT_FINISHED)
                
                # Hash nouvel état
                new_hash = self.hash_frame(frame_after)
                
                # Enregistrer transition
                new_path = path + [action_name]
                self.transitions.append({
                    'step': self.step_count,
                    'state_from': current_hash,
                    'action': action_name,
                    'state_to': new_hash,
                    'reward': reward,
                    'done': done,
                    'path': new_path
                })
                
                # Vérifier si on revient à l'état initial
                if new_hash == initial_hash:
                    print(f"\n🎯 CYCLE TROUVÉ!")
                    print(f"   Step: {self.step_count}")
                    print(f"   Longueur cycle: {len(new_path)} actions")
                    print(f"   Séquence: {' → '.join(new_path)}")
                    cycle_found = True
                    return {
                        'cycle_found': True,
                        'cycle_length': len(new_path),
                        'cycle_path': new_path,
                        'total_steps': self.step_count
                    }
                
                # Nouvel état?
                if new_hash not in explored:
                    explored.add(new_hash)
                    self.states_visited[new_hash] = {
                        'step': self.step_count,
                        'frame': frame_after.copy(),
                        'action_taken': action_name,
                        'reward': reward,
                        'analysis': self.analyze_frame(frame_after),
                        'path': new_path
                    }
                    
                    if not done:
                        to_explore.append((new_hash, new_path))
                    
                    if self.step_count % 100 == 0:
                        print(f"   Step {self.step_count}: {len(explored)} états explorés, {len(to_explore)} en attente")
                
                # Vérifier si done
                if done:
                    print(f"\n⚠️ État terminal atteint (done=True) après {len(new_path)} actions")
                    print(f"   Séquence: {' → '.join(new_path)}")
        
        # Fin exploration
        if self.step_count >= self.max_steps:
            print("\n⚠️ LIMITE SÉCURITÉ ATTEINTE")
            print(f"   Max steps: {self.max_steps}")
            pass
        
        if not cycle_found:
            print("\n❌ AUCUN CYCLE TROUVÉ")
            print(f"   États explorés: {len(explored)}")
            print(f"   Transitions testées: {len(self.transitions)}")
        
        return {
            'cycle_found': False,
            'states_explored': len(explored),
            'total_steps': self.step_count
        }
    
    def save_results(self, result):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session86_cycle_search_{timestamp}.json"
        
        results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session86_search_cycles.py',
                'total_steps': self.step_count,
                'states_discovered': len(self.states_visited),
                'max_steps': self.max_steps
            },
            'result': result,
            'transitions': self.transitions[:1000]  # Limiter taille
        }
        
        filepath = Path(__file__).parent / filename
        with open(filepath, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename


def main():
    """Point d'entrée principal"""
    searcher = CycleSearcher()
    
    try:
        # Setup
        searcher.setup_environment("ls20-9607627b")
        
        # Recherche exhaustive
        result = searcher.search_cycle_exhaustive()
        
        # Sauvegarder
        filename = searcher.save_results(result)
        
        print("\n" + "="*80)
        print("✅ SESSION 86 COMPLÉTÉE")
        print("="*80)
        print(f"   Cycle trouvé: {result.get('cycle_found', False)}")
        print(f"   Total steps: {result.get('total_steps', 0)}")
        print(f"   États explorés: {result.get('states_explored', 0)}")
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
