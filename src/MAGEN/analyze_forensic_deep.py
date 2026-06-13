"""
Analyse Forensique Profonde - Traçage Complet du Raisonnement
==============================================================

Objectif: Extraire TOUTES les actions et décisions du système
pour comprendre exactement comment il pense et pourquoi il échoue/réussit.
"""

import json
import sys
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Any, Tuple


class ForensicDeepAnalyzer:
    """Analyseur forensique profond bit-level"""
    
    def __init__(self, log_file: str):
        self.log_file = Path(log_file)
        self.data: Dict[str, Any] = {}
        self.events: List[Dict[str, Any]] = []
        self.puzzles: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
        self.reasoning_chains: Dict[str, Dict[str, Any]] = {}
        self.decision_graph: Dict[str, List[Any]] = defaultdict(list)
        
    def load(self):
        """Charger log forensique"""
        print(f"📂 Chargement: {self.log_file}")
        with open(self.log_file, 'r') as f:
            self.data = json.load(f)
        
        self.events = self.data['events']
        print(f"✅ {len(self.events)} événements chargés")
        
        # Grouper par puzzle
        for event in self.events:
            puzzle_id = event.get('data', {}).get('puzzle_id')
            if puzzle_id:
                self.puzzles[puzzle_id].append(event)
    
    def analyze_reasoning_chain(self, puzzle_id: str) -> Dict[str, Any]:
        """
        Analyser chaîne de raisonnement complète pour un puzzle.
        
        Retourne TOUTES les actions dans l'ordre chronologique.
        """
        events = self.puzzles[puzzle_id]
        
        chain = {
            'puzzle_id': puzzle_id,
            'total_events': len(events),
            'timeline': [],
            'decisions': [],
            'validations': [],
            'errors': [],
            'memory_evolution': [],
            'success': False,
            'total_time_ns': 0
        }
        
        for event in events:
            timestamp = event['timestamp_ns']
            elapsed = event['elapsed_ns']
            event_type = event['event_type']
            component = event['component']
            operation = event['operation']
            data = event.get('data', {})
            
            # Timeline complète
            chain['timeline'].append({
                'timestamp_ns': timestamp,
                'elapsed_ns': elapsed,
                'type': event_type,
                'component': component,
                'operation': operation,
                'data': data
            })
            
            # Décisions (routing, classification, etc.)
            if event_type in ['meta_arbiter_routing', 'classification']:
                chain['decisions'].append({
                    'timestamp_ns': timestamp,
                    'type': event_type,
                    'decision': data
                })
            
            # Validations
            if event_type == 'validation':
                chain['validations'].append({
                    'timestamp_ns': timestamp,
                    'result': data
                })
                if data.get('success'):
                    chain['success'] = True
            
            # Erreurs
            if event_type == 'error' or 'error' in data:
                chain['errors'].append({
                    'timestamp_ns': timestamp,
                    'error': data
                })
            
            # Évolution mémoire
            if event.get('memory_snapshot'):
                chain['memory_evolution'].append({
                    'timestamp_ns': timestamp,
                    'memory': event['memory_snapshot']
                })
        
        if events:
            chain['total_time_ns'] = events[-1]['elapsed_ns'] - events[0]['elapsed_ns']
        
        return chain
    
    def extract_decision_patterns(self) -> Dict[str, Any]:
        """Extraire patterns de décision"""
        patterns = {
            'strategy_distribution': Counter(),
            'success_by_strategy': defaultdict(lambda: {'success': 0, 'total': 0}),
            'avg_attempts_by_strategy': defaultdict(list),
            'classification_distribution': Counter(),
            'pwfe_success_rate': {'success': 0, 'total': 0}
        }
        
        for puzzle_id, events in self.puzzles.items():
            strategy = None
            classification = None
            success = False
            attempts = 0
            
            for event in events:
                data = event.get('data', {})
                
                # Stratégie choisie
                if event['event_type'] == 'meta_arbiter_routing':
                    strategy = data.get('strategy')
                    patterns['strategy_distribution'][strategy] += 1
                
                # Classification
                if event['event_type'] == 'classification':
                    classification = data.get('classification')
                    patterns['classification_distribution'][classification] += 1
                
                # PWFE
                if event['event_type'] == 'pwfe_execution':
                    patterns['pwfe_success_rate']['total'] += 1
                    if data.get('success'):
                        patterns['pwfe_success_rate']['success'] += 1
                
                # Validation finale
                if event['event_type'] == 'validation':
                    success = data.get('success', False)
                    attempts = data.get('adaptive_attempts', 0)
            
            # Statistiques par stratégie
            if strategy:
                patterns['success_by_strategy'][strategy]['total'] += 1
                if success:
                    patterns['success_by_strategy'][strategy]['success'] += 1
                patterns['avg_attempts_by_strategy'][strategy].append(attempts)
        
        return patterns
    
    def trace_failure_reasoning(self, puzzle_id: str) -> Dict[str, Any]:
        """
        Tracer raisonnement complet d'un échec.
        
        Répond à: Pourquoi ce puzzle a échoué?
        """
        chain = self.analyze_reasoning_chain(puzzle_id)
        
        if chain['success']:
            return {'error': 'Puzzle succeeded, not a failure'}
        
        trace = {
            'puzzle_id': puzzle_id,
            'failure_analysis': {},
            'decision_sequence': [],
            'hypothesis': []
        }
        
        # Séquence de décisions
        for decision in chain['decisions']:
            trace['decision_sequence'].append({
                'timestamp_ns': decision['timestamp_ns'],
                'type': decision['type'],
                'decision': decision['decision']
            })
        
        # Hypothèses sur cause d'échec
        if not chain['validations']:
            trace['hypothesis'].append("Aucune validation tentée - génération échouée")
        elif chain['validations']:
            last_val = chain['validations'][-1]
            trace['hypothesis'].append(f"Validation échouée: {last_val['result']}")
        
        if chain['errors']:
            trace['hypothesis'].append(f"{len(chain['errors'])} erreurs détectées")
        
        # Analyser tentatives adaptatives
        if chain['validations']:
            attempts = chain['validations'][-1]['result'].get('adaptive_attempts', 0)
            if attempts > 30:
                trace['hypothesis'].append(f"Trop de tentatives ({attempts}) - exploration inefficace")
            elif attempts == 0:
                trace['hypothesis'].append("Aucune tentative adaptive - stratégie rigide")
        
        trace['failure_analysis'] = {
            'total_time_ns': chain['total_time_ns'],
            'total_events': chain['total_events'],
            'decisions_count': len(chain['decisions']),
            'validations_count': len(chain['validations']),
            'errors_count': len(chain['errors'])
        }
        
        return trace
    
    def generate_report(self, output_file: str):
        """Générer rapport forensique complet"""
        print("\n" + "="*80)
        print("ANALYSE FORENSIQUE PROFONDE - RAPPORT COMPLET")
        print("="*80)
        
        report = []
        report.append("# RAPPORT ANALYSE FORENSIQUE PROFONDE V22")
        report.append(f"\n**Fichier**: `{self.log_file.name}`")
        report.append(f"**Session ID**: {self.data['session_id']}")
        report.append(f"**Durée totale**: {self.data['total_duration_ns'] / 1e9:.2f}s")
        report.append(f"**Événements**: {self.data['event_count']}")
        report.append(f"**Hash chain**: {'✅ VALIDE' if self.data['hash_chain_valid'] else '❌ INVALIDE'}")
        
        # Compteurs globaux
        report.append("\n## 1. COMPTEURS GLOBAUX")
        report.append("\n```")
        for key, value in self.data['counters'].items():
            report.append(f"{key}: {value}")
        report.append("```")
        
        # Patterns de décision
        report.append("\n## 2. PATTERNS DE DÉCISION")
        patterns = self.extract_decision_patterns()
        
        report.append("\n### 2.1 Distribution Stratégies")
        report.append("\n| Stratégie | Utilisation | Succès | Taux Succès |")
        report.append("|-----------|-------------|--------|-------------|")
        for strategy, count in patterns['strategy_distribution'].most_common():
            stats = patterns['success_by_strategy'][strategy]
            success_rate = stats['success'] / stats['total'] if stats['total'] > 0 else 0
            report.append(f"| {strategy} | {count} | {stats['success']}/{stats['total']} | {success_rate:.1%} |")
        
        report.append("\n### 2.2 Distribution Classifications")
        report.append("\n| Classification | Count |")
        report.append("|----------------|-------|")
        for classification, count in patterns['classification_distribution'].most_common():
            report.append(f"| {classification} | {count} |")
        
        report.append("\n### 2.3 PWFE Success Rate")
        pwfe = patterns['pwfe_success_rate']
        pwfe_rate = pwfe['success'] / pwfe['total'] if pwfe['total'] > 0 else 0
        report.append(f"\n- Total exécutions: {pwfe['total']}")
        report.append(f"- Succès: {pwfe['success']}")
        report.append(f"- Taux: {pwfe_rate:.1%}")
        
        # Analyse échecs
        report.append("\n## 3. ANALYSE ÉCHECS (Échantillon)")
        
        failed_puzzles = [pid for pid, events in self.puzzles.items() 
                         if not any(e.get('data', {}).get('success') 
                                  for e in events if e['event_type'] == 'validation')]
        
        report.append(f"\n**Total échecs**: {len(failed_puzzles)}/{len(self.puzzles)}")
        
        # Analyser 5 premiers échecs en détail
        report.append("\n### 3.1 Échecs Détaillés (5 premiers)")
        for puzzle_id in failed_puzzles[:5]:
            trace = self.trace_failure_reasoning(puzzle_id)
            report.append(f"\n#### Puzzle: {puzzle_id}")
            report.append(f"\n**Séquence décisions**:")
            for i, dec in enumerate(trace['decision_sequence'], 1):
                report.append(f"{i}. {dec['type']}: {dec['decision']}")
            report.append(f"\n**Hypothèses échec**:")
            for hyp in trace['hypothesis']:
                report.append(f"- {hyp}")
            report.append(f"\n**Analyse**:")
            for key, value in trace['failure_analysis'].items():
                report.append(f"- {key}: {value}")
        
        # Analyse succès
        report.append("\n## 4. ANALYSE SUCCÈS (Échantillon)")
        
        success_puzzles = [pid for pid, events in self.puzzles.items() 
                          if any(e.get('data', {}).get('success') 
                               for e in events if e['event_type'] == 'validation')]
        
        report.append(f"\n**Total succès**: {len(success_puzzles)}/{len(self.puzzles)}")
        
        # Analyser 5 premiers succès
        report.append("\n### 4.1 Succès Détaillés (5 premiers)")
        for puzzle_id in success_puzzles[:5]:
            chain = self.analyze_reasoning_chain(puzzle_id)
            report.append(f"\n#### Puzzle: {puzzle_id}")
            report.append(f"- Temps: {chain['total_time_ns'] / 1e6:.2f}ms")
            report.append(f"- Événements: {chain['total_events']}")
            report.append(f"- Décisions: {len(chain['decisions'])}")
            if chain['validations']:
                last_val = chain['validations'][-1]
                attempts = last_val['result'].get('adaptive_attempts', 0)
                report.append(f"- Tentatives adaptatives: {attempts}")
        
        # Recommandations
        report.append("\n## 5. RECOMMANDATIONS BASÉES SUR LOGS")
        report.append("\n### 5.1 Problèmes Identifiés")
        
        if self.data['counters']['pattern_detection'] == 0:
            report.append("- ❌ **pattern_detection = 0**: Détecteurs de patterns inactifs")
        
        if self.data['counters']['transformation'] == 0:
            report.append("- ❌ **transformation = 0**: TLE pas tracé dans logs")
        
        if self.data['counters']['error'] == 0:
            report.append("- ⚠️  **error = 0**: Soit aucune erreur, soit erreurs pas loggées")
        
        # Analyser adaptive_recoveries
        total_recoveries = 0
        for puzzle_id, events in self.puzzles.items():
            for event in events:
                if event['event_type'] == 'validation':
                    recoveries = event.get('data', {}).get('adaptive_recoveries', 0)
                    total_recoveries += recoveries
        
        report.append(f"- ❌ **adaptive_recoveries total = {total_recoveries}**: Pas d'apprentissage global")
        
        report.append("\n### 5.2 Actions Correctives")
        report.append("1. Activer logging pattern_detection dans détecteurs")
        report.append("2. Activer logging transformation dans TLE")
        report.append("3. Implémenter V23 Explanation-First pour adaptive_recoveries > 0")
        report.append("4. Ajouter logging décisions intermédiaires (features, scores)")
        report.append("5. Logger états internes modules (cache, mémoire, etc.)")
        
        # Sauvegarder rapport
        with open(output_file, 'w') as f:
            f.write('\n'.join(report))
        
        print(f"\n✅ Rapport sauvegardé: {output_file}")
        print(f"   Lignes: {len(report)}")


def main():
    """Point d'entrée"""
    log_file = "forensic/lumvorax_phase2_20260613_183010.json"
    output_file = "RAPPORT_FORENSIQUE_PROFOND_V22_LIGNE_PAR_LIGNE.md"
    
    print("="*80)
    print("ANALYSE FORENSIQUE PROFONDE - TRAÇAGE COMPLET")
    print("="*80)
    print(f"\nObjectif: Tracer TOUTES les actions et décisions")
    print(f"Fichier: {log_file}")
    print(f"Sortie: {output_file}")
    
    analyzer = ForensicDeepAnalyzer(log_file)
    analyzer.load()
    analyzer.generate_report(output_file)
    
    print("\n" + "="*80)
    print("✅ ANALYSE COMPLÈTE")
    print("="*80)


if __name__ == "__main__":
    main()

# Made with Bob
