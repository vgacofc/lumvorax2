"""
MAGEN Search Module
Systèmes de recherche et pruning pour contrôle explosion combinatoire

CORRECTIONS CRITIQUES identifiées par analyse experte:
- Pruning system (ABSOLUMENT CRITIQUE)
- Beam search (CRITIQUE)
- Program cost model (CRITIQUE)

Composants:
- PruningSystem: Pruning agressif avec cost model
- BeamSearch: Recherche par faisceau
- ProgramCostModel: Modèle de coût de programme
- TransformPriors: Probabilités a priori
- CanonicalEncoder: Détection duplicatas

Usage:
    from search import PruningSystem, BeamSearch
    
    pruning = PruningSystem(beam_width=10)
    beam = BeamSearch(beam_width=10, pruning_system=pruning)
    
    solutions = beam.search(input_grid, target_grid, score_function)
"""

from .pruning_system import (
    ProgramCost,
    ScoredHypothesis,
    ProgramCostModel,
    TransformPriors,
    CanonicalEncoder,
    PruningSystem
)

from .beam_search import (
    BeamNode,
    BeamSearch
)

__all__ = [
    # Pruning
    'ProgramCost',
    'ScoredHypothesis',
    'ProgramCostModel',
    'TransformPriors',
    'CanonicalEncoder',
    'PruningSystem',
    
    # Beam Search
    'BeamNode',
    'BeamSearch',
]

__version__ = '1.0.0'
__author__ = 'Bob - MAGEN Team'

# Made with Bob
