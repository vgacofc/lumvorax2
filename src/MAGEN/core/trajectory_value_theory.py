"""
MAGEN Trajectory Value Theory (TVT)
====================================

Module unifié résolvant le problème central :
"Cette trajectoire mérite-t-elle encore des ressources cognitives ?"

Intègre :
- C17 : Reputation (contextualisée topologiquement)
- C18 : Budget Dynamique (allocation aux hypothèses)
- C19 : Trajectory Monitoring (détection stagnation)
- Destruction contrôlée de politique
- Mémoire viabilité topologique

Date : 2026-06-15
Version : V30.0.0 (saut architectural majeur)
"""

import numpy as np
from collections import defaultdict, deque
from typing import Dict, List, Tuple, Set, Optional
import time


class TrajectoryValueTheory:
    """
    Théorie interne de la valeur des trajectoires.
    
    Principe central :
    Toutes les trajectoires n'ont pas la même valeur informationnelle.
    Le système doit apprendre à reconnaître et abandonner les attracteurs stériles.
    """
    
    def __init__(
        self,
        stagnation_threshold: int = 10,
        collapse_variance_threshold: float = 0.02,
        region_memory_size: int = 100,
        min_exploration_budget: float = 0.1,
        policy_break_threshold: float = 0.8
    ):
        """
        Args:
            stagnation_threshold: Steps sans progrès avant détection stagnation
            collapse_variance_threshold: Variance scores sous laquelle = collapse
            region_memory_size: Taille mémoire régions explorées
            min_exploration_budget: Budget minimum pour exploration forcée
            policy_break_threshold: Seuil pour rupture politique forcée
        """
        self.stagnation_threshold = stagnation_threshold
        self.collapse_variance_threshold = collapse_variance_threshold
        self.region_memory_size = region_memory_size
        self.min_exploration_budget = min_exploration_budget
        self.policy_break_threshold = policy_break_threshold
        
        # Mémoire viabilité topologique
        self.region_viability: Dict[Tuple[int, int], float] = defaultdict(float)
        self.region_visits: Dict[Tuple[int, int], int] = defaultdict(int)
        self.sterile_regions: Set[Tuple[int, int]] = set()
        
        # Mémoire trajectoires
        self.trajectory_history: deque = deque(maxlen=region_memory_size)
        self.reward_history: deque = deque(maxlen=stagnation_threshold)
        
        # Réputation actions contextualisée
        self.action_reputation: Dict[Tuple[str, Tuple[int, int]], Dict] = defaultdict(
            lambda: {'success': 0, 'attempts': 0, 'avg_reward': 0.0}
        )
        
        # Budget dynamique par hypothèse
        self.hypothesis_budgets: Dict[str, float] = defaultdict(lambda: 1.0)
        
        # Détection collapse politique
        self.action_distribution: deque = deque(maxlen=20)
        self.policy_entropy_history: deque = deque(maxlen=10)
        
        # Métriques
        self.total_policy_breaks = 0
        self.total_regions_marked_sterile = 0
        self.current_stagnation_steps = 0
        
    def discretize_position(self, pos: Tuple[float, float], grid_size: int = 4) -> Tuple[int, int]:
        """
        Discrétise position en région topologique.
        
        CORRECTION BUG #2 : Réduire grid_size 8→4 pour plus de régions.
        Sur grille 10x10, grid_size=4 donne ~6 régions au lieu de 2.
        """
        x, y = pos
        region_x = int(x // grid_size)
        region_y = int(y // grid_size)
        return (region_x, region_y)
    
    def update_region_viability(
        self,
        position: Tuple[float, float],
        reward: float,
        progress: bool
    ):
        """
        Met à jour viabilité d'une région topologique.
        
        Principe : Une région est viable si elle produit du progrès.
        """
        region = self.discretize_position(position)
        self.region_visits[region] += 1
        
        # Mise à jour viabilité (moyenne mobile)
        alpha = 0.1
        current_viability = self.region_viability[region]
        
        # Viabilité = reward + bonus progrès
        viability_signal = reward + (1.0 if progress else -0.5)
        self.region_viability[region] = (
            alpha * viability_signal + (1 - alpha) * current_viability
        )
        
        # Marquer région stérile si viabilité négative et visites suffisantes
        if self.region_visits[region] > 5 and self.region_viability[region] < -0.5:
            self.sterile_regions.add(region)
            self.total_regions_marked_sterile += 1
    
    def update_action_reputation(
        self,
        action: str,
        position: Tuple[float, float],
        reward: float,
        success: bool
    ):
        """
        Met à jour réputation action DANS CONTEXTE TOPOLOGIQUE.
        
        Principe : Une action n'est pas "bonne" globalement,
        mais "bonne dans cette région".
        """
        region = self.discretize_position(position)
        key = (action, region)
        
        rep = self.action_reputation[key]
        rep['attempts'] += 1
        
        if success:
            rep['success'] += 1
        
        # Moyenne mobile reward
        alpha = 0.2
        rep['avg_reward'] = alpha * reward + (1 - alpha) * rep['avg_reward']
    
    def get_action_value(
        self,
        action: str,
        position: Tuple[float, float]
    ) -> float:
        """
        Calcule valeur action dans contexte topologique actuel.
        
        Combine :
        - Réputation locale
        - Viabilité région
        - Historique succès
        """
        region = self.discretize_position(position)
        key = (action, region)
        
        rep = self.action_reputation[key]
        
        if rep['attempts'] == 0:
            # Action jamais essayée dans cette région = exploration bonus
            return 0.5 + 0.3  # Bonus exploration
        
        # Success rate
        success_rate = rep['success'] / rep['attempts']
        
        # Viabilité région
        region_viability = self.region_viability[region]
        
        # Valeur = success_rate * avg_reward * region_viability
        value = success_rate * (rep['avg_reward'] + 1.0) * (region_viability + 1.0)
        
        return value
    
    def detect_policy_collapse(self, action_scores: Dict[str, float]) -> bool:
        """
        Détecte collapse politique via variance scores.
        
        Signal critique : variance < 0.02 = toutes actions équivalentes
        """
        if len(action_scores) < 2:
            return False
        
        scores = list(action_scores.values())
        variance = float(np.var(scores))
        
        return bool(variance < self.collapse_variance_threshold)
    
    def detect_stagnation(self, current_reward: float) -> bool:
        """
        Détecte stagnation via historique rewards.
        
        Stagnation = N steps sans amélioration reward.
        """
        self.reward_history.append(current_reward)
        
        if len(self.reward_history) < self.stagnation_threshold:
            return False
        
        # Vérifier si reward stagne
        recent_rewards = list(self.reward_history)
        max_reward = max(recent_rewards)
        current_reward = recent_rewards[-1]
        
        # Stagnation si pas d'amélioration
        if current_reward < max_reward * 0.9:
            self.current_stagnation_steps += 1
        else:
            self.current_stagnation_steps = 0
        
        return self.current_stagnation_steps >= self.stagnation_threshold
    
    def compute_policy_entropy(self, action_distribution: Dict[str, float]) -> float:
        """Calcule entropie distribution actions."""
        if not action_distribution:
            return 0.0
            
        probs = np.array(list(action_distribution.values()))
        probs = probs / probs.sum()  # Normaliser
        
        # Entropie Shannon
        entropy = float(-np.sum(probs * np.log(probs + 1e-10)))
        return entropy
    
    def should_break_policy(
        self,
        action_scores: Dict[str, float],
        position: Tuple[float, float]
    ) -> bool:
        """
        Décide si politique actuelle doit être cassée.
        
        CORRECTION CRITIQUE : Collapse seul ne suffit pas.
        Rupture seulement si problème structurel détecté.
        
        Critères (OR) :
        1. Région stérile (viabilité < 0.2)
        2. Entropie politique trop faible (<30% max)
        3. Collapse + stagnation simultanés (AND)
        """
        # 1. Région stérile (priorité haute)
        region = self.discretize_position(position)
        if region in self.sterile_regions:
            return True
        
        # 2. Entropie politique faible
        entropy = self.compute_policy_entropy(action_scores)
        self.policy_entropy_history.append(entropy)
        
        if len(self.policy_entropy_history) >= 5:
            avg_entropy = np.mean(list(self.policy_entropy_history))
            max_entropy = np.log(len(action_scores))
            
            if avg_entropy < 0.3 * max_entropy:
                return True
        
        # 3. CORRECTION : Collapse ET stagnation (pas collapse seul)
        # Raison : Collapse peut être légitime si progrès existe
        collapse = self.detect_policy_collapse(action_scores)
        stagnation = self.current_stagnation_steps >= self.stagnation_threshold
        
        if collapse and stagnation:
            return True
        
        return False
    
    def break_policy(
        self,
        action_scores: Dict[str, float],
        position: Tuple[float, float]
    ) -> Dict[str, float]:
        """
        Casse politique actuelle via injection exploration forcée.
        
        Stratégie :
        1. Identifier action dominante
        2. Réduire son score drastiquement
        3. Booster actions sous-explorées
        """
        self.total_policy_breaks += 1
        
        # Identifier action dominante
        dominant_action = max(action_scores, key=lambda k: action_scores[k])
        
        # Nouvelle distribution
        new_scores = {}
        for action, score in action_scores.items():
            if action == dominant_action:
                # Réduire action dominante
                new_scores[action] = score * 0.3
            else:
                # Booster autres actions
                new_scores[action] = score * 1.5 + 0.2
        
        return new_scores
    
    def allocate_budget(
        self,
        hypotheses: List[str],
        position: Tuple[float, float]
    ) -> Dict[str, float]:
        """
        Alloue budget computationnel aux hypothèses de trajectoire.
        
        Principe : Budget proportionnel à valeur informationnelle estimée.
        """
        region = self.discretize_position(position)
        budgets = {}
        
        for hypothesis in hypotheses:
            # Budget basé sur viabilité région + réputation
            region_viability = self.region_viability[region]
            
            # Hypothèse dans région viable = plus de budget
            if region_viability > 0:
                budget = 1.0 + region_viability
            else:
                budget = self.min_exploration_budget
            
            budgets[hypothesis] = budget
        
        # Normaliser
        total = sum(budgets.values())
        budgets = {h: b / total for h, b in budgets.items()}
        
        return budgets
    
    def select_action_with_tvt(
        self,
        action_scores: Dict[str, float],
        position: Tuple[float, float],
        current_reward: float
    ) -> Tuple[str, Dict[str, float], bool]:
        """
        Sélection action avec Trajectory Value Theory.
        
        Returns:
            (action, adjusted_scores, policy_broken)
        """
        # Détecter stagnation
        is_stagnant = self.detect_stagnation(current_reward)
        
        # Décider si rupture politique nécessaire
        should_break = self.should_break_policy(action_scores, position)
        
        policy_broken = False
        adjusted_scores = action_scores.copy()
        
        if should_break or is_stagnant:
            # Casser politique
            adjusted_scores = self.break_policy(action_scores, position)
            policy_broken = True
        
        # Ajuster scores avec réputation topologique
        for action in adjusted_scores:
            action_value = self.get_action_value(action, position)
            adjusted_scores[action] *= action_value
        
        # Sélection finale (softmax)
        scores_array = np.array(list(adjusted_scores.values()))
        probs = np.exp(scores_array) / np.sum(np.exp(scores_array))
        
        actions = list(adjusted_scores.keys())
        selected_action = np.random.choice(actions, p=probs)
        
        return selected_action, adjusted_scores, policy_broken
    
    def get_metrics(self) -> Dict:
        """Retourne métriques TVT."""
        return {
            'total_policy_breaks': self.total_policy_breaks,
            'sterile_regions': len(self.sterile_regions),
            'total_regions_marked_sterile': self.total_regions_marked_sterile,
            'current_stagnation_steps': self.current_stagnation_steps,
            'avg_policy_entropy': np.mean(list(self.policy_entropy_history)) if self.policy_entropy_history else 0.0,
            'unique_regions_visited': len(self.region_viability)
        }

# Made with Bob
