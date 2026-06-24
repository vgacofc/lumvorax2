"""
MetaLearning - Couche 7 Architecture Systémique MAGEN

Méta-apprentissage pour adaptation stratégique et amélioration continue.

Principes:
- Apprentissage de stratégies efficaces entre épisodes
- Adaptation paramètres selon performance
- Mémorisation patterns réussis
- Transfert connaissances entre contextes similaires
- Optimisation continue de l'architecture

Intégration:
- Analyse performance toutes couches (1-6)
- Ajuste paramètres exploration/exploitation
- Sélectionne stratégies optimales
- Mémorise succès/échecs pour apprentissage futur

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.6
"""

import numpy as np
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, field
from collections import defaultdict, deque


@dataclass
class EpisodeMemory:
    """Mémoire d'un épisode"""
    episode_id: int
    total_reward: float
    steps: int
    victory: bool
    strategies_used: List[str]
    performance_metrics: Dict[str, float]
    key_decisions: List[Dict]
    timestamp: float


@dataclass
class Strategy:
    """Stratégie d'action"""
    name: str
    description: str
    parameters: Dict[str, float]
    success_rate: float = 0.0
    usage_count: int = 0
    average_reward: float = 0.0
    contexts: List[str] = field(default_factory=list)


class MetaLearning:
    """
    Méta-apprentissage pour adaptation stratégique
    
    Fonctionnalités:
    1. Mémorisation épisodes (succès/échecs)
    2. Analyse patterns performance
    3. Adaptation paramètres dynamique
    4. Sélection stratégies optimales
    5. Transfert apprentissage
    """
    
    def __init__(self):
        """Initialisation MetaLearning"""
        
        # Mémoire épisodes
        self.episode_memories: List[EpisodeMemory] = []
        self.max_memory_size = 100  # Garder 100 derniers épisodes
        
        # Stratégies disponibles
        self.strategies: Dict[str, Strategy] = self._initialize_strategies()
        self.current_strategy: Optional[str] = None
        
        # Paramètres adaptatifs
        self.adaptive_params = {
            'exploration_weight': 0.5,  # Poids exploration vs exploitation
            'planning_depth': 10,  # Profondeur planification
            'learning_rate': 0.1,  # Taux apprentissage règles
            'curiosity_bonus': 0.3,  # Bonus curiosité
        }
        
        # Historique performance
        self.performance_history: deque = deque(maxlen=50)
        self.reward_history: deque = deque(maxlen=50)
        self.victory_history: deque = deque(maxlen=50)
        
        # Patterns identifiés
        self.success_patterns: List[Dict] = []
        self.failure_patterns: List[Dict] = []
        
        # Statistiques
        self.total_episodes = 0
        self.total_victories = 0
        self.best_episode_reward = float('-inf')
        self.best_episode_id = -1
        
        # Seuils adaptation
        self.performance_threshold_good = 0.7  # Performance considérée bonne
        self.performance_threshold_bad = 0.3  # Performance considérée mauvaise
        self.adaptation_frequency = 5  # Adapter tous les N épisodes
        
    def _initialize_strategies(self) -> Dict[str, Strategy]:
        """Initialise stratégies disponibles"""
        strategies = {}
        
        # Stratégie 1: Exploration agressive
        strategies['aggressive_exploration'] = Strategy(
            name='aggressive_exploration',
            description='Exploration maximale, exploitation minimale',
            parameters={
                'epsilon': 0.9,
                'curiosity_weight': 0.8,
                'planning_weight': 0.2
            }
        )
        
        # Stratégie 2: Exploitation ciblée
        strategies['focused_exploitation'] = Strategy(
            name='focused_exploitation',
            description='Exploitation maximale des connaissances',
            parameters={
                'epsilon': 0.1,
                'curiosity_weight': 0.2,
                'planning_weight': 0.8
            }
        )
        
        # Stratégie 3: Équilibrée
        strategies['balanced'] = Strategy(
            name='balanced',
            description='Équilibre exploration/exploitation',
            parameters={
                'epsilon': 0.5,
                'curiosity_weight': 0.5,
                'planning_weight': 0.5
            }
        )
        
        # Stratégie 4: Adaptative
        strategies['adaptive'] = Strategy(
            name='adaptive',
            description='Adaptation dynamique selon contexte',
            parameters={
                'epsilon': 0.5,
                'curiosity_weight': 0.5,
                'planning_weight': 0.5,
                'adaptive': True
            }
        )
        
        return strategies
    
    def record_episode(
        self,
        episode_id: int,
        total_reward: float,
        steps: int,
        victory: bool,
        strategies_used: List[str],
        performance_metrics: Dict[str, float],
        key_decisions: Optional[List[Dict]] = None
    ):
        """
        Enregistre épisode dans mémoire
        
        Args:
            episode_id: ID épisode
            total_reward: Récompense totale
            steps: Nombre steps
            victory: Victoire ou non
            strategies_used: Stratégies utilisées
            performance_metrics: Métriques performance
            key_decisions: Décisions clés (optionnel)
        """
        import time
        
        memory = EpisodeMemory(
            episode_id=episode_id,
            total_reward=total_reward,
            steps=steps,
            victory=victory,
            strategies_used=strategies_used,
            performance_metrics=performance_metrics,
            key_decisions=key_decisions or [],
            timestamp=time.time()
        )
        
        self.episode_memories.append(memory)
        
        # Limiter taille mémoire
        if len(self.episode_memories) > self.max_memory_size:
            self.episode_memories.pop(0)
        
        # Mettre à jour historiques
        self.performance_history.append(performance_metrics.get('overall', 0.0))
        self.reward_history.append(total_reward)
        self.victory_history.append(1.0 if victory else 0.0)
        
        # Mettre à jour statistiques
        self.total_episodes += 1
        if victory:
            self.total_victories += 1
        
        if total_reward > self.best_episode_reward:
            self.best_episode_reward = total_reward
            self.best_episode_id = episode_id
        
        # Mettre à jour stratégies
        for strategy_name in strategies_used:
            if strategy_name in self.strategies:
                strategy = self.strategies[strategy_name]
                strategy.usage_count += 1
                
                # Mettre à jour taux succès
                old_avg = strategy.average_reward
                n = strategy.usage_count
                strategy.average_reward = (old_avg * (n-1) + total_reward) / n
                
                if victory:
                    strategy.success_rate = (
                        (strategy.success_rate * (n-1) + 1.0) / n
                    )
                else:
                    strategy.success_rate = (
                        strategy.success_rate * (n-1) / n
                    )
        
        # Identifier patterns
        if victory:
            self._identify_success_pattern(memory)
        else:
            self._identify_failure_pattern(memory)
        
        # Adapter si nécessaire
        if self.total_episodes % self.adaptation_frequency == 0:
            self.adapt_parameters()
    
    def _identify_success_pattern(self, memory: EpisodeMemory):
        """Identifie pattern de succès"""
        pattern = {
            'strategies': memory.strategies_used,
            'metrics': memory.performance_metrics,
            'steps': memory.steps,
            'reward': memory.total_reward
        }
        self.success_patterns.append(pattern)
        
        # Limiter taille
        if len(self.success_patterns) > 20:
            self.success_patterns.pop(0)
    
    def _identify_failure_pattern(self, memory: EpisodeMemory):
        """Identifie pattern d'échec"""
        pattern = {
            'strategies': memory.strategies_used,
            'metrics': memory.performance_metrics,
            'steps': memory.steps,
            'reward': memory.total_reward
        }
        self.failure_patterns.append(pattern)
        
        # Limiter taille
        if len(self.failure_patterns) > 20:
            self.failure_patterns.pop(0)
    
    def select_strategy(self, context: Dict) -> str:
        """
        Sélectionne stratégie optimale pour contexte
        
        CORRECTION PHASE 4.5.9: Logique inversée pour ls20
        - Performance faible → RÉDUIRE exploration (pas augmenter)
        - Dans ls20, échec = trop d'exploration, pas assez
        
        Args:
            context: Contexte actuel (métriques, état)
            
        Returns:
            Nom stratégie sélectionnée
        """
        # Analyser performance récente
        recent_performance = self._analyze_recent_performance()
        
        # CORRECTION: Si performance mauvaise → EXPLOITATION (pas exploration)
        # Dans ls20, échec = trop d'exploration qui interrompt les plans
        if recent_performance < self.performance_threshold_bad:
            return 'focused_exploitation'  # Changé de aggressive_exploration
        
        # Si performance bonne: maintenir équilibre
        if recent_performance > self.performance_threshold_good:
            return 'balanced'  # Changé de focused_exploitation
        
        # Sinon: sélectionner meilleure stratégie basée sur historique
        best_strategy = self._select_best_strategy_from_history()
        
        return best_strategy or 'balanced'
    
    def _analyze_recent_performance(self) -> float:
        """Analyse performance récente (10 derniers épisodes)"""
        if not self.performance_history:
            return 0.5  # Neutre
        
        recent = list(self.performance_history)[-10:]
        return float(np.mean(recent))
    
    def _select_best_strategy_from_history(self) -> Optional[str]:
        """Sélectionne meilleure stratégie basée sur historique"""
        if not self.strategies:
            return None
        
        # Trier stratégies par taux succès et reward moyen
        sorted_strategies = sorted(
            self.strategies.values(),
            key=lambda s: (s.success_rate, s.average_reward),
            reverse=True
        )
        
        # Retourner meilleure si utilisée au moins 3 fois
        if sorted_strategies[0].usage_count >= 3:
            return sorted_strategies[0].name
        
        return None
    
    def adapt_parameters(self):
        """Adapte paramètres selon performance"""
        recent_perf = self._analyze_recent_performance()
        recent_victories = sum(list(self.victory_history)[-10:]) / max(len(list(self.victory_history)[-10:]), 1)
        
        # Adapter exploration_weight
        if recent_perf < self.performance_threshold_bad:
            # Performance mauvaise: augmenter exploration
            self.adaptive_params['exploration_weight'] = min(
                0.9,
                self.adaptive_params['exploration_weight'] + 0.1
            )
        elif recent_perf > self.performance_threshold_good:
            # Performance bonne: réduire exploration
            self.adaptive_params['exploration_weight'] = max(
                0.1,
                self.adaptive_params['exploration_weight'] - 0.1
            )
        
        # Adapter curiosity_bonus
        if recent_victories < 0.1:
            # Pas de victoires: augmenter curiosité
            self.adaptive_params['curiosity_bonus'] = min(
                0.8,
                self.adaptive_params['curiosity_bonus'] + 0.1
            )
        
        # Adapter planning_depth
        if recent_perf > 0.6:
            # Bonne performance: augmenter profondeur planification
            self.adaptive_params['planning_depth'] = min(
                20,
                self.adaptive_params['planning_depth'] + 2
            )
    
    def get_adaptive_parameters(self) -> Dict[str, float]:
        """Retourne paramètres adaptatifs actuels"""
        return self.adaptive_params.copy()
    
    def get_statistics(self) -> Dict:
        """Statistiques méta-apprentissage"""
        recent_perf = self._analyze_recent_performance()
        recent_victories = (
            sum(list(self.victory_history)[-10:]) / max(len(list(self.victory_history)[-10:]), 1)
            if self.victory_history else 0.0
        )
        
        return {
            'total_episodes': self.total_episodes,
            'total_victories': self.total_victories,
            'victory_rate': self.total_victories / max(self.total_episodes, 1),
            'best_episode_reward': self.best_episode_reward,
            'best_episode_id': self.best_episode_id,
            'recent_performance': recent_perf,
            'recent_victory_rate': recent_victories,
            'success_patterns_identified': len(self.success_patterns),
            'failure_patterns_identified': len(self.failure_patterns),
            'strategies_available': len(self.strategies),
            'current_strategy': self.current_strategy,
            'adaptive_parameters': self.adaptive_params
        }
    
    def get_best_practices(self) -> List[Dict]:
        """Retourne meilleures pratiques identifiées"""
        if not self.success_patterns:
            return []
        
        # Analyser patterns succès pour identifier pratiques communes
        practices = []
        
        # Pratique 1: Stratégies les plus efficaces
        if self.success_patterns:
            strategy_counts = defaultdict(int)
            for pattern in self.success_patterns:
                for strategy in pattern['strategies']:
                    strategy_counts[strategy] += 1
            
            if strategy_counts:
                best_strategy = max(strategy_counts.items(), key=lambda x: x[1])
                practices.append({
                    'type': 'strategy',
                    'recommendation': f"Utiliser stratégie '{best_strategy[0]}'",
                    'evidence': f"Présente dans {best_strategy[1]}/{len(self.success_patterns)} succès"
                })
        
        return practices
    
    def reset_episode(self):
        """Reset pour nouvel épisode (garde mémoire long-terme)"""
        self.current_strategy = None


if __name__ == "__main__":
    print("MetaLearning - Couche 7 Architecture Systémique")
    print("=" * 60)
    print()
    print("Fonctionnalités:")
    print("- Mémorisation épisodes (succès/échecs)")
    print("- Adaptation paramètres dynamique")
    print("- Sélection stratégies optimales")
    print("- Identification patterns performance")
    print("- Transfert apprentissage")
    print()
    print("Intégration: Supervise et optimise toutes les couches 1-6")

# Made with Bob
