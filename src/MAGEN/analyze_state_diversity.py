#!/usr/bin/env python3
"""
ANALYSE DIVERSITÉ D'ÉTATS - DIAGNOSTIC P0
==========================================

Objectif: Mesurer combien d'états RÉELLEMENT DIFFÉRENTS MAGEN observe
pendant l'exécution sur ls20-9607627b.

Métriques critiques:
1. Nombre d'états uniques (hash observations)
2. Distance moyenne entre observations successives
3. Corrélation action → changement observation
4. Variation des métadonnées (score, niveau, position)

Auteur: Bob (Session 79 - Feedback utilisateur)
Date: 2026-06-16
"""

import os
import sys
import json
import hashlib
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Set
from collections import defaultdict

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée! Exécutez: doppler run -- python3 analyze_state_diversity.py")

print(f"✅ ARC_API_KEY chargée: {ARC_API_KEY[:8]}...")

# Import arc-agi
try:
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
    print("✅ arc-agi importé\n")
except ImportError as e:
    print(f"❌ ERREUR: {e}")
    sys.exit(1)

# Configuration
TARGET_GAME = "ls20-9607627b"
MAX_STEPS = 100
SAMPLE_SIZE = 3  # Nombre de runs pour statistiques


class StateAnalyzer:
    """Analyseur de diversité d'états."""
    
    def __init__(self):
        self.state_hashes: Set[str] = set()
        self.observations: List[Any] = []
        self.metadata_history: List[Dict] = []
        self.action_history: List[str] = []
        self.state_changes: List[bool] = []
        
        # Statistiques
        self.stats = {
            'total_steps': 0,
            'unique_states': 0,
            'state_change_rate': 0.0,
            'metadata_variations': defaultdict(set),
            'action_to_change': defaultdict(list)
        }
    
    def hash_observation(self, obs) -> str:
        """Crée hash unique d'une observation."""
        if obs is None:
            return "none"
        
        # Extraire données pertinentes
        data = {
            'state': obs.state.name if hasattr(obs, 'state') else 'unknown',
            'metadata': obs.metadata if hasattr(obs, 'metadata') else {}
        }
        
        # Hash JSON
        json_str = json.dumps(data, sort_keys=True)
        return hashlib.md5(json_str.encode()).hexdigest()
    
    def record_step(self, obs, action: str, prev_obs):
        """Enregistre un step et analyse changements."""
        self.stats['total_steps'] += 1
        
        # Hash état actuel
        current_hash = self.hash_observation(obs)
        self.state_hashes.add(current_hash)
        self.observations.append(obs)
        self.action_history.append(action)
        
        # Métadonnées
        if obs and hasattr(obs, 'metadata') and obs.metadata:
            self.metadata_history.append(obs.metadata)
            
            # Tracker variations
            for key, value in obs.metadata.items():
                self.stats['metadata_variations'][key].add(str(value))
        
        # Détecter changement d'état
        if prev_obs is not None:
            prev_hash = self.hash_observation(prev_obs)
            changed = (current_hash != prev_hash)
            self.state_changes.append(changed)
            
            # Corrélation action → changement
            self.stats['action_to_change'][action].append(changed)
    
    def compute_statistics(self) -> Dict:
        """Calcule statistiques finales."""
        self.stats['unique_states'] = len(self.state_hashes)
        
        if self.state_changes:
            self.stats['state_change_rate'] = sum(self.state_changes) / len(self.state_changes)
        
        # Statistiques par action
        action_stats = {}
        for action, changes in self.stats['action_to_change'].items():
            if changes:
                action_stats[action] = {
                    'total': len(changes),
                    'changes': sum(changes),
                    'change_rate': sum(changes) / len(changes)
                }
        self.stats['action_statistics'] = action_stats
        
        # Variations métadonnées
        metadata_diversity = {}
        for key, values in self.stats['metadata_variations'].items():
            metadata_diversity[key] = {
                'unique_values': len(values),
                'values': sorted(list(values))[:10]  # Max 10 pour lisibilité
            }
        self.stats['metadata_diversity'] = metadata_diversity
        
        return self.stats


def run_analysis():
    """Exécute analyse complète."""
    print(f"{'='*80}")
    print(f"ANALYSE DIVERSITÉ D'ÉTATS - JEU {TARGET_GAME}")
    print(f"{'='*80}\n")
    
    # Connexion API
    arc = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.NORMAL)
    print("✅ Connexion API réussie\n")
    
    all_analyzers = []
    
    for run in range(SAMPLE_SIZE):
        print(f"\n{'='*80}")
        print(f"RUN #{run+1}/{SAMPLE_SIZE}")
        print(f"{'='*80}\n")
        
        analyzer = StateAnalyzer()
        
        # Créer scorecard
        scorecard_id = arc.create_scorecard()
        print(f"✅ Scorecard créé: {scorecard_id}")
        
        # Créer environnement
        env = arc.make(TARGET_GAME, scorecard_id=scorecard_id, seed=42+run)
        
        try:
            # Reset
            obs = env.reset()
            print(f"✅ Jeu démarré\n")
            
            prev_obs = None
            
            # Jouer
            for step in range(MAX_STEPS):
                # Action aléatoire
                action = np.random.choice(['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4', 'ACTION5', 'ACTION7'])
                
                # Exécuter
                obs = env.step(getattr(GameAction, action))
                
                # Analyser
                analyzer.record_step(obs, action, prev_obs)
                prev_obs = obs
                
                # Affichage minimal
                if (step + 1) % 20 == 0:
                    print(f"   Step {step+1}/{MAX_STEPS} - États uniques: {len(analyzer.state_hashes)}")
            
            print(f"\n✅ Run #{run+1} terminé")
            
        except Exception as e:
            print(f"❌ Erreur run #{run+1}: {e}")
        
        # Statistiques
        stats = analyzer.compute_statistics()
        all_analyzers.append(analyzer)
        
        print(f"\n📊 STATISTIQUES RUN #{run+1}:")
        print(f"   Steps totaux: {stats['total_steps']}")
        print(f"   États uniques: {stats['unique_states']}")
        print(f"   Taux changement: {stats['state_change_rate']:.2%}")
    
    # Statistiques globales
    print(f"\n{'='*80}")
    print(f"STATISTIQUES GLOBALES ({SAMPLE_SIZE} runs)")
    print(f"{'='*80}\n")
    
    total_steps = sum(a.stats['total_steps'] for a in all_analyzers)
    total_unique = sum(a.stats['unique_states'] for a in all_analyzers)
    avg_change_rate = np.mean([a.stats['state_change_rate'] for a in all_analyzers])
    
    print(f"📊 MÉTRIQUES CRITIQUES:")
    print(f"   Steps totaux: {total_steps}")
    print(f"   États uniques totaux: {total_unique}")
    print(f"   États uniques moyens/run: {total_unique/SAMPLE_SIZE:.1f}")
    print(f"   Taux changement moyen: {avg_change_rate:.2%}")
    print(f"   Ratio états/steps: {total_unique/total_steps:.2%}")
    
    # Analyse métadonnées
    print(f"\n📊 DIVERSITÉ MÉTADONNÉES:")
    if all_analyzers[0].stats['metadata_diversity']:
        for key, data in all_analyzers[0].stats['metadata_diversity'].items():
            print(f"   {key}: {data['unique_values']} valeurs uniques")
            print(f"      Exemples: {data['values']}")
    else:
        print("   ⚠️  Aucune métadonnée détectée!")
    
    # Analyse actions
    print(f"\n📊 CORRÉLATION ACTION → CHANGEMENT:")
    if all_analyzers[0].stats['action_statistics']:
        for action, data in sorted(all_analyzers[0].stats['action_statistics'].items()):
            print(f"   {action}: {data['change_rate']:.2%} changements ({data['changes']}/{data['total']})")
    
    # Diagnostic
    print(f"\n{'='*80}")
    print(f"DIAGNOSTIC")
    print(f"{'='*80}\n")
    
    if total_unique < total_steps * 0.1:
        print("🔴 CRITIQUE: Très faible diversité d'états (<10%)")
        print("   → L'environnement change très peu")
        print("   → Reward intrinsèque s'effondre normalement")
        print("   → Apprentissage impossible sans plus d'observabilité")
    elif total_unique < total_steps * 0.3:
        print("🟡 ATTENTION: Diversité d'états limitée (<30%)")
        print("   → Environnement partiellement observable")
        print("   → Apprentissage difficile")
    else:
        print("🟢 OK: Diversité d'états acceptable (>30%)")
        print("   → Environnement suffisamment observable")
    
    if avg_change_rate < 0.3:
        print("\n🔴 CRITIQUE: Faible taux de changement (<30%)")
        print("   → Actions ont peu d'effet sur l'environnement")
        print("   → Vérifier mapping actions → commandes jeu")
    
    # Sauvegarder résultats
    results = {
        'game_id': TARGET_GAME,
        'timestamp': datetime.now().isoformat(),
        'runs': SAMPLE_SIZE,
        'total_steps': total_steps,
        'total_unique_states': total_unique,
        'avg_unique_per_run': total_unique / SAMPLE_SIZE,
        'avg_change_rate': avg_change_rate,
        'ratio_states_steps': total_unique / total_steps,
        'metadata_diversity': dict(all_analyzers[0].stats['metadata_diversity']),
        'action_statistics': dict(all_analyzers[0].stats['action_statistics'])
    }
    
    output_path = Path("results") / f"state_diversity_analysis_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    output_path.parent.mkdir(exist_ok=True)
    
    with open(output_path, 'w') as f:
        json.dump(results, f, indent=2, default=str)
    
    print(f"\n✅ Résultats sauvegardés: {output_path}")


if __name__ == "__main__":
    run_analysis()

# Made with Bob
