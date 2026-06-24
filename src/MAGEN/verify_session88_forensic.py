#!/usr/bin/env python3
"""
VÉRIFICATION FORENSIQUE SESSION 88 - LECTURE DIRECTE LOGS
==========================================================

Objectif: Lire TOUS les fichiers JSON Session 88 et vérifier:
1. Nombre réel de parties
2. Steps réels par partie
3. États uniques réels
4. Distribution actions réelle
5. Terminaison variable (129 vs 130 vs 136 steps)
6. Cohérence données
"""

import json
import glob
from pathlib import Path
from collections import defaultdict, Counter

def verify_session88_forensic():
    """Vérification forensique complète Session 88"""
    
    print("="*80)
    print("VÉRIFICATION FORENSIQUE SESSION 88")
    print("="*80)
    print("\n📁 Recherche fichiers JSON Session 88...")
    
    # Trouver tous les fichiers de parties
    game_files = sorted(glob.glob("session88_game_*.json"))
    results_file = glob.glob("session88_learning_results_*.json")
    
    print(f"✅ Fichiers parties trouvés: {len(game_files)}")
    print(f"✅ Fichiers résultats trouvés: {len(results_file)}")
    
    if not game_files:
        print("❌ ERREUR: Aucun fichier de partie trouvé!")
        return
    
    # Structures de données
    all_states = set()
    all_transitions = []
    action_counts = Counter()
    steps_per_game = []
    states_per_game = []
    
    print("\n" + "="*80)
    print("ANALYSE PARTIE PAR PARTIE")
    print("="*80)
    
    # Analyser chaque partie
    for i, filepath in enumerate(game_files, 1):
        print(f"\n📄 Partie {i}: {Path(filepath).name}")
        
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        # Extraire données
        game_number = data.get("game_number", i)
        total_steps = data.get("total_steps", 0)
        states_discovered = data.get("states_discovered", [])
        steps = data.get("steps", [])
        final_reward = data.get("final_reward", 0.0)
        done = data.get("done", False)
        
        # Vérifications
        print(f"  Game number: {game_number}")
        print(f"  Steps: {total_steps}")
        print(f"  États découverts: {len(states_discovered)}")
        print(f"  Transitions: {len(steps)}")
        print(f"  Reward: {final_reward}")
        print(f"  Done: {done}")
        
        # Vérifier cohérence
        if total_steps != len(steps):
            print(f"  ⚠️ INCOHÉRENCE: total_steps={total_steps} != len(steps)={len(steps)}")
        
        # Compter actions
        for step in steps:
            action = step.get("action", "UNKNOWN")
            action_counts[action] += 1
            all_transitions.append(step)
        
        # Ajouter états
        for state in states_discovered:
            all_states.add(state)
        
        # Statistiques
        steps_per_game.append(total_steps)
        states_per_game.append(len(states_discovered))
        
        # Vérifier état initial
        if i == 1:
            initial_state = data.get("initial_state", "")
            print(f"  État initial: {initial_state}")
    
    # Analyser fichier résultats
    print("\n" + "="*80)
    print("ANALYSE FICHIER RÉSULTATS")
    print("="*80)
    
    if results_file:
        with open(results_file[0], 'r') as f:
            results = json.load(f)
        
        metadata = results.get("metadata", {})
        print(f"\n📊 Métadonnées:")
        print(f"  Total games: {metadata.get('total_games', 0)}")
        print(f"  Total steps: {metadata.get('total_steps', 0)}")
        print(f"  Total states: {metadata.get('total_states_discovered', 0)}")
        print(f"  Duration: {metadata.get('duration_seconds', 0):.1f}s")
        
        # Vérifier cohérence avec analyse
        claimed_games = metadata.get('total_games', 0)
        claimed_steps = metadata.get('total_steps', 0)
        claimed_states = metadata.get('total_states_discovered', 0)
        
        actual_games = len(game_files)
        actual_steps = sum(steps_per_game)
        actual_states = len(all_states)
        
        print(f"\n🔍 Vérification cohérence:")
        print(f"  Games: {claimed_games} (claimed) vs {actual_games} (actual) - {'✅' if claimed_games == actual_games else '❌'}")
        print(f"  Steps: {claimed_steps} (claimed) vs {actual_steps} (actual) - {'✅' if claimed_steps == actual_steps else '❌'}")
        print(f"  States: {claimed_states} (claimed) vs {actual_states} (actual) - {'✅' if claimed_states == actual_states else '❌'}")
    
    # Statistiques globales
    print("\n" + "="*80)
    print("STATISTIQUES GLOBALES VÉRIFIÉES")
    print("="*80)
    
    print(f"\n📊 Parties:")
    print(f"  Total: {len(game_files)}")
    print(f"  Steps min: {min(steps_per_game)}")
    print(f"  Steps max: {max(steps_per_game)}")
    print(f"  Steps moyen: {sum(steps_per_game)/len(steps_per_game):.1f}")
    print(f"  Steps total: {sum(steps_per_game)}")
    
    print(f"\n📊 États:")
    print(f"  États uniques: {len(all_states)}")
    print(f"  États/partie min: {min(states_per_game)}")
    print(f"  États/partie max: {max(states_per_game)}")
    print(f"  États/partie moyen: {sum(states_per_game)/len(states_per_game):.1f}")
    
    print(f"\n📊 Actions:")
    total_actions = sum(action_counts.values())
    for action, count in sorted(action_counts.items()):
        pct = count / total_actions * 100
        print(f"  {action}: {count} ({pct:.1f}%)")
    
    print(f"\n📊 Transitions:")
    print(f"  Total: {len(all_transitions)}")
    
    # Analyse terminaison variable
    print("\n" + "="*80)
    print("ANALYSE TERMINAISON VARIABLE")
    print("="*80)
    
    steps_distribution = Counter(steps_per_game)
    print(f"\n📊 Distribution steps:")
    for steps, count in sorted(steps_distribution.items()):
        print(f"  {steps} steps: {count} parties")
    
    if len(steps_distribution) > 1:
        print(f"\n✅ TERMINAISON VARIABLE CONFIRMÉE!")
        print(f"  Min: {min(steps_per_game)} steps")
        print(f"  Max: {max(steps_per_game)} steps")
        print(f"  Variance: {max(steps_per_game) - min(steps_per_game)} steps")
    else:
        print(f"\n⚠️ Terminaison fixe: {steps_per_game[0]} steps")
    
    # Vérifications critiques
    print("\n" + "="*80)
    print("VÉRIFICATIONS CRITIQUES")
    print("="*80)
    
    checks = []
    
    # Check 1: Nombre de parties
    check1 = len(game_files) >= 13
    checks.append(("Nombre parties ≥ 13", check1))
    
    # Check 2: Steps totaux
    check2 = sum(steps_per_game) >= 1685
    checks.append(("Steps totaux ≥ 1685", check2))
    
    # Check 3: États uniques
    check3 = len(all_states) >= 872
    checks.append(("États uniques ≥ 872", check3))
    
    # Check 4: Terminaison variable
    check4 = len(steps_distribution) > 1
    checks.append(("Terminaison variable", check4))
    
    # Check 5: Distribution actions uniforme
    action_values = list(action_counts.values())
    if action_values:
        max_action = max(action_values)
        min_action = min(action_values)
        variance_pct = (max_action - min_action) / max_action * 100
        check5 = variance_pct < 10  # Variance < 10%
        checks.append(("Distribution actions uniforme (<10% variance)", check5))
    
    # Check 6: Cohérence fichier résultats
    if results_file:
        check6 = (claimed_games == actual_games and 
                 claimed_steps == actual_steps and 
                 claimed_states == actual_states)
        checks.append(("Cohérence fichier résultats", check6))
    
    print(f"\n📋 Résultats vérifications:")
    passed = 0
    for check_name, result in checks:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"  {status}: {check_name}")
        if result:
            passed += 1
    
    print(f"\n🎯 Score: {passed}/{len(checks)} ({passed/len(checks)*100:.1f}%)")
    
    # Conclusion
    print("\n" + "="*80)
    print("CONCLUSION FORENSIQUE")
    print("="*80)
    
    if passed == len(checks):
        print("\n✅ TOUTES LES VÉRIFICATIONS PASSÉES")
        print("   Les données Session 88 sont AUTHENTIQUES et COHÉRENTES")
    elif passed >= len(checks) * 0.8:
        print("\n⚠️ VÉRIFICATIONS MAJORITAIREMENT PASSÉES")
        print(f"   {passed}/{len(checks)} vérifications réussies")
        print("   Quelques incohérences mineures détectées")
    else:
        print("\n❌ VÉRIFICATIONS ÉCHOUÉES")
        print(f"   Seulement {passed}/{len(checks)} vérifications réussies")
        print("   Données potentiellement incorrectes")
    
    return {
        "total_games": len(game_files),
        "total_steps": sum(steps_per_game),
        "total_states": len(all_states),
        "action_counts": dict(action_counts),
        "steps_distribution": dict(steps_distribution),
        "checks_passed": passed,
        "checks_total": len(checks)
    }


if __name__ == "__main__":
    results = verify_session88_forensic()
    print("\n" + "="*80)
    print("FIN VÉRIFICATION FORENSIQUE")
    print("="*80)

# Made with Bob
