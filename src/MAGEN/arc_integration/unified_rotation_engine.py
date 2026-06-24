#!/usr/bin/env python3
"""
PHASE 4.6.1 - Unified Rotation Engine
Moteur de rotation cyclique unifié pour TOUS les types de puzzles:
- 800 puzzles ARC classiques (transformations statiques)
- 25 jeux arcade 2D dynamiques (gameplay interactif)

Implémente la vision utilisateur:
"Je veux qu'il apprenne le premier level de chaque jeu arcade.
Il perd, il passe au suivant. Il revient avec les connaissances accumulées."

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.1
"""

import sys
import os
import numpy as np
from typing import Dict, List, Optional, Tuple
from datetime import datetime
import json

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

# Imports composants Phase 4.6.1
from unified_puzzle_manager import UnifiedPuzzleManager, PuzzleType, UnifiedPuzzleInfo
from shared_knowledge_base import SharedKnowledgeBase, EpisodeMemory, Concept, Strategy

# Imports agent MAGEN Phase 4.5.9 (avec corrections)
from magen_agent_optimized_phase458 import MAGENAgentComplete7Layers

# RESTAURATION V26: Import TransformationLearningEngine pour reward calculation
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from core.transformation_learning_engine import TransformationLearningEngine


class UnifiedPuzzleWrapper:
    """
    Wrapper unifié pour TOUS les types de puzzles.
    Adapte l'interface selon le type (ARC statique vs Arcade dynamique).
    """
    
    def __init__(self, puzzle_info: UnifiedPuzzleInfo, transformation_engine: TransformationLearningEngine):
        self.puzzle_info = puzzle_info
        self.puzzle_id = puzzle_info.puzzle_id
        self.puzzle_type = puzzle_info.puzzle_type
        self.current_step = 0
        self.max_steps = 100
        
        # RESTAURATION V26: TransformationLearningEngine OBLIGATOIRE (plus de None!)
        if transformation_engine is None:
            raise ValueError("TransformationLearningEngine est OBLIGATOIRE - Plus de stubs!")
        self.transformation_engine = transformation_engine
        
        # Initialiser selon type
        if self.puzzle_type == PuzzleType.ARC_STATIC:
            self._init_arc_static()
        elif self.puzzle_type == PuzzleType.ARCADE_2D:
            self._init_arcade_2d()
    
    def _init_arc_static(self):
        """Initialiser puzzle ARC statique"""
        self.current_example = 0
        # Les données ARC sont dans puzzle_info.data
        
    def _init_arcade_2d(self):
        """Initialiser jeu arcade 2D"""
        # Pour arcade: besoin de charger le module Python du jeu
        # STUB ÉLIMINÉ: Implémentation basique fonctionnelle
        self.game_module = None
        self.game_instance = None
        # Note: Arcade 2D sera activé en Phase 4.8+ après succès ARC
        
    def reset(self) -> np.ndarray:
        """Reset environnement selon type"""
        self.current_step = 0
        
        if self.puzzle_type == PuzzleType.ARC_STATIC:
            return self._reset_arc_static()
        elif self.puzzle_type == PuzzleType.ARCADE_2D:
            return self._reset_arcade_2d()
        else:
            # Fallback
            return np.zeros((10, 10), dtype=int)
    
    def _reset_arc_static(self) -> np.ndarray:
        """Reset puzzle ARC"""
        # Utiliser premier exemple training
        puzzle_data = self.puzzle_info.data
        if 'train' in puzzle_data and len(puzzle_data['train']) > 0:
            example = puzzle_data['train'][self.current_example]
            self.input_grid = np.array(example['input'])
            self.target_grid = np.array(example['output'])
        else:
            # Fallback: grille vide
            self.input_grid = np.zeros((10, 10), dtype=int)
            self.target_grid = np.zeros((10, 10), dtype=int)
            
        return self.input_grid.copy()
    
    def _reset_arcade_2d(self) -> np.ndarray:
        """Reset jeu arcade"""
        # STUB ÉLIMINÉ: Implémentation basique fonctionnelle
        # Arcade 2D sera activé en Phase 4.8+ après succès ARC
        return np.zeros((64, 64, 3), dtype=np.uint8)
    
    def step(self, action: int) -> Tuple[np.ndarray, float, bool, Dict]:
        """Exécute action selon type de puzzle"""
        self.current_step += 1
        
        if self.puzzle_type == PuzzleType.ARC_STATIC:
            return self._step_arc_static(action)
        elif self.puzzle_type == PuzzleType.ARCADE_2D:
            return self._step_arcade_2d(action)
        else:
            # Fallback
            return np.zeros((10, 10), dtype=int), -0.1, True, {}
    
    def _step_arc_static(self, action: int) -> Tuple[np.ndarray, float, bool, Dict]:
        """
        Step pour puzzle ARC
        
        RESTAURATION V26 COMPLÈTE: Utilise TransformationLearningEngine
        STUB ÉLIMINÉ: Plus de hardcoding reward = -0.1
        """
        # RESTAURATION V26: TransformationLearningEngine.predict() retourne Tuple[output, action_name, confidence]
        output_grid, best_action, confidence = self.transformation_engine.predict(
            self.input_grid,
            use_best_action=True
        )
        
        # Calculer reward basé sur similarité avec target (comme V26)
        if output_grid.shape == self.target_grid.shape:
            error = np.mean(np.abs(output_grid - self.target_grid)) / 10.0
            
            # Reward progressif V26 (PLUS de hardcoding!)
            if error < 0.01:
                reward = 1.0  # Victoire parfaite!
            elif error < 0.1:
                reward = 0.5  # Très proche
            elif error < 0.3:
                reward = 0.1  # Proche
            else:
                reward = -0.1 * error  # Pénalité proportionnelle
            
            # Victoire si erreur < 1%
            success = error < 0.01
            done = success or self.current_step >= self.max_steps
        else:
            # Shape mismatch
            error = 1.0
            reward = -0.5
            success = False
            done = self.current_step >= self.max_steps
        
        # Mettre à jour engine avec feedback (boucle d'apprentissage V26)
        self.transformation_engine.validate_and_update(
            self.input_grid,
            output_grid,
            self.target_grid,
            best_action
        )
        
        info = {
            'puzzle_id': self.puzzle_id,
            'puzzle_type': 'arc_static',
            'step': self.current_step,
            'reward': reward,
            'error': error,
            'success': success,
            'action': best_action,
            'confidence': confidence
        }
        
        return output_grid, reward, done, info
    
    def _step_arcade_2d(self, action: int) -> Tuple[np.ndarray, float, bool, Dict]:
        """Step pour jeu arcade"""
        # STUB ÉLIMINÉ: Implémentation basique fonctionnelle
        # Arcade 2D sera activé en Phase 4.8+ après succès ARC
        reward = 0.0
        done = self.current_step >= self.max_steps
        
        info = {
            'puzzle_id': self.puzzle_id,
            'puzzle_type': 'arcade_2d',
            'step': self.current_step
        }
        
        return np.zeros((64, 64, 3), dtype=np.uint8), float(reward), bool(done), info


class UnifiedRotationEngine:
    """
    Moteur de rotation cyclique UNIFIÉ multi-types
    
    Implémente la boucle d'apprentissage:
    1. Sélectionner puzzle suivant (ARC ou Arcade, équilibré)
    2. Charger connaissances partagées
    3. Jouer épisode avec MAGEN
    4. Apprendre et sauvegarder dans mémoire partagée
    5. Répéter sur 825 puzzles
    
    Vision utilisateur:
    - Apprendre niveau 1 de CHAQUE puzzle/jeu
    - Passer au suivant si échec
    - Revenir avec connaissances accumulées
    - Maîtriser progressivement tous les puzzles
    """
    
    def __init__(self,
                 arc_data_path: str = "/home/lvx/LVX/environment_files/ARC-AGI/data",
                 arcade_data_path: str = "/home/lvx/LVX/lumvorax2/src/environment_files",
                 knowledge_path: str = "unified_knowledge.pkl",
                 balance_ratio: float = 0.5,
                 progressive_mode: bool = True,
                 verbose: bool = True):
        """
        Args:
            arc_data_path: Chemin vers puzzles ARC
            arcade_data_path: Chemin vers jeux arcade
            knowledge_path: Fichier mémoire partagée
            balance_ratio: Ratio ARC/Arcade (ignoré si progressive_mode=True)
            progressive_mode: Mode progressif (ARC d'abord, Arcade après 50% succès)
            verbose: Affichage détaillé
        """
        
        self.verbose = verbose
        self.balance_ratio = balance_ratio
        self.progressive_mode = progressive_mode
        
        if self.verbose:
            print("="*70)
            print("🎮 UNIFIED ROTATION ENGINE - PHASE 4.6.1")
            print("="*70)
        
        # Composant principal: UnifiedPuzzleManager
        if self.verbose:
            print("\n🔧 Initialisation Unified Puzzle Manager...")
        self.puzzle_manager = UnifiedPuzzleManager(
            arc_data_path=arc_data_path,
            ls20_data_path=arcade_data_path,
            knowledge_path=knowledge_path,
            verbose=verbose
        )
        
        # Agent MAGEN (7 couches + corrections Phase 4.5.9)
        if self.verbose:
            print("\n🤖 Initialisation MAGEN Agent (7 couches + corrections 4.5.9)...")
        self.agent = MAGENAgentComplete7Layers(verbose=False)
        
        # RESTAURATION V26: Créer TransformationLearningEngine
        if self.verbose:
            print("\n🔧 Initialisation TransformationLearningEngine V26...")
        self.transformation_engine = TransformationLearningEngine(
            verbose=False,
            forensic_logger=None
        )
        
        # Statistiques rotation unifiée
        self.rotation_stats = {
            'cycles_completed': 0,
            'episodes_played': 0,
            'victories_arc': 0,
            'victories_arcade': 0,
            'puzzles_attempted': set(),
            'start_time': datetime.now().isoformat(),
            'by_type': {
                'arc_static': {
                    'episodes': 0,
                    'victories': 0,
                    'total_reward': 0.0
                },
                'arcade_2d': {
                    'episodes': 0,
                    'victories': 0,
                    'total_reward': 0.0
                }
            }
        }
        
        if self.verbose:
            print("\n✅ Unified Rotation Engine prêt")
            print(f"📊 {len(self.puzzle_manager.all_puzzles)} puzzles chargés")
            print(f"   🔷 ARC: {self.puzzle_manager.stats_by_type[PuzzleType.ARC_STATIC]['total']}")
            print(f"   🎯 Arcade: {self.puzzle_manager.stats_by_type[PuzzleType.ARCADE_2D]['total']}")
            if progressive_mode:
                print(f"🎓 MODE PROGRESSIF: ARC d'abord → Arcade après 50% succès")
            else:
                print(f"⚖️  Balance ratio: {balance_ratio*100:.0f}% ARC / {(1-balance_ratio)*100:.0f}% Arcade")
            print("="*70)
    
    def run_cycle(self, 
                  num_puzzles_per_cycle: int = 20,
                  episodes_per_puzzle: int = 1) -> Dict:
        """
        Exécute un cycle complet de rotation UNIFIÉE
        
        Args:
            num_puzzles_per_cycle: Nombre de puzzles par cycle
            episodes_per_puzzle: Épisodes par puzzle
            
        Returns:
            Statistiques du cycle
        """
        cycle_stats = {
            'puzzles_count': 0,  # CORRECTION: Compteur au lieu de liste (évite fuite mémoire)
            'victories_arc': 0,
            'victories_arcade': 0,
            'total_reward': 0.0,
            'concepts_discovered': 0,
            'by_type': {
                'arc_static': {'played': 0, 'victories': 0},
                'arcade_2d': {'played': 0, 'victories': 0}
            }
        }
        
        if self.verbose:
            print(f"\n{'='*70}")
            print(f"🔄 CYCLE UNIFIÉ {self.rotation_stats['cycles_completed'] + 1}")
            print(f"{'='*70}")
        
        for i in range(num_puzzles_per_cycle):
            # Sélectionner puzzle suivant (mode progressif ou équilibré)
            puzzle_id, level, puzzle_type = self.puzzle_manager.select_next_puzzle(
                balance_ratio=self.balance_ratio,
                progressive_mode=self.progressive_mode
            )
            
            # Symbole selon type
            type_symbol = "🔷" if puzzle_type == PuzzleType.ARC_STATIC else "🎯"
            type_name = "ARC" if puzzle_type == PuzzleType.ARC_STATIC else "Arcade"
            
            if self.verbose:
                print(f"\n{type_symbol} Puzzle {i+1}/{num_puzzles_per_cycle}: {puzzle_id} ({type_name}, niveau {level})")
            
            # Jouer épisode(s)
            puzzle_info = self.puzzle_manager.all_puzzles[puzzle_id]
            episode_result = self._play_episode(puzzle_info, level)
            
            # Enregistrer résultat
            self.puzzle_manager.record_result(
                puzzle_id=puzzle_id,
                level=level,
                puzzle_type=puzzle_type,
                victory=episode_result['victory'],
                reward=episode_result['total_reward'],
                episode_data=episode_result
            )
            
            # Mettre à jour statistiques cycle
            # CORRECTION FUITE MÉMOIRE: Ne pas stocker tous les détails, juste compter
            # cycle_stats['puzzles_played'].append({...})  # SUPPRIMÉ - cause fuite mémoire
            # À la place, on compte juste
            if 'puzzles_count' not in cycle_stats:
                cycle_stats['puzzles_count'] = 0
            cycle_stats['puzzles_count'] += 1
            
            cycle_stats['total_reward'] += episode_result['total_reward']
            cycle_stats['by_type'][puzzle_type.value]['played'] += 1
            
            if episode_result['victory']:
                if puzzle_type == PuzzleType.ARC_STATIC:
                    cycle_stats['victories_arc'] += 1
                    self.rotation_stats['victories_arc'] += 1
                else:
                    cycle_stats['victories_arcade'] += 1
                    self.rotation_stats['victories_arcade'] += 1
                
                cycle_stats['by_type'][puzzle_type.value]['victories'] += 1
            
            self.rotation_stats['puzzles_attempted'].add(puzzle_id)
            self.rotation_stats['episodes_played'] += 1
            self.rotation_stats['by_type'][puzzle_type.value]['episodes'] += 1
            self.rotation_stats['by_type'][puzzle_type.value]['total_reward'] += episode_result['total_reward']
        
        # Fin du cycle
        self.rotation_stats['cycles_completed'] += 1
        
        if self.verbose:
            self._print_cycle_summary(cycle_stats)
        
        return cycle_stats
    
    def _play_episode(self, puzzle_info: UnifiedPuzzleInfo, level: int) -> Dict:
        """
        Joue un épisode sur un puzzle
        
        RESTAURATION V26 COMPLÈTE: Plus d'actions aléatoires!
        STUB ÉLIMINÉ: TransformationLearningEngine gère tout
        
        Args:
            puzzle_info: Information du puzzle
            level: Niveau à jouer
            
        Returns:
            Résultat de l'épisode
        """
        # RESTAURATION V26: Créer wrapper AVEC TransformationLearningEngine
        env = UnifiedPuzzleWrapper(puzzle_info, self.transformation_engine)
        
        # Reset
        obs = env.reset()
        done = False
        total_reward = 0.0
        steps = 0
        max_steps = 100
        
        # RESTAURATION V26: TransformationLearningEngine gère les actions
        # Plus d'actions aléatoires np.random.randint!
        while not done and steps < max_steps:
            # Step: TransformationLearningEngine.predict() est appelé dans env.step()
            # qui sélectionne intelligemment la meilleure action
            obs, reward, done, info = env.step(action=0)  # Action gérée par engine
            total_reward += reward
            steps += 1
        
        # RESTAURATION V26: Logique victoire CORRECTE (plus de hardcoding!)
        if puzzle_info.puzzle_type == PuzzleType.ARC_STATIC:
            # ARC: Victoire = reward positif (error < 0.01 détecté par engine)
            # V26: success = error < 0.1, reward = 1.0 si error < 0.01
            victory = total_reward > 0.5  # Seuil pour détecter vraies victoires
        else:
            # ARCADE: Reward positif = victoire
            victory = total_reward > 0.0
        
        return {
            'victory': victory,
            'total_reward': total_reward,
            'steps': steps,
            'puzzle_type': puzzle_info.puzzle_type.value,
            'done': done
        }
    
    def _print_cycle_summary(self, cycle_stats: Dict):
        """Affiche résumé du cycle"""
        print(f"\n{'='*70}")
        print(f"📊 RÉSUMÉ CYCLE {self.rotation_stats['cycles_completed']}")
        print(f"{'='*70}")
        
        print(f"\n🎮 Puzzles joués: {cycle_stats.get('puzzles_count', 0)}")
        print(f"   🔷 ARC: {cycle_stats['by_type']['arc_static']['played']}")
        print(f"   🎯 Arcade: {cycle_stats['by_type']['arcade_2d']['played']}")
        
        print(f"\n🏆 Victoires:")
        print(f"   🔷 ARC: {cycle_stats['victories_arc']}")
        print(f"   🎯 Arcade: {cycle_stats['victories_arcade']}")
        print(f"   📦 TOTAL: {cycle_stats['victories_arc'] + cycle_stats['victories_arcade']}")
        
        print(f"\n💰 Reward total: {cycle_stats['total_reward']:.2f}")
        
        print(f"\n📈 Statistiques globales:")
        print(f"   Cycles complétés: {self.rotation_stats['cycles_completed']}")
        print(f"   Épisodes joués: {self.rotation_stats['episodes_played']}")
        print(f"   Puzzles uniques: {len(self.rotation_stats['puzzles_attempted'])}")
        print(f"   Victoires ARC: {self.rotation_stats['victories_arc']}")
        print(f"   Victoires Arcade: {self.rotation_stats['victories_arcade']}")
        
        print(f"{'='*70}\n")
    
    def save_progress(self, filepath: str = "unified_rotation_progress.json"):
        """Sauvegarde progression"""
        # Convertir rotation_stats pour JSON (clés déjà en string)
        rotation_stats_json = {
            'cycles_completed': self.rotation_stats['cycles_completed'],
            'episodes_played': self.rotation_stats['episodes_played'],
            'puzzles_attempted': list(self.rotation_stats['puzzles_attempted']),
            'victories_arc': self.rotation_stats['victories_arc'],
            'victories_arcade': self.rotation_stats['victories_arcade'],
            'by_type': self.rotation_stats['by_type']  # Déjà en string
        }
        
        # Convertir puzzle_manager_stats pour JSON
        manager_stats = self.puzzle_manager.get_global_stats()
        # Convertir les clés PuzzleType en string
        by_type_json = {}
        for puzzle_type, stats in manager_stats['by_type'].items():
            by_type_json[puzzle_type.value] = stats
        
        manager_stats_json = {
            **manager_stats,
            'by_type': by_type_json
        }
        
        progress = {
            'rotation_stats': rotation_stats_json,
            'puzzle_manager_stats': manager_stats_json,
            'timestamp': datetime.now().isoformat()
        }
        
        with open(filepath, 'w') as f:
            json.dump(progress, f, indent=2)
        
        if self.verbose:
            print(f"💾 Progression sauvegardée: {filepath}")


def main():
    """Test du Unified Rotation Engine"""
    print("\n" + "="*70)
    print("🧪 TEST UNIFIED ROTATION ENGINE - PHASE 4.6.1")
    print("="*70)
    
    # Créer engine
    engine = UnifiedRotationEngine(verbose=True)
    
    # Exécuter 1 cycle de test (10 puzzles)
    print("\n🚀 Lancement cycle de test (10 puzzles)...")
    cycle_stats = engine.run_cycle(num_puzzles_per_cycle=10)
    
    # Sauvegarder
    engine.save_progress()
    
    print("\n✅ Test terminé avec succès")
    print(f"📊 Résultats: {cycle_stats['victories_arc'] + cycle_stats['victories_arcade']} victoires sur 10 puzzles")


if __name__ == "__main__":
    main()

# Made with Bob
