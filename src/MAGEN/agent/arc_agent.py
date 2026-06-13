"""
MAGEN - Agent ARC-AGI-3
Agent d'exploration et résolution de jeux ARC

Stratégie:
- Exploration trial-and-error
- Apprentissage par renforcement simple
- Exploitation patterns mémorisés
- Adaptation dynamique
"""

import numpy as np
from typing import List, Dict, Optional, Tuple, Any
import random
import time
from pathlib import Path

# Imports locaux
import sys
sys.path.append(str(Path(__file__).parent.parent))
from core.magen_memory import MAGENMemory, Experience
from perception.arc_perception import ARCPerception


class ARCAgent:
    """
    Agent ARC-AGI-3 avec mémoire MAGEN
    
    Stratégie:
    1. Analyser état actuel (perception)
    2. Chercher pattern similaire (mémoire)
    3. Si pattern trouvé → exploitation
    4. Sinon → exploration aléatoire
    5. Mémoriser résultat
    6. Consolider apprentissage
    """
    
    # Actions disponibles ARC-AGI-3
    ACTIONS = [
        'RESET',
        'ACTION1',
        'ACTION2', 
        'ACTION3',
        'ACTION4',
        'ACTION5',
        'ACTION6',  # Nécessite (x, y)
        'ACTION7'
    ]
    
    def __init__(self,
                 memory: MAGENMemory,
                 perception: ARCPerception,
                 exploration_rate: float = 0.3,
                 max_actions_per_level: int = 50):
        """
        Initialisation agent ARC
        
        Args:
            memory: Système mémoire MAGEN
            perception: Système perception ARC
            exploration_rate: Taux exploration [0-1]
            max_actions_per_level: Max actions par niveau
        """
        self.memory = memory
        self.perception = perception
        self.exploration_rate = exploration_rate
        self.max_actions_per_level = max_actions_per_level
        
        # Statistiques
        self.total_actions = 0
        self.successful_levels = 0
        self.failed_levels = 0
        
        # État actuel
        self.current_game_id: Optional[str] = None
        self.current_level: int = 0
        self.actions_this_level: List[Tuple[str, Optional[Dict[str, int]]]] = []
        
        print(f"[ARC Agent] Initialisé")
        print(f"[ARC Agent] Exploration rate: {exploration_rate}")
        print(f"[ARC Agent] Max actions/level: {max_actions_per_level}")
    
    def solve_level(self, 
                   game_id: str,
                   level: int,
                   initial_state: np.ndarray,
                   execute_action_fn) -> Tuple[bool, int, List[str]]:
        """
        Résoudre un niveau ARC
        
        Args:
            game_id: ID du jeu
            level: Numéro niveau
            initial_state: État initial grille
            execute_action_fn: Fonction exécution action (action, data) -> (new_state, done)
            
        Returns:
            (success, num_actions, action_log)
        """
        print(f"\n[ARC Agent] Résolution {game_id} niveau {level}")
        
        # Initialisation
        self.current_game_id = game_id
        self.current_level = level
        self.actions_this_level = []
        
        current_state = initial_state.copy()
        action_log = []
        done = False
        
        # Boucle résolution
        for action_idx in range(self.max_actions_per_level):
            # 1. Analyser état actuel
            features = self.perception.extract_features(current_state)
            
            # 2. Décider action (exploration vs exploitation)
            if random.random() < self.exploration_rate:
                # EXPLORATION: Action aléatoire
                action, action_data = self._explore_action(current_state)
                strategy = "exploration"
            else:
                # EXPLOITATION: Utiliser pattern mémorisé
                pattern = self.memory.retrieve_similar_pattern(current_state, game_id)
                if pattern is not None and pattern.action_sequence:
                    # Utiliser première action du pattern
                    action, action_data = pattern.action_sequence[0]
                    strategy = "exploitation"
                else:
                    # Pas de pattern → exploration
                    action, action_data = self._explore_action(current_state)
                    strategy = "exploration_fallback"
            
            # 3. Exécuter action
            try:
                new_state, done = execute_action_fn(action, action_data)
                success = True
            except Exception as e:
                print(f"[ARC Agent] Erreur action {action}: {e}")
                new_state = current_state.copy()
                done = False
                success = False
            
            # 4. Calculer reward
            if done:
                reward = 10.0  # Niveau complété!
            elif not np.array_equal(current_state, new_state):
                reward = 1.0  # Changement détecté
            else:
                reward = 0.0  # Aucun changement
            
            # 5. Mémoriser expérience
            experience = Experience(
                timestamp=time.time(),
                game_id=game_id,
                level=level,
                state=current_state.copy(),
                action=action,
                action_data=action_data,
                result_state=new_state.copy(),
                reward=reward,
                success=done,
                metadata={
                    'strategy': strategy,
                    'action_idx': action_idx,
                    'features': features.to_dict()
                }
            )
            self.memory.inject_experience(experience)
            
            # 6. Logger action
            action_str = f"{action}"
            if action_data:
                action_str += f"({action_data['x']},{action_data['y']})"
            action_log.append(f"[{action_idx}] {action_str} -> reward={reward:.1f} ({strategy})")
            
            self.actions_this_level.append((action, action_data))
            self.total_actions += 1
            
            # 7. Vérifier si niveau complété
            if done:
                print(f"[ARC Agent] ✓ Niveau complété en {action_idx + 1} actions!")
                self.successful_levels += 1
                return True, action_idx + 1, action_log
            
            # 8. Mettre à jour état
            current_state = new_state
        
        # Niveau non complété
        print(f"[ARC Agent] ✗ Niveau échoué (max actions atteint)")
        self.failed_levels += 1
        return False, self.max_actions_per_level, action_log
    
    def _explore_action(self, state: np.ndarray) -> Tuple[str, Optional[Dict[str, int]]]:
        """
        Choisir action aléatoire (exploration)
        
        Args:
            state: État actuel
            
        Returns:
            (action, action_data)
        """
        # Choisir action aléatoire
        action = random.choice(self.ACTIONS)
        
        # Si ACTION6, générer coordonnées aléatoires
        if action == 'ACTION6':
            height, width = state.shape
            x = random.randint(0, width - 1)
            y = random.randint(0, height - 1)
            action_data = {'x': x, 'y': y}
        else:
            action_data = None
        
        return action, action_data
    
    def get_statistics(self) -> Dict[str, Any]:
        """Obtenir statistiques agent"""
        total_levels = self.successful_levels + self.failed_levels
        success_rate = self.successful_levels / max(1, total_levels)
        
        return {
            'total_actions': self.total_actions,
            'successful_levels': self.successful_levels,
            'failed_levels': self.failed_levels,
            'total_levels': total_levels,
            'success_rate': success_rate,
            'avg_actions_per_level': self.total_actions / max(1, total_levels),
            'exploration_rate': self.exploration_rate,
            'memory_metrics': self.memory.get_metrics().to_dict()
        }
    
    def adjust_exploration_rate(self, new_rate: float) -> None:
        """Ajuster taux exploration dynamiquement"""
        old_rate = self.exploration_rate
        self.exploration_rate = max(0.0, min(1.0, new_rate))
        print(f"[ARC Agent] Exploration rate: {old_rate:.2f} -> {self.exploration_rate:.2f}")
    
    def save_statistics(self, filepath: str) -> None:
        """Sauvegarder statistiques agent"""
        import json
        
        stats = self.get_statistics()
        
        with open(filepath, 'w') as f:
            json.dump(stats, f, indent=2)
        
        print(f"[ARC Agent] Statistiques sauvegardées: {filepath}")
    
    def __repr__(self) -> str:
        """Représentation string"""
        stats = self.get_statistics()
        return (
            f"ARCAgent(\n"
            f"  Total actions: {stats['total_actions']}\n"
            f"  Levels: {stats['successful_levels']}✓ / {stats['failed_levels']}✗\n"
            f"  Success rate: {stats['success_rate']:.2%}\n"
            f"  Avg actions/level: {stats['avg_actions_per_level']:.1f}\n"
            f"  Exploration rate: {self.exploration_rate:.2%}\n"
            f"  Memory: {self.memory.get_metrics().short_term_size} ST, "
            f"{self.memory.get_metrics().patterns_count} patterns\n"
            f")"
        )

# Made with Bob
