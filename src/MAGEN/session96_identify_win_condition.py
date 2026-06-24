#!/usr/bin/env python3
"""
SESSION 96 - IDENTIFICATION CONDITION DE VICTOIRE
=================================================

OBJECTIF: Compléter la cartographie et identifier la VRAIE condition de victoire

4 PRIORITÉS:
1. Visualiser état bloqué (ACTION4×42) - 5 min
2. Cartographier ACTION4 pixel-par-pixel - 15 min
3. Tester hypothèse overlap exact - 10 min
4. Explorer actions après blocage - 20 min

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
import matplotlib
matplotlib.use('Agg')  # Backend non-interactif
import matplotlib.pyplot as plt

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


def extract_frame(obs):
    """Extraire la grille 64x64 depuis l'observation"""
    frame = obs.frame
    if isinstance(frame, list):
        frame = np.array(frame)
    if frame.ndim == 3:
        frame = frame[0]
    return frame


def calculate_distance_pixel(frame):
    """Distance minimale pixel-à-pixel"""
    goals = np.argwhere(frame == 8)
    blocks = np.argwhere((frame == 9) | (frame == 12))
    
    if len(goals) == 0 or len(blocks) == 0:
        return float('inf')
    
    min_distances = []
    for block_pixel in blocks:
        min_dist = min(np.linalg.norm(block_pixel - goal) for goal in goals)
        min_distances.append(min_dist)
    
    return min(min_distances)


def priorite1_visualiser_etat_bloque(env):
    """
    PRIORITÉ 1: Visualiser l'état bloqué après ACTION4×42
    """
    print("\n" + "="*80)
    print("PRIORITÉ 1: VISUALISATION ÉTAT BLOQUÉ (ACTION4×42)")
    print("="*80)
    
    obs = env.reset()
    
    # Appliquer ACTION4 × 42
    for step in range(42):
        obs = env.step(GameAction.ACTION4)
    
    frame = extract_frame(obs)
    
    # Définir les couleurs pour chaque valeur
    colors = {
        0: [0, 0, 0],        # Noir
        1: [255, 255, 255],  # Blanc
        3: [200, 200, 200],  # Gris clair (background)
        4: [50, 50, 50],     # Gris foncé (murs)
        5: [150, 150, 150],  # Gris moyen
        8: [255, 0, 0],      # Rouge (GOALS)
        9: [0, 0, 255],      # Bleu (BLOCS MOBILES)
        11: [255, 255, 0],   # Jaune (TIMER)
        12: [0, 255, 0]      # Vert (BLOC SPÉCIAL)
    }
    
    # Créer l'image colorée
    colored = np.zeros(frame.shape + (3,), dtype=np.uint8)
    for val, color in colors.items():
        mask = (frame == val)
        colored[mask] = color
    
    # Sauvegarder l'image
    plt.figure(figsize=(12, 12))
    plt.imshow(colored)
    dist = calculate_distance_pixel(frame)
    plt.title(f"État bloqué après ACTION4 × 42\nDistance min: {dist:.2f} pixels", fontsize=14)
    plt.axis('off')
    
    output_path = Path(__file__).parent / "session96_blocked_state.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    
    print(f"\n✅ Image sauvegardée: {output_path}")
    
    # Afficher les positions des éléments critiques
    print("\n=== POSITIONS DES ÉLÉMENTS CRITIQUES ===")
    for val in [8, 9, 12, 11]:
        positions = np.argwhere(frame == val)
        print(f"\nValeur {val}: {len(positions)} pixels")
        if len(positions) > 0:
            print(f"  Premières positions:")
            for pos in positions[:5]:
                print(f"    Ligne {pos[0]:2d}, Colonne {pos[1]:2d}")
            if len(positions) > 5:
                print(f"    ... et {len(positions)-5} autres")
    
    return {
        'frame': frame.tolist(),
        'distance': float(dist),
        'positions': {
            int(val): np.argwhere(frame == val).tolist()
            for val in [8, 9, 12, 11]
        }
    }


def priorite2_cartographier_action4(env):
    """
    PRIORITÉ 2: Cartographier ACTION4 pixel-par-pixel (comme Session 93)
    """
    print("\n" + "="*80)
    print("PRIORITÉ 2: CARTOGRAPHIE ACTION4 PIXEL-PAR-PIXEL")
    print("="*80)
    
    obs = env.reset()
    frame_before = extract_frame(obs).copy()
    
    obs = env.step(GameAction.ACTION4)
    frame_after = extract_frame(obs)
    
    # Détecter les changements
    changes = np.argwhere(frame_before != frame_after)
    
    print(f"\nNombre de pixels modifiés: {len(changes)}")
    
    # Analyser les transitions de valeurs
    transitions = {}
    for pos in changes:
        y, x = pos
        val_before = frame_before[y, x]
        val_after = frame_after[y, x]
        key = f"{val_before}→{val_after}"
        if key not in transitions:
            transitions[key] = []
        transitions[key].append((int(y), int(x)))
    
    print("\nTransitions de valeurs:")
    for key, positions in sorted(transitions.items()):
        print(f"  {key}: {len(positions)} fois")
    
    # Analyser les déplacements des composantes
    print("\n=== ANALYSE DES COMPOSANTES ===")
    
    for value in [9, 12]:
        mask_before = (frame_before == value)
        mask_after = (frame_after == value)
        
        labeled_before, num_before = label(mask_before)
        labeled_after, num_after = label(mask_after)
        
        print(f"\nValeur {value}:")
        print(f"  Composantes avant: {num_before}")
        print(f"  Composantes après: {num_after}")
        
        # Calculer les centroïdes
        for i in range(1, num_before + 1):
            pos_before = np.argwhere(labeled_before == i)
            centroid_before = pos_before.mean(axis=0)
            
            # Trouver la composante correspondante après
            for j in range(1, num_after + 1):
                pos_after = np.argwhere(labeled_after == j)
                centroid_after = pos_after.mean(axis=0)
                
                # Si les centroïdes sont proches, c'est la même composante
                dist = np.linalg.norm(centroid_after - centroid_before)
                if dist < 10:  # Seuil arbitraire
                    dy = centroid_after[0] - centroid_before[0]
                    dx = centroid_after[1] - centroid_before[1]
                    print(f"  Composante {i}: dy={dy:.2f}, dx={dx:.2f}")
                    break
    
    return {
        'num_changes': len(changes),
        'transitions': {k: len(v) for k, v in transitions.items()},
        'change_positions': changes.tolist()
    }


def priorite3_tester_overlap(env):
    """
    PRIORITÉ 3: Tester l'hypothèse "Overlap Exact"
    """
    print("\n" + "="*80)
    print("PRIORITÉ 3: TEST HYPOTHÈSE OVERLAP EXACT")
    print("="*80)
    
    obs = env.reset()
    
    overlap_history = []
    
    for step in range(42):
        frame = extract_frame(obs)
        
        # Calculer overlap
        goals = set(tuple(pos) for pos in np.argwhere(frame == 8))
        blocks = set(tuple(pos) for pos in np.argwhere((frame == 9) | (frame == 12)))
        overlap = len(goals & blocks)
        
        timer = np.sum(frame == 11)
        dist = calculate_distance_pixel(frame)
        
        overlap_history.append({
            'step': step,
            'overlap': overlap,
            'timer': int(timer),
            'distance': float(dist),
            'victory': obs.levels_completed > 0
        })
        
        if step % 10 == 0 or step == 41:
            print(f"\nStep {step:2d}:")
            print(f"  Overlap: {overlap} pixels")
            print(f"  Timer: {timer}")
            print(f"  Distance: {dist:.2f}")
            print(f"  Victoire: {obs.levels_completed > 0}")
        
        if obs.levels_completed > 0:
            print(f"\n🎉 VICTOIRE au step {step}!")
            print(f"   Overlap: {overlap}")
            print(f"   Timer: {timer}")
            return {
                'victory': True,
                'step': step,
                'overlap': overlap,
                'history': overlap_history
            }
        
        obs = env.step(GameAction.ACTION4)
    
    print("\n❌ Aucune victoire après 42 steps")
    print(f"   Overlap max: {max(h['overlap'] for h in overlap_history)}")
    
    return {
        'victory': False,
        'history': overlap_history
    }


def priorite4_explorer_actions_apres_blocage(env):
    """
    PRIORITÉ 4: Explorer actions alternatives après blocage
    """
    print("\n" + "="*80)
    print("PRIORITÉ 4: EXPLORATION ACTIONS APRÈS BLOCAGE")
    print("="*80)
    
    # Atteindre l'état bloqué (ACTION4 × 10)
    obs = env.reset()
    for _ in range(10):
        obs = env.step(GameAction.ACTION4)
    
    base_frame = extract_frame(obs)
    base_dist = calculate_distance_pixel(base_frame)
    
    print(f"\nÉtat bloqué atteint (distance={base_dist:.2f})")
    
    actions = [
        (GameAction.ACTION1, "ACTION1"),
        (GameAction.ACTION2, "ACTION2"),
        (GameAction.ACTION3, "ACTION3"),
        (GameAction.ACTION4, "ACTION4")
    ]
    
    results = []
    
    # Test actions simples
    print("\n--- Actions simples après blocage ---")
    for action, name in actions:
        obs_test = env.reset()
        for _ in range(10):
            obs_test = env.step(GameAction.ACTION4)
        
        obs_test = env.step(action)
        
        new_frame = extract_frame(obs_test)
        new_dist = calculate_distance_pixel(new_frame)
        delta = new_dist - base_dist
        
        result = {
            'sequence': [name],
            'distance': float(new_dist),
            'delta': float(delta),
            'victory': obs_test.levels_completed > 0
        }
        results.append(result)
        
        print(f"  ACTION4×10 + {name}: distance={new_dist:.2f} (Δ={delta:+.2f})")
    
    # Test paires d'actions
    print("\n--- Paires d'actions après blocage ---")
    for a1, name1 in actions:
        for a2, name2 in actions:
            obs_test = env.reset()
            for _ in range(10):
                obs_test = env.step(GameAction.ACTION4)
            
            obs_test = env.step(a1)
            obs_test = env.step(a2)
            
            new_frame = extract_frame(obs_test)
            new_dist = calculate_distance_pixel(new_frame)
            
            if obs_test.levels_completed > 0:
                print(f"  🎉 VICTOIRE: ACTION4×10 + {name1} + {name2} (distance={new_dist:.2f})")
                return {
                    'victory': True,
                    'sequence': [name1, name2],
                    'distance': float(new_dist),
                    'results': results
                }
    
    print("\n❌ Aucune victoire trouvée")
    
    # Trouver la meilleure séquence
    best = min(results, key=lambda x: x['distance'])
    print(f"\nMeilleure action après blocage: {best['sequence'][0]}")
    print(f"  Distance: {best['distance']:.2f} (Δ={best['delta']:+.2f})")
    
    return {
        'victory': False,
        'results': results,
        'best': best
    }


def main():
    """Exécution principale"""
    print("="*80)
    print("SESSION 96 - IDENTIFICATION CONDITION DE VICTOIRE")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print("="*80)
    
    # Initialiser l'environnement
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    results = {}
    
    # PRIORITÉ 1: Visualiser état bloqué
    results['priorite1_visualisation'] = priorite1_visualiser_etat_bloque(env)
    
    # PRIORITÉ 2: Cartographier ACTION4
    results['priorite2_cartographie'] = priorite2_cartographier_action4(env)
    
    # PRIORITÉ 3: Tester overlap
    results['priorite3_overlap'] = priorite3_tester_overlap(env)
    
    # PRIORITÉ 4: Explorer actions après blocage
    results['priorite4_exploration'] = priorite4_explorer_actions_apres_blocage(env)
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = Path(__file__).parent / f"session96_identify_win_condition_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n{'='*80}")
    print(f"Résultats sauvegardés: {output_file}")
    print(f"{'='*80}")
    
    return results


if __name__ == "__main__":
    results = main()

# Made with Bob
