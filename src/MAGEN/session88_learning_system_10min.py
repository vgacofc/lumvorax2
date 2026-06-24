#!/usr/bin/env python3
"""
SESSION 88 - SYSTÈME D'APPRENTISSAGE 10 MINUTES (DOUBLE DURÉE)
================================================================

Objectif: Cartographier COMPLÈTEMENT l'espace d'états du jeu
- Durée: 10 minutes (600 secondes) - DOUBLE de Session 87
- Chaque partie jusqu'à arrêt automatique (done=True)
- Pause 5 secondes entre parties
- Sauvegarder chaque partie pour apprentissage progressif
- Compter TOUS les pixels/états mappés
- Améliorer stratégie à chaque partie

Améliorations vs Session 87:
- Durée 2× plus longue (600s vs 300s)
- Stratégies d'exploration améliorées
- Métriques détaillées par pixel
- Analyse évolution temporelle
"""

import sys
import time
import json
import hashlib
import numpy as np
from datetime import datetime
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Set

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class EnhancedLearningSystem:
    """Système d'apprentissage amélioré avec analyse détaillée"""
    
    def __init__(self, game_id: str, duration_seconds: int = 600):
        self.game_id = game_id
        self.duration_seconds = duration_seconds
        self.pause_between_games = 5  # secondes
        
        # Environnement
        self.arcade = None
        self.env = None
        
        # Statistiques globales
        self.total_games = 0
        self.total_steps = 0
        self.total_states_discovered = 0
        self.all_states_seen = set()  # Hash de tous les états vus
        self.all_transitions = []  # Toutes les transitions
        
        # Apprentissage
        self.state_action_rewards = defaultdict(lambda: defaultdict(float))
        self.state_visit_count = defaultdict(int)
        self.action_success_rate = defaultdict(lambda: {"success": 0, "total": 0})
        
        # Analyse pixels détaillée
        self.pixel_evolution = defaultdict(list)  # pixel_value -> [counts over time]
        self.pixel_transitions = defaultdict(int)  # (pixel, from_val, to_val) -> count
        
        # Parties individuelles
        self.games_data = []
        
        # Temps
        self.start_time = None
        self.end_time = None
        
    def initialize_environment(self):
        """Initialiser l'environnement ARC-AGI"""
        print("\n" + "="*80)
        print("INITIALISATION SYSTÈME D'APPRENTISSAGE AMÉLIORÉ")
        print("="*80)
        print(f"Jeu: {self.game_id}")
        print(f"Durée: {self.duration_seconds} secondes ({self.duration_seconds//60} minutes)")
        print(f"Pause entre parties: {self.pause_between_games} secondes")
        print(f"Amélioration: Durée 2× plus longue que Session 87")
        
        # Créer arcade
        self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        print(f"✅ Arcade créé")
        
        # Créer environnement
        self.env = self.arcade.make(self.game_id)
        print(f"✅ Environnement créé pour {self.game_id}")
        
    def hash_frame(self, frame) -> str:
        """Calculer hash d'un frame"""
        # Convertir en numpy array si nécessaire
        if isinstance(frame, list):
            frame = np.array(frame)
        return hashlib.sha256(frame.tobytes()).hexdigest()[:16]
    
    def get_distribution(self, frame) -> Dict[int, int]:
        """Obtenir distribution des valeurs de pixels"""
        if isinstance(frame, list):
            frame = np.array(frame)
        unique, counts = np.unique(frame, return_counts=True)
        return {int(val): int(count) for val, count in zip(unique, counts)}
    
    def analyze_pixel_changes(self, dist_from: Dict, dist_to: Dict):
        """Analyser changements de pixels entre deux états"""
        all_keys = set(dist_from.keys()) | set(dist_to.keys())
        
        for key in all_keys:
            val_from = dist_from.get(key, 0)
            val_to = dist_to.get(key, 0)
            
            if val_from != val_to:
                self.pixel_transitions[(key, val_from, val_to)] += 1
    
    def choose_action(self, state_hash: str, game_number: int, step: int) -> GameAction:
        """
        Choisir action basée sur apprentissage amélioré
        
        Stratégies évolutives:
        - Parties 1-3: Exploration uniforme (cycle ACTION1→2→3→4)
        - Parties 4-7: Exploitation + exploration (80/20)
        - Parties 8+: Exploration ciblée (tester actions rares)
        """
        
        # Parties 1-3: Exploration pure (cycle uniforme)
        if game_number <= 3:
            actions = [GameAction.ACTION1, GameAction.ACTION2, 
                      GameAction.ACTION3, GameAction.ACTION4]
            return actions[step % len(actions)]
        
        # Parties 4-7: Exploitation des connaissances
        if game_number <= 7:
            # Choisir action avec meilleur reward moyen pour cet état
            if state_hash in self.state_action_rewards:
                best_action = None
                best_reward = -float('inf')
                
                for action_name, reward in self.state_action_rewards[state_hash].items():
                    if reward > best_reward:
                        best_reward = reward
                        best_action = action_name
                
                if best_action:
                    # 80% exploitation, 20% exploration
                    if np.random.random() < 0.8:
                        return getattr(GameAction, best_action)
            
            # Exploration: tester actions moins utilisées
            action_counts = {
                "ACTION1": self.action_success_rate["ACTION1"]["total"],
                "ACTION2": self.action_success_rate["ACTION2"]["total"],
                "ACTION3": self.action_success_rate["ACTION3"]["total"],
                "ACTION4": self.action_success_rate["ACTION4"]["total"],
            }
            
            # Choisir action la moins utilisée
            min_action = min(action_counts.items(), key=lambda x: x[1])[0]
            return getattr(GameAction, min_action)
        
        # Parties 8+: Exploration aléatoire pure (ACTION1-4 uniquement)
        # ACTION5-7 causent erreur serveur 500
        all_actions = [
            GameAction.ACTION1, GameAction.ACTION2,
            GameAction.ACTION3, GameAction.ACTION4
        ]
        return all_actions[np.random.randint(0, len(all_actions))]
    
    def play_one_game(self, game_number: int) -> Dict:
        """
        Jouer une partie complète jusqu'à done=True
        
        Returns:
            Données de la partie
        """
        print("\n" + "="*80)
        print(f"PARTIE {game_number}")
        print("="*80)
        
        # Reset environnement
        obs = self.env.reset()
        
        # État initial
        initial_hash = self.hash_frame(obs.frame)
        initial_dist = self.get_distribution(obs.frame)
        
        print(f"État initial: {initial_hash}")
        print(f"Distribution: {initial_dist}")
        
        # Données partie
        game_data = {
            "game_number": game_number,
            "start_time": datetime.now().isoformat(),
            "initial_state": initial_hash,
            "initial_distribution": initial_dist,
            "steps": [],
            "states_discovered": set(),
            "transitions": [],
            "pixel_evolution": defaultdict(list),
            "final_reward": 0.0,
            "done": False
        }
        
        # Ajouter état initial
        game_data["states_discovered"].add(initial_hash)
        self.all_states_seen.add(initial_hash)
        self.state_visit_count[initial_hash] += 1
        
        # Enregistrer distribution initiale
        for pixel_val, count in initial_dist.items():
            game_data["pixel_evolution"][pixel_val].append(count)
        
        # Jouer jusqu'à done=True
        step = 0
        current_hash = initial_hash
        prev_dist = initial_dist.copy()
        
        while True:
            step += 1
            self.total_steps += 1
            
            # Choisir action
            action = self.choose_action(current_hash, game_number, step)
            action_name = action.name
            
            # Exécuter action
            obs = self.env.step(action)
            
            # Nouvel état
            new_hash = self.hash_frame(obs.frame)
            new_dist = self.get_distribution(obs.frame)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Analyser changements pixels
            self.analyze_pixel_changes(prev_dist, new_dist)
            
            # Enregistrer évolution pixels
            for pixel_val, count in new_dist.items():
                game_data["pixel_evolution"][pixel_val].append(count)
            
            # Enregistrer transition
            transition = {
                "step": step,
                "state_from": current_hash,
                "action": action_name,
                "state_to": new_hash,
                "reward": reward,
                "done": done,
                "distribution": new_dist
            }
            
            game_data["steps"].append(transition)
            game_data["transitions"].append(transition)
            self.all_transitions.append(transition)
            
            # Mettre à jour apprentissage
            self.state_action_rewards[current_hash][action_name] = reward
            self.action_success_rate[action_name]["total"] += 1
            if reward > 0:
                self.action_success_rate[action_name]["success"] += 1
            
            # Nouvel état découvert?
            if new_hash not in game_data["states_discovered"]:
                game_data["states_discovered"].add(new_hash)
            
            if new_hash not in self.all_states_seen:
                self.all_states_seen.add(new_hash)
                self.total_states_discovered += 1
            
            self.state_visit_count[new_hash] += 1
            
            # Affichage progression
            if step % 10 == 0:
                print(f"  Step {step}: {action_name} → {new_hash[:8]}... "
                      f"(reward={reward:.2f}, done={done})")
            
            # Vérifier terminaison
            if done:
                game_data["final_reward"] = reward
                game_data["done"] = True
                print(f"\n✅ Partie terminée après {step} steps")
                print(f"   Reward final: {reward}")
                print(f"   États découverts cette partie: {len(game_data['states_discovered'])}")
                break
            
            # Sécurité: limite 500 steps par partie
            if step >= 500:
                print(f"\n⚠️ Limite sécurité atteinte (500 steps)")
                break
            
            current_hash = new_hash
            prev_dist = new_dist.copy()
        
        # Finaliser données partie
        game_data["end_time"] = datetime.now().isoformat()
        game_data["total_steps"] = step
        game_data["states_discovered"] = list(game_data["states_discovered"])
        
        # Convertir pixel_evolution en dict sérialisable
        game_data["pixel_evolution"] = {
            k: v for k, v in game_data["pixel_evolution"].items()
        }
        
        return game_data
    
    def run_learning_session(self):
        """
        Exécuter session d'apprentissage complète (10 minutes)
        """
        self.start_time = time.time()
        
        print("\n" + "="*80)
        print("DÉMARRAGE SESSION D'APPRENTISSAGE 10 MINUTES")
        print("="*80)
        print(f"Durée: {self.duration_seconds} secondes")
        print(f"Début: {datetime.now().strftime('%H:%M:%S')}")
        
        game_number = 0
        
        while True:
            # Vérifier temps écoulé
            elapsed = time.time() - self.start_time
            remaining = self.duration_seconds - elapsed
            
            if remaining <= 0:
                print("\n" + "="*80)
                print("⏰ TEMPS ÉCOULÉ - FIN SESSION")
                print("="*80)
                break
            
            print(f"\n⏱️ Temps restant: {int(remaining)} secondes ({int(remaining/60)}min {int(remaining%60)}s)")
            
            # Jouer une partie
            game_number += 1
            self.total_games += 1
            
            game_data = self.play_one_game(game_number)
            self.games_data.append(game_data)
            
            # Sauvegarder partie immédiatement
            self.save_game_data(game_data)
            
            # Afficher statistiques globales
            print("\n" + "-"*80)
            print("STATISTIQUES GLOBALES")
            print("-"*80)
            print(f"Parties jouées: {self.total_games}")
            print(f"Steps totaux: {self.total_steps}")
            print(f"États uniques découverts: {len(self.all_states_seen)}")
            print(f"Transitions enregistrées: {len(self.all_transitions)}")
            print(f"Progression: {len(self.all_states_seen)/500*100:.1f}% (estimation 500 états max)")
            
            # Vérifier s'il reste assez de temps pour une autre partie
            elapsed = time.time() - self.start_time
            remaining = self.duration_seconds - elapsed
            
            if remaining < 30:  # Moins de 30 secondes
                print(f"\n⚠️ Temps insuffisant pour nouvelle partie ({int(remaining)}s)")
                break
            
            # Pause entre parties
            print(f"\n⏸️ Pause {self.pause_between_games} secondes...")
            time.sleep(self.pause_between_games)
        
        self.end_time = time.time()
        
        # Sauvegarder résultats finaux
        self.save_final_results()
    
    def save_game_data(self, game_data: Dict):
        """Sauvegarder données d'une partie"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session88_game_{game_data['game_number']:03d}_{timestamp}.json"
        
        with open(filename, 'w') as f:
            json.dump(game_data, f, indent=2)
        
        print(f"💾 Partie sauvegardée: {filename}")
    
    def save_final_results(self):
        """Sauvegarder résultats finaux de la session"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session88_learning_results_{timestamp}.json"
        
        # Calculer statistiques finales
        total_duration = self.end_time - self.start_time
        
        # Analyser transitions pixels
        pixel_transition_summary = {}
        for (pixel, val_from, val_to), count in self.pixel_transitions.items():
            key = f"pixel_{pixel}"
            if key not in pixel_transition_summary:
                pixel_transition_summary[key] = []
            pixel_transition_summary[key].append({
                "from": val_from,
                "to": val_to,
                "count": count
            })
        
        results = {
            "metadata": {
                "date": datetime.now().isoformat(),
                "game_id": self.game_id,
                "duration_seconds": total_duration,
                "duration_planned": self.duration_seconds,
                "total_games": self.total_games,
                "total_steps": self.total_steps,
                "total_states_discovered": len(self.all_states_seen),
                "total_transitions": len(self.all_transitions),
                "improvement_vs_session87": {
                    "duration_multiplier": 2.0,
                    "expected_states": "~700-800 (2× Session 87)",
                    "actual_states": len(self.all_states_seen)
                }
            },
            "learning_data": {
                "state_action_rewards": {
                    k: dict(v) for k, v in self.state_action_rewards.items()
                },
                "state_visit_count": dict(self.state_visit_count),
                "action_success_rate": dict(self.action_success_rate)
            },
            "pixel_analysis": {
                "transitions": pixel_transition_summary
            },
            "games_summary": [
                {
                    "game_number": g["game_number"],
                    "steps": g["total_steps"],
                    "states_discovered": len(g["states_discovered"]),
                    "final_reward": g["final_reward"],
                    "done": g["done"]
                }
                for g in self.games_data
            ],
            "all_states": list(self.all_states_seen)
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n💾 Résultats finaux sauvegardés: {filename}")
        
        return filename


def main():
    """Point d'entrée principal"""
    
    # Configuration
    GAME_ID = "ls20-9607627b"
    DURATION = 600  # 10 minutes (DOUBLE de Session 87)
    
    # Créer système d'apprentissage
    system = EnhancedLearningSystem(GAME_ID, DURATION)
    
    # Initialiser environnement
    system.initialize_environment()
    
    # Exécuter session d'apprentissage
    system.run_learning_session()
    
    # Afficher résumé final
    print("\n" + "="*80)
    print("✅ SESSION D'APPRENTISSAGE 10 MINUTES COMPLÉTÉE")
    print("="*80)
    print(f"Parties jouées: {system.total_games}")
    print(f"Steps totaux: {system.total_steps}")
    print(f"États uniques mappés: {len(system.all_states_seen)}")
    print(f"Transitions enregistrées: {len(system.all_transitions)}")
    print(f"Durée réelle: {system.end_time - system.start_time:.1f} secondes")
    
    # Comparaison Session 87
    print("\n" + "-"*80)
    print("COMPARAISON SESSION 87 vs SESSION 88")
    print("-"*80)
    print(f"Session 87: 7 parties, 903 steps, 357 états (5 min)")
    print(f"Session 88: {system.total_games} parties, {system.total_steps} steps, {len(system.all_states_seen)} états (10 min)")
    print(f"Gain états: +{len(system.all_states_seen) - 357} ({(len(system.all_states_seen)/357-1)*100:+.1f}%)")
    
    # Statistiques par action
    print("\n" + "-"*80)
    print("STATISTIQUES PAR ACTION")
    print("-"*80)
    for action_name, stats in system.action_success_rate.items():
        total = stats["total"]
        success = stats["success"]
        rate = (success / total * 100) if total > 0 else 0
        print(f"{action_name}: {total} utilisations, {success} succès ({rate:.1f}%)")
    
    print("\n" + "="*80)


if __name__ == "__main__":
    main()

# Made with Bob
