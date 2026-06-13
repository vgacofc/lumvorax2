#!/usr/bin/env python3
"""
Validation Pipeline MAGEN Final - Avec Transformations Réelles
===============================================================

Test pipeline complet avec détection et application transformations.

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

from core.transform_detector import TransformDetector


class ForensicLogger:
    """Logger forensique LumVorax"""
    
    def __init__(self):
        self.output_dir = MAGEN_DIR / "logs" / "pipeline_final"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.log_file = self.output_dir / f"pipeline_final_{self.session_id}.jsonl"
        
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
    """Validation pipeline final"""
    print("\n" + "="*80)
    print("🚀 VALIDATION PIPELINE MAGEN FINAL - TRANSFORMATIONS RÉELLES")
    print("="*80)
    print("\n[PROTOCOLE] Claude Pilot: ✓ ACTIVÉ")
    print("[PROTOCOLE] LumVorax: ✓ ACTIVÉ")
    print("[PROTOCOLE] Mode: 🏠 LOCAL")
    
    # Logger forensique
    forensic = ForensicLogger()
    forensic.log("SESSION_START", {
        "mode": "LOCAL",
        "protocol": "CLAUDE_PILOT",
        "pipeline": "FINAL_WITH_TRANSFORMS"
    })
    
    # Initialiser détecteur
    detector = TransformDetector(verbose=False)
    forensic.log("DETECTOR_INIT", {"status": "success"})
    
    # Charger dataset
    dataset_path = MAGEN_DIR / "arc-agi_training_challenges.json"
    
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    puzzles = list(data.items())[:3]
    
    forensic.log("DATASET_LOADED", {
        "path": str(dataset_path),
        "total": len(data),
        "selected": len(puzzles)
    })
    
    print(f"\n[DATASET] ✓ {len(puzzles)} puzzles sélectionnés")
    
    # Tester chaque puzzle
    print("\n[VALIDATION] Début tests pipeline...")
    print("="*80)
    
    results = []
    
    for idx, (puzzle_id, puzzle_data) in enumerate(puzzles, 1):
        print(f"\n[PUZZLE {idx}/{len(puzzles)}] {puzzle_id}")
        
        start_time = time.time()
        
        try:
            # Préparer données
            train_examples = puzzle_data.get("train", [])
            test_examples = puzzle_data.get("test", [])
            
            if not train_examples or not test_examples:
                print("  ⚠️ Données manquantes")
                continue
            
            # Analyser premier exemple train
            train_input = np.array(train_examples[0]["input"])
            train_output = np.array(train_examples[0]["output"])
            
            # Détecter transformations
            transforms = detector.detect_transforms(train_input, train_output)
            
            forensic.log("TRANSFORMS_DETECTED", {
                "puzzle_id": puzzle_id,
                "train_input_shape": train_input.shape,
                "train_output_shape": train_output.shape,
                "transforms_count": len(transforms),
                "transforms": [{"type": t.type, "confidence": t.confidence} for t in transforms]
            })
            
            print(f"  Transformations: {len(transforms)} détectées")
            for t in transforms:
                print(f"    - {t.type} (confidence: {t.confidence})")
            
            # Appliquer sur test
            test_input = np.array(test_examples[0]["input"])
            test_output = np.array(test_examples[0]["output"])
            
            if transforms:
                # Appliquer première transformation (plus haute confidence)
                best_transform = max(transforms, key=lambda t: t.confidence)
                prediction = detector.apply_transform(test_input, best_transform)
                
                # Vérifier exactitude
                exact_match = np.array_equal(prediction, test_output)
                
                forensic.log("PREDICTION_MADE", {
                    "puzzle_id": puzzle_id,
                    "transform_used": best_transform.type,
                    "exact_match": exact_match
                })
                
                print(f"  Transform appliquée: {best_transform.type}")
                print(f"  Exact match: {exact_match}")
            else:
                # Fallback: copier input
                prediction = test_input.copy()
                exact_match = np.array_equal(prediction, test_output)
                
                forensic.log("PREDICTION_FALLBACK", {
                    "puzzle_id": puzzle_id,
                    "reason": "no_transforms",
                    "exact_match": exact_match
                })
                
                print(f"  Aucune transformation, copie input")
                print(f"  Exact match: {exact_match}")
            
            elapsed = time.time() - start_time
            
            result = {
                "puzzle_id": puzzle_id,
                "success": exact_match,
                "transforms_count": len(transforms),
                "elapsed": elapsed
            }
            
            results.append(result)
            
            status = "✓" if exact_match else "❌"
            print(f"  {status} Complété en {elapsed:.3f}s")
            
        except Exception as e:
            forensic.log("PUZZLE_ERROR", {
                "puzzle_id": puzzle_id,
                "error": str(e)
            })
            print(f"  ❌ Erreur: {e}")
            continue
    
    # Résumé
    print("\n" + "="*80)
    print("📊 RÉSULTATS PIPELINE FINAL")
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
        print(f"✅ SUCCÈS - {successes}/{total} puzzles résolus")
    else:
        print("⚠️ AUCUN SUCCÈS - Analyse supplémentaire nécessaire")
    print("="*80)
    
    return 0 if successes > 0 else 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
