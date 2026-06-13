"""
MAGEN - Pruning System
Système de pruning agressif pour contrôler explosion combinatoire

CRITIQUE ABSOLUE identifiée par analyse experte:
- Sans pruning: explosion combinatoire fatale
- Avec pruning: exploration contrôlée et efficace

Architecture:
- Program cost model (complexity penalty)
- Heuristic scoring (prior probabilities)
- Beam width control (top-k selection)
- Early rejection (fast filtering)
- Transform priors (learned probabilities)
- Canonical encoding (duplicate detection)
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Set, Optional
from dataclasses import dataclass, field
from collections import defaultdict
import hashlib

# Import modules MAGEN
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic import Transform, ComposedTransform, Hypothesis, TransformType
from objects import Object, SceneGraph, ObjectTransformation


@dataclass
class ProgramCost:
    """
    Coût d'un programme (composition de transformations)
    
    Composantes:
    - Complexity: Nombre de transformations
    - Parameter count: Nombre de paramètres
    - Description length: Longueur description
    - Execution cost: Coût d'exécution estimé
    """
    complexity: int = 0
    parameter_count: int = 0
    description_length: int = 0
    execution_cost: float = 0.0
    total_cost: float = 0.0
    
    def __repr__(self):
        return f"Cost(complexity={self.complexity}, params={self.parameter_count}, total={self.total_cost:.2f})"


@dataclass
class ScoredHypothesis:
    """
    Hypothèse avec score complet
    
    Score = accuracy - complexity_penalty - execution_cost
    """
    hypothesis: Hypothesis
    accuracy: float = 0.0
    complexity_penalty: float = 0.0
    execution_cost: float = 0.0
    heuristic_score: float = 0.0
    prior_probability: float = 0.0
    total_score: float = 0.0
    canonical_hash: str = ""
    
    def __repr__(self):
        return f"Scored[{self.hypothesis}] score={self.total_score:.3f}"


class ProgramCostModel:
    """
    Modèle de coût de programme
    
    Calcule le coût d'une composition de transformations
    selon plusieurs critères
    """
    
    def __init__(self, complexity_weight: float = 1.0,
                 parameter_weight: float = 0.5,
                 execution_weight: float = 0.3):
        self.complexity_weight = complexity_weight
        self.parameter_weight = parameter_weight
        self.execution_weight = execution_weight
        
        # Coûts par type de transformation
        self.transform_costs = self._initialize_transform_costs()
    
    def _initialize_transform_costs(self) -> Dict[str, float]:
        """Initialiser coûts par type de transformation"""
        costs = {}
        
        # Transformations simples (coût faible)
        simple_transforms = [
            'rotate_90', 'rotate_180', 'rotate_270',
            'flip_horizontal', 'flip_vertical', 'transpose',
            'identity'
        ]
        for t in simple_transforms:
            costs[t] = 1.0
        
        # Transformations moyennes
        medium_transforms = [
            'crop', 'expand', 'translate', 'scale_up', 'scale_down',
            'recolor', 'color_map', 'invert_colors'
        ]
        for t in medium_transforms:
            costs[t] = 2.0
        
        # Transformations complexes (coût élevé)
        complex_transforms = [
            'extract_objects', 'flood_fill', 'detect_symmetry',
            'complete_symmetry', 'repeat_pattern'
        ]
        for t in complex_transforms:
            costs[t] = 3.0
        
        return costs
    
    def compute_cost(self, composed: ComposedTransform) -> ProgramCost:
        """
        Calculer coût complet d'une composition
        
        Returns:
            ProgramCost avec toutes les composantes
        """
        cost = ProgramCost()
        
        # Complexity: nombre de transformations
        cost.complexity = len(composed.transforms)
        
        # Parameter count: total paramètres
        for transform in composed.transforms:
            cost.parameter_count += len(transform.parameters)
        
        # Description length: longueur description
        cost.description_length = len(composed.description)
        
        # Execution cost: somme coûts individuels
        for transform in composed.transforms:
            transform_type = transform.type.value if hasattr(transform.type, 'value') else str(transform.type)
            cost.execution_cost += self.transform_costs.get(transform_type, 2.0)
        
        # Total cost (weighted sum)
        cost.total_cost = (
            self.complexity_weight * cost.complexity +
            self.parameter_weight * cost.parameter_count +
            self.execution_weight * cost.execution_cost
        )
        
        return cost
    
    def compute_penalty(self, cost: ProgramCost) -> float:
        """
        Calculer pénalité de complexité
        
        Returns:
            Pénalité [0, 1] (0 = simple, 1 = très complexe)
        """
        # Normaliser par coût maximum raisonnable
        max_reasonable_cost = 20.0
        penalty = min(cost.total_cost / max_reasonable_cost, 1.0)
        return float(penalty)


class TransformPriors:
    """
    Probabilités a priori des transformations
    
    Apprend quelles transformations sont historiquement utiles
    """
    
    def __init__(self):
        # Compteurs succès/échecs par type
        self.success_counts: Dict[str, int] = defaultdict(int)
        self.failure_counts: Dict[str, int] = defaultdict(int)
        
        # Priors initiaux (uniform)
        self.priors: Dict[str, float] = {}
        self._initialize_priors()
    
    def _initialize_priors(self):
        """Initialiser priors uniformes"""
        transform_types = [
            'rotate_90', 'rotate_180', 'rotate_270',
            'flip_horizontal', 'flip_vertical', 'transpose',
            'crop', 'expand', 'translate', 'scale_up', 'scale_down',
            'recolor', 'color_map', 'invert_colors',
            'extract_objects', 'flood_fill', 'detect_symmetry',
            'complete_symmetry', 'repeat_pattern', 'identity'
        ]
        
        for t in transform_types:
            self.priors[t] = 1.0 / len(transform_types)
    
    def update(self, transform_type: str, success: bool):
        """Mettre à jour prior après observation"""
        if success:
            self.success_counts[transform_type] += 1
        else:
            self.failure_counts[transform_type] += 1
        
        # Recalculer prior (Bayesian update)
        total = self.success_counts[transform_type] + self.failure_counts[transform_type]
        if total > 0:
            # Prior = success_rate avec smoothing
            alpha = 1.0  # Smoothing parameter
            self.priors[transform_type] = (self.success_counts[transform_type] + alpha) / (total + 2 * alpha)
    
    def get_prior(self, transform_type: str) -> float:
        """Obtenir prior d'un type de transformation"""
        return self.priors.get(transform_type, 0.5)
    
    def get_composition_prior(self, composed: ComposedTransform) -> float:
        """Obtenir prior d'une composition (produit des priors)"""
        prior = 1.0
        for transform in composed.transforms:
            transform_type = transform.type.value if hasattr(transform.type, 'value') else str(transform.type)
            prior *= self.get_prior(transform_type)
        return float(prior)


class CanonicalEncoder:
    """
    Encodeur canonique pour détecter duplicatas
    
    Deux hypothèses équivalentes doivent avoir le même hash
    """
    
    def __init__(self):
        self.encodings_count = 0
    
    def encode(self, hypothesis: Hypothesis) -> str:
        """
        Encoder hypothèse en hash canonique
        
        Returns:
            Hash unique pour hypothèses équivalentes
        """
        # Créer signature canonique
        signature_parts = []
        
        for transform in hypothesis.composed_transform.transforms:
            # Type de transformation
            transform_type = transform.type.value if hasattr(transform.type, 'value') else str(transform.type)
            signature_parts.append(transform_type)
            
            # Paramètres triés (ordre canonique)
            if transform.parameters:
                params_str = ",".join([
                    f"{k}={v}" for k, v in sorted(transform.parameters.items())
                ])
                signature_parts.append(params_str)
        
        # Joindre et hasher
        signature = "|".join(signature_parts)
        hash_obj = hashlib.sha256(signature.encode())
        canonical_hash = hash_obj.hexdigest()[:16]  # 16 premiers caractères
        
        self.encodings_count += 1
        return canonical_hash
    
    def are_equivalent(self, hyp1: Hypothesis, hyp2: Hypothesis) -> bool:
        """Vérifier si deux hypothèses sont équivalentes"""
        return self.encode(hyp1) == self.encode(hyp2)


class PruningSystem:
    """
    Système de pruning agressif
    
    Contrôle explosion combinatoire via:
    - Program cost model
    - Heuristic scoring
    - Beam width
    - Early rejection
    - Transform priors
    - Canonical encoding
    """
    
    def __init__(self, beam_width: int = 10,
                 complexity_threshold: float = 0.7,
                 min_score_threshold: float = 0.1,
                 verbose: bool = False):
        self.beam_width = beam_width
        self.complexity_threshold = complexity_threshold
        self.min_score_threshold = min_score_threshold
        self.verbose = verbose
        
        # Composants
        self.cost_model = ProgramCostModel()
        self.priors = TransformPriors()
        self.canonical = CanonicalEncoder()
        
        # Statistiques
        self.hypotheses_evaluated = 0
        self.hypotheses_pruned = 0
        self.duplicates_removed = 0
    
    def score_hypothesis(self, hypothesis: Hypothesis,
                        accuracy: float) -> ScoredHypothesis:
        """
        Scorer hypothèse complètement
        
        Score = accuracy - complexity_penalty + prior_probability
        """
        # Calculer coût
        cost = self.cost_model.compute_cost(hypothesis.composed_transform)
        complexity_penalty = self.cost_model.compute_penalty(cost)
        
        # Calculer prior
        prior_probability = self.priors.get_composition_prior(hypothesis.composed_transform)
        
        # Heuristic score (combinaison)
        heuristic_score = prior_probability * (1.0 - complexity_penalty)
        
        # Total score
        total_score = accuracy - complexity_penalty + 0.1 * prior_probability
        
        # Canonical hash
        canonical_hash = self.canonical.encode(hypothesis)
        
        self.hypotheses_evaluated += 1
        
        return ScoredHypothesis(
            hypothesis=hypothesis,
            accuracy=accuracy,
            complexity_penalty=complexity_penalty,
            execution_cost=cost.execution_cost,
            heuristic_score=heuristic_score,
            prior_probability=prior_probability,
            total_score=total_score,
            canonical_hash=canonical_hash
        )
    
    def prune_beam(self, scored_hypotheses: List[ScoredHypothesis]) -> List[ScoredHypothesis]:
        """
        Pruning via beam search
        
        Garde seulement top-k hypothèses
        """
        # Trier par score décroissant
        sorted_hyps = sorted(scored_hypotheses, key=lambda h: h.total_score, reverse=True)
        
        # Garder top-k
        pruned = sorted_hyps[:self.beam_width]
        
        self.hypotheses_pruned += len(scored_hypotheses) - len(pruned)
        
        return pruned
    
    def early_reject(self, scored_hypothesis: ScoredHypothesis) -> bool:
        """
        Early rejection: rejeter hypothèses évidemment mauvaises
        
        Returns:
            True si doit être rejeté
        """
        # Rejeter si complexité trop élevée
        if scored_hypothesis.complexity_penalty > self.complexity_threshold:
            return True
        
        # Rejeter si score trop faible
        if scored_hypothesis.total_score < self.min_score_threshold:
            return True
        
        # Rejeter si prior trop faible
        if scored_hypothesis.prior_probability < 0.01:
            return True
        
        return False
    
    def remove_duplicates(self, scored_hypotheses: List[ScoredHypothesis]) -> List[ScoredHypothesis]:
        """
        Supprimer hypothèses duplicatas (même hash canonique)
        
        Garde celle avec meilleur score
        """
        # Grouper par hash
        hash_groups: Dict[str, List[ScoredHypothesis]] = defaultdict(list)
        for hyp in scored_hypotheses:
            hash_groups[hyp.canonical_hash].append(hyp)
        
        # Garder meilleure de chaque groupe
        unique = []
        for group in hash_groups.values():
            best = max(group, key=lambda h: h.total_score)
            unique.append(best)
            self.duplicates_removed += len(group) - 1
        
        return unique
    
    def prune_hypotheses(self, hypotheses: List[Hypothesis],
                        accuracies: List[float]) -> List[ScoredHypothesis]:
        """
        Pipeline complet de pruning
        
        1. Score toutes les hypothèses
        2. Early rejection
        3. Remove duplicates
        4. Beam pruning
        
        Returns:
            Liste pruned de ScoredHypothesis
        """
        if len(hypotheses) != len(accuracies):
            raise ValueError("Nombre hypothèses != nombre accuracies")
        
        # 1. Score
        scored = []
        for hyp, acc in zip(hypotheses, accuracies):
            scored_hyp = self.score_hypothesis(hyp, acc)
            scored.append(scored_hyp)
        
        # 2. Early rejection
        scored = [h for h in scored if not self.early_reject(h)]
        
        # 3. Remove duplicates
        scored = self.remove_duplicates(scored)
        
        # 4. Beam pruning
        scored = self.prune_beam(scored)
        
        if self.verbose:
            print(f"Pruning: {len(hypotheses)} → {len(scored)} hypotheses")
        
        return scored
    
    def update_priors(self, transform_type: str, success: bool):
        """Mettre à jour priors après observation"""
        self.priors.update(transform_type, success)
    
    def get_statistics(self) -> Dict[str, Any]:
        """Statistiques complètes"""
        return {
            'hypotheses_evaluated': self.hypotheses_evaluated,
            'hypotheses_pruned': self.hypotheses_pruned,
            'duplicates_removed': self.duplicates_removed,
            'beam_width': self.beam_width,
            'priors': dict(self.priors.priors),
            'canonical_encodings': self.canonical.encodings_count
        }


# Made with Bob - Expert en Complexity Theory, Search Optimization, AGI Systems Engineering