#!/usr/bin/env python3
"""
Test: Vérifier si les actions ARC-AGI-3 modifient réellement le grid
"""

import sys
import os
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# API key
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
grid_initial = obs_initial.frame[0] if hasattr(obs_initial, 'frame') else None

print("="*80)
print("GRID INITIAL")
print("="*80)
print(f"Shape: {grid_initial.shape}")
print(f"Min: {grid_initial.min()}, Max: {grid_initial.max()}")
print(f"Unique values: {np.unique(grid_initial)}")
print(f"\nCoin supérieur gauche (5x5):")
print(grid_initial[:5, :5])
print(f"\nCoin supérieur droit (5x5):")
print(grid_initial[:5, -5:])

# Tester 5 actions
for i in range(1, 6):
    action = getattr(GameAction, f"ACTION{i}")
    print(f"\n{'='*80}")
    print(f"ACTION{i}")
    print(f"{'='*80}")
    
    obs_after = env.step(action)
    grid_after = obs_after.frame[0] if hasattr(obs_after, 'frame') else None
    
    if grid_after is None:
        print("❌ Grid None après step!")
        continue
    
    # Comparer
    diff = np.sum(grid_initial != grid_after)
    diff_pct = (diff / grid_initial.size) * 100
    
    print(f"Shape: {grid_after.shape}")
    print(f"Différences: {diff}/{grid_initial.size} pixels ({diff_pct:.2f}%)")
    
    if diff > 0:
        print(f"✅ GRID A CHANGÉ!")
        print(f"\nCoin supérieur gauche (5x5):")
        print(grid_after[:5, :5])
        print(f"\nCoin supérieur droit (5x5):")
        print(grid_after[:5, -5:])
        
        # Positions changées
        changed_positions = np.argwhere(grid_initial != grid_after)
        print(f"\nPremières positions changées (max 10):")
        for pos in changed_positions[:10]:
            y, x = pos
            print(f"  ({y},{x}): {grid_initial[y,x]} → {grid_after[y,x]}")
    else:
        print(f"❌ GRID IDENTIQUE (aucun changement)")
    
    # Update pour prochaine itération
    grid_initial = grid_after

print(f"\n{'='*80}")
print("TEST TERMINÉ")
print(f"{'='*80}")

# Made with Bob
