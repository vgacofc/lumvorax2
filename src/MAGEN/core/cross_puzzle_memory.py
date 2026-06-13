"""
MAGEN V23 - Cross-Puzzle Memory
================================

Objectif: Mémoriser et réutiliser connaissances entre puzzles.

Principe: Construire une base de connaissances globale qui s'enrichit
au fur et à mesure des puzzles résolus.

Différence avec V22:
- V22: Chaque puzzle traité indépendamment (adaptive_recoveries = 0)
- V23: Apprentissage cumulatif cross-puzzle (adaptive_recoveries > 0)
"""

import numpy as np
from typing import List, Dict, Any, Optional, Set, Tuple
from dataclasses import dataclass, field
from collections import defaultdict
import json
from pathlib import Path
from datetime import datetime


@dataclass
class PuzzlePattern:
    """Pattern réutilisable extrait d'un puzzle résolu"""
    pattern_id: str
    pattern_type: str  # transformation, rule, color_mapping, etc.
    description: str
    formal_representation: Dict[str, Any]
    success_count: int = 0
    failure_count: int = 0
    confidence: float = 0.5
    puzzles_solved: List[str] = field(default_factory=list)
    created_at: str = field(default_factory=lambda: datetime.now().isoformat())
    
    @property
    def success_rate(self) -> float:
        """Taux de succès du pattern"""
        total = self.success_count + self.failure_count
        return self.success_count / total if total > 0 else 0.5
    
    def update_success(self, puzzle_id: str):
        """Mettre à jour après succès"""
        self.success_count += 1
        if puzzle_id not in self.puzzles_solved:
            self.puzzles_solved.append(puzzle_id)
        self._update_confidence()
    
    def update_failure(self):
        """Mettre à jour après échec"""
        self.failure_count += 1
        self._update_confidence()
    
    def _update_confidence(self):
        """Recalculer confiance basée sur succès/échecs"""
        # Formule: confidence = success_rate avec pénalité si peu d'essais
        total = self.success_count + self.failure_count
        if total < 5:
            # Pénalité si peu d'essais (incertitude)
            penalty = (5 - total) * 0.1
            self.confidence = max(0.0, self.success_rate - penalty)
        else:
            self.confidence = self.success_rate
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertir en dictionnaire"""
        return {
            'pattern_id': self.pattern_id,
            'pattern_type': self.pattern_type,
            'description': self.description,
            'formal_representation': self.formal_representation,
            'success_count': self.success_count,
            'failure_count': self.failure_count,
            'confidence': self.confidence,
            'puzzles_solved': self.puzzles_solved,
            'created_at': self.created_at
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PuzzlePattern':
        """Créer depuis dictionnaire"""
        return cls(
            pattern_id=data['pattern_id'],
            pattern_type=data['pattern_type'],
            description=data['description'],
            formal_representation=data['formal_representation'],
            success_count=data.get('success_count', 0),
            failure_count=data.get('failure_count', 0),
            confidence=data.get('confidence', 0.5),
            puzzles_solved=data.get('puzzles_solved', []),
            created_at=data.get('created_at', datetime.now().isoformat())
        )


@dataclass
class StrategyAdaptation:
    """Adaptation de stratégie basée sur feedback"""
    strategy_name: str
    original_params: Dict[str, Any]
    adapted_params: Dict[str, Any]
    improvement: float  # Amélioration mesurée (peut être négative)
    context: str  # Contexte où l'adaptation a été faite
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())


class CrossPuzzleMemory:
    """
    Mémoire globale cross-puzzle pour apprentissage cumulatif.
    
    Fonctionnalités:
    1. Mémoriser patterns réussis
    2. Indexer par type de puzzle
    3. Suggérer patterns pour nouveaux puzzles
    4. Adapter stratégies basé sur feedback
    5. Construire base de connaissances
    """
    
    def __init__(self, memory_dir: str = "memory", verbose: bool = False):
        """
        Args:
            memory_dir: Répertoire pour sauvegarder mémoire
            verbose: Mode verbeux
        """
        self.memory_dir = Path(memory_dir)
        self.memory_dir.mkdir(exist_ok=True)
        self.verbose = verbose
        
        # Base de patterns
        self.patterns: Dict[str, PuzzlePattern] = {}
        
        # Index par type
        self.patterns_by_type: Dict[str, List[str]] = defaultdict(list)
        
        # Adaptations de stratégies
        self.strategy_adaptations: List[StrategyAdaptation] = []
        
        # Statistiques
        self.stats = {
            'total_patterns': 0,
            'total_reuses': 0,
            'successful_reuses': 0,
            'failed_reuses': 0,
            'adaptive_recoveries': 0,  # MÉTRIQUE CRITIQUE V23
            'puzzles_learned_from': 0
        }
        
        # Charger mémoire existante
        self._load_memory()
    
    def learn_from_success(
        self,
        puzzle_id: str,
        explanation: Any,  # Explanation object
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> str:
        """
        Apprendre d'un puzzle résolu avec succès.
        
        Extrait le pattern réutilisable et l'ajoute à la mémoire.
        
        Args:
            puzzle_id: ID du puzzle
            explanation: Explication qui a fonctionné
            train_pairs: Paires d'entraînement
            
        Returns:
            ID du pattern créé
        """
        # Créer pattern depuis explication
        pattern_id = f"{explanation.type.value}_{len(self.patterns)}"
        
        pattern = PuzzlePattern(
            pattern_id=pattern_id,
            pattern_type=explanation.type.value,
            description=explanation.description,
            formal_representation=explanation.formal_rule,
            success_count=1,
            failure_count=0,
            confidence=explanation.confidence,
            puzzles_solved=[puzzle_id]
        )
        
        # Ajouter à la mémoire
        self.patterns[pattern_id] = pattern
        self.patterns_by_type[pattern.pattern_type].append(pattern_id)
        
        # Statistiques
        self.stats['total_patterns'] += 1
        self.stats['puzzles_learned_from'] += 1
        
        if self.verbose:
            print(f"   📚 Pattern learned: {pattern_id}")
            print(f"      Type: {pattern.pattern_type}")
            print(f"      Confidence: {pattern.confidence:.2f}")
        
        # Sauvegarder
        self._save_memory()
        
        return pattern_id
    
    def suggest_patterns(
        self,
        puzzle_context: Dict[str, Any],
        top_k: int = 5
    ) -> List[PuzzlePattern]:
        """
        Suggérer patterns pertinents pour un nouveau puzzle.
        
        Args:
            puzzle_context: Contexte du puzzle (taille, couleurs, etc.)
            top_k: Nombre de suggestions
            
        Returns:
            Liste de patterns triés par pertinence
        """
        if not self.patterns:
            return []
        
        # Scorer chaque pattern
        scored_patterns = []
        for pattern in self.patterns.values():
            score = self._compute_relevance_score(pattern, puzzle_context)
            scored_patterns.append((score, pattern))
        
        # Trier par score décroissant
        scored_patterns.sort(key=lambda x: x[0], reverse=True)
        
        # Retourner top-k
        suggestions = [p for _, p in scored_patterns[:top_k]]
        
        if self.verbose and suggestions:
            print(f"\n   💡 Pattern suggestions: {len(suggestions)}")
            for i, pattern in enumerate(suggestions[:3], 1):
                print(f"      {i}. {pattern.pattern_type} (conf={pattern.confidence:.2f})")
        
        return suggestions
    
    def record_pattern_reuse(
        self,
        pattern_id: str,
        puzzle_id: str,
        success: bool
    ):
        """
        Enregistrer réutilisation d'un pattern.
        
        Args:
            pattern_id: ID du pattern réutilisé
            puzzle_id: ID du puzzle
            success: Si la réutilisation a réussi
        """
        if pattern_id not in self.patterns:
            return
        
        pattern = self.patterns[pattern_id]
        
        if success:
            pattern.update_success(puzzle_id)
            self.stats['successful_reuses'] += 1
            self.stats['adaptive_recoveries'] += 1  # MÉTRIQUE CRITIQUE
            
            if self.verbose:
                print(f"   ✅ Pattern reused successfully: {pattern_id}")
        else:
            pattern.update_failure()
            self.stats['failed_reuses'] += 1
            
            if self.verbose:
                print(f"   ❌ Pattern reuse failed: {pattern_id}")
        
        self.stats['total_reuses'] += 1
        
        # Sauvegarder
        self._save_memory()
    
    def adapt_strategy(
        self,
        strategy_name: str,
        original_params: Dict[str, Any],
        feedback: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Adapter paramètres de stratégie basé sur feedback.
        
        Args:
            strategy_name: Nom de la stratégie
            original_params: Paramètres originaux
            feedback: Feedback (succès, temps, etc.)
            
        Returns:
            Paramètres adaptés
        """
        # Pour l'instant, adaptation simple
        adapted_params = original_params.copy()
        
        # Si échec, augmenter exploration
        if not feedback.get('success', False):
            if 'max_attempts' in adapted_params:
                adapted_params['max_attempts'] = int(adapted_params['max_attempts'] * 1.5)
            if 'exploration_rate' in adapted_params:
                adapted_params['exploration_rate'] = min(1.0, adapted_params['exploration_rate'] * 1.2)
        
        # Si succès rapide, réduire exploration
        elif feedback.get('time', 1.0) < 0.1:
            if 'max_attempts' in adapted_params:
                adapted_params['max_attempts'] = max(1, int(adapted_params['max_attempts'] * 0.8))
        
        # Enregistrer adaptation
        improvement = 1.0 if feedback.get('success', False) else -1.0
        adaptation = StrategyAdaptation(
            strategy_name=strategy_name,
            original_params=original_params,
            adapted_params=adapted_params,
            improvement=improvement,
            context=feedback.get('context', 'unknown')
        )
        self.strategy_adaptations.append(adaptation)
        
        if self.verbose:
            print(f"   🔧 Strategy adapted: {strategy_name}")
            print(f"      Improvement: {improvement:+.2f}")
        
        return adapted_params
    
    def get_learning_curve(self) -> List[Tuple[int, float]]:
        """
        Obtenir courbe d'apprentissage.
        
        Returns:
            Liste de (nombre_puzzles, taux_succès)
        """
        if not self.patterns:
            return []
        
        # Trier patterns par date de création
        sorted_patterns = sorted(
            self.patterns.values(),
            key=lambda p: p.created_at
        )
        
        curve = []
        cumulative_success = 0
        cumulative_total = 0
        
        for i, pattern in enumerate(sorted_patterns, 1):
            cumulative_success += pattern.success_count
            cumulative_total += pattern.success_count + pattern.failure_count
            
            if cumulative_total > 0:
                success_rate = cumulative_success / cumulative_total
                curve.append((i, success_rate))
        
        return curve
    
    def _compute_relevance_score(
        self,
        pattern: PuzzlePattern,
        puzzle_context: Dict[str, Any]
    ) -> float:
        """Calculer score de pertinence d'un pattern pour un puzzle"""
        score = pattern.confidence
        
        # Bonus si pattern a beaucoup de succès
        if pattern.success_count > 5:
            score += 0.1
        
        # Bonus si pattern récent
        # TODO: Implémenter bonus temporel
        
        # Pénalité si pattern a beaucoup d'échecs
        if pattern.failure_count > pattern.success_count:
            score -= 0.2
        
        return max(0.0, min(1.0, score))
    
    def _load_memory(self):
        """Charger mémoire depuis disque"""
        memory_file = self.memory_dir / "cross_puzzle_memory.json"
        
        if not memory_file.exists():
            if self.verbose:
                print("   📂 No existing memory found, starting fresh")
            return
        
        try:
            with open(memory_file, 'r') as f:
                data = json.load(f)
            
            # Charger patterns
            for pattern_data in data.get('patterns', []):
                pattern = PuzzlePattern.from_dict(pattern_data)
                self.patterns[pattern.pattern_id] = pattern
                self.patterns_by_type[pattern.pattern_type].append(pattern.pattern_id)
            
            # Charger statistiques
            self.stats.update(data.get('stats', {}))
            
            if self.verbose:
                print(f"   📂 Memory loaded: {len(self.patterns)} patterns")
                print(f"      Adaptive recoveries: {self.stats['adaptive_recoveries']}")
        
        except Exception as e:
            if self.verbose:
                print(f"   ⚠️  Error loading memory: {e}")
    
    def _save_memory(self):
        """Sauvegarder mémoire sur disque"""
        memory_file = self.memory_dir / "cross_puzzle_memory.json"
        
        try:
            data = {
                'patterns': [p.to_dict() for p in self.patterns.values()],
                'stats': self.stats,
                'last_updated': datetime.now().isoformat()
            }
            
            with open(memory_file, 'w') as f:
                json.dump(data, f, indent=2)
        
        except Exception as e:
            if self.verbose:
                print(f"   ⚠️  Error saving memory: {e}")
    
    def get_stats(self) -> Dict[str, Any]:
        """Obtenir statistiques"""
        stats: Dict[str, Any] = self.stats.copy()
        
        # Ajouter métriques dérivées
        if stats['total_reuses'] > 0:
            stats['reuse_success_rate'] = float(stats['successful_reuses']) / float(stats['total_reuses'])
        else:
            stats['reuse_success_rate'] = 0.0
        
        # Courbe d'apprentissage
        curve = self.get_learning_curve()
        if curve:
            stats['learning_curve_length'] = len(curve)
            stats['final_success_rate'] = float(curve[-1][1])
        
        return stats
    
    def export_knowledge_base(self, output_file: str):
        """
        Exporter base de connaissances en format lisible.
        
        Args:
            output_file: Fichier de sortie
        """
        knowledge = {
            'metadata': {
                'total_patterns': len(self.patterns),
                'total_puzzles_learned': self.stats['puzzles_learned_from'],
                'adaptive_recoveries': self.stats['adaptive_recoveries'],
                'export_date': datetime.now().isoformat()
            },
            'patterns_by_type': {},
            'top_patterns': [],
            'learning_curve': self.get_learning_curve()
        }
        
        # Grouper patterns par type
        for pattern_type, pattern_ids in self.patterns_by_type.items():
            patterns = [self.patterns[pid].to_dict() for pid in pattern_ids]
            knowledge['patterns_by_type'][pattern_type] = patterns
        
        # Top patterns par confiance
        top_patterns = sorted(
            self.patterns.values(),
            key=lambda p: p.confidence,
            reverse=True
        )[:10]
        knowledge['top_patterns'] = [p.to_dict() for p in top_patterns]
        
        # Sauvegarder
        with open(output_file, 'w') as f:
            json.dump(knowledge, f, indent=2)
        
        if self.verbose:
            print(f"   💾 Knowledge base exported: {output_file}")

# Made with Bob
