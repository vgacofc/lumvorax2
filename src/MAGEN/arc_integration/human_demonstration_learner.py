#!/usr/bin/env python3
"""
PHASE 4.6.4 - Human Demonstration Learner (REAL DATA)
Apprentissage par imitation à partir de replays humains RÉELS.

Permet à MAGEN d'observer et d'apprendre des stratégies gagnantes
utilisées par les humains sur les puzzles ARC.

DONNÉES RÉELLES EXTRAITES:
- Session: 8aed7120-f7a9-45a1-837a-68bc7dc37a4f
- Game: ls20-9607627b
- Actions: 548 (7/7 levels, 100% success)
- Source: ARC Prize API (/api/recordings/)

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.4
"""

import sys
import os
import json
from typing import Dict, List, Optional
from dataclasses import dataclass, field
from datetime import datetime
import numpy as np
from pathlib import Path

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

from shared_knowledge_base import SharedKnowledgeBase, Strategy


@dataclass
class HumanAction:
    """Action humaine dans un replay RÉEL"""
    timestamp: str  # ISO format from NDJSON
    frame_number: int
    action_input: Optional[Dict]
    state: str  # "PLAYING", "LEVEL_COMPLETE", etc.
    levels_completed: int
    grid_state: np.ndarray


@dataclass
class HumanDemonstration:
    """Démonstration humaine complète (DONNÉES RÉELLES)"""
    session_id: str
    game_id: str
    total_actions: int
    total_levels_completed: int
    duration_seconds: float
    actions: List[HumanAction]
    success_rate: float
    metadata: Dict = field(default_factory=dict)


@dataclass
class ExtractedStrategy:
    """Stratégie extraite d'une démonstration"""
    name: str
    description: str
    pattern: Dict
    confidence: float
    applicable_puzzles: List[str]
    success_rate: float = 1.0


class HumanDemonstrationLearner:
    """
    Apprend des démonstrations humaines RÉELLES pour améliorer MAGEN.
    
    Fonctionnalités:
    1. Charger replays depuis fichiers JSON locaux
    2. Extraire séquences d'actions
    3. Identifier patterns récurrents
    4. Extraire stratégies réutilisables
    5. Stocker dans SharedKnowledgeBase
    """
    
    def __init__(self, 
                 knowledge_base: SharedKnowledgeBase,
                 replays_dir: str = "human_replays",
                 verbose: bool = True):
        self.knowledge = knowledge_base
        self.replays_dir = Path(replays_dir)
        self.verbose = verbose
        
        # Démonstrations chargées
        self.demonstrations: Dict[str, HumanDemonstration] = {}
        
        # Stratégies extraites
        self.extracted_strategies: Dict[str, ExtractedStrategy] = {}
        
        if self.verbose:
            print("\n" + "="*70)
            print("🎓 HUMAN DEMONSTRATION LEARNER - PHASE 4.6.4 (REAL DATA)")
            print("="*70)
            print("📚 Apprentissage par imitation activé")
            print(f"📁 Répertoire replays: {self.replays_dir}")
            print("="*70)
    
    def load_from_json(self, json_path: str) -> Optional[HumanDemonstration]:
        """
        Charge une démonstration depuis un fichier JSON local
        
        Args:
            json_path: Chemin vers demo_*.json
            
        Returns:
            HumanDemonstration si succès, None sinon
        """
        if self.verbose:
            print(f"\n📂 Chargement: {json_path}")
        
        try:
            with open(json_path, 'r') as f:
                data = json.load(f)
            
            # Reconstruire actions
            actions = []
            for action_data in data['actions']:
                action = HumanAction(
                    timestamp=action_data['timestamp'],
                    frame_number=action_data['frame_number'],
                    action_input=action_data.get('action_input'),
                    state=action_data['state'],
                    levels_completed=action_data['levels_completed'],
                    grid_state=np.array(action_data['grid_state'])
                )
                actions.append(action)
            
            demo = HumanDemonstration(
                session_id=data['session_id'],
                game_id=data['game_id'],
                total_actions=data['total_actions'],
                total_levels_completed=data['total_levels_completed'],
                duration_seconds=data['duration_seconds'],
                actions=actions,
                success_rate=data['success_rate'],
                metadata=data.get('metadata', {})
            )
            
            # Stocker
            self.demonstrations[demo.session_id] = demo
            
            if self.verbose:
                print(f"✅ Démonstration chargée: {demo.game_id}")
                print(f"   Session: {demo.session_id}")
                print(f"   Actions: {demo.total_actions}")
                print(f"   Levels: {demo.total_levels_completed}/7")
                print(f"   Durée: {demo.duration_seconds:.1f}s")
                print(f"   Succès: {demo.success_rate:.0%}")
            
            return demo
            
        except Exception as e:
            if self.verbose:
                print(f"❌ Erreur chargement: {e}")
            return None
    
    def load_all_replays(self) -> int:
        """
        Charge tous les replays du répertoire
        
        Returns:
            Nombre de replays chargés
        """
        if self.verbose:
            print(f"\n🔍 Recherche replays dans: {self.replays_dir}")
        
        if not self.replays_dir.exists():
            if self.verbose:
                print(f"⚠️  Répertoire non trouvé: {self.replays_dir}")
            return 0
        
        # Trouver tous les demo_*.json
        demo_files = list(self.replays_dir.glob("demo_*.json"))
        
        if self.verbose:
            print(f"📊 Trouvé {len(demo_files)} fichiers de démonstration")
        
        loaded = 0
        for demo_file in demo_files:
            if self.load_from_json(str(demo_file)):
                loaded += 1
        
        if self.verbose:
            print(f"\n✅ {loaded}/{len(demo_files)} replays chargés avec succès")
        
        return loaded
    
    def analyze_demonstration(self, demo: HumanDemonstration) -> Dict:
        """
        Analyse une démonstration pour extraire insights
        
        Args:
            demo: Démonstration à analyser
            
        Returns:
            Dictionnaire d'insights
        """
        if self.verbose:
            print(f"\n🔍 Analyse démonstration: {demo.session_id[:8]}...")
        
        insights = {
            'total_actions': demo.total_actions,
            'levels_completed': demo.total_levels_completed,
            'duration': demo.duration_seconds,
            'success_rate': demo.success_rate,
            'avg_actions_per_level': demo.total_actions / max(demo.total_levels_completed, 1),
            'avg_time_per_action': demo.duration_seconds / demo.total_actions,
            'level_progression': []
        }
        
        # Analyser progression par level
        current_level = 0
        level_start_frame = 0
        level_start_time = None
        
        for i, action in enumerate(demo.actions):
            if action.levels_completed > current_level:
                # Level complété
                if level_start_time:
                    from datetime import datetime
                    end_time = datetime.fromisoformat(action.timestamp.replace('Z', '+00:00'))
                    start_time = datetime.fromisoformat(level_start_time.replace('Z', '+00:00'))
                    level_duration = (end_time - start_time).total_seconds()
                    
                    insights['level_progression'].append({
                        'level': current_level + 1,
                        'actions': i - level_start_frame,
                        'duration': level_duration
                    })
                
                current_level = action.levels_completed
                level_start_frame = i
                level_start_time = action.timestamp
        
        # Dernier level
        if level_start_time and current_level > 0:
            last_action = demo.actions[-1]
            from datetime import datetime
            end_time = datetime.fromisoformat(last_action.timestamp.replace('Z', '+00:00'))
            start_time = datetime.fromisoformat(level_start_time.replace('Z', '+00:00'))
            level_duration = (end_time - start_time).total_seconds()
            
            insights['level_progression'].append({
                'level': current_level,
                'actions': len(demo.actions) - level_start_frame,
                'duration': level_duration
            })
        
        if self.verbose:
            print(f"   📊 Actions totales: {insights['total_actions']}")
            print(f"   🎯 Levels complétés: {insights['levels_completed']}/7")
            print(f"   ⏱️  Durée totale: {insights['duration']:.1f}s")
            print(f"   📈 Moy. actions/level: {insights['avg_actions_per_level']:.1f}")
            print(f"   ⚡ Moy. temps/action: {insights['avg_time_per_action']:.2f}s")
        
        return insights
    
    def extract_strategy(self, demo: HumanDemonstration) -> Optional[ExtractedStrategy]:
        """
        Extrait une stratégie réutilisable d'une démonstration
        
        Args:
            demo: Démonstration source
            
        Returns:
            ExtractedStrategy si extraction réussie
        """
        if self.verbose:
            print(f"\n🎯 Extraction stratégie: {demo.session_id[:8]}...")
        
        # Analyser d'abord
        insights = self.analyze_demonstration(demo)
        
        # Construire stratégie
        strategy_name = f"human_strategy_{demo.session_id[:8]}"
        
        # Pattern basé sur insights
        pattern = {
            'avg_actions_per_level': insights['avg_actions_per_level'],
            'avg_time_per_action': insights['avg_time_per_action'],
            'level_progression': insights['level_progression'],
            'game_id': demo.game_id
        }
        
        # Description
        description = (
            f"Stratégie humaine extraite du game {demo.game_id} "
            f"({demo.total_levels_completed}/7 levels, "
            f"{demo.total_actions} actions, "
            f"{demo.duration_seconds:.0f}s)"
        )
        
        # Confiance basée sur succès
        confidence = demo.success_rate
        
        strategy = ExtractedStrategy(
            name=strategy_name,
            description=description,
            pattern=pattern,
            confidence=confidence,
            applicable_puzzles=[demo.game_id],
            success_rate=demo.success_rate
        )
        
        # Stocker
        self.extracted_strategies[strategy_name] = strategy
        
        # Ajouter à la base de connaissances
        kb_strategy = Strategy(
            name=strategy_name,
            description=description,
            parameters=pattern,
            success_rate=strategy.success_rate
        )
        self.knowledge.add_strategy(kb_strategy)
        
        if self.verbose:
            print(f"✅ Stratégie extraite: {strategy_name}")
            print(f"   Confiance: {confidence:.0%}")
            print(f"   Applicable à: {demo.game_id}")
        
        return strategy
    
    def learn_from_all_replays(self) -> bool:
        """
        Pipeline complet: charger tous + analyser + extraire + stocker
        
        Returns:
            True si apprentissage réussi
        """
        if self.verbose:
            print("\n" + "="*70)
            print("🎓 APPRENTISSAGE DEPUIS REPLAYS HUMAINS RÉELS")
            print("="*70)
        
        # 1. Charger tous les replays
        loaded = self.load_all_replays()
        if loaded == 0:
            if self.verbose:
                print("❌ Aucun replay chargé")
            return False
        
        # 2. Analyser et extraire stratégies
        for demo in self.demonstrations.values():
            self.extract_strategy(demo)
        
        # 3. Sauvegarder connaissances
        self.knowledge.save()
        
        if self.verbose:
            print("\n✅ Apprentissage terminé avec succès")
            print(f"📚 {len(self.extracted_strategies)} stratégies ajoutées")
            print("="*70)
        
        return True
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques d'apprentissage"""
        return {
            'demonstrations_loaded': len(self.demonstrations),
            'strategies_extracted': len(self.extracted_strategies),
            'total_actions_observed': sum(d.total_actions for d in self.demonstrations.values()),
            'average_confidence': np.mean([s.confidence for s in self.extracted_strategies.values()]) if self.extracted_strategies else 0.0,
            'average_success_rate': np.mean([d.success_rate for d in self.demonstrations.values()]) if self.demonstrations else 0.0
        }


def main():
    """Test du Human Demonstration Learner avec données RÉELLES"""
    print("\n" + "="*70)
    print("🧪 TEST HUMAN DEMONSTRATION LEARNER - PHASE 4.6.4 (REAL DATA)")
    print("="*70)
    
    # Créer base de connaissances
    knowledge = SharedKnowledgeBase("test_human_learning.pkl")
    
    # Créer learner
    learner = HumanDemonstrationLearner(
        knowledge, 
        replays_dir="human_replays",
        verbose=True
    )
    
    # Apprendre depuis tous les replays
    success = learner.learn_from_all_replays()
    
    # Afficher statistiques
    stats = learner.get_statistics()
    print("\n📊 STATISTIQUES FINALES:")
    print(f"   Démonstrations: {stats['demonstrations_loaded']}")
    print(f"   Stratégies extraites: {stats['strategies_extracted']}")
    print(f"   Actions observées: {stats['total_actions_observed']}")
    print(f"   Confiance moyenne: {stats['average_confidence']:.0%}")
    print(f"   Taux succès moyen: {stats['average_success_rate']:.0%}")
    
    print("\n✅ Test terminé")
    print(f"   Succès: {'✅' if success else '❌'}")


if __name__ == "__main__":
    main()

# Made with ❤️ by Bob

# Made with Bob
