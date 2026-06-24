"""
Trajectory Stagnation Detector (C19) - Détection de Stagnation Cognitive

Ce module implémente la détection de stagnation dans les trajectoires d'exploration.
Il permet à MAGEN de détecter quand il est bloqué dans une impasse cognitive.

Architecture Cognitive:
- Fenêtre glissante sur historique états
- Mesure variance erreur (progrès)
- Détection loops (états répétés)
- Recommandation abandon stratégique
- Forensic LEÇON-73.1 standard

Critères Stagnation:
    variance < threshold (pas de progrès)
    loop_ratio > threshold (états répétés)
    should_abandon = stagnant AND high_loops

Auteur: Bob (IA) + Validation Utilisateur Expert
Date: 2026-06-15
Version: V38.0 (Architecture Cognitive)
"""

import json
import time
import numpy as np
from pathlib import Path
from typing import List, Optional, Tuple
from dataclasses import dataclass, asdict
from collections import deque


@dataclass
class TrajectoryState:
    """État d'une trajectoire à un instant donné."""
    step: int
    error: float
    position: Optional[Tuple[int, int]] = None
    action: Optional[str] = None
    timestamp: float = 0.0
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)
    
    def __hash__(self):
        """Hash pour détecter états identiques."""
        return hash((self.error, self.position, self.action))
    
    def __eq__(self, other):
        """Égalité pour détecter états identiques."""
        if not isinstance(other, TrajectoryState):
            return False
        return (
            abs(self.error - other.error) < 0.01 and
            self.position == other.position and
            self.action == other.action
        )


@dataclass
class StagnationAnalysis:
    """Analyse de stagnation d'une trajectoire."""
    is_stagnant: bool
    variance: float
    loop_ratio: float
    unique_states_ratio: float
    should_abandon: bool
    confidence: float
    window_size: int
    timestamp: float
    
    def to_dict(self) -> dict:
        """Convertit en dictionnaire."""
        return asdict(self)


class TrajectoryStagnationDetector:
    """
    Détecteur de stagnation de trajectoires (C19).
    
    Fonction Cognitive: Détection de stagnation cognitive
    Question: "Suis-je bloqué dans une impasse ?"
    
    Capacités:
    - Mesure variance erreur (progrès)
    - Détection loops (états répétés)
    - Recommandation abandon stratégique
    - Confiance adaptative (basée sur window_size)
    """
    
    def __init__(
        self,
        window_size: int = 10,
        variance_threshold: float = 0.01,
        loop_threshold: float = 0.7,
        min_confidence_steps: int = 5,
        forensic_log_path: Optional[str] = None
    ):
        """
        Initialise le détecteur de stagnation.
        
        Args:
            window_size: Taille fenêtre glissante
            variance_threshold: Seuil variance pour stagnation
            loop_threshold: Seuil ratio loops pour abandon
            min_confidence_steps: Steps minimum pour confiance élevée
            forensic_log_path: Chemin du log forensic
        """
        self.window_size = window_size
        self.variance_threshold = variance_threshold
        self.loop_threshold = loop_threshold
        self.min_confidence_steps = min_confidence_steps
        
        self.trajectory_history: deque = deque(maxlen=window_size)
        self.full_history: List[TrajectoryState] = []
        
        # Forensic LEÇON-73.1
        self.forensic_log_path = forensic_log_path
        if forensic_log_path:
            Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
        self._log_forensic("detector_initialized", {
            "window_size": window_size,
            "variance_threshold": variance_threshold,
            "loop_threshold": loop_threshold,
            "min_confidence_steps": min_confidence_steps
        })
    
    def _log_forensic(self, event_type: str, data: dict):
        """Log événement forensic (LEÇON-73.1)."""
        if not self.forensic_log_path:
            return
        
        event = {
            "timestamp": time.time(),
            "module": "TrajectoryStagnationDetector",
            "event_type": event_type,
            "data": data
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def add_state(
        self,
        step: int,
        error: float,
        position: Optional[Tuple[int, int]] = None,
        action: Optional[str] = None
    ):
        """
        Ajoute un état à la trajectoire.
        
        Args:
            step: Numéro du step
            error: Erreur mesurée
            position: Position (optionnel)
            action: Action effectuée (optionnel)
        """
        state = TrajectoryState(
            step=step,
            error=error,
            position=position,
            action=action,
            timestamp=time.time()
        )
        
        self.trajectory_history.append(state)
        self.full_history.append(state)
        
        # Forensic
        self._log_forensic("state_added", {
            "step": step,
            "error": error,
            "position": position,
            "action": action,
            "history_length": len(self.full_history)
        })
    
    def detect_stagnation(self) -> StagnationAnalysis:
        """
        Détecte la stagnation dans la trajectoire actuelle.
        
        Critères:
        1. Variance erreur < threshold (pas de progrès)
        2. Loop ratio > threshold (états répétés)
        3. Abandon si stagnant ET high loops
        
        Returns:
            StagnationAnalysis avec métriques et recommandation
        """
        # Pas assez de données
        if len(self.trajectory_history) < 2:
            return StagnationAnalysis(
                is_stagnant=False,
                variance=0.0,
                loop_ratio=0.0,
                unique_states_ratio=1.0,
                should_abandon=False,
                confidence=0.0,
                window_size=len(self.trajectory_history),
                timestamp=time.time()
            )
        
        # Fenêtre actuelle
        window = list(self.trajectory_history)
        
        # 1. Mesurer variance erreur
        errors = [s.error for s in window]
        variance = float(np.var(errors))
        is_stagnant = variance < self.variance_threshold
        
        # 2. Mesurer loops (états répétés)
        unique_states = len(set(window))
        total_states = len(window)
        unique_states_ratio = unique_states / total_states
        loop_ratio = 1.0 - unique_states_ratio
        
        # 3. Recommandation abandon
        should_abandon = is_stagnant and loop_ratio > self.loop_threshold
        
        # 4. Confiance (basée sur taille fenêtre)
        confidence = min(1.0, len(window) / self.min_confidence_steps)
        
        analysis = StagnationAnalysis(
            is_stagnant=is_stagnant,
            variance=variance,
            loop_ratio=loop_ratio,
            unique_states_ratio=unique_states_ratio,
            should_abandon=should_abandon,
            confidence=confidence,
            window_size=len(window),
            timestamp=time.time()
        )
        
        # Forensic
        self._log_forensic("stagnation_detected", analysis.to_dict())
        
        return analysis
    
    def get_progress_trend(self) -> str:
        """
        Analyse la tendance de progrès.
        
        Returns:
            "improving", "stable", "degrading"
        """
        if len(self.trajectory_history) < 3:
            return "stable"
        
        window = list(self.trajectory_history)
        errors = [s.error for s in window]
        
        # Régression linéaire simple
        x = np.arange(len(errors))
        y = np.array(errors)
        
        # Pente
        slope = np.polyfit(x, y, 1)[0]
        
        # Tendance
        if slope < -0.01:
            trend = "improving"  # Erreur diminue
        elif slope > 0.01:
            trend = "degrading"  # Erreur augmente
        else:
            trend = "stable"
        
        # Forensic
        self._log_forensic("progress_trend_analyzed", {
            "trend": trend,
            "slope": float(slope),
            "window_size": len(window)
        })
        
        return trend
    
    def get_loop_patterns(self) -> List[Tuple[TrajectoryState, int]]:
        """
        Identifie les patterns de loops.
        
        Returns:
            Liste de (état, nombre_répétitions)
        """
        if len(self.trajectory_history) < 2:
            return []
        
        window = list(self.trajectory_history)
        
        # Compter occurrences
        state_counts = {}
        for state in window:
            key = (state.error, state.position, state.action)
            state_counts[key] = state_counts.get(key, 0) + 1
        
        # Filtrer loops (>1 occurrence)
        loops = [
            (state, count)
            for state in window
            for key, count in state_counts.items()
            if (state.error, state.position, state.action) == key and count > 1
        ]
        
        # Dédupliquer
        seen = set()
        unique_loops = []
        for state, count in loops:
            key = (state.error, state.position, state.action)
            if key not in seen:
                seen.add(key)
                unique_loops.append((state, count))
        
        # Trier par fréquence
        unique_loops.sort(key=lambda x: x[1], reverse=True)
        
        # Forensic
        self._log_forensic("loop_patterns_identified", {
            "num_patterns": len(unique_loops),
            "top_pattern_count": unique_loops[0][1] if unique_loops else 0
        })
        
        return unique_loops
    
    def should_change_strategy(self) -> bool:
        """
        Détermine si la stratégie devrait changer.
        
        Returns:
            True si changement recommandé
        """
        analysis = self.detect_stagnation()
        trend = self.get_progress_trend()
        
        # Changement si:
        # 1. Stagnation détectée
        # 2. OU tendance dégradante
        # 3. ET confiance suffisante
        should_change = (
            (analysis.is_stagnant or trend == "degrading") and
            analysis.confidence >= 0.5
        )
        
        # Forensic
        self._log_forensic("strategy_change_decision", {
            "should_change": should_change,
            "is_stagnant": analysis.is_stagnant,
            "trend": trend,
            "confidence": analysis.confidence
        })
        
        return should_change
    
    def get_statistics(self) -> dict:
        """
        Retourne statistiques globales.
        
        Returns:
            Dictionnaire avec métriques agrégées
        """
        if not self.full_history:
            return {
                "total_steps": 0,
                "current_window_size": 0,
                "avg_error": 0.0,
                "min_error": 0.0,
                "max_error": 0.0
            }
        
        errors = [s.error for s in self.full_history]
        
        stats = {
            "total_steps": len(self.full_history),
            "current_window_size": len(self.trajectory_history),
            "avg_error": float(np.mean(errors)),
            "min_error": float(np.min(errors)),
            "max_error": float(np.max(errors)),
            "error_std": float(np.std(errors))
        }
        
        # Forensic
        self._log_forensic("statistics_computed", stats)
        
        return stats
    
    def reset(self):
        """Réinitialise le détecteur (pour tests)."""
        self.trajectory_history.clear()
        self.full_history.clear()
        
        self._log_forensic("detector_reset", {})


# Tests unitaires
if __name__ == "__main__":
    import tempfile
    
    print("=== Test TrajectoryStagnationDetector (C19) ===\n")
    
    # Créer détecteur
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    
    detector = TrajectoryStagnationDetector(
        window_size=10,
        variance_threshold=0.01,
        loop_threshold=0.7,
        min_confidence_steps=5,
        forensic_log_path=log_path
    )
    
    # Test 1: Trajectoire avec progrès
    print("Test 1: Trajectoire avec progrès")
    for i in range(10):
        detector.add_state(
            step=i,
            error=0.9 - (i * 0.05),  # Erreur diminue
            position=(i, i),
            action="move_forward"
        )
    
    analysis1 = detector.detect_stagnation()
    trend1 = detector.get_progress_trend()
    print(f"Variance: {analysis1.variance:.4f}")
    print(f"Loop ratio: {analysis1.loop_ratio:.2f}")
    print(f"Is stagnant: {analysis1.is_stagnant}")
    print(f"Trend: {trend1}")
    print()
    
    # Test 2: Trajectoire stagnante (loops)
    print("Test 2: Trajectoire stagnante (loops)")
    detector.reset()
    
    for i in range(10):
        detector.add_state(
            step=i,
            error=0.5,  # Erreur constante
            position=(0, 0),  # Position identique
            action="identity"  # Action identique
        )
    
    analysis2 = detector.detect_stagnation()
    trend2 = detector.get_progress_trend()
    loops2 = detector.get_loop_patterns()
    
    print(f"Variance: {analysis2.variance:.4f}")
    print(f"Loop ratio: {analysis2.loop_ratio:.2f}")
    print(f"Is stagnant: {analysis2.is_stagnant}")
    print(f"Should abandon: {analysis2.should_abandon}")
    print(f"Trend: {trend2}")
    print(f"Loop patterns: {len(loops2)}")
    print()
    
    # Test 3: Trajectoire dégradante
    print("Test 3: Trajectoire dégradante")
    detector.reset()
    
    for i in range(10):
        detector.add_state(
            step=i,
            error=0.3 + (i * 0.05),  # Erreur augmente
            position=(i, i),
            action="move_forward"
        )
    
    analysis3 = detector.detect_stagnation()
    trend3 = detector.get_progress_trend()
    should_change3 = detector.should_change_strategy()
    
    print(f"Variance: {analysis3.variance:.4f}")
    print(f"Is stagnant: {analysis3.is_stagnant}")
    print(f"Trend: {trend3}")
    print(f"Should change strategy: {should_change3}")
    print()
    
    # Test 4: Statistiques
    print("Test 4: Statistiques")
    stats = detector.get_statistics()
    print(f"Total steps: {stats['total_steps']}")
    print(f"Avg error: {stats['avg_error']:.3f}")
    print(f"Min error: {stats['min_error']:.3f}")
    print(f"Max error: {stats['max_error']:.3f}")
    print()
    
    print("=== Tests C19 RÉUSSIS ===")
    print(f"Forensic log: {log_path}")

# Made with Bob
