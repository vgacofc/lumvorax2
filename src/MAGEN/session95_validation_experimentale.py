#!/usr/bin/env python3
"""
SESSION 95 - VALIDATION EXPÉRIMENTALE
=====================================

OBJECTIF: Valider les hypothèses de Session 94 avec tests expérimentaux

5 PRIORITÉS:
1. Valider métrique distance (pixel-à-pixel vs centroïde) - 15 min
2. Tester ACTION4 × 42 steps complets (limite timer) - 10 min
3. Explorer séquences mixtes (longueur 2-3) - 30 min
4. Vérifier cycle ACTION1 (hash MD5 complet) - 5 min
5. Tester condition de victoire réelle - variable

Date: 2026-06-18
Auteur: Bob (Mode Advanced)
"""

import sys
import os
import json
import numpy as np
import hashlib
from datetime import datetime
from pathlib import Path
from scipy.ndimage import label
from collections import defaultdict

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


def measure_distance_centroid(frame):
    """Distance centroïde → goals (méthode Session 94)"""
    goals = np.argwhere(frame == 8)
    
    # Trouver composantes 9 et 12
    mask_9 = (frame == 9)
    labeled_9, num_9 = label(mask_9)
    
    mask_12 = (frame == 12)
    labeled_12, num_12 = label(mask_12)
    
    distances = []
    
    # Distance composantes 9
    for i in range(1, num_9 + 1):
        positions = np.argwhere(labeled_9 == i)
        centroid = positions.mean(axis=0)
        min_dist = min(np.linalg.norm(centroid - goal) for goal in goals)
        distances.append(min_dist)
    
    # Distance composantes 12
    for i in range(1, num_12 + 1):
        positions = np.argwhere(labeled_12 == i)
        centroid = positions.mean(axis=0)
        min_dist = min(np.linalg.norm(centroid - goal) for goal in goals)
        distances.append(min_dist)
    
    if not distances:
        return float('inf')
    
    return min(distances)


def measure_distance_pixel_to_pixel(frame):
    """Distance MINIMALE pixel-à-pixel (méthode corrigée)"""
    goals = np.argwhere(frame == 8)
    blocks = np.argwhere((frame == 9) | (frame == 12))
    
    if len(goals) == 0 or len(blocks) == 0:
        return {
            'min': float('inf'),
            'mean': float('inf'),
            'max': float('inf')
        }
    
    # Distance minimale pour chaque pixel de bloc
    min_distances = []
    for block_pixel in blocks:
        min_dist = min(np.linalg.norm(block_pixel - goal) for goal in goals)
        min_distances.append(min_dist)
    
    return {
        'min': float(min(min_distances)),
        'mean': float(np.mean(min_distances)),
        'max': float(max(min_distances))
    }


def hash_frame_md5(frame):
    """Hash MD5 complet pour détecter cycles"""
    return hashlib.md5(frame.tobytes()).hexdigest()


def extract_frame(obs):
    """Extraire la grille 64x64 depuis l'observation"""
    frame = obs.frame
    if isinstance(frame, list):
        frame = np.array(frame)
    if frame.ndim == 3:
        frame = frame[0]
    return frame


def priorite1_valider_metrique_distance(env):
    """
    PRIORITÉ 1: Valider métrique distance (15 min)
    Comparer centroïde vs pixel-à-pixel pour ACTION4 × 50
    """
    print("\n" + "="*80)
    print("PRIORITÉ 1: VALIDATION MÉTRIQUE DISTANCE")
    print("="*80)
    
    obs = env.reset()
    frame = extract_frame(obs)
    
    # Distance initiale
    dist_centroid_init = measure_distance_centroid(frame)
    dist_pixel_init = measure_distance_pixel_to_pixel(frame)
    
    print(f"\nDistance INITIALE:")
    print(f"  Centroïde: {dist_centroid_init:.2f}")
    print(f"  Pixel-à-pixel (min): {dist_pixel_init['min']:.2f}")
    print(f"  Pixel-à-pixel (mean): {dist_pixel_init['mean']:.2f}")
    print(f"  Pixel-à-pixel (max): {dist_pixel_init['max']:.2f}")
    
    # Appliquer ACTION4 × 50
    for step in range(50):
        obs = env.step(GameAction.ACTION4)
    
    frame_final = extract_frame(obs)
    
    # Distance finale
    dist_centroid_final = measure_distance_centroid(frame_final)
    dist_pixel_final = measure_distance_pixel_to_pixel(frame_final)
    
    print(f"\nDistance FINALE (après 50×ACTION4):")
    print(f"  Centroïde: {dist_centroid_final:.2f}")
    print(f"  Pixel-à-pixel (min): {dist_pixel_final['min']:.2f}")
    print(f"  Pixel-à-pixel (mean): {dist_pixel_final['mean']:.2f}")
    print(f"  Pixel-à-pixel (max): {dist_pixel_final['max']:.2f}")
    
    # Variation
    print(f"\nVARIATION:")
    print(f"  Centroïde: {dist_centroid_final - dist_centroid_init:.2f}")
    print(f"  Pixel-à-pixel (min): {dist_pixel_final['min'] - dist_pixel_init['min']:.2f}")
    print(f"  Pixel-à-pixel (mean): {dist_pixel_final['mean'] - dist_pixel_init['mean']:.2f}")
    
    # Conclusion
    if dist_pixel_final['min'] < dist_pixel_init['min']:
        print("\n✅ CONFIRMÉ: ACTION4 rapproche réellement (pixel-à-pixel)")
    else:
        print("\n❌ INFIRMÉ: ACTION4 n'approche pas (métrique centroïde trompeuse)")
    
    return {
        'centroid_init': dist_centroid_init,
        'centroid_final': dist_centroid_final,
        'pixel_init': dist_pixel_init,
        'pixel_final': dist_pixel_final
    }


def priorite2_tester_action4_complet(env):
    """
    PRIORITÉ 2: Tester ACTION4 × 42 steps (10 min)
    Limite timer = 42 steps max
    """
    print("\n" + "="*80)
    print("PRIORITÉ 2: TEST ACTION4 × 42 STEPS (LIMITE TIMER)")
    print("="*80)
    
    obs = env.reset()
    
    for step in range(42):
        frame = extract_frame(obs)
        
        dist_pixel = measure_distance_pixel_to_pixel(frame)
        pixel_11 = np.sum(frame == 11)
        
        if step % 10 == 0 or step == 41:
            print(f"\nStep {step}:")
            print(f"  Distance (min): {dist_pixel['min']:.2f}")
            print(f"  Distance (mean): {dist_pixel['mean']:.2f}")
            print(f"  Pixel 11: {pixel_11}")
            print(f"  Levels completed: {obs.levels_completed}")
            print(f"  Game state: {obs.state}")
        
        # Vérifier victoire
        if obs.levels_completed > 0:
            print(f"\n🎉 VICTOIRE au step {step}!")
            return {
                'victory': True,
                'step': step,
                'distance_final': dist_pixel
            }
        
        # Vérifier fin de jeu
        if obs.state != GameState.NOT_FINISHED:
            print(f"\n⚠️ Jeu terminé: {obs.state}")
            break
        
        obs = env.step(GameAction.ACTION4)
    
    # État final
    frame_final = extract_frame(obs)
    dist_final = measure_distance_pixel_to_pixel(frame_final)
    
    print(f"\n❌ Pas de victoire après 42 steps")
    print(f"Distance finale (min): {dist_final['min']:.2f}")
    print(f"Distance finale (mean): {dist_final['mean']:.2f}")
    
    return {
        'victory': False,
        'step': 42,
        'distance_final': dist_final
    }


def priorite3_explorer_sequences_mixtes(env):
    """
    PRIORITÉ 3: Explorer séquences mixtes (30 min)
    Longueur 2-3
    """
    print("\n" + "="*80)
    print("PRIORITÉ 3: EXPLORATION SÉQUENCES MIXTES (LONGUEUR 2-3)")
    print("="*80)
    
    actions = [
        (GameAction.ACTION1, "ACTION1"),
        (GameAction.ACTION2, "ACTION2"),
        (GameAction.ACTION3, "ACTION3"),
        (GameAction.ACTION4, "ACTION4")
    ]
    
    results_length2 = []
    results_length3 = []
    
    # Longueur 2
    print("\n--- SÉQUENCES LONGUEUR 2 ---")
    for a1, name1 in actions:
        for a2, name2 in actions:
            obs = env.reset()
            
            obs = env.step(a1)
            obs = env.step(a2)
            
            frame = extract_frame(obs)
            dist = measure_distance_pixel_to_pixel(frame)
            
            results_length2.append({
                'sequence': [name1, name2],
                'distance_min': dist['min'],
                'distance_mean': dist['mean'],
                'victory': obs.levels_completed > 0
            })
    
    # Trier par distance minimale
    results_length2.sort(key=lambda x: x['distance_min'])
    
    print("\nMeilleures séquences (longueur 2):")
    for i, r in enumerate(results_length2[:5]):
        print(f"  {i+1}. {r['sequence']}: dist_min={r['distance_min']:.2f}, victoire={r['victory']}")
    
    # Longueur 3 (seulement les 5 meilleures combinaisons de longueur 2)
    print("\n--- SÉQUENCES LONGUEUR 3 (TOP 5 de longueur 2) ---")
    for result2 in results_length2[:5]:
        seq2 = result2['sequence']
        
        for a3, name3 in actions:
            obs = env.reset()
            
            # Rejouer séquence longueur 2
            for name in seq2:
                action = next(a for a, n in actions if n == name)
                obs = env.step(action)
            
            # Ajouter 3ème action
            obs = env.step(a3)
            
            frame = extract_frame(obs)
            dist = measure_distance_pixel_to_pixel(frame)
            
            results_length3.append({
                'sequence': seq2 + [name3],
                'distance_min': dist['min'],
                'distance_mean': dist['mean'],
                'victory': obs.levels_completed > 0
            })
    
    # Trier par distance minimale
    results_length3.sort(key=lambda x: x['distance_min'])
    
    print("\nMeilleures séquences (longueur 3):")
    for i, r in enumerate(results_length3[:5]):
        print(f"  {i+1}. {r['sequence']}: dist_min={r['distance_min']:.2f}, victoire={r['victory']}")
    
    return {
        'length2': results_length2,
        'length3': results_length3
    }


def priorite4_verifier_cycle_action1(env):
    """
    PRIORITÉ 4: Vérifier cycle ACTION1 (5 min)
    Hash MD5 complet
    """
    print("\n" + "="*80)
    print("PRIORITÉ 4: VÉRIFICATION CYCLE ACTION1 (HASH MD5)")
    print("="*80)
    
    obs = env.reset()
    
    states = []
    for step in range(15):
        frame = extract_frame(obs)
        state_hash = hash_frame_md5(frame)
        
        states.append({
            'step': step,
            'hash': state_hash
        })
        
        obs = env.step(GameAction.ACTION1)
    
    # Détecter cycles
    print("\nDétection de cycles:")
    for i, state_i in enumerate(states):
        for j, state_j in enumerate(states[i+1:], start=i+1):
            if state_i['hash'] == state_j['hash']:
                print(f"  État du step {i} == État du step {j}")
                print(f"  Cycle de longueur: {j - i}")
                print(f"  Hash: {state_i['hash'][:16]}...")
                
                return {
                    'cycle_detected': True,
                    'cycle_start': i,
                    'cycle_end': j,
                    'cycle_length': j - i
                }
    
    print("  Aucun cycle détecté dans les 15 premiers steps")
    return {
        'cycle_detected': False
    }


def priorite5_tester_condition_victoire(env):
    """
    PRIORITÉ 5: Tester condition de victoire réelle
    Essayer différentes stratégies
    """
    print("\n" + "="*80)
    print("PRIORITÉ 5: TEST CONDITION DE VICTOIRE")
    print("="*80)
    
    strategies = [
        ("ACTION4 × 42", [GameAction.ACTION4] * 42),
        ("ACTION4 × 35 + ACTION3 × 7", [GameAction.ACTION4] * 35 + [GameAction.ACTION3] * 7),
        ("ACTION4 × 30 + ACTION3 × 12", [GameAction.ACTION4] * 30 + [GameAction.ACTION3] * 12),
        ("ACTION4 × 20 + ACTION3 × 20 + ACTION4 × 2", [GameAction.ACTION4] * 20 + [GameAction.ACTION3] * 20 + [GameAction.ACTION4] * 2),
    ]
    
    for name, sequence in strategies:
        print(f"\n--- Stratégie: {name} ---")
        obs = env.reset()
        
        for step, action in enumerate(sequence):
            obs = env.step(action)
            
            if obs.levels_completed > 0:
                print(f"🎉 VICTOIRE au step {step+1}!")
                frame = extract_frame(obs)
                dist = measure_distance_pixel_to_pixel(frame)
                
                return {
                    'victory': True,
                    'strategy': name,
                    'step': step + 1,
                    'distance_final': dist
                }
            
            if obs.state != GameState.NOT_FINISHED:
                print(f"⚠️ Jeu terminé: {obs.state}")
                break
        
        frame = extract_frame(obs)
        dist = measure_distance_pixel_to_pixel(frame)
        print(f"❌ Échec. Distance finale (min): {dist['min']:.2f}")
    
    print("\n❌ Aucune stratégie n'a mené à la victoire")
    return {
        'victory': False
    }


def main():
    """Exécution principale"""
    print("="*80)
    print("SESSION 95 - VALIDATION EXPÉRIMENTALE")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print("="*80)
    
    # Initialiser l'environnement
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    results = {}
    
    # PRIORITÉ 1: Valider métrique distance
    results['priorite1_metrique'] = priorite1_valider_metrique_distance(env)
    
    # PRIORITÉ 2: Tester ACTION4 × 42
    results['priorite2_action4'] = priorite2_tester_action4_complet(env)
    
    # PRIORITÉ 3: Explorer séquences mixtes
    results['priorite3_sequences'] = priorite3_explorer_sequences_mixtes(env)
    
    # PRIORITÉ 4: Vérifier cycle ACTION1
    results['priorite4_cycle'] = priorite4_verifier_cycle_action1(env)
    
    # PRIORITÉ 5: Tester condition de victoire
    results['priorite5_victoire'] = priorite5_tester_condition_victoire(env)
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = Path(__file__).parent / f"session95_validation_experimentale_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n{'='*80}")
    print(f"Résultats sauvegardés: {output_file}")
    print(f"{'='*80}")
    
    return results


if __name__ == "__main__":
    results = main()

# Made with Bob
