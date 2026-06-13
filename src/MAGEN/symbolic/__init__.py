"""
MAGEN Symbolic Module
Moteur de transformations symboliques pour ARC-AGI-3

Composants:
- PrimitiveTransforms: Transformations atomiques
- TransformComposer: Composition de transformations
- HypothesisGenerator: Génération d'hypothèses
- SymbolicVerifier: Vérification exacte

Usage:
    from symbolic import PrimitiveTransforms, TransformComposer, HypothesisGenerator, SymbolicVerifier
    
    primitives = PrimitiveTransforms()
    composer = TransformComposer(primitives)
    generator = HypothesisGenerator(primitives, composer)
    verifier = SymbolicVerifier(primitives, composer)
    
    # Générer hypothèses
    hypotheses = generator.generate_candidates(input_grids, output_grids)
    
    # Vérifier
    best = verifier.find_best_hypothesis(hypotheses, test_examples)
"""

from .primitive_transforms import (
    Transform,
    TransformType,
    PrimitiveTransforms
)

from .transform_composer import (
    ComposedTransform,
    Hypothesis,
    TransformComposer,
    HypothesisGenerator
)

from .symbolic_verifier import (
    VerificationResult,
    SymbolicVerifier
)

__all__ = [
    # Primitives
    'Transform',
    'TransformType',
    'PrimitiveTransforms',
    
    # Composition
    'ComposedTransform',
    'Hypothesis',
    'TransformComposer',
    'HypothesisGenerator',
    
    # Verification
    'VerificationResult',
    'SymbolicVerifier',
]

__version__ = '1.0.0'
__author__ = 'Bob - MAGEN Team'

# Made with Bob
