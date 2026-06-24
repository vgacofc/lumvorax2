#!/usr/bin/env python3
"""
STRATÉGIE INTELLIGENTE POUR LS20
=================================

Objectif: Atteindre GAME_OVER en <50 steps pour valider hypothèse reward externe

Stratégie:
1. Extraire position agent depuis obs.frame
2. Détecter objets/obstacles
3. Navigation dirigée vers objectifs
4. Éviter répétitions infinies

Auteur: Bob (Session 79 - Stratégie Intelligente)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
from collections import deque

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

print(f"✅ ARC_API_KEY chargée: {ARC_API_KEY[:8]}...\n")

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
🧠 STRATÉGIE INTELLIGENTE - VALIDATION REWARD EXTERNE
{'='*80}
Jeu: {TARGET_GAME}
Steps max: {MAX_STEPS}
Stratégie: Navigation dirigée + détection objectifs
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
time.sleep(5.0)  # Délai critique
print("✅ Environnement démarré\n")

# ============================================================================
# STRATÉGIE INTELLIGENTE
# ============================================================================

def extract_agent_position(frame):
    """Extraire position agent depuis frame (valeur spécifique)"""
    if isinstance(frame, list):
        frame = np.array(frame)
    
    # Chercher valeur agent (généralement 10 dans ls20)
    agent_positions = np.argwhere(frame == 10)
    
    if len(agent_positions) > 0:
        return tuple(agent_positions[0])  # (y, x)
    
    # Fallback: chercher valeur la plus fréquente non-nulle
    unique, counts = np.unique(frame, return_counts=True)
    non_zero = [(u, c) for u, c in zip(unique, counts) if u > 0]
    if non_zero:
        # Prendre 2ème valeur la plus fréquente (1ère = background)
        sorted_vals = sorted(non_zero, key=lambda x: x[1], reverse=True)
        if len(sorted_vals) > 1:
            agent_val = sorted_vals[1][0]
            agent_positions = np.argwhere(frame == agent_val)
            if len(agent_positions) > 0:
                return tuple(agent_positions[0])
    
    return None

def detect_objectives(frame):
    """Détecter objectifs/cibles dans frame"""
    if isinstance(frame, list):
        frame = np.array(frame)
    
    # Chercher valeurs distinctes (objectifs potentiels)
    unique_vals = np.unique(frame)
    objectives = []
    
    for val in unique_vals:
        if val > 0 and val != 10:  # Exclure background et agent
            positions = np.argwhere(frame == val)
            if len(positions) > 0:
                objectives.append({
                    'value': int(val),
                    'positions': positions.tolist(),
                    'count': len(positions)
                })
    
    return objectives

def choose_smart_action(frame, prev_positions, step):
    """Choisir action intelligente basée sur frame"""
    
    # Extraire position agent
    agent_pos = extract_agent_position(frame)
    
    if agent_pos is None:
        # Fallback: exploration aléatoire
        return np.random.choice([GameAction.ACTION1, GameAction.ACTION2, 
                                GameAction.ACTION3, GameAction.ACTION4])
    
    # Détecter objectifs
    objectives = detect_objectives(frame)
    
    if not objectives:
        # Pas d'objectifs: exploration systématique
        # Alterner directions pour couvrir espace
        if step % 4 == 0:
            return GameAction.ACTION1  # HAUT
        elif step % 4 == 1:
            return GameAction.ACTION4  # DROITE
        elif step % 4 == 2:
            return GameAction.ACTION2  # BAS
        else:
            return GameAction.ACTION3  # GAUCHE
    
    # Trouver objectif le plus proche
    closest_obj = None
    min_dist = float('inf')
    
    for obj in objectives:
        for pos in obj['positions']:
            dist = abs(pos[0] - agent_pos[0]) + abs(pos[1] - agent_pos[1])
            if dist < min_dist:
                min_dist = dist
                closest_obj = pos
    
    if closest_obj is None:
        return GameAction.ACTION1
    
    # Naviguer vers objectif
    dy = closest_obj[0] - agent_pos[0]
    dx = closest_obj[1] - agent_pos[1]
    
    # Éviter répétitions: vérifier historique positions
    if len(prev_positions) > 3:
        if all(p == agent_pos for p in list(prev_positions)[-3:]):
            # Bloqué: changer direction
            return np.random.choice([GameAction.ACTION1, GameAction.ACTION2, 
                                    GameAction.ACTION3, GameAction.ACTION4])
    
    # Priorité: axe avec plus grande distance
    if abs(dy) > abs(dx):
        if dy > 0:
            return GameAction.ACTION2  # BAS
        else:
            return GameAction.ACTION1  # HAUT
    else:
        if dx > 0:
            return GameAction.ACTION4  # DROITE
        else:
            return GameAction.ACTION3  # GAUCHE

# ============================================================================
# BOUCLE PRINCIPALE
# ============================================================================

print(f"{'='*80}")
print(f"EXÉCUTION STRATÉGIE INTELLIGENTE")
print(f"{'='*80}\n")

history = []
game_over_indices = []
prev_positions = deque(maxlen=10)
prev_levels_completed = 0

if hasattr(obs, 'levels_completed'):
    prev_levels_completed = obs.levels_completed

for step in range(1, MAX_STEPS + 1):
    # Choisir action intelligente
    frame = obs.frame if hasattr(obs, 'frame') else None
    action = choose_smart_action(frame, prev_positions, step)
    action_name = action.name
    
    # Afficher progression
    if step % 50 == 0:
        progress = int((step / MAX_STEPS) * 100)
        bar = "█" * (progress // 10) + "░" * (10 - progress // 10)
        print(f"[{bar}] {progress:3d}% | Step {step}/{MAX_STEPS}")
    
    try:
        # Exécuter action
        obs = env.step(action)
        
        # Extraire position agent
        agent_pos = extract_agent_position(obs.frame if hasattr(obs, 'frame') else None)
        if agent_pos:
            prev_positions.append(agent_pos)
        
        # Extraire état
        state_name = obs.state.name if hasattr(obs, 'state') else "UNKNOWN"
        levels_completed = obs.levels_completed if hasattr(obs, 'levels_completed') else 0
        
        # Détecter GAME_OVER
        if state_name == "GAME_OVER":
            game_over_indices.append(step - 1)
            print(f"\n🚨 GAME_OVER DÉTECTÉ au step {step}!")
            print(f"   levels_completed: {prev_levels_completed} → {levels_completed}")
            
            if levels_completed > prev_levels_completed:
                print(f"   ✅ VICTOIRE DÉTECTÉE!")
                
                # Logging forensique
                log_file = MAGEN_ROOT / "results" / "game_over_victories.jsonl"
                log_file.parent.mkdir(parents=True, exist_ok=True)
                with open(log_file, 'a') as f:
                    json.dump({
                        "timestamp": datetime.now().isoformat(),
                        "step": step,
                        "type": "VICTORY",
                        "level_before": prev_levels_completed,
                        "level_after": levels_completed,
                        "strategy": "smart_navigation"
                    }, f)
                    f.write('\n')
            else:
                print(f"   ❌ DÉFAITE DÉTECTÉE!")
                
                # Logging forensique
                log_file = MAGEN_ROOT / "results" / "game_over_defeats.jsonl"
                log_file.parent.mkdir(parents=True, exist_ok=True)
                with open(log_file, 'a') as f:
                    json.dump({
                        "timestamp": datetime.now().isoformat(),
                        "step": step,
                        "type": "DEFEAT",
                        "level": prev_levels_completed,
                        "strategy": "smart_navigation"
                    }, f)
                    f.write('\n')
            
            # ARRÊTER après premier GAME_OVER
            print(f"\n✅ OBJECTIF ATTEINT: GAME_OVER observé au step {step}")
            break
        
        # Enregistrer
        entry = {
            "step": step,
            "action": action_name,
            "state": state_name,
            "levels_completed": levels_completed,
            "agent_pos": agent_pos
        }
        history.append(entry)
        
        # Mettre à jour prev_levels
        prev_levels_completed = levels_completed
        
        time.sleep(0.05)
        
    except Exception as e:
        print(f"\n❌ Erreur step {step}: {e}")
        break

print(f"\n{'='*80}")
print("RÉSULTATS FINAUX")
print(f"{'='*80}\n")

total_steps = len(history)
game_overs = len(game_over_indices)

print(f"Total steps: {total_steps}")
print(f"GAME_OVER détectés: {game_overs}")

if game_overs > 0:
    print(f"\n✅ SUCCÈS: Hypothèse reward externe VALIDABLE")
    print(f"   GAME_OVER atteint en {game_over_indices[0]+1} steps")
else:
    print(f"\n⚠️  Stratégie intelligente insuffisante")
    print(f"   {total_steps} steps sans GAME_OVER")

# Sauvegarder historique
history_file = MAGEN_ROOT / "results" / f"smart_strategy_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
with open(history_file, 'w') as f:
    json.dump({
        "timestamp": datetime.now().isoformat(),
        "game_id": TARGET_GAME,
        "scorecard_id": scorecard_id,
        "total_steps": total_steps,
        "game_overs": game_overs,
        "strategy": "smart_navigation",
        "history": history
    }, f, indent=2)

print(f"\n✅ Historique sauvegardé: {history_file.name}")
print(f"\n{'='*80}")
print("FIN STRATÉGIE INTELLIGENTE")
print(f"{'='*80}\n")

# Made with Bob