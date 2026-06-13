"""
MAGEN V29 - Action Reputation System (C17)
Architecture Cognitive - Mémoire Décisionnelle Globale

OBJECTIF:
Transformer MAGEN d'un moteur transformationnel en agent cognitif
qui apprend quelles actions "méritent d'être pensées"

INNOVATION:
- Réputation globale par action (success_rate * (1 - avg_error))
- Priorisation basée sur historique complet
- Évite biais exploitation prématurée via exploration_bonus
- Tracking contexte (puzzle_type, grid_size, color_count)

IMPACT ATTENDU: +15-25 puzzles (174 → 189-199/400)
"""

import numpy as np
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass, field
from collections import defaultdict
import json


@dataclass
class ActionStats:
    """Statistiques complètes d'une action"""
    name: str
    attempts: int = 0
    successes: int = 0
    total_error: float = 0.0
    error_history: List[float] = field(default_factory=list)
    context_stats: Dict[str, Dict] = field(default_factory=lambda: defaultdict(dict))
    
    @property
    def success_rate(self) -> float:
        """Taux de succès global"""
        return self.successes / self.attempts if self.attempts > 0 else 0.0
    
    @property
    def avg_error(self) -> float:
        """Erreur moyenne"""
        return self.total_error / self.attempts if self.attempts > 0 else 1.0
    
    @property
    def reputation(self) -> float:
        """Réputation = success_rate * (1 - avg_error)"""
        return self.success_rate * (1.0 - self.avg_error)
    
    def update(self, success: bool, error: float, context: Optional[Dict] = None):
        """Mettre à jour statistiques après exécution"""
        self.attempts += 1
        if success:
            self.successes += 1
        self.total_error += error
        self.error_history.append(error)
        
        # Limiter historique à 1000 dernières exécutions
        if len(self.error_history) > 1000:
            self.error_history.pop(0)
        
        # Tracking contexte si fourni
        if context:
            ctx_key = self._compute_context_key(context)
            if ctx_key not in self.context_stats:
                self.context_stats[ctx_key] = {'attempts': 0, 'successes': 0, 'total_error': 0.0}
            
            self.context_stats[ctx_key]['attempts'] += 1
            if success:
                self.context_stats[ctx_key]['successes'] += 1
            self.context_stats[ctx_key]['total_error'] += error
    
    def _compute_context_key(self, context: Dict) -> str:
        """Calculer clé de contexte"""
        grid_size = context.get('grid_size', 'unknown')
        color_count = context.get('color_count', 'unknown')
        return f"size_{grid_size}_colors_{color_count}"
    
    def get_context_reputation(self, context: Dict) -> float:
        """Réputation spécifique au contexte"""
        ctx_key = self._compute_context_key(context)
        if ctx_key not in self.context_stats:
            return self.reputation  # Fallback sur réputation globale
        
        stats = self.context_stats[ctx_key]
        if stats['attempts'] == 0:
            return self.reputation
        
        success_rate = stats['successes'] / stats['attempts']
        avg_error = stats['total_error'] / stats['attempts']
        return success_rate * (1.0 - avg_error)


class ActionReputationSystem:
    """
    Système de réputation globale des actions
    
    PRINCIPE:
    - Chaque action accumule statistiques globales
    - Réputation = success_rate * (1 - avg_error)
    - Bonus exploration pour actions peu testées
    - Contexte-aware (taille grille, nombre couleurs)
    """
    
    def __init__(
        self,
        exploration_bonus: float = 0.1,
        min_attempts_threshold: int = 10,
        verbose: bool = False
    ):
        """
        Args:
            exploration_bonus: Bonus pour actions peu testées (évite exploitation prématurée)
            min_attempts_threshold: Seuil pour considérer statistiques fiables
            verbose: Mode verbeux
        """
        self.exploration_bonus = exploration_bonus
        self.min_attempts_threshold = min_attempts_threshold
        self.verbose = verbose
        
        # Statistiques par action
        self.action_stats: Dict[str, ActionStats] = {}
        
        # Historique global
        self.total_attempts = 0
        self.total_successes = 0
        
    def register_action(self, action_name: str):
        """Enregistrer une nouvelle action"""
        if action_name not in self.action_stats:
            self.action_stats[action_name] = ActionStats(name=action_name)
    
    def update(
        self,
        action_name: str,
        success: bool,
        error: float,
        context: Optional[Dict] = None
    ):
        """
        Mettre à jour réputation après exécution
        
        Args:
            action_name: Nom de l'action
            success: Succès ou échec
            error: Erreur (0.0 = parfait, 1.0 = échec total)
            context: Contexte optionnel (grid_size, color_count, etc.)
        """
        if action_name not in self.action_stats:
            self.register_action(action_name)
        
        self.action_stats[action_name].update(success, error, context)
        self.total_attempts += 1
        if success:
            self.total_successes += 1
    
    def get_action_priority(
        self,
        action_name: str,
        context: Optional[Dict] = None
    ) -> float:
        """
        Calculer priorité d'une action
        
        Formule:
        priority = reputation + exploration_bonus * (1 / sqrt(attempts + 1))
        
        Le bonus exploration décroît avec le nombre de tentatives,
        évitant l'exploitation prématurée des actions "faciles"
        
        Returns:
            Priorité entre 0.0 et 1.0+
        """
        if action_name not in self.action_stats:
            # Action jamais testée = priorité neutre + bonus exploration max
            return 0.5 + self.exploration_bonus
        
        stats = self.action_stats[action_name]
        
        # Réputation (contexte-aware si fourni)
        if context:
            reputation = stats.get_context_reputation(context)
        else:
            reputation = stats.reputation
        
        # Bonus exploration (décroît avec sqrt(attempts))
        exploration = self.exploration_bonus * (1.0 / np.sqrt(stats.attempts + 1))
        
        priority = reputation + exploration
        
        return priority
    
    def select_actions(
        self,
        candidates: List[str],
        top_k: int = 5,
        context: Optional[Dict] = None
    ) -> List[str]:
        """
        Sélectionner top-k actions par priorité
        
        Args:
            candidates: Liste d'actions candidates
            top_k: Nombre d'actions à sélectionner
            context: Contexte optionnel
        
        Returns:
            Liste des top-k actions triées par priorité décroissante
        """
        # Calculer priorités
        priorities = [
            (action, self.get_action_priority(action, context))
            for action in candidates
        ]
        
        # Trier par priorité décroissante
        priorities.sort(key=lambda x: x[1], reverse=True)
        
        # Sélectionner top-k
        selected = [action for action, _ in priorities[:top_k]]
        
        if self.verbose and len(selected) > 0:
            print(f"[C17 REPUTATION] Top-{top_k} actions:")
            for i, (action, priority) in enumerate(priorities[:top_k]):
                stats = self.action_stats.get(action)
                if stats:
                    print(f"  {i+1}. {action:20s}: priority={priority:.3f} "
                          f"(rep={stats.reputation:.3f}, attempts={stats.attempts})")
                else:
                    print(f"  {i+1}. {action:20s}: priority={priority:.3f} (new)")
        
        return selected if selected else []
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques complètes du système"""
        return {
            'total_attempts': self.total_attempts,
            'total_successes': self.total_successes,
            'global_success_rate': self.total_successes / self.total_attempts if self.total_attempts > 0 else 0.0,
            'actions': {
                name: {
                    'attempts': stats.attempts,
                    'successes': stats.successes,
                    'success_rate': stats.success_rate,
                    'avg_error': stats.avg_error,
                    'reputation': stats.reputation,
                    'priority': self.get_action_priority(name)
                }
                for name, stats in self.action_stats.items()
            }
        }
    
    def save_to_file(self, filepath: str):
        """Sauvegarder statistiques dans fichier JSON"""
        stats = self.get_statistics()
        with open(filepath, 'w') as f:
            json.dump(stats, f, indent=2)
    
    def should_blacklist(self, action_name: str, threshold: float = 0.1) -> bool:
        """
        Déterminer si une action devrait être blacklistée
        
        Args:
            action_name: Nom de l'action
            threshold: Seuil de réputation minimum
        
        Returns:
            True si action devrait être blacklistée
        """
        if action_name not in self.action_stats:
            return False
        
        stats = self.action_stats[action_name]
        
        # Blacklist si:
        # 1. Assez de tentatives pour statistiques fiables
        # 2. Réputation très faible
        if stats.attempts >= self.min_attempts_threshold and stats.reputation < threshold:
            if self.verbose:
                print(f"[C17 BLACKLIST] {action_name}: reputation={stats.reputation:.3f} < {threshold}")
            return True
        
        return False


# Export
__all__ = [
    'ActionReputationSystem',
    'ActionStats'
]

# Made with Bob
