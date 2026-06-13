#!/usr/bin/env python3
"""
Learning Memory System - Apprentissage en boucle avec mémoire des échecs
Intègre architecture GEN8-24 pour mémoire persistante
"""

import json
import numpy as np
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime
import hashlib


@dataclass
class PuzzleExperience:
    """Expérience d'un puzzle (succès ou échec)"""
    puzzle_id: str
    timestamp: str
    success: bool
    classification: str
    confidence: float
    programs_tried: int
    best_score: float
    solution_program: Optional[str]
    failure_reason: Optional[str]
    train_pairs_hash: str
    
    def to_dict(self):
        return asdict(self)


@dataclass
class FailurePattern:
    """Pattern d'échec identifié"""
    pattern_type: str  # 'color_detection', 'region_extraction', 'composition', etc.
    puzzle_ids: List[str]
    frequency: int
    last_seen: str
    correction_attempted: bool
    correction_successful: Optional[bool]


class LearningMemory:
    """
    Système de mémoire d'apprentissage inspiré de GEN8-24
    
    Architecture:
    - Short-term: Expériences session courante
    - Mid-term: Expériences dernières 10 sessions
    - Long-term: Patterns d'échecs consolidés
    - Deep archive: Toutes expériences (append-only)
    """
    
    def __init__(self, memory_dir: Path = Path("memory")):
        self.memory_dir = memory_dir
        self.memory_dir.mkdir(exist_ok=True)
        
        # Multi-timescale memory (GEN11)
        self.short_term: List[PuzzleExperience] = []
        self.mid_term: List[PuzzleExperience] = []
        self.long_term: List[FailurePattern] = []
        
        # Deep archive (GEN8 - append-only)
        self.archive_path = self.memory_dir / "deep_archive.jsonl"
        
        # Semantic structures (GEN11)
        self.failure_patterns: Dict[str, FailurePattern] = {}
        self.success_patterns: Dict[str, List[str]] = {}  # classification -> puzzle_ids
        
        # Identity & causal (GEN8)
        self.identity_field = {}  # puzzle_id -> learned_strategy
        self.causal_sequences = []  # [(failure, correction, result)]
        
        # C8 FIX: Ajouter référence au transformation engine pour reset causal chain
        self.transformation_engine = None  # Sera injecté par le pipeline
        
        # Load existing memory
        self._load_memory()
    
    def _load_memory(self):
        """Charge mémoire existante depuis archive"""
        if self.archive_path.exists():
            with open(self.archive_path) as f:
                for line in f:
                    exp = json.loads(line)
                    experience = PuzzleExperience(**exp)
                    self.mid_term.append(experience)
                    
                    # Consolider patterns
                    if not experience.success and experience.failure_reason:
                        self._update_failure_pattern(experience)
                    elif experience.success:
                        self._update_success_pattern(experience)
        
        # Consolider long-term
        self._consolidate_memory()
    
    def store_experience(self, experience: PuzzleExperience):
        """
        Stocke expérience puzzle (GEN8 append-only)
        """
        # Short-term
        self.short_term.append(experience)
        
        # Deep archive (append-only)
        with open(self.archive_path, 'a') as f:
            f.write(json.dumps(experience.to_dict()) + '\n')
        
        # Update patterns
        if not experience.success and experience.failure_reason:
            self._update_failure_pattern(experience)
        elif experience.success:
            self._update_success_pattern(experience)
        
        # Consolidation si nécessaire
        if len(self.short_term) > 50:
            self._consolidate_memory()
    
    def _update_failure_pattern(self, experience: PuzzleExperience):
        """Met à jour patterns d'échecs"""
        pattern_type = experience.failure_reason
        
        # Skip si pas de failure_reason
        if pattern_type is None:
            return
        
        if pattern_type not in self.failure_patterns:
            self.failure_patterns[pattern_type] = FailurePattern(
                pattern_type=pattern_type,
                puzzle_ids=[experience.puzzle_id],
                frequency=1,
                last_seen=experience.timestamp,
                correction_attempted=False,
                correction_successful=None
            )
        else:
            pattern = self.failure_patterns[pattern_type]
            if experience.puzzle_id not in pattern.puzzle_ids:
                pattern.puzzle_ids.append(experience.puzzle_id)
            pattern.frequency += 1
            pattern.last_seen = experience.timestamp
    
    def _update_success_pattern(self, experience: PuzzleExperience):
        """Met à jour patterns de succès"""
        classification = experience.classification
        
        if classification not in self.success_patterns:
            self.success_patterns[classification] = []
        
        if experience.puzzle_id not in self.success_patterns[classification]:
            self.success_patterns[classification].append(experience.puzzle_id)
    
    def _consolidate_memory(self):
        """
        Consolidation mémoire (GEN11 multi-timescale)
        Short → Mid → Long
        """
        # Migrer short → mid
        if len(self.short_term) > 50:
            migrated = self.short_term[:20]
            self.mid_term.extend(migrated)
            self.short_term = self.short_term[20:]
        
        # Migrer mid → long (patterns consolidés)
        if len(self.mid_term) > 200:
            # Extraire patterns dominants
            for pattern_type, pattern in self.failure_patterns.items():
                if pattern.frequency >= 3:  # Pattern récurrent
                    if pattern not in self.long_term:
                        self.long_term.append(pattern)
            
            # Garder seulement 100 expériences mid-term
            self.mid_term = self.mid_term[-100:]
    
    def get_failure_patterns(self, min_frequency: int = 2) -> List[FailurePattern]:
        """Retourne patterns d'échecs récurrents"""
        return [p for p in self.failure_patterns.values() 
                if p.frequency >= min_frequency]
    
    def get_similar_puzzles(self, puzzle_id: str, classification: str) -> List[str]:
        """
        Trouve puzzles similaires déjà résolus (GEN11 semantic binding)
        """
        if classification in self.success_patterns:
            similar = self.success_patterns[classification]
            # Exclure puzzle courant
            return [p for p in similar if p != puzzle_id]
        return []
    
    def record_correction_attempt(self, pattern_type: str, success: bool):
        """Enregistre tentative de correction d'un pattern"""
        if pattern_type in self.failure_patterns:
            pattern = self.failure_patterns[pattern_type]
            pattern.correction_attempted = True
            pattern.correction_successful = success
            
            # Causal sequence (GEN8)
            self.causal_sequences.append({
                'failure_pattern': pattern_type,
                'correction_success': success,
                'timestamp': datetime.now().isoformat()
            })
    
    def should_retry_puzzle(self, puzzle_id: str) -> Tuple[bool, Optional[str]]:
        """
        Détermine si un puzzle devrait être réessayé avec nouvelle stratégie
        
        Returns:
            (should_retry, suggested_strategy)
        """
        # Chercher expériences passées
        past_experiences = [exp for exp in self.mid_term + self.short_term
                          if exp.puzzle_id == puzzle_id]
        
        if not past_experiences:
            return False, None
        
        # Si échec récurrent avec même stratégie
        recent_failures = [exp for exp in past_experiences[-3:]
                          if not exp.success]
        
        if len(recent_failures) >= 2:
            # Suggérer stratégie alternative basée sur classification
            last_failure = recent_failures[-1]
            classification = last_failure.classification
            
            # Trouver puzzles similaires résolus
            similar_solved = self.get_similar_puzzles(puzzle_id, classification)
            
            if similar_solved:
                # Récupérer stratégie qui a marché
                for exp in reversed(self.mid_term + self.short_term):
                    if exp.puzzle_id in similar_solved and exp.success:
                        return True, exp.solution_program
        
        return False, None
    
    def get_learning_stats(self) -> Dict:
        """Statistiques d'apprentissage"""
        total_experiences = len(self.short_term) + len(self.mid_term)
        
        successes = sum(1 for exp in self.short_term + self.mid_term if exp.success)
        failures = total_experiences - successes
        
        return {
            'total_experiences': total_experiences,
            'short_term_size': len(self.short_term),
            'mid_term_size': len(self.mid_term),
            'long_term_patterns': len(self.long_term),
            'success_rate': successes / total_experiences if total_experiences > 0 else 0,
            'failure_patterns': len(self.failure_patterns),
            'causal_sequences': len(self.causal_sequences),
            'classifications_learned': len(self.success_patterns)
        }
    
    def export_knowledge_base(self) -> Dict:
        """Exporte base de connaissances pour analyse"""
        return {
            'failure_patterns': [asdict(p) for p in self.failure_patterns.values()],
            'success_patterns': self.success_patterns,
            'causal_sequences': self.causal_sequences,
            'stats': self.get_learning_stats()
        }


def compute_train_hash(train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> str:
    """Calcule hash unique des train pairs"""
    content = ""
    for inp, out in train_pairs:
        content += inp.tobytes().hex() + out.tobytes().hex()
    return hashlib.sha256(content.encode()).hexdigest()[:16]

# Made with Bob
