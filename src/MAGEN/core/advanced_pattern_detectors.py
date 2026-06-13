#!/usr/bin/env python3
"""
MAGEN - Advanced Pattern Detectors
===================================

SESSION 56-57 - IMPLÉMENTATION 6 PATTERNS HIGH PRIORITY
Basé sur analyse forensique 355 échecs

SESSION 57 - MOTEUR TRANSFORMATIONNEL RÉEL INTÉGRÉ
Insight utilisateur: "ARC attend OBJET ENTIER pas PIXEL"

Patterns implémentés:
1. ObjectCountingDetector (64.2% échecs) - MOTEUR RÉEL ✅
2. ConditionalRulesDetector (36.1% échecs)
3. SpatialRelationshipsDetector (34.1% échecs)
4. ColorGradientDetector (33.8% échecs)
5. GridTilingDetector (32.7% échecs)
6. DiagonalSymmetryDetector (26.2% échecs)

Protocole: CLAUDE_PILOT + LUMVORAX
"""

import numpy as np
from typing import List, Tuple, Dict, Optional, Callable
from dataclasses import dataclass
from scipy.ndimage import label
from collections import Counter

# Import Object Transformation Engine RÉEL
from .object_transformation_engine import (
    ObjectExtractor,
    ObjectDuplicator,
    StructuredObject
)

# Forensic logging
FORENSIC_LOGGER = None  # Sera injecté par pipeline


@dataclass
class PatternDetectionResult:
    """Résultat détection pattern"""
    pattern_name: str
    confidence: float  # 0.0-1.0
    transformation: Optional[Callable]
    parameters: Dict
    description: str


class ObjectCountingDetector:
    """
    Détecteur comptage objets (64.2% échecs)
    
    Détecte règles basées sur nombre d'objets:
    - Ajout/suppression objets (+N, -N)
    - Multiplication/division (*N, /N)
    - Règles conditionnelles sur count
    """
    
    def __init__(self):
        self.name = "object_counting"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte pattern comptage objets"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            try:
                FORENSIC_LOGGER.log_event(
                    event_type="pattern_detection",
                    component="advanced_pattern_detectors",
                    operation="detect",
                    data={
                        "detector": self.name,
                        "num_pairs": len(train_pairs)
                    }
                )
            except Exception as e:
                pass  # Erreur silencieuse
        
        if not train_pairs:
            return None
        
        # Analyser changements nombre objets
        count_changes = []
        for inp, out in train_pairs:
            count_in = self._count_objects(inp)
            count_out = self._count_objects(out)
            count_changes.append((count_in, count_out, count_out - count_in))
        
        # Détecter règle
        rule = self._detect_counting_rule(count_changes)
        
        if rule:
            confidence = self._compute_confidence(count_changes, rule)
            
            if confidence >= self.min_confidence:
                return PatternDetectionResult(
                    pattern_name=self.name,
                    confidence=confidence,
                    transformation=self._create_transformation(rule),
                    parameters=rule,
                    description=f"Object counting rule: {rule['type']}"
                )
        
        return None
    
    def _count_objects(self, grid: np.ndarray) -> int:
        """Compte objets connectés dans grille"""
        if grid.size == 0:
            return 0
        
        # Masque binaire (non-zéro = objet)
        binary = (grid > 0).astype(int)
        
        # Connected components
        labeled, num_objects = label(binary)
        
        return num_objects
    
    def _detect_counting_rule(self, count_changes: List[Tuple[int, int, int]]) -> Optional[Dict]:
        """Détecte règle de comptage"""
        if not count_changes:
            return None
        
        # Extraire deltas
        deltas = [delta for _, _, delta in count_changes]
        
        # Règle constante (+N ou -N)
        if len(set(deltas)) == 1:
            delta = deltas[0]
            if delta > 0:
                return {'type': 'add', 'value': delta}
            elif delta < 0:
                return {'type': 'remove', 'value': abs(delta)}
            else:
                return {'type': 'preserve', 'value': 0}
        
        # Règle multiplicative (*N ou /N)
        ratios = []
        for count_in, count_out, _ in count_changes:
            if count_in > 0:
                ratio = count_out / count_in
                ratios.append(ratio)
        
        if ratios and len(set(ratios)) == 1:
            ratio = ratios[0]
            if ratio > 1:
                return {'type': 'multiply', 'value': ratio}
            elif ratio < 1 and ratio > 0:
                return {'type': 'divide', 'value': 1/ratio}
        
        # Règle conditionnelle (if count > threshold)
        # TODO: Implémenter détection conditionnelle avancée
        
        return None
    
    def _compute_confidence(self, count_changes: List[Tuple], rule: Dict) -> float:
        """Calcule confiance règle détectée"""
        if not count_changes or not rule:
            return 0.0
        
        correct = 0
        for count_in, count_out, _ in count_changes:
            predicted = self._apply_rule(count_in, rule)
            if predicted == count_out:
                correct += 1
        
        return correct / len(count_changes)
    
    def _apply_rule(self, count_in: int, rule: Dict) -> int:
        """Applique règle comptage"""
        rule_type = rule['type']
        value = rule['value']
        
        if rule_type == 'add':
            return count_in + value
        elif rule_type == 'remove':
            return max(0, count_in - value)
        elif rule_type == 'multiply':
            return int(count_in * value)
        elif rule_type == 'divide':
            return int(count_in / value)
        elif rule_type == 'preserve':
            return count_in
        
        return count_in
    
    def _create_transformation(self, rule: Dict) -> Callable:
        """
        Crée transformation STRUCTURELLE basée sur règle comptage
        
        SESSION 57 - MOTEUR TRANSFORMATIONNEL RÉEL
        Amélioration critique vs Session 56:
        - Extraction objets COMPLETS (pas pixels isolés)
        - Duplication STRUCTURELLE (préserve forme)
        - Placement INTELLIGENT (symétrie, alignement)
        """
        rule_type = rule['type']
        value = rule['value']
        
        # Initialiser moteur transformationnel
        extractor = ObjectExtractor(verbose=False)
        duplicator = ObjectDuplicator(verbose=False)
        
        def transform(grid: np.ndarray) -> np.ndarray:
            """Applique transformation comptage objets STRUCTURELLE"""
            if grid.size == 0:
                return grid.copy()
            
            # ÉTAPE 1: Extraire objets structurés COMPLETS
            objects = extractor.extract_objects(grid)
            current_count = len(objects)
            
            if current_count == 0:
                return grid.copy()
            
            # ÉTAPE 2: Calculer target count
            if rule_type == 'add':
                target_count = current_count + int(value)
            elif rule_type == 'remove':
                target_count = max(0, current_count - int(value))
            elif rule_type == 'multiply':
                target_count = int(current_count * value)
            elif rule_type == 'divide':
                target_count = int(current_count / value) if value > 0 else current_count
            elif rule_type == 'preserve':
                return grid.copy()
            else:
                return grid.copy()
            
            # ÉTAPE 3: Appliquer transformation STRUCTURELLE
            if target_count < current_count:
                # Supprimer objets (garder les N premiers)
                result = grid.copy()
                binary = (grid > 0).astype(int)
                labeled, _ = label(binary)
                
                for obj_id in range(target_count + 1, current_count + 1):
                    result[labeled == obj_id] = 0
                
                return result
            
            elif target_count > current_count:
                # DUPLICATION STRUCTURELLE avec placement intelligent
                result = duplicator.duplicate_objects(
                    grid,
                    objects,
                    target_count
                )
                return result
            
            else:
                # Nombre correct
                return grid.copy()
        
        return transform


class ConditionalRulesDetector:
    """
    Détecteur règles conditionnelles (36.1% échecs)
    
    Détecte règles if-then:
    - if color == X then action Y
    - if position in region then transform
    - if size > threshold then modify
    """
    
    def __init__(self):
        self.name = "conditional_rules"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte règles conditionnelles"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={
                    "detector": self.name,
                    "num_pairs": len(train_pairs)
                }
            )
        
        if not train_pairs:
            return None
        
        # Analyser changements couleurs
        color_rules = self._detect_color_rules(train_pairs)
        
        if color_rules:
            confidence = color_rules['confidence']
            
            if confidence >= self.min_confidence:
                return PatternDetectionResult(
                    pattern_name=self.name,
                    confidence=confidence,
                    transformation=self._create_color_transformation(color_rules),
                    parameters=color_rules,
                    description=f"Conditional color rule: {color_rules['type']}"
                )
        
        return None
    
    def _detect_color_rules(self, train_pairs: List[Tuple]) -> Optional[Dict]:
        """Détecte règles basées sur couleurs"""
        # Analyser mappings couleurs
        color_mappings = []
        
        for inp, out in train_pairs:
            mapping = self._extract_color_mapping(inp, out)
            if mapping:
                color_mappings.append(mapping)
        
        # Vérifier cohérence
        if not color_mappings:
            return None
        
        # Règle simple: mapping couleur constant
        if len(color_mappings) >= 2:
            # Vérifier si tous les mappings sont identiques
            first_mapping = color_mappings[0]
            if all(m == first_mapping for m in color_mappings):
                return {
                    'type': 'color_mapping',
                    'mapping': first_mapping,
                    'confidence': 1.0
                }
        
        return None
    
    def _extract_color_mapping(self, inp: np.ndarray, out: np.ndarray) -> Optional[Dict]:
        """Extrait mapping couleurs input→output"""
        if inp.shape != out.shape:
            return None
        
        mapping = {}
        for i in range(inp.shape[0]):
            for j in range(inp.shape[1]):
                color_in = int(inp[i, j])
                color_out = int(out[i, j])
                
                if color_in in mapping:
                    if mapping[color_in] != color_out:
                        # Mapping inconsistant
                        return None
                else:
                    mapping[color_in] = color_out
        
        return mapping
    
    def _create_color_transformation(self, rules: Dict) -> Callable:
        """Crée transformation couleur"""
        mapping = rules['mapping']
        
        def transform(grid: np.ndarray) -> np.ndarray:
            result = grid.copy()
            for color_in, color_out in mapping.items():
                result[grid == color_in] = color_out
            return result
        
        return transform


class SpatialRelationshipsDetector:
    """
    Détecteur relations spatiales (34.1% échecs)
    
    Détecte:
    - Alignement spatial
    - Positionnement relatif
    - Distance-based rules
    - Adjacency rules
    """
    
    def __init__(self):
        self.name = "spatial_relationships"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte relations spatiales"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={
                    "detector": self.name,
                    "num_pairs": len(train_pairs)
                }
            )
        
        if not train_pairs:
            return None
        
        # Détecter changement taille
        size_changes = []
        for inp, out in train_pairs:
            size_changes.append((inp.shape, out.shape))
        
        # Règle resize constante
        if len(set(size_changes)) == 1:
            inp_shape, out_shape = size_changes[0]
            
            if inp_shape != out_shape:
                return PatternDetectionResult(
                    pattern_name=self.name,
                    confidence=1.0,
                    transformation=self._create_resize_transformation(inp_shape, out_shape),
                    parameters={'type': 'resize', 'from': inp_shape, 'to': out_shape},
                    description=f"Spatial resize: {inp_shape} → {out_shape}"
                )
        
        return None
    
    def _create_resize_transformation(self, from_shape: Tuple, to_shape: Tuple) -> Callable:
        """Crée transformation resize"""
        def transform(grid: np.ndarray) -> np.ndarray:
            # Extraction ou padding selon cas
            if to_shape[0] < from_shape[0] or to_shape[1] < from_shape[1]:
                # Extraction (crop)
                return grid[:to_shape[0], :to_shape[1]]
            else:
                # Padding
                result = np.zeros(to_shape, dtype=grid.dtype)
                result[:grid.shape[0], :grid.shape[1]] = grid
                return result
        
        return transform


class ColorGradientDetector:
    """
    Détecteur gradients couleur (33.8% échecs)
    
    Détecte:
    - Gradients horizontaux/verticaux
    - Interpolation couleurs
    - Transitions progressives
    """
    
    def __init__(self):
        self.name = "color_gradient"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte gradients couleur"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={
                    "detector": self.name,
                    "num_pairs": len(train_pairs)
                }
            )
        
        if not train_pairs:
            return None
        
        # Analyser gradients
        gradients = []
        for inp, out in train_pairs:
            gradient = self._detect_gradient(out)
            if gradient:
                gradients.append(gradient)
        
        # Vérifier cohérence
        if gradients and len(gradients) == len(train_pairs):
            # Tous ont gradient
            gradient_type = gradients[0]['type']
            if all(g['type'] == gradient_type for g in gradients):
                return PatternDetectionResult(
                    pattern_name=self.name,
                    confidence=0.8,
                    transformation=self._create_gradient_transformation(gradient_type),
                    parameters={'type': gradient_type},
                    description=f"Color gradient: {gradient_type}"
                )
        
        return None
    
    def _detect_gradient(self, grid: np.ndarray) -> Optional[Dict]:
        """Détecte gradient dans grille"""
        h, w = grid.shape
        
        # Gradient horizontal
        for i in range(h):
            row = grid[i, :]
            if self._is_gradient(row):
                return {'type': 'horizontal', 'direction': 'increasing'}
        
        # Gradient vertical
        for j in range(w):
            col = grid[:, j]
            if self._is_gradient(col):
                return {'type': 'vertical', 'direction': 'increasing'}
        
        return None
    
    def _is_gradient(self, sequence: np.ndarray) -> bool:
        """Vérifie si séquence est gradient"""
        if len(sequence) < 3:
            return False
        
        unique_values = len(np.unique(sequence))
        if unique_values < len(sequence) // 2:
            return False
        
        # Vérifier monotonie
        diffs = np.diff(sequence)
        return np.all(diffs >= 0) or np.all(diffs <= 0)
    
    def _create_gradient_transformation(self, gradient_type: str) -> Callable:
        """Crée transformation gradient couleur"""
        def transform(grid: np.ndarray) -> np.ndarray:
            """Applique gradient couleur"""
            if grid.size == 0:
                return grid.copy()
            
            result = grid.copy()
            h, w = result.shape
            
            # Trouver couleurs non-nulles
            non_zero_mask = result > 0
            if not np.any(non_zero_mask):
                return result
            
            base_colors = np.unique(result[non_zero_mask])
            if len(base_colors) == 0:
                return result
            
            base_color = int(base_colors[0])
            
            if gradient_type == 'horizontal':
                # Gradient horizontal (gauche→droite)
                for i in range(h):
                    for j in range(w):
                        if result[i, j] > 0:
                            # Appliquer gradient basé sur position X
                            gradient_value = int(base_color + (j / max(1, w-1)) * 5)
                            result[i, j] = min(9, max(1, gradient_value))
            
            elif gradient_type == 'vertical':
                # Gradient vertical (haut→bas)
                for i in range(h):
                    for j in range(w):
                        if result[i, j] > 0:
                            # Appliquer gradient basé sur position Y
                            gradient_value = int(base_color + (i / max(1, h-1)) * 5)
                            result[i, j] = min(9, max(1, gradient_value))
            
            return result
        
        return transform


class GridTilingDetector:
    """
    Détecteur tiling grille (32.7% échecs)
    
    Détecte:
    - Répétition motifs
    - Tiling 2x2, 3x3, etc.
    - Pattern replication
    """
    
    def __init__(self):
        self.name = "grid_tiling"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte tiling grille"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={
                    "detector": self.name,
                    "num_pairs": len(train_pairs)
                }
            )
        
        if not train_pairs:
            return None
        
        # Analyser tiling dans outputs
        tilings = []
        for inp, out in train_pairs:
            tiling = self._detect_tiling(out)
            if tiling:
                tilings.append(tiling)
        
        # Vérifier cohérence
        if tilings and len(tilings) == len(train_pairs):
            tile_size = tilings[0]['tile_size']
            if all(t['tile_size'] == tile_size for t in tilings):
                return PatternDetectionResult(
                    pattern_name=self.name,
                    confidence=0.9,
                    transformation=self._create_tiling_transformation(tile_size),
                    parameters={'tile_size': tile_size},
                    description=f"Grid tiling: {tile_size}x{tile_size}"
                )
        
        return None
    
    def _detect_tiling(self, grid: np.ndarray) -> Optional[Dict]:
        """Détecte tiling dans grille"""
        h, w = grid.shape
        
        # Tester tailles tuiles communes
        for tile_h in [1, 2, 3, 4]:
            for tile_w in [1, 2, 3, 4]:
                if h % tile_h == 0 and w % tile_w == 0:
                    # Extraire première tuile
                    tile = grid[:tile_h, :tile_w]
                    
                    # Vérifier si répétée partout
                    is_tiled = True
                    for i in range(0, h, tile_h):
                        for j in range(0, w, tile_w):
                            if not np.array_equal(grid[i:i+tile_h, j:j+tile_w], tile):
                                is_tiled = False
                                break
                        if not is_tiled:
                            break
                    
                    if is_tiled and (tile_h > 1 or tile_w > 1):
                        return {'tile_size': (tile_h, tile_w), 'tile': tile}
        
        return None
    
    def _create_tiling_transformation(self, tile_size: Tuple[int, int]) -> Callable:
        """Crée transformation tiling (répétition motif)"""
        tile_h, tile_w = tile_size
        
        def transform(grid: np.ndarray) -> np.ndarray:
            """Applique tiling sur grille"""
            if grid.size == 0:
                return grid.copy()
            
            h, w = grid.shape
            
            # Extraire tuile source (coin supérieur gauche)
            if h < tile_h or w < tile_w:
                return grid.copy()
            
            tile = grid[:tile_h, :tile_w].copy()
            
            # Calculer dimensions output (multiple de tile_size)
            out_h = ((h // tile_h) + (1 if h % tile_h > 0 else 0)) * tile_h
            out_w = ((w // tile_w) + (1 if w % tile_w > 0 else 0)) * tile_w
            
            # Créer grille tiled
            result = np.zeros((out_h, out_w), dtype=grid.dtype)
            
            # Répéter tuile
            for i in range(0, out_h, tile_h):
                for j in range(0, out_w, tile_w):
                    # Copier tuile (avec gestion bords)
                    end_i = min(i + tile_h, out_h)
                    end_j = min(j + tile_w, out_w)
                    tile_h_actual = end_i - i
                    tile_w_actual = end_j - j
                    result[i:end_i, j:end_j] = tile[:tile_h_actual, :tile_w_actual]
            
            # Retourner taille originale si possible
            if out_h == h and out_w == w:
                return result
            else:
                # Crop à taille originale
                return result[:h, :w]
        
        return transform


class DiagonalSymmetryDetector:
    """
    Détecteur symétrie diagonale (26.2% échecs)
    
    Détecte:
    - Symétrie diagonale
    - Transpose + transformations
    - Réflexions diagonales
    """
    
    def __init__(self):
        self.name = "diagonal_symmetry"
        self.min_confidence = 0.7
    
    def detect(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Détecte symétrie diagonale"""
        global FORENSIC_LOGGER
        if FORENSIC_LOGGER:
            FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={
                    "detector": self.name,
                    "num_pairs": len(train_pairs)
                }
            )
        
        if not train_pairs:
            return None
        
        # Vérifier si output = transpose(input)
        is_transpose = []
        for inp, out in train_pairs:
            if inp.shape[0] == out.shape[1] and inp.shape[1] == out.shape[0]:
                is_transpose.append(np.array_equal(out, inp.T))
            else:
                is_transpose.append(False)
        
        # Tous sont transpose
        if all(is_transpose):
            return PatternDetectionResult(
                pattern_name=self.name,
                confidence=1.0,
                transformation=lambda grid: grid.T,
                parameters={'type': 'transpose'},
                description="Diagonal symmetry: transpose"
            )
        
        # Vérifier symétrie diagonale
        is_symmetric = []
        for inp, out in train_pairs:
            if inp.shape == out.shape and inp.shape[0] == inp.shape[1]:
                is_symmetric.append(np.array_equal(out, out.T))
            else:
                is_symmetric.append(False)
        
        if all(is_symmetric):
            return PatternDetectionResult(
                pattern_name=self.name,
                confidence=0.9,
                transformation=self._create_symmetry_transformation(),
                parameters={'type': 'diagonal_symmetric'},
                description="Diagonal symmetry: symmetric"
            )
        
        return None
    
    def _create_symmetry_transformation(self) -> Callable:
        """Crée transformation symétrie"""
        def transform(grid: np.ndarray) -> np.ndarray:
            # Rendre grille symétrique diagonalement
            if grid.shape[0] != grid.shape[1]:
                return grid.copy()
            
            result = grid.copy()
            for i in range(grid.shape[0]):
                for j in range(i+1, grid.shape[1]):
                    result[j, i] = result[i, j]
            
            return result
        
        return transform


class AdvancedPatternDetectorManager:
    """
    Manager pour tous les détecteurs avancés
    Coordonne détection et sélection meilleur pattern
    """
    
    def __init__(self):
        self.detectors = [
            ObjectCountingDetector(),
            ConditionalRulesDetector(),
            SpatialRelationshipsDetector(),
            ColorGradientDetector(),
            GridTilingDetector(),
            DiagonalSymmetryDetector()
        ]
    
    def detect_all_patterns(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> List[PatternDetectionResult]:
        """Détecte tous les patterns possibles"""
        results = []
        
        # CORRECTION P0.1: Appeler TOUS les détecteurs pour logging forensique
        for detector in self.detectors:
            try:
                result = detector.detect(train_pairs)
                if result:
                    results.append(result)
            except Exception as e:
                # Log erreur mais continue
                print(f"⚠️  Erreur détecteur {detector.name}: {e}")
                continue
        
        # Trier par confiance
        results.sort(key=lambda x: x.confidence, reverse=True)
        
        return results
    
    def get_best_pattern(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[PatternDetectionResult]:
        """Retourne meilleur pattern détecté"""
        results = self.detect_all_patterns(train_pairs)
        return results[0] if results else None


# Made with Bob - Expert Forensique LumVorax