"""
MAGEN Objects Module
Object-centric reasoning pour ARC-AGI-3

Composants:
- ObjectExtractor: Extraction d'objets et scene graphs
- ObjectTransformer: Transformations d'objets
- ShapeType, RelationType: Enums pour types

Usage:
    from objects import ObjectExtractor, ObjectTransformer, SceneGraph
    
    extractor = ObjectExtractor(connectivity=4)
    scene_graph = extractor.build_scene_graph(grid)
    
    transformer = ObjectTransformer()
    new_sg = transformer.apply_to_scene_graph(scene_graph, transformations)
"""

from .object_extractor import (
    Object,
    Relation,
    SceneGraph,
    ShapeType,
    RelationType,
    ObjectExtractor
)

from .object_transformer import (
    ObjectTransformation,
    ObjectTransformer
)

__all__ = [
    # Core classes
    'Object',
    'Relation',
    'SceneGraph',
    
    # Enums
    'ShapeType',
    'RelationType',
    
    # Extractors
    'ObjectExtractor',
    
    # Transformers
    'ObjectTransformation',
    'ObjectTransformer',
]

__version__ = '1.0.0'
__author__ = 'Bob - MAGEN Team'

# Made with Bob
