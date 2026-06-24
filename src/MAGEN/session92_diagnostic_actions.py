#!/usr/bin/env python3
"""
SESSION 92 - DIAGNOSTIC MÉCANIQUE ACTIONS
==========================================

OBJECTIF: Répondre aux 5 questions critiques AVANT toute stratégie de navigation

Questions:
1. ACTION1-4 sont-elles vraiment différentes? (ou identiques comme Session 81 suggère)
2. Quels pixels changent exactement? (cartographie complète des 52 pixels)
3. Y a-t-il un décalage spatial? (translation/rotation)
4. Pixel 11 = timer/ressource OU spatial? (84→0→20564 à expliquer)
5. Transformation globale ou locale? (concentration des changements)

Méthodologie:
- Tests contrôlés (même état initial)
- Logging forensique détaillé
- Sauvegarde résultats JSON
- Analyse comparative
"""

import sys
import json
import time
from datetime import datetime
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Set
import numpy as np

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class ActionDiagnostic:
    """Diagnostic complet de la mécanique des actions"""
    
    def __init__(self, game_id: str = "ls20-9607627b"):
        self.game_id = game_id
        self.arcade = None
        self.env = None
        self.results = {
            "game_id": game_id,
            "timestamp": datetime.now().isoformat(),
            "questions": {}
        }
        
    def initialize_game(self):
        """Initialise le jeu ARC-AGI"""
        print(f"\n{'='*80}")
        print(f"INITIALISATION JEU: {self.game_id}")
        print(f"{'='*80}\n")
        
        try:
            self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
            print(f"✓ Arcade créé")
            
            self.env = self.arcade.make(self.game_id)
            print(f"✓ Environnement créé pour {self.game_id}")
            return True
        except Exception as e:
            print(f"✗ ERREUR lors du chargement: {e}")
            return False
    
    def extract_frame(self, obs) -> np.ndarray:
        """Extrait la grille 64x64 depuis l'observation"""
        if hasattr(obs, 'frame'):
            frame = obs.frame
            if isinstance(frame, list):
                frame = np.array(frame)
            else:
                frame = np.array(frame)
            
            # Si le frame a 3 dimensions, prendre la première couche
            if frame.ndim == 3:
                frame = frame[0]
            
            # S'assurer que c'est bien 2D
            if frame.ndim != 2:
                print(f"ATTENTION: Frame a {frame.ndim} dimensions, shape={frame.shape}")
                return np.zeros((64, 64), dtype=int)
            
            return frame
        return np.zeros((64, 64), dtype=int)
    
    def get_pixel_distribution(self, frame: np.ndarray) -> Dict[int, int]:
        """Compte la distribution des valeurs de pixels"""
        unique, counts = np.unique(frame, return_counts=True)
        return dict(zip(unique.tolist(), counts.tolist()))
    
    def compute_frame_diff(self, frame1: np.ndarray, frame2: np.ndarray) -> Dict:
        """Calcule les différences entre deux frames"""
        diff = frame2 - frame1
        changed_positions = np.argwhere(diff != 0)
        
        changes = []
        for pos in changed_positions:
            y, x = int(pos[0]), int(pos[1])
            # Convertir en scalaire Python
            before_val = frame1[y, x]
            after_val = frame2[y, x]
            delta_val = diff[y, x]
            
            # Si c'est un array numpy, extraire la valeur
            if isinstance(before_val, np.ndarray):
                before_val = before_val.flat[0]
            if isinstance(after_val, np.ndarray):
                after_val = after_val.flat[0]
            if isinstance(delta_val, np.ndarray):
                delta_val = delta_val.flat[0]
            
            changes.append({
                "position": [y, x],
                "before": int(before_val),
                "after": int(after_val),
                "delta": int(delta_val)
            })
        
        return {
            "num_changes": len(changes),
            "changes": changes,
            "changed_values": {
                "before": [c["before"] for c in changes],
                "after": [c["after"] for c in changes]
            }
        }
    
    def question1_are_actions_different(self) -> Dict:
        """
        QUESTION 1: ACTION1-4 sont-elles vraiment différentes?
        
        Méthode:
        - Partir du même état initial
        - Exécuter chaque action 10 fois
        - Comparer les signatures (distribution pixels)
        """
        print(f"\n{'='*80}")
        print("QUESTION 1: ACTION1-4 SONT-ELLES DIFFÉRENTES?")
        print(f"{'='*80}\n")
        
        results = {
            "method": "Exécuter chaque action 10 fois depuis le même état initial",
            "tests": []
        }
        
        actions = [GameAction.ACTION1, GameAction.ACTION2, 
                  GameAction.ACTION3, GameAction.ACTION4]
        
        for action_idx, action in enumerate(actions, 1):
            print(f"\n--- Test ACTION{action_idx} ---")
            
            action_signatures = []
            
            for test_num in range(10):
                # Reset le jeu
                obs = self.env.reset()
                frame_before = self.extract_frame(obs)
                dist_before = self.get_pixel_distribution(frame_before)
                
                # Exécute l'action
                obs = self.env.step(action)
                frame_after = self.extract_frame(obs)
                dist_after = self.get_pixel_distribution(frame_after)
                reward = float(obs.levels_completed)
                done = (obs.state != GameState.NOT_FINISHED)
                
                # Calcule la signature (différence de distribution)
                signature = {}
                all_values = set(dist_before.keys()) | set(dist_after.keys())
                for val in all_values:
                    before_count = dist_before.get(val, 0)
                    after_count = dist_after.get(val, 0)
                    if before_count != after_count:
                        signature[val] = after_count - before_count
                
                action_signatures.append(signature)
                
                print(f"  Test {test_num+1}: Signature = {signature}")
            
            # Vérifie si toutes les signatures sont identiques
            first_sig = json.dumps(action_signatures[0], sort_keys=True)
            all_identical = all(
                json.dumps(sig, sort_keys=True) == first_sig 
                for sig in action_signatures
            )
            
            results["tests"].append({
                "action": f"ACTION{action_idx}",
                "signatures": action_signatures,
                "all_identical": all_identical,
                "unique_signatures": len(set(json.dumps(sig, sort_keys=True) for sig in action_signatures))
            })
            
            print(f"  → Toutes identiques: {all_identical}")
            print(f"  → Signatures uniques: {results['tests'][-1]['unique_signatures']}")
        
        # Comparaison inter-actions
        print(f"\n--- Comparaison Inter-Actions ---")
        action1_sig = json.dumps(results["tests"][0]["signatures"][0], sort_keys=True)
        action2_sig = json.dumps(results["tests"][1]["signatures"][0], sort_keys=True)
        action3_sig = json.dumps(results["tests"][2]["signatures"][0], sort_keys=True)
        action4_sig = json.dumps(results["tests"][3]["signatures"][0], sort_keys=True)
        
        results["inter_action_comparison"] = {
            "action1_vs_action2": action1_sig == action2_sig,
            "action1_vs_action3": action1_sig == action3_sig,
            "action1_vs_action4": action1_sig == action4_sig,
            "action2_vs_action3": action2_sig == action3_sig,
            "action2_vs_action4": action2_sig == action4_sig,
            "action3_vs_action4": action3_sig == action4_sig
        }
        
        all_same = all(results["inter_action_comparison"].values())
        results["conclusion"] = "IDENTIQUES" if all_same else "DIFFÉRENTES"
        
        print(f"  ACTION1 vs ACTION2: {results['inter_action_comparison']['action1_vs_action2']}")
        print(f"  ACTION1 vs ACTION3: {results['inter_action_comparison']['action1_vs_action3']}")
        print(f"  ACTION1 vs ACTION4: {results['inter_action_comparison']['action1_vs_action4']}")
        print(f"  ACTION2 vs ACTION3: {results['inter_action_comparison']['action2_vs_action3']}")
        print(f"  ACTION2 vs ACTION4: {results['inter_action_comparison']['action2_vs_action4']}")
        print(f"  ACTION3 vs ACTION4: {results['inter_action_comparison']['action3_vs_action4']}")
        print(f"\n  CONCLUSION: Les actions sont {results['conclusion']}")
        
        self.results["questions"]["q1_actions_different"] = results
        return results
    
    def question2_which_pixels_change(self) -> Dict:
        """
        QUESTION 2: Quels pixels changent exactement?
        
        Méthode:
        - Cartographier les 13 valeurs de pixels (0-12)
        - Pour chaque action, identifier quels pixels changent
        - Analyser les patterns de changement
        """
        print(f"\n{'='*80}")
        print("QUESTION 2: QUELS PIXELS CHANGENT EXACTEMENT?")
        print(f"{'='*80}\n")
        
        results = {
            "method": "Cartographie complète des changements de pixels par action",
            "actions": {}
        }
        
        actions = [GameAction.ACTION1, GameAction.ACTION2, 
                  GameAction.ACTION3, GameAction.ACTION4]
        
        for action_idx, action in enumerate(actions, 1):
            print(f"\n--- Analyse ACTION{action_idx} ---")
            
            # Reset et exécute l'action
            obs = self.env.reset()
            frame_before = self.extract_frame(obs)
            
            obs = self.env.step(action)
            frame_after = self.extract_frame(obs)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Calcule les différences
            diff_info = self.compute_frame_diff(frame_before, frame_after)
            
            # Analyse par valeur de pixel
            changes_by_value = defaultdict(list)
            for change in diff_info["changes"]:
                before_val = change["before"]
                after_val = change["after"]
                changes_by_value[before_val].append({
                    "position": change["position"],
                    "after": after_val
                })
            
            results["actions"][f"action{action_idx}"] = {
                "total_changes": diff_info["num_changes"],
                "changes_by_value": dict(changes_by_value),
                "unique_before_values": sorted(set(c["before"] for c in diff_info["changes"])),
                "unique_after_values": sorted(set(c["after"] for c in diff_info["changes"])),
                "sample_changes": diff_info["changes"][:10]  # Premiers 10 changements
            }
            
            print(f"  Total changements: {diff_info['num_changes']}")
            print(f"  Valeurs avant: {results['actions'][f'action{action_idx}']['unique_before_values']}")
            print(f"  Valeurs après: {results['actions'][f'action{action_idx}']['unique_after_values']}")
        
        self.results["questions"]["q2_pixel_changes"] = results
        return results
    
    def question3_spatial_shift(self) -> Dict:
        """
        QUESTION 3: Y a-t-il un décalage spatial?
        
        Méthode:
        - Comparer frame avant/après pour chaque action
        - Chercher des patterns de translation (-2 à +2 pixels)
        - Identifier rotation ou transformation géométrique
        """
        print(f"\n{'='*80}")
        print("QUESTION 3: Y A-T-IL UN DÉCALAGE SPATIAL?")
        print(f"{'='*80}\n")
        
        results = {
            "method": "Recherche de patterns de translation/rotation",
            "actions": {}
        }
        
        actions = [GameAction.ACTION1, GameAction.ACTION2, 
                  GameAction.ACTION3, GameAction.ACTION4]
        
        for action_idx, action in enumerate(actions, 1):
            print(f"\n--- Analyse ACTION{action_idx} ---")
            
            # Reset et exécute l'action
            obs = self.env.reset()
            frame_before = self.extract_frame(obs)
            
            obs = self.env.step(action)
            frame_after = self.extract_frame(obs)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Teste différents décalages
            best_match = {"shift": (0, 0), "matching_pixels": 0}
            
            for dy in range(-2, 3):
                for dx in range(-2, 3):
                    if dy == 0 and dx == 0:
                        continue
                    
                    # Crée une version décalée
                    shifted = np.zeros_like(frame_before)
                    
                    # Calcule les indices valides
                    src_y_start = max(0, -dy)
                    src_y_end = min(64, 64 - dy)
                    src_x_start = max(0, -dx)
                    src_x_end = min(64, 64 - dx)
                    
                    dst_y_start = max(0, dy)
                    dst_y_end = min(64, 64 + dy)
                    dst_x_start = max(0, dx)
                    dst_x_end = min(64, 64 + dx)
                    
                    # Copie avec décalage
                    shifted[dst_y_start:dst_y_end, dst_x_start:dst_x_end] = \
                        frame_before[src_y_start:src_y_end, src_x_start:src_x_end]
                    
                    # Compte les pixels correspondants
                    matching = np.sum(shifted == frame_after)
                    
                    if matching > best_match["matching_pixels"]:
                        best_match = {
                            "shift": (dy, dx),
                            "matching_pixels": int(matching),
                            "match_percentage": float(matching / (64 * 64) * 100)
                        }
            
            results["actions"][f"action{action_idx}"] = best_match
            
            print(f"  Meilleur décalage: {best_match['shift']}")
            print(f"  Pixels correspondants: {best_match['matching_pixels']} ({best_match['match_percentage']:.2f}%)")
        
        self.results["questions"]["q3_spatial_shift"] = results
        return results
    
    def question4_pixel11_tracking(self) -> Dict:
        """
        QUESTION 4: Pixel 11 = timer/ressource OU spatial?
        
        Méthode:
        - Tracker la valeur du pixel 11 sur 100 steps
        - Analyser son comportement (84→0→20564)
        - Déterminer si c'est un compteur, une ressource, ou une position
        """
        print(f"\n{'='*80}")
        print("QUESTION 4: PIXEL 11 = TIMER/RESSOURCE OU SPATIAL?")
        print(f"{'='*80}\n")
        
        results = {
            "method": "Tracking pixel 11 sur 100 steps avec différentes actions",
            "tracking": []
        }
        
        obs = self.env.reset()
        frame = self.extract_frame(obs)
        dist = self.get_pixel_distribution(frame)
        
        print(f"État initial - Distribution pixel 11: {dist.get(11, 0)}")
        
        actions = [GameAction.ACTION1, GameAction.ACTION2, 
                  GameAction.ACTION3, GameAction.ACTION4]
        
        # Track sur 100 steps avec actions variées
        for step in range(100):
            action = actions[step % 4]  # Alterne ACTION1-4
            
            obs = self.env.step(action)
            frame = self.extract_frame(obs)
            dist = self.get_pixel_distribution(frame)
            
            pixel11_count = dist.get(11, 0)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            results["tracking"].append({
                "step": step + 1,
                "action": f"ACTION{(step % 4) + 1}",
                "pixel11_count": pixel11_count,
                "reward": reward,
                "done": done
            })
            
            if step < 10 or step % 10 == 0:
                print(f"  Step {step+1}: ACTION{(step % 4) + 1} → Pixel 11 count = {pixel11_count}")
            
            if done:
                print(f"  GAME OVER au step {step+1}")
                break
        
        # Analyse du comportement
        counts = [t["pixel11_count"] for t in results["tracking"]]
        results["analysis"] = {
            "min_count": min(counts),
            "max_count": max(counts),
            "mean_count": float(np.mean(counts)),
            "std_count": float(np.std(counts)),
            "is_monotonic_decreasing": all(counts[i] >= counts[i+1] for i in range(len(counts)-1)),
            "is_monotonic_increasing": all(counts[i] <= counts[i+1] for i in range(len(counts)-1)),
            "has_cycles": len(set(counts)) < len(counts) / 2
        }
        
        print(f"\n--- Analyse Pixel 11 ---")
        print(f"  Min: {results['analysis']['min_count']}")
        print(f"  Max: {results['analysis']['max_count']}")
        print(f"  Moyenne: {results['analysis']['mean_count']:.2f}")
        print(f"  Écart-type: {results['analysis']['std_count']:.2f}")
        print(f"  Décroissant monotone: {results['analysis']['is_monotonic_decreasing']}")
        print(f"  Croissant monotone: {results['analysis']['is_monotonic_increasing']}")
        print(f"  A des cycles: {results['analysis']['has_cycles']}")
        
        self.results["questions"]["q4_pixel11_tracking"] = results
        return results
    
    def question5_transformation_type(self) -> Dict:
        """
        QUESTION 5: Transformation globale ou locale?
        
        Méthode:
        - Diviser la grille en quadrants
        - Analyser où se concentrent les changements
        - Déterminer si transformation affecte toute la grille ou zones spécifiques
        """
        print(f"\n{'='*80}")
        print("QUESTION 5: TRANSFORMATION GLOBALE OU LOCALE?")
        print(f"{'='*80}\n")
        
        results = {
            "method": "Analyse par quadrants de la grille 64x64",
            "actions": {}
        }
        
        actions = [GameAction.ACTION1, GameAction.ACTION2, 
                  GameAction.ACTION3, GameAction.ACTION4]
        
        for action_idx, action in enumerate(actions, 1):
            print(f"\n--- Analyse ACTION{action_idx} ---")
            
            # Reset et exécute l'action
            obs = self.env.reset()
            frame_before = self.extract_frame(obs)
            
            obs = self.env.step(action)
            frame_after = self.extract_frame(obs)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Calcule les différences
            diff = frame_after != frame_before
            
            # Analyse par quadrants
            quadrants = {
                "top_left": diff[0:32, 0:32],
                "top_right": diff[0:32, 32:64],
                "bottom_left": diff[32:64, 0:32],
                "bottom_right": diff[32:64, 32:64]
            }
            
            quadrant_changes = {}
            for name, quad in quadrants.items():
                changes = np.sum(quad)
                percentage = float(changes / (32 * 32) * 100)
                quadrant_changes[name] = {
                    "changes": int(changes),
                    "percentage": percentage
                }
            
            # Détermine le type de transformation
            total_changes = np.sum(diff)
            max_quadrant_changes = max(q["changes"] for q in quadrant_changes.values())
            
            is_global = max_quadrant_changes < total_changes * 0.4  # Aucun quadrant ne domine
            is_local = max_quadrant_changes > total_changes * 0.7   # Un quadrant domine
            
            results["actions"][f"action{action_idx}"] = {
                "total_changes": int(total_changes),
                "quadrant_changes": quadrant_changes,
                "transformation_type": "GLOBALE" if is_global else ("LOCALE" if is_local else "MIXTE")
            }
            
            print(f"  Total changements: {total_changes}")
            print(f"  Top-Left: {quadrant_changes['top_left']['changes']} ({quadrant_changes['top_left']['percentage']:.1f}%)")
            print(f"  Top-Right: {quadrant_changes['top_right']['changes']} ({quadrant_changes['top_right']['percentage']:.1f}%)")
            print(f"  Bottom-Left: {quadrant_changes['bottom_left']['changes']} ({quadrant_changes['bottom_left']['percentage']:.1f}%)")
            print(f"  Bottom-Right: {quadrant_changes['bottom_right']['changes']} ({quadrant_changes['bottom_right']['percentage']:.1f}%)")
            print(f"  Type: {results['actions'][f'action{action_idx}']['transformation_type']}")
        
        self.results["questions"]["q5_transformation_type"] = results
        return results
    
    def save_results(self):
        """Sauvegarde les résultats en JSON"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session92_diagnostic_{timestamp}.json"
        
        with open(filename, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n{'='*80}")
        print(f"RÉSULTATS SAUVEGARDÉS: {filename}")
        print(f"{'='*80}\n")
        
        return filename
    
    def run_full_diagnostic(self):
        """Exécute le diagnostic complet"""
        print(f"\n{'#'*80}")
        print(f"# SESSION 92 - DIAGNOSTIC MÉCANIQUE ACTIONS")
        print(f"# Jeu: {self.game_id}")
        print(f"# Timestamp: {self.results['timestamp']}")
        print(f"{'#'*80}\n")
        
        if not self.initialize_game():
            return None
        
        # Exécute les 5 questions
        self.question1_are_actions_different()
        self.question2_which_pixels_change()
        self.question3_spatial_shift()
        self.question4_pixel11_tracking()
        self.question5_transformation_type()
        
        # Sauvegarde les résultats
        filename = self.save_results()
        
        # Affiche le résumé
        self.print_summary()
        
        return filename
    
    def print_summary(self):
        """Affiche un résumé des résultats"""
        print(f"\n{'='*80}")
        print("RÉSUMÉ DES RÉSULTATS")
        print(f"{'='*80}\n")
        
        q1 = self.results["questions"]["q1_actions_different"]
        print(f"Q1: Les actions sont {q1['conclusion']}")
        
        q2 = self.results["questions"]["q2_pixel_changes"]
        total_changes = sum(a["total_changes"] for a in q2["actions"].values())
        print(f"Q2: Total changements pixels = {total_changes}")
        
        q3 = self.results["questions"]["q3_spatial_shift"]
        shifts = [a["shift"] for a in q3["actions"].values()]
        print(f"Q3: Décalages détectés = {shifts}")
        
        q4 = self.results["questions"]["q4_pixel11_tracking"]
        print(f"Q4: Pixel 11 - Min={q4['analysis']['min_count']}, Max={q4['analysis']['max_count']}")
        
        q5 = self.results["questions"]["q5_transformation_type"]
        types = [a["transformation_type"] for a in q5["actions"].values()]
        print(f"Q5: Types transformation = {types}")
        
        print(f"\n{'='*80}\n")


def main():
    """Point d'entrée principal"""
    diagnostic = ActionDiagnostic(game_id="ls20-9607627b")
    filename = diagnostic.run_full_diagnostic()
    
    if filename:
        print(f"✓ Diagnostic terminé avec succès")
        print(f"✓ Résultats dans: {filename}")
        return 0
    else:
        print(f"✗ Échec du diagnostic")
        return 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
