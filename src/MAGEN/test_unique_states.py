#!/usr/bin/env python3
"""
P0.1 - TEST ÉTATS UNIQUES
Mesurer combien d'états différents MAGEN observe réellement
"""

import os
import sys
import json
import hashlib
import numpy as np
import time
from pathlib import Path
from datetime import datetime
from collections import Counter
import argparse

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    print("❌ ARC_API_KEY non configurée!")
    print("Exécutez: doppler run -- python3 test_unique_states.py")
    sys.exit(1)

# Import arc-agi
try:
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
except ImportError as e:
    print(f"❌ ERREUR import arc-agi: {e}")
    sys.exit(1)


def hash_observation(obs) -> str:
    """Créer hash unique d'une observation"""
    if obs is None:
        return "none"
    
    # Créer représentation hashable
    if hasattr(obs, 'metadata') and obs.metadata:
        # Utiliser métadonnées pour hash
        meta_str = json.dumps(obs.metadata, sort_keys=True)
        return hashlib.sha256(meta_str.encode()).hexdigest()[:16]
    elif isinstance(obs, np.ndarray):
        return hashlib.sha256(obs.tobytes()).hexdigest()[:16]
    else:
        return hashlib.sha256(str(obs).encode()).hexdigest()[:16]


def test_unique_states(steps: int = 500, output_file: str = None):
    """
    Tester diversité états avec actions aléatoires
    
    Args:
        steps: Nombre de steps à tester
        output_file: Fichier de sortie JSON
    """
    print(f"\n{'='*80}")
    print(f"🧪 TEST P0.1: ÉTATS UNIQUES")
    print(f"{'='*80}\n")
    print(f"Configuration:")
    print(f"  - Steps: {steps}")
    print(f"  - Jeu: ls20-9607627b")
    print(f"  - Stratégie: Actions aléatoires")
    print(f"\n{'─'*80}\n")
    
    # Créer environnement
    print("Initialisation environnement...")
    try:
        arc = Arcade(
            arc_api_key=ARC_API_KEY,
            operation_mode=OperationMode.COMPETITION
        )
        scorecard_id = arc.create_scorecard()
        env = arc.make(
            "ls20-9607627b",
            scorecard_id=scorecard_id,
            seed=42,
            save_recording=False
        )
        print("✓ Environnement créé\n")
    except Exception as e:
        print(f"✗ Erreur création environnement: {e}")
        return None
    
    # Structures de données
    state_hashes = []
    unique_states = set()
    state_changes = []
    action_counts = Counter()
    
    # Reset
    obs = env.reset()
    prev_hash = hash_observation(obs)
    unique_states.add(prev_hash)
    state_hashes.append(prev_hash)
    
    print(f"État initial: {prev_hash}")
    print(f"\n{'─'*80}\n")
    print("Exécution actions aléatoires...\n")
    
    # Boucle principale
    actions_list = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3,
                    GameAction.ACTION4, GameAction.ACTION5, GameAction.ACTION7]
    
    for step in range(steps):
        # Action aléatoire (ACTION1-7 pour baseline)
        action = np.random.choice(actions_list)
        action_num = action.value
        action_counts[action_num] += 1
        
        # Step
        obs, reward, done, info = env.step(action)
        
        # Hash nouvel état
        current_hash = hash_observation(obs)
        state_hashes.append(current_hash)
        
        # Détecter changement
        changed = current_hash != prev_hash
        state_changes.append(changed)
        
        if changed:
            unique_states.add(current_hash)
        
        # Affichage progression
        if (step + 1) % 50 == 0:
            unique_count = len(unique_states)
            change_rate = sum(state_changes) / len(state_changes)
            print(f"Step {step+1:3d}/{steps}: "
                  f"États uniques: {unique_count:3d} ({unique_count/(step+1)*100:5.1f}%) | "
                  f"Taux changement: {change_rate*100:5.1f}%")
        
        prev_hash = current_hash
        
        if done:
            print(f"\n⚠️ Épisode terminé au step {step+1}")
            obs = env.reset()
            prev_hash = hash_observation(obs)
    
    # Calcul statistiques finales
    total_unique = len(unique_states)
    change_rate = sum(state_changes) / len(state_changes)
    ratio_states_steps = total_unique / steps
    
    # Analyse distribution actions
    print(f"\n{'─'*80}\n")
    print("📊 RÉSULTATS:\n")
    print(f"Total steps: {steps}")
    print(f"États uniques: {total_unique}")
    print(f"Ratio états/steps: {ratio_states_steps:.2%}")
    print(f"Taux changement: {change_rate:.2%}")
    print(f"\nDistribution actions:")
    for action in sorted(action_counts.keys()):
        count = action_counts[action]
        pct = count / steps * 100
        print(f"  ACTION{action}: {count:3d} fois ({pct:5.1f}%)")
    
    # Diagnostic
    print(f"\n{'─'*80}\n")
    print("🔍 DIAGNOSTIC:\n")
    
    if ratio_states_steps < 0.05:
        print("❌ CRITIQUE: Environnement quasi-statique (< 5% états uniques)")
        print("   → Probable: Actions ignorées ou environnement figé")
    elif ratio_states_steps < 0.20:
        print("⚠️  FAIBLE: Peu de diversité d'états (< 20%)")
        print("   → Possible: Certaines actions inefficaces")
    elif ratio_states_steps < 0.50:
        print("✓  MOYEN: Diversité acceptable (20-50%)")
    else:
        print("✓✓ BON: Bonne diversité d'états (> 50%)")
    
    if change_rate < 0.10:
        print("\n❌ CRITIQUE: Très peu de changements d'état (< 10%)")
        print("   → Actions majoritairement ignorées")
    elif change_rate < 0.30:
        print("\n⚠️  FAIBLE: Peu de changements (< 30%)")
    else:
        print("\n✓  BON: Changements fréquents (> 30%)")
    
    # Résultats structurés
    results = {
        "test_id": "P0.1",
        "timestamp": datetime.now().isoformat(),
        "config": {
            "steps": steps,
            "game": "ls20-9607627b",
            "strategy": "random_actions_1_7"
        },
        "metrics": {
            "total_steps": steps,
            "unique_states": total_unique,
            "ratio_states_steps": ratio_states_steps,
            "change_rate": change_rate,
            "action_distribution": dict(action_counts)
        },
        "diagnosis": {
            "environment_dynamic": ratio_states_steps > 0.20,
            "actions_effective": change_rate > 0.30,
            "severity": "critical" if ratio_states_steps < 0.05 else "warning" if ratio_states_steps < 0.20 else "ok"
        },
        "state_hashes": state_hashes[:100]  # Premiers 100 pour debug
    }
    
    # Sauvegarder
    if output_file:
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✓ Résultats sauvegardés: {output_path}")
    
    print(f"\n{'='*80}\n")
    
    return results


def main():
    parser = argparse.ArgumentParser(description="Test P0.1: États uniques")
    parser.add_argument("--steps", type=int, default=500, help="Nombre de steps")
    parser.add_argument("--output", type=str, help="Fichier de sortie JSON")
    
    args = parser.parse_args()
    
    results = test_unique_states(args.steps, args.output)
    
    if results is None:
        sys.exit(1)
    
    # Exit code basé sur diagnostic
    if results["diagnosis"]["severity"] == "critical":
        sys.exit(2)  # Critique
    elif results["diagnosis"]["severity"] == "warning":
        sys.exit(1)  # Warning
    else:
        sys.exit(0)  # OK


if __name__ == "__main__":
    main()

# Made with Bob
