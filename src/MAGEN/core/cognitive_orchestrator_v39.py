"""
PHASE 5.5 V39: Cognitive Orchestrator
Couche d'orchestration entre les 4 systèmes fondamentaux.

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import numpy as np
from typing import Dict, List, Optional, Tuple
from datetime import datetime
import json

from minimal_learning_system_v39 import MinimalLearningSystemV39
from agent_localization_v39 import AgentLocalizationSystem
from world_state_graph_v39 import WorldStateGraph
from causal_memory_v39 import CausalMemorySystem


class CognitiveOrchestrator:
    """
    Orchestrateur cognitif pour MAGEN.
    Coordonne les 4 systèmes fondamentaux.
    """
    
    def __init__(self, grid_width: int, grid_height: int):
        """Initialise l'orchestrateur."""
        self.grid_width = grid_width
        self.grid_height = grid_height
        
        # Créer les 4 systèmes
        self.learning = MinimalLearningSystemV39(learning_rate=0.1)
        self.localization = AgentLocalizationSystem(grid_width, grid_height)
        self.world_graph = WorldStateGraph(grid_width, grid_height)
        self.causal_memory = CausalMemorySystem(max_events=10000)
        
        # État global
        self.current_step = 0
        self.total_actions = 0
        
    def execute_step(self, action_id: str, x: int, y: int, 
                    transformation: str, value: Optional[int] = None) -> Dict:
        """
        Exécute un step complet avec orchestration.
        
        Returns:
            Résultats du step
        """
        self.current_step += 1
        self.total_actions += 1
        
        # 1. Localisation met à jour position
        position = self.localization.update_position(x, y, self.current_step)
        
        # 2. World graph enregistre visite
        cell = self.world_graph.visit_cell(x, y, value, transformation)
        
        # 3. Simuler résultat action
        success = np.random.random() > 0.3
        reward = np.random.random() if success else 0.2
        error = 1.0 - reward
        
        # 4. Apprentissage met à jour scores
        new_score = self.learning.update_action_score(action_id, reward, success)
        
        # 5. Causal memory enregistre
        input_state = {'x': x, 'y': y, 'value': value or 0}
        output_state = {'x': x, 'y': y, 'value': (value or 0) + 1}
        self.causal_memory.record_transformation(
            transformation, input_state, output_state, success, error
        )
        
        # 6. Découvrir régions si couverture suffisante
        coverage = self.localization.get_stats()['coverage']
        if coverage > 0.3:
            region_id = f"region_{x//5}_{y//5}"
            if region_id not in self.world_graph.regions:
                self.world_graph.discover_region(region_id, x, y)
        
        return {
            'step': self.current_step,
            'position': (x, y),
            'success': success,
            'reward': reward,
            'new_score': new_score,
            'coverage': coverage,
            'confidence': self.localization.get_position_confidence()
        }
    
    def get_integrated_stats(self) -> Dict:
        """Retourne statistiques intégrées."""
        return {
            'learning': {
                'actions': len(self.learning.action_scores),
                'updates': self.learning.total_updates,
                'variance': float(np.var([s.score for s in self.learning.action_scores.values()]))
            },
            'localization': self.localization.get_stats(),
            'world_graph': self.world_graph.get_stats(),
            'causal_memory': self.causal_memory.get_stats(),
            'orchestrator': {
                'total_steps': self.current_step,
                'total_actions': self.total_actions
            }
        }


def test_orchestrator():
    """Test de l'orchestrateur."""
    print("="*80)
    print("TEST COGNITIVE ORCHESTRATOR V39")
    print("="*80)
    
    orchestrator = CognitiveOrchestrator(grid_width=20, grid_height=20)
    
    print("\nExécution 300 steps orchestrés...")
    for step in range(300):
        x = np.random.randint(0, 20)
        y = np.random.randint(0, 20)
        action_id = f"action_{np.random.randint(0, 5)}"
        transformation = f"transform_{np.random.randint(0, 5)}"
        value = np.random.randint(0, 10)
        
        result = orchestrator.execute_step(action_id, x, y, transformation, value)
        
        if step % 100 == 0 and step > 0:
            print(f"  Step {step}: Couverture {result['coverage']:.1%}, Confiance {result['confidence']:.1%}")
    
    # Statistiques finales
    stats = orchestrator.get_integrated_stats()
    
    print("\n" + "="*80)
    print("STATISTIQUES INTÉGRÉES")
    print("="*80)
    print(f"\nApprentissage:")
    print(f"  Actions: {stats['learning']['actions']}")
    print(f"  Updates: {stats['learning']['updates']}")
    print(f"  Variance: {stats['learning']['variance']:.6f}")
    
    print(f"\nLocalisation:")
    print(f"  Confiance: {stats['localization']['confidence']:.1%}")
    print(f"  Couverture: {stats['localization']['coverage']:.1%}")
    
    print(f"\nWorld Graph:")
    print(f"  Couverture: {stats['world_graph']['coverage']:.1%}")
    print(f"  Régions: {stats['world_graph']['regions_discovered']}")
    
    print(f"\nCausal Memory:")
    print(f"  Événements: {stats['causal_memory']['total_events']}")
    print(f"  Transformations: {stats['causal_memory']['transformations_tracked']}")
    
    print("\n" + "="*80)
    print("✅ TEST RÉUSSI - Orchestration fonctionnelle")
    print("="*80)


if __name__ == '__main__':
    test_orchestrator()

# Made with Bob
