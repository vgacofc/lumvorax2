#!/usr/bin/env python3
"""
SESSION 80 - PHASE 2.5 BIS: VALIDATION CAUSALITÉ STRICTE (VERSION SIMPLIFIÉE)
==============================================================================

Objectif: Valider causalité action→mouvement avec protocole strict
- Tester 4 actions avec avatar au centre
- 20 répétitions par action
- Corrélation > 95%

Auteur: Bob
Date: 2026-06-17
"""

import os
import sys
import json
import numpy as np
from datetime import datetime
from pathlib import Path
from collections import defaultdict

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# Configuration
GAME_NAME = "ls20-9607627b"
NUM_REPETITIONS = 20
CORRELATION_THRESHOLD = 0.95
RESULTS_DIR = Path(__file__).parent / "results" / "session80_phase2_5_bis"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

print(f"""
{'='*80}
SESSION 80 - PHASE 2.5 BIS: VALIDATION CAUSALITÉ STRICTE
{'='*80}
Jeu: {GAME_NAME}
Répétitions: {NUM_REPETITIONS} par action
Seuil: {CORRELATION_THRESHOLD:.0%}
{'='*80}
""")

def find_avatar(frame):
    """Trouve avatar (pixel value 4 de Phase 1)"""
    # Gérer différentes dimensions de frame
    if len(frame.shape) == 3:
        # Frame RGB: chercher sur premier canal
        frame_2d = frame[:, :, 0]
    else:
        frame_2d = frame
    
    positions = np.argwhere(frame_2d == 4)
    if len(positions) > 0:
        return tuple(positions[0])
    
    # Debug: afficher valeurs uniques si avatar non trouvé
    unique_vals = np.unique(frame_2d)
    print(f"  Debug: Valeurs uniques dans frame: {unique_vals[:10]}")
    return None

def test_action(env, action, action_name, num_reps=20):
    """Teste une action avec répétitions"""
    print(f"\n🎯 Test {action_name}...")
    
    movements = []
    
    for rep in range(num_reps):
        # Reset
        obs = env.reset()
        frame_before = np.asarray(obs.frame)
        pos_before = find_avatar(frame_before)
        
        if pos_before is None:
            print(f"  ⚠️ Rep {rep}: Avatar non trouvé")
            continue
        
        # Exécuter action
        obs = env.step(action)
        frame_after = np.asarray(obs.frame)
        pos_after = find_avatar(frame_after)
        
        if pos_after is None:
            print(f"  ⚠️ Rep {rep}: Avatar perdu")
            continue
        
        # Calculer mouvement
        dy = pos_after[0] - pos_before[0]
        dx = pos_after[1] - pos_before[1]
        
        movements.append({
            "rep": int(rep),
            "pos_before": [int(pos_before[0]), int(pos_before[1])],
            "pos_after": [int(pos_after[0]), int(pos_after[1])],
            "dx": int(dx),
            "dy": int(dy)
        })
        
        if rep % 5 == 0:
            print(f"  Rep {rep:2d}: ({pos_before[1]:2d},{pos_before[0]:2d}) → ({pos_after[1]:2d},{pos_after[0]:2d})  Δx={dx:+3d}, Δy={dy:+3d}")
    
    # Calculer statistiques
    if movements:
        dx_values = [m['dx'] for m in movements]
        dy_values = [m['dy'] for m in movements]
        
        mean_dx = np.mean(dx_values)
        mean_dy = np.mean(dy_values)
        std_dx = np.std(dx_values)
        std_dy = np.std(dy_values)
        
        # Calculer corrélation selon direction attendue
        if action_name == "LEFT":
            correlation = sum(1 for dx in dx_values if dx < 0) / len(dx_values)
        elif action_name == "RIGHT":
            correlation = sum(1 for dx in dx_values if dx > 0) / len(dx_values)
        elif action_name == "UP":
            correlation = sum(1 for dy in dy_values if dy < 0) / len(dy_values)
        elif action_name == "DOWN":
            correlation = sum(1 for dy in dy_values if dy > 0) / len(dy_values)
        else:
            correlation = 0.0
        
        validated = correlation >= CORRELATION_THRESHOLD
        
        print(f"\n  📈 Statistiques:")
        print(f"     Mouvement moyen: Δx={mean_dx:+.2f}, Δy={mean_dy:+.2f}")
        print(f"     Écart-type: σx={std_dx:.2f}, σy={std_dy:.2f}")
        print(f"     Corrélation: {correlation:.2%}")
        print(f"     Validé (>{CORRELATION_THRESHOLD:.0%}): {'✅ OUI' if validated else '❌ NON'}")
        
        return {
            "action_name": action_name,
            "num_tests": len(movements),
            "mean_dx": float(mean_dx),
            "mean_dy": float(mean_dy),
            "std_dx": float(std_dx),
            "std_dy": float(std_dy),
            "correlation": float(correlation),
            "validated": validated,
            "movements": movements
        }
    
    return None

def main():
    """Exécution principale"""
    
    # Initialisation
    print("🔧 Initialisation...")
    arcade = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
    env = arcade.make(GAME_NAME)
    
    # Tester chaque action
    actions = [
        (GameAction.ACTION1, "LEFT"),
        (GameAction.ACTION2, "RIGHT"),
        (GameAction.ACTION3, "UP"),
        (GameAction.ACTION4, "DOWN")
    ]
    
    results = {}
    validated_count = 0
    
    for action, name in actions:
        result = test_action(env, action, name, NUM_REPETITIONS)
        if result:
            results[name] = result
            if result['validated']:
                validated_count += 1
    
    # Résultats finaux
    print(f"\n{'='*80}")
    print("RÉSULTATS FINAUX")
    print(f"{'='*80}\n")
    
    print(f"Actions validées: {validated_count}/4 ({validated_count/4:.0%})")
    
    for name, result in results.items():
        status = "✅" if result['validated'] else "❌"
        print(f"  {status} {name}: {result['correlation']:.2%}")
    
    success = validated_count == 4
    print(f"\n{'✅ SUCCÈS' if success else '❌ ÉCHEC'}: Causalité {'VALIDÉE' if success else 'NON VALIDÉE'}")
    
    # Sauvegarder
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = RESULTS_DIR / f"strict_causality_{timestamp}.json"
    
    report = {
        "timestamp": datetime.now().isoformat(),
        "game": GAME_NAME,
        "validation": {
            "total_actions": 4,
            "validated_actions": validated_count,
            "validation_rate": validated_count / 4,
            "success": success,
            "threshold": CORRELATION_THRESHOLD
        },
        "results": results
    }
    
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n💾 Rapport sauvegardé: {report_file}")
    
    return success

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
