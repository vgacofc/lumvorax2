"""
Information Gain Reward - MAGEN V23
Récompense l'exploration informative en séparant score_reward et knowledge_reward
"""

import numpy as np
from typing import Dict, Set, List
from dataclasses import dataclass
import hashlib


@dataclass
class StateTransition:
    """Transition d'état (réutilisée pour cohérence)"""
    state_before: np.ndarray
    action: str
    state_after: np.ndarray
    reward: float
    context: Dict
    timestamp: float = 0.0


class InformationGainReward:
    """
    Récompense l'exploration informative
    
    Principe:
    1. Séparer score_reward (niveaux complétés) et knowledge_reward (information gagnée)
    2. Récompenser la découverte de nouveaux états
    3. Récompenser la réduction d'incertitude causale
    4. Éviter de pénaliser les explorations utiles
    """
    
    def __init__(self, 
                 knowledge_weight: float = 0.1,
                 causal_weight: float = 0.05,
                 novelty_weight: float = 0.1):
        """
        Args:
            knowledge_weight: Poids du reward épistémique (nouveaux états)
            causal_weight: Poids du reward causal (compréhension actions)
            novelty_weight: Poids du reward de nouveauté
        """
        self.knowledge_weight = knowledge_weight
        self.causal_weight = causal_weight
        self.novelty_weight = novelty_weight
        
        # Base de connaissances
        self.known_states: Set[str] = set()
        self.action_observations: Dict[str, int] = {}
        self.action_outcomes: Dict[str, List[np.ndarray]] = {}
        
        # Statistiques
        self.stats = {
            'total_rewards_computed': 0,
            'knowledge_rewards_given': 0,
            'causal_rewards_given': 0,
            'novelty_rewards_given': 0,
            'avg_total_reward': 0.0,
            'avg_knowledge_reward': 0.0,
            'avg_causal_reward': 0.0
        }
    
    def compute_reward(self, transition: StateTransition) -> float:
        """
        Calcule reward total = score_reward + knowledge_reward + causal_reward
        
        Args:
            transition: Transition observée
        
        Returns:
            Reward total combiné
        """
        # 1. Score reward (niveaux complétés)
        score_reward = transition.reward
        
        # 2. Knowledge reward (nouveaux états)
        knowledge_reward = self._compute_knowledge_reward(transition)
        
        # 3. Causal reward (compréhension action)
        causal_reward = self._compute_causal_reward(transition)
        
        # 4. Novelty reward (diversité exploration)
        novelty_reward = self._compute_novelty_reward(transition)
        
        # Combiner
        total_reward = (
            score_reward +
            self.knowledge_weight * knowledge_reward +
            self.causal_weight * causal_reward +
            self.novelty_weight * novelty_reward
        )
        
        # Mettre à jour statistiques
        self.stats['total_rewards_computed'] += 1
        self.stats['avg_total_reward'] = (
            (self.stats['avg_total_reward'] * (self.stats['total_rewards_computed'] - 1) + total_reward) /
            self.stats['total_rewards_computed']
        )
        
        if knowledge_reward > 0:
            self.stats['knowledge_rewards_given'] += 1
            self.stats['avg_knowledge_reward'] = (
                (self.stats['avg_knowledge_reward'] * (self.stats['knowledge_rewards_given'] - 1) + knowledge_reward) /
                self.stats['knowledge_rewards_given']
            )
        
        if causal_reward > 0:
            self.stats['causal_rewards_given'] += 1
            self.stats['avg_causal_reward'] = (
                (self.stats['avg_causal_reward'] * (self.stats['causal_rewards_given'] - 1) + causal_reward) /
                self.stats['causal_rewards_given']
            )
        
        if novelty_reward > 0:
            self.stats['novelty_rewards_given'] += 1
        
        return total_reward
    
    def _compute_knowledge_reward(self, transition: StateTransition) -> float:
        """
        Calcule reward épistémique (découverte nouveaux états)
        
        Returns:
            Reward entre 0 et 1
        """
        # Hasher l'état résultant
        state_hash = hashlib.md5(transition.state_after.tobytes()).hexdigest()
        
        # Nouvel état ?
        if state_hash not in self.known_states:
            self.known_states.add(state_hash)
            return 1.0  # Reward maximal pour nouvel état
        
        return 0.0  # État déjà connu
    
    def _compute_causal_reward(self, transition: StateTransition) -> float:
        """
        Calcule reward causal (amélioration compréhension action)
        
        Mesure: Réduction de l'incertitude sur les effets de l'action
        
        Returns:
            Reward entre 0 et 1
        """
        action = transition.action
        
        # Enregistrer observation
        if action not in self.action_observations:
            self.action_observations[action] = 0
            self.action_outcomes[action] = []
        
        self.action_observations[action] += 1
        self.action_outcomes[action].append(transition.state_after)
        
        # Reward décroissant avec nombre d'observations
        # Premières observations = très informatives
        # Observations ultérieures = moins informatives
        obs_count = self.action_observations[action]
        
        if obs_count <= 5:
            # Premières 5 observations: reward élevé
            return 1.0 - (obs_count - 1) * 0.15  # 1.0, 0.85, 0.70, 0.55, 0.40
        elif obs_count <= 10:
            # Observations 6-10: reward moyen
            return 0.3 - (obs_count - 6) * 0.05  # 0.30, 0.25, 0.20, 0.15, 0.10
        else:
            # Observations >10: reward faible
            return 0.05
    
    def _compute_novelty_reward(self, transition: StateTransition) -> float:
        """
        Calcule reward de nouveauté (diversité exploration)
        
        Mesure: Distance aux états précédents de la même action
        
        Returns:
            Reward entre 0 and 1
        """
        action = transition.action
        
        if action not in self.action_outcomes or len(self.action_outcomes[action]) <= 1:
            return 1.0  # Première observation = très novel
        
        # Calculer distance moyenne aux outcomes précédents
        previous_outcomes = self.action_outcomes[action][:-1]  # Exclure le dernier (actuel)
        
        distances = []
        for prev_outcome in previous_outcomes:
            diff = transition.state_after - prev_outcome
            dist = float(np.linalg.norm(diff) / np.sqrt(transition.state_after.size))
            distances.append(dist)
        
        avg_distance = float(np.mean(distances))
        
        # Normaliser distance en reward (distance élevée = novel)
        # Utiliser sigmoïde pour mapper [0, inf] → [0, 1]
        novelty_reward = 1.0 / (1.0 + np.exp(-5 * (avg_distance - 0.5)))
        
        return novelty_reward
    
    def get_exploration_bonus(self, action: str) -> float:
        """
        Retourne un bonus d'exploration pour une action
        
        Encourage exploration d'actions peu observées
        
        Args:
            action: Nom de l'action
        
        Returns:
            Bonus entre 0 and 1
        """
        if action not in self.action_observations:
            return 1.0  # Action jamais observée = bonus maximal
        
        obs_count = self.action_observations[action]
        
        # Bonus décroissant avec observations
        if obs_count < 5:
            return 1.0 - obs_count * 0.15
        elif obs_count < 10:
            return 0.5 - (obs_count - 5) * 0.08
        else:
            return 0.1  # Bonus minimal
    
    def should_explore(self, action: str, threshold: float = 0.5) -> bool:
        """
        Décide si une action mérite exploration
        
        Args:
            action: Nom de l'action
            threshold: Seuil de bonus pour recommander exploration
        
        Returns:
            True si exploration recommandée
        """
        bonus = self.get_exploration_bonus(action)
        return bonus >= threshold
    
    def get_action_uncertainty(self, action: str) -> float:
        """
        Mesure l'incertitude sur les effets d'une action
        
        Incertitude élevée = besoin d'exploration
        
        Args:
            action: Nom de l'action
        
        Returns:
            Incertitude entre 0 (certain) et 1 (très incertain)
        """
        if action not in self.action_outcomes or len(self.action_outcomes[action]) < 2:
            return 1.0  # Très incertain (peu d'observations)
        
        outcomes = self.action_outcomes[action]
        
        # Calculer variance des outcomes
        variances = []
        mean_outcome = np.mean(outcomes, axis=0)
        
        for outcome in outcomes:
            variance = np.var(outcome - mean_outcome)
            variances.append(variance)
        
        avg_variance = float(np.mean(variances))
        
        # Mapper variance → incertitude [0, 1]
        uncertainty = 1.0 / (1.0 + np.exp(-5 * (avg_variance - 0.1)))
        
        return uncertainty
    
    def get_statistics(self) -> Dict:
        """Retourne les statistiques du système de reward"""
        return {
            **self.stats,
            'known_states': len(self.known_states),
            'actions_observed': len(self.action_observations),
            'total_action_observations': sum(self.action_observations.values()),
            'knowledge_reward_rate': self.stats['knowledge_rewards_given'] / self.stats['total_rewards_computed'] if self.stats['total_rewards_computed'] > 0 else 0.0,
            'causal_reward_rate': self.stats['causal_rewards_given'] / self.stats['total_rewards_computed'] if self.stats['total_rewards_computed'] > 0 else 0.0
        }
    
    def reset(self):
        """Réinitialise la base de connaissances (nouveau jeu)"""
        self.known_states.clear()
        self.action_observations.clear()
        self.action_outcomes.clear()
        
        # Garder statistiques globales
    
    def __repr__(self):
        return f"InformationGainReward(known_states={len(self.known_states)}, actions={len(self.action_observations)})"

# Made with Bob
