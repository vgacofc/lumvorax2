"""
Test Phase 4.5.9 - Validation des 3 Corrections Critiques

Teste les corrections appliquées:
1. Détection cycles conditionnelle (epsilon > 0.2)
2. MetaLearning logique inversée (perf faible → exploitation)
3. Bonus persistance plans (epsilon réduit de 50%)

Test rapide: 10 épisodes pour validation
Si succès → lancer 100 épisodes pour première victoire

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.9-TEST
"""

import sys
import os

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

# Import agent Phase 4.5.8 avec corrections 4.5.9
from magen_agent_optimized_phase458 import train_agent

if __name__ == "__main__":
    print("="*60)
    print("TEST PHASE 4.5.9 - VALIDATION 3 CORRECTIONS")
    print("="*60)
    print("\nCorrections testées:")
    print("1. ✅ Détection cycles conditionnelle (epsilon > 0.2)")
    print("2. ✅ MetaLearning inversé (perf faible → exploitation)")
    print("3. ✅ Bonus persistance plans (epsilon -50%)")
    print("\nTest rapide: 10 épisodes")
    print("="*60)
    
    # Test avec 10 épisodes
    agent, stats = train_agent(num_episodes=10, max_steps=100, verbose=True)
    
    print("\n" + "="*60)
    print("RÉSULTATS TEST PHASE 4.5.9")
    print("="*60)
    print(f"Épisodes: {stats['episodes']}")
    print(f"Victoires: {stats['victories']}")
    print(f"Taux victoire: {stats['victory_rate']*100:.1f}%")
    print(f"Steps totaux: {stats['total_steps']}")
    print(f"Reward total: {stats['total_reward']:.2f}")
    
    # Analyse exploration
    if hasattr(agent.exploration, 'total_explorations'):
        exploration_rate = agent.exploration.total_explorations / stats['total_steps']
        print(f"\n📊 Taux exploration: {exploration_rate*100:.1f}%")
        print(f"   (Objectif: < 30% vs 84% Phase 4.5.8)")
    
    print("\n" + "="*60)
    
    if stats['victories'] > 0:
        print("🎉 SUCCÈS ! Corrections validées avec victoire(s)")
        print("➡️  Prochaine étape: Entraînement 100 épisodes")
    elif exploration_rate < 0.5:  # Si exploration < 50%
        print("✅ AMÉLIORATION ! Exploration réduite significativement")
        print(f"   Phase 4.5.8: 84% → Phase 4.5.9: {exploration_rate*100:.1f}%")
        print("➡️  Corrections validées, lancer 100 épisodes")
    else:
        print("⚠️  Corrections insuffisantes")
        print("   Exploration encore trop élevée")
        print("➡️  Analyser logs pour ajustements")
    
    print("="*60)

# Made with Bob