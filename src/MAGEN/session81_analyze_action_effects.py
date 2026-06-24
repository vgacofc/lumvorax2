#!/usr/bin/env python3
"""
SESSION 81 - ANALYSE EFFETS ACTIONS
Révolution méthodologique: Ne pas chercher avatar, chercher pixels modifiés
Date: 2026-06-17
Auteur: Bob (basé sur analyse révolutionnaire utilisateur)
"""

import numpy as np
from scipy.ndimage import label
import json
from datetime import datetime
from pathlib import Path
import sys

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction


class ActionEffectsAnalyzer:
    """Analyse les effets de chaque action sur la frame"""
    
    def __init__(self):
        self.env = None
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'script': 'session81_analyze_action_effects.py',
                'paradigm': 'Analyse différentielle (pas recherche avatar)'
            },
            'actions': {}
        }
    
    def setup_environment(self, game_id: str = "ls20-9607627b"):
        """Initialiser environnement ARC-AGI avec nouvelle API - JEU OBLIGATOIRE: ls20-9607627b"""
        print("\n" + "="*80)
        print("INITIALISATION ENVIRONNEMENT")
        print("="*80)
        
        try:
            # Nouvelle API: Arcade() puis arcade.make(game_id)
            arcade = Arcade(operation_mode=OperationMode.COMPETITION)
            print(f"✅ Arcade créé")
            
            # Créer environnement pour jeu spécifique
            self.env = arcade.make(game_id)
            print(f"✅ Environnement créé pour {game_id}")
            
            # Test reset
            obs = self.env.reset()
            frame = self.extract_frame(obs)
            if frame is not None:
                print(f"✅ Frame shape: {frame.shape}")
                print(f"✅ Pixel values: {np.unique(frame)}")
            else:
                print("⚠️ Frame extraction retourne None")
            
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
    
    def analyze_single_action(self, action, action_name, num_tests=20):
        """Analyser effet d'une action sur la frame"""
        print(f"\n{'='*80}")
        print(f"ANALYSE {action_name}")
        print(f"{'='*80}")
        
        tests_results = []
        
        for test_num in range(num_tests):
            try:
                # Reset
                obs_before = self.env.reset()
                frame_before = self.extract_frame(obs_before)
                
                if frame_before is None:
                    print(f"❌ Test {test_num+1}: Frame before = None")
                    continue
                
                # Action
                obs_after = self.env.step(action)
                frame_after = self.extract_frame(obs_after)
                
                if frame_after is None:
                    print(f"❌ Test {test_num+1}: Frame after = None")
                    continue
                
                # Différence
                diff = (frame_before != frame_after)
                pixels_modified = int(np.sum(diff))
                
                # Composantes connexes
                labeled, num_components = label(diff)
                
                # Coordonnées modifiées
                coords_modified = np.argwhere(diff).tolist()
                
                # Valeurs changées
                if pixels_modified > 0:
                    values_before = frame_before[diff].tolist()
                    values_after = frame_after[diff].tolist()
                else:
                    values_before = []
                    values_after = []
                
                # Analyser composantes
                components_info = []
                for comp_id in range(1, num_components + 1):
                    comp_mask = (labeled == comp_id)
                    comp_size = int(np.sum(comp_mask))
                    comp_coords = np.argwhere(comp_mask).tolist()
                    
                    # Bounding box
                    if len(comp_coords) > 0:
                        coords_array = np.array(comp_coords)
                        min_y, min_x = coords_array.min(axis=0).tolist()
                        max_y, max_x = coords_array.max(axis=0).tolist()
                        
                        components_info.append({
                            'id': comp_id,
                            'size': comp_size,
                            'bounding_box': {
                                'min_y': min_y,
                                'min_x': min_x,
                                'max_y': max_y,
                                'max_x': max_x,
                                'width': max_x - min_x + 1,
                                'height': max_y - min_y + 1
                            }
                        })
                
                test_result = {
                    'test_num': test_num + 1,
                    'pixels_modified': pixels_modified,
                    'num_components': int(num_components),
                    'coords_modified': coords_modified,
                    'values_before': values_before,
                    'values_after': values_after,
                    'components': components_info
                }
                
                tests_results.append(test_result)
                
                # Affichage
                if test_num == 0:
                    print(f"\n📊 Test 1 (exemple détaillé):")
                    print(f"   Pixels modifiés: {pixels_modified}")
                    print(f"   Composantes connexes: {num_components}")
                    if pixels_modified > 0 and pixels_modified <= 10:
                        print(f"   Coordonnées: {coords_modified}")
                        print(f"   Valeurs avant: {values_before}")
                        print(f"   Valeurs après: {values_after}")
                    elif pixels_modified > 10:
                        print(f"   (Trop de pixels pour afficher détails)")
                
                if (test_num + 1) % 5 == 0:
                    print(f"   ✅ Tests {test_num-3}-{test_num+1} complétés")
                
            except Exception as e:
                print(f"❌ Test {test_num+1}: Erreur {e}")
                continue
        
        # Statistiques globales
        if tests_results:
            pixels_modified_list = [r['pixels_modified'] for r in tests_results]
            components_list = [r['num_components'] for r in tests_results]
            
            stats = {
                'num_tests': len(tests_results),
                'pixels_modified': {
                    'min': int(np.min(pixels_modified_list)),
                    'max': int(np.max(pixels_modified_list)),
                    'mean': float(np.mean(pixels_modified_list)),
                    'std': float(np.std(pixels_modified_list)),
                    'median': float(np.median(pixels_modified_list))
                },
                'num_components': {
                    'min': int(np.min(components_list)),
                    'max': int(np.max(components_list)),
                    'mean': float(np.mean(components_list)),
                    'median': float(np.median(components_list))
                }
            }
            
            print(f"\n📈 STATISTIQUES {action_name}:")
            print(f"   Tests réussis: {stats['num_tests']}/{num_tests}")
            print(f"   Pixels modifiés: {stats['pixels_modified']['min']}-{stats['pixels_modified']['max']} (moy: {stats['pixels_modified']['mean']:.1f})")
            print(f"   Composantes: {stats['num_components']['min']}-{stats['num_components']['max']} (moy: {stats['num_components']['mean']:.1f})")
            
            return {
                'action': action_name,
                'stats': stats,
                'tests': tests_results
            }
        
        return None
    
    def analyze_all_actions(self, num_tests=20):
        """Analyser toutes les actions"""
        print("\n" + "="*80)
        print("ANALYSE TOUTES ACTIONS")
        print("="*80)
        
        actions = [
            (GameAction.ACTION1, "ACTION1"),
            (GameAction.ACTION2, "ACTION2"),
            (GameAction.ACTION3, "ACTION3"),
            (GameAction.ACTION4, "ACTION4")
        ]
        
        for action, action_name in actions:
            result = self.analyze_single_action(action, action_name, num_tests)
            if result:
                self.results['actions'][action_name] = result
        
        return self.results
    
    def create_comparison_matrix(self):
        """Créer matrice comparative des actions"""
        print("\n" + "="*80)
        print("MATRICE COMPARATIVE")
        print("="*80)
        
        print(f"\n{'Action':<10} {'Pixels Modifiés':<20} {'Composantes':<15} {'Interprétation'}")
        print("-" * 80)
        
        for action_name in ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']:
            if action_name in self.results['actions']:
                stats = self.results['actions'][action_name]['stats']
                pixels = stats['pixels_modified']
                comps = stats['num_components']
                
                # Interprétation
                if pixels['mean'] == 0:
                    interp = "❌ Aucun effet"
                elif pixels['mean'] == 1:
                    interp = "✅ Avatar unique probable"
                elif pixels['mean'] < 10:
                    interp = "⚠️ Petit objet"
                elif pixels['mean'] < 100:
                    interp = "⚠️ Région modifiée"
                else:
                    interp = "⚠️ Transformation globale"
                
                print(f"{action_name:<10} {pixels['min']}-{pixels['max']} (moy:{pixels['mean']:.1f}){'':<5} {comps['min']}-{comps['max']} (moy:{comps['mean']:.1f}){'':<3} {interp}")
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"action_effects_analysis_{timestamp}.json"
        filepath = Path(__file__).parent / filename
        
        with open(filepath, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {filename}")
        return filename
    
    def generate_conclusions(self):
        """Générer conclusions basées sur résultats"""
        print("\n" + "="*80)
        print("CONCLUSIONS")
        print("="*80)
        
        if not self.results['actions']:
            print("❌ Aucune donnée à analyser")
            return
        
        # Analyser patterns
        all_pixels_zero = True
        has_single_pixel = False
        has_region = False
        has_global = False
        
        for action_name, data in self.results['actions'].items():
            mean_pixels = data['stats']['pixels_modified']['mean']
            
            if mean_pixels > 0:
                all_pixels_zero = False
            
            if 0.8 <= mean_pixels <= 1.2:
                has_single_pixel = True
            elif 2 <= mean_pixels < 100:
                has_region = True
            elif mean_pixels >= 100:
                has_global = True
        
        print("\n🔍 ANALYSE PARADIGME:")
        
        if all_pixels_zero:
            print("   ❌ AUCUNE action ne modifie la frame")
            print("   → Problème: Structure observation incorrecte")
            print("   → Action: Vérifier extraction frame")
        
        elif has_single_pixel:
            print("   ✅ Au moins une action modifie 1 pixel")
            print("   → Hypothèse: Avatar unique probable")
            print("   → Action: Peut continuer paradigme avatar")
        
        elif has_region:
            print("   ⚠️ Actions modifient régions (2-100 pixels)")
            print("   → Hypothèse: Transformation région, pas avatar unique")
            print("   → Action: Adapter méthodologie (pas recherche avatar)")
        
        elif has_global:
            print("   ⚠️ Actions modifient >100 pixels")
            print("   → Hypothèse: Transformation globale grille")
            print("   → Action: Paradigme complètement différent requis")
        
        print("\n📋 RECOMMANDATIONS SESSION 81:")
        
        if all_pixels_zero:
            print("   1. Débugger extraction frame")
            print("   2. Vérifier structure obs.frame")
            print("   3. Tester avec visualisation")
        
        elif has_single_pixel:
            print("   1. Identifier quel pixel bouge")
            print("   2. Valider contrôlabilité (corrélation >95%)")
            print("   3. Reprendre paradigme avatar")
        
        else:
            print("   1. Analyser composantes connexes")
            print("   2. Identifier patterns de transformation")
            print("   3. Adapter méthodologie au type de jeu réel")


def main():
    """Point d'entrée principal"""
    print("\n" + "="*80)
    print("SESSION 81 - ANALYSE EFFETS ACTIONS")
    print("Révolution méthodologique: Observer avant de modéliser")
    print("="*80)
    
    analyzer = ActionEffectsAnalyzer()
    
    # Setup
    if not analyzer.setup_environment():
        print("\n❌ ÉCHEC initialisation")
        return
    
    # Analyse
    analyzer.analyze_all_actions(num_tests=20)
    
    # Matrice comparative
    analyzer.create_comparison_matrix()
    
    # Conclusions
    analyzer.generate_conclusions()
    
    # Sauvegarde
    analyzer.save_results()
    
    print("\n" + "="*80)
    print("✅ ANALYSE COMPLÈTE")
    print("="*80)


if __name__ == "__main__":
    main()

# Made with Bob
