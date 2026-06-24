#!/usr/bin/env python3
"""lance 
DIAGNOSTIC P0.1 - VÉRIFICATION MOUVEMENT AGENT
==============================================

OBJECTIF: Vérifier si l'agent BOUGE réellement dans l'environnement ls20
HYPOTHÈSE: Agent immobile malgré actions envoyées (reward = 0.00 persistant)

Auteur: Bob (Session 79)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée! Exécutez: doppler run -- python3 diagnostic_agent_movement.py")

print(f"✅ ARC_API_KEY chargée: {ARC_API_KEY[:8]}...")

# Import arc-agi
try:
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
    print("✅ arc-agi importé\n")
except ImportError as e:
    print(f"❌ ERREUR: {e}")
    sys.exit(1)

# Configuration
TARGET_GAME = "ls20-9607627b"
MAGEN_ROOT = Path(__file__).parent

print(f"""
{'='*80}
🔬 DIAGNOSTIC P0.1 - VÉRIFICATION MOUVEMENT AGENT
{'='*80}
Jeu: {TARGET_GAME}
Hypothèse: Agent immobile malgré actions envoyées
Test: Séquence actions répétées + analyse position
{'='*80}
""")

# Connexion API
try:
    arc = Arcade(
        arc_api_key=ARC_API_KEY,
        operation_mode=OperationMode.COMPETITION
    )
    print("✅ Connexion API réussie\n")
except Exception as e:
    print(f"❌ ERREUR: {e}")
    sys.exit(1)

# Créer scorecard
scorecard_id = arc.create_scorecard()
print(f"✅ Scorecard créé: {scorecard_id}\n")

# Créer environnement
env = arc.make(
    TARGET_GAME,
    scorecard_id=scorecard_id,
    seed=42,
    save_recording=True
)

# RESET
print("🔄 RESET environnement...")
obs = env.reset()
time.sleep(2.0)
print("✅ Environnement démarré\n")

# Analyser observation initiale
print(f"{'='*80}")
print("ANALYSE OBSERVATION INITIALE")
print(f"{'='*80}")

print(f"\n1. Type observation: {type(obs)}")
print(f"2. Attributs disponibles: {dir(obs)}")

if hasattr(obs, 'state'):
    print(f"3. État jeu: {obs.state.name}")

if hasattr(obs, 'metadata') and obs.metadata:
    print(f"4. Metadata disponible:")
    for key, value in obs.metadata.items():
        print(f"   - {key}: {value}")
    
    # Position initiale
    x_init = obs.metadata.get('agent_x', -1)
    y_init = obs.metadata.get('agent_y', -1)
    print(f"\n5. Position initiale agent: ({x_init}, {y_init})")
else:
    print(f"4. ⚠️  Pas de metadata disponible")
    x_init, y_init = -1, -1

# Extraire observation brute si disponible
if hasattr(obs, 'observation'):
    obs_array = obs.observation
    print(f"\n6. Observation shape: {obs_array.shape}")
    print(f"7. Observation dtype: {obs_array.dtype}")
    print(f"8. Observation range: [{obs_array.min()}, {obs_array.max()}]")
    print(f"9. Observation unique values: {len(np.unique(obs_array))}")

print(f"\n{'='*80}")
print("TEST 1: ACTIONS RÉPÉTÉES - HAUT (ACTION1) × 10")
print(f"{'='*80}\n")

positions = [(x_init, y_init)]
rewards = []
states = []

for i in range(10):
    print(f"Step {i+1:2d} | Action: ACTION1 (HAUT) | ", end='', flush=True)
    
    try:
        obs = env.step(GameAction.ACTION1)
        
        # Extraire données
        state_name = obs.state.name if hasattr(obs, 'state') else "UNKNOWN"
        reward = 0.0
        x, y = -1, -1
        
        if hasattr(obs, 'metadata') and obs.metadata:
            reward = obs.metadata.get('reward', 0.0)
            x = obs.metadata.get('agent_x', -1)
            y = obs.metadata.get('agent_y', -1)
        
        positions.append((x, y))
        rewards.append(reward)
        states.append(state_name)
        
        # Vérifier mouvement
        moved = (x != positions[-2][0] or y != positions[-2][1])
        move_indicator = "✅ BOUGÉ" if moved else "❌ IMMOBILE"
        
        print(f"Pos: ({x:3d}, {y:3d}) | Reward: {reward:+5.2f} | État: {state_name:12s} | {move_indicator}")
        
        time.sleep(0.1)  # Petit délai
        
    except Exception as e:
        print(f"ERREUR: {e}")
        break

print(f"\n{'='*80}")
print("TEST 2: ACTIONS VARIÉES - SÉQUENCE DROITE/BAS")
print(f"{'='*80}\n")

# Séquence: DROITE × 5, BAS × 3
sequence = [
    (GameAction.ACTION4, "DROITE"),
    (GameAction.ACTION4, "DROITE"),
    (GameAction.ACTION4, "DROITE"),
    (GameAction.ACTION4, "DROITE"),
    (GameAction.ACTION4, "DROITE"),
    (GameAction.ACTION2, "BAS"),
    (GameAction.ACTION2, "BAS"),
    (GameAction.ACTION2, "BAS"),
]

positions_seq2 = [positions[-1]]
rewards_seq2 = []

for i, (action, name) in enumerate(sequence):
    print(f"Step {i+1:2d} | Action: {name:8s} | ", end='', flush=True)
    
    try:
        obs = env.step(action)
        
        state_name = obs.state.name if hasattr(obs, 'state') else "UNKNOWN"
        reward = 0.0
        x, y = -1, -1
        
        if hasattr(obs, 'metadata') and obs.metadata:
            reward = obs.metadata.get('reward', 0.0)
            x = obs.metadata.get('agent_x', -1)
            y = obs.metadata.get('agent_y', -1)
        
        positions_seq2.append((x, y))
        rewards_seq2.append(reward)
        
        moved = (x != positions_seq2[-2][0] or y != positions_seq2[-2][1])
        move_indicator = "✅ BOUGÉ" if moved else "❌ IMMOBILE"
        
        print(f"Pos: ({x:3d}, {y:3d}) | Reward: {reward:+5.2f} | État: {state_name:12s} | {move_indicator}")
        
        time.sleep(0.1)
        
    except Exception as e:
        print(f"ERREUR: {e}")
        break

# ANALYSE FINALE
print(f"\n{'='*80}")
print("ANALYSE FINALE")
print(f"{'='*80}\n")

# Test 1: Actions répétées
unique_positions_test1 = len(set(positions))
total_steps_test1 = len(positions) - 1
movement_rate_test1 = (unique_positions_test1 - 1) / total_steps_test1 if total_steps_test1 > 0 else 0.0
total_reward_test1 = sum(rewards)

print(f"TEST 1 (ACTION1 × 10):")
print(f"  - Positions uniques: {unique_positions_test1}/{total_steps_test1+1}")
print(f"  - Taux mouvement: {movement_rate_test1*100:.1f}%")
print(f"  - Reward total: {total_reward_test1:.2f}")
print(f"  - Positions: {positions[:5]}... (5 premières)")

# Test 2: Actions variées
unique_positions_test2 = len(set(positions_seq2))
total_steps_test2 = len(positions_seq2) - 1
movement_rate_test2 = (unique_positions_test2 - 1) / total_steps_test2 if total_steps_test2 > 0 else 0.0
total_reward_test2 = sum(rewards_seq2)

print(f"\nTEST 2 (DROITE × 5, BAS × 3):")
print(f"  - Positions uniques: {unique_positions_test2}/{total_steps_test2+1}")
print(f"  - Taux mouvement: {movement_rate_test2*100:.1f}%")
print(f"  - Reward total: {total_reward_test2:.2f}")
print(f"  - Positions: {positions_seq2}")

# DIAGNOSTIC
print(f"\n{'='*80}")
print("DIAGNOSTIC")
print(f"{'='*80}\n")

if movement_rate_test1 > 0.5 or movement_rate_test2 > 0.5:
    print("✅ AGENT MOBILE")
    print("   L'agent se déplace dans l'environnement.")
    print("   Problème reward = 0.00 a une autre cause.")
    print("\n   Hypothèses alternatives:")
    print("   1. Conditions victoire non atteintes")
    print("   2. Séquence actions incorrecte")
    print("   3. Timing/délai insuffisant")
else:
    print("❌ AGENT IMMOBILE")
    print("   L'agent NE SE DÉPLACE PAS malgré les actions.")
    print("   Cause racine du reward = 0.00 identifiée!")
    print("\n   Causes possibles:")
    print("   1. Actions non appliquées par environnement")
    print("   2. État jeu incorrect après RESET")
    print("   3. Bug API ou environnement")

if total_reward_test1 > 0 or total_reward_test2 > 0:
    print("\n✅ REWARD EXTERNE DÉTECTÉ")
    print(f"   Reward total: {total_reward_test1 + total_reward_test2:.2f}")
else:
    print("\n❌ REWARD EXTERNE = 0.00")
    print("   Aucun reward détecté sur 18 steps")

# Sauvegarder résultats
results = {
    "timestamp": datetime.now().isoformat(),
    "game_id": TARGET_GAME,
    "scorecard_id": scorecard_id,
    "test1_repeated_actions": {
        "action": "ACTION1 (HAUT)",
        "steps": total_steps_test1,
        "unique_positions": unique_positions_test1,
        "movement_rate": movement_rate_test1,
        "total_reward": total_reward_test1,
        "positions": positions
    },
    "test2_varied_actions": {
        "sequence": "DROITE × 5, BAS × 3",
        "steps": total_steps_test2,
        "unique_positions": unique_positions_test2,
        "movement_rate": movement_rate_test2,
        "total_reward": total_reward_test2,
        "positions": positions_seq2
    },
    "diagnosis": {
        "agent_mobile": movement_rate_test1 > 0.5 or movement_rate_test2 > 0.5,
        "reward_detected": total_reward_test1 > 0 or total_reward_test2 > 0,
        "conclusion": "AGENT MOBILE" if (movement_rate_test1 > 0.5 or movement_rate_test2 > 0.5) else "AGENT IMMOBILE"
    }
}

results_file = MAGEN_ROOT / "results" / f"diagnostic_movement_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
results_file.parent.mkdir(parents=True, exist_ok=True)
with open(results_file, 'w') as f:
    json.dump(results, f, indent=2)

print(f"\n✅ Résultats sauvegardés: {results_file}")

print(f"\n{'='*80}")
print("FIN DIAGNOSTIC P0.1")
print(f"{'='*80}\n")

# Made with Bob