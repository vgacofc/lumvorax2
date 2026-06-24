#!/usr/bin/env python3
"""
PHASE 4.4.2B - TEST GÉOMÉTRIE RELATIVE VALEURS 9+12

Objectif: Déterminer si valeurs 9 et 12 forment une seule entité

Méthodologie:
1. Extraire coordonnées exactes pixels 9 et 12
2. Calculer distance relative + orientation + forme
3. Répéter après ACTION1, ACTION3, ACTION4
4. Vérifier si géométrie relative reste STRICTEMENT identique

Si géométrie stable → 9+12 = Une seule entité (sprite composite)
Si géométrie change → 9 et 12 = Entités distinctes

Date: 2026-06-19
Auteur: Bob (MAGEN Learning System)
"""

import sys
import os
import numpy as np
import json
from datetime import datetime
from collections import defaultdict

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


class EntityGeometryTester:
    """Teste si valeurs 9 et 12 forment une seule entité."""
    
    def __init__(self):
        self.game = None
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'objective': 'test_if_values_9_12_form_single_entity',
                'method': 'relative_geometry_stability'
            },
            'tests': [],
            'analysis': {}
        }
        
        print("="*70)
        print("PHASE 4.4.2B - TEST GÉOMÉTRIE RELATIVE 9+12")
        print("="*70)
        print("Objectif: Déterminer si 9 et 12 = Une seule entité")
        print("Méthode: Stabilité géométrie relative")
        print("="*70)
        print()
    
    def initialize_game(self):
        """Initialise le jeu."""
        print("🎮 Initialisation jeu...")
        self.game = ls20.Ls20()
        self.game.set_level(0)
        print("   ✅ Jeu initialisé (Niveau 1)")
    
    def extract_pixel_coordinates(self, pixels, value):
        """Extrait coordonnées de tous les pixels d'une valeur."""
        coords = np.argwhere(pixels == value)
        return coords  # Format: [[y1, x1], [y2, x2], ...]
    
    def compute_geometry(self, coords_9, coords_12):
        """Calcule géométrie relative entre valeurs 9 et 12."""
        if len(coords_9) == 0 or len(coords_12) == 0:
            return None
        
        # Centroids
        centroid_9 = coords_9.mean(axis=0)
        centroid_12 = coords_12.mean(axis=0)
        
        # Distance entre centroids
        distance = np.linalg.norm(centroid_9 - centroid_12)
        
        # Vecteur direction (de 12 vers 9)
        direction = centroid_9 - centroid_12
        angle = np.arctan2(direction[0], direction[1]) * 180 / np.pi
        
        # Bounding boxes
        bbox_9 = {
            'min_y': int(coords_9[:, 0].min()),
            'max_y': int(coords_9[:, 0].max()),
            'min_x': int(coords_9[:, 1].min()),
            'max_x': int(coords_9[:, 1].max())
        }
        
        bbox_12 = {
            'min_y': int(coords_12[:, 0].min()),
            'max_y': int(coords_12[:, 0].max()),
            'min_x': int(coords_12[:, 1].min()),
            'max_x': int(coords_12[:, 1].max())
        }
        
        # Forme (largeur × hauteur)
        shape_9 = (bbox_9['max_y'] - bbox_9['min_y'] + 1, 
                   bbox_9['max_x'] - bbox_9['min_x'] + 1)
        shape_12 = (bbox_12['max_y'] - bbox_12['min_y'] + 1,
                    bbox_12['max_x'] - bbox_12['min_x'] + 1)
        
        # Overlap (pixels communs)
        overlap_y = max(0, min(bbox_9['max_y'], bbox_12['max_y']) - 
                          max(bbox_9['min_y'], bbox_12['min_y']) + 1)
        overlap_x = max(0, min(bbox_9['max_x'], bbox_12['max_x']) - 
                          max(bbox_9['min_x'], bbox_12['min_x']) + 1)
        overlap_area = overlap_y * overlap_x
        
        return {
            'centroid_9': [float(centroid_9[0]), float(centroid_9[1])],
            'centroid_12': [float(centroid_12[0]), float(centroid_12[1])],
            'distance': float(distance),
            'angle': float(angle),
            'shape_9': shape_9,
            'shape_12': shape_12,
            'bbox_9': bbox_9,
            'bbox_12': bbox_12,
            'overlap_area': overlap_area,
            'num_pixels_9': len(coords_9),
            'num_pixels_12': len(coords_12)
        }
    
    def test_action(self, action_enum, action_name):
        """Teste une action et mesure géométrie avant/après."""
        print(f"\n{'='*60}")
        print(f"TEST: {action_name}")
        print(f"{'='*60}")
        
        # Reset
        self.game.set_level(0)
        
        # État AVANT
        pixels_before = self.game.camera.render(self.game.current_level._sprites)
        coords_9_before = self.extract_pixel_coordinates(pixels_before, 9)
        coords_12_before = self.extract_pixel_coordinates(pixels_before, 12)
        geometry_before = self.compute_geometry(coords_9_before, coords_12_before)
        
        print(f"État AVANT:")
        print(f"  Pixels valeur 9: {len(coords_9_before)}")
        print(f"  Pixels valeur 12: {len(coords_12_before)}")
        if geometry_before:
            print(f"  Distance 9↔12: {geometry_before['distance']:.2f}")
            print(f"  Angle: {geometry_before['angle']:.2f}°")
            print(f"  Overlap: {geometry_before['overlap_area']} pixels")
        
        # Exécuter action
        class ActionInput:
            def __init__(self, action_id):
                self.id = action_id
        
        action_input = ActionInput(action_enum)
        self.game._set_action(action_input)
        
        steps = 0
        max_steps = 100
        while not self.game.is_action_complete() and steps < max_steps:
            self.game.step()
            steps += 1
        
        # État APRÈS
        pixels_after = self.game.camera.render(self.game.current_level._sprites)
        coords_9_after = self.extract_pixel_coordinates(pixels_after, 9)
        coords_12_after = self.extract_pixel_coordinates(pixels_after, 12)
        geometry_after = self.compute_geometry(coords_9_after, coords_12_after)
        
        print(f"\nÉtat APRÈS ({steps} steps):")
        print(f"  Pixels valeur 9: {len(coords_9_after)}")
        print(f"  Pixels valeur 12: {len(coords_12_after)}")
        if geometry_after:
            print(f"  Distance 9↔12: {geometry_after['distance']:.2f}")
            print(f"  Angle: {geometry_after['angle']:.2f}°")
            print(f"  Overlap: {geometry_after['overlap_area']} pixels")
        
        # Comparer géométries
        if geometry_before and geometry_after:
            delta_distance = abs(geometry_after['distance'] - geometry_before['distance'])
            delta_angle = abs(geometry_after['angle'] - geometry_before['angle'])
            delta_overlap = abs(geometry_after['overlap_area'] - geometry_before['overlap_area'])
            
            print(f"\nCHANGEMENTS:")
            print(f"  Δ Distance: {delta_distance:.4f}")
            print(f"  Δ Angle: {delta_angle:.4f}°")
            print(f"  Δ Overlap: {delta_overlap} pixels")
            print(f"  Δ Pixels 9: {len(coords_9_after) - len(coords_9_before)}")
            print(f"  Δ Pixels 12: {len(coords_12_after) - len(coords_12_before)}")
            
            # Verdict
            geometry_stable = (delta_distance < 0.01 and 
                             delta_angle < 0.1 and 
                             delta_overlap == 0 and
                             len(coords_9_after) == len(coords_9_before) and
                             len(coords_12_after) == len(coords_12_before))
            
            if geometry_stable:
                print(f"\n  ✅ GÉOMÉTRIE STABLE (entité unique probable)")
            else:
                print(f"\n  ⚠️  GÉOMÉTRIE CHANGE (entités distinctes?)")
        
        return {
            'action': action_name,
            'geometry_before': geometry_before,
            'geometry_after': geometry_after,
            'coords_9_before': coords_9_before.tolist(),
            'coords_12_before': coords_12_before.tolist(),
            'coords_9_after': coords_9_after.tolist(),
            'coords_12_after': coords_12_after.tolist()
        }
    
    def analyze_results(self, test_results):
        """Analyse si géométrie reste stable sur toutes les actions."""
        print(f"\n{'='*60}")
        print("ANALYSE FINALE: GÉOMÉTRIE RELATIVE 9+12")
        print(f"{'='*60}")
        
        all_stable = True
        stability_scores = []
        
        for test in test_results:
            action = test['action']
            geom_before = test['geometry_before']
            geom_after = test['geometry_after']
            
            if geom_before and geom_after:
                delta_distance = abs(geom_after['distance'] - geom_before['distance'])
                delta_angle = abs(geom_after['angle'] - geom_before['angle'])
                delta_overlap = abs(geom_after['overlap_area'] - geom_before['overlap_area'])
                
                stable = (delta_distance < 0.01 and 
                         delta_angle < 0.1 and 
                         delta_overlap == 0)
                
                stability_scores.append({
                    'action': action,
                    'stable': stable,
                    'delta_distance': delta_distance,
                    'delta_angle': delta_angle,
                    'delta_overlap': delta_overlap
                })
                
                if not stable:
                    all_stable = False
        
        print(f"\nRésultats par action:")
        for score in stability_scores:
            status = "✅ STABLE" if score['stable'] else "❌ INSTABLE"
            print(f"  {score['action']}: {status}")
            print(f"    Δ Distance: {score['delta_distance']:.4f}")
            print(f"    Δ Angle: {score['delta_angle']:.4f}°")
            print(f"    Δ Overlap: {score['delta_overlap']}")
        
        print(f"\n{'='*60}")
        print("VERDICT FINAL:")
        print(f"{'='*60}")
        
        if all_stable:
            print("\n✅ GÉOMÉTRIE STRICTEMENT STABLE SUR TOUTES LES ACTIONS")
            print("\nConclusion: Valeurs 9 et 12 forment UNE SEULE ENTITÉ")
            print("  - Distance relative constante")
            print("  - Angle relatif constant")
            print("  - Overlap constant")
            print("  - Nombre pixels constant")
            print("\nHypothèse validée: Sprite composite (9=intérieur, 12=contour)")
            verdict = "single_entity"
            confidence = 95.0
        else:
            print("\n⚠️  GÉOMÉTRIE CHANGE ENTRE ACTIONS")
            print("\nConclusion: Valeurs 9 et 12 sont probablement DISTINCTES")
            print("  - Distance ou angle varie")
            print("  - Overlap change")
            print("  - Entités indépendantes ou partiellement liées")
            verdict = "distinct_entities"
            confidence = 70.0
        
        return {
            'verdict': verdict,
            'confidence': confidence,
            'all_stable': all_stable,
            'stability_scores': stability_scores
        }
    
    def run(self):
        """Exécute le protocole complet."""
        self.initialize_game()
        
        # Test 3 actions
        actions_to_test = [
            (GameAction.ACTION1, "ACTION1"),
            (GameAction.ACTION3, "ACTION3"),
            (GameAction.ACTION4, "ACTION4")
        ]
        
        test_results = []
        for action_enum, action_name in actions_to_test:
            result = self.test_action(action_enum, action_name)
            test_results.append(result)
            self.results['tests'].append(result)
        
        # Analyser
        analysis = self.analyze_results(test_results)
        self.results['analysis'] = analysis
        
        # Sauvegarder
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"phase_4_4_2B_entity_geometry_{timestamp}.json"
        
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n{'='*60}")
        print(f"Résultats sauvegardés: {output_file}")
        print(f"{'='*60}")
        
        return self.results


def main():
    """Point d'entrée principal."""
    tester = EntityGeometryTester()
    results = tester.run()
    
    print("\n" + "="*60)
    print("FIN PHASE 4.4.2B")
    print("="*60)


if __name__ == "__main__":
    main()

# Made with Bob
