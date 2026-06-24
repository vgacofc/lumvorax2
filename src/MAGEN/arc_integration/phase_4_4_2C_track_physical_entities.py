#!/usr/bin/env python3
"""
PHASE 4.4.2C - TRACKING ENTITÉS PHYSIQUES RÉELLES

Objectif: Identifier combien d'entités physiques existent réellement

Méthodologie (selon recommandation utilisateur):
1. Construire composantes connexes COMPLÈTES (pas par valeur)
2. Attribuer identifiant STABLE à chaque composante
3. Suivre chaque composante sur plusieurs actions
4. Question: Combien d'entités physiques existent réellement?

Pas: Combien de valeurs changent?
Mais: Combien d'objets se déplacent indépendamment?

Date: 2026-06-19
Auteur: Bob (MAGEN Learning System)
"""

import sys
import os
import numpy as np
import json
from datetime import datetime
from collections import defaultdict
import scipy.ndimage as ndimage

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


class PhysicalEntityTracker:
    """Suit les entités physiques réelles à travers les actions."""
    
    def __init__(self):
        self.game = None
        self.entity_id_counter = 0
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'objective': 'track_physical_entities_across_actions',
                'method': 'connected_components_with_stable_ids'
            },
            'tests': [],
            'analysis': {}
        }
        
        print("="*70)
        print("PHASE 4.4.2C - TRACKING ENTITÉS PHYSIQUES")
        print("="*70)
        print("Objectif: Combien d'entités physiques existent?")
        print("Méthode: Composantes connexes + IDs stables")
        print("="*70)
        print()
    
    def initialize_game(self):
        """Initialise le jeu."""
        print("🎮 Initialisation jeu...")
        self.game = ls20.Ls20()
        self.game.set_level(0)
        print("   ✅ Jeu initialisé (Niveau 1)")
    
    def extract_all_entities(self, pixels):
        """Extrait TOUTES les entités (composantes connexes) de l'image."""
        entities = []
        
        # Pour chaque valeur non-background
        unique_values = np.unique(pixels)
        for value in unique_values:
            if value == 3:  # Skip background
                continue
            
            # Masque pour cette valeur
            mask = (pixels == value).astype(int)
            
            # Composantes connexes
            labeled, num_features = ndimage.label(mask)
            
            # Extraire chaque composante
            for comp_id in range(1, num_features + 1):
                comp_mask = (labeled == comp_id)
                coords = np.argwhere(comp_mask)
                
                if len(coords) > 0:
                    centroid = coords.mean(axis=0)
                    
                    entity = {
                        'value': int(value),
                        'component_id': comp_id,
                        'num_pixels': len(coords),
                        'centroid': [float(centroid[0]), float(centroid[1])],
                        'bbox': {
                            'min_y': int(coords[:, 0].min()),
                            'max_y': int(coords[:, 0].max()),
                            'min_x': int(coords[:, 1].min()),
                            'max_x': int(coords[:, 1].max())
                        },
                        'coords': coords.tolist()
                    }
                    entities.append(entity)
        
        return entities
    
    def match_entities(self, entities_before, entities_after):
        """
        Associe entités avant/après basé sur proximité centroid.
        Retourne mapping: entity_before_idx → entity_after_idx
        """
        if not entities_before or not entities_after:
            return {}
        
        matches = {}
        used_after = set()
        
        for i, ent_before in enumerate(entities_before):
            best_match = None
            best_distance = float('inf')
            
            centroid_before = np.array(ent_before['centroid'])
            
            for j, ent_after in enumerate(entities_after):
                if j in used_after:
                    continue
                
                # Même valeur?
                if ent_before['value'] != ent_after['value']:
                    continue
                
                centroid_after = np.array(ent_after['centroid'])
                distance = np.linalg.norm(centroid_before - centroid_after)
                
                # Seuil: max 20 pixels de déplacement
                if distance < 20 and distance < best_distance:
                    best_distance = distance
                    best_match = j
            
            if best_match is not None:
                matches[i] = best_match
                used_after.add(best_match)
        
        return matches
    
    def compute_entity_movement(self, entity_before, entity_after):
        """Calcule mouvement d'une entité."""
        centroid_before = np.array(entity_before['centroid'])
        centroid_after = np.array(entity_after['centroid'])
        
        displacement = centroid_after - centroid_before
        distance = np.linalg.norm(displacement)
        
        return {
            'dy': float(displacement[0]),
            'dx': float(displacement[1]),
            'distance': float(distance),
            'pixels_before': entity_before['num_pixels'],
            'pixels_after': entity_after['num_pixels']
        }
    
    def test_action(self, action_enum, action_name):
        """Teste une action et suit les entités."""
        print(f"\n{'='*60}")
        print(f"TEST: {action_name}")
        print(f"{'='*60}")
        
        # Reset
        self.game.set_level(0)
        
        # État AVANT
        pixels_before = self.game.camera.render(self.game.current_level._sprites)
        entities_before = self.extract_all_entities(pixels_before)
        
        print(f"État AVANT:")
        print(f"  Entités détectées: {len(entities_before)}")
        
        # Grouper par valeur
        by_value = defaultdict(int)
        for ent in entities_before:
            by_value[ent['value']] += 1
        
        for value, count in sorted(by_value.items()):
            print(f"    Valeur {value}: {count} composante(s)")
        
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
        entities_after = self.extract_all_entities(pixels_after)
        
        print(f"\nÉtat APRÈS ({steps} steps):")
        print(f"  Entités détectées: {len(entities_after)}")
        
        by_value_after = defaultdict(int)
        for ent in entities_after:
            by_value_after[ent['value']] += 1
        
        for value, count in sorted(by_value_after.items()):
            print(f"    Valeur {value}: {count} composante(s)")
        
        # Matcher entités
        matches = self.match_entities(entities_before, entities_after)
        
        print(f"\nENTITÉS MATCHÉES: {len(matches)}/{len(entities_before)}")
        
        # Analyser mouvements
        movements = []
        for idx_before, idx_after in matches.items():
            ent_before = entities_before[idx_before]
            ent_after = entities_after[idx_after]
            
            movement = self.compute_entity_movement(ent_before, ent_after)
            
            movements.append({
                'value': ent_before['value'],
                'component_before': ent_before['component_id'],
                'component_after': ent_after['component_id'],
                'movement': movement
            })
            
            if movement['distance'] > 0.1:
                print(f"  Valeur {ent_before['value']} comp#{ent_before['component_id']}: "
                      f"dy={movement['dy']:.2f}, dx={movement['dx']:.2f}, "
                      f"dist={movement['distance']:.2f}")
        
        # Entités non-matchées (apparues/disparues)
        unmatched_before = [i for i in range(len(entities_before)) if i not in matches]
        unmatched_after = [i for i in range(len(entities_after)) if i not in matches.values()]
        
        if unmatched_before:
            print(f"\n  Entités DISPARUES: {len(unmatched_before)}")
            for idx in unmatched_before:
                ent = entities_before[idx]
                print(f"    Valeur {ent['value']} comp#{ent['component_id']}")
        
        if unmatched_after:
            print(f"\n  Entités APPARUES: {len(unmatched_after)}")
            for idx in unmatched_after:
                ent = entities_after[idx]
                print(f"    Valeur {ent['value']} comp#{ent['component_id']}")
        
        return {
            'action': action_name,
            'entities_before': entities_before,
            'entities_after': entities_after,
            'matches': {str(k): v for k, v in matches.items()},
            'movements': movements,
            'unmatched_before': unmatched_before,
            'unmatched_after': unmatched_after
        }
    
    def analyze_results(self, test_results):
        """Analyse combien d'entités physiques distinctes existent."""
        print(f"\n{'='*60}")
        print("ANALYSE FINALE: ENTITÉS PHYSIQUES")
        print(f"{'='*60}")
        
        # Compter entités mobiles par valeur
        mobile_entities = defaultdict(set)
        
        for test in test_results:
            for movement in test['movements']:
                if movement['movement']['distance'] > 0.1:
                    value = movement['value']
                    comp_id = movement['component_before']
                    mobile_entities[value].add(comp_id)
        
        print(f"\nEntités MOBILES par valeur:")
        total_mobile = 0
        for value in sorted(mobile_entities.keys()):
            count = len(mobile_entities[value])
            total_mobile += count
            print(f"  Valeur {value}: {count} composante(s) mobile(s)")
        
        print(f"\nTOTAL ENTITÉS MOBILES: {total_mobile}")
        
        # Analyser cohérence mouvements
        print(f"\n{'='*60}")
        print("COHÉRENCE MOUVEMENTS:")
        print(f"{'='*60}")
        
        for value in sorted(mobile_entities.keys()):
            print(f"\nValeur {value}:")
            
            # Extraire tous les mouvements de cette valeur
            value_movements = []
            for test in test_results:
                for movement in test['movements']:
                    if movement['value'] == value and movement['movement']['distance'] > 0.1:
                        value_movements.append({
                            'action': test['action'],
                            'dy': movement['movement']['dy'],
                            'dx': movement['movement']['dx']
                        })
            
            if value_movements:
                # Grouper par action
                by_action = defaultdict(list)
                for mov in value_movements:
                    by_action[mov['action']].append((mov['dy'], mov['dx']))
                
                for action in sorted(by_action.keys()):
                    movements = by_action[action]
                    if len(movements) == 1:
                        dy, dx = movements[0]
                        print(f"  {action}: dy={dy:.2f}, dx={dx:.2f}")
                    else:
                        print(f"  {action}: {len(movements)} composantes")
                        for i, (dy, dx) in enumerate(movements, 1):
                            print(f"    Comp#{i}: dy={dy:.2f}, dx={dx:.2f}")
        
        # Verdict
        print(f"\n{'='*60}")
        print("VERDICT:")
        print(f"{'='*60}")
        
        if total_mobile == 1:
            print("\n✅ UNE SEULE ENTITÉ MOBILE DÉTECTÉE")
            print("   → Avatar unique identifié")
        elif total_mobile == 2:
            print("\n⚠️  DEUX ENTITÉS MOBILES DÉTECTÉES")
            print("   → Soit: Avatar + objet attaché")
            print("   → Soit: Deux objets indépendants")
        else:
            print(f"\n⚠️  {total_mobile} ENTITÉS MOBILES DÉTECTÉES")
            print("   → Jeu multi-objets confirmé")
        
        return {
            'total_mobile_entities': total_mobile,
            'mobile_by_value': {k: len(v) for k, v in mobile_entities.items()},
            'verdict': 'single' if total_mobile == 1 else 'multiple'
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
        output_file = f"phase_4_4_2C_physical_entities_{timestamp}.json"
        
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n{'='*60}")
        print(f"Résultats sauvegardés: {output_file}")
        print(f"{'='*60}")
        
        return self.results


def main():
    """Point d'entrée principal."""
    tracker = PhysicalEntityTracker()
    results = tracker.run()
    
    print("\n" + "="*60)
    print("FIN PHASE 4.4.2C")
    print("="*60)


if __name__ == "__main__":
    main()

# Made with Bob
