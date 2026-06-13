#!/usr/bin/env python3
"""
Analyse PROFONDE ligne par ligne des logs forensiques V24
Lecture COMPLÈTE sans sauter une seule ligne
Identification corrections nécessaires basées sur données RÉELLES
"""

import json
from pathlib import Path
from collections import defaultdict, Counter
from datetime import datetime
import numpy as np

def analyze_deep_forensic(log_file):
    """Analyse PROFONDE ligne par ligne - AUCUNE ligne sautée"""
    
    print(f"[ANALYSE PROFONDE] Lecture {log_file}...")
    print("[ANALYSE PROFONDE] Mode: LIGNE PAR LIGNE, AUCUNE EXCEPTION")
    
    # Structures d'analyse
    all_events = []
    events_by_type = Counter()
    events_by_component = Counter()
    
    # Analyse transformations
    transformations = []
    transformation_success = []
    transformation_errors = []
    transformation_timings = []
    transformation_shapes = []
    transformation_chains = []
    
    # Analyse patterns
    pattern_detections = []
    
    # Analyse validations
    validations = []
    validation_failures = []
    
    # Analyse puzzles
    puzzles = {}
    current_puzzle = None
    
    # Analyse temporelle nanoseconde
    timestamps_ns = []
    event_sequences = []
    
    # Anomalies détectées
    anomalies = []
    corrections_needed = []
    
    line_num = 0
    with open(log_file, 'r') as f:
        for line in f:
            line_num += 1
            
            if line_num % 5000 == 0:
                print(f"  [PROGRESS] {line_num} lignes analysées...")
            
            try:
                event = json.loads(line.strip())
                all_events.append(event)
                
                # Compteurs de base
                event_type = event.get('event_type', 'unknown')
                component = event.get('component', None)
                events_by_type[event_type] += 1
                events_by_component[component] += 1
                
                # Timestamp nanoseconde
                ts_ns = event.get('timestamp_ns', 0)
                timestamps_ns.append(ts_ns)
                
                # Analyse par type
                if event_type == 'puzzle_start':
                    puzzle_id = event['data'].get('puzzle_id')
                    current_puzzle = puzzle_id
                    puzzles[puzzle_id] = {
                        'id': puzzle_id,
                        'start_ts': ts_ns,
                        'events': [],
                        'transformations': [],
                        'success': None
                    }
                
                if current_puzzle and current_puzzle in puzzles:
                    puzzles[current_puzzle]['events'].append(event)
                
                if event_type == 'transformation':
                    operation = event.get('operation')
                    data = event.get('data', {})
                    
                    if operation == 'apply_transformation':
                        transformations.append(event)
                        
                        # Analyse succès/échec
                        success = data.get('success', False)
                        error = data.get('error', 1.0)
                        transformation_success.append(success)
                        transformation_errors.append(error)
                        
                        # Analyse timing
                        exec_time = data.get('execution_time', 0)
                        transformation_timings.append(exec_time)
                        
                        # Analyse shapes
                        input_shape = data.get('input_shape')
                        output_shape = data.get('output_shape')
                        target_shape = data.get('target_shape')
                        shape_match = data.get('shape_match')
                        
                        if input_shape and output_shape:
                            transformation_shapes.append({
                                'input': input_shape,
                                'output': output_shape,
                                'target': target_shape,
                                'match': shape_match
                            })
                        
                        # Analyse causal_chain
                        causal_chain = data.get('causal_chain', [])
                        if causal_chain:
                            transformation_chains.append(len(causal_chain))
                        
                        # Détection anomalies
                        action = data.get('action', 'unknown')
                        
                        # ANOMALIE 1: action="unknown"
                        if action == 'unknown':
                            anomalies.append({
                                'line': line_num,
                                'type': 'unknown_action',
                                'event': event,
                                'severity': 'medium'
                            })
                        
                        # ANOMALIE 2: error=0.0 (impossible)
                        if error == 0.0 and not success:
                            anomalies.append({
                                'line': line_num,
                                'type': 'zero_error_failure',
                                'event': event,
                                'severity': 'high'
                            })
                        
                        # ANOMALIE 3: shape_match=False mais success=True
                        if shape_match == False and success == True:
                            anomalies.append({
                                'line': line_num,
                                'type': 'shape_mismatch_success',
                                'event': event,
                                'severity': 'critical'
                            })
                        
                        # ANOMALIE 4: causal_consistency=False
                        causal_consistency = data.get('causal_consistency', True)
                        if not causal_consistency:
                            anomalies.append({
                                'line': line_num,
                                'type': 'causal_inconsistency',
                                'event': event,
                                'severity': 'high'
                            })
                        
                        if current_puzzle and current_puzzle in puzzles:
                            puzzles[current_puzzle]['transformations'].append(event)
                
                elif event_type == 'pattern_detection':
                    pattern_detections.append(event)
                
                elif event_type == 'validation':
                    validations.append(event)
                    success = event['data'].get('success', False)
                    if not success:
                        validation_failures.append(event)
                    
                    if current_puzzle and current_puzzle in puzzles:
                        puzzles[current_puzzle]['success'] = success
                
                # ANOMALIE 5: component=None
                if component is None:
                    anomalies.append({
                        'line': line_num,
                        'type': 'missing_component',
                        'event': event,
                        'severity': 'low'
                    })
                
            except json.JSONDecodeError as e:
                anomalies.append({
                    'line': line_num,
                    'type': 'json_decode_error',
                    'error': str(e),
                    'severity': 'critical'
                })
            except Exception as e:
                anomalies.append({
                    'line': line_num,
                    'type': 'unexpected_error',
                    'error': str(e),
                    'severity': 'critical'
                })
    
    print(f"[ANALYSE PROFONDE] ✅ {line_num} lignes analysées")
    
    # Analyse statistique transformations
    trans_stats = {}
    if transformations:
        trans_stats = {
            'total': len(transformations),
            'success_count': sum(transformation_success),
            'success_rate': sum(transformation_success) / len(transformations),
            'avg_error': np.mean(transformation_errors),
            'median_error': np.median(transformation_errors),
            'avg_timing': np.mean(transformation_timings),
            'max_timing': max(transformation_timings),
            'min_timing': min(transformation_timings),
            'chains_used': len([c for c in transformation_chains if c > 0]),
            'avg_chain_length': np.mean(transformation_chains) if transformation_chains else 0
        }
    
    # Analyse puzzles succès vs échec
    success_puzzles = [p for p in puzzles.values() if p['success'] == True]
    failed_puzzles = [p for p in puzzles.values() if p['success'] == False]
    
    # Comparaison succès vs échec
    comparison = {}
    if success_puzzles and failed_puzzles:
        success_trans = [len(p['transformations']) for p in success_puzzles]
        failed_trans = [len(p['transformations']) for p in failed_puzzles]
        
        comparison = {
            'success_avg_transformations': np.mean(success_trans),
            'failed_avg_transformations': np.mean(failed_trans),
            'success_median_transformations': np.median(success_trans),
            'failed_median_transformations': np.median(failed_trans)
        }
    
    # Identification corrections nécessaires
    corrections_needed = identify_corrections(anomalies, trans_stats, comparison)
    
    return {
        'total_lines': line_num,
        'total_events': len(all_events),
        'events_by_type': dict(events_by_type),
        'events_by_component': dict(events_by_component),
        'transformations': trans_stats,
        'pattern_detections': len(pattern_detections),
        'validations': len(validations),
        'validation_failures': len(validation_failures),
        'puzzles': {
            'total': len(puzzles),
            'success': len(success_puzzles),
            'failed': len(failed_puzzles),
            'success_rate': len(success_puzzles) / len(puzzles) if puzzles else 0
        },
        'comparison': comparison,
        'anomalies': anomalies,
        'corrections_needed': corrections_needed,
        'timestamps': {
            'first_ns': min(timestamps_ns) if timestamps_ns else 0,
            'last_ns': max(timestamps_ns) if timestamps_ns else 0,
            'duration_ns': max(timestamps_ns) - min(timestamps_ns) if timestamps_ns else 0
        }
    }

def identify_corrections(anomalies, trans_stats, comparison):
    """Identifie corrections nécessaires basées sur anomalies RÉELLES"""
    
    corrections = []
    
    # Analyse anomalies par type
    anomaly_types = Counter([a['type'] for a in anomalies])
    
    # CORRECTION 1: Actions "unknown"
    unknown_count = anomaly_types.get('unknown_action', 0)
    if unknown_count > 0:
        corrections.append({
            'id': 'C1',
            'priority': 'MEDIUM',
            'type': 'unknown_action',
            'count': unknown_count,
            'description': f'{unknown_count} transformations avec action="unknown"',
            'fix': 'Améliorer détection action dans TransformationLearningEngine',
            'file': 'transformation_learning_engine.py',
            'impact': 'Réduction bruit forensique + meilleure traçabilité'
        })
    
    # CORRECTION 2: Erreurs zero avec échec
    zero_error_count = anomaly_types.get('zero_error_failure', 0)
    if zero_error_count > 0:
        corrections.append({
            'id': 'C2',
            'priority': 'HIGH',
            'type': 'zero_error_failure',
            'count': zero_error_count,
            'description': f'{zero_error_count} échecs avec error=0.0 (impossible)',
            'fix': 'Corriger calcul erreur - error=0.0 devrait être success=True',
            'file': 'transformation_learning_engine.py',
            'impact': 'Cohérence logique + métriques correctes'
        })
    
    # CORRECTION 3: Shape mismatch avec succès
    shape_mismatch_count = anomaly_types.get('shape_mismatch_success', 0)
    if shape_mismatch_count > 0:
        corrections.append({
            'id': 'C3',
            'priority': 'CRITICAL',
            'type': 'shape_mismatch_success',
            'count': shape_mismatch_count,
            'description': f'{shape_mismatch_count} succès avec shape_match=False (incohérent)',
            'fix': 'Revoir logique validation - shape_match devrait bloquer success',
            'file': 'transformation_learning_engine.py',
            'impact': 'Validation correcte + faux positifs éliminés'
        })
    
    # CORRECTION 4: Incohérence causale
    causal_inconsistency_count = anomaly_types.get('causal_inconsistency', 0)
    if causal_inconsistency_count > 0:
        corrections.append({
            'id': 'C4',
            'priority': 'HIGH',
            'type': 'causal_inconsistency',
            'count': causal_inconsistency_count,
            'description': f'{causal_inconsistency_count} transformations avec causal_consistency=False',
            'fix': 'Analyser postconditions échouées + améliorer contraintes causales',
            'file': 'transformation_learning_engine.py',
            'impact': 'Cohérence causale + détection violations'
        })
    
    # CORRECTION 5: Composants manquants
    missing_component_count = anomaly_types.get('missing_component', 0)
    if missing_component_count > 0:
        corrections.append({
            'id': 'C5',
            'priority': 'LOW',
            'type': 'missing_component',
            'count': missing_component_count,
            'description': f'{missing_component_count} événements avec component=None',
            'fix': 'Ajouter composant à tous les types d\'événements',
            'file': 'test_phase2_400_puzzles.py',
            'impact': 'Traçabilité complète + indexation améliorée'
        })
    
    # CORRECTION 6: Taux succès transformations faible
    if trans_stats and trans_stats.get('success_rate', 0) < 0.3:
        corrections.append({
            'id': 'C6',
            'priority': 'HIGH',
            'type': 'low_success_rate',
            'count': trans_stats['total'],
            'description': f'Taux succès transformations: {trans_stats["success_rate"]:.1%} (< 30%)',
            'fix': 'Améliorer sélection transformations + pruning intelligent',
            'file': 'transformation_learning_engine.py',
            'impact': 'Efficacité +30% + réduction exploration inutile'
        })
    
    # CORRECTION 7: Différence succès vs échec
    if comparison:
        diff = comparison.get('failed_avg_transformations', 0) - comparison.get('success_avg_transformations', 0)
        if diff > 10:
            corrections.append({
                'id': 'C7',
                'priority': 'MEDIUM',
                'type': 'exploration_inefficient',
                'count': int(diff),
                'description': f'Puzzles échoués explorent {diff:.1f} transformations de plus que succès',
                'fix': 'Early stopping + détection impasses + pruning agressif',
                'file': 'meta_arbiter.py',
                'impact': 'Vitesse +20% + focus sur solutions prometteuses'
            })
    
    return corrections

def generate_deep_report(analysis, output_file):
    """Génère rapport MD avec analyse PROFONDE"""
    
    with open(output_file, 'w') as f:
        f.write("# RAPPORT ANALYSE FORENSIQUE PROFONDE V24\n\n")
        f.write(f"**Date**: {datetime.now().isoformat()}\n\n")
        f.write(f"**Total lignes analysées**: {analysis['total_lines']:,}\n")
        f.write(f"**Total événements**: {analysis['total_events']:,}\n\n")
        
        f.write("## 1. STATISTIQUES GLOBALES\n\n")
        
        f.write("### Événements par type\n\n")
        f.write("| Type | Count | % |\n")
        f.write("|------|-------|---|\n")
        for event_type, count in sorted(analysis['events_by_type'].items(), key=lambda x: x[1], reverse=True):
            pct = count / analysis['total_events'] * 100
            f.write(f"| {event_type} | {count:,} | {pct:.1f}% |\n")
        
        f.write("\n### Événements par composant\n\n")
        f.write("| Composant | Count | % |\n")
        f.write("|-----------|-------|---|\n")
        for component, count in sorted(analysis['events_by_component'].items(), key=lambda x: x[1], reverse=True):
            pct = count / analysis['total_events'] * 100
            comp_name = component if component else "None"
            f.write(f"| {comp_name} | {count:,} | {pct:.1f}% |\n")
        
        f.write("\n## 2. ANALYSE TRANSFORMATIONS\n\n")
        trans = analysis['transformations']
        if trans:
            f.write(f"- **Total**: {trans['total']:,}\n")
            f.write(f"- **Succès**: {trans['success_count']:,} ({trans['success_rate']:.1%})\n")
            f.write(f"- **Erreur moyenne**: {trans['avg_error']:.4f}\n")
            f.write(f"- **Erreur médiane**: {trans['median_error']:.4f}\n")
            f.write(f"- **Timing moyen**: {trans['avg_timing']*1000:.2f} ms\n")
            f.write(f"- **Timing max**: {trans['max_timing']*1000:.2f} ms\n")
            f.write(f"- **Timing min**: {trans['min_timing']*1000:.4f} ms\n")
            f.write(f"- **Chaînes causales utilisées**: {trans['chains_used']}\n")
            f.write(f"- **Longueur moyenne chaîne**: {trans['avg_chain_length']:.2f}\n")
        
        f.write("\n## 3. ANALYSE PUZZLES\n\n")
        puzzles = analysis['puzzles']
        f.write(f"- **Total**: {puzzles['total']}\n")
        f.write(f"- **Succès**: {puzzles['success']} ({puzzles['success_rate']:.1%})\n")
        f.write(f"- **Échecs**: {puzzles['failed']} ({1-puzzles['success_rate']:.1%})\n")
        
        if analysis['comparison']:
            comp = analysis['comparison']
            f.write("\n### Comparaison Succès vs Échec\n\n")
            f.write(f"- **Succès - Transformations moyennes**: {comp['success_avg_transformations']:.1f}\n")
            f.write(f"- **Échecs - Transformations moyennes**: {comp['failed_avg_transformations']:.1f}\n")
            f.write(f"- **Différence**: {comp['failed_avg_transformations'] - comp['success_avg_transformations']:.1f}\n")
        
        f.write("\n## 4. ANOMALIES DÉTECTÉES\n\n")
        f.write(f"**Total anomalies**: {len(analysis['anomalies']):,}\n\n")
        
        anomaly_types = Counter([a['type'] for a in analysis['anomalies']])
        f.write("### Par type\n\n")
        f.write("| Type | Count | Sévérité |\n")
        f.write("|------|-------|----------|\n")
        for atype, count in sorted(anomaly_types.items(), key=lambda x: x[1], reverse=True):
            severity = analysis['anomalies'][0]['severity'] if analysis['anomalies'] else 'unknown'
            f.write(f"| {atype} | {count:,} | {severity} |\n")
        
        f.write("\n## 5. CORRECTIONS NÉCESSAIRES\n\n")
        f.write(f"**Total corrections identifiées**: {len(analysis['corrections_needed'])}\n\n")
        
        for correction in sorted(analysis['corrections_needed'], key=lambda x: {'CRITICAL': 0, 'HIGH': 1, 'MEDIUM': 2, 'LOW': 3}[x['priority']]):
            f.write(f"### {correction['id']} - {correction['type']} [{correction['priority']}]\n\n")
            f.write(f"**Description**: {correction['description']}\n\n")
            f.write(f"**Fix**: {correction['fix']}\n\n")
            f.write(f"**Fichier**: `{correction['file']}`\n\n")
            f.write(f"**Impact**: {correction['impact']}\n\n")
            f.write("---\n\n")
        
        f.write("\n## 6. MÉTRIQUES TEMPORELLES\n\n")
        ts = analysis['timestamps']
        duration_s = ts['duration_ns'] / 1e9
        f.write(f"- **Durée totale**: {duration_s:.2f}s\n")
        f.write(f"- **Événements/seconde**: {analysis['total_events']/duration_s:.2f}\n")
        f.write(f"- **Premier timestamp**: {ts['first_ns']}\n")
        f.write(f"- **Dernier timestamp**: {ts['last_ns']}\n")

def main():
    log_file = Path("logs/forensic/v23_corrected_20260613_213653.json")
    output_file = Path("RAPPORT_FORENSIQUE_PROFOND_V24.md")
    
    print("="*80)
    print("ANALYSE FORENSIQUE PROFONDE V24 - LIGNE PAR LIGNE")
    print("="*80)
    
    analysis = analyze_deep_forensic(log_file)
    
    print(f"\n[RAPPORT] Génération {output_file}...")
    generate_deep_report(analysis, output_file)
    
    print(f"[RAPPORT] ✅ Rapport généré: {output_file}")
    print("\n" + "="*80)
    print("RÉSUMÉ")
    print("="*80)
    print(f"Total lignes: {analysis['total_lines']:,}")
    print(f"Total événements: {analysis['total_events']:,}")
    print(f"Anomalies: {len(analysis['anomalies']):,}")
    print(f"Corrections nécessaires: {len(analysis['corrections_needed'])}")
    print("="*80)

if __name__ == "__main__":
    main()

# Made with Bob
