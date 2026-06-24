m"""
Test Decision Kernel V29 Lookahead
Validation sur environnements complexes (obstacles, multi-goals, stochastique)

Objectif: Valider que lookahead 2-3 steps résout le problème d'horizon court
- V28.2.2: 42.9% succès (3/7 goals)
- V29: Objectif > 70% succès (5/7 goals)

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path

# Add MAGEN core to path
sys.path.insert(0, str(Path(__file__).parent / 'core'))

# Import test infrastructure from V28.2.2
from test_fragilites_obstacles import (
    ComplexGridEnv,
    create_complex_environments,
    run_complex_environment
)

# Import V29 kernel
from decision_kernel_v29_lookahead import DecisionKernelV29Lookahead
from world_model_minimal import WorldModelMinimal


def main():
    """Test V29 lookahead sur environnements complexes."""
    
    print("="*80)
    print("TEST V29 LOOKAHEAD - Environnements Complexes")
    print("="*80)
    print()
    
    # Créer environnements (mêmes que V28.2.2)
    environments = create_complex_environments()
    
    # Créer Decision Kernel V29 (lookahead depth=3)
    kernel = DecisionKernelV29Lookahead(
        learning_rate=0.01,
        exploration_rate=0.2,
        temperature=1.0,
        lookahead_depth=3
    )
    
    # Logs forensic
    log_dir = Path(__file__).parent / 'logs_forensic_v29_lookahead'
    log_dir.mkdir(exist_ok=True)
    
    from datetime import datetime
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    log_file_path = log_dir / f'v29_test_{timestamp}.jsonl'
    
    # Résultats globaux
    total_goals = 0
    total_goals_reached = 0
    results = []
    
    with open(log_file_path, 'w') as log_file:
        for env_config in environments:
            result = run_complex_environment(env_config, kernel, log_file)
            results.append(result)
            
            total_goals += result['goals_total']
            total_goals_reached += result['goals_reached']
    
    # Analyse comparative
    print("\n" + "="*80)
    print("ANALYSE COMPARATIVE V28.2.2 vs V29")
    print("="*80)
    print()
    
    success_rate = (total_goals_reached / total_goals * 100) if total_goals > 0 else 0
    
    print(f"V28.2.2 (baseline):")
    print(f"  Goals atteints: 3/7 (42.9%)")
    print(f"  Variance moyenne: 0.0093")
    print()
    print(f"V29 Lookahead:")
    print(f"  Goals atteints: {total_goals_reached}/{total_goals} ({success_rate:.1f}%)")
    
    # Variance moyenne
    variances = [r['scores_variance'] for r in results if r['scores_variance'] > 0]
    avg_variance = sum(variances) / len(variances) if variances else 0.0
    print(f"  Variance moyenne: {avg_variance:.4f}")
    
    # Statistiques lookahead
    print()
    print(f"Statistiques lookahead:")
    stats = kernel.get_stats()
    print(f"  Simulations: {stats['lookahead_simulations']}")
    print(f"  Cache hits: {stats['lookahead_cache_hits']}")
    
    # Gain
    gain = success_rate - 42.9
    print()
    if gain > 0:
        print(f"✅ GAIN: +{gain:.1f}% vs V28.2.2")
    else:
        print(f"❌ RÉGRESSION: {gain:.1f}% vs V28.2.2")
    
    # Objectif atteint ?
    print()
    if success_rate >= 70.0:
        print("🎯 OBJECTIF ATTEINT: > 70% succès")
    else:
        print(f"⚠️  OBJECTIF NON ATTEINT: {success_rate:.1f}% < 70%")
    
    print()
    print(f"Logs forensic: {log_file_path}")
    print()
    print("="*80)
    print("TEST TERMINÉ")
    print("="*80)


if __name__ == '__main__':
    main()

# Made with Bob
