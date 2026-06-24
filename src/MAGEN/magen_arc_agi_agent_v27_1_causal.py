"""
MAGEN V27.1 CAUSAL - Agent avec Reconstruction Monde
=====================================================

Évolution majeure:
    V27:   Agent qui agit sur représentation fausse
    V27.1: Agent qui reconstruit monde correct via inférence causale

Nouveaux modules:
    - BehavioralEntityDetector: Inférence joueur par causalité
    - ProgressiveRewardShaper: Reward dense 6 couches
    - ContextualActionValidator: Erreur API → Signal cognitif

Architecture:
    Observation → Inférence Causale → Reconstruction Monde →
    Validation Actions → Décision → Reward Dense → Apprentissage

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy as np
from typing import Dict, List, Tuple, Optional
import time
import json
import os

# Import V27 base
from magen_arc_agi_agent_v27_cognitive import MAGENArcAgiAgentV27Cognitive

# Import nouveaux modules V27.1
from core.behavioral_entity_detector import BehavioralEntityDetector
from core.progressive_reward_shaper import ProgressiveRewardShaper
from core.contextual_action_validator import ContextualActionValidator, ActionContext


class MAGENArcAGIAgentV271(MAGENArcAgiAgentV27Cognitive):
    """
    Agent MAGEN V27.1 CAUSAL
    
    Hérite de V27 et ajoute:
    - Inférence causale joueur
    - Reward dense
    - Validation contextuelle actions
    """
    
    def __init__(self, forensic_log_path: Optional[str] = None, *args, **kwargs):
        """Initialise agent V27.1 avec modules causaux"""
        # FORENSIC: Passer forensic_log_path à parent V27 qui a ForensicMixin
        super().__init__(forensic_log_path=forensic_log_path, *args, **kwargs)
        
        # Nouveaux modules V27.1
        self.entity_detector = BehavioralEntityDetector(
            causality_threshold=0.6,
            mobility_threshold=0.3,
            persistence_threshold=3
        )
        
        self.reward_shaper = ProgressiveRewardShaper(
            exploration_weight=0.05,
            novelty_weight=0.10,
            topology_weight=0.20,
            causality_weight=0.30,
            progress_weight=0.50,
            completion_weight=10.0
        )
        
        self.action_validator = ContextualActionValidator(
            confidence_threshold=0.7,
            min_samples=3
        )
        
        print("[MAGEN V27.1 CAUSAL] Agent initialisé")
        print("[V27.1] BehavioralEntityDetector: ENABLED")
        print("[V27.1] ProgressiveRewardShaper: ENABLED")
        print("[V27.1] ContextualActionValidator: ENABLED")
        print(f"[V27.1] Forensic Logging: {'ENABLED' if self.forensic else 'DISABLED'}")
    
    def perceive(self, obs) -> Dict:
        """
        Perception enrichie avec inférence causale.
        
        Étend V27.perceive() avec:
        - Détection entités comportementale
        - Inférence joueur par causalité
        """
        timestamp_start = int(time.time_ns())
        
        # FORENSIC: Log perception start (via middleware hérité de V27)
        if self.forensic:
            self.forensic.log_event('perception_v27_1_start', {
                'timestamp_ns': timestamp_start
            })
        
        # Perception V27 base (retourne GridObservation dataclass)
        grid_obs = super().perceive(obs)
        
        # Convertir GridObservation en Dict pour V27.1
        from dataclasses import asdict
        perception = asdict(grid_obs)
        
        # CRITIQUE: NE PAS réécrire perception['grid']!
        # V27 cognitive a déjà copié le grid de manière immuable.
        # Utiliser directement le grid depuis perception, pas depuis obs.
        grid = perception['grid']
        
        # Vérifier que c'est bien un ndarray
        if not isinstance(grid, np.ndarray):
            grid = np.array(grid)
        
        # Observation entités avec inférence causale
        entity_obs = self.entity_detector.observe(
            grid=grid,
            action=None  # Action sera fournie dans learn()
        )
        
        # Enrichir perception avec détection causale
        perception['agent_detected_causal'] = entity_obs['agent_detected']
        perception['agent_id'] = entity_obs['agent_id']
        perception['agent_confidence'] = entity_obs['agent_confidence']
        perception['agent_position_causal'] = entity_obs['agent_position']
        perception['mobile_entities'] = entity_obs['mobile_entities']
        perception['stable_entities'] = entity_obs['stable_entities']
        
        # Si joueur détecté par causalité, utiliser cette position
        if entity_obs['agent_detected'] and entity_obs['agent_position']:
            perception['player_position'] = entity_obs['agent_position']
            perception['player_detected'] = True
        
        # FORENSIC: Log perception complete (via middleware hérité de V27)
        timestamp_end = int(time.time_ns())
        if self.forensic:
            self.forensic.log_event('perception_v27_1_complete', {
                'timestamp_ns': timestamp_end,
                'duration_ns': timestamp_end - timestamp_start,
                'agent_detected_causal': perception['agent_detected_causal'],
                'agent_confidence': perception['agent_confidence'],
                'mobile_entities': perception['mobile_entities'],
                'stable_entities': perception['stable_entities']
            })
        
        return perception
    
    def decide(self, perception: Dict, available_actions: List[str]) -> Dict:
        """
        Décision avec validation contextuelle actions.
        
        Étend V27.decide() avec:
        - Filtrage actions invalides par contexte
        - Priorisation actions valides
        """
        # Créer contexte pour validation
        grid = perception.get('grid')
        if grid is not None and isinstance(grid, np.ndarray):
            grid_hash = self.action_validator._hash_grid(grid)
        else:
            grid_hash = "unknown"
        
        context = ActionContext(
            grid_hash=grid_hash,
            agent_position=perception.get('agent_position_causal'),
            goal_position=perception.get('goal_position'),
            entities_count=perception.get('mobile_entities', 0) + perception.get('stable_entities', 0),
            state_name=perception.get('state', 'UNKNOWN'),
            timestamp=int(time.time_ns())
        )
        
        # Filtrer actions valides
        valid_actions = self.action_validator.get_valid_actions(
            available_actions,
            context
        )
        
        # Si aucune action valide, utiliser toutes (exploration)
        if not valid_actions:
            valid_actions = [(a, 0.5) for a in available_actions]
        
        # Extraire noms actions
        filtered_actions = [a for a, conf in valid_actions]
        
        # Reconvertir Dict → GridObservation pour appel parent
        from dataclasses import asdict
        from magen_arc_agi_agent_v27_cognitive import GridObservation
        grid_obs_for_parent = GridObservation(
            grid=perception.get('grid'),
            embedding=perception.get('embedding'),
            spatial_analysis=perception.get('spatial_analysis'),
            patterns=perception.get('patterns', []),
            objects=perception.get('objects', []),
            metadata=perception.get('metadata', {})
        )
        
        # Décision V27 sur actions filtrées
        decision_obj = super().decide(grid_obs_for_parent, filtered_actions)
        
        # Convertir ActionDecision → Dict
        decision = asdict(decision_obj)
        
        # Enrichir avec info validation
        decision['action_validated'] = decision['action_name'] in filtered_actions
        decision['validation_confidence'] = next(
            (conf for a, conf in valid_actions if a == decision['action_name']),
            0.0
        )
        
        # FORENSIC: Log decision (via middleware hérité de V27)
        if self.forensic:
            self.forensic.log_event('decision_v27_1_validated', {
                'timestamp_ns': int(time.time_ns()),
                'action_name': decision['action_name'],
                'action_validated': decision['action_validated'],
                'validation_confidence': decision['validation_confidence'],
                'valid_actions_count': len(valid_actions)
            })
        
        return decision
    
    def learn(
        self,
        state_before: Dict,
        action: str,
        state_after: Dict,
        reward_env: float,
        done: bool
    ):
        """
        Apprentissage avec reward dense et validation contextuelle.
        
        Étend V27.learn() avec:
        - Reward shaping dense 6 couches
        - Apprentissage validité actions
        - Mise à jour inférence causale
        
        SPEC: Convertir Dict → GridObservation pour compatibilité V27
        """
        timestamp_start = int(time.time_ns())
        
        # Extraire grilles
        grid_before = state_before.get('grid')
        grid_after = state_after.get('grid')
        
        # Informer entity detector de l'action
        if grid_after is not None:
            self.entity_detector.observe(grid_after, action)
        
        # Calculer reward dense
        if grid_before is not None and grid_after is not None:
            agent_info = self.entity_detector.get_agent_info()
            
            reward_breakdown = self.reward_shaper.compute_reward(
                grid_before=grid_before,
                grid_after=grid_after,
                action=action,
                agent_pos_before=state_before.get('agent_position_causal'),
                agent_pos_after=state_after.get('agent_position_causal'),
                goal_pos=state_after.get('goal_position'),
                state_name=state_after.get('state', 'UNKNOWN')
            )
            
            # Utiliser reward dense au lieu de reward environnement
            reward_total = reward_breakdown.total_reward
        else:
            reward_total = reward_env
        
        # Apprendre validité action
        if grid_after is not None and isinstance(grid_after, np.ndarray):
            grid_hash = self.action_validator._hash_grid(grid_after)
        else:
            grid_hash = "unknown"
        context = ActionContext(
            grid_hash=grid_hash,
            agent_position=state_after.get('agent_position_causal'),
            goal_position=state_after.get('goal_position'),
            entities_count=state_after.get('mobile_entities', 0) + state_after.get('stable_entities', 0),
            state_name=state_after.get('state', 'UNKNOWN'),
            timestamp=int(time.time_ns())
        )
        
        # Succès = pas d'erreur ET reward positif
        success = reward_total > 0
        
        self.action_validator.learn_from_execution(
            action=action,
            context=context,
            success=success,
            error_code=None,  # Géré par runner
            error_message=None
        )
        
        # FORENSIC: Log learning (via middleware hérité de V27)
        if self.forensic:
            self.forensic.log_event('learning_v27_1_complete', {
                'timestamp_ns': int(time.time_ns()),
                'action': action,
                'reward_env': float(reward_env),
                'reward_total': float(reward_total),
                'done': done,
                'success': success
            })
        
        # Convertir Dict → GridObservation pour appel parent V27
        from dataclasses import asdict
        from magen_arc_agi_agent_v27_cognitive import GridObservation
        
        grid_obs_before = GridObservation(
            grid=grid_before if grid_before is not None else np.array([]),
            embedding=state_before.get('embedding'),
            spatial_analysis=state_before.get('spatial_analysis'),
            patterns=state_before.get('patterns', []),
            objects=state_before.get('objects', []),
            metadata=state_before.get('metadata', {})
        )
        
        grid_obs_after = GridObservation(
            grid=grid_after if grid_after is not None else np.array([]),
            embedding=state_after.get('embedding'),
            spatial_analysis=state_after.get('spatial_analysis'),
            patterns=state_after.get('patterns', []),
            objects=state_after.get('objects', []),
            metadata=state_after.get('metadata', {})
        )
        
        # Apprentissage V27 avec reward dense
        super().learn(grid_obs_before, action, grid_obs_after, reward_total, done)
    
    def reset(self):
        """Réinitialise agent pour nouveau jeu"""
        super().reset()
        self.entity_detector.reset()
        self.reward_shaper.reset()
        self.action_validator.reset()
        print("[MAGEN V27.1 CAUSAL] Agent réinitialisé pour nouveau jeu")
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques complètes V27.1"""
        # V27 cognitive n'a pas get_statistics(), on construit from scratch
        stats = {
            'v27_cognitive': {
                'stats': self.stats.copy() if hasattr(self, 'stats') else {},
                'graph_nodes': len(self.causal_graph.nodes) if hasattr(self, 'causal_graph') else 0,
                'graph_edges': len(self.causal_graph.edges) if hasattr(self, 'causal_graph') else 0,
                'reputation_enabled': self.reputation_enabled if hasattr(self, 'reputation_enabled') else False
            },
            'v27_1': {
                'entity_detector': self.entity_detector.get_world_model(),
                'reward_shaper': self.reward_shaper.get_statistics(),
                'action_validator': self.action_validator.get_statistics()
            }
        }
        
        return stats
    
    # FORENSIC: Méthodes supprimées - utilise middleware hérité de V27 via ForensicMixin
    # Les logs sont maintenant gérés par self.forensic (ForensicMiddleware)


# Test rapide
if __name__ == "__main__":
    print("="*80)
    print("MAGEN V27.1 CAUSAL - Test Initialisation")
    print("="*80)
    
    agent = MAGENArcAGIAgentV271()
    
    print("\n✅ Agent V27.1 initialisé avec succès")
    print(f"✅ Modules V27: {len([m for m in dir(agent) if not m.startswith('_')])}")
    print(f"✅ BehavioralEntityDetector: Active")
    print(f"✅ ProgressiveRewardShaper: Active")
    print(f"✅ ContextualActionValidator: Active")
    
    # Test perception simple
    test_grid = np.random.randint(0, 10, (10, 10))
    test_obs = {'grid': test_grid, 'state': 'NOT_FINISHED'}
    
    perception = agent.perceive(test_obs)
    print(f"\n✅ Perception test: {len(perception)} attributs")
    print(f"   - Agent détecté (causal): {perception.get('agent_detected_causal', False)}")
    print(f"   - Entités mobiles: {perception.get('mobile_entities', 0)}")
    print(f"   - Entités stables: {perception.get('stable_entities', 0)}")
    
    print("\n" + "="*80)
    print("✅ MAGEN V27.1 CAUSAL - Prêt pour exécution réelle")
    print("="*80)

# Made with Bob
