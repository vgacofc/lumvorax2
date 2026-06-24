#!/usr/bin/env python3
"""
MAGEN 10-Minute Test - TOUS LES 800 PUZZLES PAR CYCLE
======================================================

Test avec rotation complète des 800 puzzles ARC à chaque cycle.
Logging forensic complet pour analyse détaillée.

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.7
"""

import sys
import os
import time
import json
from datetime import datetime
from typing import Dict

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

from unified_rotation_engine import UnifiedRotationEngine
from unified_puzzle_manager import PuzzleType


class ForensicLogger:
    """Logger forensic pour analyse détaillée"""
    
    def __init__(self, log_path: str):
        self.log_path = log_path
        self.events = []
        self.start_time = time.time()
    
    def log_event(self, event_type: str, data: Dict):
        """Enregistre un événement avec timestamp"""
        event = {
            "timestamp": datetime.now().isoformat(),
            "elapsed_seconds": time.time() - self.start_time,
            "event_type": event_type,
            "data": data
        }
        self.events.append(event)
        
        # Écriture immédiate
        with open(self.log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')


def main():
    """Lance test de 10 minutes avec TOUS les 800 puzzles"""
    
    # Configuration
    test_duration = 600  # 10 minutes
    puzzles_per_cycle = 800  # TOUS LES PUZZLES ARC!
    
    print("\n" + "="*70)
    print("🔬 MAGEN 10-MINUTE TEST - 800 PUZZLES COMPLETS")
    print("="*70)
    print(f"⏱️  Durée: {test_duration}s (10 minutes)")
    print(f"🎮 Puzzles/cycle: {puzzles_per_cycle} (TOUS LES ARC!)")
    print(f"📊 Logging: FORENSIC COMPLET")
    print("="*70)
    
    # Créer logger forensic
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    forensic_log = f"forensic_800_full_{timestamp}.jsonl"
    logger = ForensicLogger(forensic_log)
    
    logger.log_event("TEST_START", {
        "duration_seconds": test_duration,
        "puzzles_per_cycle": puzzles_per_cycle,
        "mode": "FULL_800_PUZZLES"
    })
    
    # Initialiser engine
    print("\n🔧 Initialisation Unified Rotation Engine...")
    engine = UnifiedRotationEngine(verbose=True)
    
    total_arc = engine.puzzle_manager.stats_by_type[PuzzleType.ARC_STATIC]["total"]
    
    logger.log_event("ENGINE_INITIALIZED", {
        "total_puzzles": len(engine.puzzle_manager.all_puzzles),
        "arc_puzzles": total_arc,
        "arcade_puzzles": engine.puzzle_manager.stats_by_type[PuzzleType.ARCADE_2D]["total"]
    })
    
    print(f"\n📝 Log forensic: {forensic_log}")
    print(f"🎯 Mode: ROTATION COMPLÈTE ({total_arc} puzzles/cycle)")
    print("="*70)
    
    start_time = time.time()
    cycle_count = 0
    total_episodes = 0
    
    try:
        while (time.time() - start_time) < test_duration:
            cycle_count += 1
            cycle_start = time.time()
            
            elapsed = time.time() - start_time
            remaining = test_duration - elapsed
            progress = (elapsed / test_duration) * 100
            
            print(f"\n🚀 Cycle {cycle_count}")
            print(f"   📊 Progression: {progress:.1f}% ({int(elapsed)}s / {test_duration}s)")
            print(f"   ⏳ Temps restant: {int(remaining)}s")
            
            # Exécuter cycle avec TOUS les puzzles
            cycle_stats = engine.run_cycle(num_puzzles_per_cycle=puzzles_per_cycle)
            
            cycle_duration = time.time() - cycle_start
            total_episodes += puzzles_per_cycle
            
            # Log forensic du cycle
            stats = engine.puzzle_manager.get_global_stats()
            arc_stats = stats['by_type'][PuzzleType.ARC_STATIC]
            
            logger.log_event("CYCLE_COMPLETED", {
                "cycle_number": cycle_count,
                "cycle_duration": cycle_duration,
                "puzzles_played": puzzles_per_cycle,
                "total_episodes": total_episodes,
                "arc_success_rate": arc_stats['success_rate'],
                "arc_attempted": arc_stats['attempted'],
                "arc_mastered": arc_stats['mastered'],
                "victories_arc": engine.rotation_stats['victories_arc'],
                "victories_arcade": engine.rotation_stats['victories_arcade']
            })
            
            print(f"   ⏱️  Durée cycle: {cycle_duration:.2f}s")
            print(f"   🎮 Épisodes totaux: {total_episodes}")
            print(f"   🔷 Succès ARC: {arc_stats['success_rate']:.1%}")
            print(f"   🏆 Victoires: {engine.rotation_stats['victories_arc']}")
            
            # Sauvegarder chaque cycle (important!)
            engine.save_progress()
            logger.log_event("PROGRESS_SAVED", {
                "cycle": cycle_count,
                "episodes": total_episodes
            })
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrompu par l'utilisateur")
        logger.log_event("TEST_INTERRUPTED", {
            "cycle": cycle_count,
            "episodes": total_episodes
        })
    
    finally:
        # Sauvegarde finale
        engine.save_progress()
        
        # Statistiques finales
        duration = time.time() - start_time
        final_stats = engine.puzzle_manager.get_global_stats()
        final_arc_stats = final_stats['by_type'][PuzzleType.ARC_STATIC]
        
        logger.log_event("TEST_COMPLETED", {
            "total_duration": duration,
            "cycles_completed": cycle_count,
            "total_episodes": total_episodes,
            "arc_success_rate": final_arc_stats['success_rate'],
            "arc_attempted": final_arc_stats['attempted'],
            "arc_mastered": final_arc_stats['mastered'],
            "victories_arc": engine.rotation_stats['victories_arc'],
            "victories_arcade": engine.rotation_stats['victories_arcade']
        })
        
        # Résumé final
        print("\n" + "="*70)
        print("📊 RÉSUMÉ FINAL - TEST 800 PUZZLES COMPLETS")
        print("="*70)
        print(f"⏱️  Durée totale: {duration:.1f}s ({duration/60:.1f}min)")
        print(f"🔄 Cycles complétés: {cycle_count}")
        print(f"🎮 Épisodes joués: {total_episodes}")
        print(f"📈 Puzzles/cycle: {puzzles_per_cycle}")
        print(f"⚡ Vitesse: {total_episodes/duration:.2f} épisodes/s")
        print(f"\n🔷 STATISTIQUES ARC:")
        print(f"   Tentés: {final_arc_stats['attempted']}")
        print(f"   Maîtrisés: {final_arc_stats['mastered']}")
        print(f"   Taux succès: {final_arc_stats['success_rate']:.1%}")
        print(f"\n🎯 VICTOIRES:")
        print(f"   ARC: {engine.rotation_stats['victories_arc']}")
        print(f"   Arcade: {engine.rotation_stats['victories_arcade']}")
        print(f"\n📝 Log forensic: {forensic_log}")
        print(f"   Événements: {len(logger.events)}")
        print("="*70)


if __name__ == "__main__":
    main()

# Made with ❤️ by Bob - MAGEN Project Phase 4.6.7

# Made with Bob
