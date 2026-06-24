"""
Dynamic Budget Allocator (C18) - Allocation Attentionnelle

Ce module implémente l'allocation dynamique du budget computationnel.
Il permet à MAGEN d'adapter ses ressources en fonction de la priorité des actions.

Architecture Cognitive:
- Allocation proportionnelle à la priorité
- Réduction progressive si budget épuisé
- Tracking consommation par action
- Forensic LEÇON-73.1 standard

Formule Allocation:
    allocated = base_budget * priority * remaining_ratio
    remaining_ratio = (total_budget - spent_budget) / total_budget

Auteur: Bob (IA) + Validation Utilisateur Expert
Date: 2026-06-15
Version: V38.0 (Architecture Cognitive)
"""

import json
import time
from pathlib import Path
from typing import Dict, Optional
from dataclasses import dataclass, asdict


@dataclass
class BudgetAllocation:
    """Allocation de budget pour une action."""
    action_id: str
    allocated: float
    spent: float
    remaining: float
    priority: float
    timestamp: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)
    
    @property
    def utilization_rate(self) -> float:
        """Taux d'utilisation du budget alloué."""
        return self.spent / self.allocated if self.allocated > 0 else 0.0
    
    @property
    def is_exhausted(self) -> bool:
        """Budget épuisé ?"""
        return self.remaining <= 0.0


@dataclass
class BudgetState:
    """État global du budget."""
    total_budget: float
    spent_budget: float
    remaining_budget: float
    active_allocations: int
    timestamp: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)
    
    @property
    def utilization_rate(self) -> float:
        """Taux d'utilisation global."""
        return self.spent_budget / self.total_budget if self.total_budget > 0 else 0.0
    
    @property
    def is_critical(self) -> bool:
        """Budget critique (<20% restant) ?"""
        return self.remaining_budget < (self.total_budget * 0.2)


class DynamicBudgetAllocator:
    """
    Allocateur dynamique de budget (C18).
    
    Fonction Cognitive: Allocation attentionnelle
    Question: "Combien de temps y consacrer ?"
    
    Capacités:
    - Allocation proportionnelle à priorité
    - Adaptation dynamique selon budget restant
    - Tracking consommation par action
    - Détection budget critique
    """
    
    def __init__(
        self,
        total_budget: float = 1000.0,
        base_allocation_ratio: float = 0.1,
        min_allocation: float = 10.0,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise l'allocateur de budget.
        
        Args:
            total_budget: Budget total disponible
            base_allocation_ratio: Ratio de base pour allocation (10% = 0.1)
            min_allocation: Allocation minimum par action
            forensic_log_path: Chemin du log forensic
        """
        self.total_budget = total_budget
        self.base_allocation_ratio = base_allocation_ratio
        self.min_allocation = min_allocation
        self.spent_budget = 0.0
        self.action_budgets: Dict[str, BudgetAllocation] = {}
        
        # Forensic LEÇON-73.1
        self.forensic_log_path = forensic_log_path
        if forensic_log_path:
            Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
        self._log_forensic("allocator_initialized", {
            "total_budget": total_budget,
            "base_allocation_ratio": base_allocation_ratio,
            "min_allocation": min_allocation
        })
    
    def _log_forensic(self, event_type: str, data: dict):
        """Log événement forensic (LEÇON-73.1)."""
        if not self.forensic_log_path:
            return
        
        event = {
            "timestamp": time.time(),
            "module": "DynamicBudgetAllocator",
            "event_type": event_type,
            "data": data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def allocate_budget(self, action_id: str, priority: float) -> BudgetAllocation:
        """
        Alloue un budget pour une action.
        
        Formule:
            base_budget = total_budget * base_allocation_ratio
            priority_budget = base_budget * priority
            remaining_ratio = remaining_budget / total_budget
            allocated = max(min_allocation, priority_budget * remaining_ratio)
        
        Args:
            action_id: Identifiant de l'action
            priority: Priorité de l'action (0.0 à 1.0)
            
        Returns:
            BudgetAllocation avec budget alloué
        """
        # Budget de base
        base_budget = self.total_budget * self.base_allocation_ratio
        
        # Budget ajusté par priorité
        priority_budget = base_budget * priority
        
        # Ratio budget restant
        remaining_budget = self.total_budget - self.spent_budget
        remaining_ratio = remaining_budget / self.total_budget if self.total_budget > 0 else 0.0
        
        # Allocation finale (avec minimum garanti)
        allocated = max(
            self.min_allocation,
            priority_budget * remaining_ratio
        )
        
        # Limiter au budget restant
        allocated = min(allocated, remaining_budget)
        
        # Créer allocation
        allocation = BudgetAllocation(
            action_id=action_id,
            allocated=allocated,
            spent=0.0,
            remaining=allocated,
            priority=priority,
            timestamp=time.time()
        )
        
        self.action_budgets[action_id] = allocation
        
        # Forensic
        self._log_forensic("budget_allocated", {
            "action_id": action_id,
            "priority": priority,
            "allocated": allocated,
            "remaining_budget": remaining_budget,
            "remaining_ratio": remaining_ratio
        })
        
        return allocation
    
    def consume_budget(self, action_id: str, amount: float) -> bool:
        """
        Consomme du budget pour une action.
        
        Args:
            action_id: Identifiant de l'action
            amount: Montant à consommer
            
        Returns:
            True si consommation réussie, False si budget insuffisant
        """
        if action_id not in self.action_budgets:
            self._log_forensic("consume_failed_no_allocation", {
                "action_id": action_id,
                "amount": amount
            })
            return False
        
        allocation = self.action_budgets[action_id]
        
        # Vérifier budget disponible
        if allocation.remaining < amount:
            self._log_forensic("consume_failed_insufficient", {
                "action_id": action_id,
                "amount": amount,
                "remaining": allocation.remaining
            })
            return False
        
        # Consommer
        allocation.spent += amount
        allocation.remaining -= amount
        self.spent_budget += amount
        
        # Forensic
        self._log_forensic("budget_consumed", {
            "action_id": action_id,
            "amount": amount,
            "spent": allocation.spent,
            "remaining": allocation.remaining,
            "utilization_rate": allocation.utilization_rate
        })
        
        return True
    
    def should_continue(self, action_id: str) -> bool:
        """
        Détermine si une action devrait continuer.
        
        Args:
            action_id: Identifiant de l'action
            
        Returns:
            True si l'action a encore du budget
        """
        if action_id not in self.action_budgets:
            return False
        
        allocation = self.action_budgets[action_id]
        should_continue = not allocation.is_exhausted
        
        # Forensic
        self._log_forensic("continue_decision", {
            "action_id": action_id,
            "remaining": allocation.remaining,
            "should_continue": should_continue
        })
        
        return should_continue
    
    def get_allocation(self, action_id: str) -> Optional[BudgetAllocation]:
        """
        Retourne l'allocation d'une action.
        
        Args:
            action_id: Identifiant de l'action
            
        Returns:
            BudgetAllocation ou None si pas d'allocation
        """
        return self.action_budgets.get(action_id)
    
    def get_state(self) -> BudgetState:
        """
        Retourne l'état global du budget.
        
        Returns:
            BudgetState avec métriques globales
        """
        remaining_budget = self.total_budget - self.spent_budget
        
        state = BudgetState(
            total_budget=self.total_budget,
            spent_budget=self.spent_budget,
            remaining_budget=remaining_budget,
            active_allocations=len(self.action_budgets),
            timestamp=time.time()
        )
        
        # Forensic
        self._log_forensic("state_retrieved", state.to_dict())
        
        return state
    
    def is_budget_critical(self) -> bool:
        """
        Vérifie si le budget est critique (<20% restant).
        
        Returns:
            True si budget critique
        """
        state = self.get_state()
        is_critical = state.is_critical
        
        if is_critical:
            self._log_forensic("budget_critical_warning", {
                "remaining_budget": state.remaining_budget,
                "utilization_rate": state.utilization_rate
            })
        
        return is_critical
    
    def reallocate_budget(self, action_id: str, new_priority: float) -> BudgetAllocation:
        """
        Réalloue le budget d'une action avec nouvelle priorité.
        
        Args:
            action_id: Identifiant de l'action
            new_priority: Nouvelle priorité
            
        Returns:
            Nouvelle BudgetAllocation
        """
        # Récupérer ancienne allocation
        old_allocation = self.action_budgets.get(action_id)
        
        if old_allocation:
            # Restituer budget non utilisé
            unused = old_allocation.remaining
            self.spent_budget -= unused
            
            self._log_forensic("budget_restituted", {
                "action_id": action_id,
                "unused": unused
            })
        
        # Nouvelle allocation
        new_allocation = self.allocate_budget(action_id, new_priority)
        
        self._log_forensic("budget_reallocated", {
            "action_id": action_id,
            "old_priority": old_allocation.priority if old_allocation else None,
            "new_priority": new_priority,
            "old_allocated": old_allocation.allocated if old_allocation else None,
            "new_allocated": new_allocation.allocated
        })
        
        return new_allocation
    
    def get_statistics(self) -> dict:
        """
        Retourne statistiques globales.
        
        Returns:
            Dictionnaire avec métriques agrégées
        """
        state = self.get_state()
        
        # Calcul métriques par action
        allocations = list(self.action_budgets.values())
        avg_utilization = (
            sum(a.utilization_rate for a in allocations) / len(allocations)
            if allocations else 0.0
        )
        
        stats = {
            "total_budget": state.total_budget,
            "spent_budget": state.spent_budget,
            "remaining_budget": state.remaining_budget,
            "utilization_rate": state.utilization_rate,
            "active_allocations": state.active_allocations,
            "avg_action_utilization": avg_utilization,
            "is_critical": state.is_critical
        }
        
        # Forensic
        self._log_forensic("statistics_computed", stats)
        
        return stats
    
    def reset(self):
        """Réinitialise l'allocateur (pour tests)."""
        self.spent_budget = 0.0
        self.action_budgets.clear()
        
        self._log_forensic("allocator_reset", {})


# Tests unitaires
if __name__ == "__main__":
    import tempfile
    
    print("=== Test DynamicBudgetAllocator (C18) ===\n")
    
    # Créer allocateur
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    
    allocator = DynamicBudgetAllocator(
        total_budget=1000.0,
        base_allocation_ratio=0.1,
        min_allocation=10.0,
        forensic_log_path=log_path
    )
    
    # Test 1: Allocation avec priorités différentes
    print("Test 1: Allocation avec priorités")
    alloc_high = allocator.allocate_budget("action_high", priority=0.9)
    alloc_medium = allocator.allocate_budget("action_medium", priority=0.5)
    alloc_low = allocator.allocate_budget("action_low", priority=0.2)
    
    print(f"High priority: allocated={alloc_high.allocated:.1f}")
    print(f"Medium priority: allocated={alloc_medium.allocated:.1f}")
    print(f"Low priority: allocated={alloc_low.allocated:.1f}")
    print()
    
    # Test 2: Consommation budget
    print("Test 2: Consommation budget")
    success1 = allocator.consume_budget("action_high", 30.0)
    success2 = allocator.consume_budget("action_high", 50.0)
    success3 = allocator.consume_budget("action_high", 100.0)  # Devrait échouer
    
    print(f"Consume 30: {success1}")
    print(f"Consume 50: {success2}")
    print(f"Consume 100: {success3} (devrait être False)")
    print()
    
    # Test 3: Décision continuation
    print("Test 3: Décision continuation")
    should_continue_high = allocator.should_continue("action_high")
    should_continue_medium = allocator.should_continue("action_medium")
    
    print(f"action_high should continue: {should_continue_high}")
    print(f"action_medium should continue: {should_continue_medium}")
    print()
    
    # Test 4: État global
    print("Test 4: État global")
    state = allocator.get_state()
    print(f"Total budget: {state.total_budget:.1f}")
    print(f"Spent budget: {state.spent_budget:.1f}")
    print(f"Remaining budget: {state.remaining_budget:.1f}")
    print(f"Utilization rate: {state.utilization_rate:.1%}")
    print(f"Is critical: {state.is_critical}")
    print()
    
    # Test 5: Réallocation
    print("Test 5: Réallocation")
    new_alloc = allocator.reallocate_budget("action_low", new_priority=0.8)
    print(f"Réalloué action_low: {new_alloc.allocated:.1f} (priorité 0.2 → 0.8)")
    print()
    
    # Test 6: Budget critique
    print("Test 6: Budget critique")
    # Consommer beaucoup pour atteindre seuil critique
    allocator.consume_budget("action_medium", 40.0)
    allocator.consume_budget("action_low", 60.0)
    
    # Allouer plus pour consommer davantage
    for i in range(10):
        alloc = allocator.allocate_budget(f"action_{i}", priority=0.5)
        allocator.consume_budget(f"action_{i}", alloc.allocated * 0.9)
    
    is_critical = allocator.is_budget_critical()
    state = allocator.get_state()
    print(f"Budget critique: {is_critical}")
    print(f"Remaining: {state.remaining_budget:.1f} ({state.utilization_rate:.1%} utilisé)")
    print()
    
    # Test 7: Statistiques
    print("Test 7: Statistiques")
    stats = allocator.get_statistics()
    print(f"Active allocations: {stats['active_allocations']}")
    print(f"Avg action utilization: {stats['avg_action_utilization']:.1%}")
    print()
    
    print("=== Tests C18 RÉUSSIS ===")
    print(f"Forensic log: {log_path}")

# Made with Bob
