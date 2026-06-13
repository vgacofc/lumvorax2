"""
MAGEN - Memory Module
Mémoire épisodique avec analogical reasoning
"""

from .episodic_memory import (
    Episode,
    ConceptualPattern,
    MemoryEncoder,
    EpisodicMemory
)

from .analogical_abstraction import (
    StructuralMapping,
    AbstractPattern,
    StructuralMatcher,
    AbstractPatternExtractor
)

__all__ = [
    'Episode',
    'ConceptualPattern',
    'MemoryEncoder',
    'EpisodicMemory',
    'StructuralMapping',
    'AbstractPattern',
    'StructuralMatcher',
    'AbstractPatternExtractor'
]

# Made with Bob
