#!/usr/bin/env python3
"""
SESSION 90 - RECHERCHE SOLUTION PUZZLE ls20-9607627b
=====================================================

Objectif: Trouver séquence d'actions menant à reward > 0
Basé sur: Cartographie 100% complète (3253 états, Session 89)

Stratégies de recherche:
1. Patterns spécifiques (séquences connues)
2. Exploration systématique avec ACTION5
3. Chemins critiques identifiés
4. Combinaisons rares
"""

import json
import time
import hashlib
from datetime import datetime
from typing import List, Dict, Tuple, Set
import numpy as np

# Import ARC-AGI
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class WinningSolutionSearcher:
    """Recherche solution gagnante pour ls20-9607627b"""
    
    def __init__(self, game_id: str = "ls20-9607627b"):
        self.game_id = game_id
        self.arcade = None
        self.env = None
        
        # Résultats
        self.tested_sequences = []
        self.best_reward = 0.0
        self.winning_sequences = []
        
        # Statistiques
        self.total_tests = 0
        self.start_time = None
        
        print(f"\n{'='*80}")
        print(f"SESSION 90 - RECHERCHE SOLUTION PUZZLE")
        print(f"{'='*80}")
        print(f"Jeu: {game_id}")
        print(f"Cartographie: 3253 états (Session 89)")
        print(f"Objectif: Trouver reward > 0")
        
    def initialize(self):
        """Initialiser l'environnement"""
        print("\n🎮 Initialisation environnement...")
        
        self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
        print("✅ Arcade créé")
        
        self.env = self.arcade.make(self.game_id)
        print(f"✅ Environnement créé pour {self.game_id}")
        
        self.start_time = time.time()
        
    def get_state_hash(self, state: GameState) -> str:
        """Calculer hash d'un état"""
        state_str = json.dumps(state.distribution, sort_keys=True)
        return hashlib.md5(state_str.encode()).hexdigest()[:16]
    
    def test_sequence(self, actions: List[GameAction], 
                     description: str = "") -> Tuple[float, bool, int]:
        """
        Tester une séquence d'actions
        
        Returns:
            (reward_final, done, steps)
        """
        self.total_tests += 1
        
        # Reset
        obs = self.env.reset()
        
        # Exécuter séquence
        reward_final = 0.0
        done = False
        steps = 0
        
        for action in actions:
            obs = self.env.step(action)
            reward_final = float(obs.levels_completed)  # Reward = niveaux complétés
            done = (obs.state != GameState.NOT_FINISHED)  # Done si état != NOT_FINISHED
            steps += 1
            
            if done:
                break
        
        # Enregistrer résultat
        result = {
            "test_number": self.total_tests,
            "description": description,
            "actions": [a.name for a in actions],
            "steps": steps,
            "reward": reward_final,
            "done": done,
            "timestamp": datetime.now().isoformat()
        }
        
        self.tested_sequences.append(result)
        
        # Si reward > 0, c'est une solution!
        if reward_final > 0:
            print(f"\n🎉 SOLUTION TROUVÉE!")
            print(f"   Séquence: {[a.name for a in actions]}")
            print(f"   Reward: {reward_final}")
            print(f"   Steps: {steps}")
            self.winning_sequences.append(result)
            self.best_reward = max(self.best_reward, reward_final)
        
        return reward_final, done, steps
    
    def strategy_1_known_patterns(self):
        """
        Stratégie 1: Tester patterns connus
        
        Basé sur analyse Session 89:
        - Durée = 129 + k (k = nombre ACTION5)
        - Terminaison: pixels_11=0 ET pixels_8=0
        - ACTION5 gèle compteur c8
        """
        print(f"\n{'='*80}")
        print("STRATÉGIE 1: PATTERNS CONNUS")
        print(f"{'='*80}")
        
        patterns = [
            # Pattern 1: Cycle pur (baseline)
            {
                "name": "Cycle pur ACTION1-4",
                "actions": [GameAction.ACTION1, GameAction.ACTION2, 
                           GameAction.ACTION3, GameAction.ACTION4] * 33  # 132 actions
            },
            
            # Pattern 2: ACTION5 au début
            {
                "name": "ACTION5 × 10 puis cycle",
                "actions": [GameAction.ACTION5] * 10 + 
                          [GameAction.ACTION1, GameAction.ACTION2, 
                           GameAction.ACTION3, GameAction.ACTION4] * 30
            },
            
            # Pattern 3: ACTION5 à la fin
            {
                "name": "Cycle puis ACTION5 × 10",
                "actions": [GameAction.ACTION1, GameAction.ACTION2, 
                           GameAction.ACTION3, GameAction.ACTION4] * 30 +
                          [GameAction.ACTION5] * 10
            },
            
            # Pattern 4: ACTION5 intercalées
            {
                "name": "ACTION5 intercalées (1 sur 5)",
                "actions": ([GameAction.ACTION1, GameAction.ACTION2, 
                            GameAction.ACTION3, GameAction.ACTION4, 
                            GameAction.ACTION5] * 26)
            },
            
            # Pattern 5: Séquence spéciale (pixels_8 = 0 avant pixels_11)
            {
                "name": "Forcer pixels_8=0 rapidement",
                "actions": [GameAction.ACTION1] * 43 +  # Atteindre c8=43 (pixels_8=8)
                          [GameAction.ACTION2] * 43 +  # c8=86 (pixels_8=4)
                          [GameAction.ACTION3] * 43    # c8=129 (pixels_8=0)
            },
            
            # Pattern 6: ACTION5 pour prolonger
            {
                "name": "ACTION5 × 50 (durée 179)",
                "actions": [GameAction.ACTION5] * 50 +
                          [GameAction.ACTION1, GameAction.ACTION2, 
                           GameAction.ACTION3, GameAction.ACTION4] * 33
            },
            
            # Pattern 7: Alternance ACTION5/autres
            {
                "name": "Alternance ACTION5/ACTION1",
                "actions": ([GameAction.ACTION5, GameAction.ACTION1] * 65)
            },
            
            # Pattern 8: Séquence inverse
            {
                "name": "Cycle inverse ACTION4-1",
                "actions": [GameAction.ACTION4, GameAction.ACTION3, 
                           GameAction.ACTION2, GameAction.ACTION1] * 33
            },
            
            # Pattern 9: ACTION5 uniquement
            {
                "name": "ACTION5 × 200",
                "actions": [GameAction.ACTION5] * 200
            },
            
            # Pattern 10: Séquence aléatoire longue
            {
                "name": "Aléatoire × 150",
                "actions": [np.random.choice([GameAction.ACTION1, GameAction.ACTION2,
                                             GameAction.ACTION3, GameAction.ACTION4])
                           for _ in range(150)]
            }
        ]
        
        for i, pattern in enumerate(patterns, 1):
            print(f"\n[{i}/10] Test: {pattern['name']}")
            print(f"        Actions: {len(pattern['actions'])}")
            
            reward, done, steps = self.test_sequence(
                pattern['actions'],
                description=pattern['name']
            )
            
            print(f"        Résultat: reward={reward:.3f}, done={done}, steps={steps}")
            
            if reward > 0:
                print(f"        ✅ SOLUTION TROUVÉE!")
                return True
        
        print(f"\n❌ Aucune solution trouvée avec patterns connus")
        return False
    
    def strategy_2_systematic_action5(self):
        """
        Stratégie 2: Exploration systématique avec ACTION5
        
        Tester différentes positions et quantités d'ACTION5
        """
        print(f"\n{'='*80}")
        print("STRATÉGIE 2: EXPLORATION SYSTÉMATIQUE ACTION5")
        print(f"{'='*80}")
        
        # Tester ACTION5 à différentes positions
        base_sequence = [GameAction.ACTION1, GameAction.ACTION2, 
                        GameAction.ACTION3, GameAction.ACTION4] * 32  # 128 actions
        
        positions = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120]
        quantities = [1, 5, 10, 20, 30]
        
        test_count = 0
        for pos in positions:
            for qty in quantities:
                test_count += 1
                
                # Insérer ACTION5 à position
                sequence = base_sequence[:pos] + [GameAction.ACTION5] * qty + base_sequence[pos:]
                
                print(f"\n[{test_count}/{len(positions)*len(quantities)}] " +
                      f"ACTION5 × {qty} à position {pos}")
                
                reward, done, steps = self.test_sequence(
                    sequence,
                    description=f"ACTION5 × {qty} @ pos {pos}"
                )
                
                print(f"        Résultat: reward={reward:.3f}, done={done}, steps={steps}")
                
                if reward > 0:
                    print(f"        ✅ SOLUTION TROUVÉE!")
                    return True
        
        print(f"\n❌ Aucune solution trouvée avec exploration ACTION5")
        return False
    
    def strategy_3_critical_paths(self):
        """
        Stratégie 3: Chemins critiques
        
        Tester séquences menant à états rares ou critiques
        """
        print(f"\n{'='*80}")
        print("STRATÉGIE 3: CHEMINS CRITIQUES")
        print(f"{'='*80}")
        
        # Séquences ciblant états spécifiques
        critical_sequences = [
            # Séquence 1: Atteindre pixels_11=0 en premier
            {
                "name": "pixels_11=0 en premier (42 steps)",
                "actions": [GameAction.ACTION1] * 42
            },
            
            # Séquence 2: Atteindre pixels_8=0 en premier
            {
                "name": "pixels_8=0 en premier (129 steps)",
                "actions": [GameAction.ACTION1] * 129
            },
            
            # Séquence 3: Équilibrer les deux compteurs
            {
                "name": "Équilibrage compteurs",
                "actions": ([GameAction.ACTION1] * 21 +  # t=21, c8=21
                           [GameAction.ACTION5] * 21 +  # t=42, c8=21 (pixels_11=0)
                           [GameAction.ACTION1] * 108)  # t=150, c8=129 (pixels_8=0)
            },
            
            # Séquence 4: Maximiser durée
            {
                "name": "Durée maximale (ACTION5 × 100)",
                "actions": [GameAction.ACTION5] * 100 +
                          [GameAction.ACTION1] * 129
            },
            
            # Séquence 5: Minimiser durée
            {
                "name": "Durée minimale (129 steps)",
                "actions": [GameAction.ACTION1] * 129
            }
        ]
        
        for i, seq in enumerate(critical_sequences, 1):
            print(f"\n[{i}/{len(critical_sequences)}] Test: {seq['name']}")
            print(f"        Actions: {len(seq['actions'])}")
            
            reward, done, steps = self.test_sequence(
                seq['actions'],
                description=seq['name']
            )
            
            print(f"        Résultat: reward={reward:.3f}, done={done}, steps={steps}")
            
            if reward > 0:
                print(f"        ✅ SOLUTION TROUVÉE!")
                return True
        
        print(f"\n❌ Aucune solution trouvée avec chemins critiques")
        return False
    
    def strategy_4_random_exploration(self, num_tests: int = 50):
        """
        Stratégie 4: Exploration aléatoire
        
        Tester séquences aléatoires pour découvrir patterns inattendus
        """
        print(f"\n{'='*80}")
        print(f"STRATÉGIE 4: EXPLORATION ALÉATOIRE ({num_tests} tests)")
        print(f"{'='*80}")
        
        for i in range(num_tests):
            # Générer séquence aléatoire
            length = np.random.randint(100, 200)
            
            # Probabilités: ACTION1-4 (80%), ACTION5 (20%)
            actions = []
            for _ in range(length):
                if np.random.random() < 0.2:
                    actions.append(GameAction.ACTION5)
                else:
                    actions.append(np.random.choice([
                        GameAction.ACTION1, GameAction.ACTION2,
                        GameAction.ACTION3, GameAction.ACTION4
                    ]))
            
            if (i + 1) % 10 == 0:
                print(f"\n[{i+1}/{num_tests}] Test aléatoire")
                print(f"        Actions: {length}")
            
            reward, done, steps = self.test_sequence(
                actions,
                description=f"Aléatoire #{i+1}"
            )
            
            if (i + 1) % 10 == 0:
                print(f"        Résultat: reward={reward:.3f}, done={done}, steps={steps}")
            
            if reward > 0:
                print(f"\n        ✅ SOLUTION TROUVÉE!")
                return True
        
        print(f"\n❌ Aucune solution trouvée avec exploration aléatoire")
        return False
    
    def save_results(self):
        """Sauvegarder résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session90_solution_search_{timestamp}.json"
        
        results = {
            "metadata": {
                "date": datetime.now().isoformat(),
                "game_id": self.game_id,
                "duration_seconds": time.time() - self.start_time,
                "total_tests": self.total_tests,
                "best_reward": self.best_reward,
                "winning_sequences_found": len(self.winning_sequences)
            },
            "tested_sequences": self.tested_sequences,
            "winning_sequences": self.winning_sequences
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n💾 Résultats sauvegardés: {filename}")
        return filename
    
    def run(self):
        """Exécuter recherche complète"""
        print(f"\n{'='*80}")
        print("DÉMARRAGE RECHERCHE SOLUTION")
        print(f"{'='*80}")
        print(f"Début: {datetime.now().strftime('%H:%M:%S')}")
        
        # Initialiser
        self.initialize()
        
        # Stratégie 1: Patterns connus
        if self.strategy_1_known_patterns():
            print(f"\n✅ Solution trouvée avec Stratégie 1!")
        
        # Stratégie 2: Exploration ACTION5
        elif self.strategy_2_systematic_action5():
            print(f"\n✅ Solution trouvée avec Stratégie 2!")
        
        # Stratégie 3: Chemins critiques
        elif self.strategy_3_critical_paths():
            print(f"\n✅ Solution trouvée avec Stratégie 3!")
        
        # Stratégie 4: Exploration aléatoire
        elif self.strategy_4_random_exploration(num_tests=50):
            print(f"\n✅ Solution trouvée avec Stratégie 4!")
        
        else:
            print(f"\n❌ AUCUNE SOLUTION TROUVÉE")
            print(f"   Tests effectués: {self.total_tests}")
            print(f"   Meilleur reward: {self.best_reward}")
        
        # Sauvegarder résultats
        filename = self.save_results()
        
        # Statistiques finales
        duration = time.time() - self.start_time
        print(f"\n{'='*80}")
        print("STATISTIQUES FINALES")
        print(f"{'='*80}")
        print(f"Durée totale: {duration:.1f} secondes")
        print(f"Tests effectués: {self.total_tests}")
        print(f"Solutions trouvées: {len(self.winning_sequences)}")
        print(f"Meilleur reward: {self.best_reward}")
        
        if self.winning_sequences:
            print(f"\n🎉 SOLUTIONS GAGNANTES:")
            for i, sol in enumerate(self.winning_sequences, 1):
                print(f"\n   Solution {i}:")
                print(f"   - Description: {sol['description']}")
                print(f"   - Reward: {sol['reward']}")
                print(f"   - Steps: {sol['steps']}")
                print(f"   - Actions: {sol['actions'][:10]}... ({len(sol['actions'])} total)")


def main():
    """Point d'entrée principal"""
    searcher = WinningSolutionSearcher("ls20-9607627b")
    searcher.run()


if __name__ == "__main__":
    main()

# Made with Bob
