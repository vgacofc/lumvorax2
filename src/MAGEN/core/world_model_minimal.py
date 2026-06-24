"""
World Model Minimal - MAGEN V28
Projection d'état futur avant décision (causalité réelle)

Principe:
- Au lieu de: reward_score = f(state, action)
- Faire: future_state = model_transition(state, action)
         reward_score = f(future_state)

Objectif: Passer de "scoring d'actions" à "projection d'état futur"

Date: 2026-06-14
"""

from typing import Dict, List, Any, Tuple, Optional
import numpy as np
from copy import deepcopy


class WorldModelMinimal:
    """
    Modèle de transition ultra-minimal: state + action → future_state
    
    Principe:
    - Apprend les transitions observées
    - Prédit l'état futur pour une action donnée
    - Permet évaluation causale (pas juste corrélation)
    
    Différence critique:
    - AVANT: score = f(state, action) → évaluation locale
    - APRÈS: future_state = model(state, action)
             score = f(future_state) → évaluation prédictive
    """
    
    def __init__(
        self,
        grid_size: Tuple[int, int] = (64, 64),
        learning_rate: float = 0.1
    ):
        """
        Args:
            grid_size: Taille de la grille (H, W)
            learning_rate: Taux d'apprentissage transitions
        """
        self.grid_size = grid_size
        self.learning_rate = learning_rate
        
        # Historique transitions observées
        self.transitions = []  # [(state, action, next_state)]
        
        # Modèle appris: {action: transition_model}
        self.action_models = {}
        
        # Statistiques
        self.stats = {
            'transitions_observed': 0,
            'predictions_made': 0,
            'model_updates': 0
        }
    
    def observe_transition(
        self,
        state: Dict[str, Any],
        action: str,
        next_state: Dict[str, Any]
    ):
        """
        Observe une transition réelle et met à jour le modèle.
        
        Args:
            state: État avant action
            action: Action exécutée
            next_state: État après action
        """
        # Stocker transition
        self.transitions.append((state, action, next_state))
        self.stats['transitions_observed'] += 1
        
        # Mettre à jour modèle pour cette action
        self._update_action_model(state, action, next_state)
        self.stats['model_updates'] += 1
        
        # Limiter historique (garder 100 dernières transitions)
        if len(self.transitions) > 100:
            self.transitions = self.transitions[-100:]
    
    def predict_next_state(
        self,
        state: Dict[str, Any],
        action: str
    ) -> Dict[str, Any]:
        """
        Prédit l'état futur si action exécutée.
        
        CRITIQUE: C'est ici que la causalité devient réelle.
        
        Args:
            state: État actuel
            action: Action à simuler
        
        Returns:
            dict: État futur prédit
        """
        self.stats['predictions_made'] += 1
        
        # Si pas de modèle pour cette action, retourner état actuel
        if action not in self.action_models:
            return deepcopy(state)
        
        # Prédire changements
        predicted_state = deepcopy(state)
        
        # Appliquer modèle de transition
        model = self.action_models[action]
        
        # Prédire position agent (si détecté)
        if 'agent_pos' in state and state['agent_pos'] is not None:
            predicted_state['agent_pos'] = self._predict_agent_position(
                state['agent_pos'], action, model
            )
        
        # Prédire changements grid (si disponible)
        if 'grid' in state and isinstance(state['grid'], np.ndarray):
            predicted_state['grid'] = self._predict_grid_changes(
                state['grid'], action, model
            )
        
        # Prédire entités (si disponibles)
        if 'entities' in state:
            predicted_state['entities'] = self._predict_entity_changes(
                state['entities'], action, model
            )
        
        return predicted_state
    
    def _update_action_model(
        self,
        state: Dict[str, Any],
        action: str,
        next_state: Dict[str, Any]
    ):
        """
        Met à jour le modèle de transition pour une action.
        
        Modèle simple: moyenne des changements observés
        """
        if action not in self.action_models:
            self.action_models[action] = {
                'agent_delta': [],
                'grid_changes': [],
                'entity_changes': [],
                'observations': 0
            }
        
        model = self.action_models[action]
        
        # Observer changement position agent
        if ('agent_pos' in state and state['agent_pos'] is not None and
            'agent_pos' in next_state and next_state['agent_pos'] is not None):
            
            delta = (
                next_state['agent_pos'][0] - state['agent_pos'][0],
                next_state['agent_pos'][1] - state['agent_pos'][1]
            )
            model['agent_delta'].append(delta)
        
        # Observer changements grid
        if ('grid' in state and 'grid' in next_state and
            isinstance(state['grid'], np.ndarray) and
            isinstance(next_state['grid'], np.ndarray)):
            
            diff = next_state['grid'] - state['grid']
            changed_positions = np.argwhere(diff != 0)
            model['grid_changes'].append(len(changed_positions))
        
        model['observations'] += 1
        
        # Limiter historique par action (garder 20 dernières)
        if len(model['agent_delta']) > 20:
            model['agent_delta'] = model['agent_delta'][-20:]
        if len(model['grid_changes']) > 20:
            model['grid_changes'] = model['grid_changes'][-20:]
    
    def _default_agent_movement(
        self,
        pos: Tuple[int, int],
        action: str
    ) -> Tuple[int, int]:
        """
        Mouvement agent par défaut (heuristique).
        
        Mapping actions ARC-AGI-3:
        - ACTION1: move_up
        - ACTION2: move_down
        - ACTION3: move_left
        - ACTION4: move_right
        """
        x, y = pos
        
        if action == 'ACTION1' or action == 'move_up':
            return (x, max(0, y - 1))
        elif action == 'ACTION2' or action == 'move_down':
            return (x, min(self.grid_size[0] - 1, y + 1))
        elif action == 'ACTION3' or action == 'move_left':
            return (max(0, x - 1), y)
        elif action == 'ACTION4' or action == 'move_right':
            return (min(self.grid_size[1] - 1, x + 1), y)
        else:
            return pos
    
    def _predict_grid_changes(
        self,
        grid: np.ndarray,
        action: str,
        model: Dict[str, Any]
    ) -> np.ndarray:
        """
        Prédit changements dans la grille.
        
        Pour l'instant: retourne copie (pas de prédiction fine)
        """
        return grid.copy()
    
    def _predict_entity_changes(
        self,
        entities: List[Dict[str, Any]],
        action: str,
        model: Dict[str, Any]
    ) -> List[Dict[str, Any]]:
        """
        Prédit changements dans les entités.
        
        Pour l'instant: retourne copie (pas de prédiction fine)
        """
        return deepcopy(entities)
    
    def _predict_agent_position(
        self,
        current_pos: Tuple[int, int],
        action: str,
        model: Dict[str, Any]
    ) -> Tuple[int, int]:
        """
        Prédit position agent après action.
        
        Utilise moyenne des deltas observés pour cette action.
        """
        if not model['agent_delta']:
            # Pas de données: utiliser heuristique par défaut
            return self._default_agent_movement(current_pos, action)
        
        # Moyenne des deltas observés
        avg_delta = (
            sum(d[0] for d in model['agent_delta']) / len(model['agent_delta']),
            sum(d[1] for d in model['agent_delta']) / len(model['agent_delta'])
        )
        
        # Arrondir et appliquer
        new_pos = (
            int(current_pos[0] + round(avg_delta[0])),
            int(current_pos[1] + round(avg_delta[1]))
        )
        
        # Clip dans bounds
        new_pos = (
            max(0, min(self.grid_size[1] - 1, new_pos[0])),
            max(0, min(self.grid_size[0] - 1, new_pos[1]))
        )
        
        return new_pos
    
    def get_action_confidence(self, action: str) -> float:
        """
        Confiance dans la prédiction pour une action.
        
        Returns:
            float: 0.0-1.0 (0 = aucune donnée, 1 = beaucoup de données)
        """
        if action not in self.action_models:
            return 0.0
        
        observations = self.action_models[action]['observations']
        
        # Confiance = min(observations / 10, 1.0)
        # 10 observations = confiance maximale
        return min(observations / 10.0, 1.0)
    
    def get_stats(self) -> Dict[str, Any]:
        """Retourne statistiques du modèle."""
        return {
            **self.stats,
            'actions_modeled': len(self.action_models),
            'avg_observations_per_action': (
                sum(m['observations'] for m in self.action_models.values()) /
                max(1, len(self.action_models))
            )
        }
    
    def reset(self):
        """Réinitialise le modèle (nouveau jeu)."""
        self.transitions = []
        self.action_models = {}
        self.stats = {
            'transitions_observed': 0,
            'predictions_made': 0,
            'model_updates': 0
        }

# Made with Bob
