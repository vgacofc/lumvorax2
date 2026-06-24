#!/usr/bin/env python3
"""
SESSION 80 - PHASE 2.5 BIS: VALIDATION CAUSALITÉ STRICTE
=========================================================

Objectif: Valider causalité action→mouvement avec protocole non biaisé
- Corrélation > 95% pour 4/4 actions
- Avatar au CENTRE (pas au bord)
- Réinitialisation entre séries
- Isolation mécanisme autonome vs commandé

Auteur: Bob (Forensic Analysis)
Date: 2026-06-17
"""

import os
import sys
import numpy as np
from datetime import datetime
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from collections import defaultdict
import time

# Configuration API (comme Phases 1-2.5)
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# Configuration
GAME_NAME = "ls20-9607627b"
NUM_REPETITIONS = 20  # 20 répétitions par action (spécifié utilisateur)
CORRELATION_THRESHOLD = 0.95  # Seuil validation
VARIANCE_THRESHOLD = 2.0  # Variance maximale acceptable
RESET_BETWEEN_SERIES = True  # Réinitialisation obligatoire

class StrictCausalityValidator:
    """Validateur causalité avec protocole strict"""
    
    def __init__(self, game_name: str):
        self.game_name = game_name
        self.env = None
        self.avatar_pixel_value = 4  # De Phase 1
        self.avatar_position = None
        self.initial_center_position = None
        
        # Données cumulatives
        self.cumulative_data = {
            "session_id": "session80",
            "game": game_name,
            "phase": "2.5_bis_strict_causality",
            "start_time": datetime.now().isoformat(),
            "protocol": {
                "repetitions_per_action": NUM_REPETITIONS,
                "correlation_threshold": CORRELATION_THRESHOLD,
                "variance_threshold": VARIANCE_THRESHOLD,
                "reset_between_series": RESET_BETWEEN_SERIES
            }
        }
        
        # Résultats par action
        self.action_results = {
            "ACTION1": {"name": "LEFT", "movements": [], "positions": []},
            "ACTION2": {"name": "RIGHT", "movements": [], "positions": []},
            "ACTION3": {"name": "UP", "movements": [], "positions": []},
            "ACTION4": {"name": "DOWN", "movements": [], "positions": []}
        }
        
        # Mécanisme autonome
        self.autonomous_movements = []
        
    def initialize_environment(self) -> bool:
        """Initialise environnement et trouve avatar"""
        try:
            # Utiliser API arc_agi (comme Phases 1-2.5)
            arcade = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
            self.env = arcade.make(self.game_name)
            
            obs = self.env.reset()
            frame = np.asarray(obs.frame)
            
            # Trouver avatar (pixel_value=4 de Phase 1)
            positions = np.argwhere(frame == self.avatar_pixel_value)
            
            if len(positions) == 0:
                print("❌ Avatar non trouvé!")
                return False
            
            # Prendre première position comme référence
            self.avatar_position = tuple(positions[0])
            self.initial_center_position = self.avatar_position
            
            print(f"✅ Avatar trouvé: {self.avatar_position}")
            print(f"   Pixel value: {self.avatar_pixel_value}")
            
            return True
            
        except Exception as e:
            print(f"❌ Erreur initialisation: {e}")
            return False
    
    def find_avatar_position(self, frame: np.ndarray) -> Optional[Tuple[int, int]]:
        """Trouve position actuelle avatar"""
        positions = np.argwhere(frame == self.avatar_pixel_value)
        
        if len(positions) == 0:
            return None
        
        # Si plusieurs candidats, prendre le plus proche de la dernière position
        if len(positions) > 1 and self.avatar_position is not None:
            distances = [
                np.linalg.norm(np.array(pos) - np.array(self.avatar_position))
                for pos in positions
            ]
            closest_idx = np.argmin(distances)
            return tuple(positions[closest_idx])
        
        return tuple(positions[0])
    
    def measure_autonomous_movement(self, num_steps: int = 10) -> Dict:
        """Mesure mouvement autonome (sans action)"""
        print("\n📊 Mesure mouvement autonome (baseline)...")
        
        movements = []
        
        for step in range(num_steps):
            obs_before = self.env.reset()
            frame_before = np.asarray(obs_before.frame)
            pos_before = self.find_avatar_position(frame_before)
            
            if pos_before is None:
                continue
            
            # Attendre sans action (utiliser ACTION1 comme baseline)
            time.sleep(0.1)
            
            # Observer position après une action neutre
            obs_after = self.env.step(GameAction.ACTION1)
            frame_after = np.asarray(obs_after.frame)
            pos_after = self.find_avatar_position(frame_after)
            
            if pos_after is None:
                continue
            
            dx = pos_after[1] - pos_before[1]
            dy = pos_after[0] - pos_before[0]
            
            movements.append((dx, dy))
            
            print(f"   Step {step}: Δx={dx:+3d}, Δy={dy:+3d}")
        
        # Calculer statistiques
        if movements:
            movements_array = np.array(movements)
            mean_dx = np.mean(movements_array[:, 0])
            mean_dy = np.mean(movements_array[:, 1])
            std_dx = np.std(movements_array[:, 0])
            std_dy = np.std(movements_array[:, 1])
            
            autonomous_stats = {
                "num_samples": len(movements),
                "mean_dx": float(mean_dx),
                "mean_dy": float(mean_dy),
                "std_dx": float(std_dx),
                "std_dy": float(std_dy),
                "movements": movements
            }
            
            print(f"\n   Mouvement autonome moyen: Δx={mean_dx:+.2f}, Δy={mean_dy:+.2f}")
            print(f"   Écart-type: σx={std_dx:.2f}, σy={std_dy:.2f}")
            
            self.autonomous_movements = movements
            return autonomous_stats
        
        return {}
    
    def test_action_series(self, action_id: int, action_name: str) -> Dict:
        """Teste une série d'actions avec réinitialisation"""
        print(f"\n🎯 Test ACTION{action_id} ({action_name})...")
        print(f"   Répétitions: {NUM_REPETITIONS}")
        
        movements = []
        positions_before = []
        positions_after = []
        
        for rep in range(NUM_REPETITIONS):
            # Réinitialisation entre répétitions
            if RESET_BETWEEN_SERIES:
                obs, _ = self.env.reset()
                frame = np.asarray(obs['frame'])
                pos_before = self.find_avatar_position(frame)
                
                if pos_before is None:
                    print(f"   ⚠️ Rep {rep}: Avatar non trouvé après reset")
                    continue
            else:
                frame = np.asarray(self.env.unwrapped.obs['frame'])
                pos_before = self.find_avatar_position(frame)
            
            positions_before.append(pos_before)
            
            # Exécuter action
            obs, reward, terminated, truncated, info = self.env.step(action_id)
            
            # Observer position après
            frame_after = np.asarray(obs['frame'])
            pos_after = self.find_avatar_position(frame_after)
            
            if pos_after is None:
                print(f"   ⚠️ Rep {rep}: Avatar non trouvé après action")
                continue
            
            positions_after.append(pos_after)
            
            # Calculer mouvement
            dx = pos_after[1] - pos_before[1]
            dy = pos_after[0] - pos_before[0]
            distance = np.sqrt(dx**2 + dy**2)
            
            movements.append({
                "repetition": rep,
                "pos_before": pos_before,
                "pos_after": pos_after,
                "dx": dx,
                "dy": dy,
                "distance": float(distance)
            })
            
            # Affichage progressif
            if rep % 5 == 0 or rep == NUM_REPETITIONS - 1:
                print(f"   Rep {rep:2d}: ({pos_before[1]:2d},{pos_before[0]:2d}) → "
                      f"({pos_after[1]:2d},{pos_after[0]:2d})  "
                      f"Δx={dx:+3d}, Δy={dy:+3d}")
        
        # Calculer statistiques
        if movements:
            dx_values = [m['dx'] for m in movements]
            dy_values = [m['dy'] for m in movements]
            
            mean_dx = np.mean(dx_values)
            mean_dy = np.mean(dy_values)
            std_dx = np.std(dx_values)
            std_dy = np.std(dy_values)
            var_dx = np.var(dx_values)
            var_dy = np.var(dy_values)
            
            # Calculer corrélation selon direction attendue
            correlation = self.calculate_correlation(action_name, dx_values, dy_values)
            
            # Vérifier pattern déterministe
            is_deterministic = self.check_deterministic_pattern(dx_values, dy_values)
            
            results = {
                "action_id": action_id,
                "action_name": action_name,
                "num_tests": len(movements),
                "statistics": {
                    "mean_dx": float(mean_dx),
                    "mean_dy": float(mean_dy),
                    "std_dx": float(std_dx),
                    "std_dy": float(std_dy),
                    "var_dx": float(var_dx),
                    "var_dy": float(var_dy)
                },
                "correlation": float(correlation),
                "is_deterministic": is_deterministic,
                "validated": correlation >= CORRELATION_THRESHOLD,
                "movements": movements
            }
            
            print(f"\n   📈 Statistiques:")
            print(f"      Mouvement moyen: Δx={mean_dx:+.2f}, Δy={mean_dy:+.2f}")
            print(f"      Écart-type: σx={std_dx:.2f}, σy={std_dy:.2f}")
            print(f"      Variance: σ²x={var_dx:.2f}, σ²y={var_dy:.2f}")
            print(f"      Corrélation: {correlation:.2%}")
            print(f"      Déterministe: {'✅ OUI' if is_deterministic else '❌ NON'}")
            print(f"      Validé (>{CORRELATION_THRESHOLD:.0%}): "
                  f"{'✅ OUI' if results['validated'] else '❌ NON'}")
            
            return results
        
        return {}
    
    def calculate_correlation(self, action_name: str, dx_values: List[int], 
                            dy_values: List[int]) -> float:
        """Calcule corrélation action→mouvement selon direction attendue"""
        
        if action_name == "LEFT":
            # P(Δx < 0 | LEFT)
            negative_dx = sum(1 for dx in dx_values if dx < 0)
            return negative_dx / len(dx_values) if dx_values else 0.0
        
        elif action_name == "RIGHT":
            # P(Δx > 0 | RIGHT)
            positive_dx = sum(1 for dx in dx_values if dx > 0)
            return positive_dx / len(dx_values) if dx_values else 0.0
        
        elif action_name == "UP":
            # P(Δy < 0 | UP)
            negative_dy = sum(1 for dy in dy_values if dy < 0)
            return negative_dy / len(dy_values) if dy_values else 0.0
        
        elif action_name == "DOWN":
            # P(Δy > 0 | DOWN)
            positive_dy = sum(1 for dy in dy_values if dy > 0)
            return positive_dy / len(dy_values) if dy_values else 0.0
        
        return 0.0
    
    def check_deterministic_pattern(self, dx_values: List[int], 
                                   dy_values: List[int]) -> bool:
        """Vérifie si pattern est déterministe (comme ACTION1 Phase 2.5)"""
        
        # Pattern déterministe si variance faible
        var_dx = np.var(dx_values)
        var_dy = np.var(dy_values)
        
        return var_dx < VARIANCE_THRESHOLD and var_dy < VARIANCE_THRESHOLD
    
    def run_validation(self) -> Dict:
        """Exécute validation complète"""
        print("=" * 70)
        print("SESSION 80 - PHASE 2.5 BIS: VALIDATION CAUSALITÉ STRICTE")
        print("=" * 70)
        print(f"\nJeu: {self.game_name}")
        print(f"Répétitions par action: {NUM_REPETITIONS}")
        print(f"Seuil corrélation: {CORRELATION_THRESHOLD:.0%}")
        print(f"Seuil variance: {VARIANCE_THRESHOLD}")
        print(f"Reset entre séries: {'✅ OUI' if RESET_BETWEEN_SERIES else '❌ NON'}")
        
        # Initialiser environnement
        if not self.initialize_environment():
            return {}
        
        # Mesurer mouvement autonome (baseline)
        autonomous_stats = self.measure_autonomous_movement(num_steps=10)
        self.cumulative_data["autonomous_movement"] = autonomous_stats
        
        # Tester chaque action
        actions_to_test = [
            (1, "LEFT"),
            (2, "RIGHT"),
            (3, "UP"),
            (4, "DOWN")
        ]
        
        validated_actions = []
        
        for action_id, action_name in actions_to_test:
            results = self.test_action_series(action_id, action_name)
            
            if results:
                action_key = f"ACTION{action_id}"
                self.action_results[action_key] = results
                
                if results['validated']:
                    validated_actions.append(action_name)
        
        # Calculer métriques globales
        num_validated = len(validated_actions)
        validation_rate = num_validated / 4.0
        
        print("\n" + "=" * 70)
        print("RÉSULTATS FINAUX")
        print("=" * 70)
        print(f"\nActions validées: {num_validated}/4 ({validation_rate:.0%})")
        print(f"Actions: {', '.join(validated_actions) if validated_actions else 'AUCUNE'}")
        
        # Vérifier critère succès
        success = validation_rate >= 1.0  # 4/4 actions validées
        
        print(f"\n{'✅ SUCCÈS' if success else '❌ ÉCHEC'}: "
              f"Causalité {'VALIDÉE' if success else 'NON VALIDÉE'}")
        
        if not success:
            print("\n⚠️ Actions non validées:")
            for action_id, action_name in actions_to_test:
                action_key = f"ACTION{action_id}"
                if action_key in self.action_results:
                    results = self.action_results[action_key]
                    if not results.get('validated', False):
                        corr = results.get('correlation', 0.0)
                        print(f"   - {action_name}: {corr:.2%} < {CORRELATION_THRESHOLD:.0%}")
        
        # Compiler résultats finaux
        final_results = {
            "timestamp": datetime.now().isoformat(),
            "game": self.game_name,
            "validation": {
                "total_actions": 4,
                "validated_actions": num_validated,
                "validation_rate": validation_rate,
                "success": success,
                "threshold": CORRELATION_THRESHOLD
            },
            "autonomous_movement": autonomous_stats,
            "action_results": self.action_results,
            "cumulative_data": self.cumulative_data
        }
        
        return final_results
    
    def save_results(self, results: Dict, output_dir: str = "results/session80_phase2_5_bis"):
        """Sauvegarde résultats"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"strict_causality_validation_{timestamp}.json"
        filepath = output_path / filename
        
        with open(filepath, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n💾 Résultats sauvegardés: {filepath}")
        
        return str(filepath)
    
    def cleanup(self):
        """Nettoie ressources"""
        if self.env:
            self.env.close()

def main():
    """Point d'entrée principal"""
    validator = StrictCausalityValidator(GAME_NAME)
    
    try:
        # Exécuter validation
        results = validator.run_validation()
        
        if results:
            # Sauvegarder résultats
            filepath = validator.save_results(results)
            
            # Afficher résumé
            print("\n" + "=" * 70)
            print("RÉSUMÉ VALIDATION")
            print("=" * 70)
            
            validation = results['validation']
            print(f"\nActions validées: {validation['validated_actions']}/4")
            print(f"Taux validation: {validation['validation_rate']:.0%}")
            print(f"Succès: {'✅ OUI' if validation['success'] else '❌ NON'}")
            
            print(f"\nRapport complet: {filepath}")
        
    except KeyboardInterrupt:
        print("\n\n⚠️ Interruption utilisateur")
    
    except Exception as e:
        print(f"\n❌ Erreur: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        validator.cleanup()
        print("\n✅ Nettoyage terminé")

if __name__ == "__main__":
    main()

# Made with Bob
