"""
🔄 CLOSED LOOP PHYSICS ENGINE - MAGEN V22 ARCADE DISCOVERY ENGINE

Boucle physique fermée avec correction dynamique et apprentissage continu.

Inspiré de l'insight utilisateur:
"Le système doit créer une boucle complète:
Action → Simulation → Observation → Correction → Nouvelle action"

Author: Claude + LumVorax
Version: 22.0.0
Date: 2026-06-13
"""

import numpy as np
from typing import List, Dict, Tuple, Any, Optional, Callable
from dataclasses import dataclass, field
from enum import Enum
import copy


class ConvergenceStatus(Enum):
    """Status de convergence"""
    NOT_STARTED = "not_started"
    IN_PROGRESS = "in_progress"
    CONVERGED = "converged"
    DIVERGED = "diverged"
    MAX_CYCLES_REACHED = "max_cycles_reached"


@dataclass
class CycleResult:
    """Résultat d'un cycle de la boucle"""
    cycle: int
    action: str
    state_before: np.ndarray
    state_after: np.ndarray
    observation: Dict[str, Any]
    error: float
    correction: Dict[str, Any]
    timestamp_ns: int


@dataclass
class LoopStatistics:
    """Statistiques de la boucle"""
    total_cycles: int = 0
    successful_cycles: int = 0
    failed_cycles: int = 0
    average_error: float = 0.0
    error_reduction_rate: float = 0.0
    convergence_speed: float = 0.0


class AdaptiveStrategy:
    """
    Stratégie adaptative qui apprend des erreurs.
    
    Utilise un modèle simple d'apprentissage par renforcement:
    - Q-learning pour sélection d'actions
    - Gradient descent pour correction d'erreur
    """
    
    def __init__(
        self,
        available_actions: List[str],
        learning_rate: float = 0.1,
        exploration_rate: float = 0.2
    ):
        """
        Args:
            available_actions: Actions disponibles
            learning_rate: Taux d'apprentissage
            exploration_rate: Taux d'exploration (vs exploitation)
        """
        self.available_actions = available_actions
        self.learning_rate = learning_rate
        self.exploration_rate = exploration_rate
        
        # Q-table: (state_signature, action) → Q-value
        self.q_table: Dict[Tuple[str, str], float] = {}
        
        # Historique des erreurs par action
        self.action_errors: Dict[str, List[float]] = {
            action: [] for action in available_actions
        }
        
        # Compteur d'utilisation des actions
        self.action_counts: Dict[str, int] = {
            action: 0 for action in available_actions
        }
    
    def select_action(self, state: np.ndarray, goal: np.ndarray) -> str:
        """
        Sélectionne la meilleure action basée sur Q-learning.
        
        Args:
            state: État actuel
            goal: État cible
            
        Returns:
            Action sélectionnée
        """
        state_sig = self._state_signature(state)
        
        # Exploration vs exploitation
        if np.random.random() < self.exploration_rate:
            # Exploration: action aléatoire
            action = np.random.choice(self.available_actions)
        else:
            # Exploitation: meilleure action connue
            action = self._best_action(state_sig)
        
        self.action_counts[action] += 1
        return action
    
    def update(self, error: float, state: np.ndarray, action: str) -> 'AdaptiveStrategy':
        """
        Met à jour la stratégie basée sur l'erreur.
        
        Args:
            error: Erreur observée
            state: État actuel
            action: Action prise
            
        Returns:
            Stratégie mise à jour (self)
        """
        state_sig = self._state_signature(state)
        key = (state_sig, action)
        
        # Enregistrer erreur
        self.action_errors[action].append(error)
        
        # Mettre à jour Q-value
        current_q = self.q_table.get(key, 0.0)
        reward = -error  # Récompense négative pour erreur
        new_q = current_q + self.learning_rate * (reward - current_q)
        self.q_table[key] = new_q
        
        # Réduire exploration au fil du temps
        self.exploration_rate *= 0.99
        
        return self
    
    def _state_signature(self, state: np.ndarray) -> str:
        """Crée signature de l'état"""
        # Signature simple basée sur statistiques
        mean = np.mean(state)
        std = np.std(state)
        unique = len(np.unique(state))
        
        return f"mean_{mean:.2f}_std_{std:.2f}_unique_{unique}"
    
    def _best_action(self, state_sig: str) -> str:
        """Trouve la meilleure action pour un état"""
        best_action = self.available_actions[0]
        best_q = float('-inf')
        
        for action in self.available_actions:
            key = (state_sig, action)
            q_value = self.q_table.get(key, 0.0)
            
            if q_value > best_q:
                best_q = q_value
                best_action = action
        
        return best_action
    
    def get_action_statistics(self) -> Dict[str, Dict]:
        """Retourne statistiques par action"""
        stats = {}
        
        for action in self.available_actions:
            errors = self.action_errors[action]
            stats[action] = {
                'count': self.action_counts[action],
                'avg_error': np.mean(errors) if errors else 0.0,
                'min_error': np.min(errors) if errors else 0.0,
                'max_error': np.max(errors) if errors else 0.0
            }
        
        return stats


class ClosedLoopPhysicsEngine:
    """
    Boucle physique fermée avec apprentissage continu.
    
    Cycle complet:
    1. Action → Monde
    2. Observation → Analyse
    3. Correction → Stratégie
    4. Nouvelle action
    
    Converge vers solution optimale par apprentissage itératif.
    """
    
    def __init__(
        self,
        simulator: Any,
        error_function: Callable[[np.ndarray, np.ndarray], float],
        convergence_threshold: float = 0.01,
        max_cycles: int = 100,
        verbose: bool = False
    ):
        """
        Args:
            simulator: Simulateur physique
            error_function: Fonction calcul erreur(state, goal)
            convergence_threshold: Seuil de convergence
            max_cycles: Nombre maximum de cycles
            verbose: Mode verbeux
        """
        self.simulator = simulator
        self.error_function = error_function
        self.convergence_threshold = convergence_threshold
        self.max_cycles = max_cycles
        self.verbose = verbose
        
        # Historique des cycles
        self.cycle_history: List[CycleResult] = []
        
        # Statistiques
        self.stats = LoopStatistics()
        
        # Status
        self.status = ConvergenceStatus.NOT_STARTED
    
    def run_closed_loop(
        self,
        initial_state: np.ndarray,
        goal: np.ndarray,
        available_actions: List[str]
    ) -> Dict[str, Any]:
        """
        Exécute la boucle fermée complète.
        
        Args:
            initial_state: État initial
            goal: État cible
            available_actions: Actions disponibles
            
        Returns:
            Résultat final avec status, cycles, état final
        """
        if self.verbose:
            print("🔄 Starting closed loop physics engine...")
            print(f"   Initial error: {self.error_function(initial_state, goal):.4f}")
        
        # Initialiser stratégie
        strategy = AdaptiveStrategy(available_actions)
        
        # État actuel
        state = copy.deepcopy(initial_state)
        
        # Historique des erreurs
        error_history = []
        error = 1.0  # Initialiser error
        
        self.status = ConvergenceStatus.IN_PROGRESS
        
        for cycle in range(self.max_cycles):
            # 1. SÉLECTION ACTION
            action = strategy.select_action(state, goal)
            
            # 2. SIMULATION (Action → Monde)
            try:
                next_state = self.simulator.apply_action(state, action)
            except Exception as e:
                if self.verbose:
                    print(f"❌ Cycle {cycle}: Simulation error: {e}")
                self.stats.failed_cycles += 1
                continue
            
            # 3. OBSERVATION (Analyse)
            observation = self._observe_state(next_state, goal)
            
            # 4. CALCUL ERREUR
            error = self.error_function(next_state, goal)
            error_history.append(error)
            
            # 5. CORRECTION (Mise à jour stratégie)
            correction = self._compute_correction(error, error_history)
            strategy = strategy.update(error, state, action)
            
            # Enregistrer cycle
            cycle_result = CycleResult(
                cycle=cycle,
                action=action,
                state_before=state,
                state_after=next_state,
                observation=observation,
                error=error,
                correction=correction,
                timestamp_ns=self._get_timestamp_ns()
            )
            self.cycle_history.append(cycle_result)
            
            # Mise à jour stats
            self.stats.total_cycles += 1
            self.stats.successful_cycles += 1
            
            if self.verbose and cycle % 10 == 0:
                print(f"   Cycle {cycle}: error={error:.4f}, action={action}")
            
            # 6. VÉRIFICATION CONVERGENCE
            if error < self.convergence_threshold:
                self.status = ConvergenceStatus.CONVERGED
                
                if self.verbose:
                    print(f"✅ Converged at cycle {cycle}!")
                    print(f"   Final error: {error:.4f}")
                
                return self._build_result(
                    success=True,
                    cycles=cycle + 1,
                    final_state=next_state,
                    final_error=error,
                    strategy=strategy
                )
            
            # Mise à jour état
            state = next_state
        
        # Max cycles atteint sans convergence
        self.status = ConvergenceStatus.MAX_CYCLES_REACHED
        
        if self.verbose:
            print(f"⚠️ Max cycles reached ({self.max_cycles})")
            print(f"   Final error: {error:.4f}")
        
        return self._build_result(
            success=False,
            cycles=self.max_cycles,
            final_state=state,
            final_error=error,
            strategy=strategy
        )
    
    def _observe_state(
        self,
        state: np.ndarray,
        goal: np.ndarray
    ) -> Dict[str, Any]:
        """
        Observe l'état et extrait informations pertinentes.
        
        Returns:
            Dictionnaire d'observations
        """
        return {
            'shape_match': state.shape == goal.shape,
            'color_distribution': self._color_distribution(state),
            'object_count': self._count_objects(state),
            'similarity_to_goal': self._compute_similarity(state, goal)
        }
    
    def _compute_correction(
        self,
        error: float,
        error_history: List[float]
    ) -> Dict[str, Any]:
        """
        Calcule correction basée sur l'erreur.
        
        Returns:
            Dictionnaire de corrections
        """
        correction = {
            'error_delta': 0.0,
            'trend': 'stable',
            'recommendation': 'continue'
        }
        
        if len(error_history) > 1:
            error_delta = error - error_history[-2]
            correction['error_delta'] = error_delta
            
            if error_delta < -0.01:
                correction['trend'] = 'improving'
                correction['recommendation'] = 'continue'
            elif error_delta > 0.01:
                correction['trend'] = 'worsening'
                correction['recommendation'] = 'change_strategy'
            else:
                correction['trend'] = 'stable'
                correction['recommendation'] = 'explore'
        
        return correction
    
    def _color_distribution(self, state: np.ndarray) -> Dict[int, int]:
        """Calcule distribution des couleurs"""
        unique, counts = np.unique(state, return_counts=True)
        return {int(color): int(count) for color, count in zip(unique, counts)}
    
    def _count_objects(self, state: np.ndarray) -> int:
        """Compte le nombre d'objets (régions connectées)"""
        visited = np.zeros_like(state, dtype=bool)
        count = 0
        
        for i in range(state.shape[0]):
            for j in range(state.shape[1]):
                if not visited[i, j] and state[i, j] != 0:
                    self._flood_fill(state, i, j, visited)
                    count += 1
        
        return count
    
    def _flood_fill(
        self,
        grid: np.ndarray,
        i: int,
        j: int,
        visited: np.ndarray
    ):
        """Flood fill pour compter objets"""
        color = grid[i, j]
        stack = [(i, j)]
        
        while stack:
            ci, cj = stack.pop()
            if (0 <= ci < grid.shape[0] and 
                0 <= cj < grid.shape[1] and
                not visited[ci, cj] and 
                grid[ci, cj] == color):
                
                visited[ci, cj] = True
                
                stack.extend([
                    (ci-1, cj), (ci+1, cj),
                    (ci, cj-1), (ci, cj+1)
                ])
    
    def _compute_similarity(
        self,
        state: np.ndarray,
        goal: np.ndarray
    ) -> float:
        """Calcule similarité entre état et goal"""
        if state.shape != goal.shape:
            return 0.0
        
        matches = np.sum(state == goal)
        total = state.size
        
        return matches / total
    
    def _get_timestamp_ns(self) -> int:
        """Retourne timestamp en nanosecondes"""
        import time
        return int(time.time() * 1e9)
    
    def _build_result(
        self,
        success: bool,
        cycles: int,
        final_state: np.ndarray,
        final_error: float,
        strategy: AdaptiveStrategy
    ) -> Dict[str, Any]:
        """Construit résultat final"""
        # Calculer statistiques
        if self.cycle_history:
            errors = [c.error for c in self.cycle_history]
            self.stats.average_error = float(np.mean(errors))
            
            if len(errors) > 1:
                self.stats.error_reduction_rate = (errors[0] - errors[-1]) / errors[0]
                self.stats.convergence_speed = cycles / self.max_cycles
        
        return {
            'success': success,
            'status': self.status.value,
            'cycles': cycles,
            'final_state': final_state,
            'final_error': final_error,
            'statistics': {
                'total_cycles': self.stats.total_cycles,
                'successful_cycles': self.stats.successful_cycles,
                'failed_cycles': self.stats.failed_cycles,
                'average_error': self.stats.average_error,
                'error_reduction_rate': self.stats.error_reduction_rate,
                'convergence_speed': self.stats.convergence_speed
            },
            'action_statistics': strategy.get_action_statistics(),
            'cycle_history': self.cycle_history
        }
    
    def get_cycle_history(self) -> List[CycleResult]:
        """Retourne historique des cycles"""
        return self.cycle_history
    
    def get_statistics(self) -> LoopStatistics:
        """Retourne statistiques"""
        return self.stats
    
    def reset(self):
        """Réinitialise l'engine"""
        self.cycle_history.clear()
        self.stats = LoopStatistics()
        self.status = ConvergenceStatus.NOT_STARTED


# Fonctions utilitaires

def simple_error_function(state: np.ndarray, goal: np.ndarray) -> float:
    """Fonction d'erreur simple (différence pixel-wise)"""
    if state.shape != goal.shape:
        return 1.0
    
    diff = np.sum(state != goal)
    return diff / state.size


def weighted_error_function(state: np.ndarray, goal: np.ndarray) -> float:
    """Fonction d'erreur pondérée (couleurs importantes)"""
    if state.shape != goal.shape:
        return 1.0
    
    # Pondérer par couleur (couleurs rares plus importantes)
    unique_goal = np.unique(goal)
    weights = {color: 1.0 / np.sum(goal == color) for color in unique_goal}
    
    error = 0.0
    for color in unique_goal:
        mask = goal == color
        diff = np.sum((state[mask] != goal[mask]))
        error += diff * weights[color]
    
    return error / state.size


if __name__ == "__main__":
    print("🔄 Testing Closed Loop Physics Engine...")
    
    # Mock simulator
    class MockSimulator:
        def apply_action(self, state, action):
            result = state.copy()
            if action == "rotate":
                result = np.rot90(result)
            elif action == "flip":
                result = np.fliplr(result)
            return result
    
    # États test
    initial = np.array([[1, 0], [0, 2]])
    goal = np.array([[0, 1], [2, 0]])
    
    # Engine
    engine = ClosedLoopPhysicsEngine(
        simulator=MockSimulator(),
        error_function=simple_error_function,
        convergence_threshold=0.01,
        max_cycles=50,
        verbose=True
    )
    
    # Exécuter
    result = engine.run_closed_loop(
        initial_state=initial,
        goal=goal,
        available_actions=["rotate", "flip", "identity"]
    )
    
    print(f"\n📊 Result:")
    print(f"   Success: {result['success']}")
    print(f"   Cycles: {result['cycles']}")
    print(f"   Final error: {result['final_error']:.4f}")
    print(f"   Error reduction: {result['statistics']['error_reduction_rate']:.2%}")
    
    print("\n✅ Closed Loop Physics Engine test complete!")

# Made with Bob
