#!/usr/bin/env python3
"""Compare scores internes vs externes"""
import json
from pathlib import Path

RESULTS_DIR = Path("/home/lvx/LVX/lumvorax2/src/MAGEN/results/external_validation")

# Score interne (Session 77)
internal_score = 100.00

# Scores externes
kaggle_file = RESULTS_DIR / "kaggle_score.txt"
arcade_file = RESULTS_DIR / "arcade_score.txt"

kaggle_score = None
arcade_score = None

if kaggle_file.exists():
    kaggle_score = float(kaggle_file.read_text().strip())

if arcade_file.exists():
    arcade_score = float(arcade_file.read_text().strip())

print("="*80)
print("MAGEN V40.3 - COMPARAISON SCORES")
print("="*80)
print(f"\nScore interne:  {internal_score:.2f}%")
print(f"Score Kaggle:   {kaggle_score:.2f}%" if kaggle_score else "Score Kaggle:   ⏳ En attente")
print(f"Score Arcade:   {arcade_score:.2f}%" if arcade_score else "Score Arcade:   ⏳ En attente")

if kaggle_score and arcade_score:
    external_avg = (kaggle_score + arcade_score) / 2
    delta = internal_score - external_avg
    
    print(f"\nScore externe moyen: {external_avg:.2f}%")
    print(f"Delta (interne - externe): {delta:+.2f}%")
    
    print("\n" + "="*80)
    if abs(delta) < 5.0:
        print("✅ VERDICT: Scores COHÉRENTS (delta < 5%)")
        print("   → Hypothèse 'mémoire fondamentale' VALIDÉE")
        print("   → Architecture MAGEN confirmée")
    elif delta > 20:
        print("⚠️  VERDICT: SURESTIMATION INTERNE (delta > 20%)")
        print("   → Simulation détectée")
        print("   → Investigation code de résolution nécessaire")
    else:
        print("⚠️  VERDICT: Écart modéré (5% < delta < 20%)")
        print("   → Analyse approfondie recommandée")
    print("="*80)
else:
    print("\n⏳ En attente des scores externes")
    print("   Suivez les instructions de soumission:")
    print(f"   - Kaggle: {RESULTS_DIR}/kaggle_submission/INSTRUCTIONS_KAGGLE.txt")
    print(f"   - Arcade: {RESULTS_DIR}/arcade_submission/INSTRUCTIONS_ARCADE.txt")
    print("="*80)

# Sauvegarder résultats
results = {
    "timestamp": "20260616_114213",
    "internal_score": internal_score,
    "kaggle_score": kaggle_score,
    "arcade_score": arcade_score,
    "external_avg": (kaggle_score + arcade_score) / 2 if (kaggle_score and arcade_score) else None,
    "delta": internal_score - ((kaggle_score + arcade_score) / 2) if (kaggle_score and arcade_score) else None
}

with open(RESULTS_DIR / "comparison_results.json", 'w') as f:
    json.dump(results, f, indent=2)

print(f"\n✅ Résultats sauvegardés: {RESULTS_DIR}/comparison_results.json")
