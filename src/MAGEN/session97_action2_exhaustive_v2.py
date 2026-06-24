#!/usr/bin/env python3
"""
SESSION 97 - ANALYSE EXHAUSTIVE ACTION2 (Version simplifiée)
=============================================================

OBJECTIF: Comprendre le mystère ACTION2 (50 états uniques, aucun effet spatial)

4 PRIORITÉS:
1. Capturer les 50 états uniques de ACTION2
2. Analyser transitions pixel-par-pixel
3. Tester effet cumulatif
4. Tester préparation pour ACTION4

Date: 2026-06-18
"""

import os
import json
import hashlib
from datetime import datetime
from pathlib import Path
import numpy as np
from scipy.ndimage import label
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

def extract_frame(obs):
    """Extraire la grille 64x64 depuis l'observation"""
    frame = obs.frame
    if isinstance(frame, list):
        frame = np.array(frame)
    if frame.ndim == 3:
        frame = frame[0]
    return frame

def hash_frame(frame):
    """Créer un hash unique pour un état de grille"""
    if isinstance(frame, list):
        frame = np.array(frame)
    return hashlib.md5(frame.tobytes()).hexdigest()

def measure_distance(frame):
    """Distance minimale pixel-à-pixel entre blocs (9,12) et goals (8)"""
    goals = np.argwhere(frame == 8)
    blocks = np.argwhere((frame == 9) | (frame == 12))
    
    if len(goals) == 0 or len(blocks) == 0:
        return float('inf')
    
    min_dist = float('inf')
    for block in blocks:
        for goal in goals:
            dist = np.sqrt(np.sum((block - goal) ** 2))
            min_dist = min(min_dist, dist)
    
    return min_dist

def analyze_distribution(frame):
    """Analyser distribution des valeurs"""
    unique, counts = np.unique(frame, return_counts=True)
    return dict(zip(unique.tolist(), counts.tolist()))

def compare_frames(frame1, frame2):
    """Comparer deux frames"""
    diff = frame1 != frame2
    num_changes = np.sum(diff)
    
    if num_changes == 0:
        return {'num_changes': 0, 'transitions': {}}
    
    positions = np.argwhere(diff)
    transitions = {}
    
    for pos in positions:
        y, x = pos
        old_val = frame1[y, x]
        new_val = frame2[y, x]
        key = f"{old_val}→{new_val}"
        transitions[key] = transitions.get(key, 0) + 1
    
    return {'num_changes': int(num_changes), 'transitions': transitions}

def priorite1_capturer_50_etats():
    """PRIORITÉ 1: Capturer les 50 états uniques de ACTION2"""
    print("\n" + "="*80)
    print("PRIORITÉ 1: CAPTURE DES 50 ÉTATS ACTION2")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    obs = env.reset()
    
    states = []
    hashes = set()
    metadata = []
    
    frame_initial = extract_frame(obs).copy()
    states.append(frame_initial)
    hash_initial = hash_frame(frame_initial)
    hashes.add(hash_initial)
    
    metadata.append({
        'step': 0,
        'hash': hash_initial,
        'distance': measure_distance(frame_initial),
        'distribution': analyze_distribution(frame_initial)
    })
    
    print(f"\nÉtat initial capturé")
    print(f"  Hash: {hash_initial[:16]}...")
    print(f"  Distance: {metadata[0]['distance']:.2f}")
    print(f"  Distribution: {metadata[0]['distribution']}")
    
    # Exécuter ACTION2 jusqu'à 50 états uniques ou cycle
    max_steps = 100
    for step in range(1, max_steps + 1):
        obs = env.step(GameAction.ACTION2)
        
        frame_current = extract_frame(obs)
        frame_hash = hash_frame(frame_current)
        
        # Vérifier si nouvel état
        if frame_hash not in hashes:
            states.append(frame_current.copy())
            hashes.add(frame_hash)
            
            metadata.append({
                'step': step,
                'hash': frame_hash,
                'distance': measure_distance(frame_current),
                'distribution': analyze_distribution(frame_current)
            })
            
            if len(states) % 10 == 0:
                print(f"\nStep {step}: {len(states)} états uniques capturés")
                print(f"  Distance: {metadata[-1]['distance']:.2f}")
        
        # Arrêter si cycle détecté
        if frame_hash in hashes and step > len(states):
            print(f"\n🔄 Cycle détecté au step {step}")
            break
        
        # Arrêter si 50 états uniques atteints
        if len(states) >= 50:
            print(f"\n✅ 50 états uniques capturés au step {step}")
            break
    
    print(f"\n=== RÉSUMÉ CAPTURE ===")
    print(f"Total états uniques: {len(states)}")
    print(f"Total steps exécutés: {step}")
    
    return {
        'num_unique_states': len(states),
        'total_steps': step,
        'metadata': metadata
    }

def priorite2_analyser_transitions():
    """PRIORITÉ 2: Analyser transitions pixel-par-pixel"""
    print("\n" + "="*80)
    print("PRIORITÉ 2: ANALYSE TRANSITIONS PIXEL-PAR-PIXEL")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    obs = env.reset()
    
    frame_prev = extract_frame(obs).copy()
    transitions_summary = []
    
    # Analyser 20 premières transitions
    for step in range(1, 21):
        obs = env.step(GameAction.ACTION2)
        frame_curr = extract_frame(obs).copy()
        
        comparison = compare_frames(frame_prev, frame_curr)
        
        transitions_summary.append({
            'step': step,
            'num_changes': comparison['num_changes'],
            'transitions': comparison['transitions']
        })
        
        if step <= 5 or comparison['num_changes'] > 0:
            print(f"\nStep {step}:")
            print(f"  Changements: {comparison['num_changes']} pixels")
            if comparison['transitions']:
                print(f"  Transitions: {comparison['transitions']}")
        
        frame_prev = frame_curr
    
    total_changes = sum(t['num_changes'] for t in transitions_summary)
    avg_changes = total_changes / len(transitions_summary)
    
    print(f"\n=== STATISTIQUES TRANSITIONS ===")
    print(f"Total transitions analysées: {len(transitions_summary)}")
    print(f"Total changements: {total_changes} pixels")
    print(f"Moyenne changements/step: {avg_changes:.2f} pixels")
    
    return {
        'transitions': transitions_summary,
        'total_changes': total_changes,
        'avg_changes_per_step': avg_changes
    }

def priorite3_tester_effet_cumulatif():
    """PRIORITÉ 3: Tester si ACTION2 a un effet cumulatif"""
    print("\n" + "="*80)
    print("PRIORITÉ 3: TEST EFFET CUMULATIF ACTION2")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    
    sequences = [5, 10, 20, 30, 40, 50]
    results = []
    
    for seq_length in sequences:
        env = arcade.make("ls20-9607627b")
        obs = env.reset()
        
        frame_initial = extract_frame(obs).copy()
        dist_initial = measure_distance(frame_initial)
        
        # Exécuter ACTION2 × seq_length
        for _ in range(seq_length):
            obs = env.step(GameAction.ACTION2)
        
        frame_final = extract_frame(obs).copy()
        dist_final = measure_distance(frame_final)
        
        comparison = compare_frames(frame_initial, frame_final)
        
        results.append({
            'sequence_length': seq_length,
            'distance_initial': dist_initial,
            'distance_final': dist_final,
            'distance_delta': dist_final - dist_initial,
            'num_changes': comparison['num_changes']
        })
        
        print(f"\nACTION2×{seq_length}:")
        print(f"  Distance: {dist_initial:.2f} → {dist_final:.2f} (Δ={dist_final - dist_initial:+.2f})")
        print(f"  Changements: {comparison['num_changes']} pixels")
    
    distances_change = any(r['distance_delta'] != 0 for r in results)
    print(f"\n=== ANALYSE EFFET CUMULATIF ===")
    print(f"Distance varie avec longueur: {distances_change}")
    
    return {
        'results': results,
        'distances_vary': distances_change
    }

def priorite4_tester_preparation_action4():
    """PRIORITÉ 4: Tester si ACTION2 prépare configuration pour ACTION4"""
    print("\n" + "="*80)
    print("PRIORITÉ 4: TEST PRÉPARATION POUR ACTION4")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    
    combinations = [
        ('ACTION2×0 + ACTION4×10', 0, 10),
        ('ACTION2×5 + ACTION4×10', 5, 10),
        ('ACTION2×10 + ACTION4×10', 10, 10),
        ('ACTION2×20 + ACTION4×10', 20, 10),
        ('ACTION2×30 + ACTION4×10', 30, 10),
    ]
    
    results = []
    
    for label, num_action2, num_action4 in combinations:
        env = arcade.make("ls20-9607627b")
        obs = env.reset()
        
        dist_initial = measure_distance(extract_frame(obs))
        
        # Exécuter ACTION2
        for _ in range(num_action2):
            obs = env.step(GameAction.ACTION2)
        
        dist_after_action2 = measure_distance(extract_frame(obs))
        
        # Exécuter ACTION4
        for _ in range(num_action4):
            obs = env.step(GameAction.ACTION4)
        
        dist_final = measure_distance(extract_frame(obs))
        
        results.append({
            'label': label,
            'num_action2': num_action2,
            'num_action4': num_action4,
            'distance_initial': dist_initial,
            'distance_after_action2': dist_after_action2,
            'distance_final': dist_final,
            'improvement': dist_initial - dist_final
        })
        
        print(f"\n{label}:")
        print(f"  Distance: {dist_initial:.2f} → {dist_after_action2:.2f} → {dist_final:.2f}")
        print(f"  Amélioration totale: {dist_initial - dist_final:.2f}")
    
    best = max(results, key=lambda r: r['improvement'])
    
    print(f"\n=== MEILLEURE COMBINAISON ===")
    print(f"{best['label']}")
    print(f"  Amélioration: {best['improvement']:.2f}")
    print(f"  Distance finale: {best['distance_final']:.2f}")
    
    return {
        'results': results,
        'best': best
    }

def main():
    """Fonction principale"""
    print("="*80)
    print("SESSION 97 - ANALYSE EXHAUSTIVE ACTION2")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print("="*80)
    
    results = {}
    
    # PRIORITÉ 1: Capturer 50 états
    results['priorite1'] = priorite1_capturer_50_etats()
    
    # PRIORITÉ 2: Analyser transitions
    results['priorite2'] = priorite2_analyser_transitions()
    
    # PRIORITÉ 3: Tester effet cumulatif
    results['priorite3'] = priorite3_tester_effet_cumulatif()
    
    # PRIORITÉ 4: Tester préparation ACTION4
    results['priorite4'] = priorite4_tester_preparation_action4()
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = Path(__file__).parent / f"session97_action2_exhaustive_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print("\n" + "="*80)
    print(f"Résultats sauvegardés: {output_file}")
    print("="*80)

if __name__ == "__main__":
    main()

# Made with Bob
