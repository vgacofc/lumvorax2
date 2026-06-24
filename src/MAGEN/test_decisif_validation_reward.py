#!/usr/bin/env python3
"""
TEST DÉCISIF - VALIDATION REWARD EXTERNE
=========================================

OBJECTIF: Observer AU MOINS UNE transition complète victoire/défaite
MÉTHODE: Logger complet 500 steps + extraction GAME_OVER

Test critique pour valider hypothèse:
- obs.state = source reward externe
- GAME_OVER + levels_completed++ = victoire
- GAME_OVER + levels_completed= = défaite

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
MAX_STEPS = 500

print(f"""
{'='*80}
🔬 TEST DÉCISIF - VALIDATION REWARD EXTERNE
{'='*80}
Jeu: {TARGET_GAME}
Steps: {MAX_STEPS}
Objectif: Observer première victoire/défaite
Méthode: Logger complet + extraction GAME_OVER
{'='*80}
""")

# Connexion API
arc = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
print("✅ Connexion API réussie\n")

scorecard_id = arc.create_scorecard()
print(f"✅ Scorecard créé: {scorecard_id}\n")

env = arc.make(TARGET_GAME, scorecard_id=scorecard_id, seed=42, save_recording=True)

# RESET avec délai approprié
print("🔄 RESET environnement...")
obs = env.reset()
print("⏳ Attente stabilisation API (5s)...")
time.sleep(5.0)  # Délai critique pour éviter GAME_NOT_STARTED
print("✅ Environnement démarré\n")

# ============================================================================
# LOGGER COMPLET
# ============================================================================

print(f"{'='*80}")
print(f"LOGGER COMPLET - {MAX_STEPS} STEPS")
print(f"{'='*80}\n")

# Actions aléatoires
actions_pool = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]

# Historique complet
history = []
game_over_indices = []

prev_levels_completed = 0
if hasattr(obs, 'levels_completed'):
    prev_levels_completed = obs.levels_completed

for step in range(1, MAX_STEPS + 1):
    # Sélectionner action aléatoire
    action = np.random.choice(actions_pool)
    action_name = action.name
    
    # Afficher progression
    if step % 50 == 0:
        progress = int((step / MAX_STEPS) * 100)
        bar = "█" * (progress // 10) + "░" * (10 - progress // 10)
        print(f"[{bar}] {progress:3d}% | Step {step}/{MAX_STEPS}")
    
    try:
        # Exécuter action
        obs = env.step(action)
        
        # Extraire TOUTES les données
        frame = obs.frame if hasattr(obs, 'frame') else None
        frame_hash = None
        frame_diff = 0
        
        if frame is not None:
            if isinstance(frame, list):
                frame = np.array(frame)
            frame_hash = hashlib.sha256(frame.tobytes()).hexdigest()[:16]
            
            # Calculer diff avec step précédent
            if history:
                prev_frame = history[-1].get('frame_array')
                if prev_frame is not None:
                    frame_diff = int(np.abs(frame - prev_frame).sum())
        
        # Extraire état
        state_name = obs.state.name if hasattr(obs, 'state') else "UNKNOWN"
        levels_completed = obs.levels_completed if hasattr(obs, 'levels_completed') else 0
        win_levels = obs.win_levels if hasattr(obs, 'win_levels') else 0
        
        # Détecter GAME_OVER
        if state_name == "GAME_OVER":
            game_over_indices.append(step - 1)  # Index dans history
            print(f"\n🚨 GAME_OVER DÉTECTÉ au step {step}!")
            print(f"   levels_completed: {prev_levels_completed} → {levels_completed}")
            if levels_completed > prev_levels_completed:
                print(f"   ✅ VICTOIRE DÉTECTÉE!")
            else:
                print(f"   ❌ DÉFAITE DÉTECTÉE!")
        
        # Enregistrer
        entry = {
            "step": step,
            "action": action_name,
            "state": state_name,
            "levels_completed": levels_completed,
            "win_levels": win_levels,
            "frame_hash": frame_hash,
            "frame_diff": frame_diff,
            "frame_array": frame  # Pour calcul diff
        }
        history.append(entry)
        
        # Mettre à jour prev_levels
        prev_levels_completed = levels_completed
        
        time.sleep(0.05)
        
    except Exception as e:
        print(f"\n❌ Erreur step {step}: {e}")
        break

print(f"\n{'='*80}")
print("ANALYSE RÉSULTATS")
print(f"{'='*80}\n")

# Statistiques
total_steps = len(history)
game_overs = len(game_over_indices)

print(f"Total steps: {total_steps}")
print(f"GAME_OVER détectés: {game_overs}")

# Analyser chaque GAME_OVER
if game_overs > 0:
    print(f"\n{'='*80}")
    print("EXTRACTION GAME_OVER")
    print(f"{'='*80}\n")
    
    for i, go_idx in enumerate(game_over_indices):
        print(f"\nGAME_OVER #{i+1} (step {go_idx+1}):")
        
        # Extraire 100 steps avant et après
        start_idx = max(0, go_idx - 100)
        end_idx = min(len(history), go_idx + 100)
        
        context = history[start_idx:end_idx]
        
        # Analyser transition
        if go_idx > 0:
            before = history[go_idx - 1]
            after = history[go_idx]
            
            print(f"  AVANT (step {before['step']}):")
            print(f"    state: {before['state']}")
            print(f"    levels_completed: {before['levels_completed']}")
            
            print(f"  APRÈS (step {after['step']}):")
            print(f"    state: {after['state']}")
            print(f"    levels_completed: {after['levels_completed']}")
            
            # Déterminer type
            if after['levels_completed'] > before['levels_completed']:
                print(f"  TYPE: ✅ VICTOIRE (niveau complété)")
            else:
                print(f"  TYPE: ❌ DÉFAITE (pas de progression)")
            
            # Vérifier reset automatique
            if go_idx + 1 < len(history):
                next_step = history[go_idx + 1]
                print(f"  STEP SUIVANT (step {next_step['step']}):")
                print(f"    state: {next_step['state']}")
                print(f"    levels_completed: {next_step['levels_completed']}")
                
                if next_step['state'] == "NOT_FINISHED":
                    print(f"  RESET: ✅ Automatique détecté")
                else:
                    print(f"  RESET: ❌ Pas de reset automatique")
        
        # Sauvegarder contexte
        context_file = MAGEN_ROOT / "results" / f"game_over_{i+1}_context.json"
        context_file.parent.mkdir(parents=True, exist_ok=True)
        
        # Nettoyer pour JSON (enlever numpy arrays)
        context_clean = []
        for entry in context:
            clean_entry = {k: v for k, v in entry.items() if k != 'frame_array'}
            context_clean.append(clean_entry)
        
        with open(context_file, 'w') as f:
            json.dump(context_clean, f, indent=2)
        
        print(f"  Contexte sauvegardé: {context_file.name}")

else:
    print("\n⚠️  AUCUN GAME_OVER détecté sur {total_steps} steps")
    print("   Recommandation: Augmenter MAX_STEPS ou améliorer stratégie")

# Statistiques globales
states = {}
for entry in history:
    state = entry['state']
    states[state] = states.get(state, 0) + 1

print(f"\n{'='*80}")
print("STATISTIQUES ÉTATS")
print(f"{'='*80}\n")

for state, count in sorted(states.items(), key=lambda x: x[1], reverse=True):
    pct = (count / total_steps) * 100
    print(f"  {state:20s}: {count:4d} ({pct:5.1f}%)")

# Progression niveaux
levels_progression = []
for entry in history:
    if entry['levels_completed'] not in [e['levels_completed'] for e in levels_progression]:
        levels_progression.append(entry)

print(f"\n{'='*80}")
print("PROGRESSION NIVEAUX")
print(f"{'='*80}\n")

if len(levels_progression) > 1:
    print(f"Niveaux atteints: {len(levels_progression)}")
    for entry in levels_progression:
        print(f"  Step {entry['step']:4d}: Niveau {entry['levels_completed']}")
else:
    print(f"Aucune progression (niveau {levels_progression[0]['levels_completed']} constant)")

# Sauvegarder historique complet
history_file = MAGEN_ROOT / "results" / f"test_decisif_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"

# Nettoyer pour JSON
history_clean = []
for entry in history:
    clean_entry = {k: v for k, v in entry.items() if k != 'frame_array'}
    history_clean.append(clean_entry)

with open(history_file, 'w') as f:
    json.dump({
        "timestamp": datetime.now().isoformat(),
        "game_id": TARGET_GAME,
        "scorecard_id": scorecard_id,
        "total_steps": total_steps,
        "game_overs": game_overs,
        "states": states,
        "history": history_clean
    }, f, indent=2)

print(f"\n✅ Historique complet sauvegardé: {history_file.name}")

# VERDICT FINAL
print(f"\n{'='*80}")
print("VERDICT FINAL")
print(f"{'='*80}\n")

if game_overs > 0:
    print("✅ HYPOTHÈSE VALIDABLE")
    print(f"   {game_overs} GAME_OVER observé(s)")
    print("   Analyse contexte disponible")
    print("   Vérifier si reward externe cohérent")
else:
    print("❌ HYPOTHÈSE NON VALIDÉE")
    print("   Aucun GAME_OVER observé")
    print("   Impossible de confirmer logique reward")
    print("   Stratégie aléatoire inefficace")

print(f"\n{'='*80}")
print("FIN TEST DÉCISIF")
print(f"{'='*80}\n")

# Made with Bob