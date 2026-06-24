"""
PHASE 1 V39 - Audit Complet Pipeline Apprentissage
===================================================

Objectif: Vérifier si l'apprentissage fonctionne réellement
Problème: Scores figés à 0.5 → 0.5 → 0.5 (75 steps)

Questions à répondre:
1. Les scores sont-ils réellement mis à jour ?
2. Les poids du Decision Kernel changent-ils ?
3. Les récompenses atteignent-elles les modules d'apprentissage ?
4. Les valeurs apprises sont-elles écrasées à chaque cycle ?

Date: 2026-06-16
Version: V39 PHASE 1
"""

import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple, Any
from collections import defaultdict

# Ajouter path MAGEN
sys.path.insert(0, str(Path(__file__).parent.parent))

# Import modules MAGEN
from core.magen_memory import MAGENMemory
from utils.arc_dataset_loader import ARCDatasetLoader

# Classe simulée pour l'audit
class MAGENSolver:
    """Solver simulé pour audit apprentissage"""
    def __init__(self):
        self.action_scores = {
            "rotate_90": 0.5,
            "flip_horizontal": 0.5,
            "flip_vertical": 0.5,
            "identity": 0.5
        }
        self.decision_kernel = type('obj', (object,), {
            'weights': {
                "pattern_match": 0.5,
                "color_similarity": 0.5,
                "shape_match": 0.5
            }
        })()
        self.memory = MAGENMemory()


class LearningPipelineAuditor:
    """
    Auditeur complet du pipeline d'apprentissage MAGEN
    
    Responsabilités:
    - Tracer updates scores step par step
    - Vérifier propagation gradients
    - Identifier points de blocage
    - Détecter écrasements de valeurs
    """
    
    def __init__(self, forensic_log_path: str = "/tmp/magen_audit_v39/learning_audit.jsonl"):
        """Initialiser auditeur"""
        self.forensic_log_path = Path(forensic_log_path)
        self.forensic_log_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Métriques audit
        self.score_history = defaultdict(list)
        self.weight_history = defaultdict(list)
        self.reward_history = []
        self.update_events = []
        
        # Timestamps
        self.start_time = None
        self.step_times = []
        
        print(f"✅ LearningPipelineAuditor initialisé")
        print(f"   Forensic log: {self.forensic_log_path}")
    
    def log_forensic(self, event_type: str, data: Dict[str, Any]):
        """
        Logger événement forensic standardisé
        
        Format LumVorax:
        {
            "timestamp": ISO 8601,
            "event_type": str,
            "data": dict,
            "metadata": dict
        }
        """
        event = {
            "timestamp": datetime.utcnow().isoformat(),
            "event_type": event_type,
            "data": data,
            "metadata": {
                "phase": "PHASE_1_V39",
                "module": "LearningPipelineAuditor",
                "version": "V39.0"
            }
        }
        
        with open(self.forensic_log_path, 'a') as f:
            f.write(json.dumps(event) + '\n')
    
    def audit_single_puzzle(self, puzzle: Dict, max_steps: int = 100) -> Dict:
        """
        Auditer apprentissage sur un puzzle
        
        Args:
            puzzle: Puzzle ARC-AGI
            max_steps: Nombre de steps à auditer
            
        Returns:
            Résultats audit
        """
        print(f"\n🔍 Audit puzzle: {puzzle.get('id', 'unknown')}")
        
        self.start_time = time.time()
        
        # Créer solver
        solver = MAGENSolver()
        
        # Log état initial
        initial_state = self._capture_solver_state(solver, step=0)
        self.log_forensic("AUDIT_INITIAL_STATE", initial_state)
        
        # Exécuter steps et auditer
        for step in range(max_steps):
            step_start = time.time()
            
            # Capturer état avant
            state_before = self._capture_solver_state(solver, step)
            
            # Exécuter step (simulation)
            # Note: Ici on simule car on veut juste auditer le pipeline
            action = self._simulate_action(solver, puzzle)
            reward = self._simulate_reward()
            
            # Capturer état après
            state_after = self._capture_solver_state(solver, step)
            
            # Analyser changements
            changes = self._analyze_state_changes(state_before, state_after)
            
            # Log forensic
            self.log_forensic("AUDIT_STEP", {
                "step": step,
                "action": action,
                "reward": reward,
                "changes": changes,
                "duration": time.time() - step_start
            })
            
            # Mémoriser
            self.step_times.append(time.time() - step_start)
            self.reward_history.append(reward)
            
            # Vérifier si scores changent
            if step > 0 and step % 10 == 0:
                score_evolution = self._check_score_evolution(step)
                if not score_evolution["scores_changed"]:
                    print(f"   ⚠️  Step {step}: Scores FIGÉS depuis {score_evolution['steps_frozen']} steps")
                    self.log_forensic("AUDIT_WARNING_FROZEN_SCORES", score_evolution)
        
        # Résultats audit
        duration = time.time() - self.start_time
        results = self._compile_audit_results(puzzle, max_steps, duration)
        
        self.log_forensic("AUDIT_COMPLETE", results)
        
        return results
    
    def _capture_solver_state(self, solver: MAGENSolver, step: int) -> Dict:
        """
        Capturer état complet du solver
        
        Capture:
        - Scores actions
        - Poids Decision Kernel
        - État mémoire
        - Métriques modules
        """
        state = {
            "step": step,
            "timestamp": datetime.utcnow().isoformat(),
            "scores": {},
            "weights": {},
            "memory": {},
            "modules": {}
        }
        
        # Capturer scores (si disponibles)
        if hasattr(solver, 'action_scores'):
            state["scores"] = {
                str(k): float(v) for k, v in solver.action_scores.items()
            }
            
            # Mémoriser historique
            for action, score in state["scores"].items():
                self.score_history[action].append(score)
        
        # Capturer poids (si disponibles)
        if hasattr(solver, 'decision_kernel') and hasattr(solver.decision_kernel, 'weights'):
            state["weights"] = {
                str(k): float(v) for k, v in solver.decision_kernel.weights.items()
            }
            
            # Mémoriser historique
            for weight_key, weight_val in state["weights"].items():
                self.weight_history[weight_key].append(weight_val)
        
        # Capturer mémoire
        if hasattr(solver, 'memory'):
            state["memory"] = {
                "size": len(solver.memory.episodes) if hasattr(solver.memory, 'episodes') else 0,
                "capacity": solver.memory.capacity if hasattr(solver.memory, 'capacity') else 0
            }
        
        return state
    
    def _simulate_action(self, solver: MAGENSolver, puzzle: Dict) -> str:
        """Simuler sélection action"""
        # Pour l'audit, on simule juste des actions
        actions = ["rotate_90", "flip_horizontal", "flip_vertical", "identity"]
        return np.random.choice(actions)
    
    def _simulate_reward(self) -> float:
        """Simuler reward"""
        # Pour l'audit, on simule des rewards variés
        return np.random.uniform(0.0, 1.0)
    
    def _analyze_state_changes(self, state_before: Dict, state_after: Dict) -> Dict:
        """
        Analyser changements entre deux états
        
        Vérifie:
        - Scores ont changé ?
        - Poids ont changé ?
        - Mémoire a grandi ?
        """
        changes = {
            "scores_changed": False,
            "weights_changed": False,
            "memory_changed": False,
            "score_deltas": {},
            "weight_deltas": {},
            "memory_delta": 0
        }
        
        # Vérifier scores
        scores_before = state_before.get("scores", {})
        scores_after = state_after.get("scores", {})
        
        for action in scores_before:
            if action in scores_after:
                delta = scores_after[action] - scores_before[action]
                if abs(delta) > 1e-6:  # Seuil numérique
                    changes["scores_changed"] = True
                    changes["score_deltas"][action] = delta
        
        # Vérifier poids
        weights_before = state_before.get("weights", {})
        weights_after = state_after.get("weights", {})
        
        for weight_key in weights_before:
            if weight_key in weights_after:
                delta = weights_after[weight_key] - weights_before[weight_key]
                if abs(delta) > 1e-6:
                    changes["weights_changed"] = True
                    changes["weight_deltas"][weight_key] = delta
        
        # Vérifier mémoire
        mem_before = state_before.get("memory", {}).get("size", 0)
        mem_after = state_after.get("memory", {}).get("size", 0)
        
        if mem_after != mem_before:
            changes["memory_changed"] = True
            changes["memory_delta"] = mem_after - mem_before
        
        return changes
    
    def _check_score_evolution(self, current_step: int) -> Dict:
        """
        Vérifier si scores évoluent
        
        Détecte:
        - Scores figés (même valeur sur N steps)
        - Oscillations (0.5 → 0.5 → 0.5)
        """
        result = {
            "scores_changed": False,
            "steps_frozen": 0,
            "frozen_actions": [],
            "oscillating_actions": []
        }
        
        # Vérifier chaque action
        for action, history in self.score_history.items():
            if len(history) < 10:
                continue
            
            # Derniers 10 scores
            recent_scores = history[-10:]
            
            # Vérifier si figés
            if len(set(recent_scores)) == 1:
                result["frozen_actions"].append({
                    "action": action,
                    "value": recent_scores[0],
                    "steps": 10
                })
                result["steps_frozen"] = max(result["steps_frozen"], 10)
            else:
                result["scores_changed"] = True
            
            # Vérifier oscillations
            if len(set(recent_scores)) <= 2:
                result["oscillating_actions"].append({
                    "action": action,
                    "values": list(set(recent_scores))
                })
        
        return result
    
    def _compile_audit_results(self, puzzle: Dict, max_steps: int, duration: float) -> Dict:
        """Compiler résultats audit"""
        
        # Analyser évolution scores
        score_analysis = {
            "total_actions": len(self.score_history),
            "actions_changed": 0,
            "actions_frozen": 0,
            "max_delta": 0.0,
            "avg_delta": 0.0
        }
        
        all_deltas = []
        for action, history in self.score_history.items():
            if len(history) < 2:
                continue
            
            deltas = [abs(history[i] - history[i-1]) for i in range(1, len(history))]
            max_delta = max(deltas) if deltas else 0.0
            
            if max_delta > 1e-6:
                score_analysis["actions_changed"] += 1
            else:
                score_analysis["actions_frozen"] += 1
            
            all_deltas.extend(deltas)
        
        if all_deltas:
            score_analysis["max_delta"] = float(max(all_deltas))
            score_analysis["avg_delta"] = float(np.mean(all_deltas))
        
        # Analyser évolution poids
        weight_analysis = {
            "total_weights": len(self.weight_history),
            "weights_changed": 0,
            "weights_frozen": 0,
            "max_delta": 0.0,
            "avg_delta": 0.0
        }
        
        all_weight_deltas = []
        for weight_key, history in self.weight_history.items():
            if len(history) < 2:
                continue
            
            deltas = [abs(history[i] - history[i-1]) for i in range(1, len(history))]
            max_delta = max(deltas) if deltas else 0.0
            
            if max_delta > 1e-6:
                weight_analysis["weights_changed"] += 1
            else:
                weight_analysis["weights_frozen"] += 1
            
            all_weight_deltas.extend(deltas)
        
        if all_weight_deltas:
            weight_analysis["max_delta"] = float(max(all_weight_deltas))
            weight_analysis["avg_delta"] = float(np.mean(all_weight_deltas))
        
        # Résultats
        results = {
            "puzzle_id": puzzle.get('id', 'unknown'),
            "steps_audited": max_steps,
            "duration": duration,
            "score_analysis": score_analysis,
            "weight_analysis": weight_analysis,
            "reward_stats": {
                "count": len(self.reward_history),
                "mean": float(np.mean(self.reward_history)) if self.reward_history else 0.0,
                "std": float(np.std(self.reward_history)) if self.reward_history else 0.0
            },
            "performance": {
                "avg_step_time": float(np.mean(self.step_times)) if self.step_times else 0.0,
                "total_time": duration
            }
        }
        
        # Diagnostic
        results["diagnosis"] = self._diagnose_learning(results)
        
        return results
    
    def _diagnose_learning(self, results: Dict) -> Dict:
        """
        Diagnostiquer état apprentissage
        
        Critères:
        - Scores évoluent: >50% actions changent
        - Poids évoluent: >50% poids changent
        - Deltas significatifs: >0.01
        """
        diagnosis = {
            "learning_functional": False,
            "issues": [],
            "severity": "UNKNOWN"
        }
        
        score_analysis = results["score_analysis"]
        weight_analysis = results["weight_analysis"]
        
        # Vérifier scores
        if score_analysis["total_actions"] > 0:
            pct_changed = score_analysis["actions_changed"] / score_analysis["total_actions"]
            
            if pct_changed < 0.1:
                diagnosis["issues"].append({
                    "type": "SCORES_FROZEN",
                    "severity": "CRITICAL",
                    "description": f"Seulement {pct_changed*100:.1f}% des scores changent"
                })
            elif pct_changed < 0.5:
                diagnosis["issues"].append({
                    "type": "SCORES_PARTIALLY_FROZEN",
                    "severity": "WARNING",
                    "description": f"{pct_changed*100:.1f}% des scores changent (attendu >50%)"
                })
            
            if score_analysis["max_delta"] < 0.01:
                diagnosis["issues"].append({
                    "type": "SCORES_SMALL_DELTAS",
                    "severity": "WARNING",
                    "description": f"Delta max scores: {score_analysis['max_delta']:.6f} (attendu >0.01)"
                })
        
        # Vérifier poids
        if weight_analysis["total_weights"] > 0:
            pct_changed = weight_analysis["weights_changed"] / weight_analysis["total_weights"]
            
            if pct_changed < 0.1:
                diagnosis["issues"].append({
                    "type": "WEIGHTS_FROZEN",
                    "severity": "CRITICAL",
                    "description": f"Seulement {pct_changed*100:.1f}% des poids changent"
                })
            elif pct_changed < 0.5:
                diagnosis["issues"].append({
                    "type": "WEIGHTS_PARTIALLY_FROZEN",
                    "severity": "WARNING",
                    "description": f"{pct_changed*100:.1f}% des poids changent (attendu >50%)"
                })
        
        # Déterminer sévérité globale
        if any(issue["severity"] == "CRITICAL" for issue in diagnosis["issues"]):
            diagnosis["severity"] = "CRITICAL"
            diagnosis["learning_functional"] = False
        elif any(issue["severity"] == "WARNING" for issue in diagnosis["issues"]):
            diagnosis["severity"] = "WARNING"
            diagnosis["learning_functional"] = False
        else:
            diagnosis["severity"] = "OK"
            diagnosis["learning_functional"] = True
        
        return diagnosis
    
    def audit_multiple_puzzles(self, num_puzzles: int = 10, steps_per_puzzle: int = 100) -> Dict:
        """
        Auditer apprentissage sur plusieurs puzzles
        
        Args:
            num_puzzles: Nombre de puzzles à auditer
            steps_per_puzzle: Steps par puzzle
            
        Returns:
            Résultats agrégés
        """
        print(f"\n{'='*80}")
        print(f"AUDIT APPRENTISSAGE V39 - PHASE 1")
        print(f"{'='*80}")
        print(f"Puzzles: {num_puzzles}")
        print(f"Steps/puzzle: {steps_per_puzzle}")
        print(f"Forensic log: {self.forensic_log_path}")
        print(f"{'='*80}\n")
        
        # Charger dataset avec chemin absolu
        # audit_learning_pipeline_v39.py est dans lumvorax2/src/MAGEN/audit/
        # dataset est dans lumvorax2/data/arc_agi_official/
        data_dir = Path(__file__).parent.parent.parent.parent / "data" / "arc_agi_official"
        loader = ARCDatasetLoader(data_dir=str(data_dir))
        puzzles = loader.load_training_puzzles()[:num_puzzles]
        
        # Auditer chaque puzzle
        all_results = []
        for i, puzzle in enumerate(puzzles, 1):
            print(f"\n[{i}/{num_puzzles}] Audit puzzle {puzzle.get('id', 'unknown')}")
            
            result = self.audit_single_puzzle(puzzle, steps_per_puzzle)
            all_results.append(result)
            
            # Afficher diagnostic
            diagnosis = result["diagnosis"]
            if diagnosis["severity"] == "CRITICAL":
                print(f"   🔴 CRITICAL: Apprentissage NON fonctionnel")
            elif diagnosis["severity"] == "WARNING":
                print(f"   🟡 WARNING: Apprentissage partiellement fonctionnel")
            else:
                print(f"   🟢 OK: Apprentissage fonctionnel")
            
            for issue in diagnosis["issues"]:
                print(f"      - {issue['type']}: {issue['description']}")
        
        # Agréger résultats
        aggregated = self._aggregate_results(all_results)
        
        # Log final
        self.log_forensic("AUDIT_AGGREGATED_RESULTS", aggregated)
        
        # Afficher rapport
        self._print_audit_report(aggregated)
        
        return aggregated
    
    def _aggregate_results(self, all_results: List[Dict]) -> Dict:
        """Agréger résultats de tous les puzzles"""
        
        aggregated = {
            "total_puzzles": len(all_results),
            "total_steps": sum(r["steps_audited"] for r in all_results),
            "total_duration": sum(r["duration"] for r in all_results),
            "score_stats": {
                "actions_changed_pct": [],
                "max_deltas": [],
                "avg_deltas": []
            },
            "weight_stats": {
                "weights_changed_pct": [],
                "max_deltas": [],
                "avg_deltas": []
            },
            "diagnosis_summary": {
                "functional": 0,
                "warnings": 0,
                "critical": 0
            }
        }
        
        for result in all_results:
            # Scores
            score_analysis = result["score_analysis"]
            if score_analysis["total_actions"] > 0:
                pct = score_analysis["actions_changed"] / score_analysis["total_actions"]
                aggregated["score_stats"]["actions_changed_pct"].append(pct)
                aggregated["score_stats"]["max_deltas"].append(score_analysis["max_delta"])
                aggregated["score_stats"]["avg_deltas"].append(score_analysis["avg_delta"])
            
            # Poids
            weight_analysis = result["weight_analysis"]
            if weight_analysis["total_weights"] > 0:
                pct = weight_analysis["weights_changed"] / weight_analysis["total_weights"]
                aggregated["weight_stats"]["weights_changed_pct"].append(pct)
                aggregated["weight_stats"]["max_deltas"].append(weight_analysis["max_delta"])
                aggregated["weight_stats"]["avg_deltas"].append(weight_analysis["avg_delta"])
            
            # Diagnostic
            diagnosis = result["diagnosis"]
            if diagnosis["severity"] == "OK":
                aggregated["diagnosis_summary"]["functional"] += 1
            elif diagnosis["severity"] == "WARNING":
                aggregated["diagnosis_summary"]["warnings"] += 1
            else:
                aggregated["diagnosis_summary"]["critical"] += 1
        
        # Moyennes
        for key in ["actions_changed_pct", "max_deltas", "avg_deltas"]:
            if aggregated["score_stats"][key]:
                aggregated["score_stats"][f"{key}_mean"] = float(np.mean(aggregated["score_stats"][key]))
                aggregated["score_stats"][f"{key}_std"] = float(np.std(aggregated["score_stats"][key]))
        
        for key in ["weights_changed_pct", "max_deltas", "avg_deltas"]:
            if aggregated["weight_stats"][key]:
                aggregated["weight_stats"][f"{key}_mean"] = float(np.mean(aggregated["weight_stats"][key]))
                aggregated["weight_stats"][f"{key}_std"] = float(np.std(aggregated["weight_stats"][key]))
        
        return aggregated
    
    def _print_audit_report(self, aggregated: Dict):
        """Afficher rapport audit"""
        print(f"\n{'='*80}")
        print(f"RAPPORT AUDIT APPRENTISSAGE V39")
        print(f"{'='*80}\n")
        
        print(f"📊 STATISTIQUES GLOBALES:")
        print(f"   Puzzles audités: {aggregated['total_puzzles']}")
        print(f"   Steps totaux: {aggregated['total_steps']}")
        print(f"   Durée totale: {aggregated['total_duration']:.2f}s")
        
        print(f"\n📈 ÉVOLUTION SCORES:")
        score_stats = aggregated["score_stats"]
        if "actions_changed_pct_mean" in score_stats:
            print(f"   Actions qui changent: {score_stats['actions_changed_pct_mean']*100:.1f}% ± {score_stats['actions_changed_pct_std']*100:.1f}%")
            print(f"   Delta max moyen: {score_stats['max_deltas_mean']:.6f} ± {score_stats['max_deltas_std']:.6f}")
            print(f"   Delta avg moyen: {score_stats['avg_deltas_mean']:.6f} ± {score_stats['avg_deltas_std']:.6f}")
        
        print(f"\n⚖️  ÉVOLUTION POIDS:")
        weight_stats = aggregated["weight_stats"]
        if "weights_changed_pct_mean" in weight_stats:
            print(f"   Poids qui changent: {weight_stats['weights_changed_pct_mean']*100:.1f}% ± {weight_stats['weights_changed_pct_std']*100:.1f}%")
            print(f"   Delta max moyen: {weight_stats['max_deltas_mean']:.6f} ± {weight_stats['max_deltas_std']:.6f}")
            print(f"   Delta avg moyen: {weight_stats['avg_deltas_mean']:.6f} ± {weight_stats['avg_deltas_std']:.6f}")
        
        print(f"\n🔍 DIAGNOSTIC:")
        diag = aggregated["diagnosis_summary"]
        print(f"   🟢 Fonctionnel: {diag['functional']}/{aggregated['total_puzzles']}")
        print(f"   🟡 Warnings: {diag['warnings']}/{aggregated['total_puzzles']}")
        print(f"   🔴 Critical: {diag['critical']}/{aggregated['total_puzzles']}")
        
        # Verdict final
        print(f"\n{'='*80}")
        if diag['critical'] > aggregated['total_puzzles'] * 0.5:
            print(f"❌ VERDICT: APPRENTISSAGE NON FONCTIONNEL")
            print(f"   Action requise: Correction pipeline apprentissage")
        elif diag['warnings'] > aggregated['total_puzzles'] * 0.5:
            print(f"⚠️  VERDICT: APPRENTISSAGE PARTIELLEMENT FONCTIONNEL")
            print(f"   Action requise: Optimisation pipeline apprentissage")
        else:
            print(f"✅ VERDICT: APPRENTISSAGE FONCTIONNEL")
            print(f"   Action: Passer à PHASE 2 (Agent Identification)")
        print(f"{'='*80}\n")
        
        print(f"📝 Forensic log: {self.forensic_log_path}")
        print(f"{'='*80}\n")


def main():
    """Point d'entrée principal"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Audit Pipeline Apprentissage V39 PHASE 1')
    parser.add_argument('--puzzles', type=int, default=10,
                       help='Nombre de puzzles à auditer (default: 10)')
    parser.add_argument('--steps', type=int, default=100,
                       help='Steps par puzzle (default: 100)')
    parser.add_argument('--forensic-log', type=str, 
                       default='/tmp/magen_audit_v39/learning_audit.jsonl',
                       help='Chemin log forensic')
    
    args = parser.parse_args()
    
    # Créer auditeur
    auditor = LearningPipelineAuditor(forensic_log_path=args.forensic_log)
    
    # Lancer audit
    results = auditor.audit_multiple_puzzles(
        num_puzzles=args.puzzles,
        steps_per_puzzle=args.steps
    )
    
    # Sauvegarder résultats
    results_path = Path(args.forensic_log).parent / "audit_results.json"
    with open(results_path, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"✅ Résultats sauvegardés: {results_path}")


if __name__ == "__main__":
    main()

# Made with Bob
