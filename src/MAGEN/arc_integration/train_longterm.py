#!/usr/bin/env python3
"""
MAGEN Long-Term Training - Phase 1 (ARC Puzzles)
================================================

Lance l'entraînement continu sur 800 puzzles ARC jusqu'à atteindre 50% de succès.

Objectif: Débloquer Phase 2 (HUMANS) avec imitation learning.

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.5
"""

import sys
import os
import time
from datetime import datetime

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

from unified_rotation_engine import UnifiedRotationEngine
from unified_puzzle_manager import PuzzleType


def main():
    """Lance entraînement long terme"""
    print("\n" + "="*70)
    print("🎓 MAGEN LONG-TERM TRAINING - PHASE 1")
    print("="*70)
    print("🎯 Objectif: 50% succès ARC → Débloquer Phase 2 (HUMANS)")
    print("📊 Dataset: 800 puzzles ARC + 25 arcade")
    print("🔄 Mode: Rotation cyclique continue")
    print("="*70)
    
    # Créer engine
    print("\n🔧 Initialisation Unified Rotation Engine...")
    engine = UnifiedRotationEngine(verbose=True)
    
    # Paramètres entraînement
    max_cycles = 1000  # Maximum 1000 cycles
    puzzles_per_cycle = 50  # 50 puzzles par cycle
    target_success_rate = 0.50  # 50% objectif
    
    print(f"\n📋 Configuration:")
    print(f"   Cycles max: {max_cycles}")
    print(f"   Puzzles/cycle: {puzzles_per_cycle}")
    print(f"   Objectif: {target_success_rate:.0%} succès ARC")
    print("="*70)
    
    start_time = time.time()
    
    try:
        for cycle in range(1, max_cycles + 1):
            print(f"\n🚀 Cycle {cycle}/{max_cycles}")
            
            # Exécuter cycle
            cycle_stats = engine.run_cycle(num_puzzles_per_cycle=puzzles_per_cycle)
            
            # Sauvegarder progression
            if cycle % 10 == 0:  # Tous les 10 cycles
                engine.save_progress()
            
            # Vérifier objectif atteint
            stats = engine.puzzle_manager.get_global_stats()
            arc_stats = stats['by_type'][PuzzleType.ARC_STATIC]
            arc_success_rate = arc_stats['success_rate']
            
            if arc_success_rate >= target_success_rate:
                print("\n" + "="*70)
                print("🎉 OBJECTIF ATTEINT!")
                print(f"✅ {arc_success_rate:.1%} succès sur puzzles ARC")
                print("🎓 Phase 2 (HUMANS) maintenant disponible!")
                print("="*70)
                break
            
            # Afficher progression
            if cycle % 5 == 0:
                elapsed = time.time() - start_time
                print(f"\n📊 Progression après {cycle} cycles:")
                print(f"   ⏱️  Temps écoulé: {elapsed/60:.1f}min")
                print(f"   🔷 Succès ARC: {arc_success_rate:.1%}")
                print(f"   📈 Vers objectif: {arc_success_rate/target_success_rate:.1%}")
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Entraînement interrompu par l'utilisateur")
    
    finally:
        # Sauvegarde finale
        engine.save_progress()
        
        # Résumé final
        duration = time.time() - start_time
        final_stats = engine.puzzle_manager.get_global_stats()
        
        print("\n" + "="*70)
        print("📊 RÉSUMÉ FINAL")
        print("="*70)
        print(f"⏱️  Durée totale: {duration/60:.1f}min ({duration/3600:.1f}h)")
        print(f"🔄 Cycles complétés: {engine.rotation_stats['cycles_completed']}")
        print(f"🎮 Episodes joués: {engine.rotation_stats['episodes_played']}")
        print(f"🔷 Victoires ARC: {engine.rotation_stats['victories_arc']}")
        print(f"🎯 Victoires Arcade: {engine.rotation_stats['victories_arcade']}")
        
        final_arc_stats = final_stats['by_type'][PuzzleType.ARC_STATIC]
        arc_rate = final_arc_stats['success_rate']
        arc_attempted = final_arc_stats['attempted']
        arc_mastered = final_arc_stats['mastered']
        print(f"\n📈 Taux succès ARC: {arc_rate:.1%} ({arc_mastered}/{arc_attempted} maîtrisés)")
        
        if arc_rate >= target_success_rate:
            print("✅ PHASE 2 (HUMANS) DÉBLOQUÉE!")
        else:
            print(f"⏳ Progression: {arc_rate/target_success_rate:.1%} vers objectif")
        
        print("="*70)


if __name__ == "__main__":
    main()

# Made with ❤️ by Bob - MAGEN Project

# Made with Bob
