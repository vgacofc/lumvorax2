#!/usr/bin/env python3
"""
SESSION 84 - COMPARAISON COMPLÈTE 52 PIXELS
Objectif: Vérifier si ACTION1/2/3/4 sont RÉELLEMENT identiques
Méthode: Comparer TOUS les 52 pixels modifiés, pas seulement value=11
Date: 2026-06-17
Auteur: Bob (basé sur feedback utilisateur critique)
"""

import numpy as np
import json
from datetime import datetime
from pathlib import Path
import sys

# Import ARC-AGI
from arc_agi import Arcade, OperationMode
from arcengine import GameAction


class Complete52PixelsComparator:
    """Comparer les 52 pixels complets de chaque action"""
    
    def __init__(self):
        self.env = None
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session84_compare_all_52_pixels.py',
                'objective': 'Vérifier si ACTION1/2/3/4 sont RÉELLEMENT identiques',
                'method': 'Comparer TOUS les 52 pixels modifiés'
            },
            'actions': {}
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
    
    def analyze_action_complete(self, action, action_name):
        """Analyser TOUS les pixels modifiés par une action"""
        print(f"\n{'='*80}")
        print(f"ANALYSE COMPLÈTE {action_name}")
        print(f"{'='*80}")
        
        # Reset
        obs = self.env.reset()
        frame_before = self.extract_frame(obs)
        
        if frame_before is None:
            print("❌ Frame before = None")
            return None
        
        # Exécuter action
        obs = self.env.step(action)
        frame_after = self.extract_frame(obs)
        
        if frame_after is None:
            print("❌ Frame after = None")
            return None
        
        # Différence complète
        diff = (frame_before != frame_after)
        pixels_changed = np.argwhere(diff)
        
        print(f"\n📊 Pixels modifiés: {len(pixels_changed)}")
        
        # Extraire TOUTES les transformations
        transformations = []
        for coord in pixels_changed:
            x, y = int(coord[0]), int(coord[1])
            val_before = int(frame_before[x, y])
            val_after = int(frame_after[x, y])
            transformations.append({
                'coord': (x, y),
                'value_before': val_before,
                'value_after': val_after
            })
        
        # Trier par coordonnées pour comparaison
        transformations.sort(key=lambda t: t['coord'])
        
        # Afficher premiers 10
        print(f"\n🔍 Premières 10 transformations:")
        for i, t in enumerate(transformations[:10]):
            print(f"  {i+1}. {t['coord']}: {t['value_before']} → {t['value_after']}")
        
        if len(transformations) > 10:
            print(f"  ... et {len(transformations)-10} autres")
        
        # Grouper par type de transformation
        transformation_types = {}
        for t in transformations:
            key = f"{t['value_before']}→{t['value_after']}"
            if key not in transformation_types:
                transformation_types[key] = []
            transformation_types[key].append(t['coord'])
        
        print(f"\n📈 Types de transformations:")
        for trans_type, coords in sorted(transformation_types.items()):
            print(f"  {trans_type}: {len(coords)} pixels")
        
        result = {
            'action': action_name,
            'pixels_changed': len(transformations),
            'transformations': transformations,
            'transformation_types': {k: len(v) for k, v in transformation_types.items()}
        }
        
        self.results['actions'][action_name] = result
        
        return result
    
    def compare_actions(self):
        """Comparer les transformations complètes des actions"""
        print("\n" + "="*80)
        print("COMPARAISON COMPLÈTE DES ACTIONS")
        print("="*80)
        
        actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
        
        # Vérifier que toutes les actions ont été analysées
        for action in actions:
            if action not in self.results['actions']:
                print(f"❌ {action} manquante")
                return
        
        # Comparer nombre de pixels
        print(f"\n📊 Nombre de pixels modifiés:")
        for action in actions:
            count = self.results['actions'][action]['pixels_changed']
            print(f"  {action}: {count} pixels")
        
        # Comparer types de transformations
        print(f"\n📈 Types de transformations par action:")
        for action in actions:
            types = self.results['actions'][action]['transformation_types']
            print(f"\n  {action}:")
            for trans_type, count in sorted(types.items()):
                print(f"    {trans_type}: {count} pixels")
        
        # Comparaison pixel par pixel ACTION1 vs ACTION2
        print(f"\n{'='*80}")
        print("COMPARAISON PIXEL PAR PIXEL: ACTION1 vs ACTION2")
        print(f"{'='*80}")
        
        trans1 = self.results['actions']['ACTION1']['transformations']
        trans2 = self.results['actions']['ACTION2']['transformations']
        
        # Créer sets de transformations complètes
        set1 = set((t['coord'], t['value_before'], t['value_after']) for t in trans1)
        set2 = set((t['coord'], t['value_before'], t['value_after']) for t in trans2)
        
        # Intersection et différences
        intersection = set1 & set2
        only_in_1 = set1 - set2
        only_in_2 = set2 - set1
        
        print(f"\n🔍 Intersection: {len(intersection)} transformations identiques")
        print(f"🔍 Uniquement dans ACTION1: {len(only_in_1)} transformations")
        print(f"🔍 Uniquement dans ACTION2: {len(only_in_2)} transformations")
        
        if len(only_in_1) > 0:
            print(f"\n  Transformations uniquement dans ACTION1 (max 5):")
            for i, t in enumerate(list(only_in_1)[:5]):
                print(f"    {t[0]}: {t[1]} → {t[2]}")
        
        if len(only_in_2) > 0:
            print(f"\n  Transformations uniquement dans ACTION2 (max 5):")
            for i, t in enumerate(list(only_in_2)[:5]):
                print(f"    {t[0]}: {t[1]} → {t[2]}")
        
        # Conclusion
        if len(set1) == len(set2) == len(intersection):
            print(f"\n✅ ACTION1 et ACTION2 sont IDENTIQUES (pixel par pixel)")
        else:
            print(f"\n❌ ACTION1 et ACTION2 sont DIFFÉRENTES")
        
        # Sauvegarder comparaison
        self.results['comparison'] = {
            'action1_vs_action2': {
                'intersection': len(intersection),
                'only_in_action1': len(only_in_1),
                'only_in_action2': len(only_in_2),
                'are_identical': len(set1) == len(set2) == len(intersection)
            }
        }
        
        # Comparer ACTION1 vs ACTION3 vs ACTION4
        print(f"\n{'='*80}")
        print("COMPARAISON: ACTION1 vs ACTION3 vs ACTION4")
        print(f"{'='*80}")
        
        trans3 = self.results['actions']['ACTION3']['transformations']
        trans4 = self.results['actions']['ACTION4']['transformations']
        
        set3 = set((t['coord'], t['value_before'], t['value_after']) for t in trans3)
        set4 = set((t['coord'], t['value_before'], t['value_after']) for t in trans4)
        
        intersection_all = set1 & set3 & set4
        union_all = set1 | set3 | set4
        
        print(f"\n🔍 Intersection ACTION1 ∩ ACTION3 ∩ ACTION4: {len(intersection_all)}")
        print(f"🔍 Union ACTION1 ∪ ACTION3 ∪ ACTION4: {len(union_all)}")
        
        if len(intersection_all) == len(union_all):
            print(f"\n✅ ACTION1, ACTION3 et ACTION4 sont IDENTIQUES")
        else:
            print(f"\n❌ ACTION1, ACTION3 et ACTION4 sont DIFFÉRENTES")
        
        self.results['comparison']['action1_vs_action3_vs_action4'] = {
            'intersection': len(intersection_all),
            'union': len(union_all),
            'are_identical': len(intersection_all) == len(union_all)
        }
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session84_complete_comparison_{timestamp}.json"
        filepath = Path(__file__).parent / filename
        
        with open(filepath, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename


def main():
    """Fonction principale"""
    print("\n" + "="*80)
    print("SESSION 84 - COMPARAISON COMPLÈTE 52 PIXELS")
    print("="*80)
    print("\nObjectif: Vérifier si ACTION1/2/3/4 sont RÉELLEMENT identiques")
    print("Méthode: Comparer TOUS les 52 pixels modifiés (pas seulement value=11)")
    
    comparator = Complete52PixelsComparator()
    
    # Setup
    if not comparator.setup_environment():
        return 1
    
    # Analyser chaque action
    for action, action_name in [
        (GameAction.ACTION1, "ACTION1"),
        (GameAction.ACTION2, "ACTION2"),
        (GameAction.ACTION3, "ACTION3"),
        (GameAction.ACTION4, "ACTION4")
    ]:
        comparator.analyze_action_complete(action, action_name)
    
    # Comparer
    comparator.compare_actions()
    
    # Sauvegarder
    comparator.save_results()
    
    print("\n" + "="*80)
    print("✅ SESSION 84 COMPLÉTÉE")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
