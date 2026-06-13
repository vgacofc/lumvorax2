#!/usr/bin/env python3
"""
MAGEN - Architecture Bidirectionnelle Méta-Cognitive
====================================================

SESSION 55 - RÉVOLUTION APPRENTISSAGE

Insight utilisateur critique:
"apprend til comment il a réfléchi pour arriver à la solution du nouveau problème
et ce rappelle til de ces anciens raisonnements?"

PROBLÈME IDENTIFIÉ:
- V5-V9: Architecture unidirectionnelle (input → output)
- Pas d'apprentissage sur le PROCESSUS de raisonnement
- Pas de mémoire des STRATÉGIES qui fonctionnent
- Pas de transfert de PATTERNS de résolution

SOLUTION: Architecture Bidirectionnelle
┌─────────────────────────────────────────────────────────────┐
│  FORWARD: Puzzle → Raisonnement → Solution                  │
│     ↓                                                        │
│  BACKWARD: Solution → Analyse Raisonnement → Apprentissage  │
│     ↓                                                        │
│  MEMORY: Stockage Stratégies Réussies                       │
│     ↓                                                        │
│  TRANSFER: Application Stratégies Apprises → Nouveau Puzzle │
└─────────────────────────────────────────────────────────────┘

Protocole: CLAUDE_PILOT + LUMVORAX
Mode: 100% LOCAL
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any, Set, Callable
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
import json
import hashlib
import time
from collections import defaultdict


@dataclass
class ReasoningStep:
    """Un pas de raisonnement dans la résolution"""
    step_id: int
    timestamp: float
    step_type: str  # "perception", "hypothesis", "validation", "elimination"
    description: str
    input_state: Any
    output_state: Any
    success: bool
    confidence: float
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class ReasoningPath:
    """
    Chemin complet de raisonnement pour résoudre un puzzle
    
    Capture COMMENT le système a raisonné, pas seulement QUOI il a produit
    """
    puzzle_id: str
    puzzle_classification: str
    steps: List[ReasoningStep]
    final_solution: Optional[Any]
    success: bool
    total_time: float
    
    # Méta-informations
    key_insights: List[str]  # Insights critiques qui ont mené au succès
    dead_ends_avoided: List[str]  # Impasses évitées
    strategy_used: str  # Nom de la stratégie principale
    
    # Signature du pattern de raisonnement
    reasoning_signature: str  # Hash du pattern de raisonnement
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation"""
        return {
            'puzzle_id': self.puzzle_id,
            'puzzle_classification': self.puzzle_classification,
            'num_steps': len(self.steps),
            'success': self.success,
            'total_time': self.total_time,
            'key_insights': self.key_insights,
            'dead_ends_avoided': self.dead_ends_avoided,
            'strategy_used': self.strategy_used,
            'reasoning_signature': self.reasoning_signature
        }


@dataclass
class ReasoningStrategy:
    """
    Stratégie de raisonnement réutilisable
    
    Abstraction d'un pattern de raisonnement qui a fonctionné
    """
    strategy_id: str
    strategy_name: str
    applicable_classifications: List[str]
    
    # Pattern de raisonnement
    reasoning_template: List[str]  # Séquence de types de steps
    success_rate: float
    usage_count: int
    
    # Puzzles où cette stratégie a fonctionné
    successful_puzzles: List[str]
    failed_puzzles: List[str]
    
    # Conditions d'applicabilité
    preconditions: Dict[str, Any]
    expected_outcomes: Dict[str, Any]
    
    # Méta-données
    created_at: float
    last_used: float
    last_success: float
    
    def is_applicable(self, puzzle_features: Dict[str, Any]) -> Tuple[bool, float]:
        """
        Détermine si cette stratégie est applicable au puzzle
        
        Returns:
            (applicable, confidence)
        """
        confidence = 0.0
        
        # Vérifier classification
        if puzzle_features.get('classification') in self.applicable_classifications:
            confidence += 0.3
        
        # Vérifier préconditions
        matches = 0
        for key, expected_value in self.preconditions.items():
            if key in puzzle_features:
                if puzzle_features[key] == expected_value:
                    matches += 1
        
        if self.preconditions:
            confidence += 0.7 * (matches / len(self.preconditions))
        
        applicable = confidence > 0.5
        return applicable, confidence


class MetaCognitiveMemory:
    """
    Mémoire méta-cognitive: Se souvient COMMENT résoudre les problèmes
    
    Différence avec mémoire classique:
    - Mémoire classique: Se souvient des SOLUTIONS
    - Mémoire méta-cognitive: Se souvient des PROCESSUS DE RÉSOLUTION
    """
    
    def __init__(self, memory_dir: Optional[Path] = None):
        self.memory_dir = memory_dir or Path("memory/metacognitive")
        self.memory_dir.mkdir(parents=True, exist_ok=True)
        
        # Chemins de raisonnement stockés
        self.reasoning_paths: Dict[str, ReasoningPath] = {}
        
        # Stratégies extraites
        self.strategies: Dict[str, ReasoningStrategy] = {}
        
        # Index par classification
        self.strategies_by_classification: Dict[str, List[str]] = defaultdict(list)
        
        # Statistiques
        self.total_paths_stored = 0
        self.total_strategies_extracted = 0
        self.successful_transfers = 0
        self.failed_transfers = 0
        
        # Charger mémoire existante
        self._load_memory()
    
    def store_reasoning_path(self, path: ReasoningPath):
        """
        Stocke un chemin de raisonnement complet
        
        BACKWARD LEARNING: Analyse le succès pour extraire la stratégie
        """
        self.reasoning_paths[path.puzzle_id] = path
        self.total_paths_stored += 1
        
        # Si succès, extraire et stocker la stratégie
        if path.success:
            self._extract_and_store_strategy(path)
        
        # Sauvegarder
        self._save_reasoning_path(path)
    
    def _extract_and_store_strategy(self, path: ReasoningPath):
        """
        Extrait une stratégie réutilisable depuis un chemin de raisonnement réussi
        
        APPRENTISSAGE MÉTA-COGNITIF: Comprendre POURQUOI ça a marché
        """
        # Créer signature de la stratégie
        reasoning_template = [step.step_type for step in path.steps]
        strategy_signature = hashlib.sha256(
            f"{path.puzzle_classification}:{':'.join(reasoning_template)}".encode()
        ).hexdigest()[:16]
        
        # Vérifier si stratégie existe déjà
        if strategy_signature in self.strategies:
            # Mettre à jour stratégie existante
            strategy = self.strategies[strategy_signature]
            strategy.usage_count += 1
            strategy.successful_puzzles.append(path.puzzle_id)
            strategy.last_used = time.time()
            strategy.last_success = time.time()
            
            # Recalculer success_rate
            total = len(strategy.successful_puzzles) + len(strategy.failed_puzzles)
            strategy.success_rate = len(strategy.successful_puzzles) / total if total > 0 else 0.0
        else:
            # Créer nouvelle stratégie
            strategy = ReasoningStrategy(
                strategy_id=strategy_signature,
                strategy_name=f"{path.puzzle_classification}_{path.strategy_used}",
                applicable_classifications=[path.puzzle_classification],
                reasoning_template=reasoning_template,
                success_rate=1.0,
                usage_count=1,
                successful_puzzles=[path.puzzle_id],
                failed_puzzles=[],
                preconditions=self._extract_preconditions(path),
                expected_outcomes=self._extract_expected_outcomes(path),
                created_at=time.time(),
                last_used=time.time(),
                last_success=time.time()
            )
            
            self.strategies[strategy_signature] = strategy
            self.strategies_by_classification[path.puzzle_classification].append(strategy_signature)
            self.total_strategies_extracted += 1
        
        # Sauvegarder stratégie
        self._save_strategy(strategy)
    
    def _extract_preconditions(self, path: ReasoningPath) -> Dict[str, Any]:
        """Extrait les préconditions depuis le chemin de raisonnement"""
        preconditions = {}
        
        # Analyser les premiers steps pour identifier les conditions initiales
        if path.steps:
            first_step = path.steps[0]
            if 'grid_size' in first_step.metadata:
                preconditions['grid_size'] = first_step.metadata['grid_size']
            if 'num_colors' in first_step.metadata:
                preconditions['num_colors'] = first_step.metadata['num_colors']
            if 'has_symmetry' in first_step.metadata:
                preconditions['has_symmetry'] = first_step.metadata['has_symmetry']
        
        return preconditions
    
    def _extract_expected_outcomes(self, path: ReasoningPath) -> Dict[str, Any]:
        """Extrait les résultats attendus depuis le chemin de raisonnement"""
        outcomes = {}
        
        if path.steps:
            last_step = path.steps[-1]
            outcomes['final_confidence'] = last_step.confidence
            outcomes['num_steps'] = len(path.steps)
            outcomes['time_range'] = (0.0, path.total_time)
        
        return outcomes
    
    def retrieve_applicable_strategies(
        self, 
        puzzle_features: Dict[str, Any]
    ) -> List[Tuple[ReasoningStrategy, float]]:
        """
        Récupère les stratégies applicables au puzzle actuel
        
        TRANSFER LEARNING: Appliquer stratégies apprises à nouveau problème
        
        Returns:
            Liste de (stratégie, confidence) triée par confidence décroissante
        """
        applicable_strategies = []
        
        # Chercher dans stratégies de même classification
        classification = puzzle_features.get('classification', 'unknown')
        candidate_ids = self.strategies_by_classification.get(classification, [])
        
        # Ajouter stratégies de classifications similaires
        for cls, ids in self.strategies_by_classification.items():
            if cls != classification and cls in puzzle_features.get('similar_classifications', []):
                candidate_ids.extend(ids)
        
        # Évaluer applicabilité de chaque stratégie
        for strategy_id in candidate_ids:
            strategy = self.strategies[strategy_id]
            applicable, confidence = strategy.is_applicable(puzzle_features)
            
            if applicable:
                # Ajuster confidence par success_rate
                adjusted_confidence = confidence * strategy.success_rate
                applicable_strategies.append((strategy, adjusted_confidence))
        
        # Trier par confidence décroissante
        applicable_strategies.sort(key=lambda x: x[1], reverse=True)
        
        return applicable_strategies
    
    def apply_strategy_to_puzzle(
        self,
        strategy: ReasoningStrategy,
        puzzle_data: Dict[str, Any],
        execution_callback: Callable
    ) -> Tuple[bool, Any, ReasoningPath]:
        """
        Applique une stratégie apprise à un nouveau puzzle
        
        FORWARD APPLICATION: Utiliser stratégie mémorisée sur nouveau problème
        
        Args:
            strategy: Stratégie à appliquer
            puzzle_data: Données du puzzle
            execution_callback: Fonction qui exécute un step de raisonnement
        
        Returns:
            (success, solution, reasoning_path)
        """
        puzzle_id = puzzle_data['puzzle_id']
        classification = puzzle_data['classification']
        
        # Créer nouveau chemin de raisonnement
        steps = []
        start_time = time.time()
        
        # Suivre le template de raisonnement de la stratégie
        for i, step_type in enumerate(strategy.reasoning_template):
            try:
                # Exécuter step via callback
                step_result = execution_callback(
                    step_type=step_type,
                    puzzle_data=puzzle_data,
                    previous_steps=steps
                )
                
                # Créer ReasoningStep
                step = ReasoningStep(
                    step_id=i,
                    timestamp=time.time(),
                    step_type=step_type,
                    description=step_result.get('description', ''),
                    input_state=step_result.get('input_state'),
                    output_state=step_result.get('output_state'),
                    success=step_result.get('success', False),
                    confidence=step_result.get('confidence', 0.0),
                    metadata=step_result.get('metadata', {})
                )
                
                steps.append(step)
                
                # Si step échoue, arrêter
                if not step.success:
                    break
                    
            except Exception as e:
                # Step a échoué
                step = ReasoningStep(
                    step_id=i,
                    timestamp=time.time(),
                    step_type=step_type,
                    description=f"Error: {str(e)}",
                    input_state=None,
                    output_state=None,
                    success=False,
                    confidence=0.0,
                    metadata={'error': str(e)}
                )
                steps.append(step)
                break
        
        # Évaluer succès global
        success = all(step.success for step in steps) and len(steps) == len(strategy.reasoning_template)
        solution = steps[-1].output_state if steps and steps[-1].success else None
        
        # Créer ReasoningPath
        path = ReasoningPath(
            puzzle_id=puzzle_id,
            puzzle_classification=classification,
            steps=steps,
            final_solution=solution,
            success=success,
            total_time=time.time() - start_time,
            key_insights=[f"Applied strategy: {strategy.strategy_name}"],
            dead_ends_avoided=[],
            strategy_used=strategy.strategy_name,
            reasoning_signature=strategy.strategy_id
        )
        
        # Mettre à jour statistiques
        if success:
            self.successful_transfers += 1
            strategy.successful_puzzles.append(puzzle_id)
        else:
            self.failed_transfers += 1
            strategy.failed_puzzles.append(puzzle_id)
        
        # Recalculer success_rate de la stratégie
        total = len(strategy.successful_puzzles) + len(strategy.failed_puzzles)
        strategy.success_rate = len(strategy.successful_puzzles) / total if total > 0 else 0.0
        strategy.last_used = time.time()
        if success:
            strategy.last_success = time.time()
        
        return success, solution, path
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques méta-cognitives"""
        return {
            'total_paths_stored': self.total_paths_stored,
            'total_strategies_extracted': self.total_strategies_extracted,
            'successful_transfers': self.successful_transfers,
            'failed_transfers': self.failed_transfers,
            'transfer_success_rate': (
                self.successful_transfers / (self.successful_transfers + self.failed_transfers)
                if (self.successful_transfers + self.failed_transfers) > 0 else 0.0
            ),
            'strategies_by_classification': {
                cls: len(ids) for cls, ids in self.strategies_by_classification.items()
            },
            'top_strategies': self._get_top_strategies(5)
        }
    
    def _get_top_strategies(self, n: int = 5) -> List[Dict[str, Any]]:
        """Retourne les n meilleures stratégies"""
        sorted_strategies = sorted(
            self.strategies.values(),
            key=lambda s: (s.success_rate, s.usage_count),
            reverse=True
        )
        
        return [
            {
                'strategy_name': s.strategy_name,
                'success_rate': s.success_rate,
                'usage_count': s.usage_count,
                'applicable_classifications': s.applicable_classifications
            }
            for s in sorted_strategies[:n]
        ]
    
    def _save_reasoning_path(self, path: ReasoningPath):
        """Sauvegarde un chemin de raisonnement"""
        path_file = self.memory_dir / f"path_{path.puzzle_id}.json"
        with open(path_file, 'w') as f:
            json.dump(path.to_dict(), f, indent=2)
    
    def _save_strategy(self, strategy: ReasoningStrategy):
        """Sauvegarde une stratégie"""
        strategy_file = self.memory_dir / f"strategy_{strategy.strategy_id}.json"
        with open(strategy_file, 'w') as f:
            json.dump({
                'strategy_id': strategy.strategy_id,
                'strategy_name': strategy.strategy_name,
                'applicable_classifications': strategy.applicable_classifications,
                'reasoning_template': strategy.reasoning_template,
                'success_rate': strategy.success_rate,
                'usage_count': strategy.usage_count,
                'successful_puzzles': strategy.successful_puzzles,
                'failed_puzzles': strategy.failed_puzzles,
                'preconditions': strategy.preconditions,
                'expected_outcomes': strategy.expected_outcomes,
                'created_at': strategy.created_at,
                'last_used': strategy.last_used,
                'last_success': strategy.last_success
            }, f, indent=2)
    
    def _load_memory(self):
        """Charge la mémoire méta-cognitive depuis disque"""
        # Charger stratégies
        for strategy_file in self.memory_dir.glob("strategy_*.json"):
            try:
                with open(strategy_file) as f:
                    data = json.load(f)
                    strategy = ReasoningStrategy(**data)
                    self.strategies[strategy.strategy_id] = strategy
                    for cls in strategy.applicable_classifications:
                        self.strategies_by_classification[cls].append(strategy.strategy_id)
                    self.total_strategies_extracted += 1
            except Exception as e:
                print(f"Warning: Could not load strategy {strategy_file}: {e}")


class BidirectionalLearningLoop:
    """
    Boucle d'apprentissage bidirectionnelle
    
    FORWARD: Puzzle → Raisonnement → Solution
    BACKWARD: Solution → Analyse → Apprentissage → Mémoire
    TRANSFER: Mémoire → Application → Nouveau Puzzle
    """
    
    def __init__(self, metacognitive_memory: MetaCognitiveMemory):
        self.memory = metacognitive_memory
        self.current_reasoning_steps: List[ReasoningStep] = []
        self.current_puzzle_id: Optional[str] = None
        self.current_classification: Optional[str] = None
        self.start_time: float = 0.0
    
    def start_puzzle(self, puzzle_id: str, classification: str):
        """Démarre le raisonnement sur un nouveau puzzle"""
        self.current_puzzle_id = puzzle_id
        self.current_classification = classification
        self.current_reasoning_steps = []
        self.start_time = time.time()
    
    def add_reasoning_step(
        self,
        step_type: str,
        description: str,
        input_state: Any,
        output_state: Any,
        success: bool,
        confidence: float,
        metadata: Optional[Dict[str, Any]] = None
    ):
        """Ajoute un pas de raisonnement"""
        step = ReasoningStep(
            step_id=len(self.current_reasoning_steps),
            timestamp=time.time(),
            step_type=step_type,
            description=description,
            input_state=input_state,
            output_state=output_state,
            success=success,
            confidence=confidence,
            metadata=metadata or {}
        )
        self.current_reasoning_steps.append(step)
    
    def complete_puzzle(
        self,
        solution: Optional[Any],
        success: bool,
        strategy_used: str,
        key_insights: Optional[List[str]] = None,
        dead_ends_avoided: Optional[List[str]] = None
    ):
        """
        Complète le puzzle et déclenche l'apprentissage BACKWARD
        
        C'est ici que la magie opère: on analyse COMMENT on a résolu
        """
        # Créer chemin de raisonnement complet
        path = ReasoningPath(
            puzzle_id=self.current_puzzle_id or "unknown",
            puzzle_classification=self.current_classification or "unknown",
            steps=self.current_reasoning_steps,
            final_solution=solution,
            success=success,
            total_time=time.time() - self.start_time,
            key_insights=key_insights or [],
            dead_ends_avoided=dead_ends_avoided or [],
            strategy_used=strategy_used,
            reasoning_signature=self._compute_signature()
        )
        
        # BACKWARD LEARNING: Stocker et apprendre
        self.memory.store_reasoning_path(path)
        
        # Reset pour prochain puzzle
        self.current_reasoning_steps = []
        self.current_puzzle_id = None
        self.current_classification = None
    
    def _compute_signature(self) -> str:
        """Calcule signature du pattern de raisonnement"""
        template = ':'.join([step.step_type for step in self.current_reasoning_steps])
        signature = hashlib.sha256(
            f"{self.current_classification}:{template}".encode()
        ).hexdigest()[:16]
        return signature


# Export
__all__ = [
    'ReasoningStep',
    'ReasoningPath',
    'ReasoningStrategy',
    'MetaCognitiveMemory',
    'BidirectionalLearningLoop'
]

# Made with Bob
