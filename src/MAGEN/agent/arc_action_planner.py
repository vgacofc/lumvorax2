"""
ARCActionPlanner - Planification d'actions pour atteindre output prédit
Convertit prédiction de grille en séquence d'actions exécutables
"""

import numpy as np
from typing import List, Dict, Tuple, Optional, Set
from dataclasses import dataclass
from collections import deque
import heapq


@dataclass
class ActionPlan:
    """Plan d'actions pour atteindre objectif"""
    actions: List[int]
    estimated_cost: float
    confidence: float
    reasoning: str
    subgoals: List[str]


@dataclass
class GridState:
    """État de grille avec métadonnées"""
    grid: np.ndarray
    action_sequence: List[int]
    cost: float
    
    def __lt__(self, other):
        return self.cost < other.cost


class ARCActionPlanner:
    """
    Planificateur d'actions pour ARC-AGI-3
    
    Stratégies:
    1. Analyse différence current → predicted
    2. Décomposition en sous-objectifs
    3. Recherche séquence d'actions (A*, beam search)
    4. Validation plan avant exécution
    
    LIMITATION ACTUELLE:
    - Actions ARC-AGI-3 sont opaques (pas de modèle forward)
    - Impossible de simuler effet d'une action
    - Solution: Heuristiques basées sur patterns observés
    """
    
    def __init__(self, 
                 max_search_depth: int = 50,
                 beam_width: int = 5,
                 verbose: bool = False):
        self.max_search_depth = max_search_depth
        self.beam_width = beam_width
        self.verbose = verbose
        
        # Historique actions observées (pour apprentissage)
        self.action_effects: Dict[int, List[Dict]] = {}
        
    def plan_actions(self,
                    current_grid: np.ndarray,
                    predicted_output: np.ndarray,
                    available_actions: List[int],
                    training_pairs: Optional[List[Tuple[np.ndarray, np.ndarray]]] = None) -> ActionPlan:
        """
        Planifier séquence d'actions pour atteindre output prédit
        
        Args:
            current_grid: Grille actuelle
            predicted_output: Grille objectif prédite
            available_actions: Actions disponibles
            training_pairs: Paires d'entraînement (optionnel)
            
        Returns:
            Plan d'actions avec séquence, coût, confiance
        """
        if self.verbose:
            print(f"\n[ActionPlanner] 🎯 Planification actions")
            print(f"[ActionPlanner] Current: {current_grid.shape}, Target: {predicted_output.shape}")
            print(f"[ActionPlanner] Actions disponibles: {available_actions}")
        
        # 1. Analyser différence
        diff_analysis = self._analyze_difference(current_grid, predicted_output)
        
        # 2. Générer sous-objectifs
        subgoals = self._generate_subgoals(diff_analysis)
        
        # 3. Stratégie de planification
        if self._is_simple_transformation(diff_analysis):
            # Transformation simple: heuristique directe
            plan = self._plan_simple_transformation(
                current_grid, 
                predicted_output, 
                available_actions,
                diff_analysis
            )
        else:
            # Transformation complexe: recherche
            plan = self._plan_complex_transformation(
                current_grid,
                predicted_output,
                available_actions,
                subgoals
            )
        
        if self.verbose:
            print(f"\n[ActionPlanner] ✅ Plan généré:")
            print(f"   Actions: {plan.actions[:10]}{'...' if len(plan.actions) > 10 else ''}")
            print(f"   Nombre: {len(plan.actions)}")
            print(f"   Confiance: {plan.confidence:.1%}")
            print(f"   Sous-objectifs: {len(plan.subgoals)}")
        
        return plan
    
    def _analyze_difference(self, 
                           current: np.ndarray, 
                           target: np.ndarray) -> Dict:
        """Analyser différence entre grilles"""
        diff = {
            "shape_changed": current.shape != target.shape,
            "current_shape": current.shape,
            "target_shape": target.shape,
            "colors_changed": not np.array_equal(np.unique(current), np.unique(target)),
            "current_colors": set(np.unique(current).tolist()),
            "target_colors": set(np.unique(target).tolist()),
            "pixel_diff_count": 0,
            "pixel_diff_ratio": 0.0,
            "transformation_type": "unknown"
        }
        
        # Compter pixels différents (si même shape)
        if not diff["shape_changed"]:
            diff["pixel_diff_count"] = np.sum(current != target)
            diff["pixel_diff_ratio"] = diff["pixel_diff_count"] / current.size
        
        # Détecter type transformation
        if not diff["shape_changed"]:
            if np.array_equal(current, target):
                diff["transformation_type"] = "identity"
            elif np.array_equal(np.rot90(current), target):
                diff["transformation_type"] = "rotation_90"
            elif np.array_equal(np.rot90(current, 2), target):
                diff["transformation_type"] = "rotation_180"
            elif np.array_equal(np.rot90(current, 3), target):
                diff["transformation_type"] = "rotation_270"
            elif np.array_equal(np.fliplr(current), target):
                diff["transformation_type"] = "flip_horizontal"
            elif np.array_equal(np.flipud(current), target):
                diff["transformation_type"] = "flip_vertical"
            elif diff["pixel_diff_ratio"] < 0.3:
                diff["transformation_type"] = "local_modification"
            else:
                diff["transformation_type"] = "complex"
        else:
            diff["transformation_type"] = "shape_change"
        
        return diff
    
    def _generate_subgoals(self, diff_analysis: Dict) -> List[str]:
        """Générer sous-objectifs basés sur analyse"""
        subgoals = []
        
        if diff_analysis["shape_changed"]:
            subgoals.append(f"Changer shape {diff_analysis['current_shape']} → {diff_analysis['target_shape']}")
        
        if diff_analysis["colors_changed"]:
            added = diff_analysis["target_colors"] - diff_analysis["current_colors"]
            removed = diff_analysis["current_colors"] - diff_analysis["target_colors"]
            if added:
                subgoals.append(f"Ajouter couleurs: {added}")
            if removed:
                subgoals.append(f"Retirer couleurs: {removed}")
        
        trans_type = diff_analysis["transformation_type"]
        if trans_type != "identity":
            subgoals.append(f"Appliquer transformation: {trans_type}")
        
        if diff_analysis.get("pixel_diff_count", 0) > 0:
            subgoals.append(f"Modifier {diff_analysis['pixel_diff_count']} pixels")
        
        return subgoals
    
    def _is_simple_transformation(self, diff_analysis: Dict) -> bool:
        """Vérifier si transformation est simple"""
        simple_types = [
            "identity", "rotation_90", "rotation_180", "rotation_270",
            "flip_horizontal", "flip_vertical", "local_modification"
        ]
        return diff_analysis["transformation_type"] in simple_types
    
    def _plan_simple_transformation(self,
                                   current: np.ndarray,
                                   target: np.ndarray,
                                   available_actions: List[int],
                                   diff_analysis: Dict) -> ActionPlan:
        """
        Planifier transformation simple
        
        HEURISTIQUE: Essayer actions dans ordre intelligent
        basé sur type de transformation détecté
        """
        trans_type = diff_analysis["transformation_type"]
        
        # Mapping transformation → actions prioritaires (heuristique)
        action_priorities = {
            "identity": [],  # Aucune action nécessaire
            "rotation_90": [3, 4, 1],  # Actions rotation probables
            "rotation_180": [3, 3, 1],
            "rotation_270": [3, 4, 4, 1],
            "flip_horizontal": [2, 1],
            "flip_vertical": [2, 2, 1],
            "local_modification": [5, 6, 7, 1],  # Actions modification locale
            "complex": available_actions[:5],  # Essayer premières actions
            "shape_change": [1, 2, 3, 4]  # Actions changement shape
        }
        
        priority_actions = action_priorities.get(trans_type, available_actions[:3])
        
        # Filtrer actions disponibles
        planned_actions = [a for a in priority_actions if a in available_actions]
        
        # Si pas assez d'actions, ajouter actions disponibles
        if len(planned_actions) < 3:
            for action in available_actions:
                if action not in planned_actions:
                    planned_actions.append(action)
                if len(planned_actions) >= 5:
                    break
        
        # Calculer confiance basée sur type transformation
        confidence_map = {
            "identity": 1.0,
            "rotation_90": 0.7,
            "rotation_180": 0.7,
            "rotation_270": 0.7,
            "flip_horizontal": 0.7,
            "flip_vertical": 0.7,
            "local_modification": 0.5,
            "complex": 0.3,
            "shape_change": 0.4
        }
        confidence = confidence_map.get(trans_type, 0.3)
        
        reasoning = f"Transformation détectée: {trans_type}. "
        reasoning += f"Actions prioritaires sélectionnées basées sur heuristique. "
        reasoning += f"Confiance {confidence:.0%} car transformation {'simple' if confidence > 0.6 else 'complexe'}."
        
        return ActionPlan(
            actions=planned_actions,
            estimated_cost=len(planned_actions),
            confidence=confidence,
            reasoning=reasoning,
            subgoals=self._generate_subgoals(diff_analysis)
        )
    
    def _plan_complex_transformation(self,
                                    current: np.ndarray,
                                    target: np.ndarray,
                                    available_actions: List[int],
                                    subgoals: List[str]) -> ActionPlan:
        """
        Planifier transformation complexe
        
        STRATÉGIE: Beam search avec heuristique de distance
        
        LIMITATION: Sans modèle forward, on ne peut pas simuler
        → Utiliser heuristique basée sur patterns observés
        """
        if self.verbose:
            print(f"[ActionPlanner] Transformation complexe détectée")
            print(f"[ActionPlanner] Utilisation beam search (width={self.beam_width})")
        
        # Beam search simplifié
        # État initial
        initial_state = GridState(
            grid=current.copy(),
            action_sequence=[],
            cost=0.0
        )
        
        beam = [initial_state]
        best_plan = None
        best_distance = float('inf')
        
        for depth in range(min(self.max_search_depth, 20)):  # Limiter profondeur
            if not beam:
                break
            
            # Générer successeurs
            next_beam = []
            
            for state in beam:
                # Essayer chaque action disponible
                for action in available_actions[:3]:  # Limiter branching
                    # LIMITATION: On ne peut pas simuler l'action
                    # → Créer état hypothétique avec coût heuristique
                    
                    new_sequence = state.action_sequence + [action]
                    new_cost = len(new_sequence)
                    
                    # Heuristique: distance à l'objectif (approximation)
                    # Dans un vrai système, on simulerait l'action
                    distance = self._estimate_distance(state.grid, target)
                    
                    new_state = GridState(
                        grid=state.grid.copy(),  # Pas de simulation réelle
                        action_sequence=new_sequence,
                        cost=new_cost + distance
                    )
                    
                    next_beam.append(new_state)
                    
                    # Vérifier si meilleur plan
                    if distance < best_distance:
                        best_distance = distance
                        best_plan = new_sequence
            
            # Garder top-k états (beam)
            next_beam.sort(key=lambda s: s.cost)
            beam = next_beam[:self.beam_width]
            
            # Early stopping si distance très faible
            if best_distance < 0.1:
                break
        
        # Si aucun plan trouvé, utiliser actions par défaut
        if best_plan is None or len(best_plan) == 0:
            best_plan = available_actions[:5]
            confidence = 0.2
            reasoning = "Aucun plan optimal trouvé. Utilisation actions par défaut."
        else:
            confidence = max(0.3, 1.0 - best_distance)
            reasoning = f"Beam search (depth={depth+1}). Distance finale: {best_distance:.2f}."
        
        return ActionPlan(
            actions=best_plan,
            estimated_cost=len(best_plan),
            confidence=confidence,
            reasoning=reasoning,
            subgoals=subgoals
        )
    
    def _estimate_distance(self, current: np.ndarray, target: np.ndarray) -> float:
        """
        Estimer distance entre grilles (heuristique)
        
        Utilise plusieurs métriques:
        - Différence pixels
        - Différence couleurs
        - Différence shape
        """
        distance = 0.0
        
        # Shape différente
        if current.shape != target.shape:
            distance += 1.0
        else:
            # Pixels différents
            pixel_diff = np.sum(current != target) / current.size
            distance += pixel_diff
        
        # Couleurs différentes
        current_colors = set(np.unique(current).tolist())
        target_colors = set(np.unique(target).tolist())
        color_diff = len(current_colors.symmetric_difference(target_colors)) / 10.0
        distance += color_diff
        
        return distance
    
    def record_action_effect(self,
                            action: int,
                            grid_before: np.ndarray,
                            grid_after: np.ndarray,
                            success: bool):
        """
        Enregistrer effet d'une action (apprentissage)
        
        Permet d'améliorer heuristiques au fil du temps
        """
        if action not in self.action_effects:
            self.action_effects[action] = []
        
        effect = {
            "grid_before_shape": grid_before.shape,
            "grid_after_shape": grid_after.shape,
            "shape_changed": grid_before.shape != grid_after.shape,
            "pixels_changed": np.sum(grid_before != grid_after) if grid_before.shape == grid_after.shape else -1,
            "success": success
        }
        
        self.action_effects[action].append(effect)
        
        if self.verbose and len(self.action_effects[action]) % 10 == 0:
            print(f"[ActionPlanner] Action {action}: {len(self.action_effects[action])} effets enregistrés")
    
    def get_action_statistics(self) -> Dict:
        """Obtenir statistiques sur actions observées"""
        stats = {}
        for action, effects in self.action_effects.items():
            stats[action] = {
                "total_observations": len(effects),
                "success_rate": sum(e["success"] for e in effects) / len(effects) if effects else 0.0,
                "shape_change_rate": sum(e["shape_changed"] for e in effects) / len(effects) if effects else 0.0
            }
        return stats

# Made with Bob
