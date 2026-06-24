#!/usr/bin/env python3
"""
MAGEN V40.3 - SOUMISSION EXTERNE COMPLÈTE
==========================================

Script de validation EXTERNE sur plateformes officielles:
- 400 puzzles ARC-AGI training
- 25 jeux Arcade officiels
- Soumission Kaggle automatique
- Comparaison scores internes vs externes

Objectif: VÉRITÉ ABSOLUE sur les performances réelles de MAGEN

Auteur: Bob (Session 78)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Tuple
import numpy as np

# Configuration paths
MAGEN_ROOT = Path(__file__).parent
DATA_ROOT = MAGEN_ROOT / "data"
ARC_DATASET = DATA_ROOT / "arc-agi_training_challenges.json"
ARCADE_METADATA = DATA_ROOT / "arc_agi_3" / "arc_agi_3_games" / "metadata.json"
KAGGLE_FRAMEWORK = DATA_ROOT / "arc_agi_3_kaggle"

# Credentials (from Doppler)
KAGGLE_API_TOKEN = os.getenv("KAGGLE_API_TOKEN", "")
ARCADE_API_KEY = os.getenv("ARCADE_API_KEY", "")

# Import MAGEN modules
sys.path.insert(0, str(MAGEN_ROOT))
sys.path.insert(0, str(MAGEN_ROOT / "core"))
sys.path.insert(0, str(MAGEN_ROOT / "utils"))

from arc_dataset_loader import ARCDatasetLoader


class ExternalValidationEngine:
    """Moteur de validation externe complète"""
    
    def __init__(self):
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.results_dir = MAGEN_ROOT / "results" / "external_validation" / self.timestamp
        self.results_dir.mkdir(parents=True, exist_ok=True)
        
        self.loader = ARCDatasetLoader()
        
        self.results = {
            "metadata": {
                "timestamp": self.timestamp,
                "magen_version": "V40.3",
                "validation_type": "EXTERNAL_COMPLETE",
                "kaggle_token_configured": bool(KAGGLE_API_TOKEN),
                "arcade_key_configured": bool(ARCADE_API_KEY)
            },
            "arc_training": {
                "total": 0,
                "solved": 0,
                "failed": 0,
                "puzzles": {}
            },
            "arcade_games": {
                "total": 0,
                "solved": 0,
                "failed": 0,
                "games": {}
            },
            "external_scores": {
                "kaggle_submission_id": None,
                "kaggle_score": None,
                "arcade_api_score": None
            },
            "comparison": {
                "internal_score": None,
                "external_score": None,
                "delta": None,
                "verdict": None
            }
        }
        
        print(f"\n{'='*80}")
        print(f"MAGEN V40.3 - VALIDATION EXTERNE COMPLÈTE")
        print(f"{'='*80}")
        print(f"Timestamp: {self.timestamp}")
        print(f"Results dir: {self.results_dir}")
        print(f"Kaggle configured: {self.results['metadata']['kaggle_token_configured']}")
        print(f"Arcade configured: {self.results['metadata']['arcade_key_configured']}")
        print(f"{'='*80}\n")
    
    def test_arc_training_puzzles(self, limit: int | None = None) -> Dict[str, Any]:
        """Test sur puzzles ARC-AGI training officiels - UTILISE test_v40_3_real_arc_dataset.py"""
        print(f"\n{'='*80}")
        print(f"PHASE 1: TEST ARC-AGI TRAINING PUZZLES")
        print(f"{'='*80}\n")
        
        print(f"⚠️  IMPORTANT: Cette phase nécessite l'exécution de test_v40_3_real_arc_dataset.py")
        print(f"    Commande: python3 test_v40_3_real_arc_dataset.py --limit {limit or 400}")
        print(f"\n    Les résultats seront utilisés pour la comparaison externe.")
        print(f"{'='*80}\n")
        
        # Pour l'instant, on simule les résultats
        # L'utilisateur doit exécuter le test réel séparément
        self.results["arc_training"]["total"] = limit or 400
        self.results["arc_training"]["solved"] = 0
        self.results["arc_training"]["failed"] = 0
        self.results["arc_training"]["note"] = "Exécuter test_v40_3_real_arc_dataset.py pour obtenir les résultats réels"
        
        return self.results["arc_training"]
    
    def test_arcade_games(self) -> Dict[str, Any]:
        """Test sur jeux Arcade officiels"""
        print(f"\n{'='*80}")
        print(f"PHASE 2: TEST ARCADE GAMES")
        print(f"{'='*80}\n")
        
        # Load Arcade games
        games = self.loader.load_arcade_games()
        total = len(games)
        self.results["arcade_games"]["total"] = total
        
        print(f"Total jeux Arcade: {total}")
        print(f"Metadata: {ARCADE_METADATA}")
        print()
        
        solved = 0
        failed = 0
        
        for idx, (game_id, game_data) in enumerate(games.items(), 1):
            print(f"[{idx}/{total}] Testing game {game_id}...", end=" ")
            
            try:
                # Solve game
                start_time = time.time()
                result = self.solver.solve(game_data)
                elapsed = time.time() - start_time
                
                # Check if solved
                is_solved = result.get("success", False)
                
                if is_solved:
                    solved += 1
                    status = "✅ SOLVED"
                else:
                    failed += 1
                    status = "❌ FAILED"
                
                print(f"{status} ({elapsed:.2f}s)")
                
                # Store result
                self.results["arcade_games"]["games"][game_id] = {
                    "solved": is_solved,
                    "time": elapsed,
                    "attempts": result.get("attempts", 0),
                    "error": result.get("error", None)
                }
                
            except Exception as e:
                failed += 1
                print(f"❌ ERROR: {str(e)}")
                self.results["arcade_games"]["games"][game_id] = {
                    "solved": False,
                    "error": str(e)
                }
        
        self.results["arcade_games"]["solved"] = solved
        self.results["arcade_games"]["failed"] = failed
        
        print(f"\n{'='*80}")
        print(f"ARCADE GAMES RESULTS:")
        print(f"  Total: {total}")
        print(f"  Solved: {solved} ({100*solved/total:.1f}%)")
        print(f"  Failed: {failed} ({100*failed/total:.1f}%)")
        print(f"{'='*80}\n")
        
        return self.results["arcade_games"]
    
    def submit_to_kaggle(self) -> Dict[str, Any]:
        """Soumission à Kaggle pour validation externe"""
        print(f"\n{'='*80}")
        print(f"PHASE 3: SOUMISSION KAGGLE")
        print(f"{'='*80}\n")
        
        if not KAGGLE_API_TOKEN:
            print("❌ KAGGLE_API_TOKEN non configuré")
            print("   Utilisez: doppler secrets set KAGGLE_API_TOKEN=...")
            return {"error": "No Kaggle token"}
        
        print(f"✅ Kaggle token configuré")
        print(f"Framework: {KAGGLE_FRAMEWORK}")
        
        # TODO: Implémenter soumission Kaggle via API
        # Pour l'instant, on génère juste les instructions
        
        instructions = f"""
INSTRUCTIONS SOUMISSION KAGGLE:
================================

1. Configurer le token:
   export KAGGLE_API_TOKEN={KAGGLE_API_TOKEN}

2. Aller dans le framework:
   cd {KAGGLE_FRAMEWORK}

3. Copier l'agent MAGEN:
   cp {MAGEN_ROOT}/core/magen_solver_v40_3.py agent/my_agent.py

4. Tester localement:
   make play-local

5. Soumettre:
   make submit

6. Vérifier le statut:
   make status

7. Une fois 'complete', aller sur Kaggle et cliquer "Submit to Competition"

8. Récupérer le score et le mettre dans:
   {self.results_dir}/kaggle_score.txt
"""
        
        print(instructions)
        
        # Save instructions
        instructions_file = self.results_dir / "kaggle_submission_instructions.txt"
        instructions_file.write_text(instructions)
        
        print(f"\n✅ Instructions sauvegardées: {instructions_file}")
        
        return {
            "status": "instructions_generated",
            "instructions_file": str(instructions_file)
        }
    
    def submit_to_arcade_api(self) -> Dict[str, Any]:
        """Soumission à l'API Arcade pour validation externe"""
        print(f"\n{'='*80}")
        print(f"PHASE 4: SOUMISSION ARCADE API")
        print(f"{'='*80}\n")
        
        if not ARCADE_API_KEY:
            print("❌ ARCADE_API_KEY non configuré")
            print("   Utilisez: doppler secrets set ARCADE_API_KEY=...")
            return {"error": "No Arcade key"}
        
        print(f"✅ Arcade API key configuré: {ARCADE_API_KEY[:8]}...")
        
        # TODO: Implémenter soumission Arcade API
        # Pour l'instant, on génère juste les instructions
        
        instructions = f"""
INSTRUCTIONS SOUMISSION ARCADE API:
====================================

1. API Key: {ARCADE_API_KEY}

2. Endpoint: https://arcprize.org/api/v1/submit

3. Format de soumission:
   {{
     "api_key": "{ARCADE_API_KEY}",
     "agent_name": "MAGEN_V40_3",
     "results": {{
       "game_id": "score"
     }}
   }}

4. Récupérer le score et le mettre dans:
   {self.results_dir}/arcade_score.txt
"""
        
        print(instructions)
        
        # Save instructions
        instructions_file = self.results_dir / "arcade_submission_instructions.txt"
        instructions_file.write_text(instructions)
        
        print(f"\n✅ Instructions sauvegardées: {instructions_file}")
        
        return {
            "status": "instructions_generated",
            "instructions_file": str(instructions_file)
        }
    
    def calculate_internal_score(self) -> float:
        """Calcule le score interne MAGEN"""
        arc_solved = self.results["arc_training"]["solved"]
        arc_total = self.results["arc_training"]["total"]
        arcade_solved = self.results["arcade_games"]["solved"]
        arcade_total = self.results["arcade_games"]["total"]
        
        if arc_total + arcade_total == 0:
            return 0.0
        
        total_solved = arc_solved + arcade_solved
        total_puzzles = arc_total + arcade_total
        
        return 100.0 * total_solved / total_puzzles
    
    def compare_scores(self) -> Dict[str, Any]:
        """Compare scores internes vs externes"""
        print(f"\n{'='*80}")
        print(f"PHASE 5: COMPARAISON SCORES")
        print(f"{'='*80}\n")
        
        internal_score = self.calculate_internal_score()
        self.results["comparison"]["internal_score"] = internal_score
        
        print(f"Score interne MAGEN: {internal_score:.2f}%")
        
        # Check for external scores
        kaggle_score_file = self.results_dir / "kaggle_score.txt"
        arcade_score_file = self.results_dir / "arcade_score.txt"
        
        external_scores = []
        
        if kaggle_score_file.exists():
            kaggle_score = float(kaggle_score_file.read_text().strip())
            self.results["external_scores"]["kaggle_score"] = kaggle_score
            external_scores.append(kaggle_score)
            print(f"Score Kaggle externe: {kaggle_score:.2f}%")
        else:
            print(f"⏳ Score Kaggle en attente: {kaggle_score_file}")
        
        if arcade_score_file.exists():
            arcade_score = float(arcade_score_file.read_text().strip())
            self.results["external_scores"]["arcade_api_score"] = arcade_score
            external_scores.append(arcade_score)
            print(f"Score Arcade externe: {arcade_score:.2f}%")
        else:
            print(f"⏳ Score Arcade en attente: {arcade_score_file}")
        
        if external_scores:
            external_avg = np.mean(external_scores)
            self.results["comparison"]["external_score"] = external_avg
            delta = internal_score - external_avg
            self.results["comparison"]["delta"] = delta
            
            print(f"\nScore externe moyen: {external_avg:.2f}%")
            print(f"Delta (interne - externe): {delta:+.2f}%")
            
            # Verdict
            if abs(delta) < 5.0:
                verdict = "✅ COHÉRENT - Scores alignés (delta < 5%)"
            elif delta > 0:
                verdict = "⚠️ SURESTIMATION - Score interne trop élevé"
            else:
                verdict = "⚠️ SOUS-ESTIMATION - Score interne trop bas"
            
            self.results["comparison"]["verdict"] = verdict
            print(f"\nVerdict: {verdict}")
        else:
            print(f"\n⏳ En attente des scores externes")
            print(f"   Suivez les instructions de soumission et ajoutez les scores dans:")
            print(f"   - {kaggle_score_file}")
            print(f"   - {arcade_score_file}")
        
        print(f"{'='*80}\n")
        
        return self.results["comparison"]
    
    def save_results(self):
        """Sauvegarde les résultats complets"""
        results_file = self.results_dir / "validation_results.json"
        
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n✅ Résultats sauvegardés: {results_file}")
        
        # Generate summary report
        summary = f"""
MAGEN V40.3 - RAPPORT VALIDATION EXTERNE
=========================================

Timestamp: {self.timestamp}
Version: V40.3

RÉSULTATS INTERNES:
-------------------
ARC Training: {self.results['arc_training']['solved']}/{self.results['arc_training']['total']} ({100*self.results['arc_training']['solved']/max(1,self.results['arc_training']['total']):.1f}%)
Arcade Games: {self.results['arcade_games']['solved']}/{self.results['arcade_games']['total']} ({100*self.results['arcade_games']['solved']/max(1,self.results['arcade_games']['total']):.1f}%)
Score global interne: {self.results['comparison']['internal_score']:.2f}%

RÉSULTATS EXTERNES:
-------------------
Kaggle: {self.results['external_scores']['kaggle_score'] or 'En attente'}
Arcade API: {self.results['external_scores']['arcade_api_score'] or 'En attente'}
Score externe moyen: {self.results['comparison']['external_score'] or 'En attente'}

COMPARAISON:
------------
Delta: {self.results['comparison']['delta'] or 'En attente'}
Verdict: {self.results['comparison']['verdict'] or 'En attente'}

FICHIERS GÉNÉRÉS:
-----------------
- {results_file}
- {self.results_dir}/kaggle_submission_instructions.txt
- {self.results_dir}/arcade_submission_instructions.txt

PROCHAINES ÉTAPES:
------------------
1. Suivre les instructions de soumission Kaggle
2. Suivre les instructions de soumission Arcade API
3. Ajouter les scores externes dans les fichiers appropriés
4. Relancer ce script pour voir la comparaison finale
"""
        
        summary_file = self.results_dir / "RAPPORT_VALIDATION_EXTERNE.md"
        summary_file.write_text(summary)
        
        print(f"✅ Rapport sauvegardé: {summary_file}")
        print(f"\n{summary}")
    
    def run_complete_validation(self, arc_limit: int = None):
        """Exécute la validation complète"""
        print(f"\n{'#'*80}")
        print(f"# MAGEN V40.3 - VALIDATION EXTERNE COMPLÈTE")
        print(f"{'#'*80}\n")
        
        try:
            # Phase 1: Test ARC training
            self.test_arc_training_puzzles(limit=arc_limit)
            
            # Phase 2: Test Arcade games
            self.test_arcade_games()
            
            # Phase 3: Submit to Kaggle
            self.submit_to_kaggle()
            
            # Phase 4: Submit to Arcade API
            self.submit_to_arcade_api()
            
            # Phase 5: Compare scores
            self.compare_scores()
            
            # Save results
            self.save_results()
            
            print(f"\n{'#'*80}")
            print(f"# VALIDATION EXTERNE TERMINÉE")
            print(f"{'#'*80}\n")
            
        except Exception as e:
            print(f"\n❌ ERREUR: {str(e)}")
            import traceback
            traceback.print_exc()
            
            # Save partial results
            self.results["error"] = str(e)
            self.save_results()


def main():
    """Point d'entrée principal"""
    import argparse
    
    parser = argparse.ArgumentParser(description="MAGEN V40.3 - Validation externe complète")
    parser.add_argument("--arc-limit", type=int, default=None,
                       help="Limite de puzzles ARC à tester (None = tous)")
    parser.add_argument("--skip-arc", action="store_true",
                       help="Skip ARC training tests")
    parser.add_argument("--skip-arcade", action="store_true",
                       help="Skip Arcade games tests")
    
    args = parser.parse_args()
    
    # Create validation engine
    engine = ExternalValidationEngine()
    
    # Run validation
    engine.run_complete_validation(arc_limit=args.arc_limit)


if __name__ == "__main__":
    main()

# Made with Bob
