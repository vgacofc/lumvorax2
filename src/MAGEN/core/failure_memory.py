#!/usr/bin/env python3
"""
Failure Memory - Mémoire des échecs et actions dangereuses
Inspiré par observation jeux arcade: "Je me souviens que si je fais ce mouvement je perds"
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass
from datetime import datetime
import hashlib


@dataclass
class FailureRecord:
    """Enregistrement d'un échec"""
    state_hash: str
    action: any
    reason: str
    timestamp: str
    frequency: int = 1
    
    def to_dict(self):
        return {
            'state_hash': self.state_hash,
            'action': str(self.action),
            'reason': self.reason,
            'timestamp': self.timestamp,
            'frequency': self.frequency
        }


class FailureMemory:
    """
    Mémoire court-terme des actions dangereuses/mortelles
    
    Inspiré par comportement humain jeux arcade:
    - "Je me souviens que si je fais ce mouvement je perds"
    - Évite répétition erreurs immédiates
    - Pruning intelligent espace recherche
    """
    
    def __init__(self, max_size: int = 1000):
        self.max_size = max_size
        
        # Mémoire principale: (state_hash, action) → FailureRecord
        self.failures: Dict[Tuple[str, str], FailureRecord] = {}
        
        # Index par raison d'échec
        self.by_reason: Dict[str, List[FailureRecord]] = {}
        
        # Actions globalement dangereuses (indépendantes de l'état)
        self.dangerous_actions: Set[str] = set()
        
        # Statistiques
        self.total_failures_recorded = 0
        self.total_dangers_avoided = 0
    
    def compute_state_hash(self, state: any) -> str:
        """
        Calcule hash unique d'un état
        
        Pour grilles numpy: hash du contenu
        Pour autres: hash de la représentation string
        """
        if isinstance(state, np.ndarray):
            return hashlib.sha256(state.tobytes()).hexdigest()[:16]
        else:
            return hashlib.sha256(str(state).encode()).hexdigest()[:16]
    
    def remember_failure(self, state: any, action: any, reason: str):
        """
        Mémorise un échec (action mortelle)
        
        Args:
            state: État où l'action a échoué
            action: Action qui a causé l'échec
            reason: Raison de l'échec
        """
        state_hash = self.compute_state_hash(state)
        action_str = str(action)
        key = (state_hash, action_str)
        
        if key in self.failures:
            # Incrémenter fréquence si déjà vu
            self.failures[key].frequency += 1
        else:
            # Nouveau échec
            record = FailureRecord(
                state_hash=state_hash,
                action=action,
                reason=reason,
                timestamp=datetime.now().isoformat(),
                frequency=1
            )
            self.failures[key] = record
            
            # Indexer par raison
            if reason not in self.by_reason:
                self.by_reason[reason] = []
            self.by_reason[reason].append(record)
        
        self.total_failures_recorded += 1
        
        # Si action échoue fréquemment, la marquer comme globalement dangereuse
        if self.failures[key].frequency >= 3:
            self.dangerous_actions.add(action_str)
        
        # Limiter taille mémoire
        if len(self.failures) > self.max_size:
            self._cleanup_old_failures()
    
    def is_dangerous(self, state: any, action: any) -> Tuple[bool, Optional[str]]:
        """
        Vérifie si une action est dangereuse dans un état donné
        
        Returns:
            (is_dangerous, reason)
        """
        state_hash = self.compute_state_hash(state)
        action_str = str(action)
        key = (state_hash, action_str)
        
        # Vérifier mémoire spécifique (state, action)
        if key in self.failures:
            record = self.failures[key]
            self.total_dangers_avoided += 1
            return True, f"Échec connu: {record.reason} (fréquence: {record.frequency}x)"
        
        # Vérifier actions globalement dangereuses
        if action_str in self.dangerous_actions:
            self.total_dangers_avoided += 1
            return True, f"Action globalement dangereuse (échecs multiples)"
        
        return False, None
    
    def get_safe_actions(self, state: any, all_actions: List[any]) -> List[any]:
        """
        Filtre actions pour retourner seulement les sûres
        
        Args:
            state: État actuel
            all_actions: Toutes les actions possibles
            
        Returns:
            Liste actions sûres (non-dangereuses)
        """
        safe = []
        for action in all_actions:
            is_dangerous, _ = self.is_dangerous(state, action)
            if not is_dangerous:
                safe.append(action)
        
        # Si toutes dangereuses, retourner toutes (éviter blocage)
        if len(safe) == 0:
            return all_actions
        
        return safe
    
    def get_failures_by_reason(self, reason: str) -> List[FailureRecord]:
        """Retourne tous les échecs pour une raison donnée"""
        return self.by_reason.get(reason, [])
    
    def get_most_frequent_failures(self, top_k: int = 10) -> List[FailureRecord]:
        """Retourne les échecs les plus fréquents"""
        sorted_failures = sorted(
            self.failures.values(),
            key=lambda r: r.frequency,
            reverse=True
        )
        return sorted_failures[:top_k]
    
    def _cleanup_old_failures(self):
        """Nettoie les échecs les plus anciens pour limiter mémoire"""
        # Garder seulement les plus fréquents
        sorted_failures = sorted(
            self.failures.items(),
            key=lambda item: item[1].frequency,
            reverse=True
        )
        
        # Garder top 80%
        keep_count = int(self.max_size * 0.8)
        self.failures = dict(sorted_failures[:keep_count])
        
        # Reconstruire index by_reason
        self.by_reason = {}
        for record in self.failures.values():
            if record.reason not in self.by_reason:
                self.by_reason[record.reason] = []
            self.by_reason[record.reason].append(record)
    
    def clear(self):
        """Efface toute la mémoire"""
        self.failures.clear()
        self.by_reason.clear()
        self.dangerous_actions.clear()
    
    def get_stats(self) -> Dict:
        """Statistiques mémoire échecs"""
        return {
            'total_failures_recorded': self.total_failures_recorded,
            'unique_failures': len(self.failures),
            'dangerous_actions': len(self.dangerous_actions),
            'dangers_avoided': self.total_dangers_avoided,
            'failure_reasons': len(self.by_reason),
            'avoidance_rate': self.total_dangers_avoided / max(self.total_failures_recorded, 1)
        }
    
    def export_knowledge(self) -> Dict:
        """Exporte connaissance pour analyse"""
        return {
            'failures': [record.to_dict() for record in self.failures.values()],
            'dangerous_actions': list(self.dangerous_actions),
            'by_reason': {
                reason: [r.to_dict() for r in records]
                for reason, records in self.by_reason.items()
            },
            'stats': self.get_stats()
        }


# Exemple d'utilisation
if __name__ == "__main__":
    # Test avec grilles numpy
    failure_mem = FailureMemory()
    
    # Simuler échecs
    grid1 = np.array([[1, 2], [3, 4]])
    grid2 = np.array([[5, 6], [7, 8]])
    
    # Mémoriser échecs
    failure_mem.remember_failure(grid1, "move_right", "collision_wall")
    failure_mem.remember_failure(grid1, "move_right", "collision_wall")  # Répétition
    failure_mem.remember_failure(grid2, "move_down", "enemy_hit")
    
    # Vérifier dangers
    is_dangerous, reason = failure_mem.is_dangerous(grid1, "move_right")
    print(f"move_right sur grid1: dangerous={is_dangerous}, reason={reason}")
    
    # Filtrer actions sûres
    all_actions = ["move_right", "move_left", "move_up", "move_down"]
    safe = failure_mem.get_safe_actions(grid1, all_actions)
    print(f"Actions sûres: {safe}")
    
    # Stats
    print(f"Stats: {failure_mem.get_stats()}")

# Made with Bob
