"""
🎮 ACTION SPACE EXPLORER - MAGEN V22 ARCADE DISCOVERY ENGINE

Exploration systématique de toutes les actions disponibles pour découvrir
les mécaniques cachées et les causalités action→effet.

Inspiré de l'insight utilisateur:
"Le vrai blocage n'est pas la perception, mais l'exploration complète 
des actions et des causalités"

Author: Claude + LumVorax
Version: 22.0.0
Date: 2026-06-13
"""

import numpy as np
from typing import List, Dict, Tuple, Set, Any, Optional
from dataclasses import dataclass, field
from collections import defaultdict
import copy


@dataclass
class ActionEffect:
    """Représente l'effet d'une action"""
    action_name: str
    immediate_effect: Dict[str, Any]
    delayed_effects: List[Dict[str, Any]] = field(default_factory=list)
    context_signature: str = ""
    success: bool = True
    error: Optional[str] = None


@dataclass
class CausalEdge:
    """Arête du graphe causal"""
    action: str
    context: str
    effect: str
    confidence: float
    observations: int = 1


class ActionSpaceExplorer:
    """
    Explore systématiquement toutes les actions disponibles.
    
    Fonctionnalités:
    1. Test exhaustif de toutes les actions
    2. Enregistrement effets immédiats et différés
    3. Construction graphe causal action→effet
    4. Détection actions contextuelles
    """
    
    def __init__(self, max_delay_steps: int = 3, verbose: bool = False):
        """
        Args:
            max_delay_steps: Nombre de steps pour observer effets différés
            verbose: Mode verbeux pour debugging
        """
        self.max_delay_steps = max_delay_steps
        self.verbose = verbose
        
        # Graphe causal
        self.causal_graph: Dict[Tuple[str, str], CausalEdge] = {}
        
        # Historique explorations
        self.exploration_history: List[ActionEffect] = []
        
        # Actions découvertes
        self.discovered_actions: Set[str] = set()
        
        # Statistiques
        self.stats = {
            'total_explorations': 0,
            'successful_explorations': 0,
            'failed_explorations': 0,
            'unique_effects': 0,
            'context_dependent_actions': 0
        }
    
    def explore_all_actions(
        self,
        grid: np.ndarray,
        available_actions: List[str],
        simulator: Any
    ) -> Dict[str, ActionEffect]:
        """
        Explore toutes les actions disponibles.
        
        Args:
            grid: Grille actuelle
            available_actions: Liste des actions possibles
            simulator: Simulateur pour tester les actions
            
        Returns:
            Dictionnaire action_name → ActionEffect
        """
        action_effects = {}
        
        for action in available_actions:
            if self.verbose:
                print(f"🔍 Exploring action: {action}")
            
            effect = self._explore_single_action(grid, action, simulator)
            action_effects[action] = effect
            
            # Enregistrer dans historique
            self.exploration_history.append(effect)
            self.discovered_actions.add(action)
            
            # Mettre à jour stats
            self.stats['total_explorations'] += 1
            if effect.success:
                self.stats['successful_explorations'] += 1
            else:
                self.stats['failed_explorations'] += 1
        
        # Analyser effets uniques
        unique_effects = self._count_unique_effects(action_effects)
        self.stats['unique_effects'] = unique_effects
        
        if self.verbose:
            print(f"✅ Explored {len(available_actions)} actions")
            print(f"📊 Unique effects: {unique_effects}")
        
        return action_effects
    
    def _explore_single_action(
        self,
        grid: np.ndarray,
        action: str,
        simulator: Any
    ) -> ActionEffect:
        """
        Explore une seule action avec effets immédiats et différés.
        
        Args:
            grid: Grille actuelle
            action: Action à tester
            simulator: Simulateur
            
        Returns:
            ActionEffect avec tous les effets observés
        """
        try:
            # État initial
            initial_state = copy.deepcopy(grid)
            
            # Appliquer action (t=0)
            result_t0 = simulator.apply_action(grid, action)
            
            # Effet immédiat
            immediate_effect = self._compute_diff(initial_state, result_t0)
            
            # Effets différés (t+1, t+2, ...)
            delayed_effects = []
            current_state = result_t0
            
            for step in range(1, self.max_delay_steps + 1):
                next_state = simulator.simulate_next_state(current_state)
                delayed_effect = self._compute_diff(current_state, next_state)
                
                delayed_effects.append({
                    'step': step,
                    'diff': delayed_effect,
                    'state': next_state
                })
                
                current_state = next_state
            
            # Signature contexte
            context_sig = self._extract_context_signature(initial_state)
            
            return ActionEffect(
                action_name=action,
                immediate_effect=immediate_effect,
                delayed_effects=delayed_effects,
                context_signature=context_sig,
                success=True
            )
            
        except Exception as e:
            if self.verbose:
                print(f"❌ Error exploring action {action}: {e}")
            
            return ActionEffect(
                action_name=action,
                immediate_effect={},
                delayed_effects=[],
                success=False,
                error=str(e)
            )
    
    def _compute_diff(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> Dict[str, Any]:
        """
        Calcule la différence entre deux états.
        
        Returns:
            Dictionnaire avec:
            - visual_changes: Changements de couleur
            - structural_changes: Ajout/suppression objets
            - topology_changes: Changements de forme
        """
        if before is None or after is None:
            return {'visual_changes': [], 'structural_changes': [], 'topology_changes': []}
        diff = {
            'visual_changes': [],
            'structural_changes': [],
            'topology_changes': []
        }
        
        # Changements visuels (couleur)
        if before.shape == after.shape:
            changed_cells = np.where(before != after)
            if len(changed_cells[0]) > 0:
                for i, j in zip(changed_cells[0], changed_cells[1]):
                    diff['visual_changes'].append({
                        'position': (int(i), int(j)),
                        'before': int(before[i, j]),
                        'after': int(after[i, j])
                    })
        
        # Changements structurels (taille grille)
        if before.shape != after.shape:
            diff['structural_changes'].append({
                'type': 'shape_change',
                'before_shape': before.shape,
                'after_shape': after.shape
            })
        
        # Changements topologiques (objets)
        before_objects = self._extract_objects(before)
        after_objects = self._extract_objects(after)
        
        if len(before_objects) != len(after_objects):
            diff['topology_changes'].append({
                'type': 'object_count_change',
                'before_count': len(before_objects),
                'after_count': len(after_objects)
            })
        
        return diff
    
    def _extract_objects(self, grid: np.ndarray) -> List[Dict]:
        """Extrait les objets de la grille (régions connectées)"""
        objects = []
        visited = np.zeros_like(grid, dtype=bool)
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if not visited[i, j] and grid[i, j] != 0:
                    obj = self._flood_fill(grid, i, j, visited)
                    objects.append(obj)
        
        return objects
    
    def _flood_fill(
        self,
        grid: np.ndarray,
        i: int,
        j: int,
        visited: np.ndarray
    ) -> Dict:
        """Flood fill pour extraire un objet"""
        color = grid[i, j]
        cells = []
        stack = [(i, j)]
        
        while stack:
            ci, cj = stack.pop()
            if (0 <= ci < grid.shape[0] and 
                0 <= cj < grid.shape[1] and
                not visited[ci, cj] and 
                grid[ci, cj] == color):
                
                visited[ci, cj] = True
                cells.append((ci, cj))
                
                # 4-connectivité
                stack.extend([
                    (ci-1, cj), (ci+1, cj),
                    (ci, cj-1), (ci, cj+1)
                ])
        
        return {
            'color': int(color),
            'cells': cells,
            'size': len(cells),
            'bbox': self._compute_bbox(cells)
        }
    
    def _compute_bbox(self, cells: List[Tuple[int, int]]) -> Tuple[int, int, int, int]:
        """Calcule bounding box"""
        if not cells:
            return (0, 0, 0, 0)
        
        rows = [c[0] for c in cells]
        cols = [c[1] for c in cells]
        
        return (min(rows), min(cols), max(rows), max(cols))
    
    def _extract_context_signature(self, grid: np.ndarray) -> str:
        """Extrait signature du contexte"""
        features = []
        
        # Taille grille
        features.append(f"shape_{grid.shape[0]}x{grid.shape[1]}")
        
        # Nombre couleurs uniques
        unique_colors = len(np.unique(grid))
        features.append(f"colors_{unique_colors}")
        
        # Nombre objets
        objects = self._extract_objects(grid)
        features.append(f"objects_{len(objects)}")
        
        # Densité
        density = np.count_nonzero(grid) / grid.size
        features.append(f"density_{density:.2f}")
        
        return "_".join(features)
    
    def _count_unique_effects(self, action_effects: Dict[str, ActionEffect]) -> int:
        """Compte le nombre d'effets uniques"""
        unique_effects = set()
        
        for effect in action_effects.values():
            if effect.success:
                # Signature de l'effet
                sig = self._effect_signature(effect.immediate_effect)
                unique_effects.add(sig)
        
        return len(unique_effects)
    
    def _effect_signature(self, effect: Dict) -> str:
        """Crée signature d'un effet"""
        parts = []
        
        if effect.get('visual_changes'):
            parts.append(f"visual_{len(effect['visual_changes'])}")
        
        if effect.get('structural_changes'):
            parts.append(f"structural_{len(effect['structural_changes'])}")
        
        if effect.get('topology_changes'):
            parts.append(f"topology_{len(effect['topology_changes'])}")
        
        return "_".join(parts) if parts else "no_change"
    
    def build_causal_graph(
        self,
        action_effects: Dict[str, ActionEffect]
    ) -> Dict[Tuple[str, str], CausalEdge]:
        """
        Construit graphe causal action→effet.
        
        Args:
            action_effects: Effets observés pour chaque action
            
        Returns:
            Graphe causal avec arêtes (action, context) → effet
        """
        for action_name, effect in action_effects.items():
            if not effect.success:
                continue
            
            context = effect.context_signature
            effect_sig = self._effect_signature(effect.immediate_effect)
            
            key = (action_name, context)
            
            if key in self.causal_graph:
                # Mettre à jour edge existant
                edge = self.causal_graph[key]
                edge.observations += 1
                edge.confidence = min(1.0, edge.confidence + 0.1)
            else:
                # Créer nouveau edge
                self.causal_graph[key] = CausalEdge(
                    action=action_name,
                    context=context,
                    effect=effect_sig,
                    confidence=0.5
                )
        
        return self.causal_graph
    
    def detect_context_dependent_actions(
        self,
        action_effects: Dict[str, ActionEffect]
    ) -> List[str]:
        """
        Détecte les actions dont l'effet dépend du contexte.
        
        Returns:
            Liste des actions contextuelles
        """
        action_contexts = defaultdict(set)
        
        # Grouper par action
        for effect in action_effects.values():
            if effect.success:
                effect_sig = self._effect_signature(effect.immediate_effect)
                action_contexts[effect.action_name].add(effect_sig)
        
        # Identifier actions avec effets multiples
        context_dependent = []
        for action, effects in action_contexts.items():
            if len(effects) > 1:
                context_dependent.append(action)
        
        self.stats['context_dependent_actions'] = len(context_dependent)
        
        return context_dependent
    
    def get_action_recommendation(
        self,
        current_context: str,
        desired_effect: str
    ) -> Optional[str]:
        """
        Recommande une action basée sur le contexte et l'effet désiré.
        
        Args:
            current_context: Signature du contexte actuel
            desired_effect: Effet désiré
            
        Returns:
            Action recommandée ou None
        """
        best_action = None
        best_confidence = 0.0
        
        for (action, context), edge in self.causal_graph.items():
            if context == current_context and edge.effect == desired_effect:
                if edge.confidence > best_confidence:
                    best_action = action
                    best_confidence = edge.confidence
        
        return best_action
    
    def get_stats(self) -> Dict[str, Any]:
        """Retourne statistiques d'exploration"""
        return {
            **self.stats,
            'causal_edges': len(self.causal_graph),
            'discovered_actions': len(self.discovered_actions),
            'exploration_history_size': len(self.exploration_history)
        }
    
    def reset(self):
        """Réinitialise l'explorateur"""
        self.causal_graph.clear()
        self.exploration_history.clear()
        self.discovered_actions.clear()
        self.stats = {
            'total_explorations': 0,
            'successful_explorations': 0,
            'failed_explorations': 0,
            'unique_effects': 0,
            'context_dependent_actions': 0
        }


# Simulateur simple pour tests
class SimpleSimulator:
    """Simulateur simple pour tester ActionSpaceExplorer"""
    
    def apply_action(self, grid: np.ndarray, action: str) -> np.ndarray:
        """Applique une action sur la grille"""
        result = grid.copy()
        
        if action == "rotate_90":
            result = np.rot90(result)
        elif action == "flip_horizontal":
            result = np.fliplr(result)
        elif action == "flip_vertical":
            result = np.flipud(result)
        elif action == "invert_colors":
            result = np.where(result == 0, 1, 0)
        
        return result
    
    def simulate_next_state(self, grid: np.ndarray) -> np.ndarray:
        """Simule l'état suivant (gravité, etc.)"""
        # Simulation simple: gravité
        result = grid.copy()
        
        for j in range(result.shape[1]):
            # Faire tomber les objets
            column = result[:, j]
            non_zero = column[column != 0]
            zeros = np.zeros(len(column) - len(non_zero), dtype=column.dtype)
            result[:, j] = np.concatenate([zeros, non_zero])
        
        return result


if __name__ == "__main__":
    # Test simple
    print("🎮 Testing Action Space Explorer...")
    
    # Grille test
    grid = np.array([
        [0, 1, 0],
        [0, 0, 2],
        [3, 0, 0]
    ])
    
    # Actions disponibles
    actions = ["rotate_90", "flip_horizontal", "flip_vertical", "invert_colors"]
    
    # Explorer
    explorer = ActionSpaceExplorer(verbose=True)
    simulator = SimpleSimulator()
    
    effects = explorer.explore_all_actions(grid, actions, simulator)
    
    # Construire graphe causal
    causal_graph = explorer.build_causal_graph(effects)
    
    # Détecter actions contextuelles
    context_dependent = explorer.detect_context_dependent_actions(effects)
    
    # Stats
    stats = explorer.get_stats()
    
    print("\n📊 Statistics:")
    for key, value in stats.items():
        print(f"  {key}: {value}")
    
    print(f"\n🔗 Causal graph edges: {len(causal_graph)}")
    print(f"🎯 Context-dependent actions: {context_dependent}")
    
    print("\n✅ Action Space Explorer test complete!")

# Made with Bob
