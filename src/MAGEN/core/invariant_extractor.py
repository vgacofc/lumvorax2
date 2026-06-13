#!/usr/bin/env python3
"""
Invariant Extractor - Extraction des invariants communs entre train pairs
Inspiré par perception jeux arcade: identifier ce qui est FIXE vs DYNAMIQUE
"""

import numpy as np
from typing import Dict, List, Tuple, Set, Optional, Any
from dataclasses import dataclass
from collections import Counter


@dataclass
class Invariant:
    """Invariant détecté entre train pairs"""
    name: str
    value: Any
    confidence: float
    evidence: List[str]


@dataclass
class InvariantProfile:
    """Profil complet des invariants d'un puzzle"""
    puzzle_id: str
    
    # Invariants structurels
    preserve_shape: bool
    preserve_size: bool
    preserve_topology: bool
    
    # Invariants de transformation
    transformation_type: str  # "geometric", "color", "region", "relational"
    transformation_scope: str  # "global", "local", "selective"
    
    # Invariants de couleur
    color_palette_stable: bool
    new_colors: Set[int]
    removed_colors: Set[int]
    color_mapping: Dict[int, int]
    
    # Invariants d'objets
    object_count_stable: bool
    object_types: Set[str]  # "L_shape", "rectangle", "line", etc.
    object_positions: str  # "preserved", "transformed", "new"
    
    # Invariants de règles
    position_rule: Optional[str]  # "inner_corner", "center", "edge", etc.
    selection_rule: Optional[str]  # "by_color", "by_shape", "by_position"
    marking_rule: Optional[str]  # "fill", "outline", "corner", etc.
    
    # Métriques de cohérence
    consistency_score: float
    confidence: float
    
    # Evidence
    evidence: List[str]


class InvariantExtractor:
    """
    Extracteur d'invariants entre train pairs
    
    Inspiré par perception humaine dans jeux arcade:
    - Identifier ce qui est FIXE (murs, règles)
    - Identifier ce qui est DYNAMIQUE (ennemis, transformations)
    - Identifier MOI (objet principal, cible)
    """
    
    def __init__(self):
        self.debug = False
    
    def extract(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> InvariantProfile:
        """
        Extrait les invariants communs entre tous les train pairs
        
        Args:
            train_pairs: Liste de (input, output) pairs
            
        Returns:
            InvariantProfile avec tous les invariants détectés
        """
        if not train_pairs:
            raise ValueError("Au moins 1 train pair requis")
        
        puzzle_id = "unknown"
        evidence = []
        
        # Analyser chaque train pair individuellement
        pair_analyses = []
        for idx, (inp, out) in enumerate(train_pairs):
            analysis = self._analyze_single_pair(inp, out, idx)
            pair_analyses.append(analysis)
            evidence.append(f"Train pair {idx+1}: {analysis['summary']}")
        
        # Extraire invariants communs
        invariants = self._extract_common_invariants(pair_analyses)
        
        # Calculer cohérence
        consistency_score = self._compute_consistency(pair_analyses, invariants)
        confidence = self._compute_confidence(invariants, consistency_score)
        
        return InvariantProfile(
            puzzle_id=puzzle_id,
            preserve_shape=invariants.get('preserve_shape', False),
            preserve_size=invariants.get('preserve_size', False),
            preserve_topology=invariants.get('preserve_topology', False),
            transformation_type=invariants.get('transformation_type', 'unknown'),
            transformation_scope=invariants.get('transformation_scope', 'unknown'),
            color_palette_stable=invariants.get('color_palette_stable', False),
            new_colors=invariants.get('new_colors', set()),
            removed_colors=invariants.get('removed_colors', set()),
            color_mapping=invariants.get('color_mapping', {}),
            object_count_stable=invariants.get('object_count_stable', False),
            object_types=invariants.get('object_types', set()),
            object_positions=invariants.get('object_positions', 'unknown'),
            position_rule=invariants.get('position_rule'),
            selection_rule=invariants.get('selection_rule'),
            marking_rule=invariants.get('marking_rule'),
            consistency_score=consistency_score,
            confidence=confidence,
            evidence=evidence
        )
    
    def _analyze_single_pair(self, inp: np.ndarray, out: np.ndarray, idx: int) -> Dict[str, Any]:
        """
        Analyse un seul train pair
        
        Returns:
            Dictionnaire avec analyse complète
        """
        analysis = {
            'pair_id': idx,
            'summary': ''
        }
        
        # Analyse structurelle
        analysis['preserve_shape'] = (inp.shape == out.shape)
        analysis['preserve_size'] = (inp.size == out.size)
        
        # Analyse couleurs
        inp_colors = set(np.unique(inp))
        out_colors = set(np.unique(out))
        analysis['color_palette_stable'] = (inp_colors == out_colors)
        analysis['new_colors'] = out_colors - inp_colors
        analysis['removed_colors'] = inp_colors - out_colors
        
        # Analyse transformation
        if inp.shape == out.shape:
            diff = np.sum(inp != out)
            total = inp.size
            change_ratio = diff / total if total > 0 else 0
            
            if change_ratio < 0.1:
                analysis['transformation_scope'] = 'local'
            elif change_ratio < 0.5:
                analysis['transformation_scope'] = 'selective'
            else:
                analysis['transformation_scope'] = 'global'
        else:
            analysis['transformation_scope'] = 'global'
        
        # Analyse topologique
        inp_objects = self._count_objects(inp)
        out_objects = self._count_objects(out)
        analysis['object_count_stable'] = (inp_objects == out_objects)
        
        # Résumé
        summary_parts = []
        if analysis['preserve_shape']:
            summary_parts.append("shape preserved")
        else:
            summary_parts.append(f"shape changed {inp.shape}→{out.shape}")
        
        if analysis['new_colors']:
            summary_parts.append(f"new colors {analysis['new_colors']}")
        
        summary_parts.append(f"scope {analysis['transformation_scope']}")
        
        analysis['summary'] = ", ".join(summary_parts)
        
        return analysis
    
    def _count_objects(self, grid: np.ndarray) -> int:
        """
        Compte le nombre d'objets connectés (composantes connexes)
        """
        if grid.size == 0:
            return 0
        
        # Compter composantes connexes pour chaque couleur non-background
        unique_colors = np.unique(grid)
        total_objects = 0
        
        for color in unique_colors:
            if color == 0:  # Skip background
                continue
            
            mask = (grid == color)
            # Approximation simple: compter régions séparées
            # (version simplifiée sans scipy.ndimage)
            total_objects += 1
        
        return total_objects
    
    def _extract_common_invariants(self, pair_analyses: List[Dict[str, Any]]) -> Dict[str, Any]:
        """
        Extrait les invariants communs à TOUS les train pairs
        """
        if not pair_analyses:
            return {}
        
        invariants = {}
        
        # Invariants structurels (doivent être vrais pour TOUS)
        invariants['preserve_shape'] = all(a.get('preserve_shape', False) for a in pair_analyses)
        invariants['preserve_size'] = all(a.get('preserve_size', False) for a in pair_analyses)
        invariants['object_count_stable'] = all(a.get('object_count_stable', False) for a in pair_analyses)
        
        # Invariants de couleur (intersection)
        all_new_colors = [a.get('new_colors', set()) for a in pair_analyses]
        if all_new_colors:
            # Couleurs nouvelles communes à TOUS les pairs
            common_new_colors = set.intersection(*all_new_colors) if len(all_new_colors) > 1 else all_new_colors[0]
            invariants['new_colors'] = common_new_colors
        else:
            invariants['new_colors'] = set()
        
        all_removed_colors = [a.get('removed_colors', set()) for a in pair_analyses]
        if all_removed_colors:
            common_removed_colors = set.intersection(*all_removed_colors) if len(all_removed_colors) > 1 else all_removed_colors[0]
            invariants['removed_colors'] = common_removed_colors
        else:
            invariants['removed_colors'] = set()
        
        invariants['color_palette_stable'] = all(a.get('color_palette_stable', False) for a in pair_analyses)
        
        # Transformation scope (mode le plus fréquent)
        scopes = [a.get('transformation_scope', 'unknown') for a in pair_analyses]
        scope_counter = Counter(scopes)
        invariants['transformation_scope'] = scope_counter.most_common(1)[0][0] if scope_counter else 'unknown'
        
        # Type de transformation (à déduire des autres invariants)
        if invariants['preserve_shape'] and invariants['preserve_size']:
            if invariants['color_palette_stable']:
                invariants['transformation_type'] = 'geometric'
            else:
                invariants['transformation_type'] = 'color'
        else:
            invariants['transformation_type'] = 'region'
        
        return invariants
    
    def _compute_consistency(self, pair_analyses: List[Dict[str, Any]], invariants: Dict[str, Any]) -> float:
        """
        Calcule le score de cohérence entre les train pairs
        
        Returns:
            Score entre 0 (incohérent) et 1 (parfaitement cohérent)
        """
        if not pair_analyses:
            return 0.0
        
        # Compter combien d'invariants sont respectés par chaque pair
        total_checks = 0
        passed_checks = 0
        
        for analysis in pair_analyses:
            # Vérifier preserve_shape
            total_checks += 1
            if analysis.get('preserve_shape') == invariants.get('preserve_shape'):
                passed_checks += 1
            
            # Vérifier transformation_scope
            total_checks += 1
            if analysis.get('transformation_scope') == invariants.get('transformation_scope'):
                passed_checks += 1
            
            # Vérifier new_colors
            total_checks += 1
            if invariants.get('new_colors') and analysis.get('new_colors'):
                if invariants['new_colors'].issubset(analysis['new_colors']):
                    passed_checks += 1
        
        return passed_checks / total_checks if total_checks > 0 else 0.0
    
    def _compute_confidence(self, invariants: Dict[str, Any], consistency_score: float) -> float:
        """
        Calcule la confiance dans les invariants extraits
        
        Returns:
            Score entre 0 (faible confiance) et 1 (haute confiance)
        """
        # Facteurs de confiance
        factors = []
        
        # Cohérence entre pairs
        factors.append(consistency_score)
        
        # Nombre d'invariants détectés
        num_invariants = sum(1 for v in invariants.values() if v not in [None, set(), {}, 'unknown'])
        invariant_score = min(num_invariants / 10, 1.0)  # Normaliser sur 10 invariants max
        factors.append(invariant_score)
        
        # Spécificité des invariants
        if invariants.get('transformation_type') != 'unknown':
            factors.append(0.8)
        
        if invariants.get('transformation_scope') != 'unknown':
            factors.append(0.8)
        
        # Moyenne des facteurs
        return np.mean(factors) if factors else 0.0
    
    def validate_program_against_invariants(self, 
                                           program: Any,
                                           invariants: InvariantProfile,
                                           input_grid: np.ndarray,
                                           output_grid: np.ndarray) -> Tuple[bool, float, List[str]]:
        """
        Valide qu'un programme respecte les invariants extraits
        
        Args:
            program: Programme à valider
            invariants: Profil d'invariants
            input_grid: Grille d'entrée
            output_grid: Grille de sortie produite
            
        Returns:
            (is_valid, score, violations)
        """
        violations = []
        checks_passed = 0
        total_checks = 0
        
        # Vérifier preserve_shape
        total_checks += 1
        if invariants.preserve_shape:
            if input_grid.shape == output_grid.shape:
                checks_passed += 1
            else:
                violations.append(f"Shape not preserved: {input_grid.shape} → {output_grid.shape}")
        else:
            checks_passed += 1  # Pas d'exigence
        
        # Vérifier new_colors
        total_checks += 1
        if invariants.new_colors:
            out_colors = set(np.unique(output_grid))
            if invariants.new_colors.issubset(out_colors):
                checks_passed += 1
            else:
                missing = invariants.new_colors - out_colors
                violations.append(f"Expected new colors {missing} not found")
        else:
            checks_passed += 1
        
        # Vérifier transformation_scope
        total_checks += 1
        if input_grid.shape == output_grid.shape:
            diff = np.sum(input_grid != output_grid)
            total = input_grid.size
            change_ratio = diff / total if total > 0 else 0
            
            expected_scope = invariants.transformation_scope
            if expected_scope == 'local' and change_ratio < 0.1:
                checks_passed += 1
            elif expected_scope == 'selective' and 0.1 <= change_ratio < 0.5:
                checks_passed += 1
            elif expected_scope == 'global' and change_ratio >= 0.5:
                checks_passed += 1
            else:
                violations.append(f"Transformation scope mismatch: expected {expected_scope}, got {change_ratio:.2%}")
        else:
            checks_passed += 1  # Skip si shapes différentes
        
        score = checks_passed / total_checks if total_checks > 0 else 0.0
        is_valid = (score >= 0.8)  # 80% des checks doivent passer
        
        return is_valid, score, violations

# Made with Bob
