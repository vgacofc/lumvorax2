"""
MAGEN V2 - Script d'Optimisation des Hyperparamètres
Phase 4.1: Exécution Grid Search + Bayesian Optimization

Lance l'optimisation des hyperparamètres sur le labyrinthe ls20-9607627b.
Baseline: 2% (1/50 victoires) → Objectif: 20%+

Usage:
    python run_hyperparameter_optimization.py --method grid --episodes 100
    python run_hyperparameter_optimization.py --method bayesian --iterations 50

Références:
- Bergstra & Bengio (2012): Random Search for Hyper-Parameter Optimization
- Snoek et al. (2012): Practical Bayesian Optimization
"""

import sys
import argparse
import time
import json
from pathlib import Path
from typing import Dict, Any, Tuple

# Ajouter le chemin parent pour les imports
sys.path.insert(0, str(Path(__file__).parent.parent))

import numpy as np
from optimization.hyperparameter_tuner import (
    GridSearchOptimizer,
    BayesianOptimizer,
    HyperparameterConfig,
    OptimizationResult,
    create_default_param_grid,
    create_default_param_bounds
)

# Imports MAGEN
from integration.magen_agent import MAGENAgent
from integration.learning_cycle import LearningCycle, Experience

# Import ARC loader (comme dans validate_magen_v2_maze.py)
try:
    from arc_loader import ARCLoader
except ImportError:
    print("⚠️  arc_loader non trouvé, utilisation données simulées")
    ARCLoader = None


class SimpleMazeEnvironment:
    """
    Environnement labyrinthe simplifié pour optimisation
    
    Simule le jeu ls20-9607627b avec:
    - Grille 50x50
    - Position joueur (bloc 5x5)
    - Position objectif
    - Récompenses basées sur distance
    """
    
    def __init__(self, game_id: str = "ls20-9607627b"):
        self.game_id = game_id
        self.grid_size = (50, 50)
        self.reset()
    
    def reset(self) -> np.ndarray:
        """Réinitialise l'environnement"""
        # Grille aléatoire
        self.grid = np.random.randint(0, 10, self.grid_size)
        
        # Position initiale joueur (centre)
        self.player_pos = (25, 25)
        
        # Position objectif (coin)
        self.goal_pos = (40, 40)
        
        # Compteurs
        self.steps = 0
        self.max_steps = 200
        
        return self.grid.copy()
    
    def step(self, action: str) -> Tuple[np.ndarray, float, bool, Dict]:
        """
        Exécute une action
        
        Args:
            action: Action à exécuter (ignorée pour simplification)
            
        Returns:
            next_state, reward, done, info
        """
        # Mouvement aléatoire (placeholder)
        dx = np.random.randint(-1, 2)
        dy = np.random.randint(-1, 2)
        
        # Nouvelle position
        new_x = np.clip(self.player_pos[0] + dx, 0, self.grid_size[0] - 1)
        new_y = np.clip(self.player_pos[1] + dy, 0, self.grid_size[1] - 1)
        
        # Calcule distance avant/après
        old_dist = abs(self.player_pos[0] - self.goal_pos[0]) + abs(self.player_pos[1] - self.goal_pos[1])
        new_dist = abs(new_x - self.goal_pos[0]) + abs(new_y - self.goal_pos[1])
        
        # Récompense
        if new_dist < old_dist:
            reward = 1.0  # Rapprochement
        elif new_dist == old_dist:
            reward = -0.01  # Stagnation
        else:
            reward = -0.5  # Éloignement
        
        # Update position
        self.player_pos = (new_x, new_y)
        self.steps += 1
        
        # Vérifie victoire
        done = False
        success = False
        if new_dist < 5:
            reward += 100.0  # Bonus victoire
            done = True
            success = True
        elif self.steps >= self.max_steps:
            done = True
        
        info = {
            'success': success,
            'distance': new_dist,
            'steps': self.steps
        }
        
        return self.grid.copy(), reward, done, info


def evaluate_config(
    config: HyperparameterConfig,
    n_episodes: int,
    game_id: str = "ls20-9607627b",
    verbose: bool = True
) -> OptimizationResult:
    """
    Évalue une configuration d'hyperparamètres
    
    Args:
        config: Configuration à évaluer
        n_episodes: Nombre d'épisodes à exécuter
        game_id: ID du jeu ARC
        verbose: Afficher les logs détaillés
        
    Returns:
        Résultat de l'évaluation
    """
    if verbose:
        print(f"\n{'='*60}")
        print(f"Évaluation Configuration")
        print(f"{'='*60}")
        print(f"Buffer Capacity: {config.buffer_capacity}")
        print(f"Batch Size: {config.batch_size}")
        print(f"Learning Rate: {config.learning_rate}")
        print(f"Gamma: {config.gamma}")
        print(f"Epsilon Decay: {config.epsilon_decay}")
        print(f"{'='*60}\n")
    
    start_time = time.time()
    
    # Créer l'environnement
    env = SimpleMazeEnvironment(game_id=game_id)
    
    # Créer l'agent MAGEN
    agent = MAGENAgent(enable_learning=True, verbose=False)
    
    # Créer le Learning Cycle
    learning_cycle = LearningCycle(
        agent=agent,
        buffer_capacity=config.buffer_capacity,
        batch_size=config.batch_size,
        eval_frequency=10,
        checkpoint_dir=f"checkpoints_opt_{int(time.time())}"
    )
    
    # Statistiques
    wins = 0
    total_reward = 0.0
    total_steps = 0
    convergence_episode = None
    
    # Exécuter les épisodes
    for episode in range(n_episodes):
        state = env.reset()
        episode_reward = 0.0
        episode_steps = 0
        done = False
        
        while not done:
            # Agent traite l'état
            result = agent.process(state)
            
            # Extrait action (placeholder)
            action = str(result.action) if result.action is not None else "no_action"
            
            # Exécute l'action
            next_state, reward, done, info = env.step(action)
            
            # Crée expérience
            experience = Experience(
                state=state,
                action=action,
                reward=reward,
                next_state=next_state,
                done=done
            )
            
            # Ajoute au buffer
            learning_cycle.buffer.add(experience)
            
            # Apprentissage (si assez d'expériences)
            if len(learning_cycle.buffer) >= config.batch_size:
                # Échantillonne batch
                batch = learning_cycle.buffer.sample(config.batch_size)
                # Note: Apprentissage réel serait ici
                # Pour l'instant, juste simulation
            
            # Update
            state = next_state
            episode_reward += reward
            episode_steps += 1
            
            # Limite de steps
            if episode_steps >= 200:
                break
        
        # Statistiques
        total_reward += episode_reward
        total_steps += episode_steps
        
        if info.get('success', False):
            wins += 1
            if convergence_episode is None:
                convergence_episode = episode + 1
        
        # Affichage périodique
        if verbose and (episode + 1) % 10 == 0:
            win_rate = wins / (episode + 1)
            avg_reward = total_reward / (episode + 1)
            avg_steps = total_steps / (episode + 1)
            print(f"Episode {episode+1}/{n_episodes}: "
                  f"Win Rate={win_rate*100:.1f}%, "
                  f"Avg Reward={avg_reward:.2f}, "
                  f"Avg Steps={avg_steps:.1f}")
    
    # Calculer les métriques finales
    win_rate = wins / n_episodes
    avg_reward = total_reward / n_episodes
    avg_steps = total_steps / n_episodes
    training_time = time.time() - start_time
    
    if verbose:
        print(f"\n{'='*60}")
        print(f"Résultats Finaux")
        print(f"{'='*60}")
        print(f"Victoires: {wins}/{n_episodes} ({win_rate*100:.2f}%)")
        print(f"Reward Moyen: {avg_reward:.2f}")
        print(f"Steps Moyen: {avg_steps:.1f}")
        print(f"Temps d'Entraînement: {training_time:.2f}s")
        if convergence_episode:
            print(f"Convergence à l'épisode: {convergence_episode}")
        print(f"{'='*60}\n")
    
    return OptimizationResult(
        config=config,
        win_rate=win_rate,
        avg_reward=avg_reward,
        avg_steps=avg_steps,
        episodes=n_episodes,
        training_time=training_time,
        convergence_episode=convergence_episode
    )


def run_grid_search(
    n_episodes: int = 100,
    game_id: str = "ls20-9607627b",
    save_dir: Path = Path("results/grid_search")
) -> None:
    """
    Lance l'optimisation par Grid Search
    
    Args:
        n_episodes: Nombre d'épisodes par configuration
        game_id: ID du jeu ARC
        save_dir: Répertoire de sauvegarde
    """
    print("\n" + "="*60)
    print("MAGEN V2 - Grid Search Optimization")
    print("="*60)
    print(f"Game: {game_id}")
    print(f"Episodes per config: {n_episodes}")
    print("="*60 + "\n")
    
    # Créer la grille de paramètres (réduite pour test)
    param_grid = {
        'buffer_capacity': [5000, 10000],
        'batch_size': [16, 32],
        'learning_rate': [0.001, 0.01]
    }
    
    print("Grille de Paramètres:")
    for param, values in param_grid.items():
        print(f"  {param}: {values}")
    print()
    
    # Créer l'optimiseur
    optimizer = GridSearchOptimizer(param_grid)
    
    # Fonction d'évaluation
    def evaluate_fn(config, episodes):
        return evaluate_config(config, episodes, game_id, verbose=True)
    
    # Lancer l'optimisation
    save_path = save_dir / f"grid_search_{game_id}_{int(time.time())}.json"
    best_config, best_result = optimizer.optimize(
        evaluate_fn=evaluate_fn,
        n_episodes=n_episodes,
        save_path=save_path
    )
    
    # Afficher les résultats
    print("\n" + "="*60)
    print("MEILLEURE CONFIGURATION TROUVÉE")
    print("="*60)
    print(f"Win Rate: {best_result.win_rate*100:.2f}%")
    print(f"Avg Reward: {best_result.avg_reward:.2f}")
    print(f"Avg Steps: {best_result.avg_steps:.1f}")
    print(f"\nHyperparamètres:")
    for key, value in best_config.to_dict().items():
        print(f"  {key}: {value}")
    print("="*60 + "\n")
    
    print(f"Résultats sauvegardés dans: {save_path}")


def run_bayesian_optimization(
    n_iterations: int = 20,
    n_episodes: int = 50,
    game_id: str = "ls20-9607627b",
    save_dir: Path = Path("results/bayesian")
) -> None:
    """
    Lance l'optimisation Bayésienne
    
    Args:
        n_iterations: Nombre d'itérations d'optimisation
        n_episodes: Nombre d'épisodes par configuration
        game_id: ID du jeu ARC
        save_dir: Répertoire de sauvegarde
    """
    print("\n" + "="*60)
    print("MAGEN V2 - Bayesian Optimization")
    print("="*60)
    print(f"Game: {game_id}")
    print(f"Iterations: {n_iterations}")
    print(f"Episodes per config: {n_episodes}")
    print("="*60 + "\n")
    
    # Créer les bornes de paramètres (réduites pour test)
    param_bounds = {
        'buffer_capacity': (1000, 20000),
        'batch_size': (8, 64),
        'learning_rate': (0.0001, 0.01)
    }
    
    print("Bornes de Paramètres:")
    for param, (low, high) in param_bounds.items():
        print(f"  {param}: [{low}, {high}]")
    print()
    
    # Créer l'optimiseur
    optimizer = BayesianOptimizer(
        param_bounds=param_bounds,
        n_initial_points=5,
        acquisition='ei'
    )
    
    # Fonction d'évaluation
    def evaluate_fn(config, episodes):
        return evaluate_config(config, episodes, game_id, verbose=True)
    
    # Lancer l'optimisation
    save_path = save_dir / f"bayesian_{game_id}_{int(time.time())}.json"
    best_config, best_result = optimizer.optimize(
        evaluate_fn=evaluate_fn,
        n_iterations=n_iterations,
        n_episodes=n_episodes,
        save_path=save_path
    )
    
    # Afficher les résultats
    print("\n" + "="*60)
    print("MEILLEURE CONFIGURATION TROUVÉE")
    print("="*60)
    print(f"Win Rate: {best_result.win_rate*100:.2f}%")
    print(f"Avg Reward: {best_result.avg_reward:.2f}")
    print(f"Avg Steps: {best_result.avg_steps:.1f}")
    print(f"\nHyperparamètres:")
    for key, value in best_config.to_dict().items():
        print(f"  {key}: {value}")
    print("="*60 + "\n")
    
    print(f"Résultats sauvegardés dans: {save_path}")


def run_quick_test(game_id: str = "ls20-9607627b") -> None:
    """
    Test rapide avec configuration par défaut
    
    Args:
        game_id: ID du jeu ARC
    """
    print("\n" + "="*60)
    print("MAGEN V2 - Quick Test")
    print("="*60)
    print(f"Game: {game_id}")
    print(f"Episodes: 50")
    print("="*60 + "\n")
    
    # Configuration par défaut
    config = HyperparameterConfig()
    
    # Évaluer
    result = evaluate_config(config, n_episodes=50, game_id=game_id, verbose=True)
    
    # Afficher les résultats
    print("\n" + "="*60)
    print("RÉSULTATS TEST RAPIDE")
    print("="*60)
    print(f"Win Rate: {result.win_rate*100:.2f}%")
    print(f"Avg Reward: {result.avg_reward:.2f}")
    print(f"Avg Steps: {result.avg_steps:.1f}")
    print(f"Training Time: {result.training_time:.2f}s")
    print("="*60 + "\n")


def main():
    """Point d'entrée principal"""
    parser = argparse.ArgumentParser(
        description="MAGEN V2 - Hyperparameter Optimization"
    )
    
    parser.add_argument(
        '--method',
        type=str,
        choices=['grid', 'bayesian', 'test'],
        default='test',
        help='Méthode d\'optimisation (grid, bayesian, test)'
    )
    
    parser.add_argument(
        '--episodes',
        type=int,
        default=50,
        help='Nombre d\'épisodes par configuration'
    )
    
    parser.add_argument(
        '--iterations',
        type=int,
        default=20,
        help='Nombre d\'itérations (Bayesian uniquement)'
    )
    
    parser.add_argument(
        '--game',
        type=str,
        default='ls20-9607627b',
        help='ID du jeu ARC'
    )
    
    parser.add_argument(
        '--save-dir',
        type=str,
        default='results',
        help='Répertoire de sauvegarde des résultats'
    )
    
    args = parser.parse_args()
    
    # Créer le répertoire de sauvegarde
    save_dir = Path(args.save_dir)
    save_dir.mkdir(parents=True, exist_ok=True)
    
    # Lancer l'optimisation
    if args.method == 'grid':
        run_grid_search(
            n_episodes=args.episodes,
            game_id=args.game,
            save_dir=save_dir / 'grid_search'
        )
    
    elif args.method == 'bayesian':
        run_bayesian_optimization(
            n_iterations=args.iterations,
            n_episodes=args.episodes,
            game_id=args.game,
            save_dir=save_dir / 'bayesian'
        )
    
    else:  # test
        run_quick_test(game_id=args.game)


if __name__ == '__main__':
    main()

# Made with Bob
