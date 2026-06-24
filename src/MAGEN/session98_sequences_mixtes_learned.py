#!/usr/bin/env python3
"""
SESSION 98 - EXPLORATION SÉQUENCES MIXTES (APPRENTISSAGE FORENSIQUE)
====================================================================

OBJECTIF: Appliquer les patterns de victoire des logs forensiques au jeu ls20-9607627b

APPRENTISSAGE DES LOGS FORENSIQUES:
- Victoires en 3 actions (séquences courtes)
- Séquences mixtes (pas de répétition)
- reward=10.0 = victoire
- Exploration rapide avec actions variées

PATTERNS DE SUCCÈS OBSERVÉS:
1. ACTION6 + ACTION7 + ACTION4 → reward=10.0
2. ACTION2 + ACTION7 + ACTION4 → reward=10.0  
3. ACTION1 + ACTION7 + ACTION3 → reward=10.0
4. RESET + ACTION4 + ACTION1 → reward=10.0
5. ACTION6 + ACTION4 + ACTION7 → reward=10.0
6. ACTION3 + RESET + ACTION3 → reward=10.0

STRATÉGIE:
1. Tester toutes les séquences de longueur 3 (4³ = 64 combinaisons)
2. Tester séquences de longueur 4 si nécessaire (4⁴ = 256)
3. Identifier patterns qui réduisent distance
4. Chercher reward=10.0 (victoire)

Date: 2026-06-18
"""

import os
import json
from datetime import datetime
from pathlib import Path
import numpy as np
from itertools import product

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

def extract_frame(obs):
    """Extraire la grille 64x64"""
    frame = obs.frame
    if isinstance(frame, list):
        frame = np.array(frame)
    if frame.ndim == 3:
        frame = frame[0]
    return frame

def measure_distance(frame):
    """Distance minimale pixel-à-pixel"""
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

def check_victory(obs):
    """Vérifier si victoire (levels_completed > 0)"""
    return hasattr(obs, 'levels_completed') and obs.levels_completed > 0

def test_sequence(env, sequence, sequence_name):
    """Tester une séquence d'actions"""
    obs = env.reset()
    
    dist_initial = measure_distance(extract_frame(obs))
    
    # Exécuter la séquence
    for action in sequence:
        obs = env.step(action)
        
        # Vérifier victoire immédiatement
        if check_victory(obs):
            dist_final = measure_distance(extract_frame(obs))
            return {
                'sequence': sequence_name,
                'actions': [str(a) for a in sequence],
                'distance_initial': dist_initial,
                'distance_final': dist_final,
                'improvement': dist_initial - dist_final,
                'victory': True,
                'steps': len(sequence)
            }
    
    dist_final = measure_distance(extract_frame(obs))
    
    return {
        'sequence': sequence_name,
        'actions': [str(a) for a in sequence],
        'distance_initial': dist_initial,
        'distance_final': dist_final,
        'improvement': dist_initial - dist_final,
        'victory': False,
        'steps': len(sequence)
    }

def phase1_sequences_longueur3():
    """PHASE 1: Tester toutes les séquences de longueur 3"""
    print("\n" + "="*80)
    print("PHASE 1: SÉQUENCES LONGUEUR 3 (64 combinaisons)")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
    action_names = ['A1', 'A2', 'A3', 'A4']
    
    results = []
    victories = []
    
    total = 4**3
    count = 0
    
    for combo in product(actions, repeat=3):
        count += 1
        seq_name = ''.join([action_names[actions.index(a)] for a in combo])
        
        result = test_sequence(env, combo, seq_name)
        results.append(result)
        
        if result['victory']:
            victories.append(result)
            print(f"\n🎉 VICTOIRE TROUVÉE! Séquence: {seq_name}")
            print(f"   Distance: {result['distance_initial']:.2f} → {result['distance_final']:.2f}")
            print(f"   Actions: {' → '.join(result['actions'])}")
            break
        
        if count % 10 == 0:
            print(f"\nProgress: {count}/{total} séquences testées")
            # Afficher meilleures séquences
            best = sorted(results, key=lambda r: r['improvement'], reverse=True)[:3]
            print("  Top 3 améliorations:")
            for b in best:
                print(f"    {b['sequence']}: {b['improvement']:.2f} (dist={b['distance_final']:.2f})")
    
    print(f"\n=== RÉSUMÉ PHASE 1 ===")
    print(f"Séquences testées: {len(results)}")
    print(f"Victoires: {len(victories)}")
    
    if not victories:
        # Identifier meilleures séquences
        best = sorted(results, key=lambda r: r['improvement'], reverse=True)[:10]
        print(f"\nTop 10 meilleures séquences:")
        for i, b in enumerate(best, 1):
            print(f"  {i}. {b['sequence']}: amélioration={b['improvement']:.2f}, dist_final={b['distance_final']:.2f}")
    
    return {
        'results': results,
        'victories': victories,
        'best_sequences': sorted(results, key=lambda r: r['improvement'], reverse=True)[:10]
    }

def phase2_sequences_longueur4():
    """PHASE 2: Tester séquences de longueur 4 (si nécessaire)"""
    print("\n" + "="*80)
    print("PHASE 2: SÉQUENCES LONGUEUR 4 (256 combinaisons)")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    actions = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
    action_names = ['A1', 'A2', 'A3', 'A4']
    
    results = []
    victories = []
    
    total = 4**4
    count = 0
    
    for combo in product(actions, repeat=4):
        count += 1
        seq_name = ''.join([action_names[actions.index(a)] for a in combo])
        
        result = test_sequence(env, combo, seq_name)
        results.append(result)
        
        if result['victory']:
            victories.append(result)
            print(f"\n🎉 VICTOIRE TROUVÉE! Séquence: {seq_name}")
            print(f"   Distance: {result['distance_initial']:.2f} → {result['distance_final']:.2f}")
            print(f"   Actions: {' → '.join(result['actions'])}")
            break
        
        if count % 20 == 0:
            print(f"\nProgress: {count}/{total} séquences testées")
            best = sorted(results, key=lambda r: r['improvement'], reverse=True)[:3]
            print("  Top 3 améliorations:")
            for b in best:
                print(f"    {b['sequence']}: {b['improvement']:.2f} (dist={b['distance_final']:.2f})")
    
    print(f"\n=== RÉSUMÉ PHASE 2 ===")
    print(f"Séquences testées: {len(results)}")
    print(f"Victoires: {len(victories)}")
    
    if not victories:
        best = sorted(results, key=lambda r: r['improvement'], reverse=True)[:10]
        print(f"\nTop 10 meilleures séquences:")
        for i, b in enumerate(best, 1):
            print(f"  {i}. {b['sequence']}: amélioration={b['improvement']:.2f}, dist_final={b['distance_final']:.2f}")
    
    return {
        'results': results,
        'victories': victories,
        'best_sequences': sorted(results, key=lambda r: r['improvement'], reverse=True)[:10]
    }

def phase3_patterns_forensiques():
    """PHASE 3: Tester patterns spécifiques des logs forensiques"""
    print("\n" + "="*80)
    print("PHASE 3: PATTERNS FORENSIQUES (6 patterns de succès)")
    print("="*80)
    
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    
    # Patterns observés dans les logs forensiques (adaptés à nos 4 actions)
    patterns = [
        ([GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION4], "A1-A2-A4"),
        ([GameAction.ACTION2, GameAction.ACTION1, GameAction.ACTION4], "A2-A1-A4"),
        ([GameAction.ACTION1, GameAction.ACTION3, GameAction.ACTION4], "A1-A3-A4"),
        ([GameAction.ACTION4, GameAction.ACTION1, GameAction.ACTION3], "A4-A1-A3"),
        ([GameAction.ACTION3, GameAction.ACTION4, GameAction.ACTION1], "A3-A4-A1"),
        ([GameAction.ACTION4, GameAction.ACTION3, GameAction.ACTION4], "A4-A3-A4"),
    ]
    
    results = []
    victories = []
    
    for sequence, name in patterns:
        print(f"\nTest pattern: {name}")
        result = test_sequence(env, sequence, name)
        results.append(result)
        
        print(f"  Distance: {result['distance_initial']:.2f} → {result['distance_final']:.2f}")
        print(f"  Amélioration: {result['improvement']:.2f}")
        print(f"  Victoire: {result['victory']}")
        
        if result['victory']:
            victories.append(result)
            print(f"\n🎉 VICTOIRE AVEC PATTERN FORENSIQUE!")
            break
    
    print(f"\n=== RÉSUMÉ PHASE 3 ===")
    print(f"Patterns testés: {len(results)}")
    print(f"Victoires: {len(victories)}")
    
    return {
        'results': results,
        'victories': victories
    }

def main():
    """Fonction principale"""
    print("="*80)
    print("SESSION 98 - EXPLORATION SÉQUENCES MIXTES (APPRENTISSAGE FORENSIQUE)")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print("="*80)
    
    results = {}
    
    # PHASE 1: Séquences longueur 3
    results['phase1'] = phase1_sequences_longueur3()
    
    # Si victoire trouvée, arrêter
    if results['phase1']['victories']:
        print("\n" + "="*80)
        print("🎉 VICTOIRE TROUVÉE EN PHASE 1!")
        print("="*80)
    else:
        # PHASE 2: Séquences longueur 4
        results['phase2'] = phase2_sequences_longueur4()
        
        if results['phase2']['victories']:
            print("\n" + "="*80)
            print("🎉 VICTOIRE TROUVÉE EN PHASE 2!")
            print("="*80)
        else:
            # PHASE 3: Patterns forensiques
            results['phase3'] = phase3_patterns_forensiques()
            
            if results['phase3']['victories']:
                print("\n" + "="*80)
                print("🎉 VICTOIRE TROUVÉE EN PHASE 3!")
                print("="*80)
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = Path(__file__).parent / f"session98_sequences_mixtes_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print("\n" + "="*80)
    print(f"Résultats sauvegardés: {output_file}")
    print("="*80)
    
    # Résumé final
    total_victories = sum(len(phase.get('victories', [])) for phase in results.values())
    print(f"\n🎯 RÉSUMÉ FINAL:")
    print(f"   Victoires totales: {total_victories}")
    
    if total_victories > 0:
        print(f"\n🎉 PUZZLE RÉSOLU!")
        for phase_name, phase_data in results.items():
            if phase_data.get('victories'):
                for v in phase_data['victories']:
                    print(f"\n   Séquence gagnante: {v['sequence']}")
                    print(f"   Actions: {' → '.join(v['actions'])}")
                    print(f"   Distance: {v['distance_initial']:.2f} → {v['distance_final']:.2f}")
    else:
        print(f"\n❌ Aucune victoire trouvée")
        print(f"   Continuer avec Session 99 (analyse patterns de valeurs)")

if __name__ == "__main__":
    main()

# Made with Bob
