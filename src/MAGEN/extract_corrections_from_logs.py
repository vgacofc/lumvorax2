"""
Extraction Corrections depuis Logs Forensiques
==============================================

Lit TOUS les logs ligne par ligne et identifie corrections nécessaires
"""

import json
from pathlib import Path
from collections import defaultdict, Counter
from typing import List, Dict, Any


class CorrectionExtractor:
    """Extracteur de corrections depuis logs forensiques"""
    
    def __init__(self, log_file: str):
        self.log_file = Path(log_file)
        self.corrections = []
        self.stats = defaultdict(int)
        
    def analyze_all_lines(self):
        """Analyser TOUTES les lignes du log"""
        print(f"📂 Lecture: {self.log_file}")
        print(f"   Taille: {self.log_file.stat().st_size / 1024 / 1024:.2f} MB")
        
        with open(self.log_file, 'r') as f:
            data = json.load(f)
        
        print(f"✅ {len(data['events'])} événements chargés")
        
        # Analyser compteurs globaux
        self._analyze_counters(data['counters'])
        
        # Analyser TOUS les événements
        for i, event in enumerate(data['events'], 1):
            if i % 500 == 0:
                print(f"   Progression: {i}/{len(data['events'])} ({100*i/len(data['events']):.1f}%)")
            
            self._analyze_event(event)
        
        print(f"\n✅ Analyse complète: {len(data['events'])} événements")
        
    def _analyze_counters(self, counters: Dict[str, int]):
        """Analyser compteurs globaux"""
        print("\n📊 COMPTEURS GLOBAUX:")
        for key, value in counters.items():
            print(f"   {key}: {value}")
            self.stats[key] = value
            
            # Identifier corrections
            if key == 'pattern_detection' and value == 0:
                self.corrections.append({
                    'priority': 'P0',
                    'type': 'logging_missing',
                    'component': 'pattern_detectors',
                    'issue': 'pattern_detection = 0 (détecteurs pas tracés)',
                    'fix': 'Activer logging dans advanced_pattern_detectors.py'
                })
            
            if key == 'transformation' and value == 0:
                self.corrections.append({
                    'priority': 'P0',
                    'type': 'logging_missing',
                    'component': 'transformation_learning_engine',
                    'issue': 'transformation = 0 (TLE pas tracé)',
                    'fix': 'Activer logging dans transformation_learning_engine.py'
                })
            
            if key == 'error' and value == 0:
                self.corrections.append({
                    'priority': 'P1',
                    'type': 'logging_missing',
                    'component': 'error_handler',
                    'issue': 'error = 0 (erreurs pas loggées ou aucune erreur)',
                    'fix': 'Vérifier logging erreurs dans global_error_handler.py'
                })
    
    def _analyze_event(self, event: Dict[str, Any]):
        """Analyser un événement"""
        event_type = event['event_type']
        component = event['component']
        data = event.get('data', {})
        
        # Analyser validations
        if event_type == 'validation':
            self._analyze_validation(data)
        
        # Analyser PWFE
        if event_type == 'pwfe_execution':
            self._analyze_pwfe(data)
        
        # Analyser meta-arbiter
        if event_type == 'meta_arbiter_routing':
            self._analyze_routing(data)
    
    def _analyze_validation(self, data: Dict[str, Any]):
        """Analyser événement validation"""
        success = data.get('success', False)
        predicted_correct = data.get('predicted_correct', False)
        details = data.get('details', {})
        best_score = details.get('best_score', 0)
        adaptive_attempts = details.get('adaptive_attempts', 0)
        adaptive_successes = details.get('adaptive_successes', 0)
        
        # Correction 1: Incohérence success vs best_score
        if best_score >= 0.85 and not success:
            self.stats['validation_inconsistency'] += 1
            
            if self.stats['validation_inconsistency'] == 1:  # Log une seule fois
                self.corrections.append({
                    'priority': 'P0',
                    'type': 'validation_inconsistency',
                    'component': 'validator',
                    'issue': f'best_score={best_score:.2f} >= 0.85 mais success=False',
                    'fix': 'Unifier définition succès: utiliser Symbolic Verifier'
                })
        
        # Correction 2: Adaptive attempts sans successes
        if adaptive_attempts > 0 and adaptive_successes == 0:
            self.stats['adaptive_no_success'] += 1
            
            if self.stats['adaptive_no_success'] == 1:
                self.corrections.append({
                    'priority': 'P0',
                    'type': 'adaptive_ineffective',
                    'component': 'adaptive_strategy',
                    'issue': f'adaptive_attempts={adaptive_attempts} mais successes=0',
                    'fix': 'Améliorer stratégie adaptive ou activer Cross-Puzzle Memory'
                })
    
    def _analyze_pwfe(self, data: Dict[str, Any]):
        """Analyser événement PWFE"""
        success = data.get('success', False)
        total_worlds = data.get('total_worlds', 0)
        
        if not success and total_worlds > 0:
            self.stats['pwfe_failed'] += 1
        elif success:
            self.stats['pwfe_success'] += 1
    
    def _analyze_routing(self, data: Dict[str, Any]):
        """Analyser événement routing"""
        strategy = data.get('strategy', '')
        confidence = data.get('confidence', 0)
        
        self.stats[f'strategy_{strategy}'] += 1
        
        # Correction 3: Stratégies avec 0% succès
        # (sera détecté en croisant avec validations)
    
    def generate_report(self) -> str:
        """Générer rapport corrections"""
        lines = []
        lines.append("# RAPPORT CORRECTIONS IDENTIFIÉES DEPUIS LOGS")
        lines.append("")
        lines.append(f"**Fichier analysé**: `{self.log_file.name}`")
        lines.append(f"**Événements**: {self.stats.get('validation', 0) + self.stats.get('memory_snapshot', 0)}")
        lines.append("")
        
        # Statistiques
        lines.append("## 1. STATISTIQUES GLOBALES")
        lines.append("")
        lines.append("```")
        for key, value in sorted(self.stats.items()):
            if not key.startswith('strategy_'):
                lines.append(f"{key}: {value}")
        lines.append("```")
        lines.append("")
        
        # Stratégies
        lines.append("## 2. DISTRIBUTION STRATÉGIES")
        lines.append("")
        for key, value in sorted(self.stats.items()):
            if key.startswith('strategy_'):
                strategy = key.replace('strategy_', '')
                lines.append(f"- {strategy}: {value}")
        lines.append("")
        
        # PWFE Stats
        pwfe_total = self.stats.get('pwfe_success', 0) + self.stats.get('pwfe_failed', 0)
        if pwfe_total > 0:
            pwfe_rate = self.stats.get('pwfe_success', 0) / pwfe_total
            lines.append("## 3. PWFE PERFORMANCE")
            lines.append("")
            lines.append(f"- Succès: {self.stats.get('pwfe_success', 0)}/{pwfe_total} ({pwfe_rate:.1%})")
            lines.append("")
        
        # Corrections
        lines.append("## 4. CORRECTIONS IDENTIFIÉES")
        lines.append("")
        
        # Grouper par priorité
        p0_corrections = [c for c in self.corrections if c['priority'] == 'P0']
        p1_corrections = [c for c in self.corrections if c['priority'] == 'P1']
        
        lines.append(f"**Total**: {len(self.corrections)} corrections")
        lines.append(f"- P0 (Critique): {len(p0_corrections)}")
        lines.append(f"- P1 (Important): {len(p1_corrections)}")
        lines.append("")
        
        lines.append("### 4.1 Corrections P0 (CRITIQUE)")
        lines.append("")
        for i, corr in enumerate(p0_corrections, 1):
            lines.append(f"#### P0.{i} - {corr['component']}")
            lines.append(f"**Type**: {corr['type']}")
            lines.append(f"**Issue**: {corr['issue']}")
            lines.append(f"**Fix**: {corr['fix']}")
            lines.append("")
        
        lines.append("### 4.2 Corrections P1 (IMPORTANT)")
        lines.append("")
        for i, corr in enumerate(p1_corrections, 1):
            lines.append(f"#### P1.{i} - {corr['component']}")
            lines.append(f"**Type**: {corr['type']}")
            lines.append(f"**Issue**: {corr['issue']}")
            lines.append(f"**Fix**: {corr['fix']}")
            lines.append("")
        
        # Plan d'action
        lines.append("## 5. PLAN D'ACTION IMMÉDIAT")
        lines.append("")
        lines.append("### Phase 1: Corrections P0")
        for i, corr in enumerate(p0_corrections, 1):
            lines.append(f"{i}. {corr['fix']}")
        lines.append("")
        
        lines.append("### Phase 2: Corrections P1")
        for i, corr in enumerate(p1_corrections, 1):
            lines.append(f"{i}. {corr['fix']}")
        lines.append("")
        
        return '\n'.join(lines)


def main():
    """Point d'entrée"""
    log_file = "forensic/lumvorax_phase2_20260613_183010.json"
    output_file = "RAPPORT_CORRECTIONS_LOGS_FORENSIQUES.md"
    
    print("="*80)
    print("EXTRACTION CORRECTIONS DEPUIS LOGS FORENSIQUES")
    print("="*80)
    print("Objectif: Lire TOUTES les lignes et identifier corrections")
    print("="*80)
    
    extractor = CorrectionExtractor(log_file)
    extractor.analyze_all_lines()
    
    report = extractor.generate_report()
    
    with open(output_file, 'w') as f:
        f.write(report)
    
    print(f"\n✅ Rapport sauvegardé: {output_file}")
    print(f"   Corrections identifiées: {len(extractor.corrections)}")
    
    # Afficher résumé
    print("\n" + "="*80)
    print("RÉSUMÉ CORRECTIONS")
    print("="*80)
    for corr in extractor.corrections:
        print(f"{corr['priority']} - {corr['component']}: {corr['issue']}")
    
    print("\n" + "="*80)
    print("✅ ANALYSE COMPLÈTE")
    print("="*80)


if __name__ == "__main__":
    main()

# Made with Bob
