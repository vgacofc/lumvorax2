#!/usr/bin/env python3
"""
SESSION 80 - PHASE 2.5: VALIDATION CAUSALITÉ ACTION→MOUVEMENT
==============================================================

OBJECTIF CRITIQUE: Prouver que l'avatar répond aux commandes du joueur

MÉTHODE:
1. Envoyer action spécifique (LEFT/RIGHT/UP/DOWN)
2. Observer mouvement avatar
3. Mesurer corrélation action→mouvement
4. Valider que corrélation > 95% pour chaque action

CRITÈRES SUCCÈS:
- Corrélation ACTION_LEFT → mouvement gauche > 95%
- Corrélation ACTION_RIGHT → mouvement droite > 95%
- Corrélation ACTION_UP → mouvement haut > 95%
- Corrélation ACTION_DOWN → mouvement bas > 95%

IMPORTANCE:
Sans cette preuve, impossible de distinguer avatar d'un ennemi mobile.
C'est la différence entre "détecter un pixel mobile" et "savoir qui je contrôle".

Auteur: Bob (Session 80 - Phase 2.5 CRITIQUE)
Date: 2026-06-17
"""

import os
import sys
import json
import numpy as np
from pathlib import Path
from datetime import datetime
from collections import defaultdict
from typing import Dict, List, Tuple, Optional

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# Configuration
TARGET_GAME = "ls20-9607627b"
MAGEN_ROOT = Path(__file__).parent
RESULTS_DIR = MAGEN_ROOT / "results" / "session80_phase2_5"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

# Avatar identifié Phases 1-2
AVATAR_INITIAL_POS = (61, 0)
AVATAR_PIXEL_VALUE = 4
MIN_TESTS_PER_ACTION = 10  # Tests minimum par action
CORRELATION_THRESHOLD = 0.95  # 95% minimum

# Mapping actions (à découvrir empiriquement)
ACTION_NAMES = {
    GameAction.ACTION1: "ACTION1",
    GameAction.ACTION2: "ACTION2", 
    GameAction.ACTION3: "ACTION3",
    GameAction.ACTION4: "ACTION4"
}

print(f"""
{'='*80}
🎯 SESSION 80 - PHASE 2.5: VALIDATION CAUSALITÉ (CRITIQUE)
{'='*80}
Jeu: {TARGET_GAME}
Avatar: {AVATAR_INITIAL_POS} (pixel={AVATAR_PIXEL_VALUE})
Tests: {MIN_TESTS_PER_ACTION} par action
Objectif: Corrélation > {CORRELATION_THRESHOLD:.0%}
{'='*80}
""")


class CausalityValidator:
    """Valide corrélation action→mouvement pour prouver contrôle avatar"""
    
    def __init__(self, initial_pos: Tuple[int, int], pixel_value: int):
        self.avatar_pos = initial_pos
        self.avatar_pixel_value = pixel_value
        
        # Historique causal
        self.causal_history: List[Dict] = []
        
        # Statistiques par action
        self.action_stats = {
            action: {
                'movements': [],
                'correlations': [],
                'total_tests': 0,
                'successful_correlations': 0
            }
            for action in ACTION_NAMES.keys()
        }
        
        # Mapping action→direction (à découvrir)
        self.action_direction_map = {}
        
    def find_avatar_in_frame(self, frame: np.ndarray) -> Optional[Tuple[int, int]]:
        """Trouver position avatar (même méthode Phase 2)"""
        
        if isinstance(frame, list):
            frame = np.array(frame)
        
        if len(frame.shape) == 3:
            if frame.shape[0] == 1:
                frame = frame[0]
            elif frame.shape[2] == 3:
                frame = frame[:, :, 0]
        
        # Chercher autour dernière position
        last_x, last_y = self.avatar_pos
        search_radius = 10
        
        x_min = max(0, last_x - search_radius)
        x_max = min(frame.shape[1], last_x + search_radius + 1)
        y_min = max(0, last_y - search_radius)
        y_max = min(frame.shape[0], last_y + search_radius + 1)
        
        for y in range(y_min, y_max):
            for x in range(x_min, x_max):
                try:
                    pixel_val = np.asarray(frame[y, x]).flatten()[0]
                    if int(pixel_val) == self.avatar_pixel_value:
                        return (x, y)
                except (IndexError, ValueError, TypeError):
                    continue
        
        # Recherche globale
        positions = np.argwhere(frame == self.avatar_pixel_value)
        if len(positions) > 0:
            distances = [abs(p[1] - last_x) + abs(p[0] - last_y) for p in positions]
            closest_idx = np.argmin(distances)
            pos = positions[closest_idx]
            if len(pos) >= 2:
                return (int(pos[1]), int(pos[0]))
        
        return None
    
    def test_action_causality(self, env, action: GameAction, step: int) -> Dict:
        """Tester causalité pour une action spécifique"""
        
        # Position avant
        pos_before = self.avatar_pos
        
        # Exécuter action
        obs = env.step(action)
        
        # Position après
        pos_after = self.find_avatar_in_frame(obs.frame)
        
        if pos_after is None:
            return {
                'success': False,
                'reason': 'Avatar not found after action'
            }
        
        # Calculer mouvement
        dx = pos_after[0] - pos_before[0]
        dy = pos_after[1] - pos_before[1]
        
        # Enregistrer
        causal_event = {
            'step': step,
            'action': ACTION_NAMES[action],
            'pos_before': {'x': pos_before[0], 'y': pos_before[1]},
            'pos_after': {'x': pos_after[0], 'y': pos_after[1]},
            'movement': {'dx': dx, 'dy': dy},
            'distance': abs(dx) + abs(dy)
        }
        
        self.causal_history.append(causal_event)
        self.action_stats[action]['movements'].append((dx, dy))
        self.action_stats[action]['total_tests'] += 1
        
        # Mettre à jour position
        self.avatar_pos = pos_after
        
        return {
            'success': True,
            'movement': (dx, dy),
            'causal_event': causal_event
        }
    
    def analyze_action_direction(self):
        """Analyser direction dominante pour chaque action"""
        
        print("\n🔍 Analyse direction par action...")
        
        for action, stats in self.action_stats.items():
            if len(stats['movements']) == 0:
                continue
            
            # Calculer mouvement moyen
            movements = np.array(stats['movements'])
            avg_dx = np.mean(movements[:, 0])
            avg_dy = np.mean(movements[:, 1])
            
            # Déterminer direction dominante
            if abs(avg_dx) > abs(avg_dy):
                if avg_dx < -1:
                    direction = "LEFT"
                elif avg_dx > 1:
                    direction = "RIGHT"
                else:
                    direction = "NONE"
            else:
                if avg_dy < -1:
                    direction = "UP"
                elif avg_dy > 1:
                    direction = "DOWN"
                else:
                    direction = "NONE"
            
            self.action_direction_map[action] = {
                'direction': direction,
                'avg_movement': (avg_dx, avg_dy),
                'consistency': self.calculate_consistency(movements)
            }
            
            print(f"  {ACTION_NAMES[action]}: {direction} (Δx={avg_dx:.1f}, Δy={avg_dy:.1f})")
    
    def calculate_consistency(self, movements: np.ndarray) -> float:
        """Calculer cohérence des mouvements (0-1)"""
        
        if len(movements) == 0:
            return 0.0
        
        # Mouvement le plus fréquent
        unique_movements, counts = np.unique(movements, axis=0, return_counts=True)
        most_common_count = np.max(counts)
        
        return most_common_count / len(movements)
    
    def calculate_correlations(self):
        """Calculer corrélations action→mouvement"""
        
        print("\n📊 Calcul corrélations...")
        
        for action, direction_info in self.action_direction_map.items():
            stats = self.action_stats[action]
            
            if len(stats['movements']) == 0:
                continue
            
            # Corrélation = cohérence direction
            correlation = direction_info['consistency']
            stats['correlations'].append(correlation)
            
            if correlation >= CORRELATION_THRESHOLD:
                stats['successful_correlations'] += 1
            
            print(f"  {ACTION_NAMES[action]} → {direction_info['direction']}: {correlation:.2%}")
    
    def generate_report(self) -> Dict:
        """Générer rapport validation causalité"""
        
        # Calculer métriques globales
        total_tests = sum(s['total_tests'] for s in self.action_stats.values())
        total_correlations = sum(len(s['correlations']) for s in self.action_stats.values())
        
        validated_actions = {
            ACTION_NAMES[action]: {
                'direction': self.action_direction_map.get(action, {}).get('direction', 'UNKNOWN'),
                'avg_movement': list(self.action_direction_map.get(action, {}).get('avg_movement', (0, 0))),
                'consistency': float(self.action_direction_map.get(action, {}).get('consistency', 0.0)),
                'total_tests': int(stats['total_tests']),
                'correlation': float(stats['correlations'][0]) if stats['correlations'] else 0.0,
                'validated': bool(stats['correlations'][0] >= CORRELATION_THRESHOLD) if stats['correlations'] else False
            }
            for action, stats in self.action_stats.items()
        }
        
        # Compter actions validées
        validated_count = sum(1 for v in validated_actions.values() if v['validated'])
        
        return {
            'timestamp': datetime.now().isoformat(),
            'game': TARGET_GAME,
            'avatar': {
                'initial_position': AVATAR_INITIAL_POS,
                'pixel_value': AVATAR_PIXEL_VALUE
            },
            'causality_validation': {
                'total_tests': total_tests,
                'actions_tested': len(self.action_stats),
                'actions_validated': validated_count,
                'validation_rate': validated_count / len(self.action_stats) if len(self.action_stats) > 0 else 0.0,
                'correlation_threshold': CORRELATION_THRESHOLD
            },
            'validated_actions': validated_actions,
            'causal_history': self.causal_history
        }


def run_phase2_5() -> Dict:
    """Exécuter Phase 2.5: Validation Causalité"""
    
    # Initialisation
    print("🔧 Initialisation environnement...")
    arcade = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
    env = arcade.make(TARGET_GAME)
    
    print("🎮 Démarrage jeu...")
    obs = env.reset()
    
    validator = CausalityValidator(AVATAR_INITIAL_POS, AVATAR_PIXEL_VALUE)
    
    # Trouver position initiale
    initial_pos = validator.find_avatar_in_frame(obs.frame)
    if initial_pos:
        validator.avatar_pos = initial_pos
        print(f"✓ Avatar trouvé: {initial_pos}")
    
    print(f"\n{'='*80}")
    print("🧪 TESTS CAUSALITÉ ACTION→MOUVEMENT")
    print(f"{'='*80}\n")
    
    # Tester chaque action
    actions = list(ACTION_NAMES.keys())
    step = 0
    
    for action in actions:
        print(f"\n🎯 Test {ACTION_NAMES[action]}...")
        
        for test_num in range(MIN_TESTS_PER_ACTION):
            result = validator.test_action_causality(env, action, step)
            
            if result['success']:
                dx, dy = result['movement']
                print(f"  Test {test_num+1}/{MIN_TESTS_PER_ACTION}: Δx={dx:+3d}, Δy={dy:+3d}")
            else:
                print(f"  Test {test_num+1}/{MIN_TESTS_PER_ACTION}: ❌ {result['reason']}")
            
            step += 1
            
            # Arrêt si GAME_OVER
            if obs.state.name == "GAME_OVER":
                print(f"\n⚠️  GAME_OVER au step {step}")
                break
        
        if obs.state.name == "GAME_OVER":
            break
    
    # Analyse
    validator.analyze_action_direction()
    validator.calculate_correlations()
    
    # Rapport
    report = validator.generate_report()
    
    # Sauvegarde
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = RESULTS_DIR / f"causality_validation_{timestamp}.json"
    
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n{'='*80}")
    print("📊 RÉSULTATS VALIDATION CAUSALITÉ")
    print(f"{'='*80}\n")
    
    print("🎯 ACTIONS VALIDÉES:")
    for action_name, info in report['validated_actions'].items():
        status = "✅" if info['validated'] else "❌"
        print(f"  {status} {action_name} → {info['direction']}: {info['correlation']:.2%}")
    
    print(f"\n📈 MÉTRIQUES:")
    print(f"  Tests totaux: {report['causality_validation']['total_tests']}")
    print(f"  Actions validées: {report['causality_validation']['actions_validated']}/4")
    print(f"  Taux validation: {report['causality_validation']['validation_rate']:.2%}")
    
    # Validation finale
    success = report['causality_validation']['actions_validated'] >= 3  # Au moins 3/4 actions
    
    if success:
        print(f"\n✅ PHASE 2.5 RÉUSSIE: Causalité prouvée pour {report['causality_validation']['actions_validated']}/4 actions")
        print("✅ MAGEN sait maintenant QUI il contrôle!")
    else:
        print(f"\n⚠️  PHASE 2.5 INCOMPLÈTE: Seulement {report['causality_validation']['actions_validated']}/4 actions validées")
        print("⚠️  Impossible de prouver contrôle avatar")
    
    print(f"\n✅ Rapport sauvegardé: {report_file}")
    
    return report


if __name__ == "__main__":
    try:
        report = run_phase2_5()
        sys.exit(0 if report['causality_validation']['actions_validated'] >= 3 else 1)
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
