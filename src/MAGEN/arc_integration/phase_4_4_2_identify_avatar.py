#!/usr/bin/env python3
"""
PHASE 4.4.2 - IDENTIFICATION AVATAR (PRIORITÉ 1)

Protocole simple pour identifier l'avatar avec certitude:
1. Reset + ACTION1 → Observer composantes mobiles
2. Reset + ACTION3 → Observer composantes mobiles
3. Reset + ACTION4 → Observer composantes mobiles
4. Comparer: Avatar = objet qui répond à PLUSIEURS actions

Critères avatar:
- Répond immédiatement à plusieurs actions
- Conserve son identité entre états
- Influence autres objets

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


class AvatarIdentifier:
    """Identifie l'avatar avec certitude via protocole simple."""
    
    def __init__(self):
        self.game = ls20.Ls20()
        self.results = {
            'metadata': {
                'date': datetime.now().isoformat(),
                'protocol': 'simple_reset_actions',
                'objective': 'identify_avatar_with_certainty'
            },
            'tests': [],
            'analysis': {},
            'avatar_identification': {}
        }
    
    def get_state_snapshot(self):
        """Capture état complet du jeu."""
        pixels = self.game.camera.render(self.game.current_level._sprites)
        
        # Extraire composantes connectées pour chaque valeur
        components = {}
        unique_values = np.unique(pixels)
        
        for val in unique_values:
            if val == 3:  # Skip background
                continue
            
            mask = (pixels == val).astype(int)
            labeled, num_features = ndimage.label(mask)
            
            components[int(val)] = {
                'count': num_features,
                'total_pixels': int(np.sum(mask)),
                'positions': []
            }
            
            # Extraire positions de chaque composante
            for i in range(1, num_features + 1):
                comp_mask = (labeled == i)
                positions = np.argwhere(comp_mask)
                if len(positions) > 0:
                    centroid = positions.mean(axis=0)
                    components[int(val)]['positions'].append({
                        'component_id': i,
                        'pixels': int(np.sum(comp_mask)),
                        'centroid': [float(centroid[0]), float(centroid[1])],
                        'bbox': {
                            'min_y': int(positions[:, 0].min()),
                            'max_y': int(positions[:, 0].max()),
                            'min_x': int(positions[:, 1].min()),
                            'max_x': int(positions[:, 1].max())
                        }
                    })
        
        return {
            'pixels': pixels.tolist(),
            'shape': list(pixels.shape),
            'components': components,
            'unique_values': [int(v) for v in unique_values]
        }
    
    def compare_states(self, state_before, state_after, action_name):
        """Compare deux états et identifie composantes mobiles."""
        pixels_before = np.array(state_before['pixels'])
        pixels_after = np.array(state_after['pixels'])
        
        # Calculer différence
        diff = pixels_after != pixels_before
        num_changed = int(np.sum(diff))
        
        # Identifier composantes qui ont bougé
        mobile_components = {}
        
        for val in state_before['components'].keys():
            if val not in state_after['components']:
                continue
            
            comps_before = state_before['components'][val]['positions']
            comps_after = state_after['components'][val]['positions']
            
            # Comparer centroids
            movements = []
            for i, comp_before in enumerate(comps_before):
                if i < len(comps_after):
                    comp_after = comps_after[i]
                    
                    dy = comp_after['centroid'][0] - comp_before['centroid'][0]
                    dx = comp_after['centroid'][1] - comp_before['centroid'][1]
                    
                    if abs(dy) > 0.1 or abs(dx) > 0.1:  # Seuil mouvement
                        movements.append({
                            'component_id': i + 1,
                            'dy': float(dy),
                            'dx': float(dx),
                            'distance': float(np.sqrt(dy**2 + dx**2)),
                            'pixels_before': comp_before['pixels'],
                            'pixels_after': comp_after['pixels']
                        })
            
            if movements:
                mobile_components[val] = {
                    'num_mobile': len(movements),
                    'movements': movements
                }
        
        return {
            'action': action_name,
            'pixels_changed': num_changed,
            'mobile_components': mobile_components
        }
    
    def test_action(self, action_enum, action_name):
        """Test une action et identifie composantes mobiles."""
        print(f"\n{'='*60}")
        print(f"TEST: {action_name}")
        print(f"{'='*60}")
        
        # Reset
        self.game.set_level(0)
        state_before = self.get_state_snapshot()
        print(f"État initial capturé: {len(state_before['components'])} valeurs uniques")
        
        # Exécuter action (méthode correcte)
        class ActionInput:
            def __init__(self, action_id):
                self.id = action_id
        
        action_input = ActionInput(action_enum)
        self.game._set_action(action_input)
        
        # Boucle step jusqu'à action complète
        steps = 0
        max_steps = 100
        while not self.game.is_action_complete() and steps < max_steps:
            self.game.step()
            steps += 1
        
        state_after = self.get_state_snapshot()
        print(f"État après action capturé ({steps} steps)")
        
        # Comparer
        comparison = self.compare_states(state_before, state_after, action_name)
        
        print(f"\nRésultats:")
        print(f"  Pixels changés: {comparison['pixels_changed']}")
        print(f"  Valeurs mobiles: {len(comparison['mobile_components'])}")
        
        for val, info in comparison['mobile_components'].items():
            print(f"\n  Valeur {val}:")
            print(f"    Composantes mobiles: {info['num_mobile']}")
            for mov in info['movements']:
                print(f"      Composante {mov['component_id']}: dy={mov['dy']:.2f}, dx={mov['dx']:.2f}, dist={mov['distance']:.2f}")
        
        return {
            'action': action_name,
            'state_before': state_before,
            'state_after': state_after,
            'comparison': comparison
        }
    
    def identify_avatar(self, test_results):
        """Identifie l'avatar en analysant les résultats des tests."""
        print(f"\n{'='*60}")
        print("ANALYSE: IDENTIFICATION AVATAR")
        print(f"{'='*60}")
        
        # Compter combien d'actions affectent chaque valeur
        value_action_count = defaultdict(int)
        value_movements = defaultdict(list)
        
        for test in test_results:
            action = test['action']
            for val, info in test['comparison']['mobile_components'].items():
                value_action_count[val] += 1
                value_movements[val].append({
                    'action': action,
                    'num_mobile': info['num_mobile'],
                    'movements': info['movements']
                })
        
        print("\nRésumé par valeur:")
        for val in sorted(value_action_count.keys()):
            count = value_action_count[val]
            print(f"\n  Valeur {val}:")
            print(f"    Répond à {count}/3 actions")
            print(f"    Mouvements:")
            for mov_info in value_movements[val]:
                print(f"      {mov_info['action']}: {mov_info['num_mobile']} composantes mobiles")
        
        # Critère avatar: Répond à PLUSIEURS actions (au moins 2)
        avatar_candidates = []
        for val, count in value_action_count.items():
            if count >= 2:
                avatar_candidates.append({
                    'value': val,
                    'action_count': count,
                    'confidence': count / 3.0,  # 3 actions testées
                    'movements': value_movements[val]
                })
        
        # Trier par confiance
        avatar_candidates.sort(key=lambda x: x['confidence'], reverse=True)
        
        print(f"\n{'='*60}")
        print("CANDIDATS AVATAR:")
        print(f"{'='*60}")
        
        if not avatar_candidates:
            print("❌ AUCUN CANDIDAT TROUVÉ")
            return None
        
        for i, candidate in enumerate(avatar_candidates, 1):
            print(f"\n{i}. Valeur {candidate['value']}")
            print(f"   Confiance: {candidate['confidence']*100:.1f}%")
            print(f"   Répond à {candidate['action_count']}/3 actions")
            
            if candidate['confidence'] >= 0.67:  # Au moins 2/3 actions
                print(f"   ✅ AVATAR IDENTIFIÉ AVEC HAUTE CONFIANCE")
            elif candidate['confidence'] >= 0.33:  # Au moins 1/3 actions
                print(f"   ⚠️  CANDIDAT POSSIBLE (confiance moyenne)")
        
        return avatar_candidates
    
    def run(self):
        """Exécute le protocole complet d'identification."""
        print("="*60)
        print("PHASE 4.4.2 - IDENTIFICATION AVATAR")
        print("="*60)
        print("\nProtocole: Reset + Action × 3")
        print("Objectif: Identifier avatar avec certitude\n")
        
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
        
        # Identifier avatar
        avatar_candidates = self.identify_avatar(test_results)
        
        if avatar_candidates:
            self.results['avatar_identification'] = {
                'status': 'identified',
                'candidates': avatar_candidates,
                'best_candidate': avatar_candidates[0] if avatar_candidates else None
            }
        else:
            self.results['avatar_identification'] = {
                'status': 'not_found',
                'candidates': [],
                'best_candidate': None
            }
        
        # Sauvegarder résultats
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"phase_4_4_2_avatar_identification_{timestamp}.json"
        
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n{'='*60}")
        print(f"Résultats sauvegardés: {output_file}")
        print(f"{'='*60}")
        
        return self.results


def main():
    """Point d'entrée principal."""
    identifier = AvatarIdentifier()
    results = identifier.run()
    
    # Afficher résumé final
    print("\n" + "="*60)
    print("RÉSUMÉ FINAL")
    print("="*60)
    
    if results['avatar_identification']['status'] == 'identified':
        best = results['avatar_identification']['best_candidate']
        print(f"\n✅ AVATAR IDENTIFIÉ:")
        print(f"   Valeur: {best['value']}")
        print(f"   Confiance: {best['confidence']*100:.1f}%")
        print(f"   Répond à {best['action_count']}/3 actions")
    else:
        print("\n❌ AVATAR NON IDENTIFIÉ")
        print("   Aucune valeur ne répond à plusieurs actions")
    
    print("\n" + "="*60)


if __name__ == "__main__":
    main()

# Made with Bob
