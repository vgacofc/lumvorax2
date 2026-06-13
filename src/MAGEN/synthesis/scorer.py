"""
MAGEN Program Scorer - Scoring de programmes par qualité

Évalue la qualité d'un programme selon plusieurs critères:
- Exactitude (correctness)
- Simplicité (MDL - Minimum Description Length)
- Consistance
- Généralité
- Confiance
"""

import numpy as np
from typing import List, Dict, Tuple
from dataclasses import dataclass

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dsl.program import Program


@dataclass
class ProgramScore:
    """Score détaillé d'un programme"""
    total: float
    correctness: float
    simplicity: float
    consistency: float
    generality: float
    confidence: float
    
    def to_dict(self) -> dict:
        return {
            'total': self.total,
            'correctness': self.correctness,
            'simplicity': self.simplicity,
            'consistency': self.consistency,
            'generality': self.generality,
            'confidence': self.confidence
        }


class ProgramScorer:
    """
    Scorer de programmes
    
    Évalue la qualité d'un programme selon plusieurs dimensions et
    combine les scores avec des poids configurables.
    """
    
    def __init__(self,
                 weight_correctness: float = 0.5,
                 weight_simplicity: float = 0.2,
                 weight_consistency: float = 0.15,
                 weight_generality: float = 0.1,
                 weight_confidence: float = 0.05):
        """
        Args:
            weight_*: Poids pour chaque dimension (doivent sommer à 1.0)
        """
        self.weights = {
            'correctness': weight_correctness,
            'simplicity': weight_simplicity,
            'consistency': weight_consistency,
            'generality': weight_generality,
            'confidence': weight_confidence
        }
        
        # Vérifier que les poids somment à 1.0
        total_weight = sum(self.weights.values())
        if abs(total_weight - 1.0) > 0.01:
            raise ValueError(f"Weights must sum to 1.0, got {total_weight}")
    
    def score_program(self, program: Program, train_pairs: List[Dict]) -> float:
        """
        Score un programme sur les train pairs
        
        Args:
            program: Programme à scorer
            train_pairs: Liste de dicts avec 'input' et 'output'
            
        Returns:
            Score total entre 0.0 et 1.0
        """
        scores = self.score_program_detailed(program, train_pairs)
        return scores.total
    
    def score_program_detailed(self, 
                               program: Program, 
                               train_pairs: List[Dict]) -> ProgramScore:
        """
        Score détaillé d'un programme
        
        Returns:
            ProgramScore avec tous les scores individuels
        """
        # 1. Correctness: Exactitude sur train pairs
        correctness = self._score_correctness(program, train_pairs)
        
        # 2. Simplicity: Complexité du programme (MDL)
        simplicity = self._score_simplicity(program)
        
        # 3. Consistency: Cohérence des transformations
        consistency = self._score_consistency(program, train_pairs)
        
        # 4. Generality: Généralité du programme
        generality = self._score_generality(program)
        
        # 5. Confidence: Confiance dans le programme
        confidence = self._score_confidence(program, train_pairs)
        
        # Score total pondéré
        total = (
            correctness * self.weights['correctness'] +
            simplicity * self.weights['simplicity'] +
            consistency * self.weights['consistency'] +
            generality * self.weights['generality'] +
            confidence * self.weights['confidence']
        )
        
        return ProgramScore(
            total=total,
            correctness=correctness,
            simplicity=simplicity,
            consistency=consistency,
            generality=generality,
            confidence=confidence
        )
    
    def _score_correctness(self, program: Program, train_pairs: List[Dict]) -> float:
        """
        Score d'exactitude: proportion de train pairs correctement résolus
        
        Returns:
            Score entre 0.0 (aucun correct) et 1.0 (tous corrects)
        """
        if not train_pairs:
            return 0.0
        
        correct_count = 0
        partial_scores = []
        
        for pair in train_pairs:
            try:
                predicted = program.execute(pair['input'])
                expected = pair['output']
                
                # Vérification exacte
                if np.array_equal(predicted, expected):
                    correct_count += 1
                    partial_scores.append(1.0)
                else:
                    # Score partiel: pixels corrects
                    if predicted.shape == expected.shape:
                        correct_pixels = np.sum(predicted == expected)
                        total_pixels = expected.size
                        partial_score = correct_pixels / total_pixels
                        partial_scores.append(partial_score)
                    else:
                        partial_scores.append(0.0)
            
            except Exception:
                partial_scores.append(0.0)
        
        # Score: moyenne des scores partiels
        return np.mean(partial_scores)
    
    def _score_simplicity(self, program: Program) -> float:
        """
        Score de simplicité basé sur MDL (Minimum Description Length)
        
        Plus le programme est court et simple, meilleur est le score.
        
        Returns:
            Score entre 0.0 (très complexe) et 1.0 (très simple)
        """
        complexity = program.complexity()
        
        # Normalisation: complexité 1-3 = excellent, >20 = mauvais
        if complexity <= 3:
            return 1.0
        elif complexity >= 20:
            return 0.1
        else:
            # Décroissance linéaire
            return 1.0 - (complexity - 3) / 17 * 0.9
    
    def _score_consistency(self, program: Program, train_pairs: List[Dict]) -> float:
        """
        Score de consistance: le programme applique-t-il des transformations
        cohérentes sur tous les train pairs?
        
        Mesure la variance des résultats intermédiaires.
        
        Returns:
            Score entre 0.0 (incohérent) et 1.0 (très cohérent)
        """
        if len(train_pairs) < 2:
            return 1.0  # Pas assez de données pour mesurer consistance
        
        # Analyser les transformations appliquées
        shape_changes = []
        color_changes = []
        
        for pair in train_pairs:
            try:
                input_grid = pair['input']
                output_grid = pair['output']
                predicted = program.execute(input_grid)
                
                # Changement de forme
                shape_ratio = (predicted.shape[0] * predicted.shape[1]) / \
                             (input_grid.shape[0] * input_grid.shape[1])
                shape_changes.append(shape_ratio)
                
                # Changement de couleurs
                colors_in = len(np.unique(input_grid))
                colors_out = len(np.unique(predicted))
                color_ratio = colors_out / max(colors_in, 1)
                color_changes.append(color_ratio)
            
            except Exception:
                continue
        
        if not shape_changes:
            return 0.5  # Neutre si erreurs
        
        # Consistance = faible variance
        shape_variance = np.var(shape_changes) if len(shape_changes) > 1 else 0.0
        color_variance = np.var(color_changes) if len(color_changes) > 1 else 0.0
        
        # Score: inverse de la variance (normalisé)
        shape_score = 1.0 / (1.0 + shape_variance)
        color_score = 1.0 / (1.0 + color_variance)
        
        return (shape_score + color_score) / 2
    
    def _score_generality(self, program: Program) -> float:
        """
        Score de généralité: le programme utilise-t-il des opérations
        génériques plutôt que des valeurs hardcodées?
        
        Returns:
            Score entre 0.0 (très spécifique) et 1.0 (très général)
        """
        if program.length() == 0:
            return 0.0
        
        # Compter opérations génériques vs spécifiques
        generic_count = 0
        specific_count = 0
        
        generic_names = {
            'rotate90', 'rotate180', 'rotate270',
            'mirror_horizontal', 'mirror_vertical',
            'transpose'
        }
        
        specific_names = {
            'recolor', 'floodfill', 'crop'
        }
        
        for op in program.operations:
            if op.name in generic_names:
                generic_count += 1
            elif op.name in specific_names:
                specific_count += 1
                # Pénaliser si beaucoup de paramètres hardcodés
                if len(op.args) > 2:
                    specific_count += 0.5
        
        total_ops = generic_count + specific_count
        if total_ops == 0:
            return 0.5
        
        return generic_count / total_ops
    
    def _score_confidence(self, program: Program, train_pairs: List[Dict]) -> float:
        """
        Score de confiance: à quel point sommes-nous confiants que ce
        programme généralisera bien?
        
        Basé sur:
        - Nombre de train pairs réussis
        - Marge de succès (pixels corrects)
        - Stabilité d'exécution
        
        Returns:
            Score entre 0.0 (faible confiance) et 1.0 (haute confiance)
        """
        if not train_pairs:
            return 0.0
        
        success_count = 0
        margins = []
        
        for pair in train_pairs:
            try:
                predicted = program.execute(pair['input'])
                expected = pair['output']
                
                if np.array_equal(predicted, expected):
                    success_count += 1
                    margins.append(1.0)
                elif predicted.shape == expected.shape:
                    # Marge: proportion de pixels corrects
                    correct = np.sum(predicted == expected)
                    total = expected.size
                    margins.append(correct / total)
                else:
                    margins.append(0.0)
            
            except Exception:
                margins.append(0.0)
        
        # Confiance basée sur:
        # 1. Taux de succès
        success_rate = success_count / len(train_pairs)
        
        # 2. Marge moyenne (même pour échecs)
        avg_margin = np.mean(margins) if margins else 0.0
        
        # 3. Consistance des marges (faible variance = haute confiance)
        margin_variance = np.var(margins) if len(margins) > 1 else 0.0
        consistency_score = 1.0 / (1.0 + margin_variance)
        
        # Combinaison
        confidence = (
            success_rate * 0.5 +
            avg_margin * 0.3 +
            consistency_score * 0.2
        )
        
        return confidence
    
    def compare_programs(self, 
                        prog1: Program, 
                        prog2: Program,
                        train_pairs: List[Dict]) -> Dict:
        """
        Compare deux programmes
        
        Returns:
            Dict avec comparaison détaillée
        """
        score1 = self.score_program_detailed(prog1, train_pairs)
        score2 = self.score_program_detailed(prog2, train_pairs)
        
        return {
            'program1': {
                'score': score1.total,
                'details': score1.to_dict(),
                'complexity': prog1.complexity()
            },
            'program2': {
                'score': score2.total,
                'details': score2.to_dict(),
                'complexity': prog2.complexity()
            },
            'winner': 'program1' if score1.total > score2.total else 'program2',
            'score_difference': abs(score1.total - score2.total)
        }


__all__ = ['ProgramScorer', 'ProgramScore']

# Made with Bob
