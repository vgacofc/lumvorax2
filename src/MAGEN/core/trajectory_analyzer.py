"""
CORRECTION C19: TRAJECTORY-BASED EARLY STOPPING
================================================

PROBLÈME IDENTIFIÉ (Session 64):
- Exploration continue même si trajectoire stagne/diverge
- Pas de détection régression linéaire sur error_history
- Gaspillage temps sur chemins morts

SOLUTION C19:
- Régression linéaire sur dernières N tentatives
- Stop si pente ≥ -0.01 (stagnation/divergence)
- Détection oscillation (variance haute, progrès nul)

FORMULE RÉGRESSION:
slope = Σ((x - x̄)(y - ȳ)) / Σ((x - x̄)²)
où x = index tentative, y = error

CRITÈRES STOP:
1. Stagnation: slope ≥ -0.01 (pas d'amélioration)
2. Divergence: slope > 0 (erreur augmente)
3. Oscillation: variance > seuil ET slope ≈ 0

INTÉGRATION:
- Appelé APRÈS chaque tentative exploration
- Retourne (should_stop: bool, reason: str, metrics: dict)
- Historique maintenu par action

AUTEUR: Session 64 - Analyse Forensique Comparative V26/V27/V28
DATE: 2026-06-13
"""

from typing import List, Tuple, Dict, Optional
from dataclasses import dataclass
import math

# V32: Import forensic logger pour traçabilité
from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from ..forensic.lumvorax_logger import LumVoraxLogger


@dataclass
class TrajectoryMetrics:
    """Métriques trajectoire exploration"""
    action_name: str
    attempts: int
    error_history: List[float]
    slope: float
    variance: float
    mean_error: float
    is_stagnating: bool
    is_diverging: bool
    is_oscillating: bool
    
    @property
    def should_stop(self) -> bool:
        """Arrêt recommandé?"""
        return self.is_stagnating or self.is_diverging or self.is_oscillating
    
    @property
    def stop_reason(self) -> str:
        """Raison arrêt"""
        if self.is_diverging:
            return "DIVERGENCE"
        elif self.is_oscillating:
            return "OSCILLATION"
        elif self.is_stagnating:
            return "STAGNATION"
        else:
            return "NONE"


class TrajectoryAnalyzer:
    """
    Analyseur trajectoires exploration avec early stopping.
    
    PRINCIPE:
    - Régression linéaire sur error_history
    - Détection stagnation/divergence/oscillation
    - Stop intelligent pour économiser budget
    
    MÉTRIQUES:
    - Pente régression (amélioration?)
    - Variance erreurs (stabilité?)
    - Nombre stops précoces
    - Budget économisé
    """
    
    def __init__(
        self,
        min_attempts_before_stop: int = 20,      # PHASE 4.7.1: ×4 (5→20)
        stagnation_threshold: float = -0.001,    # PHASE 4.7.1: ×10 plus tolérant (-0.01→-0.001)
        oscillation_variance_threshold: float = 0.05,
        oscillation_slope_threshold: float = 0.005,
        verbose: bool = False,
        forensic_logger: Optional['LumVoraxLogger'] = None
    ):
        """
        Args:
            min_attempts_before_stop: Tentatives min avant analyse (PHASE 4.7.1: augmenté ×4)
            stagnation_threshold: Pente min pour progrès (PHASE 4.7.1: ×10 plus tolérant)
            oscillation_variance_threshold: Variance max pour oscillation
            oscillation_slope_threshold: Pente max pour oscillation
            verbose: Logs détaillés
            forensic_logger: Logger forensique LumVorax (V32)
        """
        self.min_attempts_before_stop = min_attempts_before_stop
        self.stagnation_threshold = stagnation_threshold
        self.oscillation_variance_threshold = oscillation_variance_threshold
        self.oscillation_slope_threshold = oscillation_slope_threshold
        self.verbose = verbose
        self.forensic_logger = forensic_logger
        
        # Historiques par action
        self.trajectories: Dict[str, List[float]] = {}
        
        # Statistiques
        self.total_stops = 0
        self.stops_by_reason: Dict[str, int] = {
            "STAGNATION": 0,
            "DIVERGENCE": 0,
            "OSCILLATION": 0
        }
        self.budget_saved = 0
        
        # V32: Log initialisation
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type="c19_initialization",
                component="trajectory_analyzer",
                operation="init",
                data={
                    'min_attempts_before_stop': min_attempts_before_stop,
                    'stagnation_threshold': stagnation_threshold,
                    'oscillation_variance_threshold': oscillation_variance_threshold,
                    'oscillation_slope_threshold': oscillation_slope_threshold,
                    'verbose': verbose
                }
            )
        
    def reset_action(self, action_name: str) -> None:
        """Réinitialiser historique action"""
        if action_name in self.trajectories:
            del self.trajectories[action_name]
    
    def reset_all(self) -> None:
        """Réinitialiser tous historiques"""
        self.trajectories.clear()
    
    def add_attempt(self, action_name: str, error: float) -> None:
        """
        Ajouter tentative à historique.
        
        Args:
            action_name: Nom action
            error: Erreur mesurée [0-1]
        """
        if action_name not in self.trajectories:
            self.trajectories[action_name] = []
        
        self.trajectories[action_name].append(error)
        
        if self.verbose:
            attempts = len(self.trajectories[action_name])
            print(f"[C19 TRAJECTORY] {action_name}: attempt #{attempts}, error={error:.4f}")
    
    def compute_linear_regression(self, values: List[float]) -> Tuple[float, float]:
        """
        Calculer régression linéaire.
        
        Args:
            values: Liste valeurs y
        
        Returns:
            (slope, intercept)
        """
        n = len(values)
        if n < 2:
            return 0.0, values[0] if n == 1 else 0.0
        
        # x = indices [0, 1, 2, ...]
        x_mean = (n - 1) / 2.0
        y_mean = sum(values) / n
        
        # Calcul pente
        numerator = sum((i - x_mean) * (y - y_mean) for i, y in enumerate(values))
        denominator = sum((i - x_mean) ** 2 for i in range(n))
        
        slope = numerator / denominator if denominator > 0 else 0.0
        intercept = y_mean - slope * x_mean
        
        return slope, intercept
    
    def compute_variance(self, values: List[float]) -> float:
        """
        Calculer variance.
        
        Args:
            values: Liste valeurs
        
        Returns:
            Variance
        """
        if len(values) < 2:
            return 0.0
        
        mean = sum(values) / len(values)
        variance = sum((x - mean) ** 2 for x in values) / len(values)
        
        return variance
    
    def analyze_trajectory(self, action_name: str) -> Optional[TrajectoryMetrics]:
        """
        Analyser trajectoire action.
        
        Args:
            action_name: Nom action
        
        Returns:
            TrajectoryMetrics ou None si pas assez données
        """
        if action_name not in self.trajectories:
            return None
        
        error_history = self.trajectories[action_name]
        attempts = len(error_history)
        
        # Pas assez de données
        if attempts < self.min_attempts_before_stop:
            return None
        
        # Calculer métriques
        slope, _ = self.compute_linear_regression(error_history)
        variance = self.compute_variance(error_history)
        mean_error = sum(error_history) / attempts
        
        # Détection stagnation
        is_stagnating = slope >= self.stagnation_threshold
        
        # Détection divergence
        is_diverging = slope > 0
        
        # Détection oscillation
        is_oscillating = (
            variance > self.oscillation_variance_threshold and
            abs(slope) < self.oscillation_slope_threshold
        )
        
        metrics = TrajectoryMetrics(
            action_name=action_name,
            attempts=attempts,
            error_history=error_history.copy(),
            slope=slope,
            variance=variance,
            mean_error=mean_error,
            is_stagnating=is_stagnating,
            is_diverging=is_diverging,
            is_oscillating=is_oscillating
        )
        
        if self.verbose and metrics.should_stop:
            print(f"[C19 TRAJECTORY] STOP RECOMMANDÉ - {action_name}")
            print(f"  Raison: {metrics.stop_reason}")
            print(f"  Tentatives: {attempts}")
            print(f"  Pente: {slope:.6f} (seuil: {self.stagnation_threshold})")
            print(f"  Variance: {variance:.6f}")
            print(f"  Erreur moyenne: {mean_error:.4f}")
        
        return metrics
    
    def should_stop_exploration(
        self,
        action_name: str,
        current_error: float,
        remaining_budget: int
    ) -> Tuple[bool, str, Optional[TrajectoryMetrics]]:
        """
        Décider si arrêt exploration.
        
        Args:
            action_name: Nom action
            current_error: Erreur actuelle
            remaining_budget: Budget restant
        
        Returns:
            (should_stop, reason, metrics)
        """
        # V32: Log entrée
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type="c19_should_stop",
                component="trajectory_analyzer",
                operation="should_stop_check",
                data={
                    'action_name': action_name,
                    'current_error': current_error,
                    'remaining_budget': remaining_budget,
                    'num_attempts': len(self.trajectories.get(action_name, []))
                }
            )
        
        # Ajouter tentative
        self.add_attempt(action_name, current_error)
        
        # Analyser trajectoire
        metrics = self.analyze_trajectory(action_name)
        
        # Pas assez de données
        if metrics is None:
            # V32: Log données insuffisantes
            if self.forensic_logger:
                self.forensic_logger.log_event(
                    event_type="c19_should_stop",
                    component="trajectory_analyzer",
                    operation="should_stop_insufficient_data",
                    data={'action_name': action_name, 'reason': 'INSUFFICIENT_DATA'}
                )
            return False, "INSUFFICIENT_DATA", None
        
        # Décision stop
        if metrics.should_stop:
            self.total_stops += 1
            self.stops_by_reason[metrics.stop_reason] += 1
            self.budget_saved += remaining_budget
            
            # V32: Log arrêt
            if self.forensic_logger:
                self.forensic_logger.log_event(
                    event_type="c19_should_stop",
                    component="trajectory_analyzer",
                    operation="should_stop_approved",
                    data={
                        'action_name': action_name,
                        'reason': metrics.stop_reason,
                        'slope': metrics.slope,
                        'variance': metrics.variance,
                        'mean_error': metrics.mean_error,
                        'attempts': metrics.attempts,
                        'budget_saved': remaining_budget
                    }
                )
            
            return True, metrics.stop_reason, metrics
        
        # V32: Log continuation
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type="c19_should_stop",
                component="trajectory_analyzer",
                operation="should_stop_continue",
                data={
                    'action_name': action_name,
                    'reason': 'CONTINUE',
                    'slope': metrics.slope,
                    'variance': metrics.variance,
                    'attempts': metrics.attempts
                }
            )
        
        return False, "CONTINUE", metrics
    
    def get_statistics(self) -> Dict[str, int]:
        """
        Statistiques arrêts précoces.
        
        Returns:
            Dict avec métriques clés
        """
        return {
            "total_stops": self.total_stops,
            "stops_stagnation": self.stops_by_reason["STAGNATION"],
            "stops_divergence": self.stops_by_reason["DIVERGENCE"],
            "stops_oscillation": self.stops_by_reason["OSCILLATION"],
            "budget_saved": self.budget_saved,
            "active_trajectories": len(self.trajectories)
        }
    
    def print_statistics(self) -> None:
        """Afficher statistiques"""
        stats = self.get_statistics()
        
        print("\n" + "="*60)
        print("C19 TRAJECTORY ANALYZER - STATISTIQUES")
        print("="*60)
        print(f"Total arrêts précoces: {stats['total_stops']}")
        print(f"  - Stagnation: {stats['stops_stagnation']}")
        print(f"  - Divergence: {stats['stops_divergence']}")
        print(f"  - Oscillation: {stats['stops_oscillation']}")
        print(f"Budget économisé: {stats['budget_saved']}")
        print(f"Trajectoires actives: {stats['active_trajectories']}")
        print("="*60 + "\n")


# Test unitaire
if __name__ == "__main__":
    print("TEST C19 - TRAJECTORY ANALYZER")
    print("="*60)
    
    analyzer = TrajectoryAnalyzer(
        min_attempts_before_stop=5,
        stagnation_threshold=-0.01,
        verbose=True
    )
    
    # Test 1: Trajectoire stagnante
    print("\n### TEST 1: STAGNATION ###")
    action = "test_stagnation"
    errors = [0.5, 0.49, 0.48, 0.48, 0.47, 0.47, 0.47]
    
    for i, error in enumerate(errors):
        should_stop, reason, metrics = analyzer.should_stop_exploration(
            action, error, remaining_budget=10
        )
        print(f"Tentative {i+1}: error={error:.2f}, stop={should_stop}, reason={reason}")
        if should_stop:
            break
    
    # Test 2: Trajectoire divergente
    print("\n### TEST 2: DIVERGENCE ###")
    action = "test_divergence"
    analyzer.reset_action(action)
    errors = [0.3, 0.35, 0.4, 0.45, 0.5, 0.55]
    
    for i, error in enumerate(errors):
        should_stop, reason, metrics = analyzer.should_stop_exploration(
            action, error, remaining_budget=10
        )
        print(f"Tentative {i+1}: error={error:.2f}, stop={should_stop}, reason={reason}")
        if should_stop:
            break
    
    # Test 3: Trajectoire oscillante
    print("\n### TEST 3: OSCILLATION ###")
    action = "test_oscillation"
    analyzer.reset_action(action)
    errors = [0.5, 0.3, 0.6, 0.2, 0.7, 0.4, 0.5]
    
    for i, error in enumerate(errors):
        should_stop, reason, metrics = analyzer.should_stop_exploration(
            action, error, remaining_budget=10
        )
        print(f"Tentative {i+1}: error={error:.2f}, stop={should_stop}, reason={reason}")
        if should_stop:
            break
    
    # Test 4: Trajectoire saine (amélioration continue)
    print("\n### TEST 4: AMÉLIORATION CONTINUE ###")
    action = "test_healthy"
    analyzer.reset_action(action)
    errors = [0.5, 0.4, 0.3, 0.2, 0.1, 0.05]
    
    for i, error in enumerate(errors):
        should_stop, reason, metrics = analyzer.should_stop_exploration(
            action, error, remaining_budget=10
        )
        print(f"Tentative {i+1}: error={error:.2f}, stop={should_stop}, reason={reason}")
        if should_stop:
            break
    
    # Statistiques
    analyzer.print_statistics()
    
    print("\n✅ Test C19 terminé")

# Made with Bob
