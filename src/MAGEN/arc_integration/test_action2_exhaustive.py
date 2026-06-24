#!/usr/bin/env python3
"""
PHASE 4.4.1 - ANALYSE EXHAUSTIVE ACTION2
========================================

Objectif: Comprendre les 50 états distincts d'ACTION2
Méthodologie: 1,000 observations systématiques
Livrables: Matrice transitions 50×50 + Rapport complet

Auteur: Bob (MAGEN Learning System)
Date: 2026-06-19
"""

import sys
import os
import json
import numpy as np
from collections import defaultdict, Counter
from datetime import datetime
import hashlib

# Ajouter le chemin du module ls20
ls20_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../environment_files/ls20/9607627b'))
if ls20_path not in sys.path:
    sys.path.insert(0, ls20_path)

try:
    import ls20
    print(f"✅ Module ls20 importé depuis: {ls20.__file__}")
except ImportError as e:
    print(f"❌ Erreur import ls20: {e}")
    print(f"   Chemin recherché: {ls20_path}")
    print(f"   Chemin existe: {os.path.exists(ls20_path)}")
    sys.exit(1)


class ACTION2Analyzer:
    """Analyseur exhaustif ACTION2"""
    
    def __init__(self, num_observations=1000):
        self.num_observations = num_observations
        self.game = None
        self.observations = []
        self.state_hashes = []
        self.transitions = defaultdict(lambda: defaultdict(int))
        self.state_to_index = {}
        self.index_to_state = {}
        self.next_state_index = 0
        
        # Statistiques
        self.stats = {
            'total_observations': 0,
            'unique_states': 0,
            'cycles_detected': [],
            'periodicity': None,
            'state_changes': defaultdict(list)
        }
    
    def initialize_game(self):
        """Initialise le jeu ARC-AGI"""
        print("\n🎮 Initialisation jeu ARC-AGI ls20-9607627b...")
        self.game = ls20.Ls20()
        self.game.set_level(1)
        # Obtenir pixels avec dimensions camera
        obs = self.game.get_pixels(0, 0, self.game.camera.width, self.game.camera.height)
        print(f"   ✅ Jeu initialisé: Niveau 1")
        print(f"   📊 Actions disponibles: 4 (ACTION1-4)")
        print(f"   📐 Dimensions: {obs.shape}")
        return obs
    
    def hash_state(self, state):
        """Crée hash unique pour un état"""
        if isinstance(state, dict):
            # Extraire observation si dict
            obs = state.get('observation', state)
        else:
            obs = state
        
        # Convertir en bytes pour hash
        if isinstance(obs, np.ndarray):
            state_bytes = obs.tobytes()
        else:
            state_bytes = str(obs).encode()
        
        return hashlib.sha256(state_bytes).hexdigest()[:16]
    
    def get_state_index(self, state_hash):
        """Obtient ou crée index pour un état"""
        if state_hash not in self.state_to_index:
            self.state_to_index[state_hash] = self.next_state_index
            self.index_to_state[self.next_state_index] = state_hash
            self.next_state_index += 1
        return self.state_to_index[state_hash]
    
    def extract_state_features(self, state):
        """Extrait features d'un état pour analyse"""
        if isinstance(state, dict):
            obs = state.get('observation', state)
        else:
            obs = state
        
        if isinstance(obs, np.ndarray):
            return {
                'shape': obs.shape,
                'dtype': str(obs.dtype),
                'min': float(np.min(obs)),
                'max': float(np.max(obs)),
                'mean': float(np.mean(obs)),
                'std': float(np.std(obs)),
                'unique_values': len(np.unique(obs)),
                'sum': float(np.sum(obs)),
                'non_zero': int(np.count_nonzero(obs))
            }
        else:
            return {'raw': str(obs)[:100]}
    
    def run_observation(self, obs_num):
        """Exécute une observation ACTION2"""
        # Reset jeu
        self.game.set_level(0)  # Niveau 1 (index 0)
        
        # État initial via camera.render()
        initial_state = self.game.camera.render(self.game.current_level._sprites)
        initial_hash = self.hash_state(initial_state)
        initial_features = self.extract_state_features(initial_state)
        
        # Exécuter ACTION2 avec la méthode correcte
        # Créer objet action pour _set_action()
        from arcengine import GameAction
        
        class ActionInput:
            def __init__(self, action_id):
                self.id = action_id
        
        # ACTION2 = GameAction.ACTION2
        game_action_enum = GameAction.ACTION2
        action_input = ActionInput(game_action_enum)
        
        # Exécuter action
        self.game._set_action(action_input)
        
        # Exécuter steps jusqu'à action complète
        max_steps = 100
        steps = 0
        while not self.game.is_action_complete() and steps < max_steps:
            self.game.step()
            steps += 1
        
        # État final
        final_state = self.game.camera.render(self.game.current_level._sprites)
        final_hash = self.hash_state(final_state)
        final_features = self.extract_state_features(final_state)
        
        # Info
        reward = -0.01  # Pénalité par défaut
        done = False
        info = {'steps_taken': steps, 'action_complete': self.game.is_action_complete()}
        
        # Enregistrer observation
        observation = {
            'num': obs_num,
            'initial_state_hash': initial_hash,
            'final_state_hash': final_hash,
            'initial_features': initial_features,
            'final_features': final_features,
            'reward': float(reward),
            'done': bool(done),
            'info': info,
            'state_changed': initial_hash != final_hash
        }
        
        self.observations.append(observation)
        self.state_hashes.append(final_hash)
        
        # Enregistrer transition
        initial_idx = self.get_state_index(initial_hash)
        final_idx = self.get_state_index(final_hash)
        self.transitions[initial_idx][final_idx] += 1
        
        # Enregistrer changements
        if initial_hash != final_hash:
            self.stats['state_changes'][initial_hash].append({
                'to': final_hash,
                'obs_num': obs_num,
                'features_diff': self.compute_features_diff(initial_features, final_features)
            })
        
        return observation
    
    def compute_features_diff(self, feat1, feat2):
        """Calcule différence entre features"""
        diff = {}
        for key in feat1:
            if key in feat2 and isinstance(feat1[key], (int, float)):
                diff[key] = feat2[key] - feat1[key]
        return diff
    
    def run_analysis(self):
        """Exécute analyse complète"""
        print(f"\n🔬 ANALYSE ACTION2 EXHAUSTIVE")
        print(f"=" * 70)
        print(f"Observations cibles: {self.num_observations}")
        print(f"Action testée: ACTION2 (index 1)")
        print(f"=" * 70)
        
        # Initialiser jeu
        self.initialize_game()
        
        # Exécuter observations
        print(f"\n📊 Exécution observations...")
        for i in range(self.num_observations):
            obs = self.run_observation(i + 1)
            
            # Afficher progression tous les 100
            if (i + 1) % 100 == 0:
                unique = len(set(self.state_hashes[:i+1]))
                pct = ((i + 1) / self.num_observations) * 100
                print(f"   Observation {i+1:4d}/{self.num_observations} ({pct:5.1f}%) - États uniques: {unique}")
        
        # Calculer statistiques
        self.compute_statistics()
        
        # Afficher résumé
        self.print_summary()
        
        return self.observations
    
    def compute_statistics(self):
        """Calcule statistiques complètes"""
        print(f"\n📈 Calcul statistiques...")
        
        # États uniques
        unique_states = set(self.state_hashes)
        self.stats['unique_states'] = len(unique_states)
        self.stats['total_observations'] = len(self.observations)
        
        # Distribution états
        state_counts = Counter(self.state_hashes)
        self.stats['state_distribution'] = dict(state_counts.most_common(20))
        
        # Détecter cycles
        self.detect_cycles()
        
        # Analyser périodicité
        self.analyze_periodicity()
        
        # Matrice transitions
        self.build_transition_matrix()
    
    def detect_cycles(self):
        """Détecte cycles dans séquence états"""
        print(f"   🔄 Détection cycles...")
        
        # Chercher patterns répétitifs
        for cycle_len in range(2, min(51, len(self.state_hashes) // 2)):
            for start in range(len(self.state_hashes) - cycle_len * 2):
                pattern = self.state_hashes[start:start + cycle_len]
                next_pattern = self.state_hashes[start + cycle_len:start + cycle_len * 2]
                
                if pattern == next_pattern:
                    self.stats['cycles_detected'].append({
                        'length': cycle_len,
                        'start': start,
                        'pattern': pattern[:5]  # Premiers 5 états
                    })
                    break
            
            if self.stats['cycles_detected']:
                break
    
    def analyze_periodicity(self):
        """Analyse périodicité séquence"""
        print(f"   📊 Analyse périodicité...")
        
        # Compter occurrences chaque état
        state_positions = defaultdict(list)
        for i, state_hash in enumerate(self.state_hashes):
            state_positions[state_hash].append(i)
        
        # Chercher périodicité
        periodicities = []
        for state_hash, positions in state_positions.items():
            if len(positions) >= 3:
                diffs = [positions[i+1] - positions[i] for i in range(len(positions)-1)]
                if len(set(diffs)) == 1:  # Périodicité constante
                    periodicities.append({
                        'state': state_hash,
                        'period': diffs[0],
                        'occurrences': len(positions)
                    })
        
        if periodicities:
            # Prendre périodicité la plus fréquente
            self.stats['periodicity'] = max(periodicities, key=lambda x: x['occurrences'])
    
    def build_transition_matrix(self):
        """Construit matrice transitions"""
        print(f"   🔢 Construction matrice transitions...")
        
        n_states = len(self.state_to_index)
        matrix = np.zeros((n_states, n_states), dtype=int)
        
        for from_idx, to_dict in self.transitions.items():
            for to_idx, count in to_dict.items():
                matrix[from_idx, to_idx] = count
        
        self.stats['transition_matrix'] = matrix
        self.stats['transition_matrix_shape'] = matrix.shape
    
    def print_summary(self):
        """Affiche résumé analyse"""
        print(f"\n" + "=" * 70)
        print(f"📊 RÉSUMÉ ANALYSE ACTION2")
        print(f"=" * 70)
        
        print(f"\n🔢 STATISTIQUES GLOBALES:")
        print(f"   Observations totales: {self.stats['total_observations']}")
        print(f"   États uniques: {self.stats['unique_states']}")
        print(f"   Ratio états/observations: {self.stats['unique_states']/self.stats['total_observations']:.2%}")
        
        # Distribution états
        print(f"\n📈 DISTRIBUTION ÉTATS (Top 10):")
        for i, (state_hash, count) in enumerate(list(self.stats['state_distribution'].items())[:10], 1):
            pct = (count / self.stats['total_observations']) * 100
            print(f"   {i:2d}. État {state_hash}: {count:4d} ({pct:5.2f}%)")
        
        # Cycles
        if self.stats['cycles_detected']:
            print(f"\n🔄 CYCLES DÉTECTÉS:")
            for cycle in self.stats['cycles_detected'][:5]:
                print(f"   Longueur: {cycle['length']}, Début: {cycle['start']}")
        else:
            print(f"\n🔄 CYCLES: Aucun cycle détecté")
        
        # Périodicité
        if self.stats['periodicity']:
            p = self.stats['periodicity']
            print(f"\n📊 PÉRIODICITÉ:")
            print(f"   Période: {p['period']}")
            print(f"   Occurrences: {p['occurrences']}")
            print(f"   État: {p['state']}")
        else:
            print(f"\n📊 PÉRIODICITÉ: Aucune périodicité détectée")
        
        # Matrice transitions
        print(f"\n🔢 MATRICE TRANSITIONS:")
        print(f"   Dimensions: {self.stats['transition_matrix_shape']}")
        print(f"   Transitions totales: {np.sum(self.stats['transition_matrix'])}")
        print(f"   Transitions non-nulles: {np.count_nonzero(self.stats['transition_matrix'])}")
    
    def save_results(self, output_dir="."):
        """Sauvegarde résultats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Sauvegarder observations
        obs_file = os.path.join(output_dir, f"action2_observations_{timestamp}.json")
        with open(obs_file, 'w') as f:
            json.dump(self.observations, f, indent=2)
        print(f"\n💾 Observations sauvegardées: {obs_file}")
        
        # Sauvegarder statistiques
        stats_file = os.path.join(output_dir, f"action2_statistics_{timestamp}.json")
        stats_serializable = {
            'total_observations': self.stats['total_observations'],
            'unique_states': self.stats['unique_states'],
            'state_distribution': self.stats['state_distribution'],
            'cycles_detected': self.stats['cycles_detected'],
            'periodicity': self.stats['periodicity'],
            'transition_matrix_shape': self.stats['transition_matrix_shape'],
            'state_to_index': self.state_to_index,
            'index_to_state': self.index_to_state
        }
        with open(stats_file, 'w') as f:
            json.dump(stats_serializable, f, indent=2)
        print(f"💾 Statistiques sauvegardées: {stats_file}")
        
        # Sauvegarder matrice transitions
        matrix_file = os.path.join(output_dir, f"action2_transition_matrix_{timestamp}.npy")
        np.save(matrix_file, self.stats['transition_matrix'])
        print(f"💾 Matrice transitions sauvegardée: {matrix_file}")
        
        return {
            'observations': obs_file,
            'statistics': stats_file,
            'matrix': matrix_file
        }


def main():
    """Fonction principale"""
    print("=" * 70)
    print("PHASE 4.4.1 - ANALYSE EXHAUSTIVE ACTION2")
    print("=" * 70)
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Objectif: Comprendre les 50 états ACTION2")
    print("=" * 70)
    
    # Créer analyseur
    analyzer = ACTION2Analyzer(num_observations=1000)
    
    # Exécuter analyse
    observations = analyzer.run_analysis()
    
    # Sauvegarder résultats
    files = analyzer.save_results()
    
    print(f"\n✅ ANALYSE COMPLÉTÉE")
    print(f"   Observations: {len(observations)}")
    print(f"   États uniques: {analyzer.stats['unique_states']}")
    print(f"   Fichiers générés: {len(files)}")
    
    return analyzer


if __name__ == "__main__":
    analyzer = main()

# Made with Bob
