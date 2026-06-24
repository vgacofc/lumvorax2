#!/usr/bin/env python3
"""
PHASE 4.7.0 - TEST ACTIVATION COMPLÈTE
======================================

OBJECTIF: Tester MAGEN avec TOUS les modules activés
- Trio cognitif (C17+C18+C19) ACTIVÉ
- TransformationLearningEngine COMPLET
- 800 puzzles/cycle
- 10 minutes de test
- Forensic logging complet

COMPARAISON:
- Phase 4.6.8: 68.5% success rate (mode simplifié)
- Phase 4.7.0: ? success rate (mode COMPLET)

Auteur: Bob (MAGEN Project)
Date: 2026-06-22
"""

import sys
import os
import time
from datetime import datetime
import numpy as np
import json

# Add paths
sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import unified rotation engine
from unified_rotation_engine import UnifiedRotationEngine

class ForensicLogger:
    """Logger forensique pour traçabilité complète"""
    def __init__(self, log_path: str):
        self.log_path = log_path
        self.start_time = time.time()
        
        # Créer fichier
        with open(log_path, 'w') as f:
            f.write("")  # Fichier vide
    
    def convert_numpy_types(self, obj):
        """Convertir types numpy en types Python natifs pour JSON"""
        if isinstance(obj, np.bool_):
            return bool(obj)
        elif isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        elif isinstance(obj, np.ndarray):
            return obj.tolist()
        elif isinstance(obj, dict):
            return {key: self.convert_numpy_types(value) for key, value in obj.items()}
        elif isinstance(obj, list):
            return [self.convert_numpy_types(item) for item in obj]
        else:
            return obj
    
    def log_event(self, event_type: str, data: dict):
        """Log un événement"""
        # Convertir types numpy avant JSON
        data_clean = self.convert_numpy_types(data)
        
        event = {
            "timestamp": datetime.now().isoformat(),
            "elapsed_seconds": time.time() - self.start_time,
            "event_type": event_type,
            "data": data_clean
        }
        
        with open(self.log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')


def main():
    print("="*80)
    print("🚀 PHASE 4.7.0 - ACTIVATION COMPLÈTE MAGEN")
    print("="*80)
    print()
    print("📋 Configuration:")
    print("  - Trio cognitif (C17+C18+C19): ACTIVÉ")
    print("  - TransformationLearningEngine: COMPLET")
    print("  - Puzzles par cycle: 800")
    print("  - Durée test: 10 minutes")
    print("  - Forensic logging: ACTIVÉ")
    print()
    
    # Configuration
    test_duration = 600  # 10 minutes
    puzzles_per_cycle = 800
    
    # Forensic logger
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    forensic_logger = ForensicLogger(
        log_path=f"forensic_FULL_ACTIVATION_{timestamp}.jsonl"
    )
    
    # Log test start
    forensic_logger.log_event("TEST_START", {
        "duration_seconds": test_duration,
        "puzzles_per_cycle": puzzles_per_cycle,
        "mode": "FULL_ACTIVATION_C17_C18_C19",
        "trio_cognitif": "ENABLED",
        "transformation_engine": "COMPLETE"
    })
    
    # Créer engine
    print("🔧 Initialisation Unified Rotation Engine...")
    engine = UnifiedRotationEngine(
        arc_data_path="/home/lvx/LVX/environment_files/ARC-AGI/data",
        arcade_data_path="/home/lvx/LVX/lumvorax2/src/environment_files",
        progressive_mode=True,
        verbose=True
    )
    
    forensic_logger.log_event("ENGINE_INITIALIZED", {
        "total_puzzles": len(engine.puzzle_manager.all_puzzles),
        "arc_puzzles": 800,
        "arcade_puzzles": 25,
        "transformation_engine_trio_active": engine.transformation_engine.use_cognitive_trio
    })
    
    print()
    print("="*80)
    print("🎮 DÉMARRAGE TEST 10 MINUTES")
    print("="*80)
    print()
    
    # Boucle principale
    start_time = time.time()
    cycle_number = 0
    
    try:
        while time.time() - start_time < test_duration:
            cycle_number += 1
            cycle_start = time.time()
            
            print(f"\n{'='*80}")
            print(f"🔄 CYCLE #{cycle_number}")
            print(f"{'='*80}")
            
            # Exécuter cycle
            cycle_stats = engine.run_cycle(
                num_puzzles_per_cycle=puzzles_per_cycle,
                episodes_per_puzzle=1
            )
            
            cycle_duration = time.time() - cycle_start
            cycle_stats['cycle_duration'] = cycle_duration
            
            # Log cycle
            forensic_logger.log_event("CYCLE_COMPLETED", cycle_stats)
            
            # Afficher stats
            elapsed = time.time() - start_time
            remaining = test_duration - elapsed
            episodes_total = cycle_stats.get('total_episodes', 0)
            victories_arc = cycle_stats.get('victories_arc', 0)
            
            print(f"\n📊 Statistiques Cycle #{cycle_number}:")
            print(f"  Durée: {cycle_duration:.2f}s")
            print(f"  Puzzles joués: {cycle_stats.get('puzzles_played', 0)}")
            print(f"  Épisodes totaux: {episodes_total}")
            print(f"  Victoires ARC: {victories_arc}")
            if episodes_total > 0:
                success_rate = (victories_arc / episodes_total) * 100
                print(f"  Success rate: {success_rate:.1f}%")
            print(f"  Temps écoulé: {elapsed:.1f}s / {test_duration}s")
            print(f"  Temps restant: {remaining:.1f}s")
            
            # Sauvegarder progrès
            forensic_logger.log_event("PROGRESS_SAVED", {
                "cycle": cycle_number,
                "episodes": episodes_total
            })
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrompu par l'utilisateur")
    
    except Exception as e:
        print(f"\n\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        forensic_logger.log_event("ERROR", {
            "error": str(e),
            "traceback": traceback.format_exc()
        })
    
    finally:
        # Stats finales
        elapsed_total = time.time() - start_time
        
        print()
        print("="*80)
        print("📊 RÉSULTATS FINAUX - PHASE 4.7.0 ACTIVATION COMPLÈTE")
        print("="*80)
        print()
        print(f"⏱️  Durée totale: {elapsed_total:.1f}s ({elapsed_total/60:.1f} minutes)")
        print(f"🔄 Cycles complétés: {cycle_number}")
        
        stats = engine.rotation_stats
        total_episodes = stats['episodes_played']
        victories_arc = stats['victories_arc']
        
        print(f"🎮 Épisodes totaux: {total_episodes}")
        print(f"🏆 Victoires ARC: {victories_arc}")
        
        if total_episodes > 0:
            success_rate = (victories_arc / total_episodes) * 100
            eps_per_second = total_episodes / elapsed_total
            
            print(f"📈 Success rate: {success_rate:.2f}%")
            print(f"⚡ Vitesse: {eps_per_second:.1f} épisodes/seconde")
            
            # Comparaison avec Phase 4.6.8
            print()
            print("📊 COMPARAISON Phase 4.6.8 vs 4.7.0:")
            print(f"  Phase 4.6.8 (mode simplifié): 68.5% success rate")
            print(f"  Phase 4.7.0 (mode COMPLET):   {success_rate:.1f}% success rate")
            
            if success_rate > 68.5:
                improvement = success_rate - 68.5
                print(f"  ✅ AMÉLIORATION: +{improvement:.1f}% avec trio cognitif!")
            elif success_rate < 68.5:
                regression = 68.5 - success_rate
                print(f"  ⚠️  RÉGRESSION: -{regression:.1f}% (à investiguer)")
            else:
                print(f"  ➡️  IDENTIQUE (pas de changement)")
        
        print()
        print(f"📁 Logs forensiques: forensic_FULL_ACTIVATION_{timestamp}.jsonl")
        print()
        print("="*80)
        print("✅ TEST TERMINÉ")
        print("="*80)
        
        # Log final
        forensic_logger.log_event("TEST_COMPLETED", {
            "duration_seconds": elapsed_total,
            "cycles_completed": cycle_number,
            "total_episodes": total_episodes,
            "victories_arc": victories_arc,
            "success_rate": (victories_arc / total_episodes * 100) if total_episodes > 0 else 0.0
        })


if __name__ == "__main__":
    main()
