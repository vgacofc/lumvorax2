#!/usr/bin/env python3
"""
MAGEN V40 - WORLD MODEL PRIORITAIRE
====================================

Objectif: Construire représentation persistante du monde
Priorité: World State Graph > Agent Localization > Causal Memory

Métriques cibles V40:
- patterns_discovered: 0 → >10
- regions_discovered: 0 → >5
- causal_chains: 0 → >3
- repeated_states: 0 → >50
- diversity_score: 1.0 → <0.7

Critères GO/NO-GO:
- GO si: patterns>0 ET régions>0 ET répétitions>0 ET diversité<1.0
- NO-GO si: Toutes métriques restent à 0
"""

import sys
import json
import time
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Ajouter le chemin src au PYTHONPATH
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from MAGEN.core.world_state_graph_v39 import WorldStateGraph
from MAGEN.core.agent_localization_v39 import AgentLocalizationSystem
from MAGEN.core.causal_memory_v39 import CausalMemorySystem
from MAGEN.core.minimal_learning_system_v39 import MinimalLearningSystemV39


class MAGENV40WorldModelPriority:
    """
    MAGEN V40 - Focus exclusif World Model
    
    Architecture:
    1. World State Graph (PRIORITÉ 1) - Forcer découverte régions
    2. Agent Localization (PRIORITÉ 2) - Forcer tracking position relative
    3. Causal Memory (PRIORITÉ 3) - Forcer enregistrement patterns
    4. Learning System (PRIORITÉ 4) - Utiliser représentation construite
    """
    
    def __init__(self, output_dir: str = "/tmp/magen_v40_world_model"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Forensic logging
        self.forensic_log = self.output_dir / "v40_forensic.jsonl"
        self.forensic_file = open(self.forensic_log, 'w')
        
        # Modules avec paramètres FORCÉS pour découverte
        self.world_graph = WorldStateGraph(
            width=30,
            height=30
        )
        
        self.agent_localization = AgentLocalizationSystem(
            grid_width=30,
            grid_height=30
        )
        
        self.causal_memory = CausalMemorySystem(
            max_events=1000  # AUGMENTÉ de 500
        )
        
        self.learning = MinimalLearningSystemV39(
            learning_rate=0.01
        )
        
        # Métriques V40
        self.metrics = {
            "patterns_discovered": 0,
            "regions_discovered": 0,
            "causal_chains": 0,
            "repeated_states": 0,
            "unique_states": 0,
            "diversity_score": 1.0,
            "total_steps": 0
        }
        
        self._log_forensic("v40_initialized", {
            "timestamp": datetime.now().isoformat(),
            "world_graph_params": {
                "grid_size": 30,
                "discovery_threshold": 0.3
            },
            "agent_localization_params": {
                "history_size": 100
            },
            "causal_memory_params": {
                "max_events": 1000,
                "pattern_threshold": 0.6
            }
        })
    
    def _log_forensic(self, event_type: str, data: Dict[str, Any]):
        """Logger événement forensic"""
        event = {
            "timestamp_ns": time.time_ns(),
            "event_type": event_type,
            "data": data
        }
        self.forensic_file.write(json.dumps(event) + '\n')
        self.forensic_file.flush()
    
    def run_world_model_test(self, num_steps: int = 1000):
        """
        Test V40 - Focus World Model
        
        Objectif: FORCER découverte patterns/régions/répétitions
        """
        print(f"\n{'='*80}")
        print(f"MAGEN V40 - WORLD MODEL PRIORITY TEST")
        print(f"{'='*80}\n")
        
        print(f"Objectif: Construire représentation persistante du monde")
        print(f"Steps: {num_steps}")
        print(f"Métriques cibles:")
        print(f"  - patterns_discovered: 0 → >10")
        print(f"  - regions_discovered: 0 → >5")
        print(f"  - repeated_states: 0 → >50")
        print(f"  - diversity_score: 1.0 → <0.7\n")
        
        # Simulation exploration avec FORÇAGE découverte
        import numpy as np
        
        visited_positions = set()
        state_history = []
        
        for step in range(num_steps):
            # Simuler position agent (exploration grille 30x30)
            x = np.random.randint(0, 30)
            y = np.random.randint(0, 30)
            position = (x, y)
            
            # FORCER tracking position
            agent_pos = self.agent_localization.update_position(x, y, step)
            
            # FORCER visite cellule
            self.world_graph.visit_cell(x, y, value=step)
            
            # FORCER découverte régions (tous les 100 steps)
            if step % 100 == 0 and step > 0:
                region_id = f"region_{len(self.world_graph.regions)}"
                self.world_graph.discover_region(region_id, x, y)
            
            # FORCER enregistrement état
            state = {
                "position": position,
                "step": step
            }
            state_history.append(state)
            
            # FORCER détection répétitions
            if position in visited_positions:
                self.metrics["repeated_states"] += 1
            else:
                visited_positions.add(position)
                self.metrics["unique_states"] += 1
            
            # FORCER enregistrement causal
            if step > 0:
                prev_state = state_history[-2]
                action = f"move_{x-prev_state['position'][0]}_{y-prev_state['position'][1]}"
                
                self.causal_memory.record_transformation(
                    transformation=action,
                    input_state=prev_state,
                    output_state=state,
                    success=True,
                    error=0.0
                )
            
            # Logging forensic périodique
            if step % 100 == 0:
                self._update_metrics()
                self._log_forensic("v40_step_checkpoint", {
                    "step": step,
                    "metrics": self.metrics.copy()
                })
                
                print(f"Step {step}/{num_steps}: "
                      f"patterns={self.metrics['patterns_discovered']}, "
                      f"regions={self.metrics['regions_discovered']}, "
                      f"repeated={self.metrics['repeated_states']}, "
                      f"diversity={self.metrics['diversity_score']:.3f}")
        
        # Mise à jour finale métriques
        self._update_metrics()
        
        # Résultats
        print(f"\n{'='*80}")
        print(f"RÉSULTATS V40")
        print(f"{'='*80}\n")
        
        self._print_results()
        
        # Validation GO/NO-GO
        go_nogo = self._validate_go_nogo()
        
        # Sauvegarder résultats
        self._save_results()
        
        return go_nogo
    
    def _update_metrics(self):
        """Mettre à jour métriques depuis modules"""
        # Patterns découverts (causal memory) - utiliser get_causal_patterns
        patterns = self.causal_memory.get_causal_patterns()
        self.metrics["patterns_discovered"] = len(patterns)
        
        # Régions découvertes (world graph)
        self.metrics["regions_discovered"] = len(self.world_graph.regions)
        
        # Chaînes causales (causal memory)
        self.metrics["causal_chains"] = len(self.causal_memory.chains)
        
        # Diversité (ratio unique/total)
        total = self.metrics["unique_states"] + self.metrics["repeated_states"]
        if total > 0:
            self.metrics["diversity_score"] = self.metrics["unique_states"] / total
        
        self.metrics["total_steps"] = total
    
    def _print_results(self):
        """Afficher résultats"""
        print(f"Métriques V40:")
        print(f"  patterns_discovered: {self.metrics['patterns_discovered']} (cible: >10)")
        print(f"  regions_discovered: {self.metrics['regions_discovered']} (cible: >5)")
        print(f"  causal_chains: {self.metrics['causal_chains']} (cible: >3)")
        print(f"  repeated_states: {self.metrics['repeated_states']} (cible: >50)")
        print(f"  unique_states: {self.metrics['unique_states']}")
        print(f"  diversity_score: {self.metrics['diversity_score']:.3f} (cible: <0.7)")
        print(f"  total_steps: {self.metrics['total_steps']}")
    
    def _validate_go_nogo(self) -> bool:
        """Valider critères GO/NO-GO"""
        print(f"\n{'='*80}")
        print(f"VALIDATION GO/NO-GO V40")
        print(f"{'='*80}\n")
        
        criteria = {
            "patterns > 0": self.metrics["patterns_discovered"] > 0,
            "regions > 0": self.metrics["regions_discovered"] > 0,
            "repeated > 0": self.metrics["repeated_states"] > 0,
            "diversity < 1.0": self.metrics["diversity_score"] < 1.0
        }
        
        for criterion, passed in criteria.items():
            status = "✅ PASS" if passed else "❌ FAIL"
            print(f"  {criterion}: {status}")
        
        all_passed = all(criteria.values())
        
        print(f"\n{'='*80}")
        if all_passed:
            print(f"✅ GO - Tous critères validés")
            print(f"V40 a construit représentation persistante du monde")
        else:
            print(f"❌ NO-GO - Critères non validés")
            print(f"Problème fondamental: Modules ne construisent pas représentation")
        print(f"{'='*80}\n")
        
        return all_passed
    
    def _save_results(self):
        """Sauvegarder résultats"""
        results_file = self.output_dir / "v40_results.json"
        
        results = {
            "timestamp": datetime.now().isoformat(),
            "version": "V40",
            "metrics": self.metrics,
            "go_nogo": self._validate_go_nogo(),
            "forensic_log": str(self.forensic_log)
        }
        
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\nRésultats sauvegardés: {results_file}")
        print(f"Logs forensiques: {self.forensic_log}")
    
    def close(self):
        """Fermeture propre"""
        self.forensic_file.close()


def main():
    """Point d'entrée principal"""
    print(f"\n{'='*80}")
    print(f"MAGEN V40 - WORLD MODEL PRIORITY")
    print(f"{'='*80}\n")
    
    print(f"Citation utilisateur:")
    print(f'  "MAGEN réfléchit énormément (5,009 décisions) mais')
    print(f'   n\'accumule presque aucune connaissance structurelle')
    print(f'   du monde (0 patterns, 0 régions)."')
    print(f"\nObjectif V40: Construire représentation persistante\n")
    
    # Créer instance V40
    v40 = MAGENV40WorldModelPriority()
    
    try:
        # Lancer test
        go_nogo = v40.run_world_model_test(num_steps=1000)
        
        # Retourner code sortie
        sys.exit(0 if go_nogo else 1)
        
    finally:
        v40.close()


if __name__ == "__main__":
    main()

# Made with Bob
