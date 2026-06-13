"""
MAGEN Program Synthesizer - Génération automatique de programmes ARC

Ce module implémente le cœur du système de synthesis: génération de programmes
candidats à partir de train pairs, avec scoring et validation.
"""

import numpy as np
from typing import List, Dict, Tuple, Optional, Set
from dataclasses import dataclass
import time

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dsl.program import Program, simplify_program
from dsl.composer import Composer, generate_program_variants
from .scorer import ProgramScorer
from .validator import ProgramValidator


@dataclass
class SynthesisResult:
    """Résultat de la synthesis"""
    program: Program
    score: float
    train_accuracy: float
    execution_time: float
    metadata: Dict = None
    
    def __post_init__(self):
        if self.metadata is None:
            self.metadata = {}


class ProgramSynthesizer:
    """
    Synthétiseur de programmes ARC
    
    Génère automatiquement des programmes candidats en analysant les train pairs
    et en composant des primitives DSL.
    
    Architecture:
        1. Analyse train pairs → Détection patterns
        2. Génération candidats → Composer
        3. Scoring candidats → ProgramScorer
        4. Validation → ProgramValidator
        5. Sélection meilleur programme
    """
    
    def __init__(self, 
                 max_candidates: int = 100,
                 max_depth: int = 5,
                 timeout: float = 5.0,
                 enable_simplification: bool = True):
        """
        Args:
            max_candidates: Nombre maximum de programmes candidats
            max_depth: Profondeur maximale de composition
            timeout: Timeout en secondes pour la synthesis
            enable_simplification: Activer simplification programmes
        """
        self.max_candidates = max_candidates
        self.max_depth = max(max_depth, 7)  # Minimum 7 pour patterns complexes
        self.timeout = timeout
        self.enable_simplification = enable_simplification
        
        # Composants
        self.composer = Composer(max_depth=max_depth, max_candidates=max_candidates)
        self.scorer = ProgramScorer()
        self.validator = ProgramValidator()
        
        # Statistiques
        self.stats = {
            'total_syntheses': 0,
            'successful_syntheses': 0,
            'total_candidates_generated': 0,
            'total_candidates_validated': 0,
            'avg_synthesis_time': 0.0
        }
    
    def synthesize(self, train_pairs: List[Dict]) -> Optional[SynthesisResult]:
        """
        Synthétise un programme à partir de train pairs
        
        Args:
            train_pairs: Liste de dicts avec 'input' et 'output' (np.ndarray)
            
        Returns:
            SynthesisResult avec meilleur programme, ou None si échec
        """
        start_time = time.time()
        self.stats['total_syntheses'] += 1
        
        if not train_pairs:
            return None
        
        try:
            # Phase 1: Génération candidats
            candidates = self._generate_candidates(train_pairs)
            self.stats['total_candidates_generated'] += len(candidates)
            
            if not candidates:
                return None
            
            # Phase 2: Scoring et filtrage
            scored_candidates = self._score_candidates(candidates, train_pairs)
            
            if not scored_candidates:
                return None
            
            # Phase 3: Validation et sélection
            best_result = self._select_best_program(scored_candidates, train_pairs)
            
            # Statistiques
            elapsed = time.time() - start_time
            if best_result:
                self.stats['successful_syntheses'] += 1
                best_result.execution_time = elapsed
            
            # Mise à jour moyenne
            n = self.stats['total_syntheses']
            self.stats['avg_synthesis_time'] = (
                (self.stats['avg_synthesis_time'] * (n-1) + elapsed) / n
            )
            
            return best_result
            
        except Exception as e:
            print(f"Synthesis error: {e}")
            return None
    
    def _generate_candidates(self, train_pairs: List[Dict]) -> List[Program]:
        """
        Génère programmes candidats
        
        Stratégie multi-niveaux:
        1. Programmes basés sur analyse input→output
        2. Programmes à 1 opération
        3. Programmes à 2 opérations
        4. Variantes des meilleurs
        """
        candidates = []
        first_pair = train_pairs[0]
        input_grid = first_pair['input']
        output_grid = first_pair['output']
        
        # Stratégie 1: Analyse-driven
        analysis_programs = self.composer.generate_programs_from_analysis(
            input_grid, output_grid, max_programs=30
        )
        print(f"  [DEBUG] Analysis programs: {len(analysis_programs)}")
        candidates.extend(analysis_programs)
        
        # Stratégie 2: Single-op exhaustif
        if len(candidates) < self.max_candidates // 2:
            single_op = self.composer.generate_single_op_programs(input_grid)
            print(f"  [DEBUG] Single-op programs: {len(single_op)}")
            candidates.extend(single_op[:self.max_candidates // 2])
        
        # Stratégie 3: Two-op ciblé
        if len(candidates) < self.max_candidates:
            two_op = self.composer.generate_two_op_programs(
                input_grid, categories=['geometric', 'scale', 'color']
            )
            candidates.extend(two_op[:self.max_candidates - len(candidates)])
        
        # Stratégie 4: Variantes des meilleurs (si on a déjà des candidats)
        if len(candidates) > 10 and len(candidates) < self.max_candidates:
            # Tester rapidement les 5 premiers
            quick_scores = []
            for prog in candidates[:5]:
                try:
                    result = prog.execute(input_grid)
                    if np.array_equal(result, output_grid):
                        quick_scores.append((prog, 1.0))
                    else:
                        # Score basique: pixels corrects
                        if result.shape == output_grid.shape:
                            correct = np.sum(result == output_grid)
                            total = output_grid.size
                            quick_scores.append((prog, correct / total))
                except Exception:
                    pass
            
            # Générer variantes des meilleurs
            if quick_scores:
                quick_scores.sort(key=lambda x: x[1], reverse=True)
                for prog, score in quick_scores[:3]:
                    if score > 0.5:
                        variants = generate_program_variants(prog, max_variants=5)
                        candidates.extend(variants)
        
        # Dédupliquer
        unique_candidates = []
        seen = set()
        for prog in candidates:
            prog_hash = hash(prog)
            if prog_hash not in seen:
                seen.add(prog_hash)
                unique_candidates.append(prog)
        
        print(f"  [DEBUG] Total unique candidates: {len(unique_candidates)}")
        return unique_candidates[:self.max_candidates]
    
    def _score_candidates(self,
                         candidates: List[Program],
                         train_pairs: List[Dict]) -> List[Tuple[Program, float]]:
        """
        Score tous les candidats sur les train pairs
        
        Returns:
            Liste de (program, score) triée par score décroissant
        """
        scored = []
        
        for program in candidates:
            try:
                score = self.scorer.score_program(program, train_pairs)
                scored.append((program, score))
                self.stats['total_candidates_validated'] += 1
            except Exception as e:
                # Programme invalide, logger et ignorer
                print(f"  [DEBUG] Scoring failed for {program}: {e}")
                continue
        
        # Trier par score décroissant
        scored.sort(key=lambda x: x[1], reverse=True)
        
        return scored
    
    def _select_best_program(self,
                            scored_candidates: List[Tuple[Program, float]],
                            train_pairs: List[Dict]) -> Optional[SynthesisResult]:
        """
        Sélectionne le meilleur programme parmi les candidats scorés
        
        Critères:
        1. Exactitude sur train pairs (priorité absolue)
        2. Simplicité (MDL)
        3. Généralité
        """
        if not scored_candidates:
            return None
        
        # Chercher premier programme avec 100% exactitude
        for program, score in scored_candidates:
            # Valider exactitude
            is_valid, train_acc = self.validator.validate_on_train(
                program, train_pairs
            )
            
            if is_valid and train_acc == 1.0:
                # Programme parfait trouvé!
                
                # Simplifier si activé
                if self.enable_simplification:
                    simplified = simplify_program(program)
                    # Vérifier que simplification n'a pas cassé
                    is_valid_simp, train_acc_simp = self.validator.validate_on_train(
                        simplified, train_pairs
                    )
                    if is_valid_simp and train_acc_simp == 1.0:
                        program = simplified
                
                return SynthesisResult(
                    program=program,
                    score=score,
                    train_accuracy=train_acc,
                    execution_time=0.0,  # Sera mis à jour
                    metadata={
                        'complexity': program.complexity(),
                        'num_operations': program.length(),
                        'simplified': self.enable_simplification
                    }
                )
        
        # Aucun programme parfait, retourner le meilleur
        best_program, best_score = scored_candidates[0]
        is_valid, train_acc = self.validator.validate_on_train(
            best_program, train_pairs
        )
        
        if is_valid:
            return SynthesisResult(
                program=best_program,
                score=best_score,
                train_accuracy=train_acc,
                execution_time=0.0,
                metadata={
                    'complexity': best_program.complexity(),
                    'num_operations': best_program.length(),
                    'perfect_match': False
                }
            )
        
        return None
    
    def synthesize_with_feedback(self,
                                train_pairs: List[Dict],
                                max_iterations: int = 3) -> Optional[SynthesisResult]:
        """
        Synthesis itérative avec feedback
        
        Si première tentative échoue, analyse les erreurs et réessaie
        avec des stratégies adaptées.
        
        Args:
            train_pairs: Train pairs
            max_iterations: Nombre maximum d'itérations
            
        Returns:
            SynthesisResult ou None
        """
        for iteration in range(max_iterations):
            result = self.synthesize(train_pairs)
            
            if result and result.train_accuracy == 1.0:
                # Succès!
                result.metadata['iterations'] = iteration + 1
                return result
            
            # Échec, analyser et adapter stratégie
            if iteration < max_iterations - 1:
                # Augmenter nombre de candidats pour prochaine itération
                self.max_candidates = min(self.max_candidates * 2, 500)
                self.composer.max_candidates = self.max_candidates
        
        # Retourner meilleur résultat même si imparfait
        return result
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques de synthesis"""
        stats = self.stats.copy()
        if stats['total_syntheses'] > 0:
            stats['success_rate'] = (
                stats['successful_syntheses'] / stats['total_syntheses']
            )
        else:
            stats['success_rate'] = 0.0
        
        return stats
    
    def reset_statistics(self):
        """Réinitialise les statistiques"""
        self.stats = {
            'total_syntheses': 0,
            'successful_syntheses': 0,
            'total_candidates_generated': 0,
            'total_candidates_validated': 0,
            'avg_synthesis_time': 0.0
        }


__all__ = ['ProgramSynthesizer', 'SynthesisResult']

# Made with Bob
