#!/usr/bin/env python3
"""Stratégie focus victoire - Analyse transformateurs"""
import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime

ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

TARGET_GAME = "ls20-9607627b"
MAX_STEPS = 1000  # Plus de steps pour trouver victoire

print(f"🎯 STRATÉGIE FOCUS VICTOIRE - {TARGET_GAME}")

arc = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
scorecard_id = arc.create_scorecard()
env = arc.make(TARGET_GAME, scorecard_id=scorecard_id, seed=42, save_recording=True)

obs = env.reset()
time.sleep(5.0)

print("✅ Environnement démarré\n")

# Stratégie: Explorer systématiquement pour trouver transformateurs
actions_sequence = [
    GameAction.ACTION4, GameAction.ACTION4, GameAction.ACTION4,  # Droite x3
    GameAction.ACTION2, GameAction.ACTION2,  # Bas x2
    GameAction.ACTION3, GameAction.ACTION3, GameAction.ACTION3,  # Gauche x3
    GameAction.ACTION1, GameAction.ACTION1,  # Haut x2
] * 100  # Répéter pattern

for step, action in enumerate(actions_sequence[:MAX_STEPS], 1):
    try:
        obs = env.step(action)
        
        if hasattr(obs, 'state') and obs.state.name == "GAME_OVER":
            levels = obs.levels_completed if hasattr(obs, 'levels_completed') else 0
            print(f"\n🚨 GAME_OVER au step {step}!")
            print(f"   Niveaux: {levels}")
            
            if levels > 0:
                print(f"   ✅ VICTOIRE!")
                log_file = Path(__file__).parent / "results" / "game_over_victories.jsonl"
                log_file.parent.mkdir(parents=True, exist_ok=True)
                with open(log_file, 'a') as f:
                    json.dump({
                        "timestamp": datetime.now().isoformat(),
                        "step": step,
                        "type": "VICTORY",
                        "levels_completed": levels
                    }, f)
                    f.write('\n')
            break
        
        if step % 100 == 0:
            print(f"Step {step}/{MAX_STEPS}...")
        
        time.sleep(0.05)
        
    except Exception as e:
        print(f"❌ Erreur step {step}: {e}")
        break

print("\n✅ Test terminé")
