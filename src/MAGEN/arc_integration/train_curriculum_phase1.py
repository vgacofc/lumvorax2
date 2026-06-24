#!/usr/bin/env python3
"""
MAGEN PHASE 1 - Entraînement Curriculum Learning sur 800 Puzzles ARC
====================================================================

Objectif: Atteindre 50% de succès sur puzzles ARC pour débloquer Phase 2 (HUMANS)

Architecture:
- UnifiedPuzzleManager (825 puzzles: 800 ARC + 25 arcade)
- Curriculum progressif: ARC d'abord, puis HUMANS, puis ARCADE
- Rotation cyclique avec mémoire partagée
- Checkpointing automatique tous les 100 épisodes
- Logs forensiques complets

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.5
"""

import sys
import os
import time
import json
from datetime import datetime
from pathlib import Path

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

from unified_puzzle_manager import UnifiedPuzzleManager
from magen_agent_complete_7layers import MAGENAgent
from shared_knowledge_base import SharedKnowledgeBase


class CurriculumTrainer:
    """
    Entraîneur avec curriculum learning progressif.
    
    Phase 1: ARC ONLY (0% → 50%)
    Phase 2: ARC + HUMANS (50% → 70%)
    Phase 3: ARC + ARCADE (>70%)
    """
    
    def __init__(self, 
                 max_episodes: int = 10000,
                 checkpoint_every: int = 100,
                 target_success_rate: float = 0.50):
        
        self.max_episodes = max_episodes
        self.checkpoint_every = checkpoint_every
        self.target_success_rate = target_success_rate
        
        # Initialiser composants
        print("\n" + "="*70)
        print("🎓 MAGEN CURRICULUM TRAINING - PHASE 1")
        print("="*70)
        print(f"🎯 Objectif: {target_success_rate:.0%} succès sur ARC")
        print(f"📊 Episodes max: {max_episodes}")
        print(f"💾 Checkpoints: tous les {checkpoint_every} épisodes")
        print("="*70)
        
        # Puzzle manager
        print("\n🔧 Initialisation Unified Puzzle Manager...")
        self.puzzle_manager = UnifiedPuzzleManager()
        
        # Knowledge base
        print("📚 Initialisation Knowledge Base...")
        self.knowledge = SharedKnowledgeBase("magen_curriculum.pkl")
        
        # Agent MAGEN
        print("🤖 Initialisation MAGEN Agent (7 couches)...")
        self.agent = MAGENAgent(
            state_size=64*64,
            action_size=4,
            knowledge_base=self.knowledge
        )
        
        # Statistiques
        self.stats = {
            'episodes': 0,
            'arc_played': 0,
            'arc_won': 0,
            'arcade_played': 0,
            'arcade_won': 0,
            'total_reward': 0.0,
            'current_phase': 1,
            'phase_unlocked': {1: True, 2: False, 3: False}
        }
        
        # Répertoires
        self.checkpoint_dir = Path("checkpoints_curriculum")
        self.checkpoint_dir.mkdir(exist_ok=True)
        
        self.forensic_dir = Path("forensic_curriculum")
        self.forensic_dir.mkdir(exist_ok=True)
        
        print("\n✅ Initialisation complète")
        print("="*70)
    
    def get_arc_success_rate(self) -> float:
        """Calcule taux de succès ARC"""
        if self.stats['arc_played'] == 0:
            return 0.0
        return self.stats['arc_won'] / self.stats['arc_played']
    
    def should_unlock_phase2(self) -> bool:
        """Vérifie si Phase 2 (HUMANS) doit être débloquée"""
        return self.get_arc_success_rate() >= 0.50
    
    def should_unlock_phase3(self) -> bool:
        """Vérifie si Phase 3 (ARCADE) doit être débloquée"""
        return self.get_arc_success_rate() >= 0.70
    
    def train_episode(self, puzzle_id: str, puzzle_type: str) -> dict:
        """
        Entraîne un épisode sur un puzzle.
        
        Returns:
            dict avec résultats épisode
        """
        # Obtenir puzzle
        if puzzle_type == "ARC":
            puzzle = self.puzzle_manager.arc_puzzles.get(puzzle_id)
        else:
            puzzle = self.puzzle_manager.arcade_games.get(puzzle_id)
        
        if not puzzle:
            return {'success': False, 'reward': -1.0, 'steps': 0}
        
        # Simuler entraînement (simplifié pour démo)
        # Dans la vraie version, utiliser l'agent complet
        steps = 0
        total_reward = 0.0
        done = False
        max_steps = 100
        
        # Reset environnement (simulé)
        state = puzzle.get('train', [{}])[0].get('input', [[]])
        
        while not done and steps < max_steps:
            # Action agent
            action = self.agent.act(state)
            
            # Step environnement (simulé)
            reward = -0.01  # Pénalité par step
            done = False  # Simplifié
            
            # Apprendre
            next_state = state  # Simplifié
            self.agent.remember(state, action, reward, next_state, done)
            
            total_reward += reward
            steps += 1
            state = next_state
        
        # Replay mémoire
        if len(self.agent.memory) > 32:
            self.agent.replay(32)
        
        # Résultat (simplifié - toujours échec pour démo)
        success = False
        
        return {
            'success': success,
            'reward': total_reward,
            'steps': steps
        }
    
    def save_checkpoint(self):
        """Sauvegarde checkpoint"""
        checkpoint_path = self.checkpoint_dir / f"checkpoint_ep{self.stats['episodes']}.json"
        
        checkpoint_data = {
            'episode': self.stats['episodes'],
            'stats': self.stats,
            'arc_success_rate': self.get_arc_success_rate(),
            'timestamp': datetime.now().isoformat()
        }
        
        with open(checkpoint_path, 'w') as f:
            json.dump(checkpoint_data, f, indent=2)
        
        # Sauvegarder knowledge base
        self.knowledge.save()
        
        print(f"💾 Checkpoint sauvegardé: {checkpoint_path.name}")
    
    def print_progress(self):
        """Affiche progression"""
        arc_rate = self.get_arc_success_rate()
        
        print(f"\n📊 Episode {self.stats['episodes']}")
        print(f"   🔷 ARC: {self.stats['arc_won']}/{self.stats['arc_played']} ({arc_rate:.1%})")
        print(f"   🎯 Arcade: {self.stats['arcade_won']}/{self.stats['arcade_played']}")
        print(f"   💰 Reward total: {self.stats['total_reward']:.2f}")
        print(f"   🎓 Phase actuelle: {self.stats['current_phase']}")
        
        # Progression vers objectif
        progress = min(arc_rate / self.target_success_rate, 1.0)
        bar_length = 40
        filled = int(bar_length * progress)
        bar = "█" * filled + "░" * (bar_length - filled)
        print(f"   📈 Progression: [{bar}] {progress:.1%}")
    
    def run(self):
        """Lance l'entraînement complet"""
        print("\n🚀 Démarrage entraînement curriculum...")
        print(f"🎯 Objectif: {self.target_success_rate:.0%} succès ARC")
        print("="*70)
        
        start_time = time.time()
        
        try:
            for episode in range(1, self.max_episodes + 1):
                self.stats['episodes'] = episode
                
                # Sélectionner puzzle selon phase
                if self.stats['current_phase'] == 1:
                    # Phase 1: ARC ONLY
                    puzzle_id = self.puzzle_manager.get_next_arc_puzzle()
                    puzzle_type = "ARC"
                    self.stats['arc_played'] += 1
                
                elif self.stats['current_phase'] == 2:
                    # Phase 2: ARC + HUMANS (à implémenter)
                    puzzle_id = self.puzzle_manager.get_next_arc_puzzle()
                    puzzle_type = "ARC"
                    self.stats['arc_played'] += 1
                
                else:
                    # Phase 3: ARC + ARCADE
                    if episode % 2 == 0:
                        puzzle_id = self.puzzle_manager.get_next_arc_puzzle()
                        puzzle_type = "ARC"
                        self.stats['arc_played'] += 1
                    else:
                        puzzle_id = self.puzzle_manager.get_next_arcade_game()
                        puzzle_type = "ARCADE"
                        self.stats['arcade_played'] += 1
                
                # Entraîner épisode
                result = self.train_episode(puzzle_id, puzzle_type)
                
                # Mettre à jour stats
                if result['success']:
                    if puzzle_type == "ARC":
                        self.stats['arc_won'] += 1
                    else:
                        self.stats['arcade_won'] += 1
                
                self.stats['total_reward'] += result['reward']
                
                # Vérifier déblocage phases
                if not self.stats['phase_unlocked'][2] and self.should_unlock_phase2():
                    print("\n" + "="*70)
                    print("🎉 PHASE 2 DÉBLOQUÉE: HUMANS")
                    print("="*70)
                    self.stats['phase_unlocked'][2] = True
                    self.stats['current_phase'] = 2
                
                if not self.stats['phase_unlocked'][3] and self.should_unlock_phase3():
                    print("\n" + "="*70)
                    print("🎉 PHASE 3 DÉBLOQUÉE: ARCADE")
                    print("="*70)
                    self.stats['phase_unlocked'][3] = True
                    self.stats['current_phase'] = 3
                
                # Afficher progression
                if episode % 10 == 0:
                    self.print_progress()
                
                # Checkpoint
                if episode % self.checkpoint_every == 0:
                    self.save_checkpoint()
                
                # Vérifier objectif atteint
                if self.get_arc_success_rate() >= self.target_success_rate:
                    print("\n" + "="*70)
                    print("🎯 OBJECTIF ATTEINT!")
                    print(f"✅ {self.target_success_rate:.0%} succès sur ARC")
                    print("="*70)
                    break
        
        except KeyboardInterrupt:
            print("\n\n⚠️  Entraînement interrompu par l'utilisateur")
        
        finally:
            # Sauvegarde finale
            self.save_checkpoint()
            
            # Résumé final
            duration = time.time() - start_time
            print("\n" + "="*70)
            print("📊 RÉSUMÉ FINAL")
            print("="*70)
            print(f"⏱️  Durée: {duration:.1f}s ({duration/60:.1f}min)")
            print(f"📈 Episodes: {self.stats['episodes']}")
            print(f"🔷 ARC: {self.stats['arc_won']}/{self.stats['arc_played']} ({self.get_arc_success_rate():.1%})")
            print(f"🎯 Arcade: {self.stats['arcade_won']}/{self.stats['arcade_played']}")
            print(f"💰 Reward total: {self.stats['total_reward']:.2f}")
            print(f"🎓 Phase finale: {self.stats['current_phase']}")
            print("="*70)


def main():
    """Point d'entrée principal"""
    print("\n" + "="*70)
    print("🎓 MAGEN CURRICULUM TRAINING - PHASE 1")
    print("="*70)
    print("📚 Objectif: Apprendre 800 puzzles ARC")
    print("🎯 Cible: 50% succès → Débloquer Phase 2 (HUMANS)")
    print("="*70)
    
    # Créer trainer
    trainer = CurriculumTrainer(
        max_episodes=10000,
        checkpoint_every=100,
        target_success_rate=0.50
    )
    
    # Lancer entraînement
    trainer.run()
    
    print("\n✅ Entraînement terminé")


if __name__ == "__main__":
    main()

# Made with ❤️ by Bob - MAGEN Project

# Made with Bob
