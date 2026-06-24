#!/usr/bin/env python3
"""
Analyse Forensic Logs V36 - Validation Critique
================================================

Analyse systématique des logs forensic pour valider/invalider
les découvertes de l'analyse utilisateur :

1. Boucle 2×2 pathologique ([0,0] ↔ [0,1])
2. Cohérence figée (stagnation informationnelle)
3. Une seule région découverte
4. Métacognition consultée mais ignorée
5. Zones prometteuses passives

Auteur: Bob (MAGEN V36 Forensic Analysis)
Date: 2026-06-15
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple, Set
from collections import Counter, defaultdict
from dataclasses import dataclass, field


@dataclass
class PuzzleAnalysis:
    """Analyse complète d'un puzzle."""
    puzzle_id: str
    total_steps: int
    positions_visited: List[Tuple[int, int]] = field(default_factory=list)
    coherence_evolution: List[float] = field(default_factory=list)
    loops_detected: int = 0
    regions_discovered: int = 0
    final_coherence: float = 0.0
    duration_s: float = 0.0
    
    # Patterns critiques
    position_oscillations: Dict[str, int] = field(default_factory=dict)
    coherence_stable_steps: int = 0
    unique_positions: int = 0
    loop_positions: Set[Tuple[int, int]] = field(default_factory=set)


class ForensicAnalyzer:
    """Analyseur forensic pour logs V36."""
    
    def __init__(self, log_path: Path):
        self.log_path = log_path
        self.events: List[Dict] = []
        self.puzzles: Dict[str, PuzzleAnalysis] = {}
        self.current_puzzle: str | None = None
        
    def load_logs(self):
        """Charge tous les événements forensic."""
        print(f"📂 Chargement logs: {self.log_path}")
        
        with open(self.log_path, 'r') as f:
            for line in f:
                if line.strip():
                    event = json.loads(line)
                    self.events.append(event)
        
        print(f"✅ {len(self.events)} événements chargés")
    
    def analyze_all_puzzles(self):
        """Analyse tous les puzzles."""
        print("\n🔍 ANALYSE PUZZLES")
        print("=" * 80)
        
        for event in self.events:
            event_type = event.get('event')
            
            if event_type == 'puzzle_started':
                self._start_puzzle(event)
            elif event_type == 'solver_step':
                self._process_step(event)
            elif event_type == 'loop_detected':
                self._process_loop(event)
            elif event_type == 'puzzle_completed':
                self._complete_puzzle(event)
        
        print(f"\n✅ {len(self.puzzles)} puzzles analysés")
    
    def _start_puzzle(self, event: Dict):
        """Démarre l'analyse d'un nouveau puzzle."""
        puzzle_id = event['data']['puzzle_id']
        self.current_puzzle = puzzle_id
        self.puzzles[puzzle_id] = PuzzleAnalysis(
            puzzle_id=puzzle_id,
            total_steps=0
        )
    
    def _process_step(self, event: Dict):
        """Traite un step de résolution."""
        if not self.current_puzzle:
            return
        
        puzzle = self.puzzles[self.current_puzzle]
        data = event['data']
        
        # Position
        pos = tuple(data['position'])
        puzzle.positions_visited.append(pos)
        
        # Cohérence
        coherence = data['coherence']
        puzzle.coherence_evolution.append(coherence)
        
        # Détection cohérence stable
        if len(puzzle.coherence_evolution) >= 2:
            prev = puzzle.coherence_evolution[-2]
            if abs(coherence - prev) < 0.001:  # Stable si diff < 0.1%
                puzzle.coherence_stable_steps += 1
    
    def _process_loop(self, event: Dict):
        """Traite une détection de loop."""
        if not self.current_puzzle:
            return
        
        puzzle = self.puzzles[self.current_puzzle]
        data = event['data']
        
        pos = tuple(data['position'])
        puzzle.loop_positions.add(pos)
    
    def _complete_puzzle(self, event: Dict):
        """Finalise l'analyse du puzzle."""
        if not self.current_puzzle:
            return
        
        puzzle = self.puzzles[self.current_puzzle]
        data = event['data']
        
        puzzle.total_steps = data['steps']
        puzzle.loops_detected = data['loops']
        puzzle.regions_discovered = data['regions']
        puzzle.final_coherence = data['coherence']
        puzzle.duration_s = data['duration_s']
        
        # Calcul patterns
        puzzle.unique_positions = len(set(puzzle.positions_visited))
        
        # Oscillations (paires de positions répétées)
        for i in range(len(puzzle.positions_visited) - 1):
            pos1 = puzzle.positions_visited[i]
            pos2 = puzzle.positions_visited[i + 1]
            pair = f"{pos1}↔{pos2}"
            puzzle.position_oscillations[pair] = puzzle.position_oscillations.get(pair, 0) + 1
        
        self.current_puzzle = None
    
    def validate_user_analysis(self):
        """Valide/invalide les découvertes de l'analyse utilisateur."""
        print("\n" + "=" * 80)
        print("🎯 VALIDATION ANALYSE UTILISATEUR")
        print("=" * 80)
        
        # 1. Boucle 2×2 pathologique
        print("\n1️⃣  BOUCLE 2×2 PATHOLOGIQUE")
        print("-" * 80)
        
        total_oscillations = 0
        max_oscillation = 0
        max_puzzle = None
        
        for puzzle_id, puzzle in self.puzzles.items():
            if puzzle.position_oscillations:
                top_osc = max(puzzle.position_oscillations.values())
                if top_osc > max_oscillation:
                    max_oscillation = top_osc
                    max_puzzle = puzzle_id
                total_oscillations += sum(puzzle.position_oscillations.values())
        
        print(f"Total oscillations détectées: {total_oscillations}")
        print(f"Oscillation maximale: {max_oscillation} (puzzle: {max_puzzle})")
        
        if max_puzzle:
            puzzle = self.puzzles[max_puzzle]
            print(f"\nTop 3 oscillations dans {max_puzzle}:")
            sorted_osc = sorted(puzzle.position_oscillations.items(), 
                              key=lambda x: x[1], reverse=True)[:3]
            for pair, count in sorted_osc:
                print(f"  {pair}: {count} fois")
        
        verdict_1 = "✅ VALIDÉ" if max_oscillation > 10 else "❌ INVALIDÉ"
        print(f"\n{verdict_1}: Boucle 2×2 pathologique détectée")
        
        # 2. Cohérence figée
        print("\n2️⃣  COHÉRENCE FIGÉE (STAGNATION)")
        print("-" * 80)
        
        total_stable = 0
        total_steps = 0
        
        for puzzle in self.puzzles.values():
            total_stable += puzzle.coherence_stable_steps
            total_steps += puzzle.total_steps
        
        stable_ratio = (total_stable / total_steps * 100) if total_steps > 0 else 0
        
        print(f"Steps avec cohérence stable: {total_stable}/{total_steps} ({stable_ratio:.1f}%)")
        
        # Analyse évolution cohérence
        print("\nÉvolution cohérence par puzzle:")
        for puzzle_id, puzzle in list(self.puzzles.items())[:3]:  # 3 premiers
            if puzzle.coherence_evolution:
                initial = puzzle.coherence_evolution[0]
                final = puzzle.final_coherence
                delta = final - initial
                print(f"  {puzzle_id}: {initial:.4f} → {final:.4f} (Δ={delta:+.4f})")
        
        verdict_2 = "✅ VALIDÉ" if stable_ratio > 50 else "⚠️  PARTIEL"
        print(f"\n{verdict_2}: Cohérence figée détectée ({stable_ratio:.1f}% stable)")
        
        # 3. Une seule région découverte
        print("\n3️⃣  UNE SEULE RÉGION DÉCOUVERTE")
        print("-" * 80)
        
        regions_stats = [p.regions_discovered for p in self.puzzles.values()]
        avg_regions = sum(regions_stats) / len(regions_stats) if regions_stats else 0
        
        print(f"Régions moyennes: {avg_regions:.2f}")
        print(f"Distribution régions: {Counter(regions_stats)}")
        
        verdict_3 = "✅ VALIDÉ" if avg_regions < 2.0 else "❌ INVALIDÉ"
        print(f"\n{verdict_3}: Segmentation spatiale insuffisante")
        
        # 4. Loops massifs
        print("\n4️⃣  LOOPS MASSIFS (83%)")
        print("-" * 80)
        
        total_loops = sum(p.loops_detected for p in self.puzzles.values())
        total_steps_all = sum(p.total_steps for p in self.puzzles.values())
        loop_ratio = (total_loops / total_steps_all * 100) if total_steps_all > 0 else 0
        
        print(f"Loops totaux: {total_loops}/{total_steps_all} ({loop_ratio:.1f}%)")
        
        verdict_4 = "✅ VALIDÉ" if loop_ratio > 70 else "❌ INVALIDÉ"
        print(f"\n{verdict_4}: Exploration pathologique confirmée")
        
        # 5. Positions uniques vs visitées
        print("\n5️⃣  DIVERSITÉ EXPLORATION")
        print("-" * 80)
        
        for puzzle_id, puzzle in list(self.puzzles.items())[:5]:
            unique_ratio = (puzzle.unique_positions / puzzle.total_steps * 100) if puzzle.total_steps > 0 else 0
            print(f"{puzzle_id}: {puzzle.unique_positions}/{puzzle.total_steps} positions uniques ({unique_ratio:.1f}%)")
        
        avg_unique_ratio = sum(
            (p.unique_positions / p.total_steps * 100) if p.total_steps > 0 else 0
            for p in self.puzzles.values()
        ) / len(self.puzzles)
        
        verdict_5 = "✅ VALIDÉ" if avg_unique_ratio < 30 else "❌ INVALIDÉ"
        print(f"\n{verdict_5}: Exploration répétitive ({avg_unique_ratio:.1f}% unique)")
    
    def generate_report(self) -> str:
        """Génère rapport complet."""
        lines = []
        lines.append("=" * 80)
        lines.append("RAPPORT ANALYSE FORENSIC V36 - VALIDATION CRITIQUE")
        lines.append("=" * 80)
        lines.append("")
        lines.append(f"📊 Événements analysés: {len(self.events)}")
        lines.append(f"🧩 Puzzles complétés: {len(self.puzzles)}")
        lines.append("")
        
        # Statistiques globales
        lines.append("📈 STATISTIQUES GLOBALES")
        lines.append("-" * 80)
        
        total_steps = sum(p.total_steps for p in self.puzzles.values())
        total_loops = sum(p.loops_detected for p in self.puzzles.values())
        avg_coherence = sum(p.final_coherence for p in self.puzzles.values()) / len(self.puzzles)
        avg_regions = sum(p.regions_discovered for p in self.puzzles.values()) / len(self.puzzles)
        
        lines.append(f"Steps totaux: {total_steps}")
        lines.append(f"Loops totaux: {total_loops} ({total_loops/total_steps*100:.1f}%)")
        lines.append(f"Cohérence moyenne: {avg_coherence:.4f}")
        lines.append(f"Régions moyennes: {avg_regions:.2f}")
        lines.append("")
        
        # Détails par puzzle
        lines.append("🔍 DÉTAILS PAR PUZZLE")
        lines.append("-" * 80)
        
        for puzzle_id, puzzle in self.puzzles.items():
            lines.append(f"\n{puzzle_id}:")
            lines.append(f"  Steps: {puzzle.total_steps}")
            lines.append(f"  Loops: {puzzle.loops_detected} ({puzzle.loops_detected/puzzle.total_steps*100:.1f}%)")
            lines.append(f"  Régions: {puzzle.regions_discovered}")
            lines.append(f"  Cohérence: {puzzle.final_coherence:.4f}")
            lines.append(f"  Positions uniques: {puzzle.unique_positions}/{puzzle.total_steps}")
            
            if puzzle.position_oscillations:
                top_osc = sorted(puzzle.position_oscillations.items(), 
                               key=lambda x: x[1], reverse=True)[:2]
                lines.append(f"  Top oscillations:")
                for pair, count in top_osc:
                    lines.append(f"    {pair}: {count}×")
        
        return "\n".join(lines)


def main():
    """Point d'entrée principal."""
    log_path = Path("logs/magen_v36/arc_real_puzzles.jsonl")
    
    if not log_path.exists():
        print(f"❌ Fichier introuvable: {log_path}")
        return
    
    analyzer = ForensicAnalyzer(log_path)
    analyzer.load_logs()
    analyzer.analyze_all_puzzles()
    analyzer.validate_user_analysis()
    
    # Génération rapport
    print("\n" + "=" * 80)
    print("📝 GÉNÉRATION RAPPORT")
    print("=" * 80)
    
    report = analyzer.generate_report()
    
    report_path = Path("RAPPORT_ANALYSE_FORENSIC_V36_VALIDATION_20260615.md")
    with open(report_path, 'w') as f:
        f.write(report)
    
    print(f"\n✅ Rapport généré: {report_path}")
    print(f"📄 Lignes: {len(report.splitlines())}")


if __name__ == "__main__":
    main()

# Made with Bob
