#!/usr/bin/env python3
"""
PHASE 4.4.2G - Test ACTION2 comme Modificateur de Règles

Hypothèse critique (feedback utilisateur) :
    ACTION2 ne modifie pas la couche VISUELLE
    ACTION2 modifie la couche LOGIQUE (règles du moteur)

Test B : ACTION2 modifie-t-il l'interprétation future des actions ?

Protocole :
    1. Baseline : ACTION4×10 (sans ACTION2)
    2. Test : ACTION2×N + ACTION4×10 (avec ACTION2)
    3. Comparer : Distance finale, état absorbant, mobilité

Si distance finale change → ACTION2 modifie règles du moteur
Si état absorbant disparaît → ACTION2 déverrouille contraintes

Auteur: Bob (MAGEN Project)
Date: 2026-06-19
"""

import sys
import os
import numpy as np
import json
from datetime import datetime
from scipy.ndimage import label

# Add arc_integration to path
arc_integration_path = os.path.join(os.path.dirname(__file__), 'arc_integration')
sys.path.insert(0, arc_integration_path)

from ls20_real_wrapper import LS20RealWrapper

def extract_components(grid):
    """Extrait composantes connexes pour chaque valeur"""
    components = {}
    for value in range(13):
        mask = (grid == value)
        if not mask.any():
            continue
        labeled, num = label(mask)
        components[value] = []
        for comp_id in range(1, num + 1):
            coords = np.argwhere(labeled == comp_id)
            if len(coords) > 0:
                centroid = coords.mean(axis=0)
                components[value].append({
                    'comp_id': comp_id,
                    'centroid': centroid.tolist(),
                    'pixels': len(coords),
                    'coords': coords.tolist()
                })
    return components

def get_mobile_entities(components):
    """Identifie entités mobiles (9#3, 11#1, 12#1)"""
    mobile = {}
    if 9 in components and len(components[9]) >= 3:
        mobile['avatar_9_3'] = components[9][2]  # comp#3
    if 11 in components and len(components[11]) >= 1:
        mobile['timer_11_1'] = components[11][0]  # comp#1
    if 12 in components and len(components[12]) >= 1:
        mobile['attached_12_1'] = components[12][0]  # comp#1
    return mobile

def calculate_distance_to_goals(avatar_pos, goals):
    """Calcule distance euclidienne minimale vers goals"""
    if not goals or avatar_pos is None:
        return None
    distances = []
    for goal in goals:
        goal_pos = np.array(goal['centroid'])
        dist = np.linalg.norm(np.array(avatar_pos) - goal_pos)
        distances.append(dist)
    return min(distances) if distances else None

def test_baseline_action4():
    """Test baseline : ACTION4×10 sans ACTION2"""
    print("\n" + "="*80)
    print("TEST BASELINE : ACTION4×10 (sans ACTION2)")
    print("="*80)
    
    env = LS20RealWrapper()
    obs = env.reset()
    info = {}
    
    # État initial
    initial_components = extract_components(obs)
    initial_mobile = get_mobile_entities(initial_components)
    initial_goals = initial_components.get(8, [])
    
    print(f"\nÉtat initial :")
    print(f"  Avatar 9#3 : {initial_mobile.get('avatar_9_3', {}).get('centroid', 'N/A')}")
    print(f"  Goals (8) : {len(initial_goals)} détectés")
    
    initial_dist = None
    if 'avatar_9_3' in initial_mobile:
        initial_dist = calculate_distance_to_goals(
            initial_mobile['avatar_9_3']['centroid'],
            initial_goals
        )
        print(f"  Distance initiale : {initial_dist:.2f}")
    
    # Exécuter ACTION4×10
    print(f"\nExécution ACTION4×10...")
    states = []
    for step in range(10):
        obs, reward, done, info = env.step(3)  # ACTION4 = index 3
        components = extract_components(obs)
        mobile = get_mobile_entities(components)
        goals = components.get(8, [])
        
        state = {
            'step': step + 1,
            'mobile': mobile,
            'goals': goals,
            'done': done,
            'reward': reward
        }
        
        if 'avatar_9_3' in mobile:
            dist = calculate_distance_to_goals(mobile['avatar_9_3']['centroid'], goals)
            state['distance'] = dist
            print(f"  Step {step+1}: Avatar={mobile['avatar_9_3']['centroid']}, Distance={dist:.2f}, Done={done}")
        else:
            print(f"  Step {step+1}: Avatar non détecté, Done={done}")
        
        states.append(state)
        
        if done:
            print(f"  ⚠️ État absorbant atteint au step {step+1}")
            break
    
    # Pas de close() pour LS20RealWrapper
    
    return {
        'initial': {
            'mobile': initial_mobile,
            'goals': initial_goals,
            'distance': initial_dist if 'avatar_9_3' in initial_mobile else None
        },
        'states': states
    }

def test_action2_modifier(n_action2):
    """Test : ACTION2×N + ACTION4×10"""
    print("\n" + "="*80)
    print(f"TEST MODIFICATEUR : ACTION2×{n_action2} + ACTION4×10")
    print("="*80)
    
    env = LS20RealWrapper()
    obs = env.reset()
    info = {}
    
    # État initial
    initial_components = extract_components(obs)
    initial_mobile = get_mobile_entities(initial_components)
    initial_goals = initial_components.get(8, [])
    
    print(f"\nÉtat initial :")
    print(f"  Avatar 9#3 : {initial_mobile.get('avatar_9_3', {}).get('centroid', 'N/A')}")
    print(f"  Goals (8) : {len(initial_goals)} détectés")
    
    initial_dist = None
    if 'avatar_9_3' in initial_mobile:
        initial_dist = calculate_distance_to_goals(
            initial_mobile['avatar_9_3']['centroid'],
            initial_goals
        )
        print(f"  Distance initiale : {initial_dist:.2f}")
    
    # Phase 1 : ACTION2×N
    print(f"\nPhase 1 : Exécution ACTION2×{n_action2}...")
    action2_states = []
    for step in range(n_action2):
        obs, reward, done, info = env.step(1)  # ACTION2 = index 1
        components = extract_components(obs)
        mobile = get_mobile_entities(components)
        
        # Détecter composante 3#2 (porte)
        door_3_2 = None
        if 3 in components and len(components[3]) >= 2:
            door_3_2 = components[3][1]  # comp#2
        
        state = {
            'step': step + 1,
            'mobile': mobile,
            'door_3_2': door_3_2,
            'done': done
        }
        action2_states.append(state)
        
        if door_3_2:
            print(f"  Step {step+1}: Porte 3#2 pixels={door_3_2['pixels']}, centroid={door_3_2['centroid']}")
        else:
            print(f"  Step {step+1}: Porte 3#2 non détectée")
    
    # Phase 2 : ACTION4×10
    print(f"\nPhase 2 : Exécution ACTION4×10...")
    action4_states = []
    for step in range(10):
        obs, reward, done, info = env.step(3)  # ACTION4 = index 3
        components = extract_components(obs)
        mobile = get_mobile_entities(components)
        goals = components.get(8, [])
        
        state = {
            'step': step + 1,
            'mobile': mobile,
            'goals': goals,
            'done': done,
            'reward': reward
        }
        
        if 'avatar_9_3' in mobile:
            dist = calculate_distance_to_goals(mobile['avatar_9_3']['centroid'], goals)
            state['distance'] = dist
            print(f"  Step {step+1}: Avatar={mobile['avatar_9_3']['centroid']}, Distance={dist:.2f}, Done={done}")
        else:
            print(f"  Step {step+1}: Avatar non détecté, Done={done}")
        
        action4_states.append(state)
        
        if done:
            print(f"  ⚠️ État absorbant atteint au step {step+1}")
            break
    
    # Pas de close() pour LS20RealWrapper
    
    return {
        'initial': {
            'mobile': initial_mobile,
            'goals': initial_goals,
            'distance': initial_dist if 'avatar_9_3' in initial_mobile else None
        },
        'action2_phase': action2_states,
        'action4_phase': action4_states
    }

def compare_results(baseline, tests):
    """Compare résultats baseline vs tests avec ACTION2"""
    print("\n" + "="*80)
    print("ANALYSE COMPARATIVE")
    print("="*80)
    
    # Distance finale baseline
    baseline_final = baseline['states'][-1]
    baseline_dist = baseline_final.get('distance', None)
    baseline_done = baseline_final.get('done', False)
    baseline_steps = len(baseline['states'])
    
    print(f"\nBaseline (ACTION4×10 seul) :")
    if baseline_dist is not None:
        print(f"  Distance finale : {baseline_dist:.2f}")
    else:
        print(f"  Distance finale : N/A")
    print(f"  État absorbant : {baseline_done}")
    print(f"  Steps exécutés : {baseline_steps}")
    
    # Comparer chaque test
    results = []
    for n_action2, test_data in tests.items():
        test_final = test_data['action4_phase'][-1]
        test_dist = test_final.get('distance', None)
        test_done = test_final.get('done', False)
        test_steps = len(test_data['action4_phase'])
        
        print(f"\nTest ACTION2×{n_action2} + ACTION4×10 :")
        if test_dist is not None:
            print(f"  Distance finale : {test_dist:.2f}")
        else:
            print(f"  Distance finale : N/A")
        print(f"  État absorbant : {test_done}")
        print(f"  Steps exécutés : {test_steps}")
        
        # Calcul différences
        if baseline_dist and test_dist:
            delta_dist = test_dist - baseline_dist
            print(f"  Δ Distance : {delta_dist:+.2f}")
            
            result = {
                'n_action2': n_action2,
                'baseline_dist': baseline_dist,
                'test_dist': test_dist,
                'delta_dist': delta_dist,
                'baseline_done': baseline_done,
                'test_done': test_done,
                'baseline_steps': baseline_steps,
                'test_steps': test_steps
            }
            results.append(result)
    
    return results

def main():
    """Exécution complète Test B"""
    print("="*80)
    print("PHASE 4.4.2G - TEST ACTION2 COMME MODIFICATEUR DE RÈGLES")
    print("="*80)
    print("\nHypothèse : ACTION2 modifie l'interprétation future des actions")
    print("Test B : Comparer ACTION4×10 vs ACTION2×N + ACTION4×10")
    
    # Test baseline
    baseline = test_baseline_action4()
    
    # Tests avec différentes valeurs N
    test_values = [1, 5, 10, 25, 50]
    tests = {}
    
    for n in test_values:
        tests[n] = test_action2_modifier(n)
    
    # Analyse comparative
    comparison = compare_results(baseline, tests)
    
    # Verdict
    print("\n" + "="*80)
    print("VERDICT")
    print("="*80)
    
    significant_changes = [r for r in comparison if abs(r['delta_dist']) > 1.0]
    
    if significant_changes:
        print("\n✅ ACTION2 MODIFIE LES RÈGLES DU MOTEUR")
        print("\nChangements significatifs détectés :")
        for r in significant_changes:
            print(f"  ACTION2×{r['n_action2']} : Δ={r['delta_dist']:+.2f} pixels")
        print("\n➡️ ACTION2 agit sur couche LOGIQUE, pas seulement VISUELLE")
    else:
        print("\n❌ PAS DE MODIFICATION DÉTECTÉE")
        print("\nAucun changement significatif (|Δ| < 1.0)")
        print("➡️ ACTION2 pourrait agir sur autre variable non mesurée")
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output = {
        'test': 'action2_modifier',
        'timestamp': timestamp,
        'baseline': baseline,
        'tests': {str(k): v for k, v in tests.items()},
        'comparison': comparison
    }
    
    filename = f"phase_4_4_2G_action2_modifier_{timestamp}.json"
    with open(filename, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Résultats sauvegardés : {filename}")

if __name__ == "__main__":
    main()

# Made with Bob
