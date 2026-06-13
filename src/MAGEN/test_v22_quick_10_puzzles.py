#!/usr/bin/env python3
"""
MAGEN V22 - Test Rapide 10 Puzzles
===================================

Test rapide pour valider intégration Arcade Discovery Engine
avant test complet 400 puzzles.

Session 58 - 2026-06-13
"""

import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime

# Imports MAGEN
from test_phase2_400_puzzles import test_puzzle, load_dataset
from core.learning_memory import LearningMemory
from synthesis.adaptive_strategy import AdaptiveStrategy
from forensic.lumvorax_logger import get_logger

def main():
    print("="*80)
    print("🎮 MAGEN V22 - TEST RAPIDE 10 PUZZLES")
    print("="*80)
    print(f"Date: {datetime.now().isoformat()}")
    print(f"Version: V22.0.0 (Arcade Discovery Engine)")
    print()
    
    # Initialiser logger forensique
    lumvorax = get_logger()
    print("🔍 Logger forensique LumVorax activé")
    
    # Charger dataset
    print("📂 Chargement dataset...")
    try:
        dataset = load_dataset()
        print(f"✅ Dataset chargé: {len(dataset)} puzzles disponibles")
    except FileNotFoundError as e:
        print(f"❌ ERREUR: {e}")
        return
    
    # Sélectionner 10 premiers puzzles
    puzzle_ids = list(dataset.keys())[:10]
    print(f"🎯 Puzzles sélectionnés: {len(puzzle_ids)}")
    for i, pid in enumerate(puzzle_ids, 1):
        print(f"   {i}. {pid}")
    print()
    
    # Initialiser mémoire et stratégie adaptative
    memory = LearningMemory()
    adaptive = AdaptiveStrategy()
    
    # Tester chaque puzzle
    results = []
    successes = 0
    total_time = 0
    
    print("🚀 Début des tests...")
    print("-"*80)
    
    for i, puzzle_id in enumerate(puzzle_ids, 1):
        print(f"\n[{i}/10] Testing {puzzle_id}...")
        
        puzzle_data = dataset[puzzle_id]
        result = test_puzzle(puzzle_id, puzzle_data, memory, adaptive)
        
        results.append(result)
        total_time += result['time']
        
        if result['success']:
            successes += 1
            print(f"✅ SUCCESS - {result['time']:.2f}s")
        else:
            error_msg = result['error'] if result['error'] else "validation_failed"
            print(f"❌ FAILED - {result['time']:.2f}s - {error_msg}")
        
        print(f"   Adaptive: attempts={result['adaptive_attempts']}, recoveries={result['adaptive_recoveries']}")
    
    print("\n" + "="*80)
    print("📊 RÉSULTATS FINAUX")
    print("="*80)
    print(f"Succès: {successes}/10 ({successes*10}%)")
    print(f"Échecs: {10-successes}/10 ({(10-successes)*10}%)")
    print(f"Temps total: {total_time:.2f}s")
    print(f"Temps moyen: {total_time/10:.2f}s/puzzle")
    print()
    
    # Statistiques détaillées
    total_attempts = sum(r['adaptive_attempts'] for r in results)
    total_recoveries = sum(r['adaptive_recoveries'] for r in results)
    errors = [r for r in results if r['error']]
    
    print("📈 STATISTIQUES DÉTAILLÉES")
    print(f"   Adaptive attempts: {total_attempts}")
    print(f"   Adaptive recoveries: {total_recoveries}")
    print(f"   Erreurs système: {len(errors)}")
    
    if errors:
        print("\n⚠️  ERREURS DÉTECTÉES:")
        for r in errors:
            print(f"   - {r['puzzle_id']}: {r['error']}")
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_file = f"results_v22_quick_10_{timestamp}.json"
    
    with open(results_file, 'w') as f:
        json.dump({
            'version': 'V22.0.0',
            'test_type': 'quick_validation',
            'timestamp': datetime.now().isoformat(),
            'total_puzzles': 10,
            'successes': successes,
            'failures': 10 - successes,
            'success_rate': successes / 10,
            'total_time': total_time,
            'avg_time': total_time / 10,
            'results': results
        }, f, indent=2)
    
    print(f"\n💾 Résultats sauvegardés: {results_file}")
    print(f"🔍 Logs forensiques: forensic/lumvorax_v22_quick_*.json")
    
    print("\n" + "="*80)
    print("✅ TEST V22 QUICK 10 PUZZLES TERMINÉ")
    print("="*80)
    
    # Comparaison avec V21
    print("\n📊 COMPARAISON V21 vs V22 (projection)")
    print(f"   V21 Baseline: 52/400 (13.0%)")
    print(f"   V22 Quick Test: {successes}/10 ({successes*10}%)")
    
    if successes >= 2:  # Au moins 20% sur 10 puzzles
        print(f"   ✅ Performance V22 >= V21 baseline")
    else:
        print(f"   ⚠️  Performance V22 < V21 baseline - investigation requise")
    
    return successes

if __name__ == "__main__":
    main()

# Made with Bob
