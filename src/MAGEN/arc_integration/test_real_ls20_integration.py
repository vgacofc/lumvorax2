"""
Test Réel ls20 - Intégration Complète Architecture Systémique

Objectif: Tester MAGEN sur environnement ls20 RÉEL OFFICIEL

Architecture testée:
- Couche 1: WorldModel (cartographie)
- Couche 2: SelfIdentification (avatar)
- Couche 3: RulesLearning (règles)
- Couche 4: WorldDynamics (dynamiques)
- Couche 5: HierarchicalPlanner (planification)

Test: 1 épisode complet sur ls20 niveau 1

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.4-REAL
"""

import sys
import os
import numpy as np
from datetime import datetime

# Ajouter paths avec path absolu
workspace_root = '/home/lvx/LVX'
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, os.path.join(workspace_root, 'environment_files/ls20/9607627b'))

# Imports architecture systémique
try:
    from arc_integration.world_model import WorldModel
except ImportError:
    # Fallback: version simple sans NetworkX
    from arc_integration.world_model_simple import WorldModel
    print("⚠️ Utilisation world_model_simple (sans NetworkX)")

from arc_integration.self_identification import SelfIdentification
from arc_integration.rules_learning import RulesLearning
from arc_integration.world_dynamics import WorldDynamics
from arc_integration.hierarchical_planner import HierarchicalPlanner, Goal

# Import environnement ls20 - Utiliser wrapper simplifié (sans NetworkX/scipy)
try:
    from ls20_wrapper_simple import LS20EnvSimple as LS20Env
    LS20_AVAILABLE = True
    print("✓ Utilisation ls20_wrapper_simple (sans dépendances externes)")
except ImportError:
    print("❌ ERREUR: ls20_wrapper_simple non disponible")
    LS20_AVAILABLE = False


class MAGENSystemicAgentReal:
    """
    Agent MAGEN avec architecture systémique complète
    
    Test RÉEL sur ls20
    """
    
    def __init__(self):
        # Couches systémiques
        self.world_model = WorldModel()
        self.self_identification = SelfIdentification()
        self.rules_learning = RulesLearning()
        self.world_dynamics = WorldDynamics()
        self.planner = HierarchicalPlanner(
            world_model=self.world_model,
            rules_learning=self.rules_learning,
            world_dynamics=self.world_dynamics
        )
        
        # Statistiques
        self.episode_count = 0
        self.total_reward = 0.0
        self.total_steps = 0
        self.victories = 0
        
        # Historique
        self.state_history = []
        self.action_history = []
        self.reward_history = []
    
    def reset(self):
        """Reset pour nouvel épisode"""
        self.episode_count += 1
        self.state_history = []
        self.action_history = []
        self.reward_history = []
    
    def observe_state(self, observation):
        """
        Observer état complet
        
        Utilise WorldModel pour extraire entités
        """
        # Convertir observation en grille
        if isinstance(observation, dict):
            grid = observation.get('grid', observation.get('image', None))
        else:
            grid = observation
        
        if grid is None:
            return None
        
        # Observer avec WorldModel
        self.world_model.observe_complete_state(grid)
        
        # Créer état structuré
        state = {
            'grid': grid,
            'entities': self.world_model.entities,
            'step': len(self.state_history)
        }
        
        self.state_history.append(state)
        return state
    
    def select_action(self, state):
        """
        Sélectionner action
        
        Utilise HierarchicalPlanner si objectif défini, sinon exploration
        """
        # Si pas de plan actif, créer objectif
        if self.planner.current_plan is None or not self.planner.current_plan.actions:
            # Objectif: atteindre porte (entité 3#1)
            goal = Goal(
                type="reach_entity",
                target="3#1",  # Porte dans ls20
                priority=1.0
            )
            
            # Planifier
            plan = self.planner.plan_to_goal(goal, state)
            
            if plan:
                self.planner.current_plan = plan
                self.planner.active_goal = goal
            else:
                # Pas de plan, exploration aléatoire
                return np.random.randint(0, 6)
        
        # Exécuter plan
        if self.planner.current_plan and self.planner.current_plan.actions:
            action = self.planner.execute_plan(self.planner.current_plan)
            return action
        
        # Fallback: exploration
        return np.random.randint(0, 6)
    
    def learn_from_transition(self, state, action, next_state, reward, done):
        """
        Apprendre depuis transition
        
        Met à jour toutes les couches
        """
        # SelfIdentification
        if len(self.state_history) >= 2:
            entities_before = self.state_history[-2].get('entities', {})
            entities_after = state.get('entities', {})
            self.self_identification.update(action, entities_before, entities_after)
        
        # RulesLearning
        if len(self.state_history) >= 2:
            entities_before = self.state_history[-2].get('entities', {})
            entities_after = state.get('entities', {})
            self.rules_learning.learn_rule(action, entities_before, entities_after, reward)
        
        # WorldDynamics (apprend sur séquences)
        if len(self.state_history) >= 10:
            recent_states = self.state_history[-10:]
            recent_actions = self.action_history[-9:]
            self.world_dynamics.learn_dynamics(recent_states, recent_actions)
        
        # Statistiques
        self.total_reward += reward
        self.total_steps += 1
        
        if done and reward > 0:
            self.victories += 1
    
    def get_statistics(self):
        """Obtenir statistiques complètes"""
        return {
            'episodes': self.episode_count,
            'total_steps': self.total_steps,
            'total_reward': self.total_reward,
            'victories': self.victories,
            'avg_reward': self.total_reward / max(self.episode_count, 1),
            'world_model': {
                'entities': len(self.world_model.entities),
                'spatial_graph_nodes': len(self.world_model.spatial_graph.nodes) if hasattr(self.world_model.spatial_graph, 'nodes') else 0
            },
            'self_identification': {
                'avatar_identified': self.self_identification.avatar_id is not None,
                'confidence': max(self.self_identification.confidence_scores.values()) if self.self_identification.confidence_scores else 0.0
            },
            'rules_learning': {
                'rules_learned': len(self.rules_learning.rules),
                'observations': sum(rule.observations for rule in self.rules_learning.rules.values())
            },
            'world_dynamics': self.world_dynamics.get_statistics(),
            'planner': self.planner.get_statistics()
        }


def run_real_test_ls20(max_steps=100):
    """
    Exécuter test RÉEL sur ls20
    
    1 épisode complet
    """
    print("\n" + "="*60)
    print("TEST RÉEL LS20 - ARCHITECTURE SYSTÉMIQUE COMPLÈTE")
    print("="*60)
    
    if not LS20_AVAILABLE:
        print("\n❌ ERREUR: ls20.py non disponible")
        print("Vérifier path: environment_files/ls20/9607627b/ls20.py")
        return None
    
    # Créer environnement
    try:
        env = LS20Env()
        print("\n✓ Environnement ls20 chargé")
    except Exception as e:
        print(f"\n❌ ERREUR chargement ls20: {e}")
        return None
    
    # Créer agent
    agent = MAGENSystemicAgentReal()
    print("✓ Agent MAGEN systémique créé")
    
    # Reset
    observation = env.reset()
    agent.reset()
    
    print(f"\n✓ Épisode 1 démarré")
    print(f"  Observation shape: {observation.shape if hasattr(observation, 'shape') else type(observation)}")
    
    # Boucle épisode
    done = False
    step = 0
    
    while not done and step < max_steps:
        # Observer
        state = agent.observe_state(observation)
        
        if state is None:
            print(f"\n⚠️ Step {step}: État None, skip")
            action = 0
        else:
            # Sélectionner action
            action = agent.select_action(state)
        
        # Exécuter
        try:
            next_observation, reward, done, info = env.step(action)
        except Exception as e:
            print(f"\n❌ ERREUR step {step}: {e}")
            break
        
        # Observer état suivant
        next_state = agent.observe_state(next_observation)
        
        # Apprendre
        if state and next_state:
            agent.learn_from_transition(state, action, next_state, reward, done)
        
        # Enregistrer
        agent.action_history.append(action)
        agent.reward_history.append(reward)
        
        # Affichage périodique
        if step % 10 == 0:
            print(f"  Step {step}: action={action}, reward={reward:.3f}, done={done}")
        
        # Préparer prochain step
        observation = next_observation
        step += 1
    
    # Résultats
    print("\n" + "="*60)
    print("RÉSULTATS TEST RÉEL")
    print("="*60)
    
    stats = agent.get_statistics()
    
    print(f"\n✓ Épisode terminé")
    print(f"  Steps: {step}")
    print(f"  Reward total: {agent.total_reward:.3f}")
    print(f"  Victoire: {done and agent.total_reward > 0}")
    
    print(f"\n✓ WorldModel:")
    print(f"  Entités détectées: {stats['world_model']['entities']}")
    
    print(f"\n✓ SelfIdentification:")
    print(f"  Avatar identifié: {stats['self_identification']['avatar_identified']}")
    print(f"  Confiance: {stats['self_identification']['confidence']:.2%}")
    
    print(f"\n✓ RulesLearning:")
    print(f"  Règles apprises: {stats['rules_learning']['rules_learned']}")
    print(f"  Observations: {stats['rules_learning']['observations']}")
    
    print(f"\n✓ WorldDynamics:")
    print(f"  Invariants: {stats['world_dynamics']['invariants_detected']}")
    print(f"  Cycles: {stats['world_dynamics']['cycles_detected']}")
    print(f"  Transitions: {stats['world_dynamics']['transitions_modeled']}")
    
    print(f"\n✓ HierarchicalPlanner:")
    print(f"  Plans générés: {stats['planner']['plans_generated']}")
    print(f"  Replanning: {stats['planner']['replanning_count']}")
    
    return stats


def generate_report(stats):
    """Générer rapport test réel"""
    if stats is None:
        return
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = f"RAPPORT_TEST_REEL_LS20_{timestamp}.md"
    
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("# RAPPORT TEST RÉEL LS20 - ARCHITECTURE SYSTÉMIQUE\n\n")
        f.write(f"**Date**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        f.write("## Configuration\n\n")
        f.write("- **Environnement**: ls20 (ARC-AGI niveau 1)\n")
        f.write("- **Architecture**: 5 couches systémiques\n")
        f.write("- **Épisodes**: 1\n")
        f.write("- **Max steps**: 100\n\n")
        
        f.write("## Résultats\n\n")
        f.write(f"- **Steps exécutés**: {stats['total_steps']}\n")
        f.write(f"- **Reward total**: {stats['total_reward']:.3f}\n")
        f.write(f"- **Victoires**: {stats['victories']}\n\n")
        
        f.write("## Couches Systémiques\n\n")
        
        f.write("### WorldModel\n")
        f.write(f"- Entités: {stats['world_model']['entities']}\n\n")
        
        f.write("### SelfIdentification\n")
        f.write(f"- Avatar identifié: {stats['self_identification']['avatar_identified']}\n")
        f.write(f"- Confiance: {stats['self_identification']['confidence']:.2%}\n\n")
        
        f.write("### RulesLearning\n")
        f.write(f"- Règles: {stats['rules_learning']['rules_learned']}\n")
        f.write(f"- Observations: {stats['rules_learning']['observations']}\n\n")
        
        f.write("### WorldDynamics\n")
        f.write(f"- Invariants: {stats['world_dynamics']['invariants_detected']}\n")
        f.write(f"- Cycles: {stats['world_dynamics']['cycles_detected']}\n")
        f.write(f"- Transitions: {stats['world_dynamics']['transitions_modeled']}\n\n")
        
        f.write("### HierarchicalPlanner\n")
        f.write(f"- Plans: {stats['planner']['plans_generated']}\n")
        f.write(f"- Replanning: {stats['planner']['replanning_count']}\n\n")
        
        f.write("## Conclusion\n\n")
        if stats['victories'] > 0:
            f.write("✅ **VICTOIRE** - Premier succès architecture systémique !\n")
        else:
            f.write("⚠️ **Pas de victoire** - Apprentissage en cours\n")
            f.write("\nProchaines étapes:\n")
            f.write("- Analyser règles apprises\n")
            f.write("- Affiner planification\n")
            f.write("- Augmenter nombre épisodes\n")
    
    print(f"\n📄 Rapport généré: {report_path}")


def main():
    """Point d'entrée"""
    stats = run_real_test_ls20(max_steps=100)
    
    if stats:
        generate_report(stats)
        return 0
    else:
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
