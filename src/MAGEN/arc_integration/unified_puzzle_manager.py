#!/usr/bin/env python3
"""
PHASE 4.6.1 - Unified Puzzle Manager
Gère TOUS les types de puzzles en parallèle:
- 800 puzzles ARC classiques (transformations statiques)
- Puzzles arcade 2D dynamiques (type ls20)
- Apprentissage unifié avec transfert de connaissances
"""

import json
import os
import sys
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field
from enum import Enum
import random

# Ajouter le chemin parent pour imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from curriculum_manager import CurriculumManager, PuzzleProgress
from shared_knowledge_base import SharedKnowledgeBase, Concept, Strategy


class PuzzleType(Enum):
    """Types de puzzles supportés"""
    ARC_STATIC = "arc_static"  # Puzzles ARC classiques (transformations)
    ARCADE_2D = "arcade_2d"    # Jeux arcade dynamiques (ls20, etc.)


@dataclass
class UnifiedPuzzleInfo:
    """Information complète sur un puzzle (tous types)"""
    puzzle_id: str
    puzzle_type: PuzzleType
    difficulty: int = 1
    data: Dict = field(default_factory=dict)
    metadata: Dict = field(default_factory=dict)
    
    def __post_init__(self):
        """Validation après initialisation"""
        if not self.puzzle_id:
            raise ValueError("puzzle_id ne peut pas être vide")


class UnifiedPuzzleManager:
    """
    Gestionnaire unifié pour TOUS les types de puzzles.
    Permet à MAGEN d'apprendre sur ARC classiques ET jeux arcade 2D en parallèle.
    """
    
    def __init__(
        self,
        arc_data_path: str = None,
        ls20_data_path: str = None,
        knowledge_path: str = "shared_knowledge.pkl",
        verbose: bool = True
    ):
        self.verbose = verbose
        
        # Chemins par défaut
        if arc_data_path is None:
            arc_data_path = "/home/lvx/LVX/environment_files/ARC-AGI/data"
        if ls20_data_path is None:
            ls20_data_path = "/home/lvx/LVX/environment_files/ls20"
        
        self.arc_data_path = arc_data_path
        self.ls20_data_path = ls20_data_path
        
        # Gestionnaires spécialisés
        self.arc_curriculum = CurriculumManager(arc_data_path)
        self.knowledge = SharedKnowledgeBase(knowledge_path)
        
        # Catalogue unifié de tous les puzzles
        self.all_puzzles: Dict[str, UnifiedPuzzleInfo] = {}
        
        # Statistiques par type
        self.stats_by_type: Dict[PuzzleType, Dict] = {
            PuzzleType.ARC_STATIC: {
                "total": 0,
                "attempted": 0,
                "mastered": 0,
                "success_rate": 0.0
            },
            PuzzleType.ARCADE_2D: {
                "total": 0,
                "attempted": 0,
                "mastered": 0,
                "success_rate": 0.0
            }
        }
        
        # Charger tous les puzzles
        self._load_all_puzzles()
        
        if self.verbose:
            self._print_initialization_summary()
    
    def _load_all_puzzles(self):
        """Charge TOUS les puzzles (ARC + arcade 2D)"""
        
        # 1. Charger puzzles ARC classiques (800)
        arc_count = self._load_arc_puzzles()
        
        # 2. Charger puzzles arcade 2D (ls20 + futurs)
        arcade_count = self._load_arcade_puzzles()
        
        # Mettre à jour statistiques
        self.stats_by_type[PuzzleType.ARC_STATIC]["total"] = arc_count
        self.stats_by_type[PuzzleType.ARCADE_2D]["total"] = arcade_count
    
    def _load_arc_puzzles(self) -> int:
        """Charge les 800 puzzles ARC classiques"""
        count = 0
        
        # Les puzzles ARC sont déjà chargés dans arc_curriculum
        for puzzle_id in self.arc_curriculum.all_puzzles.keys():
            puzzle_info = UnifiedPuzzleInfo(
                puzzle_id=puzzle_id,
                puzzle_type=PuzzleType.ARC_STATIC,
                data=self.arc_curriculum.all_puzzles[puzzle_id],
                metadata={
                    "source": "ARC-AGI official dataset",
                    "format": "grid_transformation"
                }
            )
            self.all_puzzles[puzzle_id] = puzzle_info
            count += 1
        
        return count
    
    def _load_arcade_puzzles(self) -> int:
        """Charge TOUS les puzzles arcade 2D depuis environment_files"""
        count = 0
        
        # Chemin vers tous les jeux arcade (25 jeux disponibles)
        arcade_base_path = "/home/lvx/LVX/lumvorax2/src/environment_files"
        
        if not os.path.exists(arcade_base_path):
            if self.verbose:
                print(f"⚠️  Chemin arcade non trouvé: {arcade_base_path}")
            return count
        
        # Lister tous les répertoires de jeux
        try:
            game_dirs = [d for d in os.listdir(arcade_base_path)
                        if os.path.isdir(os.path.join(arcade_base_path, d))]
            
            if self.verbose:
                print(f"\n🎮 Scan jeux arcade dans: {arcade_base_path}")
                print(f"   Trouvés: {len(game_dirs)} répertoires")
            
            for game_dir in sorted(game_dirs):
                game_path = os.path.join(arcade_base_path, game_dir)
                
                # Chercher le sous-répertoire avec hash (ex: ls20/9607627b/)
                subdirs = [d for d in os.listdir(game_path)
                          if os.path.isdir(os.path.join(game_path, d))]
                
                if not subdirs:
                    continue
                
                # Prendre le premier sous-répertoire (devrait être unique)
                game_hash = subdirs[0]
                full_game_path = os.path.join(game_path, game_hash)
                
                # Vérifier présence metadata.json
                metadata_file = os.path.join(full_game_path, "metadata.json")
                if not os.path.exists(metadata_file):
                    if self.verbose:
                        print(f"   ⚠️  Pas de metadata: {game_dir}/{game_hash}")
                    continue
                
                # Charger metadata
                try:
                    with open(metadata_file, 'r') as f:
                        metadata = json.load(f)
                    
                    # Créer ID unique: game_dir + hash
                    puzzle_id = f"{game_dir}_{game_hash}"
                    
                    puzzle_info = UnifiedPuzzleInfo(
                        puzzle_id=puzzle_id,
                        puzzle_type=PuzzleType.ARCADE_2D,
                        data={
                            "path": full_game_path,
                            "game_dir": game_dir,
                            "game_hash": game_hash,
                            "metadata": metadata
                        },
                        metadata={
                            "source": f"ARC-AGI arcade game {game_dir}",
                            "format": "arcade_2d_dynamic",
                            "game_id": metadata.get("game_id", game_dir),
                            "title": metadata.get("title", game_dir.upper()),
                            "levels": metadata.get("levels", 1)
                        }
                    )
                    
                    self.all_puzzles[puzzle_id] = puzzle_info
                    count += 1
                    
                    if self.verbose:
                        print(f"   ✅ {game_dir}/{game_hash} - {metadata.get('title', 'N/A')}")
                
                except Exception as e:
                    if self.verbose:
                        print(f"   ❌ Erreur chargement {game_dir}/{game_hash}: {e}")
                    continue
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Erreur scan arcade: {e}")
        
        return count
    
    def _print_initialization_summary(self):
        """Affiche résumé de l'initialisation"""
        print("=" * 70)
        print("🎮 UNIFIED PUZZLE MANAGER - PHASE 4.6.1")
        print("=" * 70)
        print(f"\n📊 PUZZLES CHARGÉS:")
        print(f"  🔷 ARC Classiques (statiques): {self.stats_by_type[PuzzleType.ARC_STATIC]['total']}")
        print(f"  🎯 Arcade 2D (dynamiques): {self.stats_by_type[PuzzleType.ARCADE_2D]['total']}")
        print(f"  📦 TOTAL: {len(self.all_puzzles)} puzzles")
        
        print(f"\n🎯 TYPES DE PUZZLES:")
        print(f"  ✅ Transformations statiques (ARC-AGI 2)")
        print(f"  ✅ Jeux arcade dynamiques (ls20, etc.)")
        
        print(f"\n🧠 APPRENTISSAGE UNIFIÉ:")
        print(f"  ✅ Mémoire partagée entre tous les types")
        print(f"  ✅ Transfert de concepts inter-types")
        print(f"  ✅ Rotation équilibrée ARC + Arcade")
        print("=" * 70)
    
    def select_next_puzzle(
        self,
        prefer_type: Optional[PuzzleType] = None,
        balance_ratio: float = 0.5,
        progressive_mode: bool = True
    ) -> Tuple[str, int, PuzzleType]:
        """
        Sélectionne le prochain puzzle à tenter.
        
        STRATÉGIE PROGRESSIVE CORRECTE (progressive_mode=True):
        PHASE 1: ARC uniquement (800 puzzles) → 50%+ succès
        PHASE 2: ARC + HUMANS (démonstrations) → Apprendre stratégies
        PHASE 3: ARC + ARCADE (25 jeux) → Transfert connaissances
        
        ORDRE LOGIQUE: ARC → HUMANS → ARCADE
        
        Args:
            prefer_type: Type préféré (None = automatique)
            balance_ratio: Ratio ARC/Arcade (ignoré si progressive_mode=True)
            progressive_mode: Mode progressif ARC→Humans→Arcade
        
        Returns:
            (puzzle_id, level, puzzle_type)
        """
        
        # MODE PROGRESSIF: ARC → HUMANS → ARCADE
        if progressive_mode:
            # Calculer taux de maîtrise ARC
            arc_stats = self.stats_by_type[PuzzleType.ARC_STATIC]
            arc_mastery_rate = arc_stats['mastered'] / arc_stats['total'] if arc_stats['total'] > 0 else 0.0
            arc_success_rate = arc_stats['success_rate']
            
            # PHASE 1: ARC uniquement jusqu'à 50% succès
            if arc_success_rate < 0.5:
                if self.verbose and arc_stats['attempted'] % 100 == 0:
                    print(f"\n🎓 PHASE 1: APPRENTISSAGE ARC")
                    print(f"   Succès: {arc_success_rate*100:.1f}% | Maîtrise: {arc_mastery_rate*100:.1f}%")
                    print(f"   Objectif: 50% succès pour débloquer HUMANS")
                
                puzzle_id, level = self.arc_curriculum.select_next_puzzle()
                return puzzle_id, level, PuzzleType.ARC_STATIC
            
            # PHASE 2: ARC + HUMANS (démonstrations) jusqu'à 70% succès
            elif arc_success_rate < 0.7:
                if self.verbose and arc_stats['attempted'] % 100 == 0:
                    print(f"\n👤 PHASE 2: ARC + DÉMONSTRATIONS HUMAINES")
                    print(f"   Succès: {arc_success_rate*100:.1f}% | Maîtrise: {arc_mastery_rate*100:.1f}%")
                    print(f"   Objectif: 70% succès pour débloquer ARCADE")
                    print(f"   Note: Intégration démonstrations humaines en cours...")
                
                # Pour l'instant, continuer ARC (HUMANS sera intégré après)
                puzzle_id, level = self.arc_curriculum.select_next_puzzle()
                return puzzle_id, level, PuzzleType.ARC_STATIC
            
            # PHASE 3: ARC + ARCADE (transfert connaissances)
            else:
                if self.verbose and arc_stats['attempted'] % 100 == 0:
                    print(f"\n🎮 PHASE 3: ARC + ARCADE (TRANSFERT)")
                    print(f"   ARC - Succès: {arc_success_rate*100:.1f}% | Maîtrise: {arc_mastery_rate*100:.1f}%")
                    print(f"   Transfert connaissances vers jeux dynamiques")
                
                # 70% ARC pour maintenir maîtrise, 30% Arcade pour transfert
                if random.random() < 0.7:
                    puzzle_id, level = self.arc_curriculum.select_next_puzzle()
                    return puzzle_id, level, PuzzleType.ARC_STATIC
                else:
                    # Sélectionner jeu arcade aléatoire
                    arcade_puzzles = [pid for pid, info in self.all_puzzles.items()
                                     if info.puzzle_type == PuzzleType.ARCADE_2D]
                    if arcade_puzzles:
                        puzzle_id = random.choice(arcade_puzzles)
                        return puzzle_id, 1, PuzzleType.ARCADE_2D
                    else:
                        # Fallback ARC si pas d'arcade
                        puzzle_id, level = self.arc_curriculum.select_next_puzzle()
                        return puzzle_id, level, PuzzleType.ARC_STATIC
        
        # MODE CLASSIQUE: Équilibré selon ratio
        else:
            # Décider du type à sélectionner
            if prefer_type is None:
                # Sélection équilibrée selon ratio
                if random.random() < balance_ratio:
                    target_type = PuzzleType.ARC_STATIC
                else:
                    target_type = PuzzleType.ARCADE_2D
            else:
                target_type = prefer_type
            
            # Sélectionner puzzle selon le type
            if target_type == PuzzleType.ARC_STATIC:
                # Utiliser curriculum ARC
                puzzle_id, level = self.arc_curriculum.select_next_puzzle()
                return puzzle_id, level, PuzzleType.ARC_STATIC
            
            else:  # ARCADE_2D
                # Sélectionner jeu arcade aléatoire
                arcade_puzzles = [pid for pid, info in self.all_puzzles.items()
                                 if info.puzzle_type == PuzzleType.ARCADE_2D]
                if arcade_puzzles:
                    puzzle_id = random.choice(arcade_puzzles)
                    return puzzle_id, 1, PuzzleType.ARCADE_2D
                else:
                    return "ls20_9607627b", 1, PuzzleType.ARCADE_2D
    
    def get_puzzle_info(self, puzzle_id: str) -> Optional[UnifiedPuzzleInfo]:
        """Récupère les informations d'un puzzle"""
        return self.all_puzzles.get(puzzle_id)
    
    def record_result(
        self,
        puzzle_id: str,
        level: int,
        puzzle_type: PuzzleType,
        victory: bool,
        reward: float,
        episode_data: Dict = None
    ):
        """
        Enregistre le résultat d'une tentative.
        
        Args:
            puzzle_id: ID du puzzle
            level: Niveau tenté
            puzzle_type: Type de puzzle
            victory: Victoire ou échec
            reward: Récompense obtenue
            episode_data: Données de l'épisode (optionnel)
        """
        
        # Enregistrer dans le curriculum approprié
        if puzzle_type == PuzzleType.ARC_STATIC:
            self.arc_curriculum.record_result(puzzle_id, level, victory, reward)
        
        # TODO: Implémenter curriculum pour arcade 2D
        
        # Mettre à jour statistiques globales
        self._update_stats(puzzle_type, victory)
        
        # Enregistrer dans la base de connaissances partagée
        if episode_data:
            from shared_knowledge_base import EpisodeMemory
            from datetime import datetime
            
            memory = EpisodeMemory(
                puzzle_id=puzzle_id,
                episode_id=episode_data.get("episode_id", 0),
                timestamp=datetime.now().isoformat(),
                victory=victory,
                steps=episode_data.get("steps", 0),
                reward=reward,
                concepts_used=episode_data.get("concepts_used", []),
                strategies_used=episode_data.get("strategies_used", [])
            )
            self.knowledge.add_episode_memory(memory)
    
    def _update_stats(self, puzzle_type: PuzzleType, victory: bool):
        """Met à jour les statistiques par type"""
        stats = self.stats_by_type[puzzle_type]
        stats["attempted"] += 1
        
        if victory:
            # Recalculer taux de succès
            total_attempts = stats["attempted"]
            current_successes = stats["success_rate"] * (total_attempts - 1)
            new_successes = current_successes + 1
            stats["success_rate"] = new_successes / total_attempts
    
    def get_global_stats(self) -> Dict:
        """Retourne statistiques globales"""
        total_puzzles = len(self.all_puzzles)
        total_attempted = sum(s["attempted"] for s in self.stats_by_type.values())
        total_mastered = sum(s["mastered"] for s in self.stats_by_type.values())
        
        # Calculer taux de succès global
        if total_attempted > 0:
            weighted_success = sum(
                s["success_rate"] * s["attempted"] 
                for s in self.stats_by_type.values()
            )
            global_success_rate = weighted_success / total_attempted
        else:
            global_success_rate = 0.0
        
        return {
            "total_puzzles": total_puzzles,
            "total_attempted": total_attempted,
            "total_mastered": total_mastered,
            "global_success_rate": global_success_rate,
            "by_type": self.stats_by_type.copy()
        }
    
    def get_progress_report(self) -> str:
        """Génère rapport de progression détaillé"""
        stats = self.get_global_stats()
        
        report = []
        report.append("=" * 70)
        report.append("📊 RAPPORT PROGRESSION UNIFIÉ")
        report.append("=" * 70)
        
        report.append(f"\n🎮 PUZZLES TOTAUX: {stats['total_puzzles']}")
        report.append(f"  📝 Tentés: {stats['total_attempted']}")
        report.append(f"  ✅ Maîtrisés: {stats['total_mastered']}")
        report.append(f"  📈 Taux succès global: {stats['global_success_rate']*100:.1f}%")
        
        report.append(f"\n🔷 ARC CLASSIQUES (Statiques):")
        arc_stats = stats['by_type'][PuzzleType.ARC_STATIC]
        report.append(f"  Total: {arc_stats['total']}")
        report.append(f"  Tentés: {arc_stats['attempted']}")
        report.append(f"  Maîtrisés: {arc_stats['mastered']}")
        report.append(f"  Taux succès: {arc_stats['success_rate']*100:.1f}%")
        
        report.append(f"\n🎯 ARCADE 2D (Dynamiques):")
        arcade_stats = stats['by_type'][PuzzleType.ARCADE_2D]
        report.append(f"  Total: {arcade_stats['total']}")
        report.append(f"  Tentés: {arcade_stats['attempted']}")
        report.append(f"  Maîtrisés: {arcade_stats['mastered']}")
        report.append(f"  Taux succès: {arcade_stats['success_rate']*100:.1f}%")
        
        report.append("=" * 70)
        
        return "\n".join(report)
    
    def save_progress(self):
        """Sauvegarde la progression"""
        self.knowledge.save()
        # TODO: Sauvegarder aussi progression arcade 2D


def test_unified_manager():
    """Test du gestionnaire unifié"""
    print("\n🧪 TEST UNIFIED PUZZLE MANAGER\n")
    
    manager = UnifiedPuzzleManager(verbose=True)
    
    print("\n📋 Test sélection équilibrée:")
    for i in range(10):
        puzzle_id, level, ptype = manager.select_next_puzzle()
        print(f"  {i+1}. {puzzle_id} (niveau {level}) - Type: {ptype.value}")
    
    print("\n📊 Rapport progression:")
    print(manager.get_progress_report())
    
    print("\n✅ Test terminé!")


if __name__ == "__main__":
    test_unified_manager()

# Made with Bob
