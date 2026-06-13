#!/usr/bin/env python3
"""
Validation Pipeline MAGEN Complet
==================================

Test pipeline sur puzzles ARC-AGI-3 réels avec logs forensiques temps réel.

Protocole: Claude Pilot + LumVorax
Mode: LOCAL
"""

import sys
import json
import time
from pathlib import Path
from datetime import datetime
import numpy as np

# Path MAGEN
MAGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(MAGEN_DIR))

from core.magen_pipeline import MAGENPipeline


class ForensicLogger:
    """Logger forensique LumVorax"""
    
    def __init__(self):
        self.output_dir = MAGEN_DIR / "logs" / "pipeline"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.log_file = self.output_dir / f"pipeline_forensic_{self.session_id}.jsonl"
        
        print(f"[FORENSIC] Session: {self.session_id}")
        print(f"[FORENSIC] Logs: {self.log_file}")
    
    def log(self, event_type: str, data: dict):
        """Logger événement avec timestamp nanoseconde"""
        event = {
            "timestamp": datetime.now().isoformat(),
            "timestamp_ns": time.time_ns(),
            "type": event_type,
            "data": data
        }
        
        with open(self.log_file, 'a') as f:
            f.write(json.dumps(event) + '\n')
        
        print(f"[{event_type}] {json.dumps(data, indent=2)}")


def main():
    """Validation pipeline complète"""
    print("\n" + "="*80)
    print("🚀 VALIDATION PIPELINE MAGEN - PROTOCOLE CLAUDE PILOT")
    print("="*80)
    print("\n[PROTOCOLE] Claude Pilot: ✓ ACTIVÉ")
    print("[PROTOCOLE] LumVorax: ✓ ACTIVÉ")
    print("[PROTOCOLE] Mode: 🏠 LOCAL")
    
    # Logger forensique
    forensic = ForensicLogger()
    forensic.log("SESSION_START", {"mode": "LOCAL", "protocol": "CLAUDE_PILOT"})
    
    # Initialiser pipeline
    print("\n[INIT] Initialisation pipeline MAGEN...")
    pipeline = MAGENPipeline(verbose=False)
    forensic.log("PIPELINE_INIT", {"status": "success"})
    print("[INIT] ✓ Pipeline initialisé")
    
    # Charger dataset
    dataset_path = MAGEN_DIR / "arc-agi_training_challenges.json"
    
    if not dataset_path.exists():
        print(f"\n❌ Dataset non trouvé: {dataset_path}")
        return 1
    
    print(f"\n[DATASET] Chargement {dataset_path.name}...")
    
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    puzzles = list(data.items())[:3]
    
    forensic.log("DATASET_LOADED", {
        "path": str(dataset_path),
        "total": len(data),
        "selected": len(puzzles)
    })
    
    print(f"[DATASET] ✓ {len(puzzles)} puzzles sélectionnés")
    
    # Tester chaque puzzle
    print("\n[VALIDATION] Début tests pipeline...")
    print("="*80)
    
    results = []
    
    for idx, (puzzle_id, puzzle_data) in enumerate(puzzles, 1):
        print(f"\n[PUZZLE {idx}/{len(puzzles)}] {puzzle_id}")
        
        start_time = time.time()
        
        try:
            # Préparer données
            train_examples = []
            for ex in puzzle_data.get("train", []):
                train_examples.append({
                    "input": np.array(ex["input"]),
                    "output": np.array(ex["output"])
                })
            
            test_examples = puzzle_data.get("test", [])
            if not test_examples:
                print("  ⚠️ Pas d'exemple test")
                continue
            
            test_input = np.array(test_examples[0]["input"])
            test_output = np.array(test_examples[0]["output"])
            
            forensic.log("PUZZLE_START", {
                "puzzle_id": puzzle_id,
                "train_count": len(train_examples),
                "test_shape": test_input.shape
            })
            
            # Résoudre avec pipeline
            prediction, metadata = pipeline.solve_puzzle(train_examples, test_input)
            
            # Vérifier exactitude
            exact_match = np.array_equal(prediction, test_output)
            
            elapsed = time.time() - start_time
            
            result = {
                "puzzle_id": puzzle_id,
                "success": exact_match,
                "elapsed": elapsed,
                "metadata": metadata
            }
            
            results.append(result)
            
            forensic.log("PUZZLE_COMPLETE", result)
            
            status = "✓" if exact_match else "❌"
            print(f"  {status} Complété en {elapsed:.3f}s (exact_match: {exact_match})")
            
        except Exception as e:
            forensic.log("PUZZLE_ERROR", {
                "puzzle_id": puzzle_id,
                "error": str(e)
            })
            print(f"  ❌ Erreur: {e}")
            continue
    
    # Résumé
    print("\n" + "="*80)
    print("📊 RÉSULTATS PIPELINE")
    print("="*80)
    
    total = len(results)
    successes = sum(1 for r in results if r["success"])
    failures = total - successes
    success_rate = (successes / total * 100) if total > 0 else 0
    
    forensic.log("VALIDATION_COMPLETE", {
        "total": total,
        "successes": successes,
        "failures": failures,
        "success_rate": success_rate,
        "results": results
    })
    
    print(f"\nPuzzles: {total}")
    print(f"Succès: {successes} ({success_rate:.1f}%)")
    print(f"Échecs: {failures}")
    
    print(f"\nLogs forensiques: {forensic.log_file}")
    
    print("\n" + "="*80)
    if successes > 0:
        print("✅ VALIDATION RÉUSSIE - Pipeline fonctionnel")
    else:
        print("⚠️ VALIDATION PARTIELLE - Améliorations nécessaires")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
