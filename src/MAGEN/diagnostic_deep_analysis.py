#!/usr/bin/env python3
"""
DIAGNOSTIC P0.2 - ANALYSE APPROFONDIE OBSERVATION
==================================================

OBJECTIF: Vérifier 3 hypothèses critiques
1. obs.frame change-t-il réellement ? (hash)
2. Quel format d'actions attend l'API ? (available_actions, action_input)
3. Les actions sont-elles appliquées ? (frame hash avant/après)

Auteur: Bob (Session 79 - Correction)
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
🔬 DIAGNOSTIC P0.2 - ANALYSE APPROFONDIE
{'='*80}
Hypothèse A: obs.frame change réellement (hash différent)
Hypothèse B: Format actions incorrect (API attend autre chose)
Hypothèse C: Actions non appliquées (frame identique avant/après)
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
# ANALYSE OBSERVATION INITIALE COMPLÈTE
# ============================================================================

print(f"{'='*80}")
print("ANALYSE OBSERVATION INITIALE COMPLÈTE")
print(f"{'='*80}\n")

print(f"1. Type observation: {type(obs)}")
print(f"2. Classe: {obs.__class__.__name__}")

# Tous les attributs
all_attrs = [attr for attr in dir(obs) if not attr.startswith('_')]
print(f"\n3. Attributs disponibles ({len(all_attrs)}):")
for attr in all_attrs[:20]:  # Premiers 20
    try:
        value = getattr(obs, attr)
        if not callable(value):
            print(f"   - {attr}: {type(value).__name__}")
    except:
        pass

# ANALYSE CRITIQUE: available_actions
print(f"\n{'='*80}")
print("ANALYSE CRITIQUE: available_actions")
print(f"{'='*80}")

if hasattr(obs, 'available_actions'):
    avail_actions = obs.available_actions
    print(f"Type: {type(avail_actions)}")
    print(f"Contenu: {avail_actions}")
    if isinstance(avail_actions, (list, tuple)):
        print(f"Nombre: {len(avail_actions)}")
        if len(avail_actions) > 0:
            print(f"Premier élément type: {type(avail_actions[0])}")
            print(f"Premier élément: {avail_actions[0]}")
else:
    print("❌ available_actions N'EXISTE PAS")

# ANALYSE CRITIQUE: action_input
print(f"\n{'='*80}")
print("ANALYSE CRITIQUE: action_input")
print(f"{'='*80}")

if hasattr(obs, 'action_input'):
    action_input = obs.action_input
    print(f"Type: {type(action_input)}")
    print(f"Contenu: {action_input}")
else:
    print("❌ action_input N'EXISTE PAS")

# ANALYSE CRITIQUE: frame
print(f"\n{'='*80}")
print("ANALYSE CRITIQUE: frame (observation pixels)")
print(f"{'='*80}")

if hasattr(obs, 'frame'):
    frame = obs.frame
    print(f"Type: {type(frame)}")
    
    # Convertir en numpy array si c'est une liste
    if isinstance(frame, list):
        print(f"⚠️  frame est une LIST, conversion en numpy array...")
        frame = np.array(frame)
        print(f"Après conversion: {type(frame)}")
    
    print(f"Shape: {frame.shape}")
    print(f"Dtype: {frame.dtype}")
    print(f"Min: {np.min(frame)}")
    print(f"Max: {np.max(frame)}")
    print(f"Mean: {np.mean(frame):.2f}")
    print(f"Unique values: {len(np.unique(frame))}")
    
    # Hash initial
    frame_bytes = frame.tobytes()
    frame_hash = hashlib.sha256(frame_bytes).hexdigest()[:16]
    print(f"Hash SHA256: {frame_hash}")
    
    # Afficher échantillon pixels
    if len(frame.shape) == 2:
        print(f"\nÉchantillon pixels (5x5 coin supérieur gauche):")
        print(frame[:5, :5])
    elif len(frame.shape) == 3:
        print(f"\nÉchantillon pixels (5x5 coin supérieur gauche, canal 0):")
        print(frame[:5, :5, 0])
else:
    print("❌ frame N'EXISTE PAS")
    frame = None
    frame_hash = None

# ANALYSE: state
print(f"\n{'='*80}")
print("ANALYSE: state")
print(f"{'='*80}")

if hasattr(obs, 'state'):
    print(f"État: {obs.state.name}")
    print(f"Type: {type(obs.state)}")
else:
    print("❌ state N'EXISTE PAS")

# ANALYSE: levels_completed, win_levels
print(f"\n{'='*80}")
print("ANALYSE: Progression jeu")
print(f"{'='*80}")

if hasattr(obs, 'levels_completed'):
    print(f"Niveaux complétés: {obs.levels_completed}")
if hasattr(obs, 'win_levels'):
    print(f"Niveaux gagnés: {obs.win_levels}")

# ============================================================================
# TEST HYPOTHÈSE A: obs.frame CHANGE-T-IL ?
# ============================================================================

print(f"\n{'='*80}")
print("TEST HYPOTHÈSE A: obs.frame CHANGE-T-IL ?")
print(f"{'='*80}\n")

frame_hashes = [frame_hash] if frame_hash else []
frame_stats = []

for i in range(10):
    print(f"Step {i+1:2d} | Action: ACTION1 (HAUT) | ", end='', flush=True)
    
    try:
        obs = env.step(GameAction.ACTION1)
        
        if hasattr(obs, 'frame'):
            frame = obs.frame
            if isinstance(frame, list):
                frame = np.array(frame)
            frame_bytes = frame.tobytes()
            new_hash = hashlib.sha256(frame_bytes).hexdigest()[:16]
            
            # Comparer avec hash précédent
            changed = (new_hash != frame_hashes[-1]) if frame_hashes else False
            change_indicator = "✅ CHANGÉ" if changed else "❌ IDENTIQUE"
            
            frame_hashes.append(new_hash)
            frame_stats.append({
                'step': i+1,
                'hash': new_hash,
                'changed': changed,
                'min': float(np.min(frame)),
                'max': float(np.max(frame)),
                'mean': float(np.mean(frame))
            })
            
            print(f"Hash: {new_hash} | {change_indicator}")
        else:
            print("❌ frame N'EXISTE PAS")
            break
        
        time.sleep(0.1)
        
    except Exception as e:
        print(f"ERREUR: {e}")
        break

# ============================================================================
# TEST HYPOTHÈSE B: FORMAT ACTIONS
# ============================================================================

print(f"\n{'='*80}")
print("TEST HYPOTHÈSE B: FORMAT ACTIONS ATTENDU")
print(f"{'='*80}\n")

print("Test 1: Envoyer GameAction.ACTION1 (enum)")
try:
    obs = env.step(GameAction.ACTION1)
    print("✅ GameAction.ACTION1 accepté")
except Exception as e:
    print(f"❌ GameAction.ACTION1 rejeté: {e}")

print("\nTest 2: Envoyer entier 1")
try:
    obs = env.step(1)
    print("✅ Entier 1 accepté")
except Exception as e:
    print(f"❌ Entier 1 rejeté: {e}")

print("\nTest 3: Vérifier available_actions après step")
if hasattr(obs, 'available_actions'):
    print(f"available_actions: {obs.available_actions}")

# ============================================================================
# TEST HYPOTHÈSE C: ACTIONS APPLIQUÉES ?
# ============================================================================

print(f"\n{'='*80}")
print("TEST HYPOTHÈSE C: ACTIONS SONT-ELLES APPLIQUÉES ?")
print(f"{'='*80}\n")

print("Séquence: DROITE × 5 (ACTION4)")

sequence_hashes = []
for i in range(5):
    print(f"Step {i+1} | Action: DROITE | ", end='', flush=True)
    
    try:
        # Hash AVANT action
        if hasattr(obs, 'frame'):
            frame_before = obs.frame
            if isinstance(frame_before, list):
                frame_before = np.array(frame_before)
            hash_before = hashlib.sha256(frame_before.tobytes()).hexdigest()[:16]
        else:
            hash_before = None
        
        # Exécuter action
        obs = env.step(GameAction.ACTION4)
        
        # Hash APRÈS action
        if hasattr(obs, 'frame'):
            frame_after = obs.frame
            if isinstance(frame_after, list):
                frame_after = np.array(frame_after)
            hash_after = hashlib.sha256(frame_after.tobytes()).hexdigest()[:16]
            changed = (hash_after != hash_before) if hash_before else False
            change_indicator = "✅ CHANGÉ" if changed else "❌ IDENTIQUE"
            
            sequence_hashes.append({
                'step': i+1,
                'hash_before': hash_before,
                'hash_after': hash_after,
                'changed': changed
            })
            
            print(f"Avant: {hash_before} | Après: {hash_after} | {change_indicator}")
        else:
            print("❌ frame N'EXISTE PAS")
            break
        
        time.sleep(0.1)
        
    except Exception as e:
        print(f"ERREUR: {e}")
        break

# ============================================================================
# ANALYSE FINALE
# ============================================================================

print(f"\n{'='*80}")
print("ANALYSE FINALE")
print(f"{'='*80}\n")

# Hypothèse A
unique_hashes = len(set(frame_hashes))
total_steps = len(frame_hashes) - 1
change_rate = (unique_hashes - 1) / total_steps if total_steps > 0 else 0.0

print(f"HYPOTHÈSE A: obs.frame CHANGE-T-IL ?")
print(f"  - Hashes uniques: {unique_hashes}/{len(frame_hashes)}")
print(f"  - Taux changement: {change_rate*100:.1f}%")
if change_rate > 0.5:
    print(f"  ✅ VALIDÉE: Le frame change réellement")
    print(f"  → L'environnement réagit aux actions")
else:
    print(f"  ❌ REJETÉE: Le frame reste identique")
    print(f"  → L'environnement ne réagit PAS aux actions")

# Hypothèse C
sequence_changes = sum(1 for h in sequence_hashes if h['changed'])
sequence_rate = sequence_changes / len(sequence_hashes) if sequence_hashes else 0.0

print(f"\nHYPOTHÈSE C: ACTIONS APPLIQUÉES ?")
print(f"  - Changements détectés: {sequence_changes}/{len(sequence_hashes)}")
print(f"  - Taux application: {sequence_rate*100:.1f}%")
if sequence_rate > 0.5:
    print(f"  ✅ VALIDÉE: Les actions sont appliquées")
else:
    print(f"  ❌ REJETÉE: Les actions ne sont PAS appliquées")

# DIAGNOSTIC FINAL
print(f"\n{'='*80}")
print("DIAGNOSTIC FINAL")
print(f"{'='*80}\n")

if change_rate > 0.5:
    print("✅ ENVIRONNEMENT RÉACTIF")
    print("   Le jeu évolue en réponse aux actions.")
    print("\n   Problème reward = 0.00 a une AUTRE cause:")
    print("   1. Extraction reward incorrecte (pas dans metadata)")
    print("   2. Conditions victoire non atteintes")
    print("   3. Reward encodé dans frame (pixels)")
else:
    print("❌ ENVIRONNEMENT STATIQUE")
    print("   Le jeu NE RÉAGIT PAS aux actions.")
    print("\n   Causes possibles:")
    print("   1. Format actions incorrect")
    print("   2. État jeu bloqué après RESET")
    print("   3. Bug API ou environnement")

# Sauvegarder résultats
results = {
    "timestamp": datetime.now().isoformat(),
    "game_id": TARGET_GAME,
    "scorecard_id": scorecard_id,
    "hypothesis_a_frame_changes": {
        "unique_hashes": unique_hashes,
        "total_steps": len(frame_hashes),
        "change_rate": change_rate,
        "validated": change_rate > 0.5,
        "hashes": frame_hashes[:10]  # Premiers 10
    },
    "hypothesis_c_actions_applied": {
        "changes_detected": sequence_changes,
        "total_steps": len(sequence_hashes),
        "application_rate": sequence_rate,
        "validated": sequence_rate > 0.5,
        "sequence": sequence_hashes
    },
    "frame_stats": frame_stats[:10],  # Premiers 10
    "diagnosis": {
        "environment_reactive": change_rate > 0.5,
        "actions_applied": sequence_rate > 0.5,
        "conclusion": "ENVIRONNEMENT RÉACTIF" if change_rate > 0.5 else "ENVIRONNEMENT STATIQUE"
    }
}

results_file = MAGEN_ROOT / "results" / f"diagnostic_deep_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
results_file.parent.mkdir(parents=True, exist_ok=True)
with open(results_file, 'w') as f:
    json.dump(results, f, indent=2)

print(f"\n✅ Résultats sauvegardés: {results_file}")

print(f"\n{'='*80}")
print("FIN DIAGNOSTIC P0.2")
print(f"{'='*80}\n")

# Made with Bob