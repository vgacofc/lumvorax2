"""
MAGEN V23 - Explanation Generator
==================================

Architecture: Explanation-First (NOT Accept-First)

Principe: Le système DOIT expliquer la règle du puzzle AVANT de générer une solution.

Flow:
    Input → Understand → Explain → Validate Explanation → Generate → Output

Objectif: Forcer vraie compréhension, pas approximation acceptable.
"""

import numpy as np
from typing import List, Dict, Any, Optional, Tuple
from dataclasses import dataclass
from enum import Enum
import json


class ExplanationType(Enum):
    """Types d'explications possibles"""
    TRANSFORMATION = "transformation"  # Rotation, miroir, etc.
    RULE_BASED = "rule_based"  # Si X alors Y
    PATTERN_REPETITION = "pattern_repetition"  # Répétition de motif
    COLOR_MAPPING = "color_mapping"  # Mapping de couleurs
    SPATIAL_RELATION = "spatial_relation"  # Relations spatiales
    OBJECT_MANIPULATION = "object_manipulation"  # Manipulation d'objets
    GRID_OPERATION = "grid_operation"  # Opérations sur grille
    COMPOSITE = "composite"  # Combinaison de plusieurs règles


@dataclass
class Explanation:
    """
    Représentation d'une explication de puzzle.
    
    Une explication est une hypothèse sur la règle du puzzle,
    formulée de manière testable et vérifiable.
    """
    type: ExplanationType
    description: str  # Description humaine de la règle
    formal_rule: Dict[str, Any]  # Règle formalisée (testable)
    confidence: float  # Confiance dans l'explication (0.0-1.0)
    evidence: List[str]  # Preuves supportant l'explication
    counter_evidence: List[str]  # Contre-preuves
    testable: bool  # Est-ce que l'explication est testable?
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertir en dictionnaire"""
        return {
            'type': self.type.value,
            'description': self.description,
            'formal_rule': self.formal_rule,
            'confidence': self.confidence,
            'evidence': self.evidence,
            'counter_evidence': self.counter_evidence,
            'testable': self.testable
        }


@dataclass
class ExplanationValidationResult:
    """Résultat de validation d'une explication"""
    is_valid: bool
    accuracy: float  # Précision sur exemples de train (0.0-1.0)
    consistency: float  # Cohérence interne (0.0-1.0)
    generalizability: float  # Capacité de généralisation (0.0-1.0)
    failures: List[str]  # Exemples où l'explication échoue
    score: float  # Score global (0.0-1.0)


class ExplanationGenerator:
    """
    Générateur d'explications pour puzzles ARC-AGI.
    
    Principe: Analyser les exemples de train pour générer des hypothèses
    explicites sur la règle du puzzle, AVANT de tenter de résoudre.
    """
    
    def __init__(self, min_confidence: float = 0.6, verbose: bool = False):
        """
        Args:
            min_confidence: Confiance minimale pour accepter une explication
            verbose: Mode verbeux pour debugging
        """
        self.min_confidence = min_confidence
        self.verbose = verbose
        
        # Statistiques
        self.stats = {
            'explanations_generated': 0,
            'explanations_validated': 0,
            'explanations_rejected': 0,
            'avg_confidence': 0.0
        }
    
    def generate_explanations(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """
        Générer toutes les explications possibles pour un puzzle.
        
        Args:
            train_pairs: Liste de paires (input, output) d'entraînement
            
        Returns:
            Liste d'explications triées par confiance décroissante
        """
        if self.verbose:
            print(f"\n🧠 EXPLANATION GENERATOR")
            print(f"   Train pairs: {len(train_pairs)}")
        
        explanations = []
        
        # 1. Détecter transformations géométriques
        transform_explanations = self._detect_transformations(train_pairs)
        explanations.extend(transform_explanations)
        
        # 2. Détecter règles basées sur conditions
        rule_explanations = self._detect_rules(train_pairs)
        explanations.extend(rule_explanations)
        
        # 3. Détecter patterns de répétition
        pattern_explanations = self._detect_patterns(train_pairs)
        explanations.extend(pattern_explanations)
        
        # 4. Détecter mappings de couleurs
        color_explanations = self._detect_color_mappings(train_pairs)
        explanations.extend(color_explanations)
        
        # 5. Détecter relations spatiales
        spatial_explanations = self._detect_spatial_relations(train_pairs)
        explanations.extend(spatial_explanations)
        
        # 6. Détecter manipulations d'objets
        object_explanations = self._detect_object_manipulations(train_pairs)
        explanations.extend(object_explanations)
        
        # Trier par confiance
        explanations.sort(key=lambda e: e.confidence, reverse=True)
        
        # Statistiques
        self.stats['explanations_generated'] += len(explanations)
        if explanations:
            self.stats['avg_confidence'] = np.mean([e.confidence for e in explanations])
        
        if self.verbose:
            print(f"   Explanations generated: {len(explanations)}")
            if explanations:
                print(f"   Best confidence: {explanations[0].confidence:.2f}")
                print(f"   Best type: {explanations[0].type.value}")
        
        return explanations
    
    def validate_explanation(
        self,
        explanation: Explanation,
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> ExplanationValidationResult:
        """
        Valider une explication sur les exemples de train.
        
        Une explication est valide si elle:
        1. Prédit correctement tous les outputs de train
        2. Est cohérente (pas de contradictions)
        3. Est généralisable (pas d'overfitting)
        
        Args:
            explanation: Explication à valider
            train_pairs: Paires d'entraînement
            
        Returns:
            Résultat de validation
        """
        if not explanation.testable:
            return ExplanationValidationResult(
                is_valid=False,
                accuracy=0.0,
                consistency=0.0,
                generalizability=0.0,
                failures=["Explanation not testable"],
                score=0.0
            )
        
        # Tester l'explication sur chaque paire
        correct = 0
        failures = []
        
        for i, pair in enumerate(train_pairs):
            predicted = self._apply_explanation(explanation, pair['input'])
            expected = pair['output']
            
            if predicted is not None and np.array_equal(predicted, expected):
                correct += 1
            else:
                failures.append(f"Train pair {i}: prediction mismatch")
        
        accuracy = correct / len(train_pairs) if train_pairs else 0.0
        
        # Cohérence: vérifier pas de contradictions internes
        consistency = self._check_consistency(explanation, train_pairs)
        
        # Généralisabilité: vérifier pas d'overfitting
        generalizability = self._check_generalizability(explanation, train_pairs)
        
        # Score global
        score = (accuracy * 0.5 + consistency * 0.25 + generalizability * 0.25)
        
        is_valid = (accuracy >= 0.8 and consistency >= 0.7 and score >= 0.75)
        
        if is_valid:
            self.stats['explanations_validated'] += 1
        else:
            self.stats['explanations_rejected'] += 1
        
        return ExplanationValidationResult(
            is_valid=is_valid,
            accuracy=accuracy,
            consistency=consistency,
            generalizability=generalizability,
            failures=failures,
            score=score
        )
    
    def _detect_transformations(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter transformations géométriques (rotation, miroir, etc.)"""
        explanations = []
        
        # Vérifier rotation 90°
        if self._test_transformation(train_pairs, 'rotate_90'):
            explanations.append(Explanation(
                type=ExplanationType.TRANSFORMATION,
                description="Rotation de 90° dans le sens horaire",
                formal_rule={'operation': 'rotate_90', 'direction': 'clockwise'},
                confidence=0.9,
                evidence=["All train pairs match rotation 90°"],
                counter_evidence=[],
                testable=True
            ))
        
        # Vérifier rotation 180°
        if self._test_transformation(train_pairs, 'rotate_180'):
            explanations.append(Explanation(
                type=ExplanationType.TRANSFORMATION,
                description="Rotation de 180°",
                formal_rule={'operation': 'rotate_180'},
                confidence=0.9,
                evidence=["All train pairs match rotation 180°"],
                counter_evidence=[],
                testable=True
            ))
        
        # Vérifier miroir horizontal
        if self._test_transformation(train_pairs, 'mirror_horizontal'):
            explanations.append(Explanation(
                type=ExplanationType.TRANSFORMATION,
                description="Miroir horizontal (flip left-right)",
                formal_rule={'operation': 'mirror_horizontal'},
                confidence=0.9,
                evidence=["All train pairs match horizontal mirror"],
                counter_evidence=[],
                testable=True
            ))
        
        # Vérifier miroir vertical
        if self._test_transformation(train_pairs, 'mirror_vertical'):
            explanations.append(Explanation(
                type=ExplanationType.TRANSFORMATION,
                description="Miroir vertical (flip up-down)",
                formal_rule={'operation': 'mirror_vertical'},
                confidence=0.9,
                evidence=["All train pairs match vertical mirror"],
                counter_evidence=[],
                testable=True
            ))
        
        return explanations
    
    def _detect_rules(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter règles conditionnelles (if-then)"""
        explanations = []
        
        # Analyser changements de couleurs
        color_changes = self._analyze_color_changes(train_pairs)
        
        if color_changes:
            explanations.append(Explanation(
                type=ExplanationType.RULE_BASED,
                description=f"Règle de changement de couleur: {color_changes}",
                formal_rule={'type': 'color_change', 'rules': color_changes},
                confidence=0.7,
                evidence=[f"Detected {len(color_changes)} color change rules"],
                counter_evidence=[],
                testable=True
            ))
        
        return explanations
    
    def _detect_patterns(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter patterns de répétition"""
        explanations = []
        
        # Vérifier si output est répétition de input
        for pair in train_pairs:
            inp = pair['input']
            out = pair['output']
            
            # Vérifier répétition horizontale
            if out.shape[1] == inp.shape[1] * 2:
                if np.array_equal(out[:, :inp.shape[1]], inp) and \
                   np.array_equal(out[:, inp.shape[1]:], inp):
                    explanations.append(Explanation(
                        type=ExplanationType.PATTERN_REPETITION,
                        description="Répétition horizontale du pattern (x2)",
                        formal_rule={'operation': 'repeat_horizontal', 'factor': 2},
                        confidence=0.8,
                        evidence=["Output is horizontal repetition of input"],
                        counter_evidence=[],
                        testable=True
                    ))
                    break
        
        return explanations
    
    def _detect_color_mappings(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter mappings de couleurs"""
        explanations = []
        
        # Construire mapping de couleurs
        color_map = {}
        consistent = True
        
        for pair in train_pairs:
            inp = pair['input']
            out = pair['output']
            
            if inp.shape != out.shape:
                consistent = False
                break
            
            for i in range(inp.shape[0]):
                for j in range(inp.shape[1]):
                    in_color = inp[i, j]
                    out_color = out[i, j]
                    
                    if in_color in color_map:
                        if color_map[in_color] != out_color:
                            consistent = False
                            break
                    else:
                        color_map[in_color] = out_color
                
                if not consistent:
                    break
            
            if not consistent:
                break
        
        if consistent and color_map:
            explanations.append(Explanation(
                type=ExplanationType.COLOR_MAPPING,
                description=f"Mapping de couleurs: {color_map}",
                formal_rule={'type': 'color_mapping', 'mapping': color_map},
                confidence=0.85,
                evidence=[f"Consistent color mapping across {len(train_pairs)} pairs"],
                counter_evidence=[],
                testable=True
            ))
        
        return explanations
    
    def _detect_spatial_relations(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter relations spatiales"""
        # TODO: Implémenter détection relations spatiales
        return []
    
    def _detect_object_manipulations(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> List[Explanation]:
        """Détecter manipulations d'objets"""
        # TODO: Implémenter détection manipulations objets
        return []
    
    def _test_transformation(
        self, 
        train_pairs: List[Dict[str, np.ndarray]], 
        transform: str
    ) -> bool:
        """Tester si une transformation s'applique à tous les exemples"""
        for pair in train_pairs:
            inp = pair['input']
            out = pair['output']
            
            if transform == 'rotate_90':
                expected = np.rot90(inp, k=-1)  # Rotation horaire
            elif transform == 'rotate_180':
                expected = np.rot90(inp, k=2)
            elif transform == 'mirror_horizontal':
                expected = np.fliplr(inp)
            elif transform == 'mirror_vertical':
                expected = np.flipud(inp)
            else:
                return False
            
            if not np.array_equal(expected, out):
                return False
        
        return True
    
    def _analyze_color_changes(
        self, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> Dict[int, int]:
        """Analyser changements de couleurs"""
        color_map = {}
        
        for pair in train_pairs:
            inp = pair['input']
            out = pair['output']
            
            if inp.shape != out.shape:
                return {}
            
            unique_in = np.unique(inp)
            unique_out = np.unique(out)
            
            for color_in in unique_in:
                mask = (inp == color_in)
                colors_out = out[mask]
                unique_colors_out = np.unique(colors_out)
                
                if len(unique_colors_out) == 1:
                    color_out = unique_colors_out[0]
                    if color_in in color_map:
                        if color_map[color_in] != color_out:
                            return {}
                    else:
                        color_map[color_in] = color_out
        
        return color_map
    
    def _apply_explanation(
        self, 
        explanation: Explanation, 
        input_grid: np.ndarray
    ) -> Optional[np.ndarray]:
        """Appliquer une explication à une grille d'entrée"""
        try:
            rule = explanation.formal_rule
            
            if explanation.type == ExplanationType.TRANSFORMATION:
                op = rule.get('operation')
                if op == 'rotate_90':
                    return np.rot90(input_grid, k=-1)
                elif op == 'rotate_180':
                    return np.rot90(input_grid, k=2)
                elif op == 'mirror_horizontal':
                    return np.fliplr(input_grid)
                elif op == 'mirror_vertical':
                    return np.flipud(input_grid)
            
            elif explanation.type == ExplanationType.COLOR_MAPPING:
                mapping = rule.get('mapping', {})
                output = input_grid.copy()
                for color_in, color_out in mapping.items():
                    output[input_grid == color_in] = color_out
                return output
            
            elif explanation.type == ExplanationType.PATTERN_REPETITION:
                op = rule.get('operation')
                if op == 'repeat_horizontal':
                    factor = rule.get('factor', 2)
                    return np.hstack([input_grid] * factor)
            
            return None
            
        except Exception:
            return None
    
    def _check_consistency(
        self, 
        explanation: Explanation, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> float:
        """Vérifier cohérence interne de l'explication"""
        # Pour l'instant, retourner 1.0 si pas de contre-preuves
        if not explanation.counter_evidence:
            return 1.0
        
        # Pénaliser si contre-preuves
        penalty = len(explanation.counter_evidence) * 0.1
        return max(0.0, 1.0 - penalty)
    
    def _check_generalizability(
        self, 
        explanation: Explanation, 
        train_pairs: List[Dict[str, np.ndarray]]
    ) -> float:
        """Vérifier généralisabilité de l'explication"""
        # Vérifier si l'explication est trop spécifique
        rule = explanation.formal_rule
        
        # Règles simples sont plus généralisables
        if explanation.type in [ExplanationType.TRANSFORMATION, 
                               ExplanationType.COLOR_MAPPING]:
            return 0.9
        
        # Règles complexes moins généralisables
        if explanation.type == ExplanationType.COMPOSITE:
            return 0.6
        
        return 0.75
    
    def get_stats(self) -> Dict[str, Any]:
        """Obtenir statistiques du générateur"""
        return self.stats.copy()

# Made with Bob
