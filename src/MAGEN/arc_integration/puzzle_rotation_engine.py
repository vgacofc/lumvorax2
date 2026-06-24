"""
Puzzle Rotation Engine - Phase 4.6

Moteur de rotation cyclique sur TOUS les 800 puzzles ARC.
Boucle d'apprentissage continue avec mémoire partagée.

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.0
"""

import sys
import os
import numpy as np
from typing import Dict, List, Optional
from datetime import datetime

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

# Imports composants Phase 4.6
from curriculum_manager import CurriculumManager
from shared_knowledge_base import SharedKnowledgeBase, EpisodeMemory, Concept, Strategy

# Imports agent MAGEN Phase 4.5.9 (avec corrections)
from magen_agent_optimized_phase458 import MAGENAgentComplete7Layers

# Import wrapper ARC générique
try:
    from world_model import WorldModel
    USE_NETWORKX = True
except ImportError:
    from world_model_simple import WorldModel
    USE_NETWORKX = False


class ARCPuzzleWrapper:
    """Wrapper générique pour puzzles ARC"""
    
    def __init__(self, puzzle_data: Dict, puzzle_id: str):
        self.puzzle_data = puzzle_data
        self.puzzle_id = puzzle_id
        self.current_example = 0
        self.max_steps = 100
        self.current_step = 0
        
    def reset(self) -> np.ndarray:
        """Reset environnement"""
        # Utiliser premier exemple training
        if 'train' in self.puzzle_data and len(self.puzzle_data['train']) > 0:
            example = self.puzzle_data['train'][self.current_example]
            self.input_grid = np.array(example['input'])
            self.target_grid = np.array(example['output'])
        else:
            # Fallback: grille vide
            self.input_grid = np.zeros((10, 10), dtype=int)
            self.target_grid = np.zeros((10, 10), dtype=int)
            
        self.current_step = 0
        return self.input_grid.copy()
        
    def step(self, action: int) -> tuple:
        """Exécute action (simplifié pour Phase 4.6.0)"""
        self.current_step += 1
        
        # Pour l'instant: reward basique
        # TODO Phase 4.6.1: Implémenter logique ARC complète
        reward = -0.1  # Pénalité par step
        done = self.current_step >= self.max_steps
        
        # Vérifier si solution trouvée (simplifié)
        # TODO: Comparer grille actuelle avec target
        
        info = {
            'puzzle_id': self.puzzle_id,
            'step': self.current_step
        }
        
        return self.input_grid.copy(), reward, done, info


class PuzzleRotationEngine:
    """
    Moteur de rotation cyclique multi-puzzles
    
    Implémente la boucle:
    1. Sélectionner puzzle suivant
    2. Charger connaissances partagées
    3. Jouer épisode
    4. Apprendre et sauvegarder
    5. Répéter
    """
    
    def __init__(self,
                 arc_data_path: str = "/home/lvx/LVX/environment_files/ARC-AGI/data",
                 knowledge_path: str = "shared_knowledge.pkl",
                 verbose: bool = True):
        
        self.verbose = verbose
        
        # Composants Phase 4.6
        print("🔧 Initialisation Curriculum Manager...")
        self.curriculum = CurriculumManager(arc_data_path)
        
        print("🧠 Initialisation Shared Knowledge Base...")
        self.knowledge = SharedKnowledgeBase(knowledge_path)
        
        print("🤖 Initialisation MAGEN Agent (7 couches + corrections 4.5.9)...")
        self.agent = MAGENAgentComplete7Layers(verbose=False)
        
        # Statistiques rotation
        self.rotation_stats = {
            'cycles_completed': 0,
            'episodes_played': 0,
            'victories': 0,
            'puzzles_attempted': set(),
            'start_time': datetime.now().isoformat()
        }
        
        print("✅ Rotation Engine prêt")
        print(f"📊 {len(self.curriculum.all_puzzles)} puzzles ARC chargés")
        
    def run_cycle(self, num_puzzles_per_cycle: int = 10) -> Dict:
        """
        Exécute un cycle complet de rotation
        
        Args:
            num_puzzles_per_cycle: Nombre de puzzles par cycle
            
        Returns:
            Statistiques du cycle
        """
        cycle_stats = {
            'puzzles_played': [],
            'victories': 0,
            'total_reward': 0.0,
            'concepts_discovered': 0
        }
        
        if self.verbose:
            print(f"\n{'='*60}")
            print(f"🔄 CYCLE {self.rotation_stats['cycles_completed'] + 1}")
            print(f"{'='*60}")
        
        for i in range(num_puzzles_per_cycle):
            # Sélectionner puzzle suivant
            puzzle_id, level = self.curriculum.select_next_puzzle()
            
            if self.verbose:
                print(f"\n📋 Puzzle {i+1}/{num_puzzles_per_cycle}: {puzzle_id} (niveau {level})")
            
            # Jouer épisode
            result = self._play_episode(puzzle_id, level)
            
            # Enregistrer résultat
            cycle_stats['puzzles_played'].append(puzzle_id)
            if result['victory']:
                cycle_stats['victories'] += 1
            cycle_stats['total_reward'] += result['reward']
            
            # Mettre à jour statistiques
            self.rotation_stats['episodes_played'] += 1
            self.rotation_stats['puzzles_attempted'].add(puzzle_id)
            if result['victory']:
                self.rotation_stats['victories'] += 1
        
        self.rotation_stats['cycles_completed'] += 1
        self.curriculum.total_cycles += 1
        
        # Sauvegarder connaissances
        if self.rotation_stats['cycles_completed'] % 10 == 0:
            self.knowledge.save()
            if self.verbose:
                print("\n💾 Connaissances sauvegardées")
        
        return cycle_stats
        
    def _play_episode(self, puzzle_id: str, level: int) -> Dict:
        """Joue un épisode sur un puzzle"""
        # Charger données puzzle
        puzzle_data = self.curriculum.get_puzzle_data(puzzle_id)
        
        # Créer environnement
        env = ARCPuzzleWrapper(puzzle_data, puzzle_id)
        
        # Charger connaissances partagées dans agent
        concepts = self.knowledge.get_concepts_for_puzzle(puzzle_id)
        strategies = self.knowledge.get_strategies_for_puzzle(puzzle_id)
        
        # Reset
        obs = env.reset()
        self.agent.reset_episode()
        
        # Jouer épisode
        done = False
        step = 0
        total_reward = 0.0
        max_steps = 100
        
        while not done and step < max_steps:
            # Sélectionner action
            action = self.agent.select_action(obs)
            
            # Exécuter
            next_obs, reward, done, info = env.step(action)
            
            # Apprendre
            self.agent.learn_from_transition(obs, action, next_obs, reward, done)
            
            obs = next_obs
            step += 1
            total_reward += reward
        
        # Déterminer victoire (simplifié pour Phase 4.6.0)
        victory = total_reward > -10.0  # TODO: Critère réel
        
        # Enregistrer résultat
        self.curriculum.record_result(puzzle_id, level, victory, total_reward)
        
        # Créer mémoire épisode
        memory = EpisodeMemory(
            puzzle_id=puzzle_id,
            episode_id=self.rotation_stats['episodes_played'],
            timestamp=datetime.now().isoformat(),
            victory=victory,
            steps=step,
            reward=total_reward,
            concepts_used=[],  # TODO: Tracker concepts utilisés
            strategies_used=[]
        )
        self.knowledge.add_episode_memory(memory)
        
        if self.verbose:
            status = "✅ VICTOIRE" if victory else "❌ Échec"
            print(f"   {status} - {step} steps, reward: {total_reward:.2f}")
        
        return {
            'victory': victory,
            'steps': step,
            'reward': total_reward
        }
        
    def run_continuous(self, num_cycles: int = 1000, 
                      puzzles_per_cycle: int = 10,
                      report_every: int = 10):
        """
        Exécution continue sur plusieurs cycles
        
        Args:
            num_cycles: Nombre de cycles à exécuter
            puzzles_per_cycle: Puzzles par cycle
            report_every: Fréquence rapports
        """
        print(f"\n{'='*60}")
        print(f"🚀 DÉMARRAGE ROTATION CONTINUE")
        print(f"{'='*60}")
        print(f"Cycles: {num_cycles}")
        print(f"Puzzles par cycle: {puzzles_per_cycle}")
        print(f"Total épisodes: {num_cycles * puzzles_per_cycle}")
        print(f"{'='*60}\n")
        
        for cycle in range(num_cycles):
            # Exécuter cycle
            cycle_stats = self.run_cycle(puzzles_per_cycle)
            
            # Rapport périodique
            if (cycle + 1) % report_every == 0:
                self._print_progress_report()
                
        # Rapport final
        print(f"\n{'='*60}")
        print(f"🏁 ROTATION TERMINÉE")
        print(f"{'='*60}")
        self._print_final_report()
        
    def _print_progress_report(self):
        """Affiche rapport progression"""
        stats = self.curriculum.get_statistics()
        knowledge_stats = self.knowledge.get_global_stats()
        
        print(f"\n{'='*60}")
        print(f"📊 RAPPORT PROGRESSION")
        print(f"{'='*60}")
        print(f"Cycles: {self.rotation_stats['cycles_completed']}")
        print(f"Épisodes: {self.rotation_stats['episodes_played']}")
        print(f"Victoires: {self.rotation_stats['victories']}")
        print(f"Taux succès: {self.rotation_stats['victories']/self.rotation_stats['episodes_played']*100:.1f}%")
        print(f"\nPuzzles:")
        print(f"  - Tentés: {stats['puzzles_attempted']}")
        print(f"  - Maîtrisés: {stats['puzzles_mastered']}")
        print(f"  - En cours: {stats['puzzles_in_progress']}")
        print(f"\nApprentissage:")
        print(f"  - Concepts: {knowledge_stats['concepts_learned']}")
        print(f"  - Stratégies: {knowledge_stats['strategies_discovered']}")
        print(f"  - Transferts: {knowledge_stats['successful_transfers']}")
        print(f"{'='*60}\n")
        
    def _print_final_report(self):
        """Affiche rapport final"""
        self._print_progress_report()
        
        # Exporter rapports détaillés
        self.curriculum.export_progress_report("curriculum_report.md")
        self.knowledge.export_report("knowledge_report.md")
        
        print("📄 Rapports détaillés exportés:")
        print("   - curriculum_report.md")
        print("   - knowledge_report.md")


def main():
    """Point d'entrée principal"""
    print("="*60)
    print("MAGEN PHASE 4.6 - CURRICULUM LEARNING")
    print("Rotation sur 800 puzzles ARC officiels")
    print("="*60)
    
    # Créer engine
    engine = PuzzleRotationEngine(verbose=True)
    
    # Lancer rotation continue
    # Phase 4.6.0: Test 10 cycles x 10 puzzles = 100 épisodes
    engine.run_continuous(
        num_cycles=10,
        puzzles_per_cycle=10,
        report_every=2
    )
    
    print("\n✅ Exécution terminée")


if __name__ == "__main__":
    main()

# Made with Bob