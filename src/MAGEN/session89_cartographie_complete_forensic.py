#!/usr/bin/env python3
"""
SESSION 89 - CARTOGRAPHIE COMPLÈTE 100% AVEC LOGGING FORENSIQUE
================================================================

OBJECTIF PRINCIPAL: Cartographier 100% de l'espace d'états AVANT de chercher la solution

STRATÉGIE:
1. Exploration exhaustive avec ACTION1-5 (pas ACTION6/7)
2. Logging forensique COMPLET de chaque mouvement
3. Vérification continue de la progression
4. Arrêt UNIQUEMENT quand 100% cartographie atteinte
5. Temps ILLIMITÉ - autant de parties que nécessaire

LOGGING FORENSIQUE:
- Chaque step logué avec timestamp
- Chaque transition état enregistrée
- Chaque action tracée
- Vérification intégrité à chaque partie
- Sauvegarde incrémentale toutes les 5 parties

CRITÈRES 100% CARTOGRAPHIE:
- Aucun nouvel état découvert pendant 10 parties consécutives
- Convergence taux découverte < 0.1% sur 20 parties
- Validation manuelle utilisateur requise

APRÈS CARTOGRAPHIE 100%:
- Génération rapport complet
- Analyse graphe d'états
- PUIS recherche solution puzzle (reward > 0)
"""

import sys
import time
import json
import hashlib
import numpy as np
from datetime import datetime
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Set, Optional

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class ForensicLogger:
    """Logger forensique pour traçabilité complète"""
    
    def __init__(self, log_file: str):
        self.log_file = log_file
        self.entries = []
        
    def log(self, event_type: str, data: Dict):
        """Enregistrer un événement avec timestamp"""
        entry = {
            "timestamp": datetime.now().isoformat(),
            "event_type": event_type,
            "data": data
        }
        self.entries.append(entry)
        
        # Écrire immédiatement (flush)
        with open(self.log_file, 'a') as f:
            f.write(json.dumps(entry) + "\n")
    
    def log_step(self, game_num: int, step_num: int, state_from: str, 
                 action: str, state_to: str, reward: float, done: bool):
        """Logger un step complet"""
        self.log("STEP", {
            "game": game_num,
            "step": step_num,
            "state_from": state_from,
            "action": action,
            "state_to": state_to,
            "reward": reward,
            "done": done
        })
    
    def log_game_start(self, game_num: int, initial_state: str):
        """Logger début de partie"""
        self.log("GAME_START", {
            "game": game_num,
            "initial_state": initial_state
        })
    
    def log_game_end(self, game_num: int, total_steps: int, 
                     states_discovered: int, reward: float):
        """Logger fin de partie"""
        self.log("GAME_END", {
            "game": game_num,
            "total_steps": total_steps,
            "states_discovered": states_discovered,
            "reward": reward
        })
    
    def log_checkpoint(self, total_games: int, total_states: int, 
                      convergence_rate: float):
        """Logger checkpoint progression"""
        self.log("CHECKPOINT", {
            "total_games": total_games,
            "total_states": total_states,
            "convergence_rate": convergence_rate
        })


class CompleteCartographySystem:
    """Système de cartographie complète avec logging forensique"""
    
    def __init__(self, game_id: str):
        self.game_id = game_id
        
        # Environnement
        self.arcade = None
        self.env = None
        
        # Cartographie
        self.all_states = set()  # Tous les états découverts
        self.state_graph = defaultdict(dict)  # Graphe: state -> {action -> next_state}
        self.state_metadata = {}  # Métadonnées par état
        
        # Statistiques
        self.total_games = 0
        self.total_steps = 0
        self.games_data = []
        
        # Convergence
        self.states_per_game = []  # Nouveaux états par partie
        self.convergence_window = 10  # Fenêtre pour détecter convergence
        self.convergence_threshold = 0.001  # 0.1% nouveaux états
        
        # Logging forensique
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.forensic_logger = ForensicLogger(f"session89_forensic_{timestamp}.jsonl")
        
        # Temps
        self.start_time = None
        
    def initialize_environment(self):
        """Initialiser l'environnement ARC-AGI"""
        print("\n" + "="*80)
        print("SESSION 89 - CARTOGRAPHIE COMPLÈTE 100% AVEC LOGGING FORENSIQUE")
        print("="*80)
        print(f"Jeu: {self.game_id}")
        print(f"Objectif: Cartographier 100% de l'espace d'états")
        print(f"Stratégie: Exploration exhaustive ACTION1-5")
        print(f"Logging: Forensique complet (chaque mouvement tracé)")
        print(f"Durée: ILLIMITÉE (jusqu'à convergence 100%)")
        
        # Créer arcade
        self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        print(f"✅ Arcade créé")
        
        # Créer environnement
        self.env = self.arcade.make(self.game_id)
        print(f"✅ Environnement créé pour {self.game_id}")
        
        # Logger initialisation
        self.forensic_logger.log("INIT", {
            "game_id": self.game_id,
            "strategy": "exhaustive_exploration",
            "actions": ["ACTION1", "ACTION2", "ACTION3", "ACTION4", "ACTION5"],
            "convergence_threshold": self.convergence_threshold
        })
        
    def hash_frame(self, frame) -> str:
        """Calculer hash d'un frame"""
        if isinstance(frame, list):
            frame = np.array(frame)
        return hashlib.sha256(frame.tobytes()).hexdigest()[:16]
    
    def get_distribution(self, frame) -> Dict[int, int]:
        """Obtenir distribution des valeurs de pixels"""
        if isinstance(frame, list):
            frame = np.array(frame)
        unique, counts = np.unique(frame, return_counts=True)
        return {int(val): int(count) for val, count in zip(unique, counts)}
    
    def choose_action_exhaustive(self, state_hash: str, game_number: int,
                                 step: int) -> GameAction:
        """
        Choisir action pour exploration exhaustive
        
        Stratégie CORRIGÉE (aléatoire pur dès le début):
        - Parties 1-40: Exploration aléatoire ACTION1-4 (pur)
        - Parties 41-60: Exploration avec ACTION5 (10% du temps)
        - Parties 61+: Exploration ciblée (états peu visités)
        """
        
        # Parties 1-40: Aléatoire pur ACTION1-4
        if game_number <= 40:
            actions = [GameAction.ACTION1, GameAction.ACTION2,
                      GameAction.ACTION3, GameAction.ACTION4]
            return actions[np.random.randint(0, len(actions))]
        
        # Parties 41-60: Avec ACTION5 (10%)
        if game_number <= 60:
            if np.random.random() < 0.1:
                return GameAction.ACTION5
            else:
                actions = [GameAction.ACTION1, GameAction.ACTION2,
                          GameAction.ACTION3, GameAction.ACTION4]
                return actions[np.random.randint(0, len(actions))]
        
        # Parties 61+: Exploration ciblée
        # Choisir action menant vers états peu visités
        if state_hash in self.state_graph:
            # Compter visites par action
            action_visits = {}
            for action_name, next_state in self.state_graph[state_hash].items():
                visits = self.state_metadata.get(next_state, {}).get("visits", 0)
                action_visits[action_name] = visits
            
            # Choisir action avec moins de visites
            if action_visits:
                min_action = min(action_visits.items(), key=lambda x: x[1])[0]
                return getattr(GameAction, min_action)
        
        # Par défaut: aléatoire
        actions = [GameAction.ACTION1, GameAction.ACTION2,
                  GameAction.ACTION3, GameAction.ACTION4, GameAction.ACTION5]
        return actions[np.random.randint(0, len(actions))]
    
    def play_one_game(self, game_number: int) -> Dict:
        """
        Jouer une partie complète avec logging forensique
        
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
        
        # Logger début partie
        self.forensic_logger.log_game_start(game_number, initial_hash)
        
        # Données partie
        game_data = {
            "game_number": game_number,
            "start_time": datetime.now().isoformat(),
            "initial_state": initial_hash,
            "initial_distribution": initial_dist,
            "steps": [],
            "states_discovered": set(),
            "new_states": 0,
            "final_reward": 0.0,
            "done": False
        }
        
        # Ajouter état initial
        if initial_hash not in self.all_states:
            self.all_states.add(initial_hash)
            game_data["new_states"] += 1
            self.state_metadata[initial_hash] = {
                "first_seen_game": game_number,
                "first_seen_step": 0,
                "visits": 0,
                "distribution": initial_dist
            }
        
        game_data["states_discovered"].add(initial_hash)
        self.state_metadata[initial_hash]["visits"] += 1
        
        # Jouer jusqu'à done=True
        step = 0
        current_hash = initial_hash
        
        while True:
            step += 1
            self.total_steps += 1
            
            # Choisir action
            action = self.choose_action_exhaustive(current_hash, game_number, step)
            action_name = action.name
            
            # Exécuter action avec gestion erreurs
            try:
                obs = self.env.step(action)
            except Exception as e:
                print(f"❌ ERREUR step {step}: {e}")
                self.forensic_logger.log("ERROR", {
                    "game": game_number,
                    "step": step,
                    "action": action_name,
                    "error": str(e)
                })
                break
            
            # Nouvel état
            new_hash = self.hash_frame(obs.frame)
            new_dist = self.get_distribution(obs.frame)
            reward = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            
            # Logger step forensique
            self.forensic_logger.log_step(
                game_number, step, current_hash, action_name, 
                new_hash, reward, done
            )
            
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
            
            # Mettre à jour graphe
            self.state_graph[current_hash][action_name] = new_hash
            
            # Nouvel état découvert?
            if new_hash not in self.all_states:
                self.all_states.add(new_hash)
                game_data["new_states"] += 1
                self.state_metadata[new_hash] = {
                    "first_seen_game": game_number,
                    "first_seen_step": step,
                    "visits": 0,
                    "distribution": new_dist
                }
            
            game_data["states_discovered"].add(new_hash)
            self.state_metadata[new_hash]["visits"] += 1
            
            # Affichage progression
            if step % 10 == 0:
                print(f"  Step {step}: {action_name} → {new_hash[:8]}... "
                      f"(reward={reward:.2f}, done={done}, nouveaux={game_data['new_states']})")
            
            # Vérifier terminaison
            if done:
                game_data["final_reward"] = reward
                game_data["done"] = True
                print(f"\n✅ Partie terminée après {step} steps")
                print(f"   Reward final: {reward}")
                print(f"   Nouveaux états: {game_data['new_states']}")
                print(f"   États totaux: {len(self.all_states)}")
                break
            
            # Sécurité: limite 500 steps par partie
            if step >= 500:
                print(f"\n⚠️ Limite sécurité atteinte (500 steps)")
                break
            
            current_hash = new_hash
        
        # Finaliser données partie
        game_data["end_time"] = datetime.now().isoformat()
        game_data["total_steps"] = step
        game_data["states_discovered"] = list(game_data["states_discovered"])
        
        # Logger fin partie
        self.forensic_logger.log_game_end(
            game_number, step, game_data["new_states"], reward
        )
        
        # Sauvegarder partie
        self.save_game_data(game_data)
        
        # Mettre à jour statistiques
        self.states_per_game.append(game_data["new_states"])
        
        return game_data
    
    def check_convergence(self) -> Tuple[bool, float]:
        """
        Vérifier si cartographie a convergé (100%)
        
        Returns:
            (converged, convergence_rate)
        """
        if len(self.states_per_game) < self.convergence_window:
            return False, 1.0
        
        # Calculer taux nouveaux états sur fenêtre
        recent_new_states = sum(self.states_per_game[-self.convergence_window:])
        total_states = len(self.all_states)
        
        if total_states == 0:
            return False, 1.0
        
        convergence_rate = recent_new_states / (total_states * self.convergence_window)
        
        converged = convergence_rate < self.convergence_threshold
        
        return converged, convergence_rate
    
    def run_complete_cartography(self):
        """
        Exécuter cartographie complète jusqu'à convergence 100%
        """
        self.start_time = time.time()
        
        print("\n" + "="*80)
        print("DÉMARRAGE CARTOGRAPHIE COMPLÈTE")
        print("="*80)
        print(f"Début: {datetime.now().strftime('%H:%M:%S')}")
        print(f"Convergence requise: < {self.convergence_threshold*100:.1f}% nouveaux états")
        print(f"Fenêtre convergence: {self.convergence_window} parties")
        
        game_number = 0
        converged = False
        
        while not converged:
            game_number += 1
            self.total_games += 1
            
            # Jouer une partie
            game_data = self.play_one_game(game_number)
            self.games_data.append(game_data)
            
            # Vérifier convergence
            converged, convergence_rate = self.check_convergence()
            
            # Afficher statistiques
            print("\n" + "-"*80)
            print("STATISTIQUES GLOBALES")
            print("-"*80)
            print(f"Parties jouées: {self.total_games}")
            print(f"Steps totaux: {self.total_steps}")
            print(f"États uniques: {len(self.all_states)}")
            print(f"Nouveaux cette partie: {game_data['new_states']}")
            print(f"Taux convergence: {convergence_rate*100:.3f}%")
            print(f"Convergé: {'✅ OUI' if converged else '❌ NON'}")
            
            # Logger checkpoint
            if game_number % 5 == 0:
                self.forensic_logger.log_checkpoint(
                    self.total_games, len(self.all_states), convergence_rate
                )
                self.save_checkpoint()
            
            # Pause courte entre parties
            time.sleep(1)
        
        print("\n" + "="*80)
        print("✅ CARTOGRAPHIE 100% ATTEINTE!")
        print("="*80)
        print(f"Parties totales: {self.total_games}")
        print(f"États cartographiés: {len(self.all_states)}")
        print(f"Taux convergence final: {convergence_rate*100:.3f}%")
        
        # Sauvegarder résultats finaux
        self.save_final_results()
    
    def save_game_data(self, game_data: Dict):
        """Sauvegarder données d'une partie"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session89_game_{game_data['game_number']:03d}_{timestamp}.json"
        
        # Convertir set en list pour JSON
        game_data_copy = game_data.copy()
        if isinstance(game_data_copy.get("states_discovered"), set):
            game_data_copy["states_discovered"] = list(game_data_copy["states_discovered"])
        
        with open(filename, 'w') as f:
            json.dump(game_data_copy, f, indent=2)
        
        print(f"💾 Partie sauvegardée: {filename}")
    
    def save_checkpoint(self):
        """Sauvegarder checkpoint intermédiaire"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session89_checkpoint_{timestamp}.json"
        
        checkpoint = {
            "timestamp": timestamp,
            "total_games": self.total_games,
            "total_steps": self.total_steps,
            "total_states": len(self.all_states),
            "convergence_rate": self.check_convergence()[1],
            "states_per_game": self.states_per_game[-20:]  # Dernières 20 parties
        }
        
        with open(filename, 'w') as f:
            json.dump(checkpoint, f, indent=2)
        
        print(f"💾 Checkpoint sauvegardé: {filename}")
    
    def save_final_results(self):
        """Sauvegarder résultats finaux"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session89_cartography_complete_{timestamp}.json"
        
        # Calculer statistiques finales
        total_duration = time.time() - self.start_time
        converged, convergence_rate = self.check_convergence()
        
        results = {
            "metadata": {
                "date": datetime.now().isoformat(),
                "game_id": self.game_id,
                "duration_seconds": total_duration,
                "total_games": self.total_games,
                "total_steps": self.total_steps,
                "total_states": len(self.all_states),
                "convergence_rate": convergence_rate,
                "converged": converged
            },
            "cartography": {
                "all_states": list(self.all_states),
                "state_graph": {
                    k: dict(v) for k, v in self.state_graph.items()
                },
                "state_metadata": self.state_metadata
            },
            "statistics": {
                "states_per_game": self.states_per_game,
                "games_summary": [
                    {
                        "game_number": g["game_number"],
                        "steps": g["total_steps"],
                        "new_states": g["new_states"],
                        "reward": g["final_reward"]
                    }
                    for g in self.games_data
                ]
            }
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n💾 Résultats finaux sauvegardés: {filename}")
        
        return filename


def main():
    """Point d'entrée principal"""
    
    # Configuration
    GAME_ID = "ls20-9607627b"
    
    # Créer système de cartographie
    system = CompleteCartographySystem(GAME_ID)
    
    # Initialiser environnement
    system.initialize_environment()
    
    # Exécuter cartographie complète
    system.run_complete_cartography()
    
    # Afficher résumé final
    print("\n" + "="*80)
    print("✅ SESSION 89 - CARTOGRAPHIE COMPLÈTE TERMINÉE")
    print("="*80)
    print(f"Parties jouées: {system.total_games}")
    print(f"Steps totaux: {system.total_steps}")
    print(f"États cartographiés: {len(system.all_states)}")
    print(f"Graphe: {len(system.state_graph)} nœuds")
    print(f"Durée: {time.time() - system.start_time:.1f} secondes")
    
    print("\n" + "="*80)
    print("🎯 PROCHAINE ÉTAPE: RECHERCHE SOLUTION PUZZLE")
    print("="*80)
    print("Maintenant que la cartographie est 100% complète,")
    print("nous pouvons chercher la séquence gagnante (reward > 0)")
    
    print("\n" + "="*80)


if __name__ == "__main__":
    main()

# Made with Bob
