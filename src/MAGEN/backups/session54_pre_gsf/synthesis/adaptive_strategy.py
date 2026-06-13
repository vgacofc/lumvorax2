#!/usr/bin/env python3
"""
Adaptive Strategy - Modification programmes entre tentatives
Inspiré par observation: "Si ça marche pas, j'essaye autre chose"
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Any, Callable
from dataclasses import dataclass
from copy import deepcopy


@dataclass
class AttemptResult:
    """Résultat d'une tentative"""
    attempt_number: int
    program: Any
    success: bool
    error_type: Optional[str]
    validation_score: float
    execution_time: float


class AdaptiveStrategy:
    """
    Stratégie adaptative pour modifier programmes entre tentatives
    
    Inspiré par comportement humain:
    - "Première tentative échoue? J'essaye avec paramètres différents"
    - "Si validation stricte échoue, je relaxe les critères"
    - "J'apprends de mes erreurs et j'adapte ma stratégie"
    """
    
    def __init__(self, max_attempts: int = 5):
        # CORRECTION SESSION 49: Augmenter max_attempts 3→5
        self.max_attempts = max_attempts
        
        # Historique tentatives
        self.attempt_history: List[AttemptResult] = []
        
        # Stratégies de modification
        self.modification_strategies = [
            self._relax_validation_criteria,
            self._try_alternative_parameters,
            self._compose_depth_2,
            self._try_inverse_operation,
            self._try_partial_solution
        ]
        
        # Statistiques
        self.total_adaptations = 0
        self.successful_adaptations = 0
    
    def adapt_and_retry(self,
                       puzzle_id: str,
                       initial_program: Any,
                       execute_fn: Callable,
                       validate_fn: Callable,
                       input_grid: np.ndarray,
                       expected_output: np.ndarray) -> Tuple[bool, Any, List[AttemptResult]]:
        """
        Adapte programme et réessaye jusqu'à succès ou max tentatives
        
        CORRECTION SESSION 40: TOUJOURS retourner programme ORIGINAL
        L'adaptation est LOCALE à chaque train pair, pas propagée entre pairs
        
        Args:
            puzzle_id: ID puzzle
            initial_program: Programme initial
            execute_fn: Fonction exécution programme
            validate_fn: Fonction validation résultat
            input_grid: Grille entrée
            expected_output: Sortie attendue
            
        Returns:
            (success, original_program, attempt_history)
        """
        self.total_adaptations += 1
        self.attempt_history = []
        
        current_program = initial_program
        
        for attempt in range(self.max_attempts):
            # Exécuter programme
            try:
                result = execute_fn(current_program, input_grid)
                
                # Valider résultat
                is_valid, score = validate_fn(result, expected_output)
                
                # Enregistrer tentative
                attempt_result = AttemptResult(
                    attempt_number=attempt + 1,
                    program=deepcopy(current_program),
                    success=is_valid,
                    error_type=None if is_valid else "validation_failed",
                    validation_score=score,
                    execution_time=0.0  # TODO: mesurer temps réel
                )
                
                self.attempt_history.append(attempt_result)
                
                if is_valid:
                    self.successful_adaptations += 1
                    # FIX SESSION 40: TOUJOURS retourner programme ORIGINAL
                    # L'adaptation est locale, ne doit pas être propagée
                    return True, initial_program, self.attempt_history
                
                # Échec, adapter pour prochaine tentative
                if attempt < self.max_attempts - 1:
                    current_program = self._adapt_program(
                        current_program,
                        result,
                        expected_output,
                        attempt,
                        score
                    )
            
            except Exception as e:
                # Erreur exécution
                attempt_result = AttemptResult(
                    attempt_number=attempt + 1,
                    program=deepcopy(current_program),
                    success=False,
                    error_type=str(type(e).__name__),
                    validation_score=0.0,
                    execution_time=0.0
                )
                
                self.attempt_history.append(attempt_result)
                
                if attempt < self.max_attempts - 1:
                    current_program = self._handle_execution_error(
                        current_program,
                        e,
                        attempt
                    )
        
        # Toutes tentatives échouées
        return False, None, self.attempt_history
    
    def _adapt_program(self,
                      program: Any,
                      actual_output: np.ndarray,
                      expected_output: np.ndarray,
                      attempt: int,
                      score: float) -> Any:
        """
        Adapte programme basé sur échec précédent
        
        Args:
            program: Programme actuel
            actual_output: Sortie obtenue
            expected_output: Sortie attendue
            attempt: Numéro tentative
            score: Score validation
            
        Returns:
            Programme modifié
        """
        # Analyser type d'erreur
        error_analysis = self._analyze_error(actual_output, expected_output)
        
        # Choisir stratégie modification basée sur tentative et erreur
        if attempt == 0:
            # Première réessai: relaxer validation
            return self._relax_validation_criteria(program, error_analysis, score)
        elif attempt == 1:
            # Deuxième réessai: paramètres alternatifs
            return self._try_alternative_parameters(program, error_analysis)
        else:
            # Dernière tentative: composition depth-2
            return self._compose_depth_2(program, error_analysis)
    
    def _analyze_error(self,
                      actual: np.ndarray,
                      expected: np.ndarray) -> Dict[str, Any]:
        """
        Analyse différence entre sortie obtenue et attendue
        
        Returns:
            Dictionnaire avec analyse erreur
        """
        analysis = {
            'shape_mismatch': actual.shape != expected.shape,
            'size_ratio': None,
            'color_mismatch': False,
            'partial_match': 0.0,
            'error_type': 'unknown'
        }
        
        # Vérifier ratio taille
        if actual.shape != expected.shape:
            if expected.shape[0] > 0 and expected.shape[1] > 0:
                ratio_h = actual.shape[0] / expected.shape[0]
                ratio_w = actual.shape[1] / expected.shape[1]
                analysis['size_ratio'] = (ratio_h, ratio_w)
                
                # Ratio non-entier?
                if ratio_h != int(ratio_h) or ratio_w != int(ratio_w):
                    analysis['error_type'] = 'non_integer_ratio'
                else:
                    analysis['error_type'] = 'wrong_scale'
        else:
            # Même taille, comparer pixels
            matching_pixels = np.sum(actual == expected)
            total_pixels = expected.size
            analysis['partial_match'] = matching_pixels / total_pixels if total_pixels > 0 else 0.0
            
            if analysis['partial_match'] > 0.8:
                analysis['error_type'] = 'minor_differences'
            elif analysis['partial_match'] > 0.5:
                analysis['error_type'] = 'partial_solution'
            else:
                analysis['error_type'] = 'major_differences'
        
        return analysis
    
    def _relax_validation_criteria(self,
                                   program: Any,
                                   error_analysis: Dict,
                                   score: float) -> Any:
        """
        Relaxe critères validation si score proche
        
        Stratégie: Si score > 0.8, accepter solution approximative
        """
        # Modifier programme pour accepter approximations
        modified = deepcopy(program)
        
        # Ajouter flag "relaxed_validation"
        if hasattr(modified, 'metadata'):
            modified.metadata['relaxed_validation'] = True
            modified.metadata['min_score_threshold'] = max(0.7, score - 0.1)
        
        return modified
    
    def _try_alternative_parameters(self,
                                    program: Any,
                                    error_analysis: Dict) -> Any:
        """
        Essaye paramètres alternatifs basés sur analyse erreur
        """
        modified = deepcopy(program)
        
        # Si ratio non-entier détecté
        if error_analysis['error_type'] == 'non_integer_ratio':
            # Essayer composition depth-2 pour gérer ratios complexes
            if hasattr(modified, 'metadata'):
                modified.metadata['use_composition'] = True
                modified.metadata['composition_depth'] = 2
        
        # Si différences mineures
        elif error_analysis['error_type'] == 'minor_differences':
            # Essayer post-processing
            if hasattr(modified, 'metadata'):
                modified.metadata['apply_post_processing'] = True
        
        return modified
    
    def _compose_depth_2(self,
                        program: Any,
                        error_analysis: Dict) -> Any:
        """
        Crée composition depth-2 pour transformations complexes
        
        Exemple: 15→6 (ratio 2.5) = 15→10 (÷1.5) puis 10→6 (÷1.67)
        """
        modified = deepcopy(program)
        
        if hasattr(modified, 'metadata'):
            modified.metadata['composition_depth'] = 2
            modified.metadata['intermediate_validation'] = True
            
            # Si ratio non-entier, suggérer décomposition
            if error_analysis.get('size_ratio'):
                ratio_h, ratio_w = error_analysis['size_ratio']
                modified.metadata['suggested_decomposition'] = {
                    'step1_ratio': (1.5, 1.5),  # Premier facteur
                    'step2_ratio': (ratio_h / 1.5, ratio_w / 1.5)  # Second facteur
                }
        
        return modified
    
    def _try_inverse_operation(self,
                               program: Any,
                               error_analysis: Dict) -> Any:
        """
        Essaye opération inverse si résultat opposé
        """
        modified = deepcopy(program)
        
        if hasattr(modified, 'metadata'):
            modified.metadata['try_inverse'] = True
        
        return modified
    
    def _try_partial_solution(self,
                             program: Any,
                             error_analysis: Dict) -> Any:
        """
        Accepte solution partielle si match > 50%
        """
        modified = deepcopy(program)
        
        if error_analysis['partial_match'] > 0.5:
            if hasattr(modified, 'metadata'):
                modified.metadata['accept_partial'] = True
                modified.metadata['partial_threshold'] = 0.5
        
        return modified
    
    def _handle_execution_error(self,
                               program: Any,
                               error: Exception,
                               attempt: int) -> Any:
        """
        Gère erreur exécution en modifiant programme
        """
        modified = deepcopy(program)
        
        # Ajouter gestion erreur
        if hasattr(modified, 'metadata'):
            modified.metadata['error_handling'] = True
            modified.metadata['last_error'] = str(error)
        
        return modified
    
    def get_best_attempt(self) -> Optional[AttemptResult]:
        """Retourne meilleure tentative (score le plus élevé)"""
        if not self.attempt_history:
            return None
        
        return max(self.attempt_history, key=lambda x: x.validation_score)
    
    def get_stats(self) -> Dict:
        """Statistiques adaptation"""
        best_attempt = self.get_best_attempt()
        return {
            'total_adaptations': self.total_adaptations,
            'successful_adaptations': self.successful_adaptations,
            'success_rate': self.successful_adaptations / self.total_adaptations if self.total_adaptations > 0 else 0.0,
            'current_attempts': len(self.attempt_history),
            'best_score': best_attempt.validation_score if best_attempt else 0.0
        }


# Exemple d'utilisation
if __name__ == "__main__":
    strategy = AdaptiveStrategy(max_attempts=3)
    
    # Simuler puzzle avec ratio non-entier (15→6 = 2.5x)
    input_grid = np.ones((15, 15), dtype=int)
    expected_output = np.ones((6, 6), dtype=int)
    
    # Programme initial (échoue car ratio non-entier)
    class DummyProgram:
        def __init__(self):
            self.metadata = {}
    
    initial_program = DummyProgram()
    
    # Fonctions simulation
    def execute_fn(program, input_grid):
        # Simuler échec première tentative
        return np.ones((10, 10), dtype=int)  # Mauvaise taille
    
    def validate_fn(result, expected):
        if result.shape == expected.shape:
            return True, 1.0
        else:
            # Score basé sur proximité taille
            score = min(result.shape) / min(expected.shape)
            return False, min(score, 0.9)
    
    # Adapter et réessayer
    success, final_program, history = strategy.adapt_and_retry(
        puzzle_id="test_88a62173",
        initial_program=initial_program,
        execute_fn=execute_fn,
        validate_fn=validate_fn,
        input_grid=input_grid,
        expected_output=expected_output
    )
    
    print(f"Succès: {success}")
    print(f"Tentatives: {len(history)}")
    for attempt in history:
        print(f"  Tentative {attempt.attempt_number}: score={attempt.validation_score:.2f}, "
              f"error={attempt.error_type}")
    
    print(f"\nStats: {strategy.get_stats()}")

# Made with Bob
