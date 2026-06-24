#!/usr/bin/env python3
"""
SESSION 80 - PHASE 2: TRACKER POSITION AGENT
=============================================

OBJECTIF: Tracker position agent sur 95%+ des steps pour construire historique complet

MÉTHODE:
1. Utiliser avatar identifié Phase 1: position (61, 0), pixel_value=4
2. Tracker position à chaque step
3. Construire historique complet (x, y, step)
4. Valider cohérence spatiale (pas de téléportation)
5. Mesurer couverture (% steps trackés)

CRITÈRES SUCCÈS:
- Position trackée sur 95%+ des steps
- Historique complet sans gaps > 5 steps
- Cohérence spatiale (mouvement ≤ 2 pixels/step)

Auteur: Bob (Session 80 - Phase 2)
Date: 2026-06-17
"""

import os
import sys
import json
import numpy as np
from pathlib import Path
from datetime import datetime
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
RESULTS_DIR = MAGEN_ROOT / "results" / "session80_phase2"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

# Avatar identifié Phase 1
AVATAR_INITIAL_POS = (61, 0)
AVATAR_PIXEL_VALUE = 4
MIN_STEPS = 100  # Minimum steps pour validation
COVERAGE_THRESHOLD = 0.95  # 95% minimum

print(f"""
{'='*80}
🎯 SESSION 80 - PHASE 2: TRACKER POSITION AGENT
{'='*80}
Jeu: {TARGET_GAME}
Avatar: {AVATAR_INITIAL_POS} (pixel={AVATAR_PIXEL_VALUE})
Steps: {MIN_STEPS}+
Objectif: Couverture > {COVERAGE_THRESHOLD:.0%}
{'='*80}
""")


class PositionTracker:
    """Tracker position agent avec historique complet"""
    
    def __init__(self, initial_pos: Tuple[int, int], pixel_value: int):
        self.avatar_pos = initial_pos
        self.avatar_pixel_value = pixel_value
        
        # Historique
        self.position_history: List[Tuple[int, int, int]] = []  # (x, y, step)
        self.tracked_steps: List[int] = []
        self.failed_steps: List[int] = []
        
        # Métriques
        self.total_steps = 0
        self.successful_tracks = 0
        self.teleportations = 0
        self.max_gap = 0
        
    def find_avatar_in_frame(self, frame: np.ndarray, step: int) -> Optional[Tuple[int, int]]:
        """Trouver position avatar dans frame actuelle"""
        
        # Convertir frame en numpy array si nécessaire
        if isinstance(frame, list):
            frame = np.array(frame)
        
        # Chercher pixel avec valeur avatar
        # Frame shape peut être (1, 64, 64) ou (64, 64) ou (64, 64, 3)
        if len(frame.shape) == 3:
            if frame.shape[0] == 1:
                # Shape (1, 64, 64) -> (64, 64)
                frame = frame[0]
            elif frame.shape[2] == 3:
                # Shape (64, 64, 3) RGB -> prendre premier canal
                frame = frame[:, :, 0]
        
        # Chercher autour de la dernière position connue (optimisation)
        last_x, last_y = self.avatar_pos
        search_radius = 5  # Chercher dans rayon 5 pixels
        
        # Zone de recherche
        x_min = max(0, last_x - search_radius)
        x_max = min(frame.shape[1], last_x + search_radius + 1)
        y_min = max(0, last_y - search_radius)
        y_max = min(frame.shape[0], last_y + search_radius + 1)
        
        # Chercher dans zone locale
        for y in range(y_min, y_max):
            for x in range(x_min, x_max):
                try:
                    # Extraire valeur pixel de manière robuste
                    pixel_val = np.asarray(frame[y, x]).flatten()[0]
                    if int(pixel_val) == self.avatar_pixel_value:
                        return (x, y)
                except (IndexError, ValueError, TypeError):
                    continue
        
        # Si pas trouvé localement, chercher dans toute la frame
        positions = np.argwhere(frame == self.avatar_pixel_value)
        if len(positions) > 0:
            # Prendre position la plus proche de la dernière
            distances = [abs(p[1] - last_x) + abs(p[0] - last_y) for p in positions]
            closest_idx = np.argmin(distances)
            pos = positions[closest_idx]
            # Gérer 2D ou 3D
            if len(pos) >= 2:
                y, x = int(pos[0]), int(pos[1])
                return (x, y)
        
        return None
    
    def track_position(self, frame: np.ndarray, step: int) -> bool:
        """Tracker position à un step donné"""
        
        pos = self.find_avatar_in_frame(frame, step)
        
        if pos is not None:
            x, y = pos
            
            # Vérifier cohérence spatiale (pas de téléportation)
            if len(self.position_history) > 0:
                last_x, last_y, _ = self.position_history[-1]
                distance = abs(x - last_x) + abs(y - last_y)
                
                if distance > 2:  # Mouvement > 2 pixels = téléportation
                    self.teleportations += 1
                    print(f"  ⚠️  Téléportation détectée step {step}: ({last_x},{last_y}) → ({x},{y})")
            
            # Enregistrer position
            self.position_history.append((x, y, step))
            self.tracked_steps.append(step)
            self.avatar_pos = (x, y)
            self.successful_tracks += 1
            return True
        else:
            self.failed_steps.append(step)
            return False
    
    def calculate_metrics(self) -> Dict:
        """Calculer métriques de tracking"""
        
        coverage = self.successful_tracks / self.total_steps if self.total_steps > 0 else 0.0
        
        # Calculer gaps (séquences de steps non trackés)
        gaps = []
        if len(self.failed_steps) > 0:
            current_gap = 1
            for i in range(1, len(self.failed_steps)):
                if self.failed_steps[i] == self.failed_steps[i-1] + 1:
                    current_gap += 1
                else:
                    gaps.append(current_gap)
                    current_gap = 1
            gaps.append(current_gap)
        
        self.max_gap = max(gaps) if gaps else 0
        
        return {
            'total_steps': self.total_steps,
            'successful_tracks': self.successful_tracks,
            'failed_tracks': len(self.failed_steps),
            'coverage': coverage,
            'teleportations': self.teleportations,
            'max_gap': self.max_gap,
            'avg_gap': np.mean(gaps) if gaps else 0.0
        }
    
    def get_position_at_step(self, step: int) -> Optional[Tuple[int, int]]:
        """Obtenir position à un step donné"""
        for x, y, s in self.position_history:
            if s == step:
                return (x, y)
        return None
    
    def generate_report(self) -> Dict:
        """Générer rapport de tracking"""
        
        metrics = self.calculate_metrics()
        
        # Statistiques spatiales
        if len(self.position_history) > 0:
            xs = [p[0] for p in self.position_history]
            ys = [p[1] for p in self.position_history]
            
            spatial_stats = {
                'x_min': int(np.min(xs)),
                'x_max': int(np.max(xs)),
                'y_min': int(np.min(ys)),
                'y_max': int(np.max(ys)),
                'x_range': int(np.max(xs) - np.min(xs)),
                'y_range': int(np.max(ys) - np.min(ys)),
                'unique_positions': len(set(zip(xs, ys)))
            }
        else:
            spatial_stats = {}
        
        return {
            'timestamp': datetime.now().isoformat(),
            'game': TARGET_GAME,
            'avatar': {
                'initial_position': AVATAR_INITIAL_POS,
                'pixel_value': AVATAR_PIXEL_VALUE
            },
            'tracking': metrics,
            'spatial_stats': spatial_stats,
            'position_history': [
                {'x': x, 'y': y, 'step': s} 
                for x, y, s in self.position_history
            ]
        }


def run_phase2() -> Dict:
    """Exécuter Phase 2: Tracker Position"""
    
    # Initialisation
    print("🔧 Initialisation environnement...")
    arcade = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
    env = arcade.make(TARGET_GAME)
    
    print("🎮 Démarrage jeu...")
    obs = env.reset()
    
    tracker = PositionTracker(AVATAR_INITIAL_POS, AVATAR_PIXEL_VALUE)
    actions_pool = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
    
    print(f"\n{'='*80}")
    print("📊 TRACKING POSITION AGENT")
    print(f"{'='*80}\n")
    
    # Tracker position initiale
    tracker.track_position(obs.frame, 0)
    tracker.total_steps = 1
    
    # Boucle principale
    for step in range(1, MIN_STEPS + 1):
        # Action aléatoire
        action = np.random.choice(actions_pool)
        obs = env.step(action)
        
        # Tracker position
        success = tracker.track_position(obs.frame, step)
        tracker.total_steps += 1
        
        # Affichage progression
        if step % 20 == 0:
            metrics = tracker.calculate_metrics()
            print(f"Step {step}/{MIN_STEPS} | "
                  f"Couverture: {metrics['coverage']:.1%} | "
                  f"Téléportations: {metrics['teleportations']} | "
                  f"Max gap: {metrics['max_gap']}")
        
        # Arrêt si GAME_OVER
        if obs.state.name == "GAME_OVER":
            print(f"\n⚠️  GAME_OVER au step {step}")
            break
    
    # Rapport final
    report = tracker.generate_report()
    
    # Sauvegarde
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = RESULTS_DIR / f"position_tracking_{timestamp}.json"
    
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n{'='*80}")
    print("📊 RÉSULTATS PHASE 2")
    print(f"{'='*80}\n")
    
    print("📍 TRACKING:")
    print(f"  Steps totaux: {report['tracking']['total_steps']}")
    print(f"  Tracks réussis: {report['tracking']['successful_tracks']}")
    print(f"  Tracks échoués: {report['tracking']['failed_tracks']}")
    print(f"  Couverture: {report['tracking']['coverage']:.2%}")
    
    print("\n🗺️  SPATIAL:")
    if report['spatial_stats']:
        print(f"  Zone explorée: {report['spatial_stats']['x_range']}×{report['spatial_stats']['y_range']} pixels")
        print(f"  Positions uniques: {report['spatial_stats']['unique_positions']}")
        print(f"  X: [{report['spatial_stats']['x_min']}, {report['spatial_stats']['x_max']}]")
        print(f"  Y: [{report['spatial_stats']['y_min']}, {report['spatial_stats']['y_max']}]")
    
    print("\n⚡ QUALITÉ:")
    print(f"  Téléportations: {report['tracking']['teleportations']}")
    print(f"  Gap maximum: {report['tracking']['max_gap']} steps")
    print(f"  Gap moyen: {report['tracking']['avg_gap']:.1f} steps")
    
    # Validation
    success = report['tracking']['coverage'] >= COVERAGE_THRESHOLD
    
    if success:
        print(f"\n✅ PHASE 2 RÉUSSIE: Couverture {report['tracking']['coverage']:.2%} ≥ {COVERAGE_THRESHOLD:.0%}")
    else:
        print(f"\n⚠️  PHASE 2 INCOMPLÈTE: Couverture {report['tracking']['coverage']:.2%} < {COVERAGE_THRESHOLD:.0%}")
    
    print(f"\n✅ Rapport sauvegardé: {report_file}")
    
    return report


if __name__ == "__main__":
    try:
        report = run_phase2()
        sys.exit(0 if report['tracking']['coverage'] >= COVERAGE_THRESHOLD else 1)
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
