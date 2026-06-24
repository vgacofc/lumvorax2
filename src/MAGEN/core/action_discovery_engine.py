"""
Action Discovery Engine (PHASE -1) - Découverte Intelligente d'Actions
========================================================================

Analyse les train examples pour découvrir quelles transformations sont
PLAUSIBLES avant même l'exploration. Réduit l'espace d'actions de 100%→10%.

Architecture:
1. Analyse train examples (input→output patterns)
2. Détecte transformations géométriques (rotation, flip, etc.)
3. Détecte transformations spatiales (crop, scale, tile)
4. Détecte transformations couleur (map, invert, filter)
5. Score de plausibilité par transformation
6. Retourne top-K actions plausibles

Date: 2026-06-15
Version: V38.2
"""

import numpy as np
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
import logging

from .transformations import ARCTransformations, TransformationResult


@dataclass
class DiscoveredAction:
    """Action découverte avec score de plausibilité."""
    transform_name: str
    plausibility: float
    evidence: Dict
    parameters: Dict


class ActionDiscoveryEngine:
    """
    Moteur de découverte d'actions plausibles.
    
    Analyse les train examples pour identifier quelles transformations
    sont susceptibles de résoudre le puzzle.
    """
    
    def __init__(self, 
                 min_plausibility: float = 0.3,
                 max_actions: int = 10,
                 logger: Optional[logging.Logger] = None):
        """
        Args:
            min_plausibility: Score minimum pour considérer une action
            max_actions: Nombre maximum d'actions à retourner
            logger: Logger optionnel
        """
        self.min_plausibility = min_plausibility
        self.max_actions = max_actions
        self.logger = logger or logging.getLogger(__name__)
        
        # Statistiques
        self.stats = {
            'total_discoveries': 0,
            'geometric_found': 0,
            'spatial_found': 0,
            'color_found': 0,
            'filtered_out': 0
        }
    
    def discover_actions(self, train_examples: List[Dict]) -> List[DiscoveredAction]:
        """
        Découvre les actions plausibles à partir des train examples.
        
        Args:
            train_examples: Liste de {'input': grid, 'output': grid}
        
        Returns:
            Liste d'actions découvertes triées par plausibilité
        """
        if not train_examples:
            self.logger.warning("Aucun train example fourni")
            return []
        
        discovered = []
        
        # Analyser chaque type de transformation
        discovered.extend(self._discover_geometric_transforms(train_examples))
        discovered.extend(self._discover_spatial_transforms(train_examples))
        discovered.extend(self._discover_color_transforms(train_examples))
        
        # Filtrer par plausibilité
        discovered = [a for a in discovered if a.plausibility >= self.min_plausibility]
        
        # Trier par plausibilité décroissante
        discovered.sort(key=lambda a: a.plausibility, reverse=True)
        
        # Limiter au top-K
        discovered = discovered[:self.max_actions]
        
        # Mettre à jour stats
        self.stats['total_discoveries'] = len(discovered)
        
        self.logger.info(f"Découvert {len(discovered)} actions plausibles")
        
        return discovered
    
    def _discover_geometric_transforms(self, train_examples: List[Dict]) -> List[DiscoveredAction]:
        """Découvre les transformations géométriques plausibles."""
        actions = []
        
        # Transformations à tester
        transforms = [
            'rotate_90', 'rotate_180', 'rotate_270',
            'flip_horizontal', 'flip_vertical', 'transpose'
        ]
        
        for transform_name in transforms:
            matches = 0
            total = len(train_examples)
            
            for example in train_examples:
                input_grid = example['input']
                output_grid = example['output']
                
                # Appliquer transformation
                result = self._apply_transform(input_grid, transform_name)
                
                if result.success:
                    # Vérifier si ça matche l'output
                    if self._grids_match(result.grid, output_grid):
                        matches += 1
            
            if matches > 0:
                plausibility = matches / total
                actions.append(DiscoveredAction(
                    transform_name=transform_name,
                    plausibility=plausibility,
                    evidence={'matches': matches, 'total': total},
                    parameters={}
                ))
                self.stats['geometric_found'] += 1
        
        return actions
    
    def _discover_spatial_transforms(self, train_examples: List[Dict]) -> List[DiscoveredAction]:
        """Découvre les transformations spatiales plausibles."""
        actions = []
        
        # Analyser les changements de taille
        for example in train_examples:
            input_grid = example['input']
            output_grid = example['output']
            
            in_h, in_w = input_grid.shape
            out_h, out_w = output_grid.shape
            
            # Crop détecté ?
            if out_h < in_h or out_w < in_w:
                result = ARCTransformations.crop_nonzero(input_grid)
                if result.success and self._grids_match(result.grid, output_grid):
                    actions.append(DiscoveredAction(
                        transform_name='crop_nonzero',
                        plausibility=0.8,
                        evidence={'size_reduction': True},
                        parameters={}
                    ))
                    self.stats['spatial_found'] += 1
                    break
            
            # Scale up détecté ?
            if out_h > in_h and out_w > in_w:
                factor_h = out_h // in_h
                factor_w = out_w // in_w
                
                if factor_h == factor_w and factor_h > 1:
                    result = ARCTransformations.scale_up(input_grid, factor=factor_h)
                    if result.success and self._grids_match(result.grid, output_grid):
                        actions.append(DiscoveredAction(
                            transform_name='scale_up',
                            plausibility=0.8,
                            evidence={'scale_factor': factor_h},
                            parameters={'factor': factor_h}
                        ))
                        self.stats['spatial_found'] += 1
                        break
            
            # Tile détecté ?
            if out_h % in_h == 0 and out_w % in_w == 0:
                rows = out_h // in_h
                cols = out_w // in_w
                
                if rows > 1 or cols > 1:
                    result = ARCTransformations.tile(input_grid, rows=rows, cols=cols)
                    if result.success and self._grids_match(result.grid, output_grid):
                        actions.append(DiscoveredAction(
                            transform_name='tile',
                            plausibility=0.8,
                            evidence={'rows': rows, 'cols': cols},
                            parameters={'rows': rows, 'cols': cols}
                        ))
                        self.stats['spatial_found'] += 1
                        break
        
        return actions
    
    def _discover_color_transforms(self, train_examples: List[Dict]) -> List[DiscoveredAction]:
        """Découvre les transformations couleur plausibles."""
        actions = []
        
        for example in train_examples:
            input_grid = example['input']
            output_grid = example['output']
            
            # Même taille ? Peut-être transformation couleur
            if input_grid.shape != output_grid.shape:
                continue
            
            # Inversion couleur ?
            result = ARCTransformations.invert_colors(input_grid)
            if result.success and self._grids_match(result.grid, output_grid):
                actions.append(DiscoveredAction(
                    transform_name='invert_colors',
                    plausibility=0.9,
                    evidence={'perfect_match': True},
                    parameters={}
                ))
                self.stats['color_found'] += 1
                break
            
            # Mapping couleur ?
            color_map = self._detect_color_mapping(input_grid, output_grid)
            if color_map:
                result = ARCTransformations.map_colors(input_grid, color_map)
                if result.success and self._grids_match(result.grid, output_grid):
                    actions.append(DiscoveredAction(
                        transform_name='map_colors',
                        plausibility=0.9,
                        evidence={'color_map': color_map},
                        parameters={'color_map': color_map}
                    ))
                    self.stats['color_found'] += 1
                    break
        
        return actions
    
    def _apply_transform(self, grid: np.ndarray, transform_name: str, **kwargs) -> TransformationResult:
        """Applique une transformation."""
        transforms = {
            'rotate_90': ARCTransformations.rotate_90,
            'rotate_180': ARCTransformations.rotate_180,
            'rotate_270': ARCTransformations.rotate_270,
            'flip_horizontal': ARCTransformations.flip_horizontal,
            'flip_vertical': ARCTransformations.flip_vertical,
            'transpose': ARCTransformations.transpose,
        }
        
        if transform_name in transforms:
            return transforms[transform_name](grid)
        
        return TransformationResult(
            grid=grid,
            transform_name=transform_name,
            success=False,
            metadata={'error': 'Unknown transform'}
        )
    
    def _grids_match(self, grid1: np.ndarray, grid2: np.ndarray, tolerance: float = 0.0) -> bool:
        """Vérifie si deux grilles matchent."""
        if grid1.shape != grid2.shape:
            return False
        
        if tolerance == 0.0:
            return bool(np.array_equal(grid1, grid2))
        else:
            diff = np.abs(grid1 - grid2)
            return bool(np.mean(diff) <= tolerance)
    
    def _detect_color_mapping(self, input_grid: np.ndarray, output_grid: np.ndarray) -> Optional[Dict[int, int]]:
        """Détecte un mapping couleur entre input et output."""
        if input_grid.shape != output_grid.shape:
            return None
        
        color_map = {}
        
        for i in range(input_grid.shape[0]):
            for j in range(input_grid.shape[1]):
                in_color = int(input_grid[i, j])
                out_color = int(output_grid[i, j])
                
                if in_color in color_map:
                    # Vérifier cohérence
                    if color_map[in_color] != out_color:
                        return None  # Mapping incohérent
                else:
                    color_map[in_color] = out_color
        
        # Vérifier que c'est un vrai mapping (pas identity)
        if all(k == v for k, v in color_map.items()):
            return None
        
        return color_map
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return self.stats.copy()
    
    def reset_stats(self):
        """Réinitialise les statistiques."""
        self.stats = {
            'total_discoveries': 0,
            'geometric_found': 0,
            'spatial_found': 0,
            'color_found': 0,
            'filtered_out': 0
        }

# Made with Bob
