#!/usr/bin/env python3
"""
MAGEN - Global Error Handler
=============================

SESSION 56 - CORRECTION CRITIQUE #1
Gestion globale des erreurs IndexError et validation bounds

Objectif: Récupérer 16 puzzles perdus à cause d'erreurs non gérées

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
Protocole: Claude Pilot + LumVorax
"""

import numpy as np
from typing import Any, Callable, Optional, Tuple
from functools import wraps
import traceback


class GlobalErrorHandler:
    """
    Gestionnaire global d'erreurs pour MAGEN
    
    Capture et gère:
    - IndexError (bounds checking)
    - ValueError (invalid operations)
    - TypeError (type mismatches)
    - RuntimeError (execution failures)
    
    Permet au système de continuer même en cas d'erreur
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.errors_caught = []
        self.errors_by_type = {}
    
    def safe_execute(
        self,
        func: Callable,
        *args,
        fallback_value: Any = None,
        error_context: str = "",
        **kwargs
    ) -> Tuple[bool, Any]:
        """
        Exécute fonction de manière sécurisée
        
        Args:
            func: Fonction à exécuter
            *args: Arguments positionnels
            fallback_value: Valeur retournée en cas d'erreur
            error_context: Contexte pour logging
            **kwargs: Arguments nommés
        
        Returns:
            (success, result) où:
            - success: True si exécution réussie, False sinon
            - result: Résultat de la fonction ou fallback_value
        """
        try:
            result = func(*args, **kwargs)
            return True, result
        
        except IndexError as e:
            error_info = {
                'type': 'IndexError',
                'message': str(e),
                'context': error_context,
                'traceback': traceback.format_exc()
            }
            self.errors_caught.append(error_info)
            self.errors_by_type['IndexError'] = self.errors_by_type.get('IndexError', 0) + 1
            
            if self.verbose:
                print(f"⚠️  IndexError capturée: {e}")
                print(f"   Contexte: {error_context}")
            
            return False, fallback_value
        
        except ValueError as e:
            error_info = {
                'type': 'ValueError',
                'message': str(e),
                'context': error_context,
                'traceback': traceback.format_exc()
            }
            self.errors_caught.append(error_info)
            self.errors_by_type['ValueError'] = self.errors_by_type.get('ValueError', 0) + 1
            
            if self.verbose:
                print(f"⚠️  ValueError capturée: {e}")
                print(f"   Contexte: {error_context}")
            
            return False, fallback_value
        
        except TypeError as e:
            error_info = {
                'type': 'TypeError',
                'message': str(e),
                'context': error_context,
                'traceback': traceback.format_exc()
            }
            self.errors_caught.append(error_info)
            self.errors_by_type['TypeError'] = self.errors_by_type.get('TypeError', 0) + 1
            
            if self.verbose:
                print(f"⚠️  TypeError capturée: {e}")
                print(f"   Contexte: {error_context}")
            
            return False, fallback_value
        
        except Exception as e:
            error_info = {
                'type': type(e).__name__,
                'message': str(e),
                'context': error_context,
                'traceback': traceback.format_exc()
            }
            self.errors_caught.append(error_info)
            self.errors_by_type['Other'] = self.errors_by_type.get('Other', 0) + 1
            
            if self.verbose:
                print(f"⚠️  Erreur inattendue: {type(e).__name__}: {e}")
                print(f"   Contexte: {error_context}")
            
            return False, fallback_value
    
    def get_error_summary(self) -> dict:
        """Résumé des erreurs capturées"""
        return {
            'total_errors': len(self.errors_caught),
            'by_type': self.errors_by_type,
            'recent_errors': self.errors_caught[-10:] if self.errors_caught else []
        }
    
    def reset(self):
        """Réinitialiser compteurs"""
        self.errors_caught = []
        self.errors_by_type = {}


def safe_grid_operation(fallback_value: Any = None):
    """
    Décorateur pour opérations sur grilles
    
    Usage:
        @safe_grid_operation(fallback_value=np.zeros((3,3)))
        def my_grid_function(grid):
            return grid[10, 10]  # IndexError capturée
    """
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            handler = GlobalErrorHandler(verbose=False)
            success, result = handler.safe_execute(
                func,
                *args,
                fallback_value=fallback_value,
                error_context=f"{func.__name__}",
                **kwargs
            )
            return result
        return wrapper
    return decorator


def validate_grid_access(
    grid: np.ndarray,
    row: int,
    col: int,
    operation: str = "read"
) -> bool:
    """
    Valide accès grille avant opération
    
    Args:
        grid: Grille numpy
        row: Ligne à accéder
        col: Colonne à accéder
        operation: Type d'opération ("read" ou "write")
    
    Returns:
        True si accès valide, False sinon
    """
    if grid is None:
        return False
    
    if not isinstance(grid, np.ndarray):
        return False
    
    if len(grid.shape) != 2:
        return False
    
    height, width = grid.shape
    
    if row < 0 or row >= height:
        return False
    
    if col < 0 or col >= width:
        return False
    
    return True


def safe_grid_slice(
    grid: np.ndarray,
    row_start: int,
    row_end: int,
    col_start: int,
    col_end: int,
    fallback: Optional[np.ndarray] = None
) -> np.ndarray:
    """
    Extraction sécurisée de sous-grille
    
    Args:
        grid: Grille source
        row_start, row_end: Limites lignes
        col_start, col_end: Limites colonnes
        fallback: Valeur retournée si erreur
    
    Returns:
        Sous-grille ou fallback
    """
    if grid is None or not isinstance(grid, np.ndarray):
        return fallback if fallback is not None else np.array([[]])
    
    height, width = grid.shape
    
    # Clamp indices
    row_start = max(0, min(row_start, height))
    row_end = max(0, min(row_end, height))
    col_start = max(0, min(col_start, width))
    col_end = max(0, min(col_end, width))
    
    # Vérifier validité
    if row_start >= row_end or col_start >= col_end:
        return fallback if fallback is not None else np.array([[]])
    
    try:
        return grid[row_start:row_end, col_start:col_end].copy()
    except Exception:
        return fallback if fallback is not None else np.array([[]])


# Instance globale
_global_handler = GlobalErrorHandler(verbose=True)


def get_global_handler() -> GlobalErrorHandler:
    """Récupérer instance globale du handler"""
    return _global_handler

# Made with Bob
