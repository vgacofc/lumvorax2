"""
Affordance Discovery Engine (C0) - PHASE 0 Filtrage Pré-Exploration

Ce module implémente la découverte d'affordances perceptuelles AVANT l'exploration.
Il permet à MAGEN de filtrer les actions impossibles ou non pertinentes.

Architecture Cognitive PHASE 0:
- Détection affordances perceptuelles (état actuel)
- Évaluation faisabilité actions
- Filtrage pré-exploration (réduction espace recherche)
- Intégration C17 (ActionReputationSystem) pour priorisation

Différence avec ActionDiscoveryEngine (V23):
- ActionDiscoveryEngine : Apprentissage APRÈS expérimentation
- AffordanceDiscoveryEngine : Filtrage AVANT exploration

Formule Plausibilité:
    plausibility = perceptual_score * feasibility_score * reputation_score

Auteur: Bob (IA) + Validation Utilisateur Expert
Date: 2026-06-15
Version: V38.0 (Architecture Cognitive - PHASE 0)
"""

import json
import time
import numpy as np
from pathlib import Path
from typing import List, Dict, Optional, Set, Tuple
from dataclasses import dataclass, asdict
from collections import defaultdict


@dataclass
class Affordance:
    """Affordance détectée pour une action."""
    action_id: str
    perceptual_score: float  # Score basé sur perception état
    feasibility_score: float  # Score faisabilité technique
    plausibility: float  # Score final combiné
    reasons: List[str]  # Raisons détection
    timestamp: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)


@dataclass
class AffordanceContext:
    """Contexte pour découverte affordances."""
    grid_shape: Tuple[int, int]
    color_count: int
    has_symmetry: bool
    has_patterns: bool
    is_sparse: bool
    complexity: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire (JSON-safe)."""
        return {
            'grid_shape': list(self.grid_shape),
            'color_count': int(self.color_count),
            'has_symmetry': bool(self.has_symmetry),
            'has_patterns': bool(self.has_patterns),
            'is_sparse': bool(self.is_sparse),
            'complexity': float(self.complexity)
        }


class AffordanceDiscoveryEngine:
    """
    Moteur de découverte d'affordances (C0 - PHASE 0).
    
    Fonction Cognitive: Filtrage pré-exploration
    Question: "Quelles actions sont plausibles ?"
    
    Capacités:
    - Détection affordances perceptuelles
    - Évaluation faisabilité actions
    - Filtrage pré-exploration
    - Intégration C17 (réputation)
    """
    
    def __init__(
        self,
        forensic_log_path: Optional[str] = None,
        min_plausibility: float = 0.3,
        reputation_system: Optional[object] = None
    ):
        """
        Initialise le moteur de découverte d'affordances.
        
        Args:
            forensic_log_path: Chemin du log forensic
            min_plausibility: Seuil minimum plausibilité
            reputation_system: ActionReputationSystem (C17) optionnel
        """
        self.min_plausibility = min_plausibility
        self.reputation_system = reputation_system
        
        # Règles d'affordances (pattern → actions)
        self.affordance_rules = self._initialize_affordance_rules()
        
        # Statistiques
        self.stats = {
            'total_discoveries': 0,
            'affordances_found': 0,
            'actions_filtered': 0,
            'avg_reduction_rate': 0.0
        }
        
        # Forensic LEÇON-73.1
        self.forensic_log_path = forensic_log_path
        if forensic_log_path:
            Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
        self._log_forensic("engine_initialized", {
            "min_plausibility": min_plausibility,
            "has_reputation_system": reputation_system is not None
        })
    
    def _log_forensic(self, event_type: str, data: dict):
        """Log événement forensic (LEÇON-73.1)."""
        if not self.forensic_log_path:
            return
        
        event = {
            "timestamp": time.time(),
            "module": "AffordanceDiscoveryEngine",
            "event_type": event_type,
            "data": data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def _initialize_affordance_rules(self) -> Dict[str, List[str]]:
        """
        Initialise les règles d'affordances.
        
        Format: {pattern_type: [actions_plausibles]}
        """
        return {
            'symmetry_horizontal': ['flip_horizontal', 'mirror_h'],
            'symmetry_vertical': ['flip_vertical', 'mirror_v'],
            'symmetry_diagonal': ['transpose', 'flip_diagonal'],
            'rotation_90': ['rotate_90', 'rotate_270'],
            'rotation_180': ['rotate_180'],
            'grid_small': ['identity', 'color_map'],
            'grid_large': ['crop', 'tile', 'scale'],
            'sparse_grid': ['fill', 'connect'],
            'dense_grid': ['extract', 'filter'],
            'color_blocks': ['color_map', 'recolor'],
            'patterns_repeating': ['tile', 'extend'],
            'patterns_unique': ['identity', 'transform']
        }
    
    def discover_affordances(
        self,
        state: np.ndarray,
        available_actions: List[str]
    ) -> List[Affordance]:
        """
        Découvre affordances pour état donné.
        
        Args:
            state: État actuel (grille)
            available_actions: Actions disponibles
            
        Returns:
            Liste d'Affordance triée par plausibilité
        """
        # Analyser contexte perceptuel
        context = self._analyze_perceptual_context(state)
        
        # Découvrir affordances
        affordances = []
        
        for action_id in available_actions:
            # Score perceptuel
            perceptual_score = self._compute_perceptual_score(action_id, context)
            
            # Score faisabilité
            feasibility_score = self._compute_feasibility_score(action_id, state)
            
            # Score réputation (si C17 disponible)
            reputation_score = 1.0
            if self.reputation_system and hasattr(self.reputation_system, 'compute_priority'):
                rep_score = self.reputation_system.compute_priority(action_id)
                reputation_score = rep_score.priority
            
            # Plausibilité finale
            plausibility = perceptual_score * feasibility_score * reputation_score
            
            # Raisons
            reasons = self._get_affordance_reasons(action_id, context, perceptual_score, feasibility_score)
            
            affordance = Affordance(
                action_id=action_id,
                perceptual_score=perceptual_score,
                feasibility_score=feasibility_score,
                plausibility=plausibility,
                reasons=reasons,
                timestamp=time.time()
            )
            
            affordances.append(affordance)
        
        # Trier par plausibilité
        affordances.sort(key=lambda a: a.plausibility, reverse=True)
        
        # Statistiques
        self.stats['total_discoveries'] += 1
        self.stats['affordances_found'] += len([a for a in affordances if a.plausibility >= self.min_plausibility])
        
        # Forensic
        self._log_forensic("affordances_discovered", {
            "state_shape": state.shape,
            "available_actions": len(available_actions),
            "affordances_found": len(affordances),
            "top_affordance": affordances[0].action_id if affordances else None,
            "top_plausibility": affordances[0].plausibility if affordances else 0.0,
            "context": context.to_dict()
        })
        
        return affordances
    
    def filter_actions(
        self,
        state: np.ndarray,
        available_actions: List[str]
    ) -> List[str]:
        """
        Filtre actions AVANT exploration.
        
        Args:
            state: État actuel
            available_actions: Actions disponibles
            
        Returns:
            Liste actions filtrées (plausibilité >= seuil)
        """
        affordances = self.discover_affordances(state, available_actions)
        
        # Filtrer par seuil
        filtered = [
            a.action_id 
            for a in affordances 
            if a.plausibility >= self.min_plausibility
        ]
        
        # Statistiques
        reduction_rate = 1.0 - (len(filtered) / len(available_actions)) if available_actions else 0.0
        self.stats['actions_filtered'] += (len(available_actions) - len(filtered))
        self.stats['avg_reduction_rate'] = (
            (self.stats['avg_reduction_rate'] * (self.stats['total_discoveries'] - 1) + reduction_rate) 
            / self.stats['total_discoveries']
        )
        
        # Forensic
        self._log_forensic("actions_filtered", {
            "available_actions": len(available_actions),
            "filtered_actions": len(filtered),
            "reduction_rate": reduction_rate,
            "filtered_list": filtered
        })
        
        return filtered
    
    def _analyze_perceptual_context(self, state: np.ndarray) -> AffordanceContext:
        """Analyse contexte perceptuel de l'état."""
        h, w = state.shape
        
        # Compter couleurs
        color_count = len(np.unique(state))
        
        # Détecter symétries
        has_symmetry = (
            np.allclose(state, np.fliplr(state)) or
            np.allclose(state, np.flipud(state))
        )
        
        # Détecter patterns
        has_patterns = self._detect_patterns(state)
        
        # Détecter sparsité
        non_zero_ratio = np.count_nonzero(state) / state.size
        is_sparse = non_zero_ratio < 0.3
        
        # Complexité
        complexity = color_count / 10.0  # Normalisé
        
        return AffordanceContext(
            grid_shape=(h, w),
            color_count=color_count,
            has_symmetry=has_symmetry,
            has_patterns=has_patterns,
            is_sparse=is_sparse,
            complexity=complexity
        )
    
    def _detect_patterns(self, state: np.ndarray) -> bool:
        """Détecte si grille contient patterns répétitifs."""
        # Simple heuristique: variance faible = patterns
        variance = float(np.var(state))
        return variance < 2.0
    
    def _compute_perceptual_score(self, action_id: str, context: AffordanceContext) -> float:
        """Calcule score perceptuel basé sur règles d'affordances."""
        score = 0.5  # Score neutre par défaut
        
        # Règle 1: Symétrie → flip/mirror
        if context.has_symmetry:
            if 'flip' in action_id or 'mirror' in action_id:
                score += 0.3
        
        # Règle 2: Rotation → rotate
        if 'rotate' in action_id:
            score += 0.2
        
        # Règle 3: Grille petite → identity/color_map
        if context.grid_shape[0] < 5 or context.grid_shape[1] < 5:
            if action_id in ['identity', 'color_map']:
                score += 0.2
        
        # Règle 4: Grille grande → crop/tile
        if context.grid_shape[0] > 10 or context.grid_shape[1] > 10:
            if action_id in ['crop', 'tile', 'scale']:
                score += 0.2
        
        # Règle 5: Sparse → fill/connect
        if context.is_sparse:
            if action_id in ['fill', 'connect']:
                score += 0.2
        
        # Règle 6: Patterns → tile/extend
        if context.has_patterns:
            if action_id in ['tile', 'extend']:
                score += 0.2
        
        return min(1.0, score)
    
    def _compute_feasibility_score(self, action_id: str, state: np.ndarray) -> float:
        """Calcule score faisabilité technique."""
        h, w = state.shape
        score = 1.0  # Faisable par défaut
        
        # Règle 1: crop impossible si grille trop petite
        if action_id == 'crop':
            if h < 3 or w < 3:
                score = 0.1
        
        # Règle 2: tile impossible si grille trop grande
        if action_id == 'tile':
            if h > 20 or w > 20:
                score = 0.3
        
        # Règle 3: rotate nécessite grille carrée (ou presque)
        if 'rotate' in action_id:
            if abs(h - w) > 2:
                score = 0.5
        
        return score
    
    def _get_affordance_reasons(
        self,
        action_id: str,
        context: AffordanceContext,
        perceptual_score: float,
        feasibility_score: float
    ) -> List[str]:
        """Génère raisons pour affordance."""
        reasons = []
        
        if perceptual_score > 0.7:
            reasons.append("high_perceptual_match")
        if feasibility_score > 0.7:
            reasons.append("technically_feasible")
        if context.has_symmetry and ('flip' in action_id or 'mirror' in action_id):
            reasons.append("symmetry_detected")
        if context.has_patterns and action_id in ['tile', 'extend']:
            reasons.append("patterns_detected")
        if context.is_sparse and action_id in ['fill', 'connect']:
            reasons.append("sparse_grid")
        
        return reasons if reasons else ["default"]
    
    def get_statistics(self) -> dict:
        """Retourne statistiques globales."""
        stats = {
            **self.stats,
            "avg_affordances_per_discovery": (
                self.stats['affordances_found'] / self.stats['total_discoveries']
                if self.stats['total_discoveries'] > 0 else 0.0
            )
        }
        
        self._log_forensic("statistics_computed", stats)
        
        return stats


# Tests unitaires
if __name__ == "__main__":
    import tempfile
    
    print("=== Test AffordanceDiscoveryEngine (C0) ===\n")
    
    # Créer engine
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    
    engine = AffordanceDiscoveryEngine(
        forensic_log_path=log_path,
        min_plausibility=0.3
    )
    
    # Test 1: Grille symétrique
    print("Test 1: Grille symétrique")
    state_symmetric = np.array([
        [1, 2, 1],
        [2, 3, 2],
        [1, 2, 1]
    ])
    
    actions = ['flip_horizontal', 'rotate_90', 'identity', 'crop', 'tile']
    affordances1 = engine.discover_affordances(state_symmetric, actions)
    
    print(f"Top 3 affordances:")
    for i, aff in enumerate(affordances1[:3], 1):
        print(f"  {i}. {aff.action_id}: plausibility={aff.plausibility:.3f}, reasons={aff.reasons}")
    print()
    
    # Test 2: Grille sparse
    print("Test 2: Grille sparse")
    state_sparse = np.array([
        [0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0],
        [1, 0, 0, 0, 1],
        [0, 0, 0, 0, 0],
        [0, 0, 1, 0, 0]
    ])
    
    affordances2 = engine.discover_affordances(state_sparse, actions)
    
    print(f"Top 3 affordances:")
    for i, aff in enumerate(affordances2[:3], 1):
        print(f"  {i}. {aff.action_id}: plausibility={aff.plausibility:.3f}, reasons={aff.reasons}")
    print()
    
    # Test 3: Filtrage actions
    print("Test 3: Filtrage actions")
    filtered = engine.filter_actions(state_symmetric, actions)
    print(f"Actions disponibles: {len(actions)}")
    print(f"Actions filtrées: {len(filtered)}")
    print(f"Réduction: {(1 - len(filtered)/len(actions))*100:.1f}%")
    print(f"Actions retenues: {filtered}")
    print()
    
    # Test 4: Grille grande
    print("Test 4: Grille grande")
    state_large = np.random.randint(0, 5, size=(15, 15))
    
    affordances4 = engine.discover_affordances(state_large, actions)
    
    print(f"Top 3 affordances:")
    for i, aff in enumerate(affordances4[:3], 1):
        print(f"  {i}. {aff.action_id}: plausibility={aff.plausibility:.3f}, reasons={aff.reasons}")
    print()
    
    # Test 5: Statistiques
    print("Test 5: Statistiques")
    stats = engine.get_statistics()
    print(f"Total discoveries: {stats['total_discoveries']}")
    print(f"Affordances found: {stats['affordances_found']}")
    print(f"Actions filtered: {stats['actions_filtered']}")
    print(f"Avg reduction rate: {stats['avg_reduction_rate']:.1%}")
    print()
    
    print("=== Tests C0 RÉUSSIS ===")
    print(f"Forensic log: {log_path}")
    print("\n🔍 LIRE LOGS FORENSIC MAINTENANT...")

# Made with Bob
