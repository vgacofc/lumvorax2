"""
MAGEN - Beam Search
Recherche par faisceau pour exploration contrôlée

CRITIQUE identifiée par analyse experte:
- Beam search AVANT MCTS
- Contrôle explosion combinatoire
- Exploration efficace espace hypothèses

Architecture:
- Beam width configurable
- Expansion par niveau
- Scoring à chaque étape
- Pruning agressif
- Best-first ordering
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional, Callable
from dataclasses import dataclass, field
from collections import defaultdict
import copy

# Import modules MAGEN
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic import Transform, ComposedTransform, Hypothesis, TransformType, HypothesisGenerator
from search.pruning_system import PruningSystem, ScoredHypothesis


@dataclass
class BeamNode:
    """
    Nœud dans le beam search
    
    Représente un état partiel de résolution:
    - Transformations appliquées jusqu'ici
    - Grille résultante
    - Score cumulé
    """
    transforms: List[Transform] = field(default_factory=list)
    current_grid: Optional[np.ndarray] = None
    cumulative_score: float = 0.0
    depth: int = 0
    parent: Optional['BeamNode'] = None
    
    def __repr__(self):
        return f"BeamNode(depth={self.depth}, score={self.cumulative_score:.3f}, transforms={len(self.transforms)})"


class BeamSearch:
    """
    Beam Search pour ARC-AGI-3
    
    Exploration contrôlée de l'espace des transformations:
    1. Maintenir beam de k meilleures hypothèses
    2. Expand chaque hypothèse
    3. Score nouvelles hypothèses
    4. Prune pour garder top-k
    5. Répéter jusqu'à solution ou max_depth
    """
    
    def __init__(self, beam_width: int = 10,
                 max_depth: int = 5,
                 pruning_system: Optional[PruningSystem] = None,
                 hypothesis_generator: Optional[HypothesisGenerator] = None,
                 verbose: bool = False):
        self.beam_width = beam_width
        self.max_depth = max_depth
        self.pruning_system = pruning_system or PruningSystem(beam_width=beam_width)
        self.hypothesis_generator = hypothesis_generator
        self.verbose = verbose
        
        # Statistiques
        self.nodes_expanded = 0
        self.nodes_pruned = 0
        self.solutions_found = 0
        self.max_depth_reached = 0
    
    def search(self, input_grid: np.ndarray, target_grid: np.ndarray,
              score_function: Callable[[np.ndarray, np.ndarray], float]) -> List[ScoredHypothesis]:
        """
        Beam search principal
        
        Args:
            input_grid: Grille d'entrée
            target_grid: Grille cible
            score_function: Fonction de scoring (grid_result, target) -> score
        
        Returns:
            Liste de meilleures hypothèses trouvées
        """
        # Initialiser beam avec nœud racine
        root = BeamNode(
            transforms=[],
            current_grid=input_grid.copy(),
            cumulative_score=0.0,
            depth=0
        )
        
        current_beam = [root]
        best_solutions: List[ScoredHypothesis] = []
        
        # Itérer par profondeur
        for depth in range(self.max_depth):
            if self.verbose:
                print(f"\n=== Beam Search Depth {depth} ===")
                print(f"Current beam size: {len(current_beam)}")
            
            # Expand tous les nœuds du beam
            next_beam = []
            for node in current_beam:
                expanded = self._expand_node(node, input_grid, target_grid, score_function)
                next_beam.extend(expanded)
                self.nodes_expanded += len(expanded)
            
            if not next_beam:
                if self.verbose:
                    print("No more nodes to expand")
                break
            
            # Prune pour garder top-k
            next_beam = self._prune_beam(next_beam, target_grid, score_function)
            self.nodes_pruned += len(next_beam) - min(len(next_beam), self.beam_width)
            
            # Vérifier solutions
            for node in next_beam:
                if self._is_solution(node.current_grid, target_grid):
                    # Créer hypothèse
                    hypothesis = self._node_to_hypothesis(node)
                    scored = self.pruning_system.score_hypothesis(hypothesis, accuracy=1.0)
                    best_solutions.append(scored)
                    self.solutions_found += 1
                    
                    if self.verbose:
                        print(f"✓ Solution found at depth {depth}!")
            
            current_beam = next_beam
            self.max_depth_reached = depth + 1
            
            # Early stopping si solution parfaite trouvée
            if best_solutions and any(s.accuracy == 1.0 for s in best_solutions):
                if self.verbose:
                    print("Perfect solution found, stopping search")
                break
        
        # Trier solutions par score
        best_solutions.sort(key=lambda s: s.total_score, reverse=True)
        
        return best_solutions[:self.beam_width]
    
    def _expand_node(self, node: BeamNode, input_grid: np.ndarray,
                    target_grid: np.ndarray,
                    score_function: Callable) -> List[BeamNode]:
        """
        Expand un nœud en générant transformations candidates
        
        Returns:
            Liste de nœuds enfants
        """
        children = []
        
        # Générer transformations candidates
        candidate_transforms = self._generate_candidate_transforms(node, input_grid, target_grid)
        
        # Créer nœud enfant pour chaque transformation
        for transform in candidate_transforms:
            # Appliquer transformation
            try:
                new_grid = self._apply_transform(node.current_grid, transform)
                
                # Calculer score
                score = score_function(new_grid, target_grid)
                
                # Créer nœud enfant
                child = BeamNode(
                    transforms=node.transforms + [transform],
                    current_grid=new_grid,
                    cumulative_score=node.cumulative_score + score,
                    depth=node.depth + 1,
                    parent=node
                )
                
                children.append(child)
            
            except Exception as e:
                if self.verbose:
                    print(f"Error applying transform: {e}")
                continue
        
        return children
    
    def _generate_candidate_transforms(self, node: BeamNode,
                                      input_grid: np.ndarray,
                                      target_grid: np.ndarray) -> List[Transform]:
        """
        Générer transformations candidates pour expansion
        
        Stratégies:
        1. Transformations géométriques basiques
        2. Transformations basées sur différences input/target
        3. Transformations basées sur priors
        """
        candidates = []
        
        # Stratégie 1: Transformations géométriques
        geometric_types = [
            TransformType.ROTATE_90,
            TransformType.ROTATE_180,
            TransformType.FLIP_HORIZONTAL,
            TransformType.FLIP_VERTICAL,
        ]
        
        for ttype in geometric_types:
            candidates.append(Transform(type=ttype, parameters={}))
        
        # Stratégie 2: Transformations de couleurs
        # Détecter couleurs présentes
        current_colors = set(node.current_grid.flatten())
        target_colors = set(target_grid.flatten())
        
        # Si couleurs différentes, proposer recolor
        if current_colors != target_colors:
            for old_color in current_colors:
                for new_color in target_colors:
                    if old_color != new_color:
                        candidates.append(Transform(
                            type=TransformType.RECOLOR,
                            parameters={'old_color': int(old_color), 'new_color': int(new_color)}
                        ))
        
        # Stratégie 3: Transformations spatiales
        # Si tailles différentes, proposer scale/crop
        if node.current_grid.shape != target_grid.shape:
            h_ratio = target_grid.shape[0] / node.current_grid.shape[0]
            w_ratio = target_grid.shape[1] / node.current_grid.shape[1]
            
            if h_ratio > 1 and w_ratio > 1:
                # Scale up
                factor = int(min(h_ratio, w_ratio))
                if factor > 1:
                    candidates.append(Transform(
                        type=TransformType.SCALE_UP,
                        parameters={'factor': factor}
                    ))
        
        # Limiter nombre de candidates
        return candidates[:20]  # Max 20 candidates par expansion
    
    def _apply_transform(self, grid: np.ndarray, transform: Transform) -> np.ndarray:
        """Appliquer transformation à une grille"""
        # Import local pour éviter circular import
        from symbolic import PrimitiveTransforms
        
        primitives = PrimitiveTransforms()
        return primitives.apply_transform(grid, transform)
    
    def _prune_beam(self, nodes: List[BeamNode], target_grid: np.ndarray,
                   score_function: Callable) -> List[BeamNode]:
        """
        Prune beam pour garder top-k nœuds
        
        Trie par cumulative_score décroissant
        """
        # Trier par score
        sorted_nodes = sorted(nodes, key=lambda n: n.cumulative_score, reverse=True)
        
        # Garder top-k
        pruned = sorted_nodes[:self.beam_width]
        
        return pruned
    
    def _is_solution(self, grid: np.ndarray, target: np.ndarray) -> bool:
        """Vérifier si grille est solution exacte"""
        return np.array_equal(grid, target)
    
    def _node_to_hypothesis(self, node: BeamNode) -> Hypothesis:
        """Convertir nœud en hypothèse"""
        from symbolic import TransformComposer
        
        composer = TransformComposer(self.pruning_system.cost_model.transform_costs)
        composed = composer.compose(node.transforms)
        
        hypothesis = Hypothesis(
            composed_transform=composed,
            score=node.cumulative_score / max(node.depth, 1),  # Score moyen
            verified_examples=1 if node.cumulative_score > 0.9 else 0,
            total_examples=1
        )
        
        return hypothesis
    
    def get_statistics(self) -> Dict[str, Any]:
        """Statistiques complètes"""
        return {
            'nodes_expanded': self.nodes_expanded,
            'nodes_pruned': self.nodes_pruned,
            'solutions_found': self.solutions_found,
            'max_depth_reached': self.max_depth_reached,
            'beam_width': self.beam_width,
            'max_depth': self.max_depth,
            'pruning_stats': self.pruning_system.get_statistics()
        }


# Made with Bob - Expert en Search Optimization, Beam Search, Heuristic Search, AGI Systems