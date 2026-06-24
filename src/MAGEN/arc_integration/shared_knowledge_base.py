"""
Shared Knowledge Base - Phase 4.6

Mémoire persistante inter-puzzles pour apprentissage curriculum.
Permet le transfert de connaissances entre TOUS les puzzles ARC.

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.0
"""

import pickle
import json
import numpy as np
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime
from collections import defaultdict


@dataclass
class Concept:
    """Concept réutilisable appris"""
    name: str
    description: str
    pattern: Dict[str, Any]
    confidence: float
    usage_count: int = 0
    success_rate: float = 0.0
    puzzles_learned_from: List[str] = field(default_factory=list)
    
@dataclass
class Strategy:
    """Stratégie transférable"""
    name: str
    description: str
    parameters: Dict[str, Any]
    success_rate: float
    usage_count: int = 0
    
@dataclass
class EpisodeMemory:
    """Mémoire d'un épisode"""
    puzzle_id: str
    episode_id: int
    timestamp: str
    victory: bool
    steps: int
    reward: float
    concepts_used: List[str]
    strategies_used: List[str]
    
class SharedKnowledgeBase:
    """
    Base de connaissances partagée entre tous les puzzles
    
    Permet:
    - Apprentissage mutualisé
    - Transfert de connaissances
    - Mémoire persistante
    - Concepts réutilisables
    """
    
    def __init__(self, save_path: str = "shared_knowledge.pkl"):
        self.save_path = save_path
        
        # Concepts réutilisables
        self.concepts: Dict[str, Concept] = {}
        
        # Stratégies transférables
        self.strategies: Dict[str, Strategy] = {}
        
        # Patterns d'échec à éviter
        self.failure_patterns: List[Dict] = []
        
        # Patterns de succès à reproduire
        self.success_patterns: List[Dict] = []
        
        # Mémoire épisodique (derniers 10000 épisodes)
        self.episode_memories: List[EpisodeMemory] = []
        self.max_memories = 10000
        
        # Statistiques par puzzle
        self.puzzle_stats: Dict[str, Dict] = defaultdict(lambda: {
            'attempts': 0,
            'victories': 0,
            'total_reward': 0.0,
            'best_reward': float('-inf'),
            'concepts_discovered': [],
            'last_attempt': None
        })
        
        # Transferts de connaissances réussis
        self.successful_transfers: List[Dict] = []
        
        # Charger connaissances existantes si disponibles
        self.load()
        
    def add_concept(self, concept: Concept):
        """Ajoute ou met à jour un concept"""
        if concept.name in self.concepts:
            # Mettre à jour concept existant
            existing = self.concepts[concept.name]
            existing.usage_count += 1
            existing.confidence = (existing.confidence + concept.confidence) / 2
            existing.puzzles_learned_from.extend(concept.puzzles_learned_from)
        else:
            # Nouveau concept
            self.concepts[concept.name] = concept
            
    def add_strategy(self, strategy: Strategy):
        """Ajoute ou met à jour une stratégie"""
        if strategy.name in self.strategies:
            existing = self.strategies[strategy.name]
            existing.usage_count += 1
            existing.success_rate = (
                (existing.success_rate * (existing.usage_count - 1) + 
                 strategy.success_rate) / existing.usage_count
            )
        else:
            self.strategies[strategy.name] = strategy
            
    def add_episode_memory(self, memory: EpisodeMemory):
        """Ajoute mémoire d'épisode"""
        self.episode_memories.append(memory)
        
        # Limiter taille mémoire
        if len(self.episode_memories) > self.max_memories:
            self.episode_memories.pop(0)
            
        # Mettre à jour stats puzzle
        stats = self.puzzle_stats[memory.puzzle_id]
        stats['attempts'] += 1
        if memory.victory:
            stats['victories'] += 1
        stats['total_reward'] += memory.reward
        stats['best_reward'] = max(stats['best_reward'], memory.reward)
        stats['last_attempt'] = memory.timestamp
        
    def get_concepts_for_puzzle(self, puzzle_id: str) -> List[Concept]:
        """Récupère concepts pertinents pour un puzzle"""
        # Retourner tous les concepts (transfert global)
        return list(self.concepts.values())
        
    def get_strategies_for_puzzle(self, puzzle_id: str) -> List[Strategy]:
        """Récupère stratégies pertinentes pour un puzzle"""
        # Trier par taux de succès
        return sorted(
            self.strategies.values(),
            key=lambda s: s.success_rate,
            reverse=True
        )
        
    def record_transfer(self, from_puzzle: str, to_puzzle: str, 
                       concept_name: str, success: bool):
        """Enregistre un transfert de connaissance"""
        transfer = {
            'from_puzzle': from_puzzle,
            'to_puzzle': to_puzzle,
            'concept': concept_name,
            'success': success,
            'timestamp': datetime.now().isoformat()
        }
        
        if success:
            self.successful_transfers.append(transfer)
            
    def get_puzzle_stats(self, puzzle_id: str) -> Dict:
        """Récupère statistiques d'un puzzle"""
        return self.puzzle_stats[puzzle_id]
        
    def get_global_stats(self) -> Dict:
        """Récupère statistiques globales"""
        total_attempts = sum(s['attempts'] for s in self.puzzle_stats.values())
        total_victories = sum(s['victories'] for s in self.puzzle_stats.values())
        
        return {
            'total_puzzles_attempted': len(self.puzzle_stats),
            'total_attempts': total_attempts,
            'total_victories': total_victories,
            'global_success_rate': total_victories / total_attempts if total_attempts > 0 else 0.0,
            'concepts_learned': len(self.concepts),
            'strategies_discovered': len(self.strategies),
            'successful_transfers': len(self.successful_transfers),
            'total_episodes': len(self.episode_memories)
        }
        
    def save(self):
        """Sauvegarde connaissances sur disque"""
        data = {
            'concepts': self.concepts,
            'strategies': self.strategies,
            'failure_patterns': self.failure_patterns,
            'success_patterns': self.success_patterns,
            'episode_memories': self.episode_memories[-1000:],  # Garder 1000 derniers
            'puzzle_stats': dict(self.puzzle_stats),
            'successful_transfers': self.successful_transfers,
            'timestamp': datetime.now().isoformat()
        }
        
        with open(self.save_path, 'wb') as f:
            pickle.dump(data, f)
            
    def load(self):
        """Charge connaissances depuis disque"""
        try:
            with open(self.save_path, 'rb') as f:
                data = pickle.load(f)
                
            self.concepts = data.get('concepts', {})
            self.strategies = data.get('strategies', {})
            self.failure_patterns = data.get('failure_patterns', [])
            self.success_patterns = data.get('success_patterns', [])
            self.episode_memories = data.get('episode_memories', [])
            self.puzzle_stats = defaultdict(
                lambda: {
                    'attempts': 0,
                    'victories': 0,
                    'total_reward': 0.0,
                    'best_reward': float('-inf'),
                    'concepts_discovered': [],
                    'last_attempt': None
                },
                data.get('puzzle_stats', {})
            )
            self.successful_transfers = data.get('successful_transfers', [])
            
            print(f"✅ Connaissances chargées: {len(self.concepts)} concepts, "
                  f"{len(self.strategies)} stratégies")
                  
        except FileNotFoundError:
            print("📝 Nouvelle base de connaissances créée")
        except Exception as e:
            print(f"⚠️  Erreur chargement connaissances: {e}")
            
    def export_report(self, filepath: str):
        """Exporte rapport détaillé"""
        stats = self.get_global_stats()
        
        report = f"""
# RAPPORT SHARED KNOWLEDGE BASE
Date: {datetime.now().isoformat()}

## Statistiques Globales
- Puzzles tentés: {stats['total_puzzles_attempted']}
- Tentatives totales: {stats['total_attempts']}
- Victoires totales: {stats['total_victories']}
- Taux succès global: {stats['global_success_rate']:.2%}

## Apprentissage
- Concepts appris: {stats['concepts_learned']}
- Stratégies découvertes: {stats['strategies_discovered']}
- Transferts réussis: {stats['successful_transfers']}
- Épisodes en mémoire: {stats['total_episodes']}

## Top 10 Concepts
"""
        # Ajouter top concepts
        top_concepts = sorted(
            self.concepts.values(),
            key=lambda c: c.usage_count,
            reverse=True
        )[:10]
        
        for i, concept in enumerate(top_concepts, 1):
            report += f"{i}. {concept.name} (utilisé {concept.usage_count} fois, "
            report += f"confiance {concept.confidence:.2f})\n"
            
        with open(filepath, 'w') as f:
            f.write(report)

# Made with Bob