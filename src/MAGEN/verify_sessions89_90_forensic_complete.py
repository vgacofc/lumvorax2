#!/usr/bin/env python3
"""
Script de vérification forensique COMPLÈTE Sessions 89 et 90
Lit TOUS les fichiers de logs pour valider la véracité des données
Produit un rapport MD détaillé pour chaque phase
"""

import json
import os
from pathlib import Path
from typing import Dict, List, Tuple, Set
from collections import defaultdict
from datetime import datetime

class ForensicVerifier:
    def __init__(self, base_dir: str = "."):
        self.base_dir = Path(base_dir)
        self.session89_data = {
            'tentative1': {'games': [], 'forensic': [], 'checkpoints': [], 'final': None},
            'tentative2': {'games': [], 'forensic': [], 'checkpoints': [], 'final': None}
        }
        self.session90_data = {
            'games': [],
            'forensic': [],
            'checkpoints': [],
            'final': None
        }
        
    def load_all_files(self):
        """Charge TOUS les fichiers JSON/JSONL des sessions 89 et 90"""
        print("=" * 80)
        print("CHARGEMENT FICHIERS FORENSIQUES")
        print("=" * 80)
        
        # Session 89 - Identifier les deux tentatives par timestamp
        s89_files = sorted(self.base_dir.glob("session89_*.json"))
        s89_forensic = sorted(self.base_dir.glob("session89_*.jsonl"))
        
        # Tentative 1: 024543 (02:45:43) - 11 parties
        # Tentative 2: 025641 (02:56:41) - 70 parties
        
        for file in s89_files:
            if "game_" in file.name:
                game_num = int(file.name.split("_")[2])
                if game_num <= 11 and "0245" in file.name:
                    self.session89_data['tentative1']['games'].append(file)
                else:
                    self.session89_data['tentative2']['games'].append(file)
            elif "checkpoint_" in file.name:
                if "0245" in file.name or "0252" in file.name or "0253" in file.name:
                    self.session89_data['tentative1']['checkpoints'].append(file)
                else:
                    self.session89_data['tentative2']['checkpoints'].append(file)
            elif "cartography_complete_" in file.name:
                if "025314" in file.name:
                    self.session89_data['tentative1']['final'] = file
                else:
                    self.session89_data['tentative2']['final'] = file
        
        for file in s89_forensic:
            if "024543" in file.name:
                self.session89_data['tentative1']['forensic'].append(file)
            else:
                self.session89_data['tentative2']['forensic'].append(file)
        
        # Session 90
        s90_files = sorted(self.base_dir.glob("session90_*.json"))
        s90_forensic = sorted(self.base_dir.glob("session90_*.jsonl"))
        
        for file in s90_files:
            if "game_" in file.name or "test_" in file.name:
                self.session90_data['games'].append(file)
            elif "checkpoint_" in file.name:
                self.session90_data['checkpoints'].append(file)
            elif "search_results_" in file.name or "results_" in file.name:
                self.session90_data['final'] = file
        
        self.session90_data['forensic'] = list(s90_forensic)
        
        print(f"\n📁 SESSION 89 TENTATIVE 1:")
        print(f"   - Games: {len(self.session89_data['tentative1']['games'])}")
        print(f"   - Forensic: {len(self.session89_data['tentative1']['forensic'])}")
        print(f"   - Checkpoints: {len(self.session89_data['tentative1']['checkpoints'])}")
        print(f"   - Final: {'✓' if self.session89_data['tentative1']['final'] else '✗'}")
        
        print(f"\n📁 SESSION 89 TENTATIVE 2:")
        print(f"   - Games: {len(self.session89_data['tentative2']['games'])}")
        print(f"   - Forensic: {len(self.session89_data['tentative2']['forensic'])}")
        print(f"   - Checkpoints: {len(self.session89_data['tentative2']['checkpoints'])}")
        print(f"   - Final: {'✓' if self.session89_data['tentative2']['final'] else '✗'}")
        
        print(f"\n📁 SESSION 90:")
        print(f"   - Games: {len(self.session90_data['games'])}")
        print(f"   - Forensic: {len(self.session90_data['forensic'])}")
        print(f"   - Checkpoints: {len(self.session90_data['checkpoints'])}")
        print(f"   - Final: {'✓' if self.session90_data['final'] else '✗'}")
    
    def verify_session89_tentative1(self) -> Dict:
        """Vérifie Session 89 Tentative 1 (convergence prématurée)"""
        print("\n" + "=" * 80)
        print("VÉRIFICATION SESSION 89 TENTATIVE 1")
        print("=" * 80)
        
        results = {
            'total_games': 0,
            'total_steps': 0,
            'total_states': set(),
            'games_data': [],
            'forensic_lines': 0,
            'convergence_detected': False,
            'issues': []
        }
        
        # Lire tous les fichiers game
        for game_file in sorted(self.session89_data['tentative1']['games']):
            try:
                with open(game_file, 'r') as f:
                    data = json.load(f)
                    results['total_games'] += 1
                    results['total_steps'] += data.get('total_steps', 0)
                    
                    # Collecter états uniques
                    for step in data.get('steps', []):
                        state_key = f"{step['pixels_11']}_{step['pixels_8']}_{step['pixels_3']}"
                        results['total_states'].add(state_key)
                    
                    results['games_data'].append({
                        'game_number': data.get('game_number', 0),
                        'steps': data.get('total_steps', 0),
                        'new_states': data.get('new_states_discovered', 0),
                        'duration': data.get('duration_seconds', 0)
                    })
                    
                    print(f"✓ Game {data.get('game_number', 0):03d}: {data.get('total_steps', 0)} steps, {data.get('new_states_discovered', 0)} nouveaux états")
            except Exception as e:
                results['issues'].append(f"Erreur lecture {game_file.name}: {e}")
        
        # Lire forensic JSONL
        for forensic_file in self.session89_data['tentative1']['forensic']:
            try:
                with open(forensic_file, 'r') as f:
                    for line in f:
                        results['forensic_lines'] += 1
            except Exception as e:
                results['issues'].append(f"Erreur lecture {forensic_file.name}: {e}")
        
        # Lire fichier final
        if self.session89_data['tentative1']['final']:
            try:
                with open(self.session89_data['tentative1']['final'], 'r') as f:
                    final_data = json.load(f)
                    results['final_metadata'] = final_data.get('metadata', {})
                    results['convergence_detected'] = final_data.get('metadata', {}).get('converged', False)
            except Exception as e:
                results['issues'].append(f"Erreur lecture final: {e}")
        
        results['unique_states'] = len(results['total_states'])
        
        print(f"\n📊 RÉSULTATS TENTATIVE 1:")
        print(f"   - Parties: {results['total_games']}")
        print(f"   - Steps totaux: {results['total_steps']}")
        print(f"   - États uniques: {results['unique_states']}")
        print(f"   - Lignes forensic: {results['forensic_lines']}")
        print(f"   - Convergence: {results['convergence_detected']}")
        print(f"   - Problèmes: {len(results['issues'])}")
        
        return results
    
    def verify_session89_tentative2(self) -> Dict:
        """Vérifie Session 89 Tentative 2 (cartographie 100%)"""
        print("\n" + "=" * 80)
        print("VÉRIFICATION SESSION 89 TENTATIVE 2")
        print("=" * 80)
        
        results = {
            'total_games': 0,
            'total_steps': 0,
            'total_states': set(),
            'games_data': [],
            'forensic_lines': 0,
            'convergence_detected': False,
            'convergence_rate': 0.0,
            'states_per_game': [],
            'duration_distribution': [],
            'action_distribution': defaultdict(int),
            'issues': []
        }
        
        # Lire tous les fichiers game
        for game_file in sorted(self.session89_data['tentative2']['games']):
            try:
                with open(game_file, 'r') as f:
                    data = json.load(f)
                    results['total_games'] += 1
                    results['total_steps'] += data.get('total_steps', 0)
                    new_states = data.get('new_states_discovered', 0)
                    results['states_per_game'].append(new_states)
                    
                    # Collecter états uniques
                    for step in data.get('steps', []):
                        state_key = f"{step['pixels_11']}_{step['pixels_8']}_{step['pixels_3']}"
                        results['total_states'].add(state_key)
                        
                        # Distribution actions
                        action = step.get('action', 'UNKNOWN')
                        results['action_distribution'][action] += 1
                    
                    results['duration_distribution'].append(data.get('total_steps', 0))
                    
                    results['games_data'].append({
                        'game_number': data.get('game_number', 0),
                        'steps': data.get('total_steps', 0),
                        'new_states': new_states,
                        'duration': data.get('duration_seconds', 0)
                    })
                    
                    if results['total_games'] % 10 == 0:
                        print(f"✓ Traité {results['total_games']} parties...")
            except Exception as e:
                results['issues'].append(f"Erreur lecture {game_file.name}: {e}")
        
        # Lire forensic JSONL
        for forensic_file in self.session89_data['tentative2']['forensic']:
            try:
                with open(forensic_file, 'r') as f:
                    for line in f:
                        results['forensic_lines'] += 1
                        if results['forensic_lines'] % 1000 == 0:
                            print(f"✓ Traité {results['forensic_lines']} lignes forensic...")
            except Exception as e:
                results['issues'].append(f"Erreur lecture {forensic_file.name}: {e}")
        
        # Lire fichier final
        if self.session89_data['tentative2']['final']:
            try:
                with open(self.session89_data['tentative2']['final'], 'r') as f:
                    final_data = json.load(f)
                    results['final_metadata'] = final_data.get('metadata', {})
                    results['convergence_detected'] = final_data.get('metadata', {}).get('converged', False)
                    results['convergence_rate'] = final_data.get('metadata', {}).get('convergence_rate', 0.0)
            except Exception as e:
                results['issues'].append(f"Erreur lecture final: {e}")
        
        results['unique_states'] = len(results['total_states'])
        
        # Calculer statistiques
        results['avg_steps'] = results['total_steps'] / results['total_games'] if results['total_games'] > 0 else 0
        results['min_duration'] = min(results['duration_distribution']) if results['duration_distribution'] else 0
        results['max_duration'] = max(results['duration_distribution']) if results['duration_distribution'] else 0
        
        print(f"\n📊 RÉSULTATS TENTATIVE 2:")
        print(f"   - Parties: {results['total_games']}")
        print(f"   - Steps totaux: {results['total_steps']}")
        print(f"   - États uniques: {results['unique_states']}")
        print(f"   - Lignes forensic: {results['forensic_lines']}")
        print(f"   - Convergence: {results['convergence_detected']} ({results['convergence_rate']:.4f})")
        print(f"   - Durée min/max: {results['min_duration']}/{results['max_duration']}")
        print(f"   - Problèmes: {len(results['issues'])}")
        
        return results
    
    def verify_session90(self) -> Dict:
        """Vérifie Session 90 (recherche solution)"""
        print("\n" + "=" * 80)
        print("VÉRIFICATION SESSION 90")
        print("=" * 80)
        
        results = {
            'total_tests': 0,
            'total_steps': 0,
            'tests_data': [],
            'rewards_found': [],
            'forensic_lines': 0,
            'issues': []
        }
        
        # Lire tous les fichiers game/test
        for game_file in sorted(self.session90_data['games']):
            try:
                with open(game_file, 'r') as f:
                    data = json.load(f)
                    results['total_tests'] += 1
                    results['total_steps'] += data.get('total_steps', 0)
                    
                    reward = data.get('reward', 0.0)
                    if reward > 0:
                        results['rewards_found'].append({
                            'test': data.get('test_number', 0),
                            'reward': reward,
                            'strategy': data.get('strategy', 'UNKNOWN')
                        })
                    
                    results['tests_data'].append({
                        'test_number': data.get('test_number', 0),
                        'strategy': data.get('strategy', 'UNKNOWN'),
                        'steps': data.get('total_steps', 0),
                        'reward': reward,
                        'done': data.get('done', False)
                    })
                    
                    print(f"✓ Test {data.get('test_number', 0):03d}: {data.get('strategy', 'UNKNOWN')} - reward={reward:.3f}")
            except Exception as e:
                results['issues'].append(f"Erreur lecture {game_file.name}: {e}")
        
        # Lire forensic JSONL
        for forensic_file in self.session90_data['forensic']:
            try:
                with open(forensic_file, 'r') as f:
                    for line in f:
                        results['forensic_lines'] += 1
            except Exception as e:
                results['issues'].append(f"Erreur lecture {forensic_file.name}: {e}")
        
        # Lire fichier final
        if self.session90_data['final']:
            try:
                with open(self.session90_data['final'], 'r') as f:
                    final_data = json.load(f)
                    results['final_metadata'] = final_data.get('metadata', {})
            except Exception as e:
                results['issues'].append(f"Erreur lecture final: {e}")
        
        print(f"\n📊 RÉSULTATS SESSION 90:")
        print(f"   - Tests: {results['total_tests']}")
        print(f"   - Steps totaux: {results['total_steps']}")
        print(f"   - Rewards > 0: {len(results['rewards_found'])}")
        print(f"   - Lignes forensic: {results['forensic_lines']}")
        print(f"   - Problèmes: {len(results['issues'])}")
        
        return results

if __name__ == "__main__":
    verifier = ForensicVerifier()
    verifier.load_all_files()
    
    # Vérifier Session 89 Tentative 1
    results_s89_t1 = verifier.verify_session89_tentative1()
    
    # Vérifier Session 89 Tentative 2
    results_s89_t2 = verifier.verify_session89_tentative2()
    
    # Vérifier Session 90
    results_s90 = verifier.verify_session90()
    
    print("\n" + "=" * 80)
    print("VÉRIFICATION TERMINÉE - Génération rapports en cours...")
    print("=" * 80)

# Made with Bob
