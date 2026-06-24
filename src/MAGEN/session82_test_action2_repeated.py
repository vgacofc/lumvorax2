#!/usr/bin/env python3
"""
SESSION 82 - TEST ACTION2 RÉPÉTÉE
Question critique: Que deviennent les pixels après plusieurs ACTION2?
Date: 2026-06-17
Auteur: Bob (basé sur feedback utilisateur + analyse forensique)
"""

import numpy as np
import json
from datetime import datetime
from pathlib import Path
import sys

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction


class Action2RepeatedTester:
    """Tester comportement ACTION2 répétée"""
    
    def __init__(self):
        self.env = None
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session82_test_action2_repeated.py',
                'question': 'Que deviennent pixels après plusieurs ACTION2?',
                'hypotheses': [
                    'Toggle: 11 ↔ 3 (interrupteur)',
                    'One-way: 11 → 3 (permanent)',
                    'Cycle: 11 → 3 → X → 11',
                    'Cascade: Autre région change'
                ]
            },
            'tests': []
        }
    
    def setup_environment(self, game_id: str = "ls20-9607627b"):
        """Initialiser environnement - JEU OBLIGATOIRE: ls20-9607627b"""
        print("\n" + "="*80)
        print("INITIALISATION ENVIRONNEMENT")
        print("="*80)
        
        # VÉRIFICATION GAME ID (RÈGLE #14)
        REQUIRED_GAME_ID = "ls20-9607627b"
        if game_id != REQUIRED_GAME_ID:
            raise ValueError(
                f"❌ ERREUR CRITIQUE: Jeu {game_id} ≠ {REQUIRED_GAME_ID}\n"
                f"INTERDICTION de changer de jeu sans validation complète!"
            )
        
        print(f"✅ Game ID vérifié: {game_id}")
        
        try:
            # Nouvelle API
            arcade = Arcade(operation_mode=OperationMode.COMPETITION)
            self.env = arcade.make(game_id)
            print(f"✅ Environnement créé pour {game_id}")
            
            # Test reset
            obs = self.env.reset()
            frame = self.extract_frame(obs)
            if frame is not None:
                print(f"✅ Frame shape: {frame.shape}")
                print(f"✅ Pixel values: {np.unique(frame)}")
            
            return True
            
        except Exception as e:
            print(f"❌ ERREUR initialisation: {e}")
            import traceback
            traceback.print_exc()
            return False
    
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
    
    def test_action2_repeated(self, num_repetitions=10):
        """Tester ACTION2 répétée plusieurs fois"""
        print("\n" + "="*80)
        print(f"TEST ACTION2 RÉPÉTÉE ({num_repetitions} fois)")
        print("="*80)
        
        # Coordonnées ACTION2 connues (de Session 81)
        TARGET_COORDS = [(61, 13), (62, 13)]
        
        # Reset
        obs = self.env.reset()
        frame = self.extract_frame(obs)
        
        if frame is None:
            print("❌ Frame = None")
            return None
        
        # Enregistrer état initial
        initial_values = [frame[x, y] for x, y in TARGET_COORDS]
        print(f"\n📍 Coordonnées surveillées: {TARGET_COORDS}")
        print(f"📊 Valeurs initiales: {initial_values}")
        
        # Tester répétitions
        history = [{
            'step': 0,
            'action': 'RESET',
            'coords_values': initial_values,
            'frame_hash': hash(frame.tobytes())
        }]
        
        for i in range(num_repetitions):
            # Exécuter ACTION2
            obs = self.env.step(GameAction.ACTION2)
            frame = self.extract_frame(obs)
            
            if frame is None:
                print(f"❌ Step {i+1}: Frame = None")
                break
            
            # Lire valeurs
            current_values = [frame[x, y] for x, y in TARGET_COORDS]
            frame_hash = hash(frame.tobytes())
            
            history.append({
                'step': i + 1,
                'action': 'ACTION2',
                'coords_values': current_values,
                'frame_hash': frame_hash
            })
            
            print(f"\nStep {i+1}:")
            print(f"  Valeurs: {current_values}")
            
            # Détecter pattern
            if i > 0:
                prev_values = history[i]['coords_values']
                if current_values == prev_values:
                    print(f"  ⚠️ STABLE (pas de changement)")
                elif current_values == initial_values:
                    print(f"  🔄 RETOUR état initial (toggle détecté!)")
                else:
                    print(f"  ➡️ NOUVEAU état: {prev_values} → {current_values}")
        
        # Analyser pattern
        print("\n" + "="*80)
        print("ANALYSE PATTERN")
        print("="*80)
        
        values_sequence = [h['coords_values'] for h in history]
        unique_states = []
        for v in values_sequence:
            if v not in unique_states:
                unique_states.append(v)
        
        print(f"\nÉtats uniques: {len(unique_states)}")
        for i, state in enumerate(unique_states):
            print(f"  État {i}: {state}")
        
        # Détecter type pattern
        if len(unique_states) == 1:
            pattern_type = "AUCUN EFFET (stable)"
        elif len(unique_states) == 2:
            # Vérifier toggle
            if values_sequence[0] == values_sequence[2]:
                pattern_type = "TOGGLE (A ↔ B)"
            else:
                pattern_type = "ONE-WAY (A → B permanent)"
        else:
            pattern_type = f"CYCLE ({len(unique_states)} états)"
        
        print(f"\n🔍 Pattern détecté: {pattern_type}")
        
        # Sauvegarder résultats
        test_result = {
            'num_repetitions': num_repetitions,
            'target_coords': TARGET_COORDS,
            'history': history,
            'unique_states': unique_states,
            'pattern_type': pattern_type
        }
        
        self.results['tests'].append(test_result)
        
        return test_result
    
    def test_search_moving_pixels(self, num_steps=20):
        """Chercher pixels qui se déplacent réellement"""
        print("\n" + "="*80)
        print(f"RECHERCHE PIXELS MOBILES ({num_steps} steps)")
        print("="*80)
        
        # Reset
        obs = self.env.reset()
        frame_prev = self.extract_frame(obs)
        
        if frame_prev is None:
            print("❌ Frame = None")
            return None
        
        moving_pixels_detected = []
        
        # Tester chaque action
        for action_enum, action_name in [
            (GameAction.ACTION1, "ACTION1"),
            (GameAction.ACTION2, "ACTION2"),
            (GameAction.ACTION3, "ACTION3"),
            (GameAction.ACTION4, "ACTION4")
        ]:
            print(f"\n{'='*80}")
            print(f"TEST {action_name}")
            print(f"{'='*80}")
            
            # Reset pour chaque action
            obs = self.env.reset()
            frame_prev = self.extract_frame(obs)
            
            positions_history = []
            
            for step in range(num_steps):
                # Exécuter action
                obs = self.env.step(action_enum)
                frame_curr = self.extract_frame(obs)
                
                if frame_curr is None:
                    break
                
                # Détecter changements
                diff = (frame_prev != frame_curr)
                changed_coords = np.argwhere(diff)
                
                if len(changed_coords) > 0:
                    # Analyser si déplacement ou transformation
                    # Déplacement = pixel disparaît à position A, apparaît à position B
                    # Transformation = pixel change de valeur à même position
                    
                    # Pour chaque pixel qui a changé
                    for coord in changed_coords:
                        x, y = int(coord[0]), int(coord[1])
                        val_prev = frame_prev[x, y]
                        val_curr = frame_curr[x, y]
                        
                        positions_history.append({
                            'step': step + 1,
                            'coord': (int(x), int(y)),
                            'value_before': int(val_prev.item()) if hasattr(val_prev, 'item') else int(val_prev),
                            'value_after': int(val_curr.item()) if hasattr(val_curr, 'item') else int(val_curr)
                        })
                
                frame_prev = frame_curr.copy()
            
            # Analyser si mouvement détecté
            if len(positions_history) > 0:
                # Chercher pattern déplacement
                coords_changed = [p['coord'] for p in positions_history]
                unique_coords = set(coords_changed)
                
                print(f"\n  Pixels modifiés: {len(coords_changed)} changements")
                print(f"  Positions uniques: {len(unique_coords)}")
                
                if len(unique_coords) > 1:
                    print(f"  ✅ MOUVEMENT POTENTIEL détecté!")
                    print(f"  Positions: {list(unique_coords)[:5]}...")
                    moving_pixels_detected.append({
                        'action': action_name,
                        'positions': list(unique_coords),
                        'history': positions_history[:10]  # Premiers 10
                    })
                else:
                    print(f"  ❌ Pas de mouvement (même position)")
            else:
                print(f"  ❌ Aucun changement détecté")
        
        return moving_pixels_detected
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session82_action2_repeated_{timestamp}.json"
        filepath = Path(__file__).parent / filename
        
        with open(filepath, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename


def main():
    """Fonction principale"""
    print("\n" + "="*80)
    print("SESSION 82 - TEST ACTION2 RÉPÉTÉE")
    print("="*80)
    print("\nQuestions critiques:")
    print("1. Que deviennent pixels après plusieurs ACTION2?")
    print("2. Y a-t-il des pixels qui se déplacent réellement?")
    print("3. ACTION2 est-elle un toggle ou one-way?")
    
    tester = Action2RepeatedTester()
    
    # Setup
    if not tester.setup_environment():
        print("\n❌ Échec initialisation")
        return 1
    
    # Test 1: ACTION2 répétée
    print("\n" + "="*80)
    print("TEST 1: ACTION2 RÉPÉTÉE")
    print("="*80)
    result1 = tester.test_action2_repeated(num_repetitions=10)
    
    # Test 2: Chercher pixels mobiles
    print("\n" + "="*80)
    print("TEST 2: RECHERCHE PIXELS MOBILES")
    print("="*80)
    result2 = tester.test_search_moving_pixels(num_steps=20)
    
    # Sauvegarder
    tester.save_results()
    
    print("\n" + "="*80)
    print("✅ SESSION 82 COMPLÉTÉE")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
