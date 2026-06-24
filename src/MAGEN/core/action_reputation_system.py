"""
Action Reputation System (C17) - Mémoire de Valeur des Actions

Ce module implémente la mémoire décisionnelle globale sur la qualité des actions.
Il permet à MAGEN d'apprendre quelles transformations "méritent d'être pensées".

Architecture Cognitive:
- Mémoire de valeur des actions (success_rate, avg_error)
- Priorisation par réputation historique
- Bonus de diversité (anti-exploitation prématurée)
- Forensic LEÇON-73.1 standard

Formule Cognitive:
    priority = success_rate * (1.0 - avg_error) * diversity_bonus
    diversity_bonus = 1.0 / (1.0 + recent_uses)

Auteur: Bob (IA) + Validation Utilisateur Expert
Date: 2026-06-15
Version: V38.0 (Architecture Cognitive)
"""

import json
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from collections import defaultdict, deque


@dataclass
class ActionStats:
    """Statistiques d'une action."""
    action_id: str
    attempts: int = 0
    successes: int = 0
    failures: int = 0
    total_error: float = 0.0
    total_cost: float = 0.0
    recent_uses: int = 0  # Fenêtre glissante
    first_seen: float = 0.0
    last_used: float = 0.0
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)
    
    @classmethod
    def from_dict(cls, data: dict) -> 'ActionStats':
        """Crée depuis dictionnaire."""
        return cls(**data)


@dataclass
class ReputationScore:
    """Score de réputation d'une action."""
    action_id: str
    priority: float
    success_rate: float
    avg_error: float
    avg_cost: float
    diversity_bonus: float
    confidence: float  # Basé sur nombre d'essais
    timestamp: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)


class ActionReputationSystem:
    """
    Système de réputation des actions (C17).
    
    Fonction Cognitive: Mémoire de valeur des actions
    Question: "Cette action vaut-elle le coup ?"
    
    Capacités:
    - Apprentissage historique (success_rate, avg_error)
    - Priorisation dynamique (priority score)
    - Anti-exploitation (diversity_bonus)
    - Confiance adaptative (basée sur attempts)
    """
    
    def __init__(
        self,
        forensic_log_path: Optional[str] = None,
        recent_window_size: int = 10,
        min_confidence_attempts: int = 5
    ):
        """
        Initialise le système de réputation.
        
        Args:
            forensic_log_path: Chemin du log forensic
            recent_window_size: Taille fenêtre glissante pour recent_uses
            min_confidence_attempts: Nombre minimum d'essais pour confiance élevée
        """
        self.action_stats: Dict[str, ActionStats] = {}
        self.global_history: List[dict] = []
        self.recent_window: deque = deque(maxlen=recent_window_size)
        self.min_confidence_attempts = min_confidence_attempts
        
        # Forensic LEÇON-73.1
        self.forensic_log_path = forensic_log_path
        if forensic_log_path:
            Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
        self._log_forensic("system_initialized", {
            "recent_window_size": recent_window_size,
            "min_confidence_attempts": min_confidence_attempts
        })
    
    def _log_forensic(self, event_type: str, data: dict):
        """Log événement forensic (LEÇON-73.1)."""
        if not self.forensic_log_path:
            return
        
        event = {
            "timestamp": time.time(),
            "module": "ActionReputationSystem",
            "event_type": event_type,
            "data": data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def record_action_attempt(
        self,
        action_id: str,
        success: bool,
        error: float,
        cost: float = 1.0
    ):
        """
        Enregistre une tentative d'action.
        
        Args:
            action_id: Identifiant de l'action
            success: Succès ou échec
            error: Erreur mesurée (0.0 = parfait, 1.0 = maximum)
            cost: Coût computationnel (steps, temps, etc.)
        """
        # Créer stats si première fois
        if action_id not in self.action_stats:
            self.action_stats[action_id] = ActionStats(
                action_id=action_id,
                first_seen=time.time()
            )
        
        stats = self.action_stats[action_id]
        
        # Mettre à jour statistiques
        stats.attempts += 1
        if success:
            stats.successes += 1
        else:
            stats.failures += 1
        stats.total_error += error
        stats.total_cost += cost
        stats.last_used = time.time()
        
        # Mettre à jour fenêtre glissante
        self.recent_window.append(action_id)
        stats.recent_uses = sum(1 for a in self.recent_window if a == action_id)
        
        # Historique global
        self.global_history.append({
            "timestamp": time.time(),
            "action_id": action_id,
            "success": success,
            "error": error,
            "cost": cost
        })
        
        # Forensic
        self._log_forensic("action_attempt_recorded", {
            "action_id": action_id,
            "success": success,
            "error": error,
            "cost": cost,
            "total_attempts": stats.attempts,
            "success_rate": stats.successes / stats.attempts if stats.attempts > 0 else 0.0
        })
    
    def compute_priority(self, action_id: str) -> ReputationScore:
        """
        Calcule le score de priorité d'une action.
        
        Formule Cognitive (PHASE 4.7.1 - Bonus Exploration):
            priority = success_rate * (1.0 - avg_error) * diversity_bonus + exploration_bonus
            diversity_bonus = 1.0 / (1.0 + recent_uses)
            exploration_bonus = 0.5 / (1.0 + attempts/10)  # Fort en début
        
        Args:
            action_id: Identifiant de l'action
            
        Returns:
            ReputationScore avec priority, success_rate, avg_error, etc.
        """
        # Action inconnue → priorité HAUTE pour encourager exploration
        if action_id not in self.action_stats:
            return ReputationScore(
                action_id=action_id,
                priority=0.8,  # PHASE 4.7.1: 0.5→0.8 (encourager nouvelles actions)
                success_rate=0.0,
                avg_error=0.5,
                avg_cost=1.0,
                diversity_bonus=1.0,
                confidence=0.0,  # Aucune donnée
                timestamp=time.time()
            )
        
        stats = self.action_stats[action_id]
        
        # Calcul métriques
        success_rate = stats.successes / stats.attempts if stats.attempts > 0 else 0.0
        avg_error = stats.total_error / stats.attempts if stats.attempts > 0 else 0.5
        avg_cost = stats.total_cost / stats.attempts if stats.attempts > 0 else 1.0
        
        # Bonus diversité (anti-exploitation)
        diversity_bonus = 1.0 / (1.0 + stats.recent_uses)
        
        # PHASE 4.7.1: Bonus exploration (décroît avec tentatives)
        exploration_bonus = 0.5 / (1.0 + stats.attempts / 10.0)
        
        # Formule cognitive avec exploration
        priority = success_rate * (1.0 - avg_error) * diversity_bonus + exploration_bonus
        
        # Confiance (basée sur nombre d'essais)
        confidence = min(1.0, stats.attempts / self.min_confidence_attempts)
        
        score = ReputationScore(
            action_id=action_id,
            priority=priority,
            success_rate=success_rate,
            avg_error=avg_error,
            avg_cost=avg_cost,
            diversity_bonus=diversity_bonus,
            confidence=confidence,
            timestamp=time.time()
        )
        
        # Forensic
        self._log_forensic("priority_computed", score.to_dict())
        
        return score
    
    def get_top_actions(self, n: int = 5) -> List[ReputationScore]:
        """
        Retourne les N actions avec la meilleure priorité.
        
        Args:
            n: Nombre d'actions à retourner
            
        Returns:
            Liste de ReputationScore triée par priorité décroissante
        """
        scores = [self.compute_priority(action_id) for action_id in self.action_stats.keys()]
        scores.sort(key=lambda s: s.priority, reverse=True)
        
        top_scores = scores[:n]
        
        # Forensic
        self._log_forensic("top_actions_retrieved", {
            "n": n,
            "top_actions": [s.action_id for s in top_scores],
            "top_priorities": [s.priority for s in top_scores]
        })
        
        return top_scores
    
    def get_worst_actions(self, n: int = 5) -> List[ReputationScore]:
        """
        Retourne les N actions avec la pire priorité.
        
        Args:
            n: Nombre d'actions à retourner
            
        Returns:
            Liste de ReputationScore triée par priorité croissante
        """
        scores = [self.compute_priority(action_id) for action_id in self.action_stats.keys()]
        scores.sort(key=lambda s: s.priority)
        
        worst_scores = scores[:n]
        
        # Forensic
        self._log_forensic("worst_actions_retrieved", {
            "n": n,
            "worst_actions": [s.action_id for s in worst_scores],
            "worst_priorities": [s.priority for s in worst_scores]
        })
        
        return worst_scores
    
    def should_explore_action(self, action_id: str, exploration_threshold: float = 0.3) -> bool:
        """
        Détermine si une action devrait être explorée.
        
        Args:
            action_id: Identifiant de l'action
            exploration_threshold: Seuil de priorité minimum
            
        Returns:
            True si l'action mérite d'être explorée
        """
        score = self.compute_priority(action_id)
        
        # Exploration si:
        # 1. Priorité >= seuil
        # 2. OU confiance faible (besoin de plus de données)
        should_explore = (
            score.priority >= exploration_threshold or
            score.confidence < 0.5
        )
        
        # Forensic
        self._log_forensic("exploration_decision", {
            "action_id": action_id,
            "priority": score.priority,
            "confidence": score.confidence,
            "threshold": exploration_threshold,
            "should_explore": should_explore
        })
        
        return should_explore
    
    def get_statistics(self) -> dict:
        """
        Retourne statistiques globales du système.
        
        Returns:
            Dictionnaire avec métriques agrégées
        """
        if not self.action_stats:
            return {
                "total_actions": 0,
                "total_attempts": 0,
                "global_success_rate": 0.0,
                "global_avg_error": 0.0
            }
        
        total_attempts = sum(s.attempts for s in self.action_stats.values())
        total_successes = sum(s.successes for s in self.action_stats.values())
        total_error = sum(s.total_error for s in self.action_stats.values())
        
        stats = {
            "total_actions": len(self.action_stats),
            "total_attempts": total_attempts,
            "global_success_rate": total_successes / total_attempts if total_attempts > 0 else 0.0,
            "global_avg_error": total_error / total_attempts if total_attempts > 0 else 0.0,
            "history_length": len(self.global_history)
        }
        
        # Forensic
        self._log_forensic("statistics_computed", stats)
        
        return stats
    
    def reset(self):
        """Réinitialise le système (pour tests)."""
        self.action_stats.clear()
        self.global_history.clear()
        self.recent_window.clear()
        
        self._log_forensic("system_reset", {})
    
    def save_state(self, filepath: str):
        """
        Sauvegarde l'état du système.
        
        Args:
            filepath: Chemin du fichier de sauvegarde
        """
        state = {
            "action_stats": {k: v.to_dict() for k, v in self.action_stats.items()},
            "global_history": self.global_history,
            "recent_window": list(self.recent_window)
        }
        
        Path(filepath).parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            json.dump(state, f, indent=2)
        
        self._log_forensic("state_saved", {"filepath": filepath})
    
    def load_state(self, filepath: str):
        """
        Charge l'état du système.
        
        Args:
            filepath: Chemin du fichier de sauvegarde
        """
        with open(filepath, 'r') as f:
            state = json.load(f)
        
        self.action_stats = {
            k: ActionStats.from_dict(v) 
            for k, v in state["action_stats"].items()
        }
        self.global_history = state["global_history"]
        self.recent_window = deque(state["recent_window"], maxlen=self.recent_window.maxlen)
        
        self._log_forensic("state_loaded", {"filepath": filepath})


# Tests unitaires
if __name__ == "__main__":
    import tempfile
    
    print("=== Test ActionReputationSystem (C17) ===\n")
    
    # Créer système
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    
    system = ActionReputationSystem(
        forensic_log_path=log_path,
        recent_window_size=5,
        min_confidence_attempts=3
    )
    
    # Test 1: Enregistrer actions
    print("Test 1: Enregistrement actions")
    system.record_action_attempt("rotate_90", success=True, error=0.1, cost=1.0)
    system.record_action_attempt("rotate_90", success=True, error=0.2, cost=1.0)
    system.record_action_attempt("rotate_90", success=False, error=0.9, cost=1.0)
    system.record_action_attempt("identity", success=False, error=0.95, cost=0.5)
    system.record_action_attempt("identity", success=False, error=0.98, cost=0.5)
    system.record_action_attempt("flip_h", success=True, error=0.05, cost=1.5)
    print("✓ 6 actions enregistrées\n")
    
    # Test 2: Calcul priorités
    print("Test 2: Calcul priorités")
    score_rotate = system.compute_priority("rotate_90")
    score_identity = system.compute_priority("identity")
    score_flip = system.compute_priority("flip_h")
    
    print(f"rotate_90: priority={score_rotate.priority:.3f}, success_rate={score_rotate.success_rate:.3f}")
    print(f"identity: priority={score_identity.priority:.3f}, success_rate={score_identity.success_rate:.3f}")
    print(f"flip_h: priority={score_flip.priority:.3f}, success_rate={score_flip.success_rate:.3f}")
    print()
    
    # Test 3: Top actions
    print("Test 3: Top actions")
    top = system.get_top_actions(n=3)
    for i, score in enumerate(top, 1):
        print(f"{i}. {score.action_id}: priority={score.priority:.3f}")
    print()
    
    # Test 4: Décision exploration
    print("Test 4: Décision exploration")
    should_explore_rotate = system.should_explore_action("rotate_90", exploration_threshold=0.3)
    should_explore_identity = system.should_explore_action("identity", exploration_threshold=0.3)
    print(f"rotate_90: should_explore={should_explore_rotate}")
    print(f"identity: should_explore={should_explore_identity}")
    print()
    
    # Test 5: Statistiques
    print("Test 5: Statistiques globales")
    stats = system.get_statistics()
    print(f"Total actions: {stats['total_actions']}")
    print(f"Total attempts: {stats['total_attempts']}")
    print(f"Global success rate: {stats['global_success_rate']:.3f}")
    print(f"Global avg error: {stats['global_avg_error']:.3f}")
    print()
    
    # Test 6: Sauvegarde/Chargement
    print("Test 6: Sauvegarde/Chargement")
    with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
        state_path = f.name
    system.save_state(state_path)
    
    system2 = ActionReputationSystem()
    system2.load_state(state_path)
    stats2 = system2.get_statistics()
    print(f"État chargé: {stats2['total_actions']} actions, {stats2['total_attempts']} attempts")
    print()
    
    print("=== Tests C17 RÉUSSIS ===")
    print(f"Forensic log: {log_path}")

# Made with Bob
