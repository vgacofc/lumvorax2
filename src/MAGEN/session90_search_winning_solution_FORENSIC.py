#!/usr/bin/env python3
"""
SESSION 90 - RECHERCHE SOLUTION PUZZLE ls20-9607627b (AVEC LOGGING FORENSIQUE)
===============================================================================

Objectif: Trouver séquence d'actions menant à reward > 0
Basé sur: Cartographie 100% complète (3253 états, Session 89)

AMÉLIORATIONS:
- Logging forensique COMPLET (JSONL)
- Sauvegarde fichiers JSON par test
- SANS timeout (exécution complète garantie)
- 4 stratégies complètes (10 + 65 + 5 + 50 = 130 tests)
"""

import json
import time
import hashlib
from datetime import datetime
from typing import List, Dict, Tuple
from pathlib import Path
import numpy as np

# Import ARC-AGI
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class ForensicLogger:
    """Logger forensique pour traçabilité complète"""
    
    def __init__(self, log_file: str):
        self.log_file = log_file
        self.entries = []
        
    def log(self, event_type: str, data: Dict):
        """Enregistrer événement"""
        entry = {
            "timestamp": datetime.now().isoformat(),
            "event_type": event_type,
            "data": data
        }
        self.entries.append(entry)
        
        # Écrire immédiatement (flush)
        with open(self.log_file, 'a') as f:
            f.write(json.dumps(entry) + '\n')
    
    def get_stats(self) -> Dict:
        """Statistiques logging"""
        return {
            "total_entries": len(self.entries),
            "log_file": self.log_file
        }


class WinningSolutionSearcherForensic:
    """Recherche solution gagnante avec logging forensique"""
    
    def __init__(self, game_id: str = "ls20-9607627b"):
        self.game_id = game_id
        self.arcade = None
        self.env = None
        
        # Logging forensique
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.forensic_logger = ForensicLogger(f"session90_forensic_{timestamp}.jsonl")
        
        # Résultats
        self.tested_sequences = []
        self.best_reward = 0.0
        self.winning_sequences = []
        
        # Statistiques
        self.total_tests = 0
        self.start_time = None
        self.timestamp = timestamp
        
        print(f"\n{'='*80}")
        print(f"SESSION 90 - RECHERCHE SOLUTION PUZZLE (FORENSIC)")
        print(f"{'='*80}")
        print(f"Jeu: {game_id}")
        print(f"Cartographie: 3253 états (Session 89)")
        print(f"Objectif: Trouver reward > 0")
        print(f"Logging forensique: session90_forensic_{timestamp}.jsonl")
        
        self.forensic_logger.log("session_start", {
            "game_id": game_id,
            "timestamp": timestamp
        })
        
    def initialize(self):
        """Initialiser l'environnement"""
        print("\n🎮 Initialisation environnement...")
        
        self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        print("✅ Arcade créé")
        
        self.env = self.arcade.make(self.game_id)
        print(f"✅ Environnement créé pour {self.game_id}")
        
        self.start_time = time.time()
        
        self.forensic_logger.log("environment_initialized", {
            "game_id": self.game_id
        })
        
    def get_state_hash(self, obs) -> str:
        """Calculer hash d'un état (utilise guid unique)"""
        # Pour ls20, pas de distribution accessible, on utilise guid
        return obs.guid if hasattr(obs, 'guid') else str(hash(str(obs)))
    
    def test_sequence(self, actions: List[GameAction], 
                     description: str = "", 
                     strategy: str = "") -> Tuple[float, bool, int]:
        """
        Tester une séquence d'actions avec logging forensique
        
        Returns:
            (reward_final, done, steps)
        """
        self.total_tests += 1
        test_start = time.time()
        
        print(f"\n[Test {self.total_tests}] {description}")
        
        self.forensic_logger.log("test_start", {
            "test_number": self.total_tests,
            "description": description,
            "strategy": strategy,
            "actions_count": len(actions)
        })
        
        # Reset
        obs = self.env.reset()
        initial_state = self.get_state_hash(obs)
        
        self.forensic_logger.log("game_reset", {
            "test_number": self.total_tests,
            "initial_state": initial_state,
            "initial_guid": obs.guid if hasattr(obs, 'guid') else None
        })
        
        # Exécuter séquence
        reward_final = 0.0
        done = False
        steps = 0
        states_visited = [initial_state]
        
        for i, action in enumerate(actions):
            step_start = time.time()
            
            obs = self.env.step(action)
            reward_final = float(obs.levels_completed)
            done = (obs.state != GameState.NOT_FINISHED)
            steps += 1
            
            state_hash = self.get_state_hash(obs)
            states_visited.append(state_hash)
            
            # Log forensique step
            self.forensic_logger.log("step", {
                "test_number": self.total_tests,
                "step": steps,
                "action": action.name,
                "state_from": states_visited[-2],
                "state_to": state_hash,
                "guid": obs.guid if hasattr(obs, 'guid') else None,
                "reward": reward_final,
                "done": done,
                "duration_ms": (time.time() - step_start) * 1000
            })
            
            if done:
                break
        
        test_duration = time.time() - test_start
        
        # Enregistrer résultat
        result = {
            "test_number": self.total_tests,
            "description": description,
            "strategy": strategy,
            "actions": [a.name for a in actions],
            "actions_count": len(actions),
            "steps": steps,
            "reward": reward_final,
            "done": done,
            "states_visited": states_visited,
            "unique_states": len(set(states_visited)),
            "duration_seconds": test_duration,
            "timestamp": datetime.now().isoformat()
        }
        
        self.tested_sequences.append(result)
        
        # Sauvegarder fichier JSON test
        test_file = f"session90_test_{self.total_tests:03d}_{self.timestamp}.json"
        with open(test_file, 'w') as f:
            json.dump(result, f, indent=2)
        
        self.forensic_logger.log("test_complete", {
            "test_number": self.total_tests,
            "reward": reward_final,
            "done": done,
            "steps": steps,
            "duration_seconds": test_duration,
            "file_saved": test_file
        })
        
        # Si reward > 0, c'est une solution!
        if reward_final > 0:
            print(f"\n🎉 SOLUTION TROUVÉE!")
            print(f"   Reward: {reward_final}")
            print(f"   Steps: {steps}")
            self.winning_sequences.append(result)
            self.best_reward = max(self.best_reward, reward_final)
            
            self.forensic_logger.log("solution_found", {
                "test_number": self.total_tests,
                "reward": reward_final,
                "steps": steps
            })
        else:
            print(f"   Résultat: reward={reward_final:.3f}, done={done}, steps={steps}")
        
        return reward_final, done, steps
    
    def strategy_1_known_patterns(self):
        """Stratégie 1: Tester patterns connus (10 tests)"""
        print(f"\n{'='*80}")
        print("STRATÉGIE 1: PATTERNS CONNUS (10 tests)")
        print(f"{'='*80}")
        
        self.forensic_logger.log("strategy_start", {
            "strategy": "1_known_patterns",
            "tests_planned": 10
        })
        
        patterns = [
            {"name": "Cycle pur ACTION1-4", 
             "actions": [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4] * 33},
            {"name": "ACTION5 × 10 puis cycle", 
             "actions": [GameAction.ACTION5] * 10 + [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4] * 30},
            {"name": "Cycle puis ACTION5 × 10", 
             "actions": [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4] * 30 + [GameAction.ACTION5] * 10},
            {"name": "ACTION5 intercalées (1/5)", 
             "actions": [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4, GameAction.ACTION5] * 26},
            {"name": "Forcer pixels_8=0 rapidement", 
             "actions": [GameAction.ACTION1] * 43 + [GameAction.ACTION2] * 43 + [GameAction.ACTION3] * 43},
            {"name": "ACTION5 × 50 (durée 179)", 
             "actions": [GameAction.ACTION5] * 50 + [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4] * 33},
            {"name": "Alternance ACTION5/ACTION1", 
             "actions": [GameAction.ACTION5, GameAction.ACTION1] * 65},
            {"name": "Cycle inverse ACTION4-1", 
             "actions": [GameAction.ACTION4, GameAction.ACTION3, GameAction.ACTION2, GameAction.ACTION1] * 33},
            {"name": "ACTION5 × 200", 
             "actions": [GameAction.ACTION5] * 200},
            {"name": "Aléatoire × 150", 
             "actions": [np.random.choice([GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]) for _ in range(150)]}
        ]
        
        for pattern in patterns:
            reward, done, steps = self.test_sequence(
                pattern['actions'],
                description=pattern['name'],
                strategy="1_known_patterns"
            )
            
            if reward > 0:
                return True
        
        return False
    
    def strategy_2_systematic_action5(self):
        """Stratégie 2: Exploration systématique ACTION5 (65 tests)"""
        print(f"\n{'='*80}")
        print("STRATÉGIE 2: EXPLORATION SYSTÉMATIQUE ACTION5 (65 tests)")
        print(f"{'='*80}")
        
        self.forensic_logger.log("strategy_start", {
            "strategy": "2_systematic_action5",
            "tests_planned": 65
        })
        
        base_sequence = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4] * 32
        positions = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120]
        quantities = [1, 5, 10, 20, 30]
        
        for pos in positions:
            for qty in quantities:
                sequence = base_sequence[:pos] + [GameAction.ACTION5] * qty + base_sequence[pos:]
                
                reward, done, steps = self.test_sequence(
                    sequence,
                    description=f"ACTION5 × {qty} @ pos {pos}",
                    strategy="2_systematic_action5"
                )
                
                if reward > 0:
                    return True
        
        return False
    
    def strategy_3_critical_paths(self):
        """Stratégie 3: Chemins critiques (5 tests)"""
        print(f"\n{'='*80}")
        print("STRATÉGIE 3: CHEMINS CRITIQUES (5 tests)")
        print(f"{'='*80}")
        
        self.forensic_logger.log("strategy_start", {
            "strategy": "3_critical_paths",
            "tests_planned": 5
        })
        
        critical_sequences = [
            {"name": "pixels_11=0 en premier", "actions": [GameAction.ACTION1] * 42},
            {"name": "pixels_8=0 en premier", "actions": [GameAction.ACTION1] * 129},
            {"name": "Équilibrage compteurs", "actions": [GameAction.ACTION1] * 21 + [GameAction.ACTION5] * 21 + [GameAction.ACTION1] * 108},
            {"name": "Durée maximale", "actions": [GameAction.ACTION5] * 100 + [GameAction.ACTION1] * 129},
            {"name": "Durée minimale", "actions": [GameAction.ACTION1] * 129}
        ]
        
        for seq in critical_sequences:
            reward, done, steps = self.test_sequence(
                seq['actions'],
                description=seq['name'],
                strategy="3_critical_paths"
            )
            
            if reward > 0:
                return True
        
        return False
    
    def strategy_4_random_exploration(self, num_tests: int = 50):
        """Stratégie 4: Exploration aléatoire (50 tests)"""
        print(f"\n{'='*80}")
        print(f"STRATÉGIE 4: EXPLORATION ALÉATOIRE ({num_tests} tests)")
        print(f"{'='*80}")
        
        self.forensic_logger.log("strategy_start", {
            "strategy": "4_random_exploration",
            "tests_planned": num_tests
        })
        
        for i in range(num_tests):
            length = np.random.randint(100, 200)
            actions = []
            for _ in range(length):
                if np.random.random() < 0.2:
                    actions.append(GameAction.ACTION5)
                else:
                    actions.append(np.random.choice([GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]))
            
            reward, done, steps = self.test_sequence(
                actions,
                description=f"Aléatoire #{i+1}",
                strategy="4_random_exploration"
            )
            
            if reward > 0:
                return True
        
        return False
    
    def save_final_results(self):
        """Sauvegarder résultats finaux"""
        filename = f"session90_results_final_{self.timestamp}.json"
        
        results = {
            "metadata": {
                "date": datetime.now().isoformat(),
                "game_id": self.game_id,
                "duration_seconds": time.time() - self.start_time,
                "total_tests": self.total_tests,
                "best_reward": self.best_reward,
                "winning_sequences_found": len(self.winning_sequences),
                "forensic_log": self.forensic_logger.log_file
            },
            "tested_sequences": self.tested_sequences,
            "winning_sequences": self.winning_sequences,
            "forensic_stats": self.forensic_logger.get_stats()
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n💾 Résultats finaux sauvegardés: {filename}")
        return filename
    
    def run(self):
        """Exécuter recherche complète SANS timeout"""
        print(f"\n{'='*80}")
        print("DÉMARRAGE RECHERCHE SOLUTION (SANS TIMEOUT)")
        print(f"{'='*80}")
        print(f"Début: {datetime.now().strftime('%H:%M:%S')}")
        print(f"Tests prévus: 130 (10 + 65 + 5 + 50)")
        
        # Initialiser
        self.initialize()
        
        # Stratégie 1: Patterns connus (10 tests)
        found = self.strategy_1_known_patterns()
        if found:
            print(f"\n✅ Solution trouvée avec Stratégie 1!")
        
        # Stratégie 2: Exploration ACTION5 (65 tests)
        if not found:
            found = self.strategy_2_systematic_action5()
            if found:
                print(f"\n✅ Solution trouvée avec Stratégie 2!")
        
        # Stratégie 3: Chemins critiques (5 tests)
        if not found:
            found = self.strategy_3_critical_paths()
            if found:
                print(f"\n✅ Solution trouvée avec Stratégie 3!")
        
        # Stratégie 4: Exploration aléatoire (50 tests)
        if not found:
            found = self.strategy_4_random_exploration(num_tests=50)
            if found:
                print(f"\n✅ Solution trouvée avec Stratégie 4!")
        
        if not found:
            print(f"\n❌ AUCUNE SOLUTION TROUVÉE")
            print(f"   Tests effectués: {self.total_tests}")
            print(f"   Meilleur reward: {self.best_reward}")
        
        # Sauvegarder résultats
        self.save_final_results()
        
        # Statistiques finales
        duration = time.time() - self.start_time
        print(f"\n{'='*80}")
        print("STATISTIQUES FINALES")
        print(f"{'='*80}")
        print(f"Durée totale: {duration:.1f} secondes ({duration/60:.1f} minutes)")
        print(f"Tests effectués: {self.total_tests}")
        print(f"Solutions trouvées: {len(self.winning_sequences)}")
        print(f"Meilleur reward: {self.best_reward}")
        print(f"Forensic entries: {len(self.forensic_logger.entries)}")
        
        self.forensic_logger.log("session_complete", {
            "duration_seconds": duration,
            "total_tests": self.total_tests,
            "solutions_found": len(self.winning_sequences),
            "best_reward": self.best_reward
        })


def main():
    """Point d'entrée principal"""
    searcher = WinningSolutionSearcherForensic("ls20-9607627b")
    searcher.run()


if __name__ == "__main__":
    main()

# Made with Bob - Session 90 FORENSIC