"""
Safe Operations - Validation Bounds et Shapes
==============================================

Module pour opérations sécurisées sur grilles numpy avec:
- Validation bounds automatique
- Gestion erreurs index out of bounds
- Validation shapes avant transformations
- Logging forensique des accès invalides

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot + LumVorax
"""

import numpy as np
from typing import Tuple, Optional, Any
import logging

# Logger forensique
logger = logging.getLogger(__name__)


class SafeGridOperations:
    """Opérations sécurisées sur grilles avec validation bounds"""
    
    def __init__(self, enable_logging: bool = True):
        """
        Initialiser opérations sécurisées
        
        Args:
            enable_logging: Activer logging forensique
        """
        self.enable_logging = enable_logging
        self.access_violations = []
        self.write_violations = []
    
    def is_valid_position(self, grid_shape: Tuple[int, int], row: int, col: int) -> bool:
        """
        Vérifier si position est valide dans grille
        
        Args:
            grid_shape: Forme de la grille (height, width)
            row: Ligne à vérifier
            col: Colonne à vérifier
            
        Returns:
            True si position valide, False sinon
        """
        return 0 <= row < grid_shape[0] and 0 <= col < grid_shape[1]
    
    def safe_access(self, grid: np.ndarray, row: int, col: int, 
                   default: Any = 0) -> Any:
        """
        Accès sécurisé à une cellule avec validation bounds
        
        Args:
            grid: Grille numpy
            row: Index ligne
            col: Index colonne
            default: Valeur par défaut si hors limites
        
        Returns:
            Valeur cellule ou default si hors limites
        """
        # Validation bounds
        if 0 <= row < grid.shape[0] and 0 <= col < grid.shape[1]:
            return grid[row, col]
        
        # Hors limites - logger violation
        if self.enable_logging:
            violation = {
                'type': 'access',
                'row': row,
                'col': col,
                'grid_shape': grid.shape,
                'default_returned': default
            }
            self.access_violations.append(violation)
            logger.debug(f"Access violation: ({row},{col}) out of bounds for shape {grid.shape}")
        
        return default
    
    def safe_write(self, grid: np.ndarray, row: int, col: int, 
                   value: Any) -> bool:
        """
        Écriture sécurisée dans cellule avec validation bounds
        
        Args:
            grid: Grille numpy (modifiée in-place)
            row: Index ligne
            col: Index colonne
            value: Valeur à écrire
        
        Returns:
            True si écriture réussie, False si hors limites
        """
        # Validation bounds
        if 0 <= row < grid.shape[0] and 0 <= col < grid.shape[1]:
            grid[row, col] = value
            return True
        
        # Hors limites - logger violation
        if self.enable_logging:
            violation = {
                'type': 'write',
                'row': row,
                'col': col,
                'value': value,
                'grid_shape': grid.shape
            }
            self.write_violations.append(violation)
            logger.debug(f"Write violation: ({row},{col}) out of bounds for shape {grid.shape}")
        
        return False
    
    def safe_slice(self, grid: np.ndarray, 
                   row_start: int, row_end: int,
                   col_start: int, col_end: int,
                   default: Any = 0) -> np.ndarray:
        """
        Slice sécurisé avec validation bounds
        
        Args:
            grid: Grille numpy
            row_start: Début ligne (inclusif)
            row_end: Fin ligne (exclusif)
            col_start: Début colonne (inclusif)
            col_end: Fin colonne (exclusif)
            default: Valeur par défaut pour zones hors limites
        
        Returns:
            Sous-grille avec padding si nécessaire
        """
        # Clamp indices aux limites
        row_start_clamped = max(0, row_start)
        row_end_clamped = min(grid.shape[0], row_end)
        col_start_clamped = max(0, col_start)
        col_end_clamped = min(grid.shape[1], col_end)
        
        # Extraire slice valide
        valid_slice = grid[row_start_clamped:row_end_clamped, 
                          col_start_clamped:col_end_clamped]
        
        # Si slice demandé dépasse limites, créer avec padding
        if (row_start < 0 or row_end > grid.shape[0] or 
            col_start < 0 or col_end > grid.shape[1]):
            
            # Créer grille avec padding
            result_shape = (row_end - row_start, col_end - col_start)
            result = np.full(result_shape, default, dtype=grid.dtype)
            
            # Copier slice valide dans résultat
            result_row_start = max(0, -row_start)
            result_col_start = max(0, -col_start)
            result[result_row_start:result_row_start + valid_slice.shape[0],
                   result_col_start:result_col_start + valid_slice.shape[1]] = valid_slice
            
            return result
        
        return valid_slice
    
    def validate_shape_compatibility(self, grid1: np.ndarray, 
                                    grid2: np.ndarray) -> bool:
        """
        Valider compatibilité shapes pour opérations
        
        Args:
            grid1: Première grille
            grid2: Deuxième grille
        
        Returns:
            True si shapes compatibles
        """
        return grid1.shape == grid2.shape
    
    def resize_to_match(self, source: np.ndarray, 
                       target_shape: Tuple[int, int],
                       method: str = 'pad') -> np.ndarray:
        """
        Redimensionner grille pour matcher shape cible
        
        Args:
            source: Grille source
            target_shape: Shape cible (rows, cols)
            method: Méthode resize ('pad', 'crop', 'scale')
        
        Returns:
            Grille redimensionnée
        """
        if source.shape == target_shape:
            return source.copy()
        
        if method == 'pad':
            # Padding avec 0
            result = np.zeros(target_shape, dtype=source.dtype)
            rows = min(source.shape[0], target_shape[0])
            cols = min(source.shape[1], target_shape[1])
            result[:rows, :cols] = source[:rows, :cols]
            return result
        
        elif method == 'crop':
            # Crop au centre
            rows = min(source.shape[0], target_shape[0])
            cols = min(source.shape[1], target_shape[1])
            row_start = (source.shape[0] - rows) // 2
            col_start = (source.shape[1] - cols) // 2
            return source[row_start:row_start+rows, col_start:col_start+cols]
        
        elif method == 'scale':
            # Scale avec répétition (nearest neighbor)
            from scipy.ndimage import zoom
            zoom_factors = (target_shape[0] / source.shape[0],
                          target_shape[1] / source.shape[1])
            result = zoom(source, zoom_factors, order=0)
            return np.asarray(result, dtype=source.dtype)
        
        else:
            raise ValueError(f"Unknown resize method: {method}")
    
    def get_violations_report(self) -> dict:
        """
        Obtenir rapport violations forensique
        
        Returns:
            Dict avec statistiques violations
        """
        return {
            'access_violations': len(self.access_violations),
            'write_violations': len(self.write_violations),
            'total_violations': len(self.access_violations) + len(self.write_violations),
            'access_details': self.access_violations,
            'write_details': self.write_violations
        }
    
    def reset_violations(self):
        """Réinitialiser compteurs violations"""
        self.access_violations = []
        self.write_violations = []


# Instance globale pour usage simple
_global_safe_ops = SafeGridOperations(enable_logging=True)


def safe_access(grid: np.ndarray, row: int, col: int, default: Any = 0) -> Any:
    """
    Fonction utilitaire: accès sécurisé global
    
    Args:
        grid: Grille numpy
        row: Index ligne
        col: Index colonne
        default: Valeur par défaut si hors limites
    
    Returns:
        Valeur cellule ou default
    """
    return _global_safe_ops.safe_access(grid, row, col, default)


def safe_write(grid: np.ndarray, row: int, col: int, value: Any) -> bool:
    """
    Fonction utilitaire: écriture sécurisée globale
    
    Args:
        grid: Grille numpy (modifiée in-place)
        row: Index ligne
        col: Index colonne
        value: Valeur à écrire
    
    Returns:
        True si succès, False si hors limites
    """
    return _global_safe_ops.safe_write(grid, row, col, value)


def safe_slice(grid: np.ndarray, 
               row_start: int, row_end: int,
               col_start: int, col_end: int,
               default: Any = 0) -> np.ndarray:
    """
    Fonction utilitaire: slice sécurisé global
    
    Args:
        grid: Grille numpy
        row_start: Début ligne
        row_end: Fin ligne
        col_start: Début colonne
        col_end: Fin colonne
        default: Valeur padding
    
    Returns:
        Sous-grille
    """
    return _global_safe_ops.safe_slice(grid, row_start, row_end, 
                                       col_start, col_end, default)


def validate_shape_compatibility(grid1: np.ndarray, grid2: np.ndarray) -> bool:
    """
    Fonction utilitaire: validation shapes globale
    
    Args:
        grid1: Première grille
        grid2: Deuxième grille
    
    Returns:
        True si compatibles
    """
    return _global_safe_ops.validate_shape_compatibility(grid1, grid2)


def resize_to_match(source: np.ndarray, target_shape: Tuple[int, int],
                   method: str = 'pad') -> np.ndarray:
    """
    Fonction utilitaire: resize global
    
    Args:
        source: Grille source
        target_shape: Shape cible
        method: Méthode resize
    
    Returns:
        Grille redimensionnée
    """
    return _global_safe_ops.resize_to_match(source, target_shape, method)


def get_violations_report() -> dict:
    """
    Fonction utilitaire: rapport violations global
    
    Returns:
        Dict statistiques
    """
    return _global_safe_ops.get_violations_report()


def reset_violations():
    """Fonction utilitaire: reset violations global"""
    _global_safe_ops.reset_violations()


# Made with Bob