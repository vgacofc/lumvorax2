#!/usr/bin/env python3
"""
SESSION 97 - ANALYSE EXHAUSTIVE ACTION2
========================================

OBJECTIF: Comprendre le mystère ACTION2 (50 états uniques, aucun effet spatial)

CONTEXTE Session 96:
- ACTION2 produit 50 états uniques
- Distance reste à 0.00 (aucun effet spatial apparent)
- Seul mystère restant dans la cartographie

HYPOTHÈSES À TESTER:
1. ACTION2 effectue des rotations/permutations cachées
2. ACTION2 prépare une configuration pour victoire
3. ACTION2 modifie des valeurs sans déplacer les blocs
4. ACTION2 a un effet cumulatif (visible après N steps)

MÉTHODOLOGIE:
- Capturer les 50 états uniques de ACTION2
- Analyser transitions pixel-par-pixel à chaque step
- Identifier patterns cachés (rotations, permutations, cycles)
- Mesurer distances et overlaps à chaque step
- Tester si ACTION2 prépare configuration pour ACTION4

DURÉE ESTIMÉE: 30 minutes
"""

import os
import sys
import json
import hashlib
from datetime import datetime
from typing import Dict, List, Tuple, Any
import numpy as np
from scipy.ndimage import label
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# Import ARC-AGI
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

def hash_frame(frame: np.ndarray) -> str:
    """Créer un hash unique pour un état de grille"""
    if isinstance(frame, list):
        frame = np.array(frame)
    return hashlib.md5(frame.tobytes()).hexdigest()

def measure_distance_pixel_to_pixel(frame: np.ndarray) -> float:
    """Mesurer distance minimale pixel-à-pixel entre blocs (9,12) et goals (8)"""
    goals = np.argwhere(frame == 8)
    blocks_9 = np.argwhere(frame == 9)
    blocks_12 = np.argwhere(frame == 12)
    
    if len(goals) == 0 or (len(blocks_9) == 0 and len(blocks_12) == 0):
        return float('inf')
    
    all_blocks = np.vstack([blocks_9, blocks_12]) if len(blocks_9) > 0 and len(blocks_12) > 0 else (blocks_9 if len(blocks_9) > 0 else blocks_12)
    
    min_distance = float('inf')
    for block in all_blocks:
        for goal in goals:
            dist = np.sqrt(np.sum((block - goal) ** 2))
            min_distance = min(min_distance, dist)
    
    return min_distance

def measure_overlap(frame: np.ndarray) -> int:
    """Mesurer nombre de pixels où blocs (9,12) overlappent goals (8)"""
    goals = np.argwhere(frame == 8)
    blocks_9 = np.argwhere(frame == 9)
    blocks_12 = np.argwhere(frame == 12)
    
    overlap = 0
    for goal in goals:
        for block in np.vstack([blocks_9, blocks_12]) if len(blocks_9) > 0 and len(blocks_12) > 0 else (blocks_9 if len(blocks_9) > 0 else blocks_12):
            if np.array_equal(goal, block):
                overlap += 1
    
    return overlap

def analyze_value_distribution(frame: np.ndarray) -> Dict[int, int]:
    """Analyser distribution des valeurs dans la grille"""
    unique, counts = np.unique(frame, return_counts=True)
    return dict(zip(unique.tolist(), counts.tolist()))

def compare_frames(frame1: np.ndarray, frame2: np.ndarray) -> Dict[str, Any]:
    """Comparer deux frames et identifier les changements"""
    diff = frame1 != frame2
    num_changes = np.sum(diff)
    
    if num_changes == 0:
        return {
            'num_changes': 0,
            'transitions': {},
            'positions': []
        }
    
    # Identifier positions et transitions
    positions = np.argwhere(diff)
    transitions = {}
    
    for pos in positions:
        y, x = pos
        old_val = frame1[y, x]
        new_val = frame2[y, x]
        key = f"{old_val}→{new_val}"
        transitions[key] = transitions.get(key, 0) + 1
    
    return {
        'num_changes': int(num_changes),
        'transitions': transitions,
        'positions': positions.tolist()
    }

def analyze_spatial_patterns(frame: np.ndarray) -> Dict[str, Any]:
    """Analyser patterns spatiaux (clusters, lignes, colonnes)"""
    # Analyser composantes connectées pour valeur 9
    mask_9 = (frame == 9).astype(int)
    labeled_9, num_9 = label(mask_9)
    
    # Analyser composantes connectées pour valeur 12
    mask_12 = (frame == 12).astype(int)
    labeled_12, num_12 = label(mask_12)
    
    # Calculer centroids
    blocks_9 = np.argwhere(frame == 9)
    blocks_12 = np.argwhere(frame == 12)
    
    centroid_9 = blocks_9.mean(axis=0).tolist() if len(blocks_9) > 0 else None
    centroid_12 = blocks_12.mean(axis=0).tolist() if len(blocks_12) > 0 else None
    
    return {
        'num_components_9': int(num_9),
        'num_components_12': int(num_12),
        'centroid_9': centroid_9,
        'centroid_12': centroid_12,
        'num_pixels_9': len(blocks_9),
        'num_pixels_12': len(blocks_12)
    }

def detect_rotation(frame1: np.ndarray, frame2: np.ndarray) -> Dict[str, Any]:
    """Détecter si frame2 est une rotation de frame1"""
    # Tester rotations 90°, 180°, 270°
    rotations = {
        '90': np.rot90(frame1, k=1),
        '180': np.rot90(frame1, k=2),
        '270': np.rot90(frame1, k=3)
    }
    
    for angle, rotated in rotations.items():
        if np.array_equal(rotated, frame2):
            return {'is_rotation': True, 'angle': angle}
    
    return {'is_rotation': False, 'angle': None}

def detect_reflection(frame1: np.ndarray, frame2: np.ndarray) -> Dict[str, Any]:
    """Détecter si frame2 est une réflexion de frame1"""
    # Tester réflexions horizontale et verticale
    flip_h = np.fliplr(frame1)
    flip_v = np.flipud(frame1)
    
    if np.array_equal(flip_h, frame2):
        return {'is_reflection': True, 'axis': 'horizontal'}
    elif np.array_equal(flip_v, frame2):
        return {'is_reflection': True, 'axis': 'vertical'}
    
    return {'is_reflection': False, 'axis': None}

def visualize_action2_states(states: List[np.ndarray], output_path: str):
    """Visualiser les premiers états de ACTION2"""
    num_states = min(10, len(states))  # Visualiser 10 premiers états
    
    fig, axes = plt.subplots(2, 5, figsize=(20, 8))
    fig.suptitle('ACTION2 - Premiers 10 États', fontsize=16, fontweight='bold')
    
    for idx in range(num_states):
        ax = axes[idx // 5, idx % 5]
        ax.imshow(states[idx], cmap='tab20', interpolation='nearest')
        ax.set_title(f'Step {idx}', fontsize=10)
        ax.axis('off')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    
    print(f"\n✅ Visualisation sauvegardée: {output_path}")

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
    hashes.add(hash_frame(frame_initial))
    
    metadata.append({
        'step': 0,
        'hash': hash_frame(frame_initial),
        'distance': measure_distance_pixel_to_pixel(frame_initial),
        'overlap': measure_overlap(frame_initial),
        'distribution': analyze_value_distribution(frame_initial),
        'spatial': analyze_spatial_patterns(frame_initial),
        'victory': False
    })
    
    print(f"\nÉtat initial capturé")
    print(f"  Hash: {hash_frame(frame_initial)[:16]}...")
    print(f"  Distance: {metadata[0]['distance']:.2f}")
    print(f"  Distribution: {metadata[0]['distribution']}")
    
    # Exécuter ACTION2 jusqu'à 50 états uniques ou cycle
    max_steps = 100
    for step in range(1, max_steps + 1):
        obs, reward, done, info = env.step(GameAction.ACTION2)
        
        frame_current = extract_frame(obs)
        frame_hash = hash_frame(frame_current)
        
        # Vérifier si nouvel état
        if frame_hash not in hashes:
            states.append(frame_current.copy())
            hashes.add(frame_hash)
            
            metadata.append({
                'step': step,
                'hash': frame_hash,
                'distance': measure_distance_pixel_to_pixel(frame_current),
                'overlap': measure_overlap(frame_current),
                'distribution': analyze_value_distribution(frame_current),
                'spatial': analyze_spatial_patterns(frame_current),
                'victory': done and reward > 0
            })
            
            if len(states) % 10 == 0:
                print(f"\nStep {step}: {len(states)} états uniques capturés")
                print(f"  Distance: {metadata[-1]['distance']:.2f}")
                print(f"  Distribution: {metadata[-1]['distribution']}")
        
        # Arrêter si cycle détecté (retour à un état précédent)
        if frame_hash in hashes and step > len(states):
            print(f"\n🔄 Cycle détecté au step {step}")
            print(f"   Retour à l'état: {frame_hash[:16]}...")
            break
        
        # Arrêter si 50 états uniques atteints
        if len(states) >= 50:
            print(f"\n✅ 50 états uniques capturés au step {step}")
            break
        
        if done:
            print(f"\n{'✅' if reward > 0 else '❌'} Partie terminée au step {step}")
            print(f"   Reward: {reward}")
            break
    
    print(f"\n=== RÉSUMÉ CAPTURE ===")
    print(f"Total états uniques: {len(states)}")
    print(f"Total steps exécutés: {step}")
    print(f"Ratio états/steps: {len(states)/step:.2%}")
    
    return {
        'states': states,
        'metadata': metadata,
        'num_unique_states': len(states),
        'total_steps': step
    }

def priorite2_analyser_transitions():
    """PRIORITÉ 2: Analyser transitions pixel-par-pixel entre états consécutifs"""
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
        obs, reward, done, info = env.step(GameAction.ACTION2)
        frame_curr = extract_frame(obs).copy()
        
        # Comparer frames
        comparison = compare_frames(frame_prev, frame_curr)
        
        # Détecter rotations/réflexions
        rotation = detect_rotation(frame_prev, frame_curr)
        reflection = detect_reflection(frame_prev, frame_curr)
        
        transitions_summary.append({
            'step': step,
            'num_changes': comparison['num_changes'],
            'transitions': comparison['transitions'],
            'rotation': rotation,
            'reflection': reflection
        })
        
        if step <= 5 or comparison['num_changes'] > 0:
            print(f"\nStep {step}:")
            print(f"  Changements: {comparison['num_changes']} pixels")
            if comparison['transitions']:
                print(f"  Transitions: {comparison['transitions']}")
            if rotation['is_rotation']:
                print(f"  🔄 Rotation détectée: {rotation['angle']}°")
            if reflection['is_reflection']:
                print(f"  🪞 Réflexion détectée: {reflection['axis']}")
        
        frame_prev = frame_curr
        
        if done:
            break
    
    # Statistiques globales
    total_changes = sum(t['num_changes'] for t in transitions_summary)
    avg_changes = total_changes / len(transitions_summary) if transitions_summary else 0
    
    print(f"\n=== STATISTIQUES TRANSITIONS ===")
    print(f"Total transitions analysées: {len(transitions_summary)}")
    print(f"Total changements: {total_changes} pixels")
    print(f"Moyenne changements/step: {avg_changes:.2f} pixels")
    
    # Identifier patterns
    num_rotations = sum(1 for t in transitions_summary if t['rotation']['is_rotation'])
    num_reflections = sum(1 for t in transitions_summary if t['reflection']['is_reflection'])
    
    print(f"Rotations détectées: {num_rotations}")
    print(f"Réflexions détectées: {num_reflections}")
    
    return {
        'transitions': transitions_summary,
        'total_changes': total_changes,
        'avg_changes_per_step': avg_changes,
        'num_rotations': num_rotations,
        'num_reflections': num_reflections
    }

def priorite3_tester_effet_cumulatif():
    """PRIORITÉ 3: Tester si ACTION2 a un effet cumulatif"""
    print("\n" + "="*80)
    print("PRIORITÉ 3: TEST EFFET CUMULATIF ACTION2")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    
    # Tester différentes longueurs de séquences ACTION2
    sequences = [5, 10, 20, 30, 40, 50]
    results = []
    
    for seq_length in sequences:
        env = arcade.make("ls20-9607627b")
        obs = env.reset()
        
        frame_initial = extract_frame(obs).copy()
        dist_initial = measure_distance_pixel_to_pixel(frame_initial)
        
        # Exécuter ACTION2 × seq_length
        for _ in range(seq_length):
            obs, reward, done, info = env.step(GameAction.ACTION2)
            if done:
                break
        
        frame_final = extract_frame(obs).copy()
        dist_final = measure_distance_pixel_to_pixel(frame_final)
        
        # Comparer états
        comparison = compare_frames(frame_initial, frame_final)
        
        results.append({
            'sequence_length': seq_length,
            'distance_initial': dist_initial,
            'distance_final': dist_final,
            'distance_delta': dist_final - dist_initial,
            'num_changes': comparison['num_changes'],
            'victory': done and reward > 0
        })
        
        print(f"\nACTION2×{seq_length}:")
        print(f"  Distance: {dist_initial:.2f} → {dist_final:.2f} (Δ={dist_final - dist_initial:+.2f})")
        print(f"  Changements: {comparison['num_changes']} pixels")
        print(f"  Victoire: {done and reward > 0}")
    
    print(f"\n=== ANALYSE EFFET CUMULATIF ===")
    
    # Vérifier si distance change avec longueur séquence
    distances_change = any(r['distance_delta'] != 0 for r in results)
    print(f"Distance varie avec longueur: {distances_change}")
    
    # Vérifier si changements augmentent avec longueur
    changes_increase = all(results[i]['num_changes'] <= results[i+1]['num_changes'] 
                          for i in range(len(results)-1))
    print(f"Changements augmentent: {changes_increase}")
    
    return {
        'results': results,
        'distances_vary': distances_change,
        'changes_increase': changes_increase
    }

def priorite4_tester_preparation_action4():
    """PRIORITÉ 4: Tester si ACTION2 prépare configuration pour ACTION4"""
    print("\n" + "="*80)
    print("PRIORITÉ 4: TEST PRÉPARATION POUR ACTION4")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    
    # Tester différentes combinaisons ACTION2 + ACTION4
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
        
        dist_initial = measure_distance_pixel_to_pixel(obs.frame)
        
        # Exécuter ACTION2
        for _ in range(num_action2):
            obs, reward, done, info = env.step(GameAction.ACTION2)
            if done:
                break
        
        dist_after_action2 = measure_distance_pixel_to_pixel(extract_frame(obs))
        
        # Exécuter ACTION4
        for _ in range(num_action4):
            obs, reward, done, info = env.step(GameAction.ACTION4)
            if done:
                break
        
        dist_final = measure_distance_pixel_to_pixel(extract_frame(obs))
        
        results.append({
            'label': label,
            'num_action2': num_action2,
            'num_action4': num_action4,
            'distance_initial': dist_initial,
            'distance_after_action2': dist_after_action2,
            'distance_final': dist_final,
            'improvement': dist_initial - dist_final,
            'victory': done and reward > 0
        })
        
        print(f"\n{label}:")
        print(f"  Distance: {dist_initial:.2f} → {dist_after_action2:.2f} → {dist_final:.2f}")
        print(f"  Amélioration totale: {dist_initial - dist_final:.2f}")
        print(f"  Victoire: {done and reward > 0}")
    
    # Identifier meilleure combinaison
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
    
    # Visualiser états
    if results['priorite1']['states']:
        output_path = os.path.join(os.path.dirname(__file__), 'session97_action2_states.png')
        visualize_action2_states(results['priorite1']['states'], output_path)
    
    # PRIORITÉ 2: Analyser transitions
    results['priorite2'] = priorite2_analyser_transitions()
    
    # PRIORITÉ 3: Tester effet cumulatif
    results['priorite3'] = priorite3_tester_effet_cumulatif()
    
    # PRIORITÉ 4: Tester préparation ACTION4
    results['priorite4'] = priorite4_tester_preparation_action4()
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = os.path.join(
        os.path.dirname(__file__),
        f'session97_action2_exhaustive_{timestamp}.json'
    )
    
    # Convertir numpy arrays en listes pour JSON
    results_serializable = {
        'priorite1': {
            'num_unique_states': results['priorite1']['num_unique_states'],
            'total_steps': results['priorite1']['total_steps'],
            'metadata': results['priorite1']['metadata']
        },
        'priorite2': results['priorite2'],
        'priorite3': results['priorite3'],
        'priorite4': results['priorite4']
    }
    
    with open(output_file, 'w') as f:
        json.dump(results_serializable, f, indent=2)
    
    print("\n" + "="*80)
    print(f"Résultats sauvegardés: {output_file}")
    print("="*80)

if __name__ == "__main__":
    main()

# Made with Bob
