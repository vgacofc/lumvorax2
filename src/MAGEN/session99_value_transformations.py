#!/usr/bin/env python3
"""
SESSION 99 - ANALYSE TRANSFORMATIONS VALEURS PIXEL-PAR-PIXEL
=============================================================

Objectif: Identifier la condition de victoire en analysant les transformations
         de valeurs (non spatiales) plutôt que les métriques de distance.

Approche:
1. Capturer états détaillés avec histogrammes de valeurs
2. Analyser patterns de transformation (11→3, 3→autre, etc.)
3. Identifier conditions logiques complexes
4. Tester hypothèses état-dépendantes

Hypothèses à tester:
- Transformation complète de certaines valeurs
- Ratio/proportion de valeurs spécifiques
- Patterns de distribution spatiale des valeurs
- Conditions multi-critères (valeurs + distance + état)
"""

import numpy as np
from arc_agi import Arcade, OperationMode
from arcengine import GameAction
from datetime import datetime
import json
from collections import Counter
import matplotlib.pyplot as plt
from pathlib import Path

# Configuration
PUZZLE_NAME = "ls20-9607627b"
OUTPUT_DIR = Path(".")
TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")

def convert_to_json_serializable(obj):
    """Convertit les types numpy en types Python natifs pour JSON"""
    if isinstance(obj, dict):
        return {k: convert_to_json_serializable(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [convert_to_json_serializable(item) for item in obj]
    elif isinstance(obj, (np.integer, np.int64, np.int32)):
        return int(obj)
    elif isinstance(obj, (np.floating, np.float64, np.float32)):
        return float(obj)
    elif isinstance(obj, (np.bool_, bool)):
        return bool(obj)
    elif isinstance(obj, np.ndarray):
        return obj.tolist()
    else:
        return obj

def analyze_frame_values(frame):
    """Analyse détaillée des valeurs dans une frame"""
    frame = np.array(frame)  # Convertir en numpy array si nécessaire
    unique, counts = np.unique(frame, return_counts=True)
    histogram = dict(zip(unique.tolist(), counts.tolist()))
    
    # Positions par valeur
    positions = {}
    for value in unique:
        pos = np.argwhere(frame == value)
        positions[int(value)] = {
            'count': int(counts[list(unique).index(value)]),
            'positions': pos.tolist(),
            'center': pos.mean(axis=0).tolist() if len(pos) > 0 else None
        }
    
    return {
        'histogram': histogram,
        'positions': positions,
        'total_pixels': frame.size,
        'unique_values': len(unique)
    }

def calculate_distance(frame):
    """Calcule distance euclidienne agent→target"""
    frame = np.array(frame)  # Convertir en numpy array si nécessaire
    agent_pos = np.argwhere(frame == 5)
    target_pos = np.argwhere(frame == 3)
    
    if len(agent_pos) == 0 or len(target_pos) == 0:
        return float('inf')
    
    agent_center = agent_pos.mean(axis=0)
    target_center = target_pos.mean(axis=0)
    
    return float(np.linalg.norm(agent_center - target_center))

def analyze_transformation(before, after):
    """Analyse transformation entre deux états"""
    before = np.array(before)  # Convertir en numpy array si nécessaire
    after = np.array(after)  # Convertir en numpy array si nécessaire
    changes = {}
    
    # Changements de valeurs
    for val_before in np.unique(before):
        mask = (before == val_before)
        after_values = after[mask]
        
        if len(after_values) > 0:
            transitions = Counter(after_values.tolist())
            changes[int(val_before)] = dict(transitions)
    
    # Pixels modifiés
    diff = (before != after)
    modified_pixels = int(diff.sum())
    
    return {
        'transitions': changes,
        'modified_pixels': modified_pixels,
        'modification_rate': modified_pixels / before.size
    }

def test_victory_hypotheses(frame_data, distance):
    """Teste différentes hypothèses de condition de victoire"""
    histogram = frame_data['histogram']
    positions = frame_data['positions']
    
    hypotheses = {}
    
    # H1: Toutes valeurs 11 transformées en 3
    hypotheses['all_11_to_3'] = histogram.get(11, 0) == 0 and histogram.get(3, 0) > 0
    
    # H2: Ratio valeur 3 / total > seuil
    total_pixels = frame_data['total_pixels']
    ratio_3 = histogram.get(3, 0) / total_pixels
    hypotheses['ratio_3_high'] = ratio_3 > 0.5
    
    # H3: Valeur 5 (agent) et 3 (target) au même endroit
    if 5 in positions and 3 in positions:
        agent_center = np.array(positions[5]['center'])
        target_center = np.array(positions[3]['center'])
        hypotheses['centers_overlap'] = np.linalg.norm(agent_center - target_center) < 1.0
    else:
        hypotheses['centers_overlap'] = False
    
    # H4: Distance + condition valeurs
    hypotheses['distance_and_values'] = distance < 13.0 and histogram.get(11, 0) < 10
    
    # H5: Nombre de valeurs uniques réduit
    hypotheses['few_unique_values'] = frame_data['unique_values'] <= 3
    
    # H6: Valeur 0 (background) dominante
    ratio_0 = histogram.get(0, 0) / total_pixels
    hypotheses['background_dominant'] = ratio_0 > 0.95
    
    return hypotheses

def priorite1_analyser_sequence_optimale():
    """
    PRIORITÉ 1: Analyser en détail la meilleure séquence (A4A4A4)
    Capturer tous les états intermédiaires et transformations
    """
    print("\n" + "="*80)
    print("PRIORITÉ 1: ANALYSE SÉQUENCE OPTIMALE (A4A4A4)")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make(PUZZLE_NAME)
    obs = env.reset()
    
    sequence = [GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4]
    states = []
    
    # État initial
    initial_data = analyze_frame_values(obs.frame)
    initial_distance = calculate_distance(obs.frame)
    
    states.append({
        'step': 0,
        'action': None,
        'frame_data': initial_data,
        'distance': initial_distance,
        'reward': 0.0,
        'done': False,
        'levels_completed': obs.levels_completed
    })
    
    print(f"\nÉtat initial:")
    print(f"  Distance: {initial_distance:.2f}")
    print(f"  Histogramme: {initial_data['histogram']}")
    print(f"  Valeurs uniques: {initial_data['unique_values']}")
    
    # Exécuter séquence
    prev_frame = obs.frame.copy()
    
    for i, action in enumerate(sequence, 1):
        obs = env.step(action)
        reward = obs.reward if hasattr(obs, 'reward') else 0.0
        done = obs.levels_completed > 0
        
        frame_data = analyze_frame_values(obs.frame)
        distance = calculate_distance(obs.frame)
        transformation = analyze_transformation(prev_frame, obs.frame)
        hypotheses = test_victory_hypotheses(frame_data, distance)
        
        states.append({
            'step': i,
            'action': action.name,
            'frame_data': frame_data,
            'distance': distance,
            'transformation': transformation,
            'hypotheses': hypotheses,
            'reward': reward,
            'done': done,
            'levels_completed': obs.levels_completed
        })
        
        print(f"\nÉtape {i} - {action.name}:")
        print(f"  Distance: {distance:.2f}")
        print(f"  Histogramme: {frame_data['histogram']}")
        print(f"  Pixels modifiés: {transformation['modified_pixels']}")
        print(f"  Transitions: {transformation['transitions']}")
        print(f"  Reward: {reward}")
        print(f"  Done: {done}")
        print(f"  Levels completed: {obs.levels_completed}")
        print(f"  Hypothèses vraies: {[k for k, v in hypotheses.items() if v]}")
        
        prev_frame = obs.frame.copy()
    # env.close() n'existe pas dans l'API
    
    
    # Sauvegarder résultats
    output_file = OUTPUT_DIR / f"session99_sequence_optimale_{TIMESTAMP}.json"
    with open(output_file, 'w') as f:
        json.dump(convert_to_json_serializable(states), f, indent=2)
    
    print(f"\n✓ Résultats sauvegardés: {output_file}")
    
    return states

def priorite2_tester_sequences_longues():
    """
    PRIORITÉ 2: Tester séquences plus longues (5-10 actions)
    Peut-être que la victoire nécessite une préparation multi-étapes
    """
    print("\n" + "="*80)
    print("PRIORITÉ 2: TEST SÉQUENCES LONGUES (5-10 ACTIONS)")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make(PUZZLE_NAME)
    
    # Séquences à tester (basées sur patterns observés)
    test_sequences = [
        # Répétition ACTION4 prolongée
        [GameAction.ACTION4] * 5,
        [GameAction.ACTION4] * 6,
        [GameAction.ACTION4] * 7,
        [GameAction.ACTION4] * 10,
        
        # ACTION4 avec préparation ACTION2
        [GameAction.ACTION2, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4],
        [GameAction.ACTION4, GameAction.ACTION2, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4],
        
        # Séquences mixtes longues
        [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4],
        [GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION1, GameAction.ACTION4],
        
        # Patterns alternatifs
        [GameAction.ACTION4, GameAction.ACTION2, GameAction.ACTION4, GameAction.ACTION2, GameAction.ACTION4],
        [GameAction.ACTION2, GameAction.ACTION2, GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4],
    ]
    
    results = []
    
    for seq_idx, sequence in enumerate(test_sequences, 1):
        obs = env.reset()
        initial_distance = calculate_distance(obs.frame)
        
        seq_name = "".join([f"A{a.value}" for a in sequence])
        print(f"\nTest {seq_idx}/10: {seq_name}")
        
        final_distance = initial_distance
        victory = False
        
        for step, action in enumerate(sequence, 1):
            obs = env.step(action)
            reward = obs.reward if hasattr(obs, 'reward') else 0.0
            final_distance = calculate_distance(obs.frame)
            
            if obs.levels_completed > 0 or reward == 10.0:
                victory = True
                print(f"  🎉 VICTOIRE à l'étape {step}!")
                print(f"  Reward: {reward}")
                print(f"  Distance finale: {final_distance:.2f}")
                break
        
        result = {
            'sequence': seq_name,
            'length': len(sequence),
            'distance_initial': initial_distance,
            'distance_final': final_distance,
            'improvement': initial_distance - final_distance,
            'victory': victory
        }
        
        results.append(result)
        
        if not victory:
            print(f"  Distance: {initial_distance:.2f} → {final_distance:.2f}")
            print(f"  Amélioration: {result['improvement']:.2f}")
        
        if victory:
            break
    # env.close() n'existe pas dans l'API
    
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ SÉQUENCES LONGUES")
    print("="*80)
    
    victories = [r for r in results if r['victory']]
    if victories:
        print(f"\n🎉 {len(victories)} VICTOIRE(S) TROUVÉE(S)!")
        for v in victories:
            print(f"  - {v['sequence']}: distance {v['distance_final']:.2f}")
    else:
        print("\n❌ Aucune victoire trouvée")
        print("\nMeilleures séquences:")
        sorted_results = sorted(results, key=lambda x: x['improvement'], reverse=True)
        for r in sorted_results[:5]:
            print(f"  - {r['sequence']}: amélioration {r['improvement']:.2f}")
    
    # Sauvegarder
    output_file = OUTPUT_DIR / f"session99_sequences_longues_{TIMESTAMP}.json"
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✓ Résultats sauvegardés: {output_file}")
    
    return results

def priorite3_analyser_valeurs_critiques():
    """
    PRIORITÉ 3: Analyser en détail les valeurs critiques (3, 5, 11)
    Identifier patterns de transformation spécifiques
    """
    print("\n" + "="*80)
    print("PRIORITÉ 3: ANALYSE VALEURS CRITIQUES")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make(PUZZLE_NAME)
    obs = env.reset()
    
    # Analyser état initial
    print("\nÉtat initial:")
    initial_data = analyze_frame_values(obs.frame)
    
    for value, data in initial_data['positions'].items():
        print(f"\nValeur {value}:")
        print(f"  Count: {data['count']}")
        print(f"  Center: {data['center']}")
        if value in [3, 5, 11]:
            print(f"  Positions: {len(data['positions'])} pixels")
    
    # Tester différentes actions et observer transformations
    actions_to_test = [
        GameAction.ACTION1,
        GameAction.ACTION2,
        GameAction.ACTION3,
        GameAction.ACTION4
    ]
    
    transformations = {}
    
    for action in actions_to_test:
        obs = env.reset()
        before = obs.frame.copy()
        
        obs = env.step(action)
        after = obs.frame
        
        trans = analyze_transformation(before, after)
        transformations[action.name] = trans
        
        print(f"\n{action.name}:")
        print(f"  Pixels modifiés: {trans['modified_pixels']}")
        print(f"  Taux modification: {trans['modification_rate']:.2%}")
        print(f"  Transitions:")
        for val_before, transitions in trans['transitions'].items():
            print(f"    {val_before} → {transitions}")
    # env.close() n'existe pas dans l'API
    
    
    # Sauvegarder
    output_file = OUTPUT_DIR / f"session99_valeurs_critiques_{TIMESTAMP}.json"
    with open(output_file, 'w') as f:
        json.dump({
            'initial_state': initial_data,
            'transformations': transformations
        }, f, indent=2)
    
    print(f"\n✓ Résultats sauvegardés: {output_file}")
    
    return transformations

def priorite4_explorer_conditions_complexes():
    """
    PRIORITÉ 4: Explorer conditions de victoire complexes
    Tester combinaisons de critères multiples
    """
    print("\n" + "="*80)
    print("PRIORITÉ 4: EXPLORATION CONDITIONS COMPLEXES")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make(PUZZLE_NAME)
    
    # Séquences spéciales à tester
    special_sequences = [
        # Approche graduelle
        ([GameAction.ACTION4] * 3 + [GameAction.ACTION1], "A4x3+A1"),
        ([GameAction.ACTION4] * 3 + [GameAction.ACTION3], "A4x3+A3"),
        ([GameAction.ACTION4] * 3 + [GameAction.ACTION2, GameAction.ACTION4], "A4x3+A2+A4"),
        
        # Patterns circulaires
        ([GameAction.ACTION1, GameAction.ACTION3, GameAction.ACTION4, GameAction.ACTION4], "A1-A3-A4x2"),
        ([GameAction.ACTION3, GameAction.ACTION1, GameAction.ACTION4, GameAction.ACTION4], "A3-A1-A4x2"),
        
        # Préparation puis action
        ([GameAction.ACTION2] * 3 + [GameAction.ACTION4] * 3, "A2x3+A4x3"),
        ([GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4, GameAction.ACTION4], "A1-A2-A3-A4x2"),
    ]
    
    results = []
    
    for sequence, name in special_sequences:
        obs = env.reset()
        initial_distance = calculate_distance(obs.frame)
        
        print(f"\nTest: {name}")
        
        victory = False
        for step, action in enumerate(sequence, 1):
            obs = env.step(action)
            reward = obs.reward if hasattr(obs, 'reward') else 0.0
            
            if obs.levels_completed > 0 or reward == 10.0:
                victory = True
                final_distance = calculate_distance(obs.frame)
                print(f"  🎉 VICTOIRE à l'étape {step}!")
                print(f"  Reward: {reward}")
                print(f"  Distance: {initial_distance:.2f} → {final_distance:.2f}")
                break
        
        if not victory:
            final_distance = calculate_distance(obs.frame)
            print(f"  Distance: {initial_distance:.2f} → {final_distance:.2f}")
        
        results.append({
            'name': name,
            'sequence': [a.name for a in sequence],
            'victory': victory,
            'distance_initial': initial_distance,
            'distance_final': final_distance
        })
        
        if victory:
            break
    # env.close() n'existe pas dans l'API
    
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ CONDITIONS COMPLEXES")
    print("="*80)
    
    victories = [r for r in results if r['victory']]
    if victories:
        print(f"\n🎉 {len(victories)} VICTOIRE(S)!")
        for v in victories:
            print(f"  - {v['name']}")
    else:
        print("\n❌ Aucune victoire trouvée")
    
    # Sauvegarder
    output_file = OUTPUT_DIR / f"session99_conditions_complexes_{TIMESTAMP}.json"
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✓ Résultats sauvegardés: {output_file}")
    
    return results

def main():
    """Exécution principale Session 99"""
    print("="*80)
    print("SESSION 99 - ANALYSE TRANSFORMATIONS VALEURS")
    print("="*80)
    print(f"Puzzle: {PUZZLE_NAME}")
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("="*80)
    
    try:
        # PRIORITÉ 1: Analyser séquence optimale en détail
        states = priorite1_analyser_sequence_optimale()
        
        # PRIORITÉ 2: Tester séquences longues
        long_results = priorite2_tester_sequences_longues()
        
        # Si victoire trouvée, arrêter
        if any(r['victory'] for r in long_results):
            print("\n" + "="*80)
            print("🎉 VICTOIRE TROUVÉE! Session 99 terminée avec succès.")
            print("="*80)
            return
        
        # PRIORITÉ 3: Analyser valeurs critiques
        transformations = priorite3_analyser_valeurs_critiques()
        
        # PRIORITÉ 4: Explorer conditions complexes
        complex_results = priorite4_explorer_conditions_complexes()
        
        # Résumé final
        print("\n" + "="*80)
        print("RÉSUMÉ FINAL SESSION 99")
        print("="*80)
        
        all_victories = (
            any(r['victory'] for r in long_results) or
            any(r['victory'] for r in complex_results)
        )
        
        if all_victories:
            print("\n🎉 VICTOIRE TROUVÉE!")
        else:
            print("\n❌ Aucune victoire trouvée")
            print("\nContinuer avec Session 100:")
            print("  - Analyse exhaustive séquences très longues (>10 actions)")
            print("  - Exploration patterns temporels complexes")
            print("  - Reverse engineering condition victoire")
        
    except Exception as e:
        print(f"\n❌ Erreur: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()

# Made with Bob
