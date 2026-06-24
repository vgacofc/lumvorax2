#!/usr/bin/env python3
"""
Debug: Explorer structure observation ARC-AGI-3
================================================
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from arc_agi import Arcade, OperationMode

# Charger API key
api_key = os.getenv("ARC_API_KEY", "")

# Connexion
arcade = Arcade(arc_api_key=api_key, operation_mode=OperationMode.NORMAL)
print("✅ Arcade connecté\n")

# Créer env
game_id = "ar25"
arcade.create_scorecard(game_id)
env = arcade.make(game_id)

# Reset
obs_initial = env.reset()
print("="*80)
print("OBSERVATION INITIALE (env.reset())")
print("="*80)
print(f"Type: {type(obs_initial)}")
print(f"Dir: {[x for x in dir(obs_initial) if not x.startswith('_')]}")
print(f"\nAttributs:")
for attr in dir(obs_initial):
    if not attr.startswith('_'):
        try:
            val = getattr(obs_initial, attr)
            if not callable(val):
                print(f"  {attr}: {type(val)} = {val if not hasattr(val, 'shape') else f'shape={val.shape}'}")
        except:
            pass

# Step
from arcengine import GameAction
obs_after = env.step(GameAction.ACTION1)

print("\n" + "="*80)
print("OBSERVATION APRÈS STEP (env.step(ACTION1))")
print("="*80)
print(f"Type: {type(obs_after)}")
print(f"Dir: {[x for x in dir(obs_after) if not x.startswith('_')]}")
print(f"\nAttributs:")
for attr in dir(obs_after):
    if not attr.startswith('_'):
        try:
            val = getattr(obs_after, attr)
            if not callable(val):
                print(f"  {attr}: {type(val)} = {val if not hasattr(val, 'shape') else f'shape={val.shape}'}")
        except:
            pass

print("\n" + "="*80)
print("COMPARAISON")
print("="*80)
print(f"obs_initial == obs_after: {obs_initial == obs_after}")
print(f"obs_initial is obs_after: {obs_initial is obs_after}")

# Tester accès grid
print("\n" + "="*80)
print("ACCÈS GRID")
print("="*80)
for name in ['grid', 'observation', 'state', 'pixels', 'image']:
    if hasattr(obs_after, name):
        val = getattr(obs_after, name)
        print(f"✅ obs_after.{name}: {type(val)} {getattr(val, 'shape', '')}")
    else:
        print(f"❌ obs_after.{name}: N'EXISTE PAS")

# Made with Bob
