"""
MAGEN Classified Synthesizer - Synthesis guidée par classification

Utilise le PuzzleClassifier pour router vers des stratégies spécialisées
et prioriser les primitives appropriées.
"""

import numpy as np
from typing import List, Tuple, Optional
import time

from .synthesizer import ProgramSynthesizer
from ..classification import PuzzleClassifier, TransformationClass
from ..dsl.program import Program


class ClassifiedSynthesizer(ProgramSynthesizer):
    """
    Synthesizer guidé par classification
    
    Classifie le puzzle AVANT génération pour:
    - Prioriser les primitives appropriées
    - Ajuster les paramètres de recherche
    - Router vers des stratégies spécialisées
    """
    
    def __init__(self, max_depth: int = 7, max_candidates: int = 200, 
                 timeout: float = 30.0, enable_simplification: bool = True):
        """
        Args:
            max_depth: Profondeur maximale (augmentée à 7 par défaut)
            max_candidates: Nombre de candidats (augmenté à 200)
            timeout: Timeout en secondes
            enable_simplification: Activer simplification
        """
        super().__init__(max_depth, max_candidates, timeout, enable_simplification)
        self.classifier = PuzzleClassifier()
        
        # Stratégies par classe
        self.class_strategies = {
            TransformationClass.GEOMETRIC: self._geometric_strategy,
            TransformationClass.PERIODIC: self._periodic_strategy,
            TransformationClass.OBJECT_RELATIONAL: self._relational_strategy,
            TransformationClass.SEMANTIC_REGION: self._region_strategy,
        }
    
    def synthesize(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]], 
                   test_input: Optional[np.ndarray] = None) -> Optional[Program]:
        """
        Synthétise un programme avec classification préalable
        
        Args:
            train_pairs: Paires (input, output) d'entraînement
            test_input: Input de test (optionnel)
            
        Returns:
            Programme synthétisé ou None
        """
        start_time = time.time()
        
        # ÉTAPE 1: Classifier le puzzle
        classification = self.classifier.classify(train_pairs)
        
        print(f"[CLASSIFIED] Puzzle classé: {classification.primary_class.value} "
              f"(confiance: {classification.confidence:.1%})")
        
        # ÉTAPE 2: Appliquer stratégie spécialisée
        strategy = self.class_strategies.get(
            classification.primary_class,
            self._default_strategy
        )
        
        # ÉTAPE 3: Générer candidats avec stratégie
        candidates = strategy(train_pairs, classification)
        
        if not candidates:
            print(f"[CLASSIFIED] Aucun candidat généré par stratégie {classification.primary_class.value}")
            # Fallback: stratégie par défaut
            candidates = self._default_strategy(train_pairs, classification)
        
        print(f"[CLASSIFIED] {len(candidates)} candidats générés")
        
        # ÉTAPE 4: Scorer et valider (utilise la méthode parent)
        best_program = None
        best_score = -1.0
        
        for program in candidates:
            if time.time() - start_time > self.timeout:
                break
            
            # Scorer
            score = self.scorer.score(program, train_pairs)
            
            if score > best_score:
                # Valider
                if self.validator.validate(program, train_pairs):
                    best_score = score
                    best_program = program
        
        if best_program:
            print(f"[CLASSIFIED] Programme trouvé (score: {best_score:.3f})")
        
        return best_program
    
    def _geometric_strategy(self, train_pairs: List[Tuple], classification) -> List[Program]:
        """Stratégie pour transformations géométriques"""
        from ..dsl.composer import Composer
        
        composer = Composer(max_depth=self.max_depth, max_candidates=self.max_candidates)
        
        # Prioriser primitives géométriques
        features = classification.features.get('geometric', {})
        
        candidates = []
        
        # Si scale détecté, prioriser scale
        if features.get('scale_detected'):
            scale_factor = features.get('scale_factor', 2.0)
            if scale_factor > 1:
                # Scale up
                for factor in [2, 3, 4]:
                    if abs(factor - scale_factor) < 0.1:
                        candidates.extend(composer._generate_single_op_programs(
                            train_pairs, 'scale', [(factor,)]
                        ))
            else:
                # Scale down
                factor = int(1.0 / scale_factor)
                candidates.extend(composer._generate_single_op_programs(
                    train_pairs, 'downscale', [(factor,)]
                ))
        
        # Rotations et miroirs
        if features.get('rotation_detected'):
            for op in ['rotate90', 'rotate180', 'rotate270']:
                candidates.extend(composer._generate_single_op_programs(
                    train_pairs, op, [()]
                ))
        
        if features.get('mirror_detected'):
            for op in ['mirror_horizontal', 'mirror_vertical']:
                candidates.extend(composer._generate_single_op_programs(
                    train_pairs, op, [()]
                ))
        
        # Si pas assez de candidats, générer tous géométriques
        if len(candidates) < 20:
            candidates.extend(composer.generate_candidates(train_pairs))
        
        return candidates[:self.max_candidates]
    
    def _periodic_strategy(self, train_pairs: List[Tuple], classification) -> List[Program]:
        """Stratégie pour patterns périodiques"""
        from ..dsl.composer import Composer
        
        composer = Composer(max_depth=self.max_depth, max_candidates=self.max_candidates)
        
        candidates = []
        
        # Prioriser extract_repeated_pattern
        candidates.extend(composer._generate_single_op_programs(
            train_pairs, 'extract_repeated_pattern', [()]
        ))
        
        # Puis downscale avec facteurs détectés
        features = classification.features.get('periodic', {})
        ratio = features.get('compression_ratio', 3.0)
        if ratio > 1:
            factor = int(ratio)
            candidates.extend(composer._generate_single_op_programs(
                train_pairs, 'downscale', [(factor,)]
            ))
        
        # Compositions: extract + autres ops
        if len(candidates) < 50:
            candidates.extend(composer.generate_candidates(train_pairs))
        
        return candidates[:self.max_candidates]
    
    def _relational_strategy(self, train_pairs: List[Tuple], classification) -> List[Program]:
        """Stratégie pour raisonnement relationnel"""
        from ..dsl.composer import Composer
        
        composer = Composer(max_depth=self.max_depth, max_candidates=self.max_candidates)
        
        # Pour l'instant, utiliser stratégie par défaut
        # TODO: Ajouter primitives relationnelles (detect_L_shapes, mark_corners, etc.)
        return composer.generate_candidates(train_pairs)[:self.max_candidates]
    
    def _region_strategy(self, train_pairs: List[Tuple], classification) -> List[Program]:
        """Stratégie pour sélection de régions"""
        from ..dsl.composer import Composer
        
        composer = Composer(max_depth=self.max_depth, max_candidates=self.max_candidates)
        
        candidates = []
        
        # Prioriser crop et downscale
        features = classification.features.get('semantic_region', {})
        
        if features.get('is_crop'):
            # Crop détecté
            candidates.extend(composer._generate_single_op_programs(
                train_pairs, 'crop', composer._generate_crop_boxes(train_pairs[0][0])
            ))
        
        # Downscale avec ratios détectés
        h_ratio = features.get('ratio_h', 1.0)
        w_ratio = features.get('ratio_w', 1.0)
        
        if h_ratio < 1.0 and h_ratio == w_ratio:
            factor = int(1.0 / h_ratio)
            if factor > 1:
                candidates.extend(composer._generate_single_op_programs(
                    train_pairs, 'downscale', [(factor,)]
                ))
        
        # Compléter avec autres candidats
        if len(candidates) < 50:
            candidates.extend(composer.generate_candidates(train_pairs))
        
        return candidates[:self.max_candidates]
    
    def _default_strategy(self, train_pairs: List[Tuple], classification) -> List[Program]:
        """Stratégie par défaut (fallback)"""
        from ..dsl.composer import Composer
        
        composer = Composer(max_depth=self.max_depth, max_candidates=self.max_candidates)
        return composer.generate_candidates(train_pairs)[:self.max_candidates]


# Export
__all__ = ['ClassifiedSynthesizer']

# Made with Bob
