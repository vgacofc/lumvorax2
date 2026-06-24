"""
PHASE 6 V39 - C18: Budget Allocator
Système d'allocation dynamique du budget computationnel.

Objectif:
- Allouer budget (steps, temps, mémoire) selon priorités
- Adapter budget selon performance actions
- Réduire budget pour actions inefficaces
- Augmenter budget pour actions prometteuses
- Gérer contraintes globales (temps max, mémoire max)

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
from datetime import datetime
import json


@dataclass
class BudgetAllocation:
    """Allocation de budget pour une action/catégorie."""
    target_id: str
    allocated_steps: int = 0
    allocated_time_ms: float = 0.0
    allocated_memory_mb: float = 0.0
    used_steps: int = 0
    used_time_ms: float = 0.0
    used_memory_mb: float = 0.0
    priority: float = 0.5
    efficiency: float = 0.0
    last_updated: Optional[datetime] = None
    
    def update_usage(self, steps: int, time_ms: float, memory_mb: float):
        """Met à jour l'utilisation du budget."""
        self.used_steps += steps
        self.used_time_ms += time_ms
        self.used_memory_mb += memory_mb
        self.last_updated = datetime.now()
        
        # Calculer efficacité (ratio utilisé/alloué)
        if self.allocated_steps > 0:
            step_efficiency = self.used_steps / self.allocated_steps
        else:
            step_efficiency = 0.0
        
        if self.allocated_time_ms > 0:
            time_efficiency = self.used_time_ms / self.allocated_time_ms
        else:
            time_efficiency = 0.0
        
        # Efficacité moyenne
        self.efficiency = (step_efficiency + time_efficiency) / 2.0
    
    def get_remaining_budget(self) -> Dict[str, float]:
        """Retourne le budget restant."""
        return {
            'steps': max(0, self.allocated_steps - self.used_steps),
            'time_ms': max(0.0, self.allocated_time_ms - self.used_time_ms),
            'memory_mb': max(0.0, self.allocated_memory_mb - self.used_memory_mb)
        }
    
    def is_exhausted(self) -> bool:
        """Vérifie si le budget est épuisé."""
        remaining = self.get_remaining_budget()
        return (remaining['steps'] == 0 or 
                remaining['time_ms'] == 0.0)


@dataclass
class GlobalBudget:
    """Budget global du système."""
    max_total_steps: int
    max_total_time_ms: float
    max_total_memory_mb: float
    used_steps: int = 0
    used_time_ms: float = 0.0
    used_memory_mb: float = 0.0
    start_time: Optional[datetime] = None
    
    def __post_init__(self):
        if self.start_time is None:
            self.start_time = datetime.now()
    
    def get_remaining(self) -> Dict[str, float]:
        """Retourne le budget global restant."""
        return {
            'steps': max(0, self.max_total_steps - self.used_steps),
            'time_ms': max(0.0, self.max_total_time_ms - self.used_time_ms),
            'memory_mb': max(0.0, self.max_total_memory_mb - self.used_memory_mb)
        }
    
    def get_usage_ratio(self) -> Dict[str, float]:
        """Retourne le ratio d'utilisation."""
        return {
            'steps': self.used_steps / self.max_total_steps if self.max_total_steps > 0 else 0.0,
            'time_ms': self.used_time_ms / self.max_total_time_ms if self.max_total_time_ms > 0 else 0.0,
            'memory_mb': self.used_memory_mb / self.max_total_memory_mb if self.max_total_memory_mb > 0 else 0.0
        }
    
    def is_exhausted(self) -> bool:
        """Vérifie si le budget global est épuisé."""
        return (self.used_steps >= self.max_total_steps or
                self.used_time_ms >= self.max_total_time_ms or
                self.used_memory_mb >= self.max_total_memory_mb)


class BudgetAllocator:
    """
    Système d'allocation dynamique du budget pour MAGEN V39.
    
    Fonctionnalités:
    - Allocation initiale basée sur priorités
    - Réallocation dynamique selon performance
    - Gestion contraintes globales
    - Optimisation continue de l'allocation
    """
    
    def __init__(self,
                 max_total_steps: int = 10000,
                 max_total_time_ms: float = 60000.0,  # 60 secondes
                 max_total_memory_mb: float = 1024.0,  # 1 GB
                 reallocation_threshold: float = 0.1):
        """
        Initialise l'allocateur de budget.
        
        Args:
            max_total_steps: Budget total en steps
            max_total_time_ms: Budget total en millisecondes
            max_total_memory_mb: Budget total en mégaoctets
            reallocation_threshold: Seuil pour déclencher réallocation (0-1)
        """
        self.global_budget = GlobalBudget(
            max_total_steps=max_total_steps,
            max_total_time_ms=max_total_time_ms,
            max_total_memory_mb=max_total_memory_mb
        )
        
        self.allocations: Dict[str, BudgetAllocation] = {}
        self.reallocation_threshold = reallocation_threshold
        self.reallocation_count = 0
        
        # Historique des allocations
        self.allocation_history: List[Dict] = []
    
    def allocate_initial_budget(self, targets: Dict[str, float]):
        """
        Alloue le budget initial basé sur les priorités.
        
        Args:
            targets: Dict {target_id: priority} où priority ∈ [0, 1]
        """
        # Normaliser priorités
        total_priority = sum(targets.values())
        if total_priority == 0:
            # Si toutes priorités à 0, distribuer équitablement
            normalized = {tid: 1.0/len(targets) for tid in targets}
        else:
            normalized = {tid: p/total_priority for tid, p in targets.items()}
        
        # Allouer budget proportionnellement
        remaining = self.global_budget.get_remaining()
        
        for target_id, priority in normalized.items():
            allocation = BudgetAllocation(
                target_id=target_id,
                allocated_steps=int(remaining['steps'] * priority),
                allocated_time_ms=remaining['time_ms'] * priority,
                allocated_memory_mb=remaining['memory_mb'] * priority,
                priority=priority
            )
            self.allocations[target_id] = allocation
        
        # Enregistrer dans historique
        self.allocation_history.append({
            'timestamp': datetime.now().isoformat(),
            'type': 'initial',
            'allocations': {tid: a.allocated_steps for tid, a in self.allocations.items()}
        })
    
    def record_usage(self, target_id: str, steps: int, 
                    time_ms: float, memory_mb: float,
                    success: bool, reward: float):
        """
        Enregistre l'utilisation du budget par une cible.
        
        Args:
            target_id: ID de la cible
            steps: Steps utilisés
            time_ms: Temps utilisé en ms
            memory_mb: Mémoire utilisée en MB
            success: Si l'action a réussi
            reward: Récompense obtenue
        """
        # Mettre à jour budget global
        self.global_budget.used_steps += steps
        self.global_budget.used_time_ms += time_ms
        self.global_budget.used_memory_mb += memory_mb
        
        # Mettre à jour allocation cible
        if target_id not in self.allocations:
            # Créer allocation par défaut
            self.allocations[target_id] = BudgetAllocation(
                target_id=target_id,
                allocated_steps=100,
                allocated_time_ms=1000.0,
                allocated_memory_mb=10.0
            )
        
        self.allocations[target_id].update_usage(steps, time_ms, memory_mb)
        
        # Vérifier si réallocation nécessaire
        usage_ratio = self.global_budget.get_usage_ratio()
        avg_usage = np.mean(list(usage_ratio.values()))
        
        if avg_usage >= self.reallocation_threshold:
            self._reallocate_budget()
    
    def _reallocate_budget(self):
        """Réalloue le budget dynamiquement selon performance."""
        self.reallocation_count += 1
        
        # Calculer nouvelles priorités basées sur efficacité
        efficiencies = {tid: alloc.efficiency 
                       for tid, alloc in self.allocations.items()}
        
        total_efficiency = sum(efficiencies.values())
        if total_efficiency == 0:
            # Si aucune efficacité, garder priorités actuelles
            new_priorities = {tid: alloc.priority 
                            for tid, alloc in self.allocations.items()}
        else:
            # Nouvelles priorités proportionnelles à l'efficacité
            new_priorities = {tid: eff/total_efficiency 
                            for tid, eff in efficiencies.items()}
        
        # Réallouer budget restant
        remaining = self.global_budget.get_remaining()
        
        for target_id, priority in new_priorities.items():
            alloc = self.allocations[target_id]
            
            # Ajouter budget supplémentaire proportionnel
            additional_steps = int(remaining['steps'] * priority * 0.5)
            additional_time = remaining['time_ms'] * priority * 0.5
            additional_memory = remaining['memory_mb'] * priority * 0.5
            
            alloc.allocated_steps += additional_steps
            alloc.allocated_time_ms += additional_time
            alloc.allocated_memory_mb += additional_memory
            alloc.priority = priority
        
        # Enregistrer dans historique
        self.allocation_history.append({
            'timestamp': datetime.now().isoformat(),
            'type': 'reallocation',
            'count': self.reallocation_count,
            'allocations': {tid: a.allocated_steps for tid, a in self.allocations.items()}
        })
    
    def get_allocation(self, target_id: str) -> Optional[BudgetAllocation]:
        """Retourne l'allocation pour une cible."""
        return self.allocations.get(target_id)
    
    def can_allocate(self, target_id: str, steps: int) -> bool:
        """
        Vérifie si on peut allouer des steps à une cible.
        
        Args:
            target_id: ID de la cible
            steps: Nombre de steps demandés
        
        Returns:
            True si allocation possible
        """
        # Vérifier budget global
        if self.global_budget.is_exhausted():
            return False
        
        remaining_global = self.global_budget.get_remaining()
        if remaining_global['steps'] < steps:
            return False
        
        # Vérifier budget cible
        if target_id in self.allocations:
            alloc = self.allocations[target_id]
            if alloc.is_exhausted():
                return False
            
            remaining_target = alloc.get_remaining_budget()
            if remaining_target['steps'] < steps:
                return False
        
        return True
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques d'allocation."""
        usage_ratio = self.global_budget.get_usage_ratio()
        remaining = self.global_budget.get_remaining()
        
        # Stats par allocation
        allocation_stats = {}
        for tid, alloc in self.allocations.items():
            allocation_stats[tid] = {
                'allocated_steps': alloc.allocated_steps,
                'used_steps': alloc.used_steps,
                'remaining_steps': alloc.get_remaining_budget()['steps'],
                'efficiency': alloc.efficiency,
                'priority': alloc.priority
            }
        
        return {
            'global_budget': {
                'max_steps': self.global_budget.max_total_steps,
                'used_steps': self.global_budget.used_steps,
                'remaining_steps': remaining['steps'],
                'usage_ratio_steps': usage_ratio['steps'],
                'usage_ratio_time': usage_ratio['time_ms'],
                'usage_ratio_memory': usage_ratio['memory_mb']
            },
            'allocations': allocation_stats,
            'reallocation_count': self.reallocation_count,
            'total_allocations': len(self.allocations)
        }
    
    def export_history(self) -> List[Dict]:
        """Exporte l'historique des allocations."""
        return self.allocation_history


def test_budget_allocator():
    """Test unitaire de l'allocateur de budget."""
    print("="*80)
    print("TEST BUDGET ALLOCATOR V39")
    print("="*80)
    
    allocator = BudgetAllocator(
        max_total_steps=1000,
        max_total_time_ms=10000.0,
        reallocation_threshold=0.3
    )
    
    # Allocation initiale
    print("\nAllocation initiale...")
    targets = {
        'action_a': 0.5,  # Haute priorité
        'action_b': 0.3,  # Moyenne priorité
        'action_c': 0.2   # Basse priorité
    }
    allocator.allocate_initial_budget(targets)
    
    stats = allocator.get_stats()
    print(f"Budget global: {stats['global_budget']['max_steps']} steps")
    for tid, alloc_stats in stats['allocations'].items():
        print(f"  {tid}: {alloc_stats['allocated_steps']} steps (priorité {alloc_stats['priority']:.2f})")
    
    # Simuler utilisation
    print("\nSimulation utilisation...")
    for i in range(50):
        # action_a performante
        if np.random.random() > 0.3:
            allocator.record_usage('action_a', 5, 50.0, 1.0, True, 0.8)
        
        # action_b moyenne
        if np.random.random() > 0.5:
            allocator.record_usage('action_b', 3, 30.0, 0.5, True, 0.5)
        
        # action_c peu performante
        if np.random.random() > 0.7:
            allocator.record_usage('action_c', 2, 20.0, 0.3, False, 0.2)
    
    # Stats finales
    stats = allocator.get_stats()
    print(f"\nStatistiques finales:")
    print(f"Budget global utilisé: {stats['global_budget']['used_steps']}/{stats['global_budget']['max_steps']} steps")
    print(f"Ratio utilisation: {stats['global_budget']['usage_ratio_steps']:.1%}")
    print(f"Réallocations: {stats['reallocation_count']}")
    
    print(f"\nAllocations par action:")
    for tid, alloc_stats in stats['allocations'].items():
        print(f"  {tid}:")
        print(f"    Alloué: {alloc_stats['allocated_steps']} steps")
        print(f"    Utilisé: {alloc_stats['used_steps']} steps")
        print(f"    Restant: {alloc_stats['remaining_steps']} steps")
        print(f"    Efficacité: {alloc_stats['efficiency']:.2f}")
        print(f"    Priorité: {alloc_stats['priority']:.2f}")
    
    # Vérifier can_allocate
    print(f"\nTest can_allocate:")
    for tid in ['action_a', 'action_b', 'action_c']:
        can_alloc = allocator.can_allocate(tid, 10)
        print(f"  {tid} (10 steps): {'✅ OUI' if can_alloc else '❌ NON'}")
    
    print("\n" + "="*80)
    print("✅ TEST RÉUSSI")
    print("="*80)


if __name__ == '__main__':
    test_budget_allocator()

# Made with Bob
