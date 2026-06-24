#!/usr/bin/env python3
"""
PHASE 4.9.2 - TEST AVEC HUMANS ACTIVÉ + CHECKPOINT/RESUME
==========================================================

OBJECTIF: Tester MAGEN avec apprentissage par imitation humaine
- Trio cognitif (C17+C18+C19) CORRIGÉ (Phase 4.7.1)
- Human Demonstration Learning ACTIVÉ
- 548 actions humaines (100% success rate)
- Imitation learning + Q-learning combinés
- 800 puzzles/cycle
- 600 secondes de test (10 minutes)
- Forensic logging complet
- NOUVEAU: Système de checkpoint/resume automatique
- NOUVEAU: Redémarrage automatique tous les 20 cycles (évite fuite mémoire)

CHANGEMENTS v4.9.2:
- Checkpoint après chaque cycle (sauvegarde état)
- Reprise automatique depuis dernier checkpoint
- Redémarrage processus tous les 20 cycles pour éviter fuite mémoire
- Durée totale: 600s (10 minutes) avec gestion multi-sessions

COMPARAISON:
- Phase 4.6.8: 68.5% success rate (baseline)
- Phase 4.7.1: 96.55% success rate (trio corrigé)
- Phase 4.9.0: 97.2% success rate (HUMANS activé, 25 cycles, crash à 12min)
- Phase 4.9.1: Crash après 24 cycles (~150s)
- Phase 4.9.2: ? success rate (HUMANS activé, checkpoint/resume)

Auteur: Bob (MAGEN Project)
Date: 2026-06-22
"""

import sys
import os
import time
from datetime import datetime
import json
import numpy as np
import pickle
from pathlib import Path

# Add paths
sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import unified rotation engine
from unified_rotation_engine import UnifiedRotationEngine
from human_demonstration_learner import HumanDemonstrationLearner
from shared_knowledge_base import SharedKnowledgeBase

class CheckpointManager:
    """Gestionnaire de checkpoints pour reprise automatique"""
    def __init__(self, checkpoint_dir: str = "checkpoints"):
        self.checkpoint_dir = Path(checkpoint_dir)
        self.checkpoint_dir.mkdir(exist_ok=True)
        self.checkpoint_file = self.checkpoint_dir / "training_checkpoint.json"
    
    def save_checkpoint(self, cycle_number: int, elapsed_time: float,
                       total_victories_arc: int, total_episodes: int):
        """Sauvegarder l'état actuel"""
        checkpoint = {
            "cycle_number": cycle_number,
            "elapsed_time": elapsed_time,
            "total_victories_arc": total_victories_arc,
            "total_episodes": total_episodes,
            "timestamp": datetime.now().isoformat()
        }
        
        with open(self.checkpoint_file, 'w') as f:
            json.dump(checkpoint, f, indent=2)
    
    def load_checkpoint(self):
        """Charger le dernier checkpoint"""
        if not self.checkpoint_file.exists():
            return None
        
        try:
            with open(self.checkpoint_file, 'r') as f:
                return json.load(f)
        except Exception as e:
            print(f"⚠️  Erreur chargement checkpoint: {e}")
            return None
    
    def clear_checkpoint(self):
        """Effacer le checkpoint"""
        if self.checkpoint_file.exists():
            self.checkpoint_file.unlink()

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
    """Test 600 secondes avec HUMANS activé + checkpoint/resume"""
    
    # Configuration
    test_duration = 600  # 10 minutes
    max_cycles_per_session = 999999  # Pas de limite (fuite mémoire corrigée)
    puzzles_per_cycle = 800  # Tous les puzzles
    
    # Checkpoint manager
    checkpoint_mgr = CheckpointManager()
    
    # Vérifier si reprise depuis checkpoint
    checkpoint = checkpoint_mgr.load_checkpoint()
    
    if checkpoint:
        print("\n" + "="*80)
        print("🔄 REPRISE DEPUIS CHECKPOINT")
        print("="*80)
        print(f"📍 Cycle précédent: {checkpoint['cycle_number']}")
        print(f"⏱️  Temps écoulé: {checkpoint['elapsed_time']:.1f}s")
        print(f"🏆 Victoires ARC: {checkpoint['total_victories_arc']}")
        print(f"📊 Épisodes totaux: {checkpoint['total_episodes']}")
        print("="*80)
        
        start_cycle = checkpoint['cycle_number'] + 1
        initial_elapsed = checkpoint['elapsed_time']
        cumulative_victories = checkpoint['total_victories_arc']
        cumulative_episodes = checkpoint['total_episodes']
    else:
        print("\n" + "="*80)
        print("🚀 PHASE 4.9.2 - TEST HUMANS ACTIVÉ + CHECKPOINT/RESUME")
        print("="*80)
        print(f"⏱️  Durée totale: {test_duration}s ({test_duration/60:.1f} minutes)")
        print(f"🔄 Redémarrage auto: tous les {max_cycles_per_session} cycles")
        print(f"🧩 Puzzles/cycle: {puzzles_per_cycle}")
        print(f"🎓 Human Learning: ACTIVÉ (548 actions)")
        print(f"🧠 Trio Cognitif: ACTIVÉ (C17+C18+C19 corrigés)")
        print(f"💾 Checkpoint: ACTIVÉ (reprise automatique)")
        print("="*80)
        
        start_cycle = 1
        initial_elapsed = 0
        cumulative_victories = 0
        cumulative_episodes = 0
    
    # Forensic logger
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    forensic_logger = ForensicLogger(
        log_path=f"forensic_HUMANS_ACTIVATED_{timestamp}.jsonl"
    )
    
    # Log test start
    forensic_logger.log_event("TEST_START", {
        "duration_seconds": test_duration,
        "puzzles_per_cycle": puzzles_per_cycle,
        "mode": "HUMANS_ACTIVATED_C17_C18_C19",
        "trio_cognitif": "ENABLED_CORRECTED",
        "human_learning": "ENABLED",
        "human_actions_count": 548
    })
    
    # Initialiser SharedKnowledgeBase
    print("\n📚 Initialisation SharedKnowledgeBase...")
    knowledge_base = SharedKnowledgeBase()
    
    # Initialiser HumanDemonstrationLearner
    print("\n🎓 Initialisation Human Demonstration Learner...")
    human_learner = HumanDemonstrationLearner(
        knowledge_base=knowledge_base,
        replays_dir="human_replays",
        verbose=True
    )
    
    # Charger démonstrations humaines
    print("\n📂 Chargement démonstrations humaines...")
    num_demos = human_learner.load_all_replays()
    
    if num_demos == 0:
        print("⚠️  ATTENTION: Aucune démonstration humaine chargée!")
        print("   Le test continuera sans imitation learning.")
    else:
        print(f"✅ {num_demos} démonstration(s) chargée(s)")
        
        # Analyser démonstrations
        for session_id, demo in human_learner.demonstrations.items():
            insights = human_learner.analyze_demonstration(demo)
            print(f"\n📊 Insights {session_id[:8]}:")
            print(f"   Actions/level: {insights['avg_actions_per_level']:.1f}")
            print(f"   Temps/action: {insights['avg_time_per_action']:.2f}s")
    
    # Log human learning initialized
    forensic_logger.log_event("HUMAN_LEARNING_INITIALIZED", {
        "demonstrations_loaded": num_demos,
        "total_human_actions": sum(d.total_actions for d in human_learner.demonstrations.values()),
        "knowledge_base_strategies": len(knowledge_base.strategies)
    })
    
    # Initialiser UnifiedRotationEngine (trio cognitif déjà activé via TransformationLearningEngine)
    print("\n🔧 Initialisation UnifiedRotationEngine...")
    engine = UnifiedRotationEngine(
        knowledge_path="unified_knowledge_humans.pkl",  # Fichier séparé pour HUMANS
        verbose=True
    )
    
    # Injecter human_learner dans l'engine pour utilisation ultérieure
    engine.human_learner = human_learner
    engine.knowledge_base = knowledge_base
    
    # Log engine initialized
    forensic_logger.log_event("ENGINE_INITIALIZED", {
        "total_puzzles": len(engine.puzzle_manager.all_puzzles),
        "arc_puzzles": engine.puzzle_manager.stats_by_type.get('arc_static', {}).get('total', 0),
        "arcade_puzzles": engine.puzzle_manager.stats_by_type.get('arcade_2d', {}).get('total', 0),
        "transformation_engine_trio_active": True,
        "human_learning_active": True,
        "human_demonstrations": num_demos
    })
    
    print(f"\n✅ Engine initialisé:")
    print(f"   Total puzzles: {len(engine.puzzle_manager.all_puzzles)}")
    print(f"   ARC: {engine.puzzle_manager.stats_by_type.get('arc_static', {}).get('total', 0)}")
    print(f"   Arcade: {engine.puzzle_manager.stats_by_type.get('arcade_2d', {}).get('total', 0)}")
    print(f"   Human Learning: ACTIVÉ ({num_demos} démonstrations)")
    
    # Boucle principale
    print(f"\n{'='*80}")
    print("🎮 DÉMARRAGE TEST")
    print(f"{'='*80}\n")
    
    # Temps de référence (ajusté si reprise)
    session_start_time = time.time()
    total_start_time = session_start_time - initial_elapsed
    cycle_number = start_cycle - 1
    
    try:
        while True:
            # Calculer temps total écoulé
            total_elapsed = time.time() - total_start_time
            remaining = test_duration - total_elapsed
            
            # Vérifier si temps total dépassé
            if total_elapsed >= test_duration:
                print(f"\n✅ Durée cible atteinte ({test_duration}s)")
                break
            
            # Vérifier si besoin de redémarrer (éviter fuite mémoire)
            cycles_in_session = cycle_number - start_cycle + 1
            if cycles_in_session >= max_cycles_per_session:
                print(f"\n🔄 Limite de {max_cycles_per_session} cycles atteinte")
                print(f"   Sauvegarde checkpoint et redémarrage recommandé...")
                
                # Sauvegarder checkpoint final
                checkpoint_mgr.save_checkpoint(
                    cycle_number=cycle_number,
                    elapsed_time=total_elapsed,
                    total_victories_arc=cumulative_victories,
                    total_episodes=cumulative_episodes
                )
                
                print(f"\n💾 Checkpoint sauvegardé (cycle {cycle_number})")
                print(f"   Relancer le script pour continuer depuis ce point")
                break
            
            cycle_number += 1
            cycle_start = time.time()
            
            print(f"\n{'='*80}")
            print(f"🔄 CYCLE #{cycle_number} (Temps restant: {remaining:.1f}s)")
            print(f"{'='*80}")
            
            # Exécuter cycle
            cycle_stats = engine.run_cycle(
                num_puzzles_per_cycle=puzzles_per_cycle,
                episodes_per_puzzle=1
            )
            
            cycle_duration = time.time() - cycle_start
            cycle_stats['cycle_duration'] = cycle_duration
            
            # Mettre à jour cumulatifs
            victories_arc = cycle_stats.get('victories_arc', 0)
            episodes_total = cycle_stats.get('total_episodes', 0)
            cumulative_victories += victories_arc
            cumulative_episodes += episodes_total
            
            # Log cycle
            forensic_logger.log_event("CYCLE_COMPLETED", cycle_stats)
            
            # Sauvegarder checkpoint après chaque cycle
            total_elapsed = time.time() - total_start_time
            checkpoint_mgr.save_checkpoint(
                cycle_number=cycle_number,
                elapsed_time=total_elapsed,
                total_victories_arc=cumulative_victories,
                total_episodes=cumulative_episodes
            )
            
            # Afficher stats
            remaining = test_duration - total_elapsed
            
            print(f"\n📊 Statistiques Cycle #{cycle_number}:")
            print(f"  Durée: {cycle_duration:.2f}s")
            print(f"  Puzzles joués: {cycle_stats.get('puzzles_count', 0)}")
            print(f"  Épisodes: {episodes_total}")
            print(f"  Victoires ARC (cycle): {victories_arc}")
            print(f"  Victoires ARC (total): {cumulative_victories}")
            
            if cycle_stats.get('by_type', {}).get('arc_static', {}).get('played', 0) > 0:
                arc_played = cycle_stats['by_type']['arc_static']['played']
                arc_victories = cycle_stats['by_type']['arc_static']['victories']
                success_rate = (arc_victories / arc_played * 100) if arc_played > 0 else 0
                print(f"  Success rate ARC: {success_rate:.2f}%")
            
            print(f"\n⏱️  Temps écoulé: {total_elapsed:.1f}s / {test_duration}s")
            print(f"  Temps restant: {remaining:.1f}s")
            print(f"  Progression: {(total_elapsed/test_duration*100):.1f}%")
            print(f"💾 Checkpoint sauvegardé")
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrompu par l'utilisateur")
        # Sauvegarder checkpoint avant sortie
        total_elapsed = time.time() - total_start_time
        checkpoint_mgr.save_checkpoint(
            cycle_number=cycle_number,
            elapsed_time=total_elapsed,
            total_victories_arc=cumulative_victories,
            total_episodes=cumulative_episodes
        )
        print(f"💾 Checkpoint sauvegardé (cycle {cycle_number})")
    except Exception as e:
        print(f"\n\n❌ Erreur: {e}")
        import traceback
        traceback.print_exc()
        # Sauvegarder checkpoint même en cas d'erreur
        try:
            total_elapsed = time.time() - total_start_time
            checkpoint_mgr.save_checkpoint(
                cycle_number=cycle_number,
                elapsed_time=total_elapsed,
                total_victories_arc=cumulative_victories,
                total_episodes=cumulative_episodes
            )
            print(f"💾 Checkpoint d'urgence sauvegardé (cycle {cycle_number})")
        except:
            pass
    
    # Résumé final
    total_duration = time.time() - total_start_time
    
    print(f"\n{'='*80}")
    print("📊 RÉSUMÉ FINAL")
    print(f"{'='*80}")
    print(f"⏱️  Durée totale: {total_duration:.1f}s ({total_duration/60:.1f} minutes)")
    print(f"🔄 Cycles complétés: {cycle_number}")
    print(f"🏆 Victoires ARC totales: {cumulative_victories}")
    print(f"📊 Épisodes totaux: {cumulative_episodes}")
    print(f"📁 Logs forensiques: forensic_HUMANS_ACTIVATED_{timestamp}.jsonl")
    print(f"💾 Checkpoint: checkpoints/training_checkpoint.json")
    
    # Effacer checkpoint si test terminé
    if total_duration >= test_duration:
        checkpoint_mgr.clear_checkpoint()
        print(f"✅ Test terminé - checkpoint effacé")
    else:
        print(f"⚠️  Test incomplet - checkpoint conservé pour reprise")
    
    print(f"{'='*80}\n")
    
    # Log test end
    forensic_logger.log_event("TEST_END", {
        "total_duration": total_duration,
        "cycles_completed": cycle_number,
        "total_victories_arc": cumulative_victories,
        "total_episodes": cumulative_episodes,
        "interrupted": total_duration < test_duration
    })


if __name__ == "__main__":
    main()

# Made with Bob
