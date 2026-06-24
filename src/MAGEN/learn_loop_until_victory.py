#!/usr/bin/env python3
"""
APPRENTISSAGE EN LOOP - DÉCOUVERTE VICTOIRE
============================================

Répète le jeu en boucle jusqu'à:
1. Découvrir pourquoi GAME_OVER (défaite)
2. Découvrir comment WIN (victoire)

Analyse automatique après chaque épisode:
- Patterns avant GAME_OVER
- Différences victoire vs défaite
- Mécaniques jeu découvertes

Auteur: Bob (Session 79 - Apprentissage Loop)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime
from collections import defaultdict
from typing import Dict, List, Any

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# Configuration
TARGET_GAME = "ls20-9607627b"
MAGEN_ROOT = Path(__file__).parent
RESULTS_DIR = MAGEN_ROOT / "results" / "learning_loop"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

MAX_EPISODES = 100  # Nombre max épisodes (augmenté de 50 à 100)
MAX_STEPS_PER_EPISODE = 200  # Steps max par épisode

print(f"""
{'='*80}
🔄 APPRENTISSAGE EN LOOP - DÉCOUVERTE VICTOIRE
{'='*80}
Jeu: {TARGET_GAME}
Episodes max: {MAX_EPISODES}
Steps/episode: {MAX_STEPS_PER_EPISODE}
Objectif: Découvrir pourquoi GAME_OVER et comment WIN
{'='*80}
""")

# ============================================================================
# CLASSE ANALYSEUR
# ============================================================================

class GameAnalyzer:
    """Analyse patterns et découvre mécaniques"""
    
    def __init__(self):
        self.episodes = []
        self.victories = []
        self.defeats = []
        self.patterns_before_gameover = []
        
    def record_episode(self, episode_data: Dict[str, Any]):
        """Enregistrer épisode"""
        self.episodes.append(episode_data)
        
        if episode_data['result'] == 'VICTORY':
            self.victories.append(episode_data)
            print(f"\n🎉 VICTOIRE #{len(self.victories)} enregistrée!")
        elif episode_data['result'] == 'DEFEAT':
            self.defeats.append(episode_data)
            print(f"\n❌ Défaite #{len(self.defeats)} enregistrée")
        
        # Analyser pattern avant GAME_OVER
        if episode_data['game_over_step'] > 0:
            pattern = self.extract_pattern_before_gameover(episode_data)
            self.patterns_before_gameover.append(pattern)
    
    def extract_pattern_before_gameover(self, episode_data: Dict[str, Any]) -> Dict[str, Any]:
        """Extraire pattern 10 steps avant GAME_OVER"""
        go_step = episode_data['game_over_step']
        history = episode_data['history']
        
        # 10 steps avant GAME_OVER
        start_idx = max(0, go_step - 10)
        pattern_steps = history[start_idx:go_step]
        
        # Analyser actions
        actions_sequence = [s['action'] for s in pattern_steps]
        
        # Analyser changements frame
        frame_changes = []
        for i in range(1, len(pattern_steps)):
            if pattern_steps[i].get('frame_hash') != pattern_steps[i-1].get('frame_hash'):
                frame_changes.append(i)
        
        return {
            'result': episode_data['result'],
            'game_over_step': go_step,
            'actions_before': actions_sequence,
            'frame_changes': len(frame_changes),
            'last_position': pattern_steps[-1].get('agent_pos') if pattern_steps else None
        }
    
    def analyze_differences(self):
        """Analyser différences victoires vs défaites"""
        print(f"\n{'='*80}")
        print("ANALYSE DIFFÉRENCES VICTOIRES VS DÉFAITES")
        print(f"{'='*80}\n")
        
        if not self.victories:
            print("⚠️  Aucune victoire pour comparer")
            return
        
        print(f"Victoires: {len(self.victories)}")
        print(f"Défaites: {len(self.defeats)}")
        
        # Comparer durée moyenne
        avg_victory_steps = np.mean([e['game_over_step'] for e in self.victories])
        avg_defeat_steps = np.mean([e['game_over_step'] for e in self.defeats])
        
        print(f"\nDurée moyenne:")
        print(f"  Victoires: {avg_victory_steps:.1f} steps")
        print(f"  Défaites: {avg_defeat_steps:.1f} steps")
        
        # Comparer actions utilisées
        victory_actions = defaultdict(int)
        defeat_actions = defaultdict(int)
        
        for v in self.victories:
            for step in v['history']:
                victory_actions[step['action']] += 1
        
        for d in self.defeats:
            for step in d['history']:
                defeat_actions[step['action']] += 1
        
        print(f"\nActions utilisées:")
        print(f"  Victoires: {dict(victory_actions)}")
        print(f"  Défaites: {dict(defeat_actions)}")
    
    def discover_mechanics(self):
        """Découvrir mécaniques jeu"""
        print(f"\n{'='*80}")
        print("DÉCOUVERTE MÉCANIQUES JEU")
        print(f"{'='*80}\n")
        
        # Analyser patterns avant GAME_OVER
        if self.patterns_before_gameover:
            print(f"Patterns analysés: {len(self.patterns_before_gameover)}")
            
            # Grouper par résultat
            victory_patterns = [p for p in self.patterns_before_gameover if p['result'] == 'VICTORY']
            defeat_patterns = [p for p in self.patterns_before_gameover if p['result'] == 'DEFEAT']
            
            print(f"\nPatterns victoire: {len(victory_patterns)}")
            if victory_patterns:
                for i, p in enumerate(victory_patterns[:3], 1):
                    print(f"  #{i}: {p['actions_before'][-5:]} (dernières 5 actions)")
            
            print(f"\nPatterns défaite: {len(defeat_patterns)}")
            if defeat_patterns:
                for i, p in enumerate(defeat_patterns[:3], 1):
                    print(f"  #{i}: {p['actions_before'][-5:]} (dernières 5 actions)")
    
    def save_analysis(self):
        """Sauvegarder analyse complète"""
        report_file = RESULTS_DIR / f"analysis_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
        with open(report_file, 'w') as f:
            json.dump({
                'timestamp': datetime.now().isoformat(),
                'total_episodes': len(self.episodes),
                'victories': len(self.victories),
                'defeats': len(self.defeats),
                'patterns': self.patterns_before_gameover
            }, f, indent=2)
        
        print(f"\n✅ Analyse sauvegardée: {report_file.name}")

# ============================================================================
# BOUCLE APPRENTISSAGE
# ============================================================================

analyzer = GameAnalyzer()

print(f"\n{'='*80}")
print("DÉMARRAGE BOUCLE APPRENTISSAGE")
print(f"{'='*80}\n")

# Connexion API
arc = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)

for episode in range(1, MAX_EPISODES + 1):
    print(f"\n{'='*80}")
    print(f"ÉPISODE {episode}/{MAX_EPISODES}")
    print(f"{'='*80}\n")
    
    # Créer nouveau scorecard
    scorecard_id = arc.create_scorecard()
    env = arc.make(TARGET_GAME, scorecard_id=scorecard_id, seed=42+episode, save_recording=True)
    
    # Reset
    obs = env.reset()
    time.sleep(5.0)
    
    # Historique épisode
    history = []
    game_over_step = 0
    result = "UNKNOWN"
    
    # Stratégie: Exploration aléatoire variée
    actions_pool = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
    
    for step in range(1, MAX_STEPS_PER_EPISODE + 1):
        # Choisir action aléatoire
        action = np.random.choice(actions_pool)
        
        try:
            # Exécuter
            obs = env.step(action)
            
            # Enregistrer
            entry = {
                'step': step,
                'action': action.name,
                'state': obs.state.name if hasattr(obs, 'state') else "UNKNOWN",
                'levels_completed': obs.levels_completed if hasattr(obs, 'levels_completed') else 0
            }
            history.append(entry)
            
            # Détecter GAME_OVER
            if hasattr(obs, 'state') and obs.state.name == "GAME_OVER":
                game_over_step = step
                
                # Déterminer résultat
                if hasattr(obs, 'levels_completed'):
                    prev_level = history[-2]['levels_completed'] if len(history) > 1 else 0
                    curr_level = obs.levels_completed
                    
                    if curr_level > prev_level:
                        result = "VICTORY"
                        print(f"\n🎉 VICTOIRE au step {step}! Niveau {prev_level} → {curr_level}")
                    else:
                        result = "DEFEAT"
                        print(f"\n❌ Défaite au step {step}")
                
                break
            
            time.sleep(0.05)
            
        except Exception as e:
            print(f"\n⚠️  Erreur step {step}: {e}")
            break
    
    # Enregistrer épisode
    episode_data = {
        'episode': episode,
        'scorecard_id': scorecard_id,
        'game_over_step': game_over_step,
        'result': result,
        'history': history
    }
    analyzer.record_episode(episode_data)
    
    # Afficher progression
    print(f"\nÉpisode {episode} terminé:")
    print(f"  Résultat: {result}")
    print(f"  Steps: {game_over_step}")
    print(f"  Victoires totales: {len(analyzer.victories)}")
    print(f"  Défaites totales: {len(analyzer.defeats)}")
    
    # Analyser après chaque 10 épisodes
    if episode % 10 == 0:
        analyzer.analyze_differences()
        analyzer.discover_mechanics()
    
    # Arrêter si victoire trouvée
    if len(analyzer.victories) >= 1:
        print(f"\n🎉 OBJECTIF ATTEINT: Victoire découverte!")
        break

# ============================================================================
# ANALYSE FINALE
# ============================================================================

print(f"\n{'='*80}")
print("ANALYSE FINALE")
print(f"{'='*80}\n")

analyzer.analyze_differences()
analyzer.discover_mechanics()
analyzer.save_analysis()

print(f"\n{'='*80}")
print(f"FIN APPRENTISSAGE - {len(analyzer.episodes)} épisodes")
print(f"Victoires: {len(analyzer.victories)}")
print(f"Défaites: {len(analyzer.defeats)}")
print(f"{'='*80}\n")

# Made with Bob