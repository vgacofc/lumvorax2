"""
MAGEN - Transform Composer & Hypothesis Generator
Composition de transformations et génération d'hypothèses symboliques

Architecture:
- Composition séquentielle de transformations
- Génération d'hypothèses candidates
- Scoring et ranking d'hypothèses
- Validation symbolique exacte
"""

import numpy as np
from typing import List, Tuple, Optional, Dict, Any, Set
from dataclasses import dataclass, field
from itertools import combinations, permutations, product
import copy

from .primitive_transforms import Transform, TransformType, PrimitiveTransforms


@dataclass
class ComposedTransform:
    """Composition de transformations primitives"""
    transforms: List[Transform]
    confidence: float = 1.0
    description: str = ""
    
    def __repr__(self):
        steps = " → ".join([t.type.value for t in self.transforms])
        return f"Composed[{steps}] (conf={self.confidence:.2f})"
    
    def __len__(self):
        return len(self.transforms)


@dataclass
class Hypothesis:
    """Hypothèse de transformation input→output"""
    composed_transform: ComposedTransform
    score: float = 0.0
    verified_examples: int = 0
    total_examples: int = 0
    explanation: str = ""
    
    def __repr__(self):
        return f"Hypothesis[{self.composed_transform}] score={self.score:.3f} ({self.verified_examples}/{self.total_examples})"


class TransformComposer:
    """
    Compositeur de transformations symboliques
    
    Fonctionnalités:
    - Composition séquentielle
    - Validation exacte
    - Détection de transformations équivalentes
    """
    
    def __init__(self, primitives: PrimitiveTransforms):
        self.primitives = primitives
        self.composition_count = 0
    
    def compose(self, transforms: List[Transform]) -> ComposedTransform:
        """Composer plusieurs transformations"""
        self.composition_count += 1
        
        # Calculer confiance composée (produit des confiances)
        confidence = 1.0
        for t in transforms:
            confidence *= t.confidence
        
        # Générer description
        steps = [t.type.value for t in transforms]
        description = " → ".join(steps)
        
        return ComposedTransform(
            transforms=transforms,
            confidence=confidence,
            description=description
        )
    
    def apply(self, grid: np.ndarray, composed: ComposedTransform) -> np.ndarray:
        """Appliquer composition à une grille"""
        return self.primitives.compose_transforms(grid, composed.transforms)
    
    def validate(self, input_grid: np.ndarray, output_grid: np.ndarray,
                composed: ComposedTransform) -> bool:
        """Validation EXACTE (symbolique)"""
        result = self.apply(input_grid, composed)
        return self.primitives.verify_exact_match(result, output_grid)
    
    def explain_mismatch(self, input_grid: np.ndarray, output_grid: np.ndarray,
                        composed: ComposedTransform) -> str:
        """Expliquer pourquoi transformation ne marche pas"""
        result = self.apply(input_grid, composed)
        
        if self.primitives.verify_exact_match(result, output_grid):
            return "Match exact"
        
        # Analyser différences
        diff_mask = (result != output_grid)
        diff_count = np.sum(diff_mask)
        total_pixels = result.size
        diff_percent = 100.0 * diff_count / total_pixels
        
        # Différences de taille
        if result.shape != output_grid.shape:
            return f"Shape mismatch: {result.shape} vs {output_grid.shape}"
        
        # Différences de couleurs
        result_colors = set(result.flatten())
        output_colors = set(output_grid.flatten())
        missing_colors = output_colors - result_colors
        extra_colors = result_colors - output_colors
        
        explanation = f"{diff_count}/{total_pixels} pixels differ ({diff_percent:.1f}%)"
        if missing_colors:
            explanation += f", missing colors: {missing_colors}"
        if extra_colors:
            explanation += f", extra colors: {extra_colors}"
        
        return explanation
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'compositions': self.composition_count
        }


class HypothesisGenerator:
    """
    Générateur d'hypothèses de transformations
    
    Stratégies:
    1. Transformations simples (1 primitive)
    2. Compositions courtes (2-3 primitives)
    3. Patterns détectés (symétries, répétitions)
    4. Transformations spatiales + couleurs
    """
    
    def __init__(self, primitives: PrimitiveTransforms, composer: TransformComposer,
                 max_composition_length: int = 3, verbose: bool = False):
        self.primitives = primitives
        self.composer = composer
        self.max_composition_length = max_composition_length
        self.verbose = verbose
        self.hypotheses_generated = 0
    
    def generate_candidates(self, input_grids: List[np.ndarray],
                           output_grids: List[np.ndarray],
                           max_candidates: int = 100) -> List[Hypothesis]:
        """
        Générer hypothèses candidates
        
        Args:
            input_grids: Grilles d'entrée (training examples)
            output_grids: Grilles de sortie attendues
            max_candidates: Nombre max d'hypothèses à générer
        
        Returns:
            Liste d'hypothèses triées par score
        """
        if len(input_grids) != len(output_grids):
            raise ValueError("Nombre d'inputs != nombre d'outputs")
        
        n_examples = len(input_grids)
        hypotheses: List[Hypothesis] = []
        
        # Stratégie 1: Transformations géométriques simples
        geometric_transforms = [
            TransformType.ROTATE_90,
            TransformType.ROTATE_180,
            TransformType.ROTATE_270,
            TransformType.FLIP_HORIZONTAL,
            TransformType.FLIP_VERTICAL,
            TransformType.TRANSPOSE,
        ]
        
        for ttype in geometric_transforms:
            transform = Transform(type=ttype, parameters={})
            composed = self.composer.compose([transform])
            hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
            hypotheses.append(hypothesis)
            self.hypotheses_generated += 1
            
            if len(hypotheses) >= max_candidates:
                break
        
        # Stratégie 2: Détection de symétries
        if len(hypotheses) < max_candidates:
            symmetry_hypotheses = self._generate_symmetry_hypotheses(
                input_grids, output_grids
            )
            hypotheses.extend(symmetry_hypotheses[:max_candidates - len(hypotheses)])
        
        # Stratégie 3: Transformations de couleurs
        if len(hypotheses) < max_candidates:
            color_hypotheses = self._generate_color_hypotheses(
                input_grids, output_grids
            )
            hypotheses.extend(color_hypotheses[:max_candidates - len(hypotheses)])
        
        # Stratégie 4: Compositions courtes (2 transformations)
        if len(hypotheses) < max_candidates and self.max_composition_length >= 2:
            composition_hypotheses = self._generate_composition_hypotheses(
                input_grids, output_grids, length=2
            )
            hypotheses.extend(composition_hypotheses[:max_candidates - len(hypotheses)])
        
        # Stratégie 5: Transformations spatiales
        if len(hypotheses) < max_candidates:
            spatial_hypotheses = self._generate_spatial_hypotheses(
                input_grids, output_grids
            )
            hypotheses.extend(spatial_hypotheses[:max_candidates - len(hypotheses)])
        
        # Trier par score décroissant
        hypotheses.sort(key=lambda h: h.score, reverse=True)
        
        return hypotheses[:max_candidates]
    
    def _evaluate_hypothesis(self, composed: ComposedTransform,
                            input_grids: List[np.ndarray],
                            output_grids: List[np.ndarray]) -> Hypothesis:
        """Évaluer une hypothèse sur les exemples"""
        verified = 0
        total = len(input_grids)
        
        for inp, out in zip(input_grids, output_grids):
            if self.composer.validate(inp, out, composed):
                verified += 1
        
        # Score = proportion d'exemples vérifiés × confiance
        score = (verified / total) * composed.confidence if total > 0 else 0.0
        
        explanation = f"{verified}/{total} examples verified"
        if verified == total:
            explanation += " ✓ PERFECT MATCH"
        
        return Hypothesis(
            composed_transform=composed,
            score=score,
            verified_examples=verified,
            total_examples=total,
            explanation=explanation
        )
    
    def _generate_symmetry_hypotheses(self, input_grids: List[np.ndarray],
                                     output_grids: List[np.ndarray]) -> List[Hypothesis]:
        """Générer hypothèses basées sur symétries"""
        hypotheses = []
        
        # Vérifier si output est symétrie complétée de input
        for axis in ['horizontal', 'vertical', 'both']:
            transform = Transform(
                type=TransformType.COMPLETE_SYMMETRY,
                parameters={'axis': axis}
            )
            composed = self.composer.compose([transform])
            hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
            hypotheses.append(hypothesis)
            self.hypotheses_generated += 1
        
        return hypotheses
    
    def _generate_color_hypotheses(self, input_grids: List[np.ndarray],
                                  output_grids: List[np.ndarray]) -> List[Hypothesis]:
        """Générer hypothèses de transformations de couleurs"""
        hypotheses = []
        
        # Détecter color mapping commun
        if len(input_grids) > 0 and len(output_grids) > 0:
            # Analyser premier exemple pour détecter mapping
            inp = input_grids[0]
            out = output_grids[0]
            
            if inp.shape == out.shape:
                # Détecter mapping couleur par couleur
                color_map = {}
                for color_in in np.unique(inp):
                    # Trouver couleur correspondante dans output
                    mask = (inp == color_in)
                    colors_out = out[mask]
                    if len(colors_out) > 0:
                        # Couleur majoritaire
                        unique, counts = np.unique(colors_out, return_counts=True)
                        color_out = unique[np.argmax(counts)]
                        if color_in != color_out:
                            color_map[int(color_in)] = int(color_out)
                
                if color_map:
                    transform = Transform(
                        type=TransformType.COLOR_MAP,
                        parameters={'mapping': color_map}
                    )
                    composed = self.composer.compose([transform])
                    hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
                    hypotheses.append(hypothesis)
                    self.hypotheses_generated += 1
        
        # Inversion de couleurs
        transform = Transform(type=TransformType.INVERT_COLORS, parameters={})
        composed = self.composer.compose([transform])
        hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
        hypotheses.append(hypothesis)
        self.hypotheses_generated += 1
        
        return hypotheses
    
    def _generate_composition_hypotheses(self, input_grids: List[np.ndarray],
                                        output_grids: List[np.ndarray],
                                        length: int = 2) -> List[Hypothesis]:
        """Générer hypothèses de compositions"""
        hypotheses = []
        
        # Compositions géométriques courantes
        common_compositions = [
            [TransformType.ROTATE_90, TransformType.FLIP_HORIZONTAL],
            [TransformType.FLIP_HORIZONTAL, TransformType.ROTATE_90],
            [TransformType.ROTATE_180, TransformType.FLIP_VERTICAL],
            [TransformType.TRANSPOSE, TransformType.FLIP_HORIZONTAL],
        ]
        
        for composition in common_compositions:
            transforms = [Transform(type=t, parameters={}) for t in composition]
            composed = self.composer.compose(transforms)
            hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
            hypotheses.append(hypothesis)
            self.hypotheses_generated += 1
        
        return hypotheses
    
    def _generate_spatial_hypotheses(self, input_grids: List[np.ndarray],
                                    output_grids: List[np.ndarray]) -> List[Hypothesis]:
        """Générer hypothèses de transformations spatiales"""
        hypotheses = []
        
        if len(input_grids) > 0 and len(output_grids) > 0:
            inp = input_grids[0]
            out = output_grids[0]
            
            # Détection de scaling
            if out.shape[0] == inp.shape[0] * 2 and out.shape[1] == inp.shape[1] * 2:
                transform = Transform(type=TransformType.SCALE_UP, parameters={'factor': 2})
                composed = self.composer.compose([transform])
                hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
                hypotheses.append(hypothesis)
                self.hypotheses_generated += 1
            
            # Détection de répétition - tester plusieurs combinaisons
            for repeat_h in [1, 2, 3]:
                for repeat_v in [1, 2, 3]:
                    if repeat_h == 1 and repeat_v == 1:
                        continue  # Skip identity
                    
                    expected_h = inp.shape[0] * repeat_v
                    expected_w = inp.shape[1] * repeat_h
                    
                    if out.shape[0] == expected_h and out.shape[1] == expected_w:
                        transform = Transform(type=TransformType.REPEAT_PATTERN,
                                            parameters={'repeat_h': repeat_h, 'repeat_v': repeat_v})
                        composed = self.composer.compose([transform])
                        hypothesis = self._evaluate_hypothesis(composed, input_grids, output_grids)
                        hypotheses.append(hypothesis)
                        self.hypotheses_generated += 1
        
        return hypotheses
    
    def score_hypothesis(self, hypothesis: Hypothesis, examples: List[Tuple[np.ndarray, np.ndarray]]) -> float:
        """Scorer une hypothèse sur nouveaux exemples"""
        verified = 0
        total = len(examples)
        
        for inp, out in examples:
            if self.composer.validate(inp, out, hypothesis.composed_transform):
                verified += 1
        
        return (verified / total) if total > 0 else 0.0
    
    def prune_unlikely(self, hypotheses: List[Hypothesis], threshold: float = 0.5) -> List[Hypothesis]:
        """Éliminer hypothèses peu probables"""
        return [h for h in hypotheses if h.score >= threshold]
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'hypotheses_generated': self.hypotheses_generated
        }


# Made with Bob - Expert en Program Synthesis, Hypothesis Generation, Symbolic AI