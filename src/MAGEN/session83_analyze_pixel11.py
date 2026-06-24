#!/usr/bin/env python3
"""
SESSION 83 - ANALYSE PIXEL VALUE 11
Questions critiques basées sur feedback utilisateur:
1. Nombre total de pixels value=11 dans frame initiale
2. Quels pixels 11 changent avec ACTION1/3/4
3. Intersection entre ces ensembles
Date: 2026-06-17
Auteur: Bob
"""

import numpy as np
import json
from datetime import datetime
from pathlib import Path
import sys

# Import ARC-AGI
from arc_agi import Arcade, OperationMode
from arcengine import GameAction


class Pixel11Analyzer:
    """Analyser rôle pixel value 11"""
    
    def __init__(self):
        self.env = None
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session83_analyze_pixel11.py',
                'questions': [
                    'Nombre total pixels value=11 dans frame initiale',
                    'Quels pixels 11 changent avec ACTION1',
                    'Quels pixels 11 changent avec ACTION3',
                    'Quels pixels 11 changent avec ACTION4',
                    'Intersection entre ces ensembles'
                ]
            },
            'analysis': {}
        }
    
    def setup_environment(self, game_id: str = "ls20-9607627b"):
        """Initialiser environnement - JEU OBLIGATOIRE: ls20-9607627b"""
        print("\n" + "="*80)
        print("INITIALISATION ENVIRONNEMENT")
        print("="*80)
        
        # VÉRIFICATION GAME ID (RÈGLE #14)
        REQUIRED_GAME_ID = "ls20-9607627b"
        if game_id != REQUIRED_GAME_ID:
            raise ValueError(f"❌ ERREUR: Jeu {game_id} ≠ {REQUIRED_GAME_ID}")
        
        print(f"✅ Game ID vérifié: {game_id}")
        
        try:
            arcade = Arcade(operation_mode=OperationMode.COMPETITION)
            self.env = arcade.make(game_id)
            print(f"✅ Environnement créé pour {game_id}")
            
            obs = self.env.reset()
            frame = self.extract_frame(obs)
            if frame is not None:
                print(f"✅ Frame shape: {frame.shape}")
                print(f"✅ Pixel values: {np.unique(frame)}")
            
            return True
            
        except Exception as e:
            print(f"❌ ERREUR: {e}")
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
    
    def analyze_initial_frame(self):
        """Analyser frame initiale"""
        print("\n" + "="*80)
        print("ANALYSE FRAME INITIALE")
        print("="*80)
        
        obs = self.env.reset()
        frame = self.extract_frame(obs)
        
        if frame is None:
            print("❌ Frame = None")
            return None
        
        # Compter chaque pixel value
        unique_values, counts = np.unique(frame, return_counts=True)
        
        print("\n📊 Distribution pixel values:")
        for val, count in zip(unique_values, counts):
            percentage = (count / frame.size) * 100
            print(f"  Value {val:2d}: {count:4d} pixels ({percentage:5.2f}%)")
        
        # Focus sur pixel 11
        pixels_11 = np.argwhere(frame == 11)
        print(f"\n🔍 Pixels value=11:")
        print(f"  Total: {len(pixels_11)} pixels")
        print(f"  Pourcentage: {(len(pixels_11)/frame.size)*100:.2f}%")
        
        if len(pixels_11) <= 20:
            print(f"  Coordonnées: {pixels_11.tolist()}")
        else:
            print(f"  Premières 20: {pixels_11[:20].tolist()}")
        
        self.results['analysis']['initial_frame'] = {
            'shape': frame.shape,
            'total_pixels': int(frame.size),
            'distribution': {int(val): int(count) for val, count in zip(unique_values, counts)},
            'pixels_11': {
                'count': int(len(pixels_11)),
                'percentage': float((len(pixels_11)/frame.size)*100),
                'coordinates': pixels_11.tolist()
            }
        }
        
        return frame, pixels_11
    
    def analyze_action_on_pixel11(self, action, action_name):
        """Analyser effet d'une action sur pixels value=11"""
        print(f"\n{'='*80}")
        print(f"ANALYSE {action_name} SUR PIXELS 11")
        print(f"{'='*80}")
        
        # Reset
        obs = self.env.reset()
        frame_before = self.extract_frame(obs)
        
        if frame_before is None:
            print("❌ Frame before = None")
            return None
        
        # Identifier pixels 11 avant
        pixels_11_before = np.argwhere(frame_before == 11)
        print(f"\n📍 Pixels 11 avant {action_name}: {len(pixels_11_before)}")
        
        # Exécuter action
        obs = self.env.step(action)
        frame_after = self.extract_frame(obs)
        
        if frame_after is None:
            print("❌ Frame after = None")
            return None
        
        # Identifier pixels 11 après
        pixels_11_after = np.argwhere(frame_after == 11)
        print(f"📍 Pixels 11 après {action_name}: {len(pixels_11_after)}")
        
        # Différence
        diff = (frame_before != frame_after)
        pixels_changed = np.argwhere(diff)
        print(f"📍 Pixels modifiés: {len(pixels_changed)}")
        
        # Quels pixels 11 ont changé?
        pixels_11_changed = []
        for coord in pixels_11_before:
            x, y = int(coord[0]), int(coord[1])
            if frame_before[x, y] != frame_after[x, y]:
                val_before = int(frame_before[x, y])
                val_after = int(frame_after[x, y])
                pixels_11_changed.append({
                    'coord': (x, y),
                    'value_before': val_before,
                    'value_after': val_after
                })
        
        print(f"\n🔍 Pixels 11 qui ont changé: {len(pixels_11_changed)}")
        if len(pixels_11_changed) <= 10:
            for p in pixels_11_changed:
                print(f"  {p['coord']}: {p['value_before']} → {p['value_after']}")
        else:
            for p in pixels_11_changed[:10]:
                print(f"  {p['coord']}: {p['value_before']} → {p['value_after']}")
            print(f"  ... et {len(pixels_11_changed)-10} autres")
        
        result = {
            'action': action_name,
            'pixels_11_before': int(len(pixels_11_before)),
            'pixels_11_after': int(len(pixels_11_after)),
            'pixels_changed_total': int(len(pixels_changed)),
            'pixels_11_changed': len(pixels_11_changed),
            'pixels_11_changed_coords': [p['coord'] for p in pixels_11_changed],
            'transformations': pixels_11_changed
        }
        
        self.results['analysis'][action_name] = result
        
        return result
    
    def compare_actions(self):
        """Comparer effets des actions sur pixels 11"""
        print("\n" + "="*80)
        print("COMPARAISON ACTIONS")
        print("="*80)
        
        if 'ACTION1' not in self.results['analysis']:
            print("❌ Données manquantes")
            return
        
        # Extraire coordonnées pixels 11 modifiés
        coords_action1 = set(map(tuple, self.results['analysis']['ACTION1']['pixels_11_changed_coords']))
        coords_action3 = set(map(tuple, self.results['analysis']['ACTION3']['pixels_11_changed_coords']))
        coords_action4 = set(map(tuple, self.results['analysis']['ACTION4']['pixels_11_changed_coords']))
        
        print(f"\n📊 Pixels 11 modifiés:")
        print(f"  ACTION1: {len(coords_action1)} pixels")
        print(f"  ACTION3: {len(coords_action3)} pixels")
        print(f"  ACTION4: {len(coords_action4)} pixels")
        
        # Intersection
        intersection_all = coords_action1 & coords_action3 & coords_action4
        intersection_1_3 = coords_action1 & coords_action3
        intersection_1_4 = coords_action1 & coords_action4
        intersection_3_4 = coords_action3 & coords_action4
        
        print(f"\n🔍 Intersections:")
        print(f"  ACTION1 ∩ ACTION3: {len(intersection_1_3)} pixels")
        print(f"  ACTION1 ∩ ACTION4: {len(intersection_1_4)} pixels")
        print(f"  ACTION3 ∩ ACTION4: {len(intersection_3_4)} pixels")
        print(f"  ACTION1 ∩ ACTION3 ∩ ACTION4: {len(intersection_all)} pixels")
        
        # Union
        union_all = coords_action1 | coords_action3 | coords_action4
        print(f"\n  Union (ACTION1 ∪ ACTION3 ∪ ACTION4): {len(union_all)} pixels")
        
        # Pixels uniques à chaque action
        unique_1 = coords_action1 - coords_action3 - coords_action4
        unique_3 = coords_action3 - coords_action1 - coords_action4
        unique_4 = coords_action4 - coords_action1 - coords_action3
        
        print(f"\n  Uniques ACTION1: {len(unique_1)} pixels")
        print(f"  Uniques ACTION3: {len(unique_3)} pixels")
        print(f"  Uniques ACTION4: {len(unique_4)} pixels")
        
        self.results['analysis']['comparison'] = {
            'intersection_1_3': len(intersection_1_3),
            'intersection_1_4': len(intersection_1_4),
            'intersection_3_4': len(intersection_3_4),
            'intersection_all': len(intersection_all),
            'union_all': len(union_all),
            'unique_action1': len(unique_1),
            'unique_action3': len(unique_3),
            'unique_action4': len(unique_4)
        }
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session83_pixel11_analysis_{timestamp}.json"
        filepath = Path(__file__).parent / filename
        
        with open(filepath, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename


def main():
    """Fonction principale"""
    print("\n" + "="*80)
    print("SESSION 83 - ANALYSE PIXEL VALUE 11")
    print("="*80)
    
    analyzer = Pixel11Analyzer()
    
    # Setup
    if not analyzer.setup_environment():
        return 1
    
    # Analyse frame initiale
    result_initial = analyzer.analyze_initial_frame()
    
    # Analyser chaque action
    for action, action_name in [
        (GameAction.ACTION1, "ACTION1"),
        (GameAction.ACTION3, "ACTION3"),
        (GameAction.ACTION4, "ACTION4")
    ]:
        analyzer.analyze_action_on_pixel11(action, action_name)
    
    # Comparer
    analyzer.compare_actions()
    
    # Sauvegarder
    analyzer.save_results()
    
    print("\n" + "="*80)
    print("✅ SESSION 83 COMPLÉTÉE")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
