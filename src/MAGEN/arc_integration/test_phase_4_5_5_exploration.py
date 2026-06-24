"""
Tests Unitaires - IntelligentExploration (Couche 6)
Phase 4.5.5 - Architecture Systémique MAGEN

Tests:
1. Epsilon-greedy adaptatif
2. Calcul priorités exploration
3. Sélection zones cibles
4. Détection cycles
5. Enregistrement visites
6. Statistiques exploration

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.5-TEST
"""

import sys
import os
import numpy as np
from datetime import datetime

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

# Imports
from intelligent_exploration import IntelligentExploration, ExplorationZone
from world_model_simple import WorldModel
from world_dynamics import WorldDynamics
from hierarchical_planner import HierarchicalPlanner
from rules_learning import RulesLearning


def create_mock_state(avatar_pos=(2, 2), grid_size=(10, 10)):
    """Crée état mock pour tests"""
    grid = np.full(grid_size, 4, dtype=np.int32)  # Sol
    
    # Murs périphériques
    grid[0, :] = 0
    grid[-1, :] = 0
    grid[:, 0] = 0
    grid[:, -1] = 0
    
    # Avatar
    grid[avatar_pos[0], avatar_pos[1]] = 3
    
    # Porte (objectif)
    grid[5, 8] = 5
    
    return {
        'grid': grid,
        'avatar_position': avatar_pos,
        'step': 0
    }


def test_1_epsilon_greedy():
    """Test 1: Epsilon-greedy adaptatif"""
    print("\n" + "="*60)
    print("TEST 1: Epsilon-Greedy Adaptatif")
    print("="*60)
    
    # Créer composants
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Vérifier epsilon initial
    assert exploration.epsilon == 0.9, "Epsilon initial incorrect"
    print(f"✓ Epsilon initial: {exploration.epsilon}")
    
    # Simuler décroissance
    exploration_count = 0
    for _ in range(100):
        if exploration.should_explore():
            exploration_count += 1
        exploration.update_epsilon()
    
    # Vérifier décroissance
    assert exploration.epsilon < 0.9, "Epsilon devrait décroître"
    assert exploration.epsilon >= exploration.epsilon_min, "Epsilon ne devrait pas descendre sous minimum"
    print(f"✓ Epsilon après 100 updates: {exploration.epsilon:.3f}")
    print(f"✓ Taux exploration: {exploration_count}%")
    
    print("✅ Test 1 RÉUSSI")
    return True


def test_2_priority_calculation():
    """Test 2: Calcul priorités exploration"""
    print("\n" + "="*60)
    print("TEST 2: Calcul Priorités Exploration")
    print("="*60)
    
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Test différents scénarios
    
    # Scénario 1: Zone jamais visitée, haute nouveauté
    priority1 = exploration._calculate_priority(
        uncertainty=1.0,
        visit_count=0,
        last_visit=-1,
        novelty=0.9
    )
    print(f"✓ Priorité zone jamais visitée: {priority1:.3f}")
    assert priority1 > 0.7, "Priorité devrait être élevée"
    
    # Scénario 2: Zone visitée récemment
    exploration.current_step = 50
    priority2 = exploration._calculate_priority(
        uncertainty=0.3,
        visit_count=5,
        last_visit=45,  # Visite il y a 5 steps
        novelty=0.5
    )
    print(f"✓ Priorité zone visitée récemment: {priority2:.3f}")
    assert priority2 < priority1, "Priorité devrait être plus faible"
    
    # Scénario 3: Zone très visitée
    priority3 = exploration._calculate_priority(
        uncertainty=0.1,
        visit_count=20,
        last_visit=10,
        novelty=0.2
    )
    print(f"✓ Priorité zone très visitée: {priority3:.3f}")
    assert priority3 < priority2, "Priorité devrait être minimale"
    
    print("✅ Test 2 RÉUSSI")
    return True


def test_3_zone_selection():
    """Test 3: Sélection zones cibles"""
    print("\n" + "="*60)
    print("TEST 3: Sélection Zones Cibles")
    print("="*60)
    
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Créer état
    state = create_mock_state(avatar_pos=(2, 2))
    
    # Mettre à jour zones
    exploration._update_exploration_zones(state)
    
    # Vérifier zones identifiées
    assert len(exploration.exploration_zones) > 0, "Devrait identifier zones"
    print(f"✓ Zones identifiées: {len(exploration.exploration_zones)}")
    
    # Vérifier tri par priorité
    if len(exploration.exploration_zones) > 1:
        for i in range(len(exploration.exploration_zones) - 1):
            assert (exploration.exploration_zones[i].priority >= 
                   exploration.exploration_zones[i+1].priority), "Zones devraient être triées"
    print("✓ Zones triées par priorité")
    
    # Sélectionner zone cible
    target = exploration._select_target_zone(state)
    assert target is not None, "Devrait sélectionner zone cible"
    print(f"✓ Zone cible: {target.position}, priorité: {target.priority:.3f}")
    
    print("✅ Test 3 RÉUSSI")
    return True


def test_4_cycle_detection():
    """Test 4: Détection cycles"""
    print("\n" + "="*60)
    print("TEST 4: Détection Cycles")
    print("="*60)
    
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Simuler cycle: A -> B -> A -> B -> A -> B
    positions = [(2, 2), (2, 3), (2, 2), (2, 3), (2, 2), (2, 3),
                 (2, 2), (2, 3), (2, 2), (2, 3)]
    
    for pos in positions:
        exploration._record_visit(pos)
    
    # Vérifier détection cycle
    is_cycle = exploration._is_in_cycle()
    assert is_cycle, "Devrait détecter cycle"
    print(f"✓ Cycle détecté: {is_cycle}")
    print(f"✓ Cycles totaux: {exploration.cycles_detected}")
    
    # Vérifier pas de faux positif
    exploration.reset()
    diverse_positions = [(i, j) for i in range(2, 7) for j in range(2, 7)]
    for pos in diverse_positions[:10]:
        exploration._record_visit(pos)
    
    is_cycle = exploration._is_in_cycle()
    assert not is_cycle, "Ne devrait pas détecter cycle avec positions diverses"
    print("✓ Pas de faux positif")
    
    print("✅ Test 4 RÉUSSI")
    return True


def test_5_visit_recording():
    """Test 5: Enregistrement visites"""
    print("\n" + "="*60)
    print("TEST 5: Enregistrement Visites")
    print("="*60)
    
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Enregistrer visites
    pos1 = (2, 2)
    pos2 = (3, 3)
    
    exploration._record_visit(pos1)
    exploration._record_visit(pos1)
    exploration._record_visit(pos2)
    
    # Vérifier compteurs
    assert exploration.position_visits[pos1] == 2, "Compteur visites incorrect"
    assert exploration.position_visits[pos2] == 1, "Compteur visites incorrect"
    print(f"✓ Visites pos1: {exploration.position_visits[pos1]}")
    print(f"✓ Visites pos2: {exploration.position_visits[pos2]}")
    
    # Vérifier dernière visite
    assert pos1 in exploration.position_last_visit, "Dernière visite non enregistrée"
    assert pos2 in exploration.position_last_visit, "Dernière visite non enregistrée"
    print(f"✓ Dernière visite pos1: step {exploration.position_last_visit[pos1]}")
    print(f"✓ Dernière visite pos2: step {exploration.position_last_visit[pos2]}")
    
    print("✅ Test 5 RÉUSSI")
    return True


def test_6_statistics():
    """Test 6: Statistiques exploration"""
    print("\n" + "="*60)
    print("TEST 6: Statistiques Exploration")
    print("="*60)
    
    world_model = WorldModel()
    rules_learning = RulesLearning()
    world_dynamics = WorldDynamics()
    planner = HierarchicalPlanner(world_model, rules_learning, world_dynamics)
    
    exploration = IntelligentExploration(world_model, world_dynamics, planner)
    
    # Simuler exploration
    state = create_mock_state()
    
    for _ in range(10):
        if exploration.should_explore():
            action = exploration.select_exploration_action(state)
            if action is not None:
                exploration.total_explorations += 1
    
    # Enregistrer découvertes
    exploration.record_discovery("entity")
    exploration.record_discovery("rule")
    
    # Obtenir statistiques
    stats = exploration.get_statistics()
    
    # Vérifier statistiques
    assert 'total_explorations' in stats, "Statistique manquante"
    assert 'successful_discoveries' in stats, "Statistique manquante"
    assert 'epsilon' in stats, "Statistique manquante"
    assert 'unique_positions_visited' in stats, "Statistique manquante"
    
    print(f"✓ Explorations totales: {stats['total_explorations']}")
    print(f"✓ Découvertes réussies: {stats['successful_discoveries']}")
    print(f"✓ Epsilon actuel: {stats['epsilon']:.3f}")
    print(f"✓ Positions uniques: {stats['unique_positions_visited']}")
    print(f"✓ Zones identifiées: {stats['exploration_zones_identified']}")
    
    print("✅ Test 6 RÉUSSI")
    return True


def generate_report(results: dict):
    """Génère rapport de test"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"RAPPORT_PHASE_4_5_5_EXPLORATION_TEST_{timestamp}.md"
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write("# RAPPORT TEST PHASE 4.5.5 - INTELLIGENT EXPLORATION\n\n")
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
            f.write(f"- {test_name}: {status}\n")
        
        f.write("\n## Conclusion\n\n")
        if passed == total:
            f.write("✅ **Tous les tests sont passés** - IntelligentExploration validée\n")
        else:
            f.write(f"⚠️ **{total-passed} test(s) échoué(s)** - Corrections nécessaires\n")
    
    return filename


def main():
    """Exécution tests"""
    print("="*60)
    print("TESTS PHASE 4.5.5 - INTELLIGENT EXPLORATION")
    print("="*60)
    
    results = {}
    
    try:
        results['Test 1: Epsilon-Greedy'] = test_1_epsilon_greedy()
    except Exception as e:
        print(f"❌ Test 1 ÉCHOUÉ: {e}")
        results['Test 1: Epsilon-Greedy'] = False
    
    try:
        results['Test 2: Priorités'] = test_2_priority_calculation()
    except Exception as e:
        print(f"❌ Test 2 ÉCHOUÉ: {e}")
        results['Test 2: Priorités'] = False
    
    try:
        results['Test 3: Sélection Zones'] = test_3_zone_selection()
    except Exception as e:
        print(f"❌ Test 3 ÉCHOUÉ: {e}")
        results['Test 3: Sélection Zones'] = False
    
    try:
        results['Test 4: Détection Cycles'] = test_4_cycle_detection()
    except Exception as e:
        print(f"❌ Test 4 ÉCHOUÉ: {e}")
        results['Test 4: Détection Cycles'] = False
    
    try:
        results['Test 5: Enregistrement Visites'] = test_5_visit_recording()
    except Exception as e:
        print(f"❌ Test 5 ÉCHOUÉ: {e}")
        results['Test 5: Enregistrement Visites'] = False
    
    try:
        results['Test 6: Statistiques'] = test_6_statistics()
    except Exception as e:
        print(f"❌ Test 6 ÉCHOUÉ: {e}")
        results['Test 6: Statistiques'] = False
    
    # Résumé
    print("\n" + "="*60)
    print("RÉSUMÉ TESTS")
    print("="*60)
    
    total = len(results)
    passed = sum(1 for r in results.values() if r)
    
    print(f"Tests exécutés: {total}")
    print(f"Tests réussis: {passed}")
    print(f"Taux succès: {passed/total*100:.1f}%")
    
    # Générer rapport
    report_file = generate_report(results)
    print(f"\n📄 Rapport généré: {report_file}")
    
    if passed == total:
        print("\n✅ TOUS LES TESTS SONT PASSÉS")
        return 0
    else:
        print(f"\n⚠️ {total-passed} TEST(S) ÉCHOUÉ(S)")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
