"""
CORRECTION C18: EXPLORATION BUDGET MANAGER
==========================================

PROBLÈME IDENTIFIÉ (Session 64):
- Exploration uniforme sans allocation intelligente
- Pas de limite dynamique basée sur réputation
- Gaspillage budget sur actions low-value

SOLUTION C18:
- Budget proportionnel à réputation action
- Early stopping si budget épuisé
- Allocation adaptative par contexte

FORMULE BUDGET:
budget_action = base_budget * (reputation / max_reputation) * context_multiplier

INTÉGRATION:
- Appelé AVANT chaque exploration action
- Retourne (should_explore: bool, remaining_budget: int)
- Mis à jour APRÈS chaque tentative

AUTEUR: Session 64 - Analyse Forensique Comparative V26/V27/V28
DATE: 2026-06-13
"""

from typing import Dict, Tuple, Optional
from dataclasses import dataclass
import math


@dataclass
class BudgetAllocation:
    """Allocation budget pour une action"""
    action_name: str
    base_budget: int
    reputation_multiplier: float
    context_multiplier: float
    allocated_budget: int
    consumed_budget: int
    
    @property
    def remaining_budget(self) -> int:
        """Budget restant"""
        return max(0, self.allocated_budget - self.consumed_budget)
    
    @property
    def is_exhausted(self) -> bool:
        """Budget épuisé?"""
        return self.remaining_budget == 0


class ExplorationBudgetManager:
    """
    Gestionnaire budget exploration avec allocation intelligente.
    
    PRINCIPE:
    - Actions haute réputation → plus de budget
    - Actions basse réputation → budget minimal
    - Contexte puzzle influence allocation
    
    MÉTRIQUES:
    - Total budget disponible par puzzle
    - Budget alloué par action
    - Budget consommé vs alloué
    - Taux utilisation efficace
    """
    
    def __init__(
        self,
        base_budget_per_puzzle: int = 100,
        min_budget_per_action: int = 5,
        max_budget_per_action: int = 50,
        reputation_weight: float = 0.7,
        context_weight: float = 0.3,
        verbose: bool = False
    ):
        """
        Args:
            base_budget_per_puzzle: Budget total par puzzle
            min_budget_per_action: Budget minimum par action
            max_budget_per_action: Budget maximum par action
            reputation_weight: Poids réputation (0-1)
            context_weight: Poids contexte (0-1)
            verbose: Logs détaillés
        """
        self.base_budget_per_puzzle = base_budget_per_puzzle
        self.min_budget_per_action = min_budget_per_action
        self.max_budget_per_action = max_budget_per_action
        self.reputation_weight = reputation_weight
        self.context_weight = context_weight
        self.verbose = verbose
        
        # État actuel
        self.current_puzzle_budget = base_budget_per_puzzle
        self.allocations: Dict[str, BudgetAllocation] = {}
        self.total_consumed = 0
        
        # Statistiques
        self.puzzles_processed = 0
        self.total_budget_used = 0
        self.total_budget_wasted = 0  # Budget alloué non utilisé
        
    def reset_puzzle(self) -> None:
        """Réinitialiser pour nouveau puzzle"""
        # Calculer gaspillage
        for alloc in self.allocations.values():
            self.total_budget_wasted += alloc.remaining_budget
        
        # Reset
        self.current_puzzle_budget = self.base_budget_per_puzzle
        self.allocations.clear()
        self.total_consumed = 0
        self.puzzles_processed += 1
        
        if self.verbose:
            print(f"\n[C18 BUDGET] Nouveau puzzle #{self.puzzles_processed}")
            print(f"  Budget disponible: {self.current_puzzle_budget}")
    
    def allocate_budget(
        self,
        action_name: str,
        reputation: float,
        grid_size: int,
        color_count: int,
        max_reputation: float = 1.0
    ) -> BudgetAllocation:
        """
        Allouer budget pour une action.
        
        Args:
            action_name: Nom action
            reputation: Réputation action [0-1]
            grid_size: Taille grille (influence complexité)
            color_count: Nombre couleurs (influence complexité)
            max_reputation: Réputation maximale pour normalisation
        
        Returns:
            BudgetAllocation avec budget alloué
        """
        # Vérifier si déjà alloué
        if action_name in self.allocations:
            return self.allocations[action_name]
        
        # Calculer multiplicateur réputation
        rep_multiplier = (reputation / max_reputation) if max_reputation > 0 else 0.5
        rep_multiplier = max(0.1, min(1.0, rep_multiplier))  # Clamp [0.1, 1.0]
        
        # Calculer multiplicateur contexte
        # Grilles grandes + couleurs nombreuses = plus complexe = plus budget
        complexity = math.sqrt(grid_size) * math.log2(color_count + 1)
        context_multiplier = 1.0 + (complexity / 100.0)  # Normalisation empirique
        context_multiplier = max(0.5, min(2.0, context_multiplier))  # Clamp [0.5, 2.0]
        
        # Calculer budget alloué
        base = self.base_budget_per_puzzle / 10  # ~10 actions moyennes par puzzle
        weighted_multiplier = (
            self.reputation_weight * rep_multiplier +
            self.context_weight * context_multiplier
        )
        allocated = int(base * weighted_multiplier)
        allocated = max(self.min_budget_per_action, min(self.max_budget_per_action, allocated))
        
        # Créer allocation
        allocation = BudgetAllocation(
            action_name=action_name,
            base_budget=int(base),
            reputation_multiplier=rep_multiplier,
            context_multiplier=context_multiplier,
            allocated_budget=allocated,
            consumed_budget=0
        )
        
        self.allocations[action_name] = allocation
        
        if self.verbose:
            print(f"[C18 BUDGET] Allocation {action_name}:")
            print(f"  Réputation: {reputation:.3f} → multiplier={rep_multiplier:.3f}")
            print(f"  Contexte: grid={grid_size}, colors={color_count} → multiplier={context_multiplier:.3f}")
            print(f"  Budget alloué: {allocated} (min={self.min_budget_per_action}, max={self.max_budget_per_action})")
        
        return allocation
    
    def should_explore(
        self,
        action_name: str,
        reputation: float,
        grid_size: int,
        color_count: int,
        max_reputation: float = 1.0
    ) -> Tuple[bool, int]:
        """
        Décider si exploration autorisée.
        
        Args:
            action_name: Nom action
            reputation: Réputation action
            grid_size: Taille grille
            color_count: Nombre couleurs
            max_reputation: Réputation max
        
        Returns:
            (should_explore, remaining_budget)
        """
        # Vérifier budget global puzzle
        if self.current_puzzle_budget <= 0:
            if self.verbose:
                print(f"[C18 BUDGET] STOP - Budget puzzle épuisé")
            return False, 0
        
        # Obtenir/créer allocation
        allocation = self.allocations.get(action_name)
        if allocation is None:
            allocation = self.allocate_budget(
                action_name, reputation, grid_size, color_count, max_reputation
            )
        
        # Vérifier budget action
        if allocation.is_exhausted:
            if self.verbose:
                print(f"[C18 BUDGET] SKIP {action_name} - Budget action épuisé")
            return False, 0
        
        return True, allocation.remaining_budget
    
    def consume_budget(self, action_name: str, amount: int = 1) -> None:
        """
        Consommer budget après tentative.
        
        Args:
            action_name: Nom action
            amount: Quantité consommée (défaut: 1)
        """
        allocation = self.allocations.get(action_name)
        if allocation is None:
            if self.verbose:
                print(f"[C18 BUDGET] WARNING - Consommation sans allocation: {action_name}")
            return
        
        # Consommer
        allocation.consumed_budget += amount
        self.current_puzzle_budget -= amount
        self.total_consumed += amount
        self.total_budget_used += amount
        
        if self.verbose:
            print(f"[C18 BUDGET] Consommé {amount} pour {action_name}")
            print(f"  Action: {allocation.consumed_budget}/{allocation.allocated_budget}")
            print(f"  Puzzle: {self.current_puzzle_budget}/{self.base_budget_per_puzzle}")
    
    def get_statistics(self) -> Dict[str, float]:
        """
        Statistiques utilisation budget.
        
        Returns:
            Dict avec métriques clés
        """
        if self.puzzles_processed == 0:
            return {
                "puzzles_processed": 0,
                "avg_budget_used": 0.0,
                "avg_budget_wasted": 0.0,
                "efficiency_rate": 0.0
            }
        
        total_allocated = self.puzzles_processed * self.base_budget_per_puzzle
        
        return {
            "puzzles_processed": self.puzzles_processed,
            "avg_budget_used": self.total_budget_used / self.puzzles_processed,
            "avg_budget_wasted": self.total_budget_wasted / self.puzzles_processed,
            "efficiency_rate": self.total_budget_used / total_allocated if total_allocated > 0 else 0.0,
            "total_allocated": total_allocated,
            "total_used": self.total_budget_used,
            "total_wasted": self.total_budget_wasted
        }
    
    def print_statistics(self) -> None:
        """Afficher statistiques"""
        stats = self.get_statistics()
        
        print("\n" + "="*60)
        print("C18 EXPLORATION BUDGET MANAGER - STATISTIQUES")
        print("="*60)
        print(f"Puzzles traités: {stats['puzzles_processed']}")
        print(f"Budget moyen utilisé: {stats['avg_budget_used']:.1f}/{self.base_budget_per_puzzle}")
        print(f"Budget moyen gaspillé: {stats['avg_budget_wasted']:.1f}")
        print(f"Taux efficacité: {stats['efficiency_rate']*100:.1f}%")
        print(f"Total alloué: {stats['total_allocated']}")
        print(f"Total utilisé: {stats['total_used']}")
        print(f"Total gaspillé: {stats['total_wasted']}")
        print("="*60 + "\n")


# Test unitaire
if __name__ == "__main__":
    print("TEST C18 - EXPLORATION BUDGET MANAGER")
    print("="*60)
    
    manager = ExplorationBudgetManager(
        base_budget_per_puzzle=100,
        min_budget_per_action=5,
        max_budget_per_action=30,
        verbose=True
    )
    
    # Simuler puzzle 1
    print("\n### PUZZLE 1 ###")
    manager.reset_puzzle()
    
    # Action haute réputation
    should, remaining = manager.should_explore("flip_horizontal", 0.8, 10, 5)
    print(f"Exploration autorisée: {should}, budget restant: {remaining}")
    if should:
        manager.consume_budget("flip_horizontal", 1)
    
    # Action basse réputation
    should, remaining = manager.should_explore("identity", 0.1, 10, 5)
    print(f"Exploration autorisée: {should}, budget restant: {remaining}")
    if should:
        manager.consume_budget("identity", 1)
    
    # Simuler puzzle 2
    print("\n### PUZZLE 2 ###")
    manager.reset_puzzle()
    
    # Grille complexe
    should, remaining = manager.should_explore("rotate_90", 0.6, 30, 10)
    print(f"Exploration autorisée: {should}, budget restant: {remaining}")
    
    # Statistiques
    manager.print_statistics()
    
    print("\n✅ Test C18 terminé")

# Made with Bob
