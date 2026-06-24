#!/usr/bin/env python3
"""
PHASE 4.4.1B - ANALYSE PROFONDE ACTION2
========================================

Test exhaustif ACTION2 avec enregistrement COMPLET:
- Observation pixels
- Reward
- Done
- Info dict
- État interne jeu
- 60 répétitions pour détecter patterns

Objectif: Comprendre ce que ACTION2 modifie RÉELLEMENT

Author: MAGEN Team
Date: 2026-06-19
Version: 2.0 (Deep Analysis)
"""

import sys
import os
import json
import numpy as np
from collections import defaultdict
from datetime import datetime
import hashlib

# Setup paths
ls20_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../environment_files/ls20/9607627b'))
if ls20_path not in sys.path:
    sys.path.insert(0, ls20_path)

try:
    import ls20
    from arcengine import GameAction
    print(f"✅ Module ls20 importé depuis: {ls20.__file__}")
except ImportError as e:
    print(f"❌ Erreur import ls20: {e}")
    sys.exit(1)


class ACTION2DeepAnalyzer:
    """Analyseur profond ACTION2 avec enregistrement complet"""
    
    def __init__(self, num_repetitions=60):
        self.num_repetitions = num_repetitions
        self.game = None
        self.observations = []
        
        print("="*70)
        print("PHASE 4.4.1B - ANALYSE PROFONDE ACTION2")
        print("="*70)
        print(f"Répétitions: {num_repetitions}")
        print(f"Enregistrement: observation + reward + done + info + état interne")
        print("="*70)
        print()
    
    def initialize_game(self):
        """Initialise le jeu"""
        print("🎮 Initialisation jeu...")
        self.game = ls20.Ls20()
        self.game.set_level(0)  # Niveau 1
        print("   ✅ Jeu initialisé")
    
    def hash_state(self, state):
        """Hash d'un état"""
        if isinstance(state, np.ndarray):
            return hashlib.sha256(state.tobytes()).hexdigest()[:16]
        else:
            # Surface pygame
            import pygame
            arr = pygame.surfarray.array3d(state)
            return hashlib.sha256(arr.tobytes()).hexdigest()[:16]
    
    def extract_game_internals(self):
        """Extrait état interne du jeu"""
        internals = {}
        
        # Attributs du jeu
        for attr in dir(self.game):
            if not attr.startswith('_'):
                try:
                    value = getattr(self.game, attr)
                    if not callable(value):
                        # Convertir en type sérialisable
                        if isinstance(value, (int, float, str, bool)):
                            internals[attr] = value
                        elif isinstance(value, (list, tuple)):
                            internals[attr] = str(value)[:100]
                        else:
                            internals[attr] = str(type(value))
                except:
                    pass
        
        # Niveau actuel
        if hasattr(self.game, 'current_level'):
            level = self.game.current_level
            internals['level_info'] = {}
            for attr in dir(level):
                if not attr.startswith('_'):
                    try:
                        value = getattr(level, attr)
                        if not callable(value):
                            if isinstance(value, (int, float, str, bool)):
                                internals['level_info'][attr] = value
                    except:
                        pass
        
        return internals
    
    def execute_action2_with_full_logging(self, step_num):
        """Exécute ACTION2 et enregistre TOUT"""
        print(f"\n🔍 Step {step_num}/{self.num_repetitions}")
        
        # État AVANT
        state_before = self.game.camera.render(self.game.current_level._sprites)
        hash_before = self.hash_state(state_before)
        internals_before = self.extract_game_internals()
        
        print(f"   État avant: {hash_before}")
        
        # Exécuter ACTION2
        class ActionInput:
            def __init__(self, action_id):
                self.id = action_id
        
        game_action_enum = GameAction.ACTION2
        action_input = ActionInput(game_action_enum)
        
        self.game._set_action(action_input)
        
        # Exécuter steps
        steps_taken = 0
        max_steps = 100
        while not self.game.is_action_complete() and steps_taken < max_steps:
            self.game.step()
            steps_taken += 1
        
        # État APRÈS
        state_after = self.game.camera.render(self.game.current_level._sprites)
        hash_after = self.hash_state(state_after)
        internals_after = self.extract_game_internals()
        
        # Calculer différences
        pixel_diff = 0
        if isinstance(state_before, np.ndarray) and isinstance(state_after, np.ndarray):
            pixel_diff = np.sum(np.abs(state_after.astype(float) - state_before.astype(float)))
        else:
            import pygame
            arr_before = pygame.surfarray.array3d(state_before)
            arr_after = pygame.surfarray.array3d(state_after)
            pixel_diff = np.sum(np.abs(arr_after.astype(float) - arr_before.astype(float)))
        
        # Différences internes
        internal_diffs = {}
        for key in internals_before:
            if key in internals_after:
                if internals_before[key] != internals_after[key]:
                    internal_diffs[key] = {
                        'before': internals_before[key],
                        'after': internals_after[key]
                    }
        
        print(f"   État après: {hash_after}")
        print(f"   Steps pris: {steps_taken}")
        print(f"   Diff pixels: {pixel_diff:.0f}")
        print(f"   Diff internes: {len(internal_diffs)} attributs changés")
        
        if internal_diffs:
            print(f"   Changements:")
            for key, diff in list(internal_diffs.items())[:5]:
                print(f"      {key}: {diff['before']} → {diff['after']}")
        
        # Enregistrer observation
        observation = {
            'step_num': step_num,
            'hash_before': hash_before,
            'hash_after': hash_after,
            'steps_taken': steps_taken,
            'pixel_diff': float(pixel_diff),
            'state_changed': hash_before != hash_after,
            'internals_before': internals_before,
            'internals_after': internals_after,
            'internal_diffs': internal_diffs,
            'action_complete': self.game.is_action_complete()
        }
        
        self.observations.append(observation)
        return observation
    
    def run_analysis(self):
        """Exécute analyse complète"""
        print("\n🚀 Démarrage analyse profonde...")
        
        # Initialiser
        self.initialize_game()
        
        # Exécuter ACTION2 × 60
        for i in range(1, self.num_repetitions + 1):
            self.execute_action2_with_full_logging(i)
        
        # Analyser résultats
        self.analyze_results()
        
        # Sauvegarder
        self.save_results()
    
    def analyze_results(self):
        """Analyse résultats"""
        print("\n" + "="*70)
        print("📊 ANALYSE RÉSULTATS")
        print("="*70)
        
        # États uniques
        hashes_after = [obs['hash_after'] for obs in self.observations]
        unique_hashes = set(hashes_after)
        
        print(f"\n🔢 ÉTATS:")
        print(f"   Observations: {len(self.observations)}")
        print(f"   États uniques: {len(unique_hashes)}")
        print(f"   Ratio: {len(unique_hashes)/len(self.observations)*100:.1f}%")
        
        # Changements d'état
        state_changes = sum(1 for obs in self.observations if obs['state_changed'])
        print(f"\n🔄 CHANGEMENTS:")
        print(f"   États changés: {state_changes}/{len(self.observations)} ({state_changes/len(self.observations)*100:.1f}%)")
        
        # Différences pixels
        pixel_diffs = [obs['pixel_diff'] for obs in self.observations]
        print(f"\n📊 PIXELS:")
        print(f"   Diff moyenne: {np.mean(pixel_diffs):.2f}")
        print(f"   Diff max: {np.max(pixel_diffs):.2f}")
        print(f"   Diff min: {np.min(pixel_diffs):.2f}")
        
        # Attributs internes changés
        all_changed_attrs = set()
        for obs in self.observations:
            all_changed_attrs.update(obs['internal_diffs'].keys())
        
        print(f"\n🔧 ATTRIBUTS INTERNES:")
        print(f"   Attributs changés: {len(all_changed_attrs)}")
        if all_changed_attrs:
            print(f"   Liste:")
            for attr in sorted(all_changed_attrs):
                count = sum(1 for obs in self.observations if attr in obs['internal_diffs'])
                print(f"      {attr}: {count}/{len(self.observations)} fois")
        
        # Steps pris
        steps_taken = [obs['steps_taken'] for obs in self.observations]
        print(f"\n⏱️  STEPS:")
        print(f"   Steps moyen: {np.mean(steps_taken):.2f}")
        print(f"   Steps max: {np.max(steps_taken)}")
        print(f"   Steps min: {np.min(steps_taken)}")
    
    def save_results(self):
        """Sauvegarde résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"action2_deep_analysis_{timestamp}.json"
        
        # Préparer données
        data = {
            'timestamp': timestamp,
            'num_repetitions': self.num_repetitions,
            'observations': self.observations,
            'summary': {
                'unique_states': len(set(obs['hash_after'] for obs in self.observations)),
                'state_changes': sum(1 for obs in self.observations if obs['state_changed']),
                'avg_pixel_diff': float(np.mean([obs['pixel_diff'] for obs in self.observations])),
                'changed_attributes': list(set(
                    attr for obs in self.observations for attr in obs['internal_diffs'].keys()
                ))
            }
        }
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"\n💾 Résultats sauvegardés: {filename}")
        print("="*70)


def main():
    """Point d'entrée"""
    analyzer = ACTION2DeepAnalyzer(num_repetitions=60)
    analyzer.run_analysis()


if __name__ == "__main__":
    main()

# Made with Bob
