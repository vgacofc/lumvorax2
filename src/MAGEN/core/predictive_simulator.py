#!/usr/bin/env python3
"""
Predictive Simulator - Simulation future avant action
Inspiré par observation jeux arcade: "Je reset quand je vois que le temps ne suffira pas"
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Callable
from dataclasses import dataclass
from copy import deepcopy


@dataclass
class SimulationResult:
    """Résultat d'une simulation"""
    success_probability: float
    estimated_cost: float
    estimated_time: float
    final_state: any
    trajectory: List[any]
    should_abort: bool
    abort_reason: Optional[str] = None


class PredictiveSimulator:
    """
    Simulateur prédictif pour anticiper résultats futurs
    
    Inspiré par comportement humain jeux arcade:
    - "Je reset quand je vois que le temps ne suffira pas"
    - Simulation mentale avant d'agir
    - Évite branches impossibles
    """
    
    def __init__(self, 
                 max_simulation_depth: int = 10,
                 abort_threshold: float = 0.1):
        self.max_simulation_depth = max_simulation_depth
        self.abort_threshold = abort_threshold
        
        # Statistiques
        self.total_simulations = 0
        self.aborts_recommended = 0
        self.simulations_saved_time = 0
    
    def simulate(self,
                 initial_state: any,
                 action_sequence: List[any],
                 state_transition_fn: Callable,
                 success_check_fn: Callable,
                 depth: Optional[int] = None) -> SimulationResult:
        """
        Simule séquence d'actions depuis état initial
        
        Args:
            initial_state: État de départ
            action_sequence: Séquence d'actions à simuler
            state_transition_fn: Fonction (state, action) -> new_state
            success_check_fn: Fonction (state) -> bool
            depth: Profondeur simulation (défaut: max_simulation_depth)
            
        Returns:
            SimulationResult avec prédictions
        """
        if depth is None:
            depth = min(len(action_sequence), self.max_simulation_depth)
        
        self.total_simulations += 1
        
        # Simulation
        current_state = deepcopy(initial_state)
        trajectory = [current_state]
        cost = 0
        
        for i, action in enumerate(action_sequence[:depth]):
            try:
                # Transition d'état
                current_state = state_transition_fn(current_state, action)
                trajectory.append(current_state)
                cost += 1
                
                # Vérifier succès
                if success_check_fn(current_state):
                    return SimulationResult(
                        success_probability=1.0,
                        estimated_cost=cost,
                        estimated_time=cost * 0.1,  # Estimation temps
                        final_state=current_state,
                        trajectory=trajectory,
                        should_abort=False
                    )
            
            except Exception as e:
                # Échec simulation
                return SimulationResult(
                    success_probability=0.0,
                    estimated_cost=cost,
                    estimated_time=cost * 0.1,
                    final_state=current_state,
                    trajectory=trajectory,
                    should_abort=True,
                    abort_reason=f"Simulation failed: {str(e)}"
                )
        
        # Simulation complète sans succès
        # Estimer probabilité succès basée sur progression
        success_prob = self._estimate_success_probability(
            trajectory, success_check_fn
        )
        
        should_abort = success_prob < self.abort_threshold
        if should_abort:
            self.aborts_recommended += 1
        
        return SimulationResult(
            success_probability=success_prob,
            estimated_cost=cost,
            estimated_time=cost * 0.1,
            final_state=current_state,
            trajectory=trajectory,
            should_abort=should_abort,
            abort_reason=f"Low success probability: {success_prob:.2%}" if should_abort else None
        )
    
    def _estimate_success_probability(self,
                                     trajectory: List[any],
                                     success_check_fn: Callable) -> float:
        """
        Estime probabilité de succès basée sur trajectoire
        
        Heuristique simple: Progression vers objectif
        """
        if len(trajectory) < 2:
            return 0.5
        
        # Vérifier si on se rapproche de l'objectif
        # (Heuristique simple - à améliorer selon domaine)
        try:
            # Si états sont numpy arrays, mesurer distance
            if isinstance(trajectory[0], np.ndarray):
                initial_dist = np.sum(trajectory[0] != 0)  # Distance initiale
                final_dist = np.sum(trajectory[-1] != 0)   # Distance finale
                
                if initial_dist > 0:
                    progress = 1.0 - (final_dist / initial_dist)
                    return max(0.0, min(1.0, progress))
            
            # Sinon, probabilité neutre
            return 0.5
        
        except:
            return 0.5
    
    def estimate_remaining_cost(self,
                               current_state: any,
                               goal_state: any) -> float:
        """
        Estime coût restant pour atteindre objectif
        
        Args:
            current_state: État actuel
            goal_state: État objectif
            
        Returns:
            Coût estimé (nombre d'actions)
        """
        try:
            if isinstance(current_state, np.ndarray) and isinstance(goal_state, np.ndarray):
                # Distance Manhattan pour grilles
                diff = np.abs(current_state - goal_state)
                return float(np.sum(diff))
            else:
                # Heuristique générique
                return 10.0
        except:
            return 10.0
    
    def estimate_remaining_time(self,
                               current_state: any,
                               goal_state: any,
                               time_per_action: float = 0.1) -> float:
        """
        Estime temps restant pour atteindre objectif
        
        Args:
            current_state: État actuel
            goal_state: État objectif
            time_per_action: Temps moyen par action
            
        Returns:
            Temps estimé (secondes)
        """
        cost = self.estimate_remaining_cost(current_state, goal_state)
        return cost * time_per_action
    
    def should_abort(self,
                    current_state: any,
                    goal_state: any,
                    time_remaining: float,
                    time_per_action: float = 0.1) -> Tuple[bool, Optional[str]]:
        """
        Décide si on devrait abandonner (reset)
        
        Inspiré par: "Je reset quand je vois que le temps ne suffira pas"
        
        Args:
            current_state: État actuel
            goal_state: État objectif
            time_remaining: Temps restant disponible
            time_per_action: Temps moyen par action
            
        Returns:
            (should_abort, reason)
        """
        estimated_time = self.estimate_remaining_time(
            current_state, goal_state, time_per_action
        )
        
        if estimated_time > time_remaining:
            self.aborts_recommended += 1
            self.simulations_saved_time += estimated_time
            return True, f"Temps insuffisant: besoin {estimated_time:.1f}s, reste {time_remaining:.1f}s"
        
        return False, None
    
    def can_finish_before_timeout(self,
                                  current_state: any,
                                  goal_state: any,
                                  timeout: float,
                                  time_per_action: float = 0.1) -> bool:
        """
        Vérifie si on peut finir avant timeout
        
        Args:
            current_state: État actuel
            goal_state: État objectif
            timeout: Temps limite
            time_per_action: Temps moyen par action
            
        Returns:
            True si faisable, False sinon
        """
        estimated_time = self.estimate_remaining_time(
            current_state, goal_state, time_per_action
        )
        return estimated_time <= timeout
    
    def get_stats(self) -> Dict:
        """Statistiques simulation"""
        return {
            'total_simulations': self.total_simulations,
            'aborts_recommended': self.aborts_recommended,
            'abort_rate': self.aborts_recommended / max(self.total_simulations, 1),
            'time_saved_seconds': self.simulations_saved_time
        }


# Exemple d'utilisation
if __name__ == "__main__":
    simulator = PredictiveSimulator(max_simulation_depth=5)
    
    # Simuler jeu simple
    def transition(state, action):
        """Transition simple: incrémenter position"""
        new_state = state.copy()
        new_state[0] += action
        return new_state
    
    def success_check(state):
        """Succès si position >= 10"""
        return state[0] >= 10
    
    # Test simulation
    initial = np.array([0])
    actions = [1, 1, 1, 1, 1]  # 5 actions +1
    
    result = simulator.simulate(
        initial, actions, transition, success_check
    )
    
    print(f"Success probability: {result.success_probability:.2%}")
    print(f"Estimated cost: {result.estimated_cost}")
    print(f"Should abort: {result.should_abort}")
    
    # Test abort decision
    current = np.array([5])
    goal = np.array([100])
    should_abort, reason = simulator.should_abort(current, goal, time_remaining=1.0)
    print(f"\nShould abort: {should_abort}")
    print(f"Reason: {reason}")
    
    print(f"\nStats: {simulator.get_stats()}")

# Made with Bob
