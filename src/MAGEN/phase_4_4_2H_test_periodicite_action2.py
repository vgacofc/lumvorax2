#!/usr/bin/env python3
"""
PHASE 4.4.2H - Test Périodicité ACTION2

Hypothèse critique (feedback utilisateur) :
    ACTION2 = action TEMPORELLE (pas spatiale)
    Cycle = 42 steps (84 = 42 × 2)
    
Test A : Vérifier période exacte ACTION2

Protocole :
    1. Exécuter ACTION2 × 1 à ACTION2 × 100
    2. Enregistrer à chaque step :
       - Nombre pixels valeur 11 (timer)
       - Centroïde composante 3#2
       - Hash complet grille
    3. Détecter premier retour exact à état précédent

Objectif : Démontrer périodicité stricte de 42

Auteur: Bob (MAGEN Project)
Date: 2026-06-19
"""

import sys
import os
import numpy as np
import json
import hashlib
from datetime import datetime

# Add arc_integration to path
arc_integration_path = os.path.join(os.path.dirname(__file__), 'arc_integration')
sys.path.insert(0, arc_integration_path)

from ls20_real_wrapper import LS20RealWrapper
from scipy.ndimage import label

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
                    'pixels': len(coords)
                })
    return components

def compute_grid_hash(grid):
    """Calcule hash SHA256 de la grille complète"""
    return hashlib.sha256(grid.tobytes()).hexdigest()

def test_periodicite_action2(max_steps=100):
    """Test périodicité ACTION2 sur 100 steps"""
    print("\n" + "="*80)
    print("TEST PÉRIODICITÉ ACTION2 (Steps 1-100)")
    print("="*80)
    
    env = LS20RealWrapper()
    obs = env.reset()
    
    # État initial
    initial_components = extract_components(obs)
    initial_hash = compute_grid_hash(obs)
    
    print(f"\nÉtat initial :")
    print(f"  Hash grille : {initial_hash[:16]}...")
    if 11 in initial_components:
        print(f"  Timer (11) : {initial_components[11][0]['pixels']} pixels")
    if 3 in initial_components and len(initial_components[3]) >= 2:
        print(f"  Porte (3#2) : ABSENTE")
    else:
        print(f"  Porte (3#2) : ABSENTE")
    
    # Historique états
    states_history = []
    hash_history = {}
    
    # Exécuter ACTION2 × max_steps
    print(f"\nExécution ACTION2 × {max_steps}...")
    for step in range(max_steps):
        obs, reward, done, info = env.step(1)  # ACTION2 = index 1
        
        components = extract_components(obs)
        grid_hash = compute_grid_hash(obs)
        
        # Extraire données critiques
        timer_pixels = None
        if 11 in components and len(components[11]) > 0:
            timer_pixels = components[11][0]['pixels']
        
        door_3_2 = None
        if 3 in components and len(components[3]) >= 2:
            door_3_2 = {
                'centroid': components[3][1]['centroid'],
                'pixels': components[3][1]['pixels']
            }
        
        state = {
            'step': step + 1,
            'hash': grid_hash,
            'timer_pixels': timer_pixels,
            'door_3_2': door_3_2
        }
        states_history.append(state)
        
        # Vérifier si hash déjà vu
        if grid_hash in hash_history:
            first_occurrence = hash_history[grid_hash]
            period = (step + 1) - first_occurrence
            print(f"\n🔥 PÉRIODE DÉTECTÉE au step {step+1}!")
            print(f"   Premier état identique : step {first_occurrence}")
            print(f"   Période = {period} steps")
            
            # Afficher détails
            print(f"\n   État step {first_occurrence}:")
            first_state = states_history[first_occurrence - 1]
            print(f"     Timer: {first_state['timer_pixels']} pixels")
            if first_state['door_3_2']:
                print(f"     Porte: {first_state['door_3_2']['pixels']} pixels, centroid={first_state['door_3_2']['centroid']}")
            
            print(f"\n   État step {step+1}:")
            print(f"     Timer: {timer_pixels} pixels")
            if door_3_2:
                print(f"     Porte: {door_3_2['pixels']} pixels, centroid={door_3_2['centroid']}")
            
            break
        else:
            hash_history[grid_hash] = step + 1
        
        # Afficher steps critiques
        if step + 1 in [1, 10, 20, 30, 40, 41, 42, 43, 44, 45, 50, 60, 70, 80, 84, 85, 90, 100]:
            print(f"  Step {step+1:>3}: Timer={timer_pixels if timer_pixels else 'N/A':>3}, ", end="")
            if door_3_2:
                print(f"Porte={door_3_2['pixels']:>3} pixels, Y={door_3_2['centroid'][1]:.1f}")
            else:
                print(f"Porte=N/A")
    
    return states_history

def analyze_cycle_pattern(states_history):
    """Analyse pattern du cycle détecté"""
    print("\n" + "="*80)
    print("ANALYSE PATTERN CYCLE")
    print("="*80)
    
    # Analyser steps critiques
    critical_steps = [41, 42, 43, 44, 45, 84, 85, 86]
    
    print("\nSteps critiques :")
    for step_num in critical_steps:
        if step_num <= len(states_history):
            state = states_history[step_num - 1]
            print(f"  Step {step_num:>2}: Timer={state['timer_pixels'] if state['timer_pixels'] else 'N/A':>3}, ", end="")
            if state['door_3_2']:
                print(f"Porte={state['door_3_2']['pixels']:>3} pixels, Y={state['door_3_2']['centroid'][1]:.1f}")
            else:
                print(f"Porte=N/A")
    
    # Vérifier hypothèse 84 = 42 × 2
    print("\n" + "="*80)
    print("VÉRIFICATION HYPOTHÈSE : 84 = 42 × 2")
    print("="*80)
    
    if len(states_history) >= 84:
        state_42 = states_history[41]
        state_84 = states_history[83]
        
        print(f"\nComparaison Step 42 vs Step 84 :")
        print(f"  Step 42: Hash={state_42['hash'][:16]}...")
        print(f"  Step 84: Hash={state_84['hash'][:16]}...")
        
        if state_42['hash'] == state_84['hash']:
            print(f"\n✅ PÉRIODE CONFIRMÉE : 42 steps")
            print(f"   84 = 42 × 2 (correspondance mathématique validée)")
        else:
            print(f"\n❌ États différents (période ≠ 42)")

def main():
    """Exécution complète Test A"""
    print("="*80)
    print("PHASE 4.4.2H - TEST PÉRIODICITÉ ACTION2")
    print("="*80)
    print("\nHypothèse : ACTION2 = action TEMPORELLE (cycle 42 steps)")
    print("Objectif : Démontrer périodicité stricte")
    
    # Test périodicité
    states_history = test_periodicite_action2(max_steps=100)
    
    # Analyse pattern
    analyze_cycle_pattern(states_history)
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output = {
        'test': 'periodicite_action2',
        'timestamp': timestamp,
        'states': states_history
    }
    
    filename = f"phase_4_4_2H_periodicite_action2_{timestamp}.json"
    with open(filename, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\n✅ Résultats sauvegardés : {filename}")

if __name__ == "__main__":
    main()

# Made with Bob
