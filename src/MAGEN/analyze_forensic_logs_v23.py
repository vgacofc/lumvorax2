#!/usr/bin/env python3
"""
Analyse COMPLÈTE des logs forensiques V23 - Ligne par ligne
Génère rapport MD avec toutes les anomalies et corrections nécessaires
"""

import json
from pathlib import Path
from collections import defaultdict, Counter
from datetime import datetime

def analyze_forensic_logs(log_file):
    """Analyse ligne par ligne tous les événements forensiques"""
    
    print(f"[ANALYSE] Lecture {log_file}...")
    
    # Compteurs
    events_by_type = Counter()
    events_by_component = Counter()
    errors = []
    warnings = []
    anomalies = []
    
    # Analyse temporelle
    timestamps = []
    event_durations = defaultdict(list)
    
    # Patterns spécifiques
    transformation_failures = []
    pattern_detection_empty = []
    validation_failures = []
    
    line_num = 0
    with open(log_file, 'r') as f:
        for line in f:
            line_num += 1
            try:
                event = json.loads(line.strip())
                
                # Compteurs de base
                event_type = event.get('event_type', 'unknown')
                component = event.get('component', 'unknown')
                events_by_type[event_type] += 1
                events_by_component[component] += 1
                
                # Timestamp
                ts = event.get('timestamp_ns', 0)
                timestamps.append(ts)
                
                # Analyse par type d'événement
                data = event.get('data', {})
                
                if event_type == 'transformation':
                    # Analyser succès/échecs transformations
                    success = data.get('success', False)
                    error = data.get('error', 0)
                    action = data.get('action', 'unknown')
                    
                    if not success or error > 0.5:
                        transformation_failures.append({
                            'line': line_num,
                            'action': action,
                            'success': success,
                            'error': error
                        })
                
                elif event_type == 'pattern_detection':
                    # Vérifier si détection vide
                    num_pairs = data.get('num_pairs', 0)
                    detector = data.get('detector', 'unknown')
                    
                    if num_pairs == 0:
                        pattern_detection_empty.append({
                            'line': line_num,
                            'detector': detector
                        })
                
                elif event_type == 'validation':
                    # Analyser échecs validation
                    success = data.get('success', False)
                    if not success:
                        validation_failures.append({
                            'line': line_num,
                            'data': data
                        })
                
                # Détecter anomalies génériques
                if 'error' in str(data).lower() or 'fail' in str(data).lower():
                    anomalies.append({
                        'line': line_num,
                        'event_type': event_type,
                        'component': component,
                        'data': data
                    })
                    
            except json.JSONDecodeError as e:
                errors.append(f"Ligne {line_num}: Erreur JSON - {e}")
            except Exception as e:
                errors.append(f"Ligne {line_num}: Erreur - {e}")
    
    print(f"[ANALYSE] {line_num} lignes analysées")
    
    return {
        'total_lines': line_num,
        'events_by_type': dict(events_by_type),
        'events_by_component': dict(events_by_component),
        'errors': errors,
        'warnings': warnings,
        'anomalies': anomalies,
        'transformation_failures': transformation_failures,
        'pattern_detection_empty': pattern_detection_empty,
        'validation_failures': validation_failures,
        'timestamps': timestamps
    }

def generate_markdown_report(analysis, output_file):
    """Génère rapport MD complet"""
    
    with open(output_file, 'w') as f:
        f.write("# RAPPORT ANALYSE FORENSIQUE V23 - 400 PUZZLES\n\n")
        f.write(f"**Date**: {datetime.now().isoformat()}\n\n")
        f.write(f"**Total événements**: {analysis['total_lines']:,}\n\n")
        
        f.write("## 1. STATISTIQUES GLOBALES\n\n")
        f.write("### Événements par type\n\n")
        f.write("| Type | Count |\n")
        f.write("|------|-------|\n")
        for event_type, count in sorted(analysis['events_by_type'].items(), key=lambda x: -x[1]):
            f.write(f"| {event_type} | {count:,} |\n")
        
        f.write("\n### Événements par composant\n\n")
        f.write("| Composant | Count |\n")
        f.write("|-----------|-------|\n")
        for comp, count in sorted(analysis['events_by_component'].items(), key=lambda x: -x[1]):
            f.write(f"| {comp} | {count:,} |\n")
        
        f.write("\n## 2. ANOMALIES DÉTECTÉES\n\n")
        
        f.write(f"### 2.1 Échecs Transformations ({len(analysis['transformation_failures'])})\n\n")
        if analysis['transformation_failures']:
            f.write("| Ligne | Action | Success | Error |\n")
            f.write("|-------|--------|---------|-------|\n")
            for fail in analysis['transformation_failures'][:50]:  # Top 50
                f.write(f"| {fail['line']} | {fail['action']} | {fail['success']} | {fail['error']:.3f} |\n")
            if len(analysis['transformation_failures']) > 50:
                f.write(f"\n*... et {len(analysis['transformation_failures'])-50} autres*\n")
        else:
            f.write("✅ Aucun échec détecté\n")
        
        f.write(f"\n### 2.2 Détections Pattern Vides ({len(analysis['pattern_detection_empty'])})\n\n")
        if analysis['pattern_detection_empty']:
            f.write("| Ligne | Détecteur |\n")
            f.write("|-------|----------|\n")
            for empty in analysis['pattern_detection_empty'][:50]:
                f.write(f"| {empty['line']} | {empty['detector']} |\n")
        else:
            f.write("✅ Aucune détection vide\n")
        
        f.write(f"\n### 2.3 Échecs Validation ({len(analysis['validation_failures'])})\n\n")
        if analysis['validation_failures']:
            f.write(f"Total: {len(analysis['validation_failures'])} échecs\n\n")
            f.write("*Détails dans section suivante*\n")
        else:
            f.write("✅ Aucun échec validation\n")
        
        f.write(f"\n### 2.4 Anomalies Génériques ({len(analysis['anomalies'])})\n\n")
        if analysis['anomalies']:
            f.write("| Ligne | Type | Composant |\n")
            f.write("|-------|------|----------|\n")
            for anom in analysis['anomalies'][:50]:
                f.write(f"| {anom['line']} | {anom['event_type']} | {anom['component']} |\n")
        else:
            f.write("✅ Aucune anomalie générique\n")
        
        f.write("\n## 3. ERREURS CRITIQUES\n\n")
        if analysis['errors']:
            for error in analysis['errors']:
                f.write(f"- {error}\n")
        else:
            f.write("✅ Aucune erreur critique\n")
        
        f.write("\n## 4. CORRECTIONS RECOMMANDÉES\n\n")
        f.write("### P1 - Priorité Haute\n\n")
        
        # Analyser patterns d'échecs
        if len(analysis['transformation_failures']) > 1000:
            f.write("- **P1.1**: Taux d'échec transformations élevé (>1000)\n")
            f.write("  - Analyser actions les plus échouées\n")
            f.write("  - Améliorer heuristiques de sélection\n\n")
        
        if len(analysis['validation_failures']) > 200:
            f.write("- **P1.2**: Taux d'échec validation élevé (>200)\n")
            f.write("  - Revoir critères de validation\n")
            f.write("  - Améliorer scoring\n\n")
        
        f.write("### P2 - Priorité Moyenne\n\n")
        f.write("- **P2.1**: Optimiser logging (9MB pour 400 puzzles)\n")
        f.write("- **P2.2**: Ajouter métriques de performance\n\n")
        
        f.write("\n## 5. MÉTRIQUES TEMPORELLES\n\n")
        if analysis['timestamps']:
            duration_ns = analysis['timestamps'][-1] - analysis['timestamps'][0]
            duration_s = duration_ns / 1e9
            f.write(f"- Durée totale: {duration_s:.2f}s\n")
            f.write(f"- Événements/seconde: {analysis['total_lines']/duration_s:.2f}\n")
        
        f.write("\n---\n")
        f.write("\n*Rapport généré automatiquement par analyze_forensic_logs_v23.py*\n")

def main():
    import sys
    
    if len(sys.argv) > 1:
        log_file = Path(sys.argv[1])
        # Générer nom de rapport basé sur le fichier de log
        log_name = log_file.stem
        output_file = Path(f"RAPPORT_FORENSIQUE_{log_name.upper()}.md")
    else:
        log_file = Path("logs/forensic/v23_corrected_20260613_210346.json")
        output_file = Path("RAPPORT_FORENSIQUE_V23_400_PUZZLES.md")
    
    print("="*80)
    print(f"ANALYSE FORENSIQUE V23 - {log_file.name}")
    print("="*80)
    
    analysis = analyze_forensic_logs(log_file)
    
    print(f"\n[RAPPORT] Génération {output_file}...")
    generate_markdown_report(analysis, output_file)
    
    print(f"[RAPPORT] ✅ Rapport généré: {output_file}")
    print("\n" + "="*80)
    print("RÉSUMÉ")
    print("="*80)
    print(f"Total événements: {analysis['total_lines']:,}")
    print(f"Anomalies: {len(analysis['anomalies'])}")
    print(f"Échecs transformation: {len(analysis['transformation_failures'])}")
    print(f"Échecs validation: {len(analysis['validation_failures'])}")
    print("="*80)

if __name__ == "__main__":
    main()

# Made with Bob
