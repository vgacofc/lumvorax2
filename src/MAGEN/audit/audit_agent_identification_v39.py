"""
PHASE 2 V39: Audit Agent Identification
Vérifie si MAGEN sait où il est dans le puzzle.

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import numpy as np
from typing import Dict, List
from datetime import datetime
import json
from pathlib import Path

from core.agent_localization_v39 import AgentLocalizationSystem, AgentPosition


class AgentIdentificationAuditor:
    """Auditeur pour vérifier l'identification de l'agent."""
    
    def __init__(self, output_dir: str = '/tmp/magen_audit_v39_phase2'):
        """Initialise l'auditeur."""
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.forensic_path = self.output_dir / 'agent_identification_audit.jsonl'
        self.results_path = self.output_dir / 'audit_results_phase2.json'
        
        self.test_results: List[Dict] = []
        self.forensic_events: List[Dict] = []
    
    def audit_single_puzzle(self, puzzle_id: str, grid_width: int, 
                           grid_height: int, num_steps: int) -> Dict:
        """
        Audite l'identification de l'agent sur un puzzle.
        
        Args:
            puzzle_id: ID du puzzle
            grid_width: Largeur grille
            grid_height: Hauteur grille
            num_steps: Nombre de steps à simuler
        
        Returns:
            Résultats de l'audit
        """
        print(f"\n[AUDIT] Puzzle {puzzle_id}")
        
        # Créer système de localisation
        system = AgentLocalizationSystem(grid_width, grid_height)
        
        # Simuler mouvements
        confidence_history = []
        coverage_history = []
        
        for step in range(num_steps):
            # Mouvement semi-aléatoire (exploration intelligente)
            if step == 0:
                # Position initiale aléatoire
                x = np.random.randint(0, grid_width)
                y = np.random.randint(0, grid_height)
            else:
                # Suggérer prochaine position ou mouvement aléatoire
                suggestion = system.suggest_next_position()
                if suggestion and np.random.random() > 0.3:
                    x, y = suggestion
                else:
                    # Mouvement local aléatoire
                    current = system.get_current_position()
                    if current:
                        dx = np.random.randint(-2, 3)
                        dy = np.random.randint(-2, 3)
                        x = np.clip(current.x + dx, 0, grid_width - 1)
                        y = np.clip(current.y + dy, 0, grid_height - 1)
                    else:
                        x = np.random.randint(0, grid_width)
                        y = np.random.randint(0, grid_height)
            
            # Mettre à jour position
            region_id = f"region_{x//3}_{y//3}"
            system.update_position(x, y, step, region_id)
            
            # Tracker métriques
            status = system.get_exploration_status()
            confidence_history.append(status['confidence'])
            coverage_history.append(status['coverage'])
        
        # Statistiques finales
        final_stats = system.get_stats()
        final_status = system.get_exploration_status()
        
        # Calculer métriques d'audit
        avg_confidence = np.mean(confidence_history)
        final_confidence = confidence_history[-1]
        confidence_growth = final_confidence - confidence_history[0]
        
        avg_coverage = np.mean(coverage_history)
        final_coverage = coverage_history[-1]
        
        # Diagnostic (seuil ajusté à 85%)
        is_identified = final_confidence >= 0.85
        
        if is_identified:
            diagnostic = 'FUNCTIONAL'
        elif final_confidence >= 0.7:
            diagnostic = 'WARNING'
        else:
            diagnostic = 'CRITICAL'
        
        result = {
            'puzzle_id': puzzle_id,
            'grid_size': {'width': grid_width, 'height': grid_height},
            'num_steps': num_steps,
            'final_confidence': final_confidence,
            'avg_confidence': avg_confidence,
            'confidence_growth': confidence_growth,
            'final_coverage': final_coverage,
            'avg_coverage': avg_coverage,
            'unique_positions': final_stats['unique_positions'],
            'revisits': final_stats['revisits'],
            'is_identified': is_identified,
            'diagnostic': diagnostic
        }
        
        # Afficher résumé
        print(f"  Confiance finale: {final_confidence:.1%}")
        print(f"  Couverture finale: {final_coverage:.1%}")
        print(f"  Positions uniques: {final_stats['unique_positions']}/{num_steps}")
        print(f"  Agent identifié: {is_identified}")
        print(f"  Diagnostic: {diagnostic}")
        
        # Log forensic
        self._log_forensic_event({
            'event': 'puzzle_audit_complete',
            'puzzle_id': puzzle_id,
            'result': result
        })
        
        return result
    
    def audit_multiple_puzzles(self, num_puzzles: int = 10, 
                              steps_per_puzzle: int = 100) -> Dict:
        """
        Audite l'identification sur plusieurs puzzles.
        
        Args:
            num_puzzles: Nombre de puzzles à tester
            steps_per_puzzle: Steps par puzzle
        
        Returns:
            Résultats agrégés
        """
        print("="*80)
        print("AUDIT AGENT IDENTIFICATION V39")
        print("="*80)
        print(f"Puzzles: {num_puzzles}")
        print(f"Steps/puzzle: {steps_per_puzzle}")
        print(f"Total steps: {num_puzzles * steps_per_puzzle}")
        
        start_time = datetime.now()
        
        # Tester différentes tailles de grilles
        grid_sizes = [
            (10, 10),  # Petite
            (15, 15),  # Moyenne
            (20, 20),  # Grande
            (10, 20),  # Rectangle
            (30, 10),  # Rectangle large
        ]
        
        for i in range(num_puzzles):
            puzzle_id = f"test_puzzle_{i+1:03d}"
            grid_width, grid_height = grid_sizes[i % len(grid_sizes)]
            
            result = self.audit_single_puzzle(
                puzzle_id, grid_width, grid_height, steps_per_puzzle
            )
            self.test_results.append(result)
        
        duration = (datetime.now() - start_time).total_seconds()
        
        # Calculer statistiques agrégées
        stats = self._compute_aggregate_stats()
        
        # Convertir bool en int pour JSON
        def convert_to_native(obj):
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
        
        # Sauvegarder résultats
        results = {
            'total_puzzles': num_puzzles,
            'total_steps': num_puzzles * steps_per_puzzle,
            'total_duration': duration,
            'stats': convert_to_native(stats),
            'diagnosis_summary': convert_to_native(self._compute_diagnosis_summary()),
            'puzzles': convert_to_native(self.test_results)
        }
        
        with open(self.results_path, 'w') as f:
            json.dump(results, f, indent=2)
        
        # Sauvegarder forensic (convertir types)
        with open(self.forensic_path, 'w') as f:
            for event in self.forensic_events:
                f.write(json.dumps(convert_to_native(event)) + '\n')
        
        # Afficher résumé
        self._print_summary(results)
        
        return results
    
    def _compute_aggregate_stats(self) -> Dict:
        """Calcule statistiques agrégées."""
        confidences = [r['final_confidence'] for r in self.test_results]
        coverages = [r['final_coverage'] for r in self.test_results]
        identified_count = sum(1 for r in self.test_results if r['is_identified'])
        
        return {
            'confidence': {
                'mean': float(np.mean(confidences)),
                'std': float(np.std(confidences)),
                'min': float(np.min(confidences)),
                'max': float(np.max(confidences))
            },
            'coverage': {
                'mean': float(np.mean(coverages)),
                'std': float(np.std(coverages)),
                'min': float(np.min(coverages)),
                'max': float(np.max(coverages))
            },
            'identification_rate': identified_count / len(self.test_results)
        }
    
    def _compute_diagnosis_summary(self) -> Dict:
        """Calcule résumé diagnostics."""
        functional = sum(1 for r in self.test_results if r['diagnostic'] == 'FUNCTIONAL')
        warnings = sum(1 for r in self.test_results if r['diagnostic'] == 'WARNING')
        critical = sum(1 for r in self.test_results if r['diagnostic'] == 'CRITICAL')
        
        return {
            'functional': functional,
            'warnings': warnings,
            'critical': critical,
            'total': len(self.test_results)
        }
    
    def _print_summary(self, results: Dict):
        """Affiche résumé résultats."""
        print(f"\n{'='*80}")
        print("RÉSULTATS AUDIT")
        print("="*80)
        
        stats = results['stats']
        diagnosis = results['diagnosis_summary']
        
        print(f"\nStatistiques Confiance:")
        print(f"  Moyenne: {stats['confidence']['mean']:.1%} ± {stats['confidence']['std']:.1%}")
        print(f"  Min/Max: {stats['confidence']['min']:.1%} / {stats['confidence']['max']:.1%}")
        
        print(f"\nStatistiques Couverture:")
        print(f"  Moyenne: {stats['coverage']['mean']:.1%} ± {stats['coverage']['std']:.1%}")
        
        print(f"\nTaux Identification:")
        print(f"  {stats['identification_rate']:.1%} ({int(stats['identification_rate'] * len(self.test_results))}/{len(self.test_results)} puzzles)")
        
        print(f"\nDiagnostic:")
        print(f"  ✅ Fonctionnel: {diagnosis['functional']}/{diagnosis['total']}")
        print(f"  ⚠️  Warnings: {diagnosis['warnings']}/{diagnosis['total']}")
        print(f"  ❌ Critical: {diagnosis['critical']}/{diagnosis['total']}")
        
        # Verdict (seuil ajusté à 85%)
        print(f"\n{'='*80}")
        if stats['identification_rate'] >= 0.85:
            print("✅ VERDICT: GO - Agent correctement identifié (>85%)")
        elif stats['identification_rate'] >= 0.7:
            print("⚠️  VERDICT: WARNING - Identification partielle (70-85%)")
        else:
            print("❌ VERDICT: NO-GO - Identification insuffisante (<70%)")
        print("="*80)
        
        print(f"\nRésultats sauvegardés:")
        print(f"  Forensic: {self.forensic_path}")
        print(f"  Results: {self.results_path}")
    
    def _log_forensic_event(self, event: Dict):
        """Log événement forensic."""
        event['timestamp'] = datetime.now().isoformat()
        self.forensic_events.append(event)


def main():
    """Point d'entrée principal."""
    auditor = AgentIdentificationAuditor()
    results = auditor.audit_multiple_puzzles(num_puzzles=10, steps_per_puzzle=100)
    
    # Retourner code sortie basé sur verdict (seuil 85%)
    if results['stats']['identification_rate'] >= 0.85:
        sys.exit(0)  # GO
    else:
        sys.exit(1)  # NO-GO


if __name__ == '__main__':
    main()

# Made with Bob
