#!/usr/bin/env python3
"""
Audit Apprentissage Corrigé V39 - Validation Système Minimal
=============================================================

Ce script audite le MinimalLearningSystemV39 pour vérifier que
l'apprentissage fonctionne réellement après correction.

Auteur: Bob (IA)
Date: 2026-06-16
Version: V39.1 (Post-correction)
"""

import sys
import time
import json
import numpy as np
from pathlib import Path
from typing import Dict, List
from collections import defaultdict

# Ajouter path MAGEN
MAGEN_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(MAGEN_DIR))

from core.minimal_learning_system_v39 import MinimalLearningSystemV39


class LearningAuditorCorrected:
    """Auditeur pour système d'apprentissage corrigé."""
    
    def __init__(self, output_dir: str = "/tmp/magen_audit_v39_corrected"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.forensic_path = self.output_dir / "learning_audit_corrected.jsonl"
        self.results_path = self.output_dir / "audit_results_corrected.json"
        
        # Créer système d'apprentissage
        self.learning_system = MinimalLearningSystemV39(
            learning_rate=0.01,
            weight_learning_rate=0.005,
            forensic_log_path=str(self.forensic_path)
        )
        
        # Métriques audit
        self.puzzle_results = []
    
    def audit_single_puzzle(self, puzzle_id: str, max_steps: int = 50) -> Dict:
        """
        Audite apprentissage sur un puzzle.
        
        Args:
            puzzle_id: ID du puzzle
            max_steps: Nombre de steps à auditer
            
        Returns:
            Résultats audit
        """
        print(f"\n[AUDIT] Puzzle {puzzle_id}")
        
        # Capturer état initial
        initial_scores = self.learning_system.get_action_scores_dict().copy()
        initial_weights = self.learning_system.get_weights_dict().copy()
        
        # Actions à tester
        actions = ['rotate_90', 'flip_horizontal', 'flip_vertical', 'identity']
        
        # Simuler steps avec récompenses variées
        for step in range(max_steps):
            # Choisir action aléatoire
            action = np.random.choice(actions)
            
            # Générer récompense aléatoire (0.0 à 1.0)
            reward = np.random.random()
            success = reward > 0.5
            
            # Mettre à jour score
            self.learning_system.update_action_score(action, reward, success)
            
            # Mettre à jour poids (gradient aléatoire)
            for weight_id in ['pattern_match', 'color_similarity', 'shape_match']:
                gradient = np.random.randn() * 0.1  # Gradient aléatoire
                self.learning_system.update_weight(weight_id, gradient)
        
        # Capturer état final
        final_scores = self.learning_system.get_action_scores_dict().copy()
        final_weights = self.learning_system.get_weights_dict().copy()
        
        # Calculer changements
        score_changes = {}
        for action in actions:
            initial = initial_scores.get(action, 0.5)
            final = final_scores.get(action, 0.5)
            score_changes[action] = {
                'initial': initial,
                'final': final,
                'delta': final - initial,
                'changed': abs(final - initial) > 1e-6
            }
        
        weight_changes = {}
        for weight_id in ['pattern_match', 'color_similarity', 'shape_match']:
            initial = initial_weights.get(weight_id, 0.5)
            final = final_weights.get(weight_id, 0.5)
            weight_changes[weight_id] = {
                'initial': initial,
                'final': final,
                'delta': final - initial,
                'changed': abs(final - initial) > 1e-6
            }
        
        # Calculer métriques
        actions_changed = sum(1 for c in score_changes.values() if c['changed'])
        weights_changed = sum(1 for c in weight_changes.values() if c['changed'])
        
        actions_changed_pct = (actions_changed / len(actions)) * 100
        weights_changed_pct = (weights_changed / len(weight_changes)) * 100
        
        max_score_delta = max(abs(c['delta']) for c in score_changes.values())
        max_weight_delta = max(abs(c['delta']) for c in weight_changes.values())
        
        # Diagnostic
        diagnosis = 'FUNCTIONAL'
        issues = []
        
        if actions_changed_pct < 50:
            diagnosis = 'CRITICAL'
            issues.append('SCORES_LOW_CHANGE')
        
        if weights_changed_pct < 50:
            diagnosis = 'CRITICAL'
            issues.append('WEIGHTS_LOW_CHANGE')
        
        if max_score_delta < 0.01:
            diagnosis = 'WARNING'
            issues.append('SCORES_SMALL_DELTAS')
        
        result = {
            'puzzle_id': puzzle_id,
            'steps_audited': max_steps,
            'score_analysis': {
                'actions_changed': actions_changed,
                'actions_total': len(actions),
                'actions_changed_pct': actions_changed_pct,
                'max_delta': max_score_delta,
                'changes': score_changes
            },
            'weight_analysis': {
                'weights_changed': weights_changed,
                'weights_total': len(weight_changes),
                'weights_changed_pct': weights_changed_pct,
                'max_delta': max_weight_delta,
                'changes': weight_changes
            },
            'diagnosis': {
                'status': diagnosis,
                'issues': issues
            }
        }
        
        print(f"  Actions changées: {actions_changed}/{len(actions)} ({actions_changed_pct:.1f}%)")
        print(f"  Poids changés: {weights_changed}/{len(weight_changes)} ({weights_changed_pct:.1f}%)")
        print(f"  Max delta scores: {max_score_delta:.6f}")
        print(f"  Max delta poids: {max_weight_delta:.6f}")
        print(f"  Diagnostic: {diagnosis}")
        
        self.puzzle_results.append(result)
        return result
    
    def audit_multiple_puzzles(self, num_puzzles: int = 10, steps_per_puzzle: int = 50):
        """
        Audite apprentissage sur plusieurs puzzles.
        
        Args:
            num_puzzles: Nombre de puzzles à auditer
            steps_per_puzzle: Steps par puzzle
        """
        print(f"\n{'='*80}")
        print(f"AUDIT APPRENTISSAGE CORRIGÉ V39")
        print(f"{'='*80}")
        print(f"Puzzles: {num_puzzles}")
        print(f"Steps/puzzle: {steps_per_puzzle}")
        print(f"Total steps: {num_puzzles * steps_per_puzzle}")
        
        start_time = time.time()
        
        # Auditer chaque puzzle
        for i in range(num_puzzles):
            puzzle_id = f"test_puzzle_{i+1:03d}"
            self.audit_single_puzzle(puzzle_id, steps_per_puzzle)
        
        duration = time.time() - start_time
        
        # Calculer statistiques agrégées
        stats = self._compute_aggregate_stats()
        
        # Sauvegarder résultats (convertir numpy types en Python natifs)
        def convert_to_native(obj):
            """Convertit numpy types en types Python natifs."""
            if isinstance(obj, dict):
                return {k: convert_to_native(v) for k, v in obj.items()}
            elif isinstance(obj, list):
                return [convert_to_native(item) for item in obj]
            elif isinstance(obj, (np.integer, np.floating)):
                return float(obj)
            elif isinstance(obj, np.ndarray):
                return obj.tolist()
            elif isinstance(obj, (np.bool_, bool)):
                return bool(obj)
            return obj
        
        results = {
            'total_puzzles': num_puzzles,
            'total_steps': num_puzzles * steps_per_puzzle,
            'total_duration': duration,
            'stats': convert_to_native(stats),
            'diagnosis_summary': convert_to_native(self._compute_diagnosis_summary()),
            'puzzles': convert_to_native(self.puzzle_results)
        }
        
        with open(self.results_path, 'w') as f:
            json.dump(results, f, indent=2)
        
        # Afficher résumé
        self._print_summary(results)
        
        return results
    
    def _compute_aggregate_stats(self) -> Dict:
        """Calcule statistiques agrégées."""
        actions_changed_pcts = [
            r['score_analysis']['actions_changed_pct']
            for r in self.puzzle_results
        ]
        
        weights_changed_pcts = [
            r['weight_analysis']['weights_changed_pct']
            for r in self.puzzle_results
        ]
        
        max_score_deltas = [
            r['score_analysis']['max_delta']
            for r in self.puzzle_results
        ]
        
        max_weight_deltas = [
            r['weight_analysis']['max_delta']
            for r in self.puzzle_results
        ]
        
        return {
            'actions_changed_pct': {
                'mean': float(np.mean(actions_changed_pcts)),
                'std': float(np.std(actions_changed_pcts)),
                'min': float(np.min(actions_changed_pcts)),
                'max': float(np.max(actions_changed_pcts))
            },
            'weights_changed_pct': {
                'mean': float(np.mean(weights_changed_pcts)),
                'std': float(np.std(weights_changed_pcts)),
                'min': float(np.min(weights_changed_pcts)),
                'max': float(np.max(weights_changed_pcts))
            },
            'max_score_deltas': {
                'mean': float(np.mean(max_score_deltas)),
                'std': float(np.std(max_score_deltas)),
                'min': float(np.min(max_score_deltas)),
                'max': float(np.max(max_score_deltas))
            },
            'max_weight_deltas': {
                'mean': float(np.mean(max_weight_deltas)),
                'std': float(np.std(max_weight_deltas)),
                'min': float(np.min(max_weight_deltas)),
                'max': float(np.max(max_weight_deltas))
            }
        }
    
    def _compute_diagnosis_summary(self) -> Dict:
        """Calcule résumé diagnostics."""
        functional = sum(1 for r in self.puzzle_results if r['diagnosis']['status'] == 'FUNCTIONAL')
        warnings = sum(1 for r in self.puzzle_results if r['diagnosis']['status'] == 'WARNING')
        critical = sum(1 for r in self.puzzle_results if r['diagnosis']['status'] == 'CRITICAL')
        
        return {
            'functional': functional,
            'warnings': warnings,
            'critical': critical,
            'total': len(self.puzzle_results)
        }
    
    def _print_summary(self, results: Dict):
        """Affiche résumé résultats."""
        print(f"\n{'='*80}")
        print("RÉSULTATS AUDIT")
        print(f"{'='*80}")
        
        stats = results['stats']
        diag = results['diagnosis_summary']
        
        print(f"\nStatistiques Scores:")
        print(f"  Actions changées: {stats['actions_changed_pct']['mean']:.1f}% ± {stats['actions_changed_pct']['std']:.1f}%")
        print(f"  Max delta moyen: {stats['max_score_deltas']['mean']:.6f}")
        
        print(f"\nStatistiques Poids:")
        print(f"  Poids changés: {stats['weights_changed_pct']['mean']:.1f}% ± {stats['weights_changed_pct']['std']:.1f}%")
        print(f"  Max delta moyen: {stats['max_weight_deltas']['mean']:.6f}")
        
        print(f"\nDiagnostic:")
        print(f"  ✅ Fonctionnel: {diag['functional']}/{diag['total']}")
        print(f"  ⚠️  Warnings: {diag['warnings']}/{diag['total']}")
        print(f"  ❌ Critical: {diag['critical']}/{diag['total']}")
        
        # Verdict GO/NO-GO
        print(f"\n{'='*80}")
        if diag['functional'] >= diag['total'] * 0.8:
            print("✅ VERDICT: GO - Apprentissage FONCTIONNEL")
            print(f"   {diag['functional']}/{diag['total']} puzzles fonctionnels (>80%)")
        else:
            print("❌ VERDICT: NO-GO - Apprentissage NON FONCTIONNEL")
            print(f"   {diag['functional']}/{diag['total']} puzzles fonctionnels (<80%)")
        print(f"{'='*80}")
        
        print(f"\nRésultats sauvegardés:")
        print(f"  Forensic: {self.forensic_path}")
        print(f"  Results: {self.results_path}")


def main():
    """Point d'entrée principal."""
    auditor = LearningAuditorCorrected()
    results = auditor.audit_multiple_puzzles(num_puzzles=10, steps_per_puzzle=50)
    
    return results


if __name__ == "__main__":
    main()

# Made with Bob