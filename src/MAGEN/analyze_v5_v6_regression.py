#!/usr/bin/env python3
"""
Analyse de régression Test V5 → V6
Identifie les puzzles qui ont régressé après application des corrections #1 et #2
"""

import json
from typing import Dict, List, Tuple

def load_results(filepath: str) -> Dict:
    """Charge les résultats d'un test"""
    with open(filepath, 'r') as f:
        return json.load(f)

def compare_results(v5_results: Dict, v6_results: Dict) -> Tuple[List[str], List[str], List[str]]:
    """
    Compare les résultats V5 et V6
    
    Returns:
        (regressed_puzzles, new_successes, maintained_failures)
    """
    v5_map = {r['puzzle_id']: r['success'] for r in v5_results['results']}
    v6_map = {r['puzzle_id']: r['success'] for r in v6_results['results']}
    
    regressed = []  # V5 success → V6 failure
    new_successes = []  # V5 failure → V6 success
    maintained_failures = []  # V5 failure → V6 failure (same)
    
    for puzzle_id in v5_map:
        v5_success = v5_map[puzzle_id]
        v6_success = v6_map.get(puzzle_id, False)
        
        if v5_success and not v6_success:
            regressed.append(puzzle_id)
        elif not v5_success and v6_success:
            new_successes.append(puzzle_id)
        elif not v5_success and not v6_success:
            maintained_failures.append(puzzle_id)
    
    return regressed, new_successes, maintained_failures

def main():
    print("="*80)
    print("ANALYSE RÉGRESSION TEST V5 → V6")
    print("="*80)
    
    # Charger résultats
    v5 = load_results('results_phase2_20260613_133654.json')
    v6 = load_results('results_phase2_20260613_134617.json')
    
    print(f"\n📊 RÉSULTATS GLOBAUX:")
    print(f"  V5: {v5['successes']}/400 ({v5['success_rate']}%)")
    print(f"  V6: {v6['successes']}/400 ({v6['success_rate']}%)")
    print(f"  Δ:  {v6['successes'] - v5['successes']} puzzles ({v6['success_rate'] - v5['success_rate']:.1f}%)")
    
    # Comparer
    regressed, new_successes, maintained = compare_results(v5, v6)
    
    print(f"\n🔍 ANALYSE DÉTAILLÉE:")
    print(f"  ❌ Régressions (V5✓ → V6✗): {len(regressed)} puzzles")
    print(f"  ✅ Nouveaux succès (V5✗ → V6✓): {len(new_successes)} puzzles")
    print(f"  ⚪ Échecs maintenus: {len(maintained)} puzzles")
    
    print(f"\n🚨 PUZZLES RÉGRESSÉS ({len(regressed)}):")
    if regressed:
        for i, puzzle_id in enumerate(regressed[:10], 1):
            print(f"  {i}. {puzzle_id}")
        if len(regressed) > 10:
            print(f"  ... et {len(regressed) - 10} autres")
    else:
        print("  Aucune régression détectée")
    
    print(f"\n✨ NOUVEAUX SUCCÈS ({len(new_successes)}):")
    if new_successes:
        for i, puzzle_id in enumerate(new_successes, 1):
            print(f"  {i}. {puzzle_id}")
    else:
        print("  Aucun nouveau succès")
    
    # Analyse des erreurs
    v5_errors = sum(1 for r in v5['results'] if r.get('error'))
    v6_errors = sum(1 for r in v6['results'] if r.get('error'))
    
    print(f"\n⚠️  ERREURS:")
    print(f"  V5: {v5_errors} erreurs")
    print(f"  V6: {v6_errors} erreurs")
    print(f"  Δ:  {v6_errors - v5_errors} erreurs")
    
    # Analyse temps
    print(f"\n⏱️  PERFORMANCE:")
    print(f"  V5: {v5['total_time']:.2f}s total, {v5['avg_time']:.4f}s/puzzle")
    print(f"  V6: {v6['total_time']:.2f}s total, {v6['avg_time']:.4f}s/puzzle")
    print(f"  Δ:  +{v6['total_time'] - v5['total_time']:.2f}s (+{((v6['total_time']/v5['total_time'])-1)*100:.1f}%)")
    
    # Diagnostic
    print(f"\n🔬 DIAGNOSTIC:")
    if len(regressed) > len(new_successes):
        print(f"  ⚠️  RÉGRESSION NETTE: {len(regressed) - len(new_successes)} puzzles perdus")
        print(f"  🎯 CAUSE PROBABLE: CORRECTION #1 (Global Constraint Reasoning) trop stricte")
        print(f"  💡 SOLUTION: Revenir à validation séquentielle OU assouplir contraintes")
    elif len(new_successes) > len(regressed):
        print(f"  ✅ AMÉLIORATION NETTE: {len(new_successes) - len(regressed)} puzzles gagnés")
    else:
        print(f"  ⚖️  ÉQUILIBRE: Même nombre de régressions et nouveaux succès")
    
    print("\n" + "="*80)

if __name__ == '__main__':
    main()

# Made with Bob
