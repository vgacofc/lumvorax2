"""
MAGEN V2 - Hyperparameter Optimization System
Phase 4.1: Grid Search + Bayesian Optimization

Optimise les hyperparamètres critiques pour maximiser le taux de victoire.
Baseline: 2% (1/50 victoires) → Objectif: 20%+

Hyperparamètres ciblés:
- buffer_capacity: Taille du replay buffer
- batch_size: Taille des batches d'entraînement
- learning_rate: Taux d'apprentissage
- gamma: Facteur de discount
- epsilon_start/end/decay: Exploration vs exploitation
- tau: Soft update target network
- reward_scale: Échelle des récompenses
- curriculum_threshold: Seuil de progression curriculum

Références:
- Bergstra & Bengio (2012): Random Search for Hyper-Parameter Optimization
- Snoek et al. (2012): Practical Bayesian Optimization
- Schaul et al. (2015): Prioritized Experience Replay
"""

import numpy as np
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple, Any, Optional
from dataclasses import dataclass, asdict
from itertools import product
import logging

# Configuration logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@dataclass
class HyperparameterConfig:
    """Configuration des hyperparamètres à optimiser"""
    # Replay Buffer
    buffer_capacity: int = 10000
    batch_size: int = 32
    
    # Learning
    learning_rate: float = 0.001
    gamma: float = 0.99
    tau: float = 0.005
    
    # Exploration
    epsilon_start: float = 1.0
    epsilon_end: float = 0.01
    epsilon_decay: float = 0.995
    
    # Rewards
    reward_scale: float = 1.0
    reward_step: float = -0.01
    reward_goal: float = 100.0
    reward_invalid: float = -1.0
    
    # Curriculum Learning
    curriculum_threshold: float = 0.7
    curriculum_window: int = 100
    
    # Training
    update_frequency: int = 4
    target_update_frequency: int = 100
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertit en dictionnaire"""
        return asdict(self)
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'HyperparameterConfig':
        """Crée depuis un dictionnaire"""
        return cls(**data)


@dataclass
class OptimizationResult:
    """Résultat d'une évaluation d'hyperparamètres"""
    config: HyperparameterConfig
    win_rate: float
    avg_reward: float
    avg_steps: float
    episodes: int
    training_time: float
    convergence_episode: Optional[int] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertit en dictionnaire"""
        return {
            'config': self.config.to_dict(),
            'win_rate': self.win_rate,
            'avg_reward': self.avg_reward,
            'avg_steps': self.avg_steps,
            'episodes': self.episodes,
            'training_time': self.training_time,
            'convergence_episode': self.convergence_episode
        }


class GridSearchOptimizer:
    """
    Optimisation par Grid Search exhaustif
    
    Teste toutes les combinaisons d'hyperparamètres dans une grille définie.
    Avantages: Exhaustif, reproductible
    Inconvénients: Coûteux en calcul
    """
    
    def __init__(self, param_grid: Dict[str, List[Any]]):
        """
        Args:
            param_grid: Grille de paramètres à tester
                       Ex: {'learning_rate': [0.001, 0.01], 'batch_size': [32, 64]}
        """
        self.param_grid = param_grid
        self.results: List[OptimizationResult] = []
        
    def generate_configs(self) -> List[HyperparameterConfig]:
        """Génère toutes les configurations possibles"""
        # Extraire les noms et valeurs des paramètres
        param_names = list(self.param_grid.keys())
        param_values = [self.param_grid[name] for name in param_names]
        
        # Générer toutes les combinaisons
        configs = []
        for values in product(*param_values):
            # Créer un dictionnaire de paramètres
            params = dict(zip(param_names, values))
            
            # Créer la configuration (avec valeurs par défaut pour les autres)
            config = HyperparameterConfig(**params)
            configs.append(config)
        
        logger.info(f"Grid Search: {len(configs)} configurations générées")
        return configs
    
    def optimize(
        self,
        evaluate_fn,
        n_episodes: int = 100,
        save_path: Optional[Path] = None
    ) -> Tuple[HyperparameterConfig, OptimizationResult]:
        """
        Optimise les hyperparamètres par Grid Search
        
        Args:
            evaluate_fn: Fonction d'évaluation (config) -> OptimizationResult
            n_episodes: Nombre d'épisodes par configuration
            save_path: Chemin pour sauvegarder les résultats
            
        Returns:
            Meilleure configuration et son résultat
        """
        configs = self.generate_configs()
        total = len(configs)
        
        logger.info(f"Démarrage Grid Search: {total} configurations à tester")
        start_time = time.time()
        
        for i, config in enumerate(configs, 1):
            logger.info(f"\n{'='*60}")
            logger.info(f"Configuration {i}/{total} ({i/total*100:.1f}%)")
            logger.info(f"{'='*60}")
            
            # Évaluer la configuration
            result = evaluate_fn(config, n_episodes)
            self.results.append(result)
            
            # Afficher les résultats
            logger.info(f"Win Rate: {result.win_rate*100:.2f}%")
            logger.info(f"Avg Reward: {result.avg_reward:.2f}")
            logger.info(f"Avg Steps: {result.avg_steps:.1f}")
            logger.info(f"Training Time: {result.training_time:.2f}s")
            
            # Sauvegarder les résultats intermédiaires
            if save_path:
                self._save_results(save_path)
        
        # Trouver la meilleure configuration
        best_result = max(self.results, key=lambda r: r.win_rate)
        best_config = best_result.config
        
        total_time = time.time() - start_time
        logger.info(f"\n{'='*60}")
        logger.info(f"Grid Search terminé en {total_time:.2f}s")
        logger.info(f"Meilleur Win Rate: {best_result.win_rate*100:.2f}%")
        logger.info(f"{'='*60}")
        
        return best_config, best_result
    
    def _save_results(self, save_path: Path):
        """Sauvegarde les résultats"""
        save_path.parent.mkdir(parents=True, exist_ok=True)
        
        data = {
            'results': [r.to_dict() for r in self.results],
            'best_config': max(self.results, key=lambda r: r.win_rate).config.to_dict(),
            'timestamp': time.time()
        }
        
        with open(save_path, 'w') as f:
            json.dump(data, f, indent=2)


class BayesianOptimizer:
    """
    Optimisation Bayésienne avec Gaussian Process
    
    Utilise un modèle probabiliste pour guider la recherche vers les régions
    prometteuses de l'espace des hyperparamètres.
    
    Avantages: Efficace, adaptatif
    Inconvénients: Plus complexe, nécessite plus d'épisodes par config
    
    Références:
    - Snoek et al. (2012): Practical Bayesian Optimization
    - Shahriari et al. (2016): Taking the Human Out of the Loop
    """
    
    def __init__(
        self,
        param_bounds: Dict[str, Tuple[float, float]],
        n_initial_points: int = 10,
        acquisition: str = 'ei'  # 'ei', 'ucb', 'poi'
    ):
        """
        Args:
            param_bounds: Bornes des paramètres continus
                         Ex: {'learning_rate': (0.0001, 0.01)}
            n_initial_points: Nombre de points d'exploration initiale
            acquisition: Fonction d'acquisition ('ei', 'ucb', 'poi')
        """
        self.param_bounds = param_bounds
        self.n_initial_points = n_initial_points
        self.acquisition = acquisition
        self.results: List[OptimizationResult] = []
        
        # Historique pour le Gaussian Process
        self.X_observed: List[np.ndarray] = []
        self.y_observed: List[float] = []
        
    def _sample_random_config(self) -> HyperparameterConfig:
        """Échantillonne une configuration aléatoire"""
        params = {}
        for name, (low, high) in self.param_bounds.items():
            if isinstance(low, int) and isinstance(high, int):
                params[name] = np.random.randint(low, high + 1)
            else:
                params[name] = np.random.uniform(low, high)
        
        return HyperparameterConfig(**params)
    
    def _config_to_vector(self, config: HyperparameterConfig) -> np.ndarray:
        """Convertit une configuration en vecteur"""
        values = []
        for name in sorted(self.param_bounds.keys()):
            values.append(getattr(config, name))
        return np.array(values)
    
    def _vector_to_config(self, vector: np.ndarray) -> HyperparameterConfig:
        """Convertit un vecteur en configuration"""
        params = {}
        for i, name in enumerate(sorted(self.param_bounds.keys())):
            value = vector[i]
            # Arrondir si entier
            low, high = self.param_bounds[name]
            if isinstance(low, int) and isinstance(high, int):
                value = int(round(value))
            params[name] = value
        
        return HyperparameterConfig(**params)
    
    def _gaussian_process_predict(self, X: np.ndarray) -> Tuple[float, float]:
        """
        Prédiction simple du Gaussian Process
        
        Note: Implémentation simplifiée. Pour production, utiliser
        scikit-optimize ou GPyOpt.
        """
        if len(self.X_observed) == 0:
            return 0.0, 1.0
        
        # Calcul de la distance aux points observés
        X_obs = np.array(self.X_observed)
        distances = np.linalg.norm(X_obs - X, axis=1)
        
        # Kernel RBF simplifié
        length_scale = 1.0
        kernel_values = np.exp(-distances**2 / (2 * length_scale**2))
        
        # Moyenne pondérée
        weights = kernel_values / (kernel_values.sum() + 1e-10)
        mean = np.dot(weights, self.y_observed)
        
        # Variance (incertitude)
        variance = 1.0 - kernel_values.max()
        
        return mean, variance
    
    def _acquisition_function(self, X: np.ndarray) -> float:
        """
        Fonction d'acquisition pour guider la recherche
        
        Expected Improvement (EI): Balance exploration/exploitation
        """
        mean, variance = self._gaussian_process_predict(X)
        std = np.sqrt(variance)
        
        if len(self.y_observed) == 0:
            return 0.0
        
        best_y = max(self.y_observed)
        
        if self.acquisition == 'ei':
            # Expected Improvement
            z = (mean - best_y) / (std + 1e-10)
            ei = (mean - best_y) * self._normal_cdf(z) + std * self._normal_pdf(z)
            return ei
        
        elif self.acquisition == 'ucb':
            # Upper Confidence Bound
            kappa = 2.0
            return mean + kappa * std
        
        else:  # poi
            # Probability of Improvement
            z = (mean - best_y) / (std + 1e-10)
            return self._normal_cdf(z)
    
    @staticmethod
    def _normal_cdf(x: float) -> float:
        """CDF de la loi normale standard"""
        return 0.5 * (1.0 + np.tanh(x / np.sqrt(2.0)))
    
    @staticmethod
    def _normal_pdf(x: float) -> float:
        """PDF de la loi normale standard"""
        return np.exp(-0.5 * x**2) / np.sqrt(2.0 * np.pi)
    
    def _suggest_next_config(self) -> HyperparameterConfig:
        """Suggère la prochaine configuration à tester"""
        # Échantillonner plusieurs candidats
        n_candidates = 100
        best_acquisition = -np.inf
        best_config = self._sample_random_config()  # Initialisation par défaut
        
        for _ in range(n_candidates):
            config = self._sample_random_config()
            X = self._config_to_vector(config)
            acquisition_value = self._acquisition_function(X)
            
            if acquisition_value > best_acquisition:
                best_acquisition = acquisition_value
                best_config = config
        
        return best_config
    
    def optimize(
        self,
        evaluate_fn,
        n_iterations: int = 50,
        n_episodes: int = 100,
        save_path: Optional[Path] = None
    ) -> Tuple[HyperparameterConfig, OptimizationResult]:
        """
        Optimise les hyperparamètres par Bayesian Optimization
        
        Args:
            evaluate_fn: Fonction d'évaluation (config) -> OptimizationResult
            n_iterations: Nombre d'itérations d'optimisation
            n_episodes: Nombre d'épisodes par configuration
            save_path: Chemin pour sauvegarder les résultats
            
        Returns:
            Meilleure configuration et son résultat
        """
        logger.info(f"Démarrage Bayesian Optimization: {n_iterations} itérations")
        start_time = time.time()
        
        for i in range(n_iterations):
            logger.info(f"\n{'='*60}")
            logger.info(f"Itération {i+1}/{n_iterations} ({(i+1)/n_iterations*100:.1f}%)")
            logger.info(f"{'='*60}")
            
            # Exploration initiale aléatoire
            if i < self.n_initial_points:
                config = self._sample_random_config()
                logger.info("Phase d'exploration aléatoire")
            else:
                config = self._suggest_next_config()
                logger.info("Phase d'exploitation guidée")
            
            # Évaluer la configuration
            result = evaluate_fn(config, n_episodes)
            self.results.append(result)
            
            # Mettre à jour le Gaussian Process
            X = self._config_to_vector(config)
            self.X_observed.append(X)
            self.y_observed.append(result.win_rate)
            
            # Afficher les résultats
            logger.info(f"Win Rate: {result.win_rate*100:.2f}%")
            logger.info(f"Avg Reward: {result.avg_reward:.2f}")
            logger.info(f"Avg Steps: {result.avg_steps:.1f}")
            
            # Sauvegarder les résultats intermédiaires
            if save_path:
                self._save_results(save_path)
        
        # Trouver la meilleure configuration
        best_result = max(self.results, key=lambda r: r.win_rate)
        best_config = best_result.config
        
        total_time = time.time() - start_time
        logger.info(f"\n{'='*60}")
        logger.info(f"Bayesian Optimization terminé en {total_time:.2f}s")
        logger.info(f"Meilleur Win Rate: {best_result.win_rate*100:.2f}%")
        logger.info(f"{'='*60}")
        
        return best_config, best_result
    
    def _save_results(self, save_path: Path):
        """Sauvegarde les résultats"""
        save_path.parent.mkdir(parents=True, exist_ok=True)
        
        data = {
            'results': [r.to_dict() for r in self.results],
            'best_config': max(self.results, key=lambda r: r.win_rate).config.to_dict(),
            'X_observed': [x.tolist() for x in self.X_observed],
            'y_observed': self.y_observed,
            'timestamp': time.time()
        }
        
        with open(save_path, 'w') as f:
            json.dump(data, f, indent=2)


def create_default_param_grid() -> Dict[str, List[Any]]:
    """Crée une grille de paramètres par défaut pour Grid Search"""
    return {
        'learning_rate': [0.0001, 0.001, 0.01],
        'batch_size': [16, 32, 64],
        'gamma': [0.95, 0.99, 0.995],
        'epsilon_decay': [0.99, 0.995, 0.999],
        'buffer_capacity': [5000, 10000, 20000]
    }


def create_default_param_bounds() -> Dict[str, Tuple[float, float]]:
    """Crée des bornes de paramètres par défaut pour Bayesian Optimization"""
    return {
        'learning_rate': (0.0001, 0.01),
        'batch_size': (16, 128),
        'gamma': (0.9, 0.999),
        'epsilon_decay': (0.99, 0.9999),
        'buffer_capacity': (1000, 50000),
        'tau': (0.001, 0.01),
        'reward_scale': (0.1, 10.0)
    }


if __name__ == '__main__':
    # Test simple
    print("MAGEN V2 - Hyperparameter Optimization System")
    print("=" * 60)
    
    # Test Grid Search
    param_grid = {
        'learning_rate': [0.001, 0.01],
        'batch_size': [32, 64]
    }
    
    grid_search = GridSearchOptimizer(param_grid)
    configs = grid_search.generate_configs()
    print(f"\nGrid Search: {len(configs)} configurations générées")
    
    # Test Bayesian Optimization
    param_bounds = {
        'learning_rate': (0.0001, 0.01),
        'batch_size': (16, 128)
    }
    
    bayesian_opt = BayesianOptimizer(param_bounds)
    config = bayesian_opt._sample_random_config()
    print(f"\nBayesian Optimization: Configuration aléatoire générée")
    print(f"Learning Rate: {config.learning_rate}")
    print(f"Batch Size: {config.batch_size}")
    
    print("\n✅ Tests réussis!")

# Made with Bob
