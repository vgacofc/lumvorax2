#!/usr/bin/env python3
"""
Validation CORRECTION 1 - Fusion Objets Adjacents
==================================================

Test intégration object_merger dans pipeline avec logs forensiques.

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

from objects.object_merger import ObjectMerger
from objects.object_extractor import ObjectExtractor


class ForensicLogger:
    """Logger forensique LumVorax"""
    
    def __init__(self, name: str):
        self.output_dir = MAGEN_DIR / "logs" / "corrections"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.log_file = self.output_dir / f"{name}_{self.session_id}.jsonl"
        
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
        
        return event


def main():
    """Validation CORRECTION 1"""
    print("\n" + "="*80)
    print("🔬 VALIDATION CORRECTION 1 - FUSION OBJETS ADJACENTS")
    print("="*80)
    print("\n[PROTOCOLE] Claude Pilot: ✓ ACTIVÉ")
    print("[PROTOCOLE] LumVorax: ✓ ACTIVÉ")
    print("[PROTOCOLE] Mode: 🏠 LOCAL")
    
    # Logger forensique
    forensic = ForensicLogger("correction1")
    forensic.log("SESSION_START", {
        "mode": "LOCAL",
        "protocol": "CLAUDE_PILOT",
        "correction": "CORRECTION_1_FUSION_ADJACENTS"
    })
    
    # Charger dataset
    dataset_path = MAGEN_DIR / "arc-agi_training_challenges.json"
    
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    # Test sur puzzle 007bbfb7 (identifié avec anomalie 4 objets au lieu de 2)
    puzzle_id = "007bbfb7"
    puzzle_data = data[puzzle_id]
    
    forensic.log("PUZZLE_SELECTED", {
        "puzzle_id": puzzle_id,
        "reason": "Anomalie détectée: 4 objets output au lieu de 2 attendus"
    })
    
    # Extraire premier exemple train
    example = puzzle_data["train"][0]
    input_grid = np.array(example["input"])
    output_grid = np.array(example["output"])
    
    print(f"\n[TEST] Puzzle {puzzle_id}")
    print(f"  Input shape: {input_grid.shape}")
    print(f"  Output shape: {output_grid.shape}")
    
    # Test 1: Extraction SANS fusion (ancien comportement)
    print("\n[TEST 1] Extraction SANS fusion (ObjectExtractor)")
    start_time = time.time()
    
    extractor = ObjectExtractor(verbose=False)
    input_objects_old = extractor.extract_objects(input_grid)
    output_objects_old = extractor.extract_objects(output_grid)
    
    elapsed1 = time.time() - start_time
    
    forensic.log("EXTRACTION_WITHOUT_MERGE", {
        "method": "ObjectExtractor",
        "input_objects": len(input_objects_old),
        "output_objects": len(output_objects_old),
        "elapsed": elapsed1
    })
    
    print(f"  Input: {len(input_objects_old)} objets")
    print(f"  Output: {len(output_objects_old)} objets")
    print(f"  Temps: {elapsed1:.6f}s")
    
    # Test 2: Extraction AVEC fusion (nouveau comportement)
    print("\n[TEST 2] Extraction AVEC fusion (ObjectMerger)")
    start_time = time.time()
    
    merger = ObjectMerger(max_distance=1, verbose=False)
    input_objects_new = merger.extract_objects(input_grid)
    input_merged = merger.merge_adjacent_objects(input_objects_new)
    
    output_objects_new = merger.extract_objects(output_grid)
    output_merged = merger.merge_adjacent_objects(output_objects_new)
    
    elapsed2 = time.time() - start_time
    
    forensic.log("EXTRACTION_WITH_MERGE", {
        "method": "ObjectMerger",
        "input_objects_before": len(input_objects_new),
        "input_objects_after": len(input_merged),
        "output_objects_before": len(output_objects_new),
        "output_objects_after": len(output_merged),
        "elapsed": elapsed2
    })
    
    print(f"  Input: {len(input_objects_new)} → {len(input_merged)} objets (après fusion)")
    print(f"  Output: {len(output_objects_new)} → {len(output_merged)} objets (après fusion)")
    print(f"  Temps: {elapsed2:.6f}s")
    
    # Analyse résultats
    print("\n[ANALYSE] Comparaison")
    
    input_improvement = len(input_objects_old) - len(input_merged)
    output_improvement = len(output_objects_old) - len(output_merged)
    
    print(f"  Input: {len(input_objects_old)} → {len(input_merged)} ({input_improvement:+d} objets)")
    print(f"  Output: {len(output_objects_old)} → {len(output_merged)} ({output_improvement:+d} objets)")
    
    # Vérifier si correction résout anomalie
    expected_output_objects = 2  # Attendu selon analyse
    anomaly_fixed = len(output_merged) == expected_output_objects
    
    forensic.log("CORRECTION_ANALYSIS", {
        "expected_output_objects": expected_output_objects,
        "actual_output_objects_old": len(output_objects_old),
        "actual_output_objects_new": len(output_merged),
        "anomaly_fixed": anomaly_fixed,
        "improvement_input": input_improvement,
        "improvement_output": output_improvement
    })
    
    # Résumé
    print("\n" + "="*80)
    print("📊 RÉSULTATS VALIDATION")
    print("="*80)
    
    if anomaly_fixed:
        print(f"\n✅ CORRECTION 1 VALIDÉE")
        print(f"  Anomalie résolue: {len(output_objects_old)} → {len(output_merged)} objets")
        print(f"  Attendu: {expected_output_objects} objets ✓")
    else:
        print(f"\n⚠️ CORRECTION 1 PARTIELLE")
        print(f"  Objets output: {len(output_merged)} (attendu: {expected_output_objects})")
    
    print(f"\nPerformance:")
    print(f"  Sans fusion: {elapsed1:.6f}s")
    print(f"  Avec fusion: {elapsed2:.6f}s")
    print(f"  Overhead: {(elapsed2-elapsed1)*1000:.2f}ms")
    
    forensic.log("VALIDATION_COMPLETE", {
        "correction": "CORRECTION_1",
        "status": "VALIDATED" if anomaly_fixed else "PARTIAL",
        "anomaly_fixed": anomaly_fixed,
        "performance_overhead_ms": (elapsed2-elapsed1)*1000
    })
    
    print(f"\nLogs forensiques: {forensic.log_file}")
    
    print("\n" + "="*80)
    if anomaly_fixed:
        print("✅ VALIDATION RÉUSSIE - CORRECTION 1 OPÉRATIONNELLE")
    else:
        print("⚠️ VALIDATION PARTIELLE - ANALYSE SUPPLÉMENTAIRE NÉCESSAIRE")
    print("="*80)
    
    return 0 if anomaly_fixed else 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
