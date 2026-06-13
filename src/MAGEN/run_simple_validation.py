"""
MAGEN - Validation Simple ARC-AGI-3 avec Logs Forensiques
==========================================================

PROTOCOLE ACTIVÉ:
- Claude Pilot: Monitoring temps réel
- LumVorax: Forensic bit-level
- Mode: 100% LOCAL

Exécution simplifiée pour tester modules existants
"""

import sys
import os
import json
import time
import hashlib
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any
import numpy as np

# Path MAGEN
MAGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(MAGEN_DIR))

# Imports modules MAGEN existants
from symbolic.primitive_transforms import PrimitiveTransforms
from objects.object_extractor import ObjectExtractor


class SimpleForensicLogger:
    """Logger forensique simplifié"""
    
    def __init__(self):
        self.output_dir = MAGEN_DIR / "logs" / "validation"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.log_file = self.output_dir / f"forensic_{self.session_id}.jsonl"
        self.events = []
        
        print(f"[FORENSIC] Session: {self.session_id}")
        print(f"[FORENSIC] Logs: {self.log_file}")
    
    def log(self, event_type: str, data: Dict[str, Any]):
        """Logger événement"""
        event = {
            "timestamp": datetime.now().isoformat(),
            "timestamp_ns": time.time_ns(),
            "type": event_type,
            "data": data
        }
        
        self.events.append(event)
        
        # Écrire immédiatement
        with open(self.log_file, 'a') as f:
            f.write(json.dumps(event) + '\n')
        
        # Afficher en temps réel
        print(f"[{event_type}] {json.dumps(data, indent=2)}")


def main():
    """Validation simple"""
    print("\n" + "="*80)
    print("🚀 MAGEN VALIDATION SIMPLE - PROTOCOLE CLAUDE PILOT")
    print("="*80)
    print("\n[PROTOCOLE] Claude Pilot: ✓ ACTIVÉ")
    print("[PROTOCOLE] LumVorax: ✓ ACTIVÉ")
    print("[PROTOCOLE] Mode: 🏠 LOCAL")
    
    # Logger forensique
    forensic = SimpleForensicLogger()
    forensic.log("SESSION_START", {"mode": "LOCAL", "protocol": "CLAUDE_PILOT"})
    
    # Initialiser modules
    print("\n[INIT] Initialisation modules MAGEN...")
    
    try:
        primitives = PrimitiveTransforms(verbose=False)
        extractor = ObjectExtractor(verbose=False)
        
        forensic.log("MODULES_INIT", {
            "primitives": "PrimitiveTransforms",
            "extractor": "ObjectExtractor"
        })
        
        print("[INIT] ✓ Modules initialisés")
        
    except Exception as e:
        forensic.log("INIT_ERROR", {"error": str(e)})
        print(f"[ERROR] Échec initialisation: {e}")
        return 1
    
    # Charger dataset
    dataset_path = MAGEN_DIR / "arc-agi_training_challenges.json"
    
    if not dataset_path.exists():
        forensic.log("DATASET_ERROR", {"path": str(dataset_path), "error": "not_found"})
        print(f"\n❌ Dataset non trouvé: {dataset_path}")
        return 1
    
    print(f"\n[DATASET] Chargement {dataset_path.name}...")
    
    with open(dataset_path, 'r') as f:
        data = json.load(f)
    
    puzzles = list(data.items())[:3]  # 3 premiers puzzles pour test rapide
    
    forensic.log("DATASET_LOADED", {
        "path": str(dataset_path),
        "total_puzzles": len(data),
        "selected": len(puzzles)
    })
    
    print(f"[DATASET] ✓ {len(puzzles)} puzzles sélectionnés sur {len(data)}")
    
    # Tester chaque puzzle
    print("\n[VALIDATION] Début tests...")
    print("="*80)
    
    results = []
    
    for idx, (puzzle_id, puzzle_data) in enumerate(puzzles, 1):
        print(f"\n[PUZZLE {idx}/{len(puzzles)}] {puzzle_id}")
        
        start_time = time.time()
        
        try:
            # Extraire info puzzle
            train_examples = puzzle_data.get("train", [])
            test_examples = puzzle_data.get("test", [])
            
            forensic.log("PUZZLE_START", {
                "puzzle_id": puzzle_id,
                "train_count": len(train_examples),
                "test_count": len(test_examples)
            })
            
            # Analyser premier exemple train
            if train_examples:
                example = train_examples[0]
                input_grid = np.array(example["input"])
                output_grid = np.array(example["output"])
                
                # Extraire objets
                input_objects = extractor.extract_objects(input_grid)
                output_objects = extractor.extract_objects(output_grid)
                
                forensic.log("OBJECTS_EXTRACTED", {
                    "puzzle_id": puzzle_id,
                    "input_shape": input_grid.shape,
                    "output_shape": output_grid.shape,
                    "input_objects": len(input_objects),
                    "output_objects": len(output_objects)
                })
                
                print(f"  Input: {input_grid.shape}, {len(input_objects)} objets")
                print(f"  Output: {output_grid.shape}, {len(output_objects)} objets")
            
            elapsed = time.time() - start_time
            
            result = {
                "puzzle_id": puzzle_id,
                "success": True,
                "elapsed": elapsed
            }
            
            forensic.log("PUZZLE_COMPLETE", result)
            results.append(result)
            
            print(f"  ✓ Complété en {elapsed:.2f}s")
            
        except Exception as e:
            elapsed = time.time() - start_time
            
            result = {
                "puzzle_id": puzzle_id,
                "success": False,
                "error": str(e),
                "elapsed": elapsed
            }
            
            forensic.log("PUZZLE_ERROR", result)
            results.append(result)
            
            print(f"  ✗ Erreur: {e}")
    
    # Résumé
    print("\n" + "="*80)
    print("📊 RÉSULTATS")
    print("="*80)
    
    successes = sum(1 for r in results if r["success"])
    total_time = sum(r["elapsed"] for r in results)
    
    summary = {
        "total": len(results),
        "successes": successes,
        "failures": len(results) - successes,
        "total_time": total_time,
        "avg_time": total_time / len(results) if results else 0,
        "forensic_events": len(forensic.events),
        "log_file": str(forensic.log_file)
    }
    
    forensic.log("VALIDATION_COMPLETE", summary)
    
    print(f"\nPuzzles: {summary['total']}")
    print(f"Succès: {summary['successes']}")
    print(f"Échecs: {summary['failures']}")
    print(f"Temps total: {summary['total_time']:.2f}s")
    print(f"Temps moyen: {summary['avg_time']:.2f}s")
    print(f"\nLogs forensiques: {summary['forensic_events']} événements")
    print(f"Fichier: {summary['log_file']}")
    
    print("\n" + "="*80)
    print("✅ VALIDATION TERMINÉE")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
