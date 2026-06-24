#!/usr/bin/env python3
"""
SESSION 94 - CARTOGRAPHIE DYNAMIQUE COMPLÈTE
============================================

OBJECTIF CRITIQUE:
Cartographier les actions RÉPÉTÉES pour découvrir:
1. Cycles et états terminaux
2. Collisions et limites spatiales
3. L'entité réellement contrôlée
4. Le graphe d'états atteignables
5. Les conditions de victoire

MÉTHODOLOGIE:
- Répéter chaque action (ACTION1/2/3/4) jusqu'à 50 steps
- Enregistrer TOUS les états intermédiaires
- Détecter les cycles (état revient à un état déjà vu)
- Mesurer les distances vers goals (valeur 8)
- Identifier les collisions et blocages

Date: 2026-06-18
Auteur: Bob (Mode Advanced)
"""

import sys
import os
import json
import numpy as np
from datetime import datetime
from pathlib import Path
from scipy.ndimage import label
from collections import defaultdict
import hashlib

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


def hash_frame(frame):
    """Créer un hash unique pour un état de grille"""
    return hashlib.md5(frame.tobytes()).hexdigest()


def get_frame_signature(frame):
    """Signature détaillée d'un état"""
    unique, counts = np.unique(frame, return_counts=True)
    return {int(val): int(count) for val, count in zip(unique, counts)}


def find_components(frame, value):
    """Trouver toutes les composantes connexes d'une valeur"""
    mask = (frame == value)
    labeled, num_labels = label(mask)
    
    components = []
    for i in range(1, num_labels + 1):
        positions = np.argwhere(labeled == i)
        centroid = positions.mean(axis=0)
        components.append({
            'id': i,
            'size': len(positions),
            'centroid': tuple(centroid),
            'positions': positions.tolist()
        })
    
    return components


def measure_distances_to_goals(frame):
    """Mesurer les distances entre blocs mobiles et goals"""
    goals = np.argwhere(frame == 8)
    mobile_9 = find_components(frame, 9)
    mobile_12 = find_components(frame, 12)
    
    distances = {
        'goals_positions': goals.tolist(),
        'value_9_to_goals': [],
        'value_12_to_goals': []
    }
    
    # Distance de chaque composante 9 vers le goal le plus proche
    for comp in mobile_9:
        centroid = np.array(comp['centroid'])
        min_dist = float('inf')
        for goal in goals:
            dist = np.linalg.norm(centroid - goal)
            min_dist = min(min_dist, dist)
        distances['value_9_to_goals'].append({
            'component_id': comp['id'],
            'size': comp['size'],
            'centroid': comp['centroid'],
            'min_distance_to_goal': float(min_dist)
        })
    
    # Distance de chaque composante 12 vers le goal le plus proche
    for comp in mobile_12:
        centroid = np.array(comp['centroid'])
        min_dist = float('inf')
        for goal in goals:
            dist = np.linalg.norm(centroid - goal)
            min_dist = min(min_dist, dist)
        distances['value_12_to_goals'].append({
            'component_id': comp['id'],
            'size': comp['size'],
            'centroid': comp['centroid'],
            'min_distance_to_goal': float(min_dist)
        })
    
    return distances


def analyze_repeated_action(env, action, max_steps=50, action_name="ACTION"):
    """
    Analyser une action répétée jusqu'à max_steps
    
    Retourne:
    - Tous les états visités
    - Détection de cycles
    - Collisions/blocages
    - Évolution des distances vers goals
    """
    print(f"\n{'='*80}")
    print(f"ANALYSE: {action_name} RÉPÉTÉ ({max_steps} steps)")
    print(f"{'='*80}")
    
    obs = env.reset()
    
    # Extraire la grille
    frame = obs.frame
    if isinstance(frame, list):
        frame = np.array(frame)
    if frame.ndim == 3:
        frame = frame[0]
    
    initial_frame = frame
    
    states = []
    state_hashes = {}
    cycle_detected = None
    
    current_frame = initial_frame.copy()
    
    for step in range(max_steps):
        # Hash de l'état actuel
        state_hash = hash_frame(current_frame)
        
        # Détection de cycle
        if state_hash in state_hashes:
            cycle_detected = {
                'cycle_start_step': state_hashes[state_hash],
                'cycle_end_step': step,
                'cycle_length': step - state_hashes[state_hash]
            }
            print(f"\n🔄 CYCLE DÉTECTÉ!")
            print(f"   État du step {state_hashes[state_hash]} revient au step {step}")
            print(f"   Longueur du cycle: {cycle_detected['cycle_length']} steps")
            break
        
        state_hashes[state_hash] = step
        
        # Enregistrer l'état actuel
        signature = get_frame_signature(current_frame)
        distances = measure_distances_to_goals(current_frame)
        components_9 = find_components(current_frame, 9)
        components_12 = find_components(current_frame, 12)
        
        state_info = {
            'step': step,
            'hash': state_hash,
            'signature': signature,
            'pixel_11_count': signature.get(11, 0),
            'distances_to_goals': distances,
            'components_value_9': components_9,
            'components_value_12': components_12,
            'levels_completed': float(obs.levels_completed),
            'game_state': str(obs.state)
        }
        
        states.append(state_info)
        
        # Affichage périodique
        if step % 10 == 0:
            print(f"\nStep {step}:")
            print(f"  Pixel 11: {signature.get(11, 0)}")
            print(f"  Composantes 9: {len(components_9)}")
            print(f"  Composantes 12: {len(components_12)}")
            if distances['value_9_to_goals']:
                min_dist_9 = min(d['min_distance_to_goal'] for d in distances['value_9_to_goals'])
                print(f"  Distance min (9→8): {min_dist_9:.2f}")
            if distances['value_12_to_goals']:
                min_dist_12 = min(d['min_distance_to_goal'] for d in distances['value_12_to_goals'])
                print(f"  Distance min (12→8): {min_dist_12:.2f}")
        
        # Appliquer l'action
        obs = env.step(action)
        
        # Extraire la grille
        frame = obs.frame
        if isinstance(frame, list):
            frame = np.array(frame)
        if frame.ndim == 3:
            frame = frame[0]
        
        current_frame = frame
        
        # Vérifier si le jeu est terminé
        if obs.state != GameState.NOT_FINISHED:
            print(f"\n🎮 JEU TERMINÉ au step {step+1}")
            print(f"   État: {obs.state}")
            print(f"   Niveaux complétés: {obs.levels_completed}")
            break
    
    # Analyse finale
    print(f"\n{'='*80}")
    print(f"RÉSUMÉ {action_name}")
    print(f"{'='*80}")
    print(f"Steps exécutés: {len(states)}")
    print(f"États uniques: {len(state_hashes)}")
    
    if cycle_detected:
        print(f"Cycle détecté: step {cycle_detected['cycle_start_step']} → {cycle_detected['cycle_end_step']}")
    else:
        print("Aucun cycle détecté")
    
    # Évolution du pixel 11
    pixel_11_evolution = [s['pixel_11_count'] for s in states]
    print(f"\nÉvolution pixel 11: {pixel_11_evolution[0]} → {pixel_11_evolution[-1]}")
    print(f"  Décroissance: {pixel_11_evolution[0] - pixel_11_evolution[-1]} pixels")
    
    # Évolution des distances
    if states[0]['distances_to_goals']['value_9_to_goals']:
        initial_dist_9 = min(d['min_distance_to_goal'] for d in states[0]['distances_to_goals']['value_9_to_goals'])
        final_dist_9 = min(d['min_distance_to_goal'] for d in states[-1]['distances_to_goals']['value_9_to_goals'])
        print(f"\nDistance (9→8): {initial_dist_9:.2f} → {final_dist_9:.2f}")
        print(f"  Variation: {final_dist_9 - initial_dist_9:.2f}")
    
    if states[0]['distances_to_goals']['value_12_to_goals']:
        initial_dist_12 = min(d['min_distance_to_goal'] for d in states[0]['distances_to_goals']['value_12_to_goals'])
        final_dist_12 = min(d['min_distance_to_goal'] for d in states[-1]['distances_to_goals']['value_12_to_goals'])
        print(f"\nDistance (12→8): {initial_dist_12:.2f} → {final_dist_12:.2f}")
        print(f"  Variation: {final_dist_12 - initial_dist_12:.2f}")
    
    return {
        'action_name': action_name,
        'states': states,
        'cycle_detected': cycle_detected,
        'total_steps': len(states),
        'unique_states': len(state_hashes)
    }


def compare_actions(results):
    """Comparer les résultats de toutes les actions"""
    print(f"\n{'='*80}")
    print("COMPARAISON DES 4 ACTIONS")
    print(f"{'='*80}")
    
    for action_name, result in results.items():
        print(f"\n{action_name}:")
        print(f"  Steps exécutés: {result['total_steps']}")
        print(f"  États uniques: {result['unique_states']}")
        print(f"  Cycle: {'Oui' if result['cycle_detected'] else 'Non'}")
        
        if result['cycle_detected']:
            cycle = result['cycle_detected']
            print(f"    Longueur: {cycle['cycle_length']} steps")
        
        # Évolution pixel 11
        states = result['states']
        pixel_11_start = states[0]['pixel_11_count']
        pixel_11_end = states[-1]['pixel_11_count']
        print(f"  Pixel 11: {pixel_11_start} → {pixel_11_end} (Δ={pixel_11_end - pixel_11_start})")
        
        # Évolution distances
        if states[0]['distances_to_goals']['value_9_to_goals']:
            dist_9_start = min(d['min_distance_to_goal'] for d in states[0]['distances_to_goals']['value_9_to_goals'])
            dist_9_end = min(d['min_distance_to_goal'] for d in states[-1]['distances_to_goals']['value_9_to_goals'])
            print(f"  Distance (9→8): {dist_9_start:.2f} → {dist_9_end:.2f} (Δ={dist_9_end - dist_9_start:.2f})")
        
        if states[0]['distances_to_goals']['value_12_to_goals']:
            dist_12_start = min(d['min_distance_to_goal'] for d in states[0]['distances_to_goals']['value_12_to_goals'])
            dist_12_end = min(d['min_distance_to_goal'] for d in states[-1]['distances_to_goals']['value_12_to_goals'])
            print(f"  Distance (12→8): {dist_12_start:.2f} → {dist_12_end:.2f} (Δ={dist_12_end - dist_12_start:.2f})")


def main():
    """Exécution principale"""
    print("="*80)
    print("SESSION 94 - CARTOGRAPHIE DYNAMIQUE COMPLÈTE")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print(f"Objectif: Cartographier les actions répétées (50 steps)")
    print("="*80)
    
    # Initialiser l'environnement
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    # Analyser chaque action
    results = {}
    
    actions = [
        (GameAction.ACTION1, "ACTION1"),
        (GameAction.ACTION2, "ACTION2"),
        (GameAction.ACTION3, "ACTION3"),
        (GameAction.ACTION4, "ACTION4")
    ]
    
    for action, name in actions:
        result = analyze_repeated_action(env, action, max_steps=50, action_name=name)
        results[name] = result
    
    # Comparaison finale
    compare_actions(results)
    
    # Sauvegarder les résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = Path(__file__).parent / f"session94_cartographie_dynamique_{timestamp}.json"
    
    # Convertir les arrays numpy en listes pour JSON
    def convert_to_serializable(obj):
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        elif isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        elif isinstance(obj, dict):
            return {k: convert_to_serializable(v) for k, v in obj.items()}
        elif isinstance(obj, list):
            return [convert_to_serializable(item) for item in obj]
        return obj
    
    results_serializable = convert_to_serializable(results)
    
    with open(output_file, 'w') as f:
        json.dump(results_serializable, f, indent=2)
    
    print(f"\n{'='*80}")
    print(f"Résultats sauvegardés: {output_file}")
    print(f"{'='*80}")
    
    return results


if __name__ == "__main__":
    results = main()

# Made with Bob
