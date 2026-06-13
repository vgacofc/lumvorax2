#!/usr/bin/env python3
"""
MAGEN - Analyse Patterns Échecs
================================

SESSION 56 - PHASE PATTERNS
Objectif: Identifier top 10-20 patterns manquants dans les 355 échecs
Mode: 100% LOCAL
Protocole: CLAUDE_PILOT + LUMVORAX

Analyse:
1. Charge résultats V13 (45 succès, 355 échecs)
2. Pour chaque échec, analyse structure grille
3. Identifie patterns récurrents non détectés
4. Génère recommandations patterns à implémenter
"""

import json
import numpy as np
from pathlib import Path
from collections import Counter, defaultdict
from typing import List, Dict, Tuple, Set
import sys

# Import modules MAGEN
from classification import PuzzleClassifier
from dsl.shape_detector import ShapeDetector
from dsl.rule_detector import RuleDetector


class FailedPatternsAnalyzer:
    """Analyse patterns dans puzzles échoués"""
    
    def __init__(self, results_file: str, dataset_file: str):
        self.results_file = Path(results_file)
        self.dataset_file = Path(dataset_file)
        
        # Statistiques
        self.failed_puzzles = []
        self.pattern_frequencies = Counter()
        self.transformation_types = Counter()
        self.grid_properties = defaultdict(list)
        
        # Patterns détectés
        self.detected_patterns = {
            'grid_tiling': [],
            'diagonal_symmetry': [],
            'color_gradient': [],
            'object_counting': [],
            'spatial_relationships': [],
            'conditional_rules': [],
            'pattern_completion': [],
            'boundary_detection': [],
            'topology_changes': [],
            'multi_step_transforms': []
        }
    
    def load_data(self):
        """Charge résultats et dataset"""
        print("📂 Chargement données...")
        
        # Charger résultats V13
        with open(self.results_file) as f:
            results = json.load(f)
        
        # Charger dataset
        with open(self.dataset_file) as f:
            dataset = json.load(f)
        
        # Extraire échecs
        for result in results['results']:
            if not result['success']:
                puzzle_id = result['puzzle_id']
                self.failed_puzzles.append({
                    'id': puzzle_id,
                    'data': dataset[puzzle_id],
                    'classification': result.get('classification', 'unknown'),
                    'time': result.get('time', 0.0)
                })
        
        print(f"✓ {len(self.failed_puzzles)} puzzles échoués chargés")
    
    def analyze_grid_properties(self, grid: np.ndarray) -> Dict:
        """Analyse propriétés d'une grille"""
        h, w = grid.shape
        unique_colors = len(np.unique(grid))
        
        # Symétries
        is_symmetric_h = np.array_equal(grid, np.flip(grid, axis=0))
        is_symmetric_v = np.array_equal(grid, np.flip(grid, axis=1))
        is_symmetric_diag = np.array_equal(grid, grid.T)
        
        # Patterns répétitifs
        has_repetition = self._detect_repetition(grid)
        
        # Gradients
        has_gradient = self._detect_gradient(grid)
        
        return {
            'shape': (h, w),
            'colors': unique_colors,
            'symmetric_h': is_symmetric_h,
            'symmetric_v': is_symmetric_v,
            'symmetric_diag': is_symmetric_diag,
            'has_repetition': has_repetition,
            'has_gradient': has_gradient
        }
    
    def _detect_repetition(self, grid: np.ndarray) -> bool:
        """Détecte patterns répétitifs (tiling)"""
        h, w = grid.shape
        
        # Tester tailles de tuiles communes
        for tile_h in [1, 2, 3, 4]:
            for tile_w in [1, 2, 3, 4]:
                if h % tile_h == 0 and w % tile_w == 0:
                    # Extraire première tuile
                    tile = grid[:tile_h, :tile_w]
                    
                    # Vérifier si répétée partout
                    is_tiled = True
                    for i in range(0, h, tile_h):
                        for j in range(0, w, tile_w):
                            if not np.array_equal(grid[i:i+tile_h, j:j+tile_w], tile):
                                is_tiled = False
                                break
                        if not is_tiled:
                            break
                    
                    if is_tiled and (tile_h > 1 or tile_w > 1):
                        return True
        
        return False
    
    def _detect_gradient(self, grid: np.ndarray) -> bool:
        """Détecte gradients de couleur"""
        h, w = grid.shape
        
        # Gradient horizontal
        for i in range(h):
            row = grid[i, :]
            if len(np.unique(row)) > w // 2:  # Beaucoup de couleurs différentes
                diffs = np.diff(row)
                if np.all(diffs >= 0) or np.all(diffs <= 0):  # Monotone
                    return True
        
        # Gradient vertical
        for j in range(w):
            col = grid[:, j]
            if len(np.unique(col)) > h // 2:
                diffs = np.diff(col)
                if np.all(diffs >= 0) or np.all(diffs <= 0):
                    return True
        
        return False
    
    def analyze_transformation(self, train_pairs: List[Tuple]) -> Dict:
        """Analyse type de transformation"""
        if not train_pairs:
            return {'type': 'unknown'}
        
        inp, out = train_pairs[0]
        
        # Changement de taille
        size_change = inp.shape != out.shape
        
        # Changement de couleurs
        colors_in = set(np.unique(inp))
        colors_out = set(np.unique(out))
        new_colors = colors_out - colors_in
        removed_colors = colors_in - colors_out
        
        # Nombre d'objets (approximation)
        from scipy.ndimage import label
        objects_in = label(inp > 0)[1]
        objects_out = label(out > 0)[1]
        
        return {
            'type': 'transformation',
            'size_change': size_change,
            'new_colors': len(new_colors),
            'removed_colors': len(removed_colors),
            'objects_change': objects_out - objects_in
        }
    
    def detect_specific_patterns(self, puzzle_data: Dict) -> List[str]:
        """Détecte patterns spécifiques dans un puzzle"""
        patterns = []
        
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle_data['train']]
        
        for inp, out in train_pairs:
            # Grid tiling
            if self._detect_repetition(inp) or self._detect_repetition(out):
                patterns.append('grid_tiling')
            
            # Diagonal symmetry
            props_in = self.analyze_grid_properties(inp)
            props_out = self.analyze_grid_properties(out)
            if props_in['symmetric_diag'] or props_out['symmetric_diag']:
                patterns.append('diagonal_symmetry')
            
            # Color gradient
            if props_in['has_gradient'] or props_out['has_gradient']:
                patterns.append('color_gradient')
            
            # Object counting (changement nombre objets)
            trans = self.analyze_transformation([(inp, out)])
            if abs(trans['objects_change']) > 0:
                patterns.append('object_counting')
            
            # Spatial relationships (taille change)
            if trans['size_change']:
                patterns.append('spatial_relationships')
            
            # Conditional rules (nouvelles couleurs)
            if trans['new_colors'] > 0:
                patterns.append('conditional_rules')
        
        return list(set(patterns))
    
    def analyze_all_failures(self):
        """Analyse tous les échecs"""
        print("\n🔍 Analyse patterns échecs...")
        
        for i, puzzle in enumerate(self.failed_puzzles):
            if (i + 1) % 50 == 0:
                print(f"   Progression: {i+1}/{len(self.failed_puzzles)}")
            
            # Détecter patterns
            patterns = self.detect_specific_patterns(puzzle['data'])
            
            # Compter fréquences
            for pattern in patterns:
                self.pattern_frequencies[pattern] += 1
                self.detected_patterns[pattern].append(puzzle['id'])
            
            # Analyser propriétés grilles
            train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                           for p in puzzle['data']['train']]
            
            for inp, out in train_pairs:
                props_in = self.analyze_grid_properties(inp)
                props_out = self.analyze_grid_properties(out)
                
                self.grid_properties['input_shapes'].append(props_in['shape'])
                self.grid_properties['output_shapes'].append(props_out['shape'])
                self.grid_properties['input_colors'].append(props_in['colors'])
                self.grid_properties['output_colors'].append(props_out['colors'])
        
        print("✓ Analyse complète")
    
    def generate_recommendations(self) -> List[Dict]:
        """Génère recommandations patterns à implémenter"""
        recommendations = []
        
        # Top patterns par fréquence
        top_patterns = self.pattern_frequencies.most_common(10)
        
        for pattern, count in top_patterns:
            percentage = (count / len(self.failed_puzzles)) * 100
            
            recommendations.append({
                'pattern': pattern,
                'frequency': count,
                'percentage': percentage,
                'puzzle_ids': self.detected_patterns[pattern][:5],  # Top 5 exemples
                'priority': 'HIGH' if percentage > 20 else 'MEDIUM' if percentage > 10 else 'LOW'
            })
        
        return recommendations
    
    def generate_report(self, output_file: str):
        """Génère rapport complet"""
        print("\n📊 Génération rapport...")
        
        recommendations = self.generate_recommendations()
        
        report = {
            'summary': {
                'total_failures': len(self.failed_puzzles),
                'patterns_detected': len(self.pattern_frequencies),
                'top_pattern': self.pattern_frequencies.most_common(1)[0] if self.pattern_frequencies else None
            },
            'recommendations': recommendations,
            'pattern_frequencies': dict(self.pattern_frequencies),
            'grid_statistics': {
                'avg_input_colors': np.mean(self.grid_properties['input_colors']),
                'avg_output_colors': np.mean(self.grid_properties['output_colors']),
                'common_input_shapes': Counter(self.grid_properties['input_shapes']).most_common(5),
                'common_output_shapes': Counter(self.grid_properties['output_shapes']).most_common(5)
            }
        }
        
        # Sauvegarder JSON
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"✓ Rapport sauvegardé: {output_file}")
        
        # Afficher résumé
        self._print_summary(report)
    
    def _print_summary(self, report: Dict):
        """Affiche résumé console"""
        print("\n" + "="*80)
        print("RÉSUMÉ ANALYSE PATTERNS ÉCHECS")
        print("="*80)
        
        print(f"\n📊 STATISTIQUES:")
        print(f"  - Échecs analysés: {report['summary']['total_failures']}")
        print(f"  - Patterns détectés: {report['summary']['patterns_detected']}")
        
        if report['summary']['top_pattern']:
            pattern, count = report['summary']['top_pattern']
            print(f"  - Pattern le plus fréquent: {pattern} ({count} occurrences)")
        
        print(f"\n🎯 TOP 10 PATTERNS À IMPLÉMENTER:")
        for i, rec in enumerate(report['recommendations'][:10], 1):
            print(f"  {i}. {rec['pattern']}")
            print(f"     Fréquence: {rec['frequency']}/{report['summary']['total_failures']} ({rec['percentage']:.1f}%)")
            print(f"     Priorité: {rec['priority']}")
            print(f"     Exemples: {', '.join(rec['puzzle_ids'][:3])}")
            print()
        
        print("="*80)


def main():
    """Point d'entrée principal"""
    print("="*80)
    print("MAGEN - ANALYSE PATTERNS ÉCHECS")
    print("="*80)
    
    # Fichiers
    results_file = "results_phase2_20260613_153425.json"
    dataset_file = "data/arc-agi_evaluation_challenges.json"
    output_file = "patterns_analysis_20260613.json"
    
    # Vérifier existence fichiers
    if not Path(results_file).exists():
        print(f"❌ Fichier résultats non trouvé: {results_file}")
        sys.exit(1)
    
    if not Path(dataset_file).exists():
        print(f"❌ Dataset non trouvé: {dataset_file}")
        sys.exit(1)
    
    # Analyser
    analyzer = FailedPatternsAnalyzer(results_file, dataset_file)
    analyzer.load_data()
    analyzer.analyze_all_failures()
    analyzer.generate_report(output_file)
    
    print("\n✅ ANALYSE TERMINÉE")


if __name__ == "__main__":
    main()

# Made with Bob
