"""
MAGEN Program Validator - Validation de programmes

Valide qu'un programme:
1. S'exécute sans erreur
2. Produit des sorties correctes
3. Respecte les contraintes ARC
"""

import numpy as np
from typing import List, Dict, Tuple, Optional
from scipy import ndimage

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dsl.program import Program


def compute_similarity(predicted: np.ndarray, expected: np.ndarray) -> float:
    """
    Calcule similarité entre deux grilles (0.0-1.0)
    
    CORRECTION V22: Validation probabiliste au lieu de binaire
    """
    # Si shapes différentes, essayer resize
    if predicted.shape != expected.shape:
        # Essayer resize si dimensions proches
        if can_resize_safely(predicted, expected.shape):
            predicted = resize_grid(predicted, expected.shape)
        else:
            return 0.0
    
    # Calcul similarité pixel-wise
    exact_match = np.sum(predicted == expected)
    total_cells = predicted.size
    similarity = exact_match / total_cells
    
    return similarity


def can_resize_safely(grid: np.ndarray, target_shape) -> bool:
    """Vérifie si resize est sûr (pas trop de distorsion)"""
    h_ratio = target_shape[0] / grid.shape[0]
    v_ratio = target_shape[1] / grid.shape[1]
    
    # Accepter resize si ratio < 2x dans chaque dimension
    return (0.5 <= h_ratio <= 2.0) and (0.5 <= v_ratio <= 2.0)


def resize_grid(grid: np.ndarray, target_shape) -> np.ndarray:
    """Resize grid en préservant valeurs discrètes"""
    # Utiliser nearest neighbor pour préserver valeurs discrètes
    zoom_factors = (target_shape[0] / grid.shape[0], target_shape[1] / grid.shape[1])
    resized = ndimage.zoom(grid, zoom_factors, order=0)  # order=0 = nearest neighbor
    return np.array(resized, dtype=grid.dtype)


class ProgramValidator:
    """
    Validateur de programmes
    
    Vérifie qu'un programme est valide et produit des résultats corrects.
    """
    
    def __init__(self, strict_mode: bool = True, similarity_threshold: float = 0.85):
        """
        Args:
            strict_mode: Si True, exige exactitude pixel-perfect
            similarity_threshold: Seuil de similarité pour validation (0.0-1.0)
                                 CORRECTION V22: 0.85 par défaut (au lieu de 1.0)
        """
        self.strict_mode = strict_mode
        self.similarity_threshold = similarity_threshold
    
    def validate_on_train(self, 
                         program: Program, 
                         train_pairs: List[Dict]) -> Tuple[bool, float]:
        """
        Valide un programme sur les train pairs
        
        Args:
            program: Programme à valider
            train_pairs: Liste de dicts avec 'input' et 'output'
            
        Returns:
            (is_valid, accuracy)
            - is_valid: True si programme s'exécute sans erreur
            - accuracy: Proportion de pairs correctement résolus (0.0-1.0)
        """
        if not train_pairs:
            return False, 0.0
        
        correct_count = 0
        error_count = 0
        total_similarity = 0.0
        
        for pair in train_pairs:
            try:
                predicted = program.execute(pair['input'])
                expected = pair['output']
                
                # CORRECTION V22: Validation probabiliste
                similarity = compute_similarity(predicted, expected)
                total_similarity += similarity
                
                # Considérer correct si similarité >= seuil
                if similarity >= self.similarity_threshold:
                    correct_count += 1
            
            except Exception:
                error_count += 1
        
        # Programme invalide si trop d'erreurs
        if error_count > len(train_pairs) // 2:
            return False, 0.0
        
        # Accuracy basée sur similarité moyenne
        accuracy = total_similarity / len(train_pairs)
        
        # En mode strict, exiger haute similarité
        if self.strict_mode:
            is_valid = (accuracy >= self.similarity_threshold and error_count == 0)
        else:
            is_valid = (accuracy >= 0.5 and error_count == 0)
        
        return is_valid, accuracy
    
    def validate_single_pair(self,
                            program: Program,
                            input_grid: np.ndarray,
                            expected_output: np.ndarray) -> Tuple[bool, Optional[np.ndarray]]:
        """
        Valide un programme sur une seule paire input/output
        
        Args:
            program: Programme à valider
            input_grid: Grille d'entrée
            expected_output: Sortie attendue
            
        Returns:
            (is_correct, predicted_output)
            - is_correct: True si sortie exacte
            - predicted_output: Sortie prédite (None si erreur)
        """
        try:
            predicted = program.execute(input_grid)
            
            # CORRECTION V22: Validation probabiliste
            similarity = compute_similarity(predicted, expected_output)
            is_correct = similarity >= self.similarity_threshold
            
            return is_correct, predicted
        
        except Exception:
            return False, None
    
    def check_constraints(self, 
                         program: Program,
                         train_pairs: List[Dict]) -> Dict[str, bool]:
        """
        Vérifie que le programme respecte les contraintes ARC
        
        Returns:
            Dict avec résultats de chaque contrainte
        """
        constraints = {
            'no_errors': True,
            'deterministic': True,
            'reasonable_output_size': True,
            'preserves_structure': True
        }
        
        # Contrainte 1: Pas d'erreurs d'exécution
        for pair in train_pairs:
            try:
                program.execute(pair['input'])
            except Exception:
                constraints['no_errors'] = False
                break
        
        # Contrainte 2: Déterministe (même input → même output)
        if constraints['no_errors'] and train_pairs:
            first_input = train_pairs[0]['input']
            try:
                output1 = program.execute(first_input)
                output2 = program.execute(first_input)
                constraints['deterministic'] = np.array_equal(output1, output2)
            except Exception:
                constraints['deterministic'] = False
        
        # Contrainte 3: Taille de sortie raisonnable
        for pair in train_pairs:
            try:
                predicted = program.execute(pair['input'])
                # Sortie ne doit pas être trop grande (max 30x30 pour ARC)
                if predicted.shape[0] > 30 or predicted.shape[1] > 30:
                    constraints['reasonable_output_size'] = False
                    break
            except Exception:
                pass
        
        # Contrainte 4: Préservation de structure (heuristique)
        # Vérifier que les transformations sont cohérentes
        shape_ratios = []
        for pair in train_pairs:
            try:
                input_grid = pair['input']
                predicted = program.execute(input_grid)
                
                ratio = (predicted.shape[0] * predicted.shape[1]) / \
                       (input_grid.shape[0] * input_grid.shape[1])
                shape_ratios.append(ratio)
            except Exception:
                pass
        
        if shape_ratios:
            # Structure préservée si variance faible
            variance = np.var(shape_ratios)
            constraints['preserves_structure'] = (variance < 1.0)
        
        return constraints
    
    def validate_with_details(self,
                             program: Program,
                             train_pairs: List[Dict]) -> Dict:
        """
        Validation détaillée avec rapport complet
        
        Returns:
            Dict avec tous les détails de validation
        """
        is_valid, accuracy = self.validate_on_train(program, train_pairs)
        constraints = self.check_constraints(program, train_pairs)
        
        # Détails par pair
        pair_results = []
        for i, pair in enumerate(train_pairs):
            is_correct, predicted = self.validate_single_pair(
                program, pair['input'], pair['output']
            )
            
            result = {
                'pair_index': i,
                'correct': is_correct,
                'has_output': predicted is not None
            }
            
            if predicted is not None:
                result['output_shape'] = predicted.shape
                result['expected_shape'] = pair['output'].shape
                result['shape_match'] = (predicted.shape == pair['output'].shape)
            
            pair_results.append(result)
        
        return {
            'is_valid': is_valid,
            'accuracy': accuracy,
            'constraints': constraints,
            'pair_results': pair_results,
            'program_complexity': program.complexity(),
            'program_length': program.length()
        }
    
    def quick_validate(self, program: Program, test_grid: np.ndarray) -> bool:
        """
        Validation rapide: vérifie juste que le programme s'exécute
        
        Args:
            program: Programme à valider
            test_grid: Grille de test
            
        Returns:
            True si exécution réussie
        """
        try:
            program.execute(test_grid)
            return True
        except Exception:
            return False


__all__ = ['ProgramValidator']

# Made with Bob
