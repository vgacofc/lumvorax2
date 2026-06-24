"""
Curriculum Manager - Phase 4.6

Gère la progression sur TOUS les 800 puzzles ARC officiels.
Rotation cyclique avec déblocage progressif des niveaux.

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.0
"""

import json
import random
import numpy as np
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, field
from datetime import datetime
from collections import defaultdict


@dataclass
class PuzzleProgress:
    """Progression sur un puzzle"""
    puzzle_id: str
    current_level: int = 1
    max_level_unlocked: int = 1
    attempts: int = 0
    victories: int = 0
    consecutive_wins: int = 0
    consecutive_losses: int = 0
    success_rate: float = 0.0
    best_reward: float = float('-inf')
    last_attempt: Optional[str] = None
    mastered: bool = False  # 95% success rate + 10 consecutive wins
    
class CurriculumManager:
    """
    Gestionnaire de curriculum pour apprentissage multi-puzzles
    
    Gère:
    - Progression sur 800 puzzles ARC
    - Déblocage niveaux progressif
    - Sélection puzzle suivant
    - Révision automatique
    """
    
    def __init__(self, arc_data_path: str = None):
        if arc_data_path is None:
            # Chemin absolu depuis workspace
            arc_data_path = "/home/lvx/LVX/environment_files/ARC-AGI/data"
        self.arc_data_path = arc_data_path
        
        # Charger TOUS les puzzles ARC
        self.all_puzzles = self._load_all_puzzles()
        print(f"✅ {len(self.all_puzzles)} puzzles ARC chargés")
        
        # Progression par puzzle
        self.puzzle_progress: Dict[str, PuzzleProgress] = {}
        for puzzle_id in self.all_puzzles.keys():
            self.puzzle_progress[puzzle_id] = PuzzleProgress(puzzle_id=puzzle_id)
            
        # Critères de progression
        self.success_rate_threshold = 0.95  # 95% requis
        self.consecutive_wins_required = 10
        self.stability_cycles_required = 5
        
        # Historique sélections
        self.selection_history: List[str] = []
        self.max_history = 1000
        
        # Statistiques globales
        self.total_cycles = 0
        self.total_episodes = 0
        self.total_victories = 0
        
    def _load_all_puzzles(self) -> Dict[str, Dict]:
        """Charge TOUS les puzzles ARC (training + evaluation)"""
        import os
        import glob
        
        all_puzzles = {}
        
        print(f"📂 Chargement depuis: {self.arc_data_path}")
        
        # Charger training
        training_path = f"{self.arc_data_path}/training/*.json"
        training_files = glob.glob(training_path)
        print(f"   Training: {len(training_files)} fichiers trouvés")
        
        for filepath in training_files:
            try:
                puzzle_id = os.path.basename(filepath).replace('.json', '')
                with open(filepath, 'r') as f:
                    all_puzzles[puzzle_id] = json.load(f)
            except Exception as e:
                print(f"   ⚠️  Erreur chargement {filepath}: {e}")
                
        # Charger evaluation
        eval_path = f"{self.arc_data_path}/evaluation/*.json"
        eval_files = glob.glob(eval_path)
        print(f"   Evaluation: {len(eval_files)} fichiers trouvés")
        
        for filepath in eval_files:
            try:
                puzzle_id = os.path.basename(filepath).replace('.json', '')
                with open(filepath, 'r') as f:
                    all_puzzles[puzzle_id] = json.load(f)
            except Exception as e:
                print(f"   ⚠️  Erreur chargement {filepath}: {e}")
                
        print(f"✅ Total chargé: {len(all_puzzles)} puzzles")
        return all_puzzles
        
    def select_next_puzzle(self) -> Tuple[str, int]:
        """
        Sélectionne puzzle suivant selon priorités:
        1. Puzzles jamais tentés
        2. Puzzles avec progression récente
        3. Puzzles nécessitant révision
        4. Puzzles aléatoires (exploration)
        """
        # Priorité 1: Nouveaux puzzles
        untried = [pid for pid, prog in self.puzzle_progress.items() 
                   if prog.attempts == 0]
        if untried:
            puzzle_id = random.choice(untried)
            return puzzle_id, 1
            
        # Priorité 2: Puzzles en progression
        improving = self._get_improving_puzzles()
        if improving:
            puzzle_id = self._select_by_improvement_rate(improving)
            level = self.puzzle_progress[puzzle_id].current_level
            return puzzle_id, level
            
        # Priorité 3: Révision
        needing_revision = self._get_puzzles_needing_revision()
        if needing_revision:
            puzzle_id = random.choice(needing_revision)
            level = self.puzzle_progress[puzzle_id].current_level
            return puzzle_id, level
            
        # Priorité 4: Exploration aléatoire
        puzzle_id = random.choice(list(self.all_puzzles.keys()))
        level = self.puzzle_progress[puzzle_id].current_level
        return puzzle_id, level
        
    def _get_improving_puzzles(self) -> List[str]:
        """Puzzles montrant amélioration récente"""
        improving = []
        for pid, prog in self.puzzle_progress.items():
            if prog.attempts > 0 and not prog.mastered:
                # Amélioration si victoires récentes
                if prog.consecutive_wins > 0:
                    improving.append(pid)
        return improving
        
    def _select_by_improvement_rate(self, puzzle_ids: List[str]) -> str:
        """Sélectionne puzzle avec meilleur taux d'amélioration"""
        best_pid = puzzle_ids[0]
        best_rate = 0.0
        
        for pid in puzzle_ids:
            prog = self.puzzle_progress[pid]
            rate = prog.success_rate + (prog.consecutive_wins * 0.1)
            if rate > best_rate:
                best_rate = rate
                best_pid = pid
                
        return best_pid
        
    def _get_puzzles_needing_revision(self) -> List[str]:
        """Puzzles nécessitant révision"""
        needing_revision = []
        
        for pid, prog in self.puzzle_progress.items():
            # Révision si:
            # 1. Maîtrisé mais pas joué récemment
            if prog.mastered and len(self.selection_history) > 100:
                if pid not in self.selection_history[-100:]:
                    needing_revision.append(pid)
                    
            # 2. Échecs consécutifs après succès
            if prog.victories > 0 and prog.consecutive_losses > 3:
                needing_revision.append(pid)
                
        return needing_revision
        
    def record_result(self, puzzle_id: str, level: int, victory: bool, reward: float):
        """Enregistre résultat d'un épisode"""
        prog = self.puzzle_progress[puzzle_id]
        
        # Mettre à jour statistiques
        prog.attempts += 1
        prog.last_attempt = datetime.now().isoformat()
        prog.best_reward = max(prog.best_reward, reward)
        
        if victory:
            prog.victories += 1
            prog.consecutive_wins += 1
            prog.consecutive_losses = 0
        else:
            prog.consecutive_losses += 1
            prog.consecutive_wins = 0
            
        # Calculer taux succès
        prog.success_rate = prog.victories / prog.attempts
        
        # Vérifier si maîtrisé
        if (prog.success_rate >= self.success_rate_threshold and
            prog.consecutive_wins >= self.consecutive_wins_required):
            if not prog.mastered:
                prog.mastered = True
                print(f"🏆 Puzzle {puzzle_id} MAÎTRISÉ ! ({prog.success_rate:.1%} succès)")
                
            # Débloquer niveau suivant
            if level == prog.max_level_unlocked:
                prog.max_level_unlocked += 1
                prog.current_level = prog.max_level_unlocked
                print(f"🔓 Niveau {prog.max_level_unlocked} débloqué pour {puzzle_id}")
        
        # Historique
        self.selection_history.append(puzzle_id)
        if len(self.selection_history) > self.max_history:
            self.selection_history.pop(0)
            
        # Stats globales
        self.total_episodes += 1
        if victory:
            self.total_victories += 1
            
    def get_active_puzzles(self) -> List[Tuple[str, int]]:
        """Retourne liste puzzles actifs pour rotation"""
        active = []
        
        # Tous les puzzles non maîtrisés
        for pid, prog in self.puzzle_progress.items():
            if not prog.mastered:
                active.append((pid, prog.current_level))
                
        # Si tous maîtrisés, inclure tous pour révision
        if not active:
            active = [(pid, prog.current_level) 
                     for pid, prog in self.puzzle_progress.items()]
                     
        return active
        
    def get_statistics(self) -> Dict:
        """Statistiques globales"""
        mastered_count = sum(1 for p in self.puzzle_progress.values() if p.mastered)
        attempted_count = sum(1 for p in self.puzzle_progress.values() if p.attempts > 0)
        
        return {
            'total_puzzles': len(self.all_puzzles),
            'puzzles_attempted': attempted_count,
            'puzzles_mastered': mastered_count,
            'puzzles_in_progress': attempted_count - mastered_count,
            'total_cycles': self.total_cycles,
            'total_episodes': self.total_episodes,
            'total_victories': self.total_victories,
            'global_success_rate': (self.total_victories / self.total_episodes 
                                   if self.total_episodes > 0 else 0.0),
            'mastery_rate': mastered_count / len(self.all_puzzles)
        }
        
    def get_puzzle_data(self, puzzle_id: str) -> Dict:
        """Récupère données d'un puzzle"""
        return self.all_puzzles.get(puzzle_id, {})
        
    def export_progress_report(self, filepath: str):
        """Exporte rapport progression"""
        stats = self.get_statistics()
        
        report = f"""
# RAPPORT PROGRESSION CURRICULUM
Date: {datetime.now().isoformat()}

## Statistiques Globales
- Total puzzles ARC: {stats['total_puzzles']}
- Puzzles tentés: {stats['puzzles_attempted']}
- Puzzles maîtrisés: {stats['puzzles_mastered']}
- Puzzles en cours: {stats['puzzles_in_progress']}
- Taux maîtrise: {stats['mastery_rate']:.2%}

## Progression
- Cycles complétés: {stats['total_cycles']}
- Épisodes joués: {stats['total_episodes']}
- Victoires totales: {stats['total_victories']}
- Taux succès global: {stats['global_success_rate']:.2%}

## Top 10 Puzzles Maîtrisés
"""
        # Top puzzles maîtrisés
        mastered = [(pid, prog) for pid, prog in self.puzzle_progress.items() 
                   if prog.mastered]
        mastered.sort(key=lambda x: x[1].success_rate, reverse=True)
        
        for i, (pid, prog) in enumerate(mastered[:10], 1):
            report += f"{i}. {pid}: {prog.success_rate:.1%} succès "
            report += f"({prog.victories}/{prog.attempts} victoires)\n"
            
        report += "\n## Top 10 Puzzles Difficiles\n"
        
        # Top puzzles difficiles
        difficult = [(pid, prog) for pid, prog in self.puzzle_progress.items() 
                    if prog.attempts > 10 and not prog.mastered]
        difficult.sort(key=lambda x: x[1].success_rate)
        
        for i, (pid, prog) in enumerate(difficult[:10], 1):
            report += f"{i}. {pid}: {prog.success_rate:.1%} succès "
            report += f"({prog.victories}/{prog.attempts} victoires)\n"
            
        with open(filepath, 'w') as f:
            f.write(report)

# Made with Bob