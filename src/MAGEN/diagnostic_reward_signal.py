#!/usr/bin/env python3
"""
DIAGNOSTIC P0.3 - IDENTIFICATION SIGNAL REWARD
===============================================

OBJECTIF: Identifier précisément où se trouve le signal de progression/reward
MÉTHODE: Tracer TOUS les attributs à chaque step et détecter changements

Test décisif:
1. frame hash (changements pixels)
2. state (NOT_FINISHED → WON/LOST/GAME_OVER)
3. levels_completed (0 → 1 = victoire)
4. win_levels (progression)
5. Tous autres attributs numériques

Auteur: Bob (Session 79 - Test Décisif)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import hashlib
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

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
🔬 DIAGNOSTIC P0.3 - IDENTIFICATION SIGNAL REWARD
{'='*80}
Objectif: Identifier précisément le signal de progression
Méthode: Tracer TOUS les attributs à chaque step
Test: 200 steps avec actions variées
{'='*80}
""")

# Connexion API
arc = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
print("✅ Connexion API réussie\n")

scorecard_id = arc.create_scorecard()
print(f"✅ Scorecard créé: {scorecard_id}\n")

env = arc.make(TARGET_GAME, scorecard_id=scorecard_id, seed=42, save_recording=True)

# RESET
print("🔄 RESET environnement...")
obs = env.reset()
time.sleep(2.0)
print("✅ Environnement démarré\n")

# ============================================================================
# FONCTION EXTRACTION COMPLÈTE
# ============================================================================

def extract_all_attributes(obs) -> Dict[str, Any]:
    """Extrait TOUS les attributs observables."""
    data = {}
    
    # Frame
    if hasattr(obs, 'frame'):
        frame = obs.frame
        if isinstance(frame, list):
            frame = np.array(frame)
        data['frame_hash'] = hashlib.sha256(frame.tobytes()).hexdigest()[:16]
        data['frame_shape'] = frame.shape
        data['frame_min'] = int(np.min(frame))
        data['frame_max'] = int(np.max(frame))
        data['frame_mean'] = float(np.mean(frame))
        data['frame_unique'] = len(np.unique(frame))
        data['frame_nonzero'] = int(np.count_nonzero(frame))
    
    # State
    if hasattr(obs, 'state'):
        data['state'] = obs.state.name
        data['state_value'] = obs.state.value
    
    # Progression
    if hasattr(obs, 'levels_completed'):
        data['levels_completed'] = obs.levels_completed
    if hasattr(obs, 'win_levels'):
        data['win_levels'] = obs.win_levels
    
    # Actions
    if hasattr(obs, 'available_actions'):
        data['available_actions'] = obs.available_actions
    if hasattr(obs, 'action_input'):
        data['action_input_id'] = obs.action_input.id.name if hasattr(obs.action_input, 'id') else None
    
    # Autres attributs
    if hasattr(obs, 'full_reset'):
        data['full_reset'] = obs.full_reset
    if hasattr(obs, 'game_id'):
        data['game_id'] = obs.game_id
    if hasattr(obs, 'guid'):
        data['guid'] = obs.guid
    
    # Chercher attributs numériques cachés
    for attr in dir(obs):
        if not attr.startswith('_') and attr not in data:
            try:
                value = getattr(obs, attr)
                if isinstance(value, (int, float, bool)) and not callable(value):
                    data[attr] = value
            except:
                pass
    
    return data

# ============================================================================
# TRAÇAGE INITIAL
# ============================================================================

print(f"{'='*80}")
print("ÉTAT INITIAL")
print(f"{'='*80}\n")

initial_data = extract_all_attributes(obs)
for key, value in sorted(initial_data.items()):
    print(f"  {key:25s}: {value}")

# ============================================================================
# TEST: 200 STEPS AVEC ACTIONS VARIÉES
# ============================================================================

print(f"\n{'='*80}")
print("TEST: 200 STEPS - TRAÇAGE COMPLET")
print(f"{'='*80}\n")

# Séquence actions variées (exploration)
actions_sequence = [
    GameAction.ACTION4,  # DROITE
    GameAction.ACTION4,  # DROITE
    GameAction.ACTION4,  # DROITE
    GameAction.ACTION2,  # BAS
    GameAction.ACTION2,  # BAS
    GameAction.ACTION1,  # HAUT
    GameAction.ACTION3,  # GAUCHE
    GameAction.ACTION4,  # DROITE
] * 25  # Répéter 25 fois = 200 steps

history = []
changes_detected = []

prev_data = initial_data.copy()

for i, action in enumerate(actions_sequence):
    step_num = i + 1
    action_name = action.name
    
    print(f"Step {step_num:3d} | Action: {action_name:8s} | ", end='', flush=True)
    
    try:
        # Exécuter action
        obs = env.step(action)
        
        # Extraire données
        current_data = extract_all_attributes(obs)
        
        # Détecter changements
        changes = {}
        for key in current_data:
            if key in prev_data:
                if current_data[key] != prev_data[key]:
                    changes[key] = {
                        'before': prev_data[key],
                        'after': current_data[key]
                    }
        
        # Afficher changements critiques
        critical_changes = []
        if 'state' in changes:
            critical_changes.append(f"STATE: {changes['state']['before']} → {changes['state']['after']}")
        if 'levels_completed' in changes:
            critical_changes.append(f"LEVELS: {changes['levels_completed']['before']} → {changes['levels_completed']['after']}")
        if 'frame_hash' in changes:
            critical_changes.append(f"FRAME: CHANGED")
        
        if critical_changes:
            print(" | ".join(critical_changes))
            if 'state' in changes or 'levels_completed' in changes:
                print(f"\n   🎉 PROGRESSION DÉTECTÉE!")
                print(f"   Changements: {json.dumps(changes, indent=6)}")
        else:
            print("No critical changes")
        
        # Enregistrer
        history.append({
            'step': step_num,
            'action': action_name,
            'data': current_data,
            'changes': changes
        })
        
        if changes:
            changes_detected.append({
                'step': step_num,
                'action': action_name,
                'changes': list(changes.keys())
            })
        
        prev_data = current_data.copy()
        
        # Pause courte
        time.sleep(0.05)
        
    except Exception as e:
        print(f"ERREUR: {e}")
        break

# ============================================================================
# ANALYSE FINALE
# ============================================================================

print(f"\n{'='*80}")
print("ANALYSE FINALE")
print(f"{'='*80}\n")

# Statistiques changements
total_steps = len(history)
steps_with_changes = len(changes_detected)
change_rate = steps_with_changes / total_steps if total_steps > 0 else 0.0

print(f"Total steps: {total_steps}")
print(f"Steps avec changements: {steps_with_changes}")
print(f"Taux changement: {change_rate*100:.1f}%")

# Attributs qui ont changé
all_changed_attrs = set()
for change_event in changes_detected:
    all_changed_attrs.update(change_event['changes'])

print(f"\nAttributs qui ont changé:")
for attr in sorted(all_changed_attrs):
    count = sum(1 for c in changes_detected if attr in c['changes'])
    print(f"  - {attr:25s}: {count:3d} fois ({count/total_steps*100:.1f}%)")

# Chercher progressions levels_completed
level_progressions = [h for h in history if 'levels_completed' in h['changes']]
print(f"\nProgressions levels_completed: {len(level_progressions)}")
if level_progressions:
    print("  Détails:")
    for prog in level_progressions:
        print(f"    Step {prog['step']:3d}: {prog['changes']['levels_completed']}")

# Chercher changements state
state_changes = [h for h in history if 'state' in h['changes']]
print(f"\nChangements state: {len(state_changes)}")
if state_changes:
    print("  Détails:")
    for change in state_changes:
        print(f"    Step {change['step']:3d}: {change['changes']['state']}")

# État final
print(f"\nÉtat final:")
if history:
    final_data = history[-1]['data']
    print(f"  state: {final_data.get('state', 'N/A')}")
    print(f"  levels_completed: {final_data.get('levels_completed', 'N/A')}")
    print(f"  win_levels: {final_data.get('win_levels', 'N/A')}")
    print(f"  frame_hash: {final_data.get('frame_hash', 'N/A')}")

# ============================================================================
# DIAGNOSTIC SIGNAL REWARD
# ============================================================================

print(f"\n{'='*80}")
print("DIAGNOSTIC SIGNAL REWARD")
print(f"{'='*80}\n")

if len(level_progressions) > 0:
    print("✅ SIGNAL REWARD IDENTIFIÉ: levels_completed")
    print(f"   Progressions détectées: {len(level_progressions)}")
    print(f"   Signal exploitable pour reward externe")
elif len(state_changes) > 0:
    print("✅ SIGNAL REWARD IDENTIFIÉ: state")
    print(f"   Changements détectés: {len(state_changes)}")
    print(f"   Signal exploitable pour reward externe")
elif 'frame_hash' in all_changed_attrs:
    frame_changes = sum(1 for c in changes_detected if 'frame_hash' in c['changes'])
    print("⚠️  SIGNAL REWARD: frame uniquement")
    print(f"   Changements frame: {frame_changes}/{total_steps} ({frame_changes/total_steps*100:.1f}%)")
    print(f"   Reward intrinsèque possible, reward externe incertain")
else:
    print("❌ AUCUN SIGNAL REWARD DÉTECTÉ")
    print("   Environnement statique ou test insuffisant")

# Sauvegarder résultats
results = {
    "timestamp": datetime.now().isoformat(),
    "game_id": TARGET_GAME,
    "scorecard_id": scorecard_id,
    "total_steps": total_steps,
    "steps_with_changes": steps_with_changes,
    "change_rate": change_rate,
    "changed_attributes": {attr: sum(1 for c in changes_detected if attr in c['changes']) 
                          for attr in all_changed_attrs},
    "level_progressions": len(level_progressions),
    "state_changes": len(state_changes),
    "history_sample": history[:10],  # Premiers 10 steps
    "changes_detected_sample": changes_detected[:20],  # Premiers 20 changements
    "diagnosis": {
        "reward_signal_found": len(level_progressions) > 0 or len(state_changes) > 0,
        "reward_source": "levels_completed" if len(level_progressions) > 0 else ("state" if len(state_changes) > 0 else "frame_only"),
        "exploitable": len(level_progressions) > 0 or len(state_changes) > 0
    }
}

results_file = MAGEN_ROOT / "results" / f"diagnostic_reward_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
results_file.parent.mkdir(parents=True, exist_ok=True)
with open(results_file, 'w') as f:
    json.dump(results, f, indent=2)

print(f"\n✅ Résultats sauvegardés: {results_file}")

print(f"\n{'='*80}")
print("FIN DIAGNOSTIC P0.3")
print(f"{'='*80}\n")

# Made with Bob