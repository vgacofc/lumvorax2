"""
Tests Unitaires - Phase 4.5.4 HierarchicalPlanner

Objectif: Valider planification hiérarchique avec A*

Tests:
1. Décomposition objectifs
2. Planification reach_position (A*)
3. Planification reach_entity
4. Recherche A* (chemin optimal)
5. Replanning après échec
6. Statistiques complètes

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.4
"""

import sys
import os
import numpy as np
from datetime import datetime
from dataclasses import dataclass

# Ajouter path pour imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from arc_integration.hierarchical_planner import HierarchicalPlanner, Goal, Plan


@dataclass
class MockEntity:
    """Entité simulée pour tests"""
    entity_id: str
    centroid: tuple
    size: int
    value: int


def create_mock_state(entities_data):
    """Créer état simulé"""
    entities = {}
    for entity_id, centroid, size, value in entities_data:
        entities[entity_id] = MockEntity(entity_id, centroid, size, value)
    return {'entities': entities}


def test_goal_decomposition():
    """Test 1: Décomposition objectifs"""
    print("\n" + "="*60)
    print("TEST 1: DÉCOMPOSITION OBJECTIFS")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer objectif reach_entity
    goal = Goal(
        type="reach_entity",
        target="door_3#1",
        priority=1.0
    )
    
    # Décomposer
    sub_goals = planner.decompose_goal(goal)
    
    print(f"\n✓ Objectif principal: {goal.type}")
    print(f"✓ Sous-objectifs générés: {len(sub_goals)}")
    
    for i, sub_goal in enumerate(sub_goals):
        print(f"  {i+1}. {sub_goal.type} (priorité: {sub_goal.priority:.2f})")
    
    assert len(sub_goals) >= 2, "Au moins 2 sous-objectifs attendus"
    assert goal.sub_goals == sub_goals, "Sous-objectifs doivent être liés au goal parent"
    
    print("\n✅ TEST 1 RÉUSSI: Décomposition objectifs fonctionnelle")
    return True


def test_plan_reach_position():
    """Test 2: Planification reach_position"""
    print("\n" + "="*60)
    print("TEST 2: PLANIFICATION REACH_POSITION")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer état avec avatar
    state = create_mock_state([
        ("avatar", (10.0, 10.0), 100, 9),
        ("door", (50.0, 50.0), 892, 3)
    ])
    
    # Debug: vérifier position avatar
    avatar_pos = planner.get_avatar_position(state)
    print(f"\n✓ Position avatar détectée: {avatar_pos}")
    
    # Créer objectif reach_position
    goal = Goal(
        type="reach_position",
        target=(50.0, 50.0),
        priority=1.0
    )
    
    # Planifier
    plan = planner.plan_to_goal(goal, state)
    
    print(f"✓ Plan généré: {plan is not None}")
    
    if plan:
        print(f"✓ Actions: {len(plan.actions)}")
        print(f"✓ Coût: {plan.cost:.2f}")
        print(f"✓ Confiance: {plan.confidence:.2%}")
        print(f"✓ Premières actions: {plan.actions[:5]}")
    
    assert plan is not None, "Plan devrait être généré"
    assert len(plan.actions) > 0, "Plan devrait contenir actions"
    assert plan.cost > 0, "Coût devrait être positif"
    
    print("\n✅ TEST 2 RÉUSSI: Planification reach_position fonctionnelle")
    return True


def test_plan_reach_entity():
    """Test 3: Planification reach_entity"""
    print("\n" + "="*60)
    print("TEST 3: PLANIFICATION REACH_ENTITY")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer état avec avatar et porte
    state = create_mock_state([
        ("avatar", (10.0, 10.0), 100, 9),
        ("door_3#1", (50.0, 50.0), 892, 3)
    ])
    
    # Créer objectif reach_entity
    goal = Goal(
        type="reach_entity",
        target="door_3#1",
        priority=1.0
    )
    
    # Planifier
    plan = planner.plan_to_goal(goal, state)
    
    print(f"\n✓ Plan généré: {plan is not None}")
    
    if plan:
        print(f"✓ Actions: {len(plan.actions)}")
        print(f"✓ Coût: {plan.cost:.2f}")
        print(f"✓ Confiance: {plan.confidence:.2%}")
    
    assert plan is not None, "Plan devrait être généré"
    assert len(plan.actions) > 0, "Plan devrait contenir actions"
    
    print("\n✅ TEST 3 RÉUSSI: Planification reach_entity fonctionnelle")
    return True


def test_astar_search():
    """Test 4: Recherche A* (chemin optimal)"""
    print("\n" + "="*60)
    print("TEST 4: RECHERCHE A* (CHEMIN OPTIMAL)")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer état simple
    state = create_mock_state([
        ("avatar", (10.0, 10.0), 100, 9)
    ])
    
    # Recherche A* de (10,10) à (20,20)
    start_pos = (10.0, 10.0)
    goal_pos = (20.0, 20.0)
    
    path = planner.astar_search(start_pos, goal_pos, state)
    
    print(f"\n✓ Chemin trouvé: {path is not None}")
    
    if path:
        print(f"✓ Longueur chemin: {len(path)} positions")
        print(f"✓ Départ: {path[0]}")
        print(f"✓ Arrivée: {path[-1]}")
        
        # Vérifier distance finale
        final_distance = np.sqrt((path[-1][0] - goal_pos[0])**2 + 
                                 (path[-1][1] - goal_pos[1])**2)
        print(f"✓ Distance finale: {final_distance:.2f} pixels")
    
    assert path is not None, "Chemin devrait être trouvé"
    assert len(path) >= 2, "Chemin devrait avoir au moins 2 positions"
    
    # Vérifier que chemin se rapproche de l'objectif
    if path:
        start_distance = np.sqrt((start_pos[0] - goal_pos[0])**2 + 
                                 (start_pos[1] - goal_pos[1])**2)
        final_distance = np.sqrt((path[-1][0] - goal_pos[0])**2 + 
                                 (path[-1][1] - goal_pos[1])**2)
        assert final_distance < start_distance, "Chemin devrait se rapprocher de l'objectif"
    
    print("\n✅ TEST 4 RÉUSSI: Recherche A* fonctionnelle")
    return True


def test_replanning():
    """Test 5: Replanning après échec"""
    print("\n" + "="*60)
    print("TEST 5: REPLANNING APRÈS ÉCHEC")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer état
    state = create_mock_state([
        ("avatar", (10.0, 10.0), 100, 9),
        ("door", (50.0, 50.0), 892, 3)
    ])
    
    # Créer objectif et plan initial
    goal = Goal(
        type="reach_position",
        target=(50.0, 50.0),
        priority=1.0,
        status="active"
    )
    
    planner.active_goal = goal
    initial_plan = planner.plan_to_goal(goal, state)
    planner.current_plan = initial_plan
    
    initial_replanning_count = planner.replanning_count
    
    # Déclencher replanning
    planner.replan(state, reason="test_failure")
    
    print(f"\n✓ Replanning exécuté: {planner.replanning_count > initial_replanning_count}")
    print(f"✓ Compteur replanning: {planner.replanning_count}")
    print(f"✓ Nouveau plan: {planner.current_plan is not None}")
    
    assert planner.replanning_count > initial_replanning_count, "Replanning devrait incrémenter compteur"
    
    print("\n✅ TEST 5 RÉUSSI: Replanning fonctionnel")
    return True


def test_statistics():
    """Test 6: Statistiques complètes"""
    print("\n" + "="*60)
    print("TEST 6: STATISTIQUES COMPLÈTES")
    print("="*60)
    
    planner = HierarchicalPlanner()
    
    # Créer état
    state = create_mock_state([
        ("avatar", (10.0, 10.0), 100, 9),
        ("door", (50.0, 50.0), 892, 3)
    ])
    
    # Ajouter plusieurs objectifs
    for i in range(3):
        goal = Goal(
            type="reach_position",
            target=(20.0 + i*10, 20.0 + i*10),
            priority=1.0 - i*0.1
        )
        planner.add_goal(goal)
    
    # Générer plans
    for goal in planner.goals:
        plan = planner.plan_to_goal(goal, state)
        if plan:
            goal.status = "completed"
            planner.plans_succeeded += 1
    
    # Obtenir statistiques
    stats = planner.get_statistics()
    
    print(f"\n✓ Objectifs totaux: {stats['goals_total']}")
    print(f"✓ Objectifs complétés: {stats['goals_completed']}")
    print(f"✓ Plans générés: {stats['plans_generated']}")
    print(f"✓ Plans réussis: {stats['plans_succeeded']}")
    print(f"✓ Replanning: {stats['replanning_count']}")
    
    print(f"\n✓ Représentation: {planner}")
    
    assert stats['goals_total'] == 3, "3 objectifs attendus"
    assert stats['plans_generated'] >= 3, "Au moins 3 plans générés"
    
    print("\n✅ TEST 6 RÉUSSI: Statistiques complètes")
    return True


def generate_report(results):
    """Générer rapport markdown"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = f"RAPPORT_PHASE_4_5_4_PLANNER_TEST_{timestamp}.md"
    
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("# RAPPORT TEST PHASE 4.5.4 - HIERARCHICAL PLANNER\n\n")
        f.write(f"**Date**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        f.write("## Résultats Tests\n\n")
        
        total = len(results)
        passed = sum(1 for r in results.values() if r)
        
        f.write(f"- **Tests exécutés**: {total}\n")
        f.write(f"- **Tests réussis**: {passed}\n")
        f.write(f"- **Taux succès**: {passed/total*100:.1f}%\n\n")
        
        f.write("## Détails Tests\n\n")
        for test_name, success in results.items():
            status = "✅ RÉUSSI" if success else "❌ ÉCHOUÉ"
            f.write(f"- **{test_name}**: {status}\n")
        
        f.write("\n## Conclusion\n\n")
        if passed == total:
            f.write("✅ **TOUS LES TESTS RÉUSSIS**\n\n")
            f.write("HierarchicalPlanner opérationnel:\n")
            f.write("- Décomposition objectifs fonctionnelle\n")
            f.write("- Planification reach_position fonctionnelle\n")
            f.write("- Planification reach_entity fonctionnelle\n")
            f.write("- Recherche A* fonctionnelle\n")
            f.write("- Replanning fonctionnel\n")
            f.write("- Statistiques complètes\n\n")
            f.write("**Prêt pour Phase 4.5.4 suite (IntelligentExploration)**\n")
        else:
            f.write(f"⚠️ **{total - passed} TEST(S) ÉCHOUÉ(S)**\n\n")
            f.write("Corrections nécessaires avant suite\n")
    
    print(f"\n📄 Rapport généré: {report_path}")
    return report_path


def main():
    """Exécuter tous les tests"""
    print("\n" + "="*60)
    print("PHASE 4.5.4 - TESTS HIERARCHICAL PLANNER")
    print("="*60)
    print("\nObjectif: Valider planification hiérarchique avec A*")
    print("- Décomposition objectifs")
    print("- Planification reach_position")
    print("- Planification reach_entity")
    print("- Recherche A*")
    print("- Replanning")
    print("- Statistiques")
    
    results = {}
    
    try:
        results['Test 1: Décomposition'] = test_goal_decomposition()
        results['Test 2: Reach Position'] = test_plan_reach_position()
        results['Test 3: Reach Entity'] = test_plan_reach_entity()
        results['Test 4: A* Search'] = test_astar_search()
        results['Test 5: Replanning'] = test_replanning()
        results['Test 6: Statistiques'] = test_statistics()
        
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    # Générer rapport
    report_path = generate_report(results)
    
    # Résumé final
    print("\n" + "="*60)
    print("RÉSUMÉ FINAL")
    print("="*60)
    
    total = len(results)
    passed = sum(1 for r in results.values() if r)
    
    print(f"\n✓ Tests exécutés: {total}")
    print(f"✓ Tests réussis: {passed}")
    print(f"✓ Taux succès: {passed/total*100:.1f}%")
    
    if passed == total:
        print("\n✅ TOUS LES TESTS RÉUSSIS")
        print("\n🚀 HierarchicalPlanner opérationnel")
        print("📋 Prêt pour Phase 4.5.4 suite (IntelligentExploration)")
        return 0
    else:
        print(f"\n⚠️ {total - passed} TEST(S) ÉCHOUÉ(S)")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
