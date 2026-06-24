"""
Test World Model Persistent V36
Validation de l'architecture représentationnelle

Tests:
1. TopologicalMap - Découverte régions/corridors/landmarks
2. SelfAnchor - Identité spatiale stable
3. StrategicMemory - Consolidation zones explorées/prometteuses/mortes
4. WorldInference - Séparation observé/inféré
5. WorldModelPersistent - Intégration complète
6. Forensic - Conformité LEÇON-73.1

Date: 2026-06-15
"""

import sys
import os
from pathlib import Path
from datetime import datetime
import json

# Ajouter le chemin du module
sys.path.insert(0, str(Path(__file__).parent))

from core.world_model_persistent import (
    WorldModelPersistent,
    TopologicalMap,
    SelfAnchor,
    StrategicMemory,
    WorldInference,
    Region,
    Corridor,
    Landmark
)


def test_topological_map():
    """Test 1: TopologicalMap - Carte topologique."""
    print("\n" + "="*60)
    print("TEST 1: TopologicalMap - Découverte régions")
    print("="*60)
    
    tmap = TopologicalMap(grid_size=(30, 30))
    
    # Créer quelques régions
    region1 = Region(
        id="region_0",
        center=(5, 5),
        bounds=(3, 3, 7, 7),
        visit_count=1,
        last_visit_step=0,
        confidence=0.5
    )
    
    region2 = Region(
        id="region_1",
        center=(15, 15),
        bounds=(13, 13, 17, 17),
        visit_count=1,
        last_visit_step=10,
        confidence=0.7
    )
    
    tmap.add_region(region1)
    tmap.add_region(region2)
    
    # Ajouter corridor
    corridor = Corridor(
        id="corridor_0",
        start_region="region_0",
        end_region="region_1",
        path=[(5, 5), (10, 10), (15, 15)],
        traversal_count=1
    )
    tmap.add_corridor(corridor)
    
    # Ajouter landmark
    landmark = Landmark(
        id="landmark_0",
        position=(10, 10),
        type="goal",
        discovery_step=5
    )
    tmap.add_landmark(landmark)
    
    # Vérifier
    assert len(tmap.regions) == 2, "Devrait avoir 2 régions"
    assert len(tmap.corridors) == 1, "Devrait avoir 1 corridor"
    assert len(tmap.landmarks) == 1, "Devrait avoir 1 landmark"
    
    # Tester get_region_at
    region_at_5_5 = tmap.get_region_at((5, 5))
    assert region_at_5_5 is not None, "Devrait trouver région à (5,5)"
    assert region_at_5_5.id == "region_0", "Devrait être region_0"
    
    # Mettre à jour couverture
    tmap.update_coverage()
    coverage = tmap.stats['total_coverage']
    
    print(f"✅ TopologicalMap validé")
    print(f"   - Régions: {len(tmap.regions)}")
    print(f"   - Corridors: {len(tmap.corridors)}")
    print(f"   - Landmarks: {len(tmap.landmarks)}")
    print(f"   - Couverture: {coverage:.2%}")
    
    return True


def test_self_anchor():
    """Test 2: SelfAnchor - Identité spatiale."""
    print("\n" + "="*60)
    print("TEST 2: SelfAnchor - Identité spatiale")
    print("="*60)
    
    anchor = SelfAnchor(max_history=100)
    
    # Simuler mouvement
    positions = [
        (0, 0), (1, 0), (2, 0), (3, 0), (4, 0),
        (4, 1), (4, 2), (4, 3), (4, 4), (5, 4)
    ]
    
    for step, pos in enumerate(positions):
        anchor.update_position(pos, step)
    
    # Vérifier
    assert anchor.current_position == (5, 4), "Position actuelle incorrecte"
    assert len(anchor.position_history) == 10, "Historique incorrect"
    assert anchor.stats['total_moves'] == 10, "Nombre de mouvements incorrect"
    assert anchor.stats['unique_positions'] == 10, "Positions uniques incorrectes"
    
    # Vérifier direction
    assert anchor.current_direction is not None, "Direction devrait être calculée"
    
    # Calculer distance
    distance = anchor.get_distance_traveled()
    assert distance > 0, "Distance devrait être > 0"
    
    print(f"✅ SelfAnchor validé")
    print(f"   - Position actuelle: {anchor.current_position}")
    print(f"   - Mouvements totaux: {anchor.stats['total_moves']}")
    print(f"   - Positions uniques: {anchor.stats['unique_positions']}")
    print(f"   - Direction: {anchor.current_direction}")
    print(f"   - Distance parcourue: {distance:.2f}")
    
    return True


def test_strategic_memory():
    """Test 3: StrategicMemory - Consolidation zones."""
    print("\n" + "="*60)
    print("TEST 3: StrategicMemory - Consolidation zones")
    print("="*60)
    
    memory = StrategicMemory()
    
    # Marquer zones explorées
    for x in range(5):
        for y in range(5):
            memory.mark_explored((x, y), {'step': x + y})
    
    # Marquer zones prometteuses manuellement
    memory.mark_promising((10, 10), reason="Ressource détectée")
    memory.mark_promising((11, 11), reason="Passage possible")
    
    # Marquer zones mortes
    memory.mark_dead((20, 20), reason="Dead-end confirmé")
    memory.mark_dead((21, 21), reason="Obstacle permanent")
    
    # Vérifier
    assert len(memory.explored_zones) == 25, "Devrait avoir 25 zones explorées"
    # V36.1: Auto-generation crée zones prometteuses aux frontières
    # On vérifie qu'il y a AU MOINS les 2 zones manuelles
    assert len(memory.promising_zones) >= 2, f"Devrait avoir au moins 2 zones prometteuses (trouvé {len(memory.promising_zones)})"
    assert len(memory.dead_zones) == 2, "Devrait avoir 2 zones mortes"
    
    # Tester valeurs stratégiques
    assert memory.is_explored((0, 0)), "Zone (0,0) devrait être explorée"
    assert memory.is_promising((10, 10)), "Zone (10,10) devrait être prometteuse"
    assert memory.is_dead((20, 20)), "Zone (20,20) devrait être morte"
    
    # Tester get_zone_value
    value_explored = memory.get_zone_value((0, 0))
    value_promising = memory.get_zone_value((10, 10))
    value_dead = memory.get_zone_value((20, 20))
    value_unknown = memory.get_zone_value((50, 50))
    
    assert value_explored == 0.0, "Zone explorée devrait valoir 0.0"
    assert value_promising == 1.0, "Zone prometteuse devrait valoir 1.0"
    assert value_dead == -1.0, "Zone morte devrait valoir -1.0"
    assert value_unknown == 0.5, "Zone inconnue devrait valoir 0.5"
    
    print(f"✅ StrategicMemory validé")
    print(f"   - Zones explorées: {len(memory.explored_zones)}")
    print(f"   - Zones prometteuses: {len(memory.promising_zones)}")
    print(f"   - Zones mortes: {len(memory.dead_zones)}")
    print(f"   - Ratio exploration: {memory.stats['exploration_ratio']:.2%}")
    
    return True


def test_world_inference():
    """Test 4: WorldInference - Séparation observé/inféré."""
    print("\n" + "="*60)
    print("TEST 4: WorldInference - Séparation observé/inféré")
    print("="*60)
    
    inference = WorldInference()
    
    # Ajouter observations
    for step in range(10):
        state = {
            'agent_pos': (step, step),
            'grid': [[0] * 10 for _ in range(10)],
            'entities': []
        }
        inference.add_observation(state, step)
    
    # Vérifier
    assert inference.stats['observations'] == 10, "Devrait avoir 10 observations"
    
    # Inférer structure
    structure = inference.infer_structure()
    assert 'static_obstacles' in structure, "Structure devrait contenir obstacles"
    assert inference.stats['inferences_made'] > 0, "Devrait avoir fait inférences"
    
    # Mettre à jour confiance
    inference.update_confidence((5, 5), 0.9)
    inference.update_confidence((6, 6), 0.8)
    
    confidence_5_5 = inference.get_confidence((5, 5))
    assert confidence_5_5 == 0.9, "Confiance (5,5) devrait être 0.9"
    
    print(f"✅ WorldInference validé")
    print(f"   - Observations: {inference.stats['observations']}")
    print(f"   - Inférences: {inference.stats['inferences_made']}")
    print(f"   - Confiance moyenne: {inference.stats['avg_confidence']:.2f}")
    
    return True


def test_world_model_persistent_integration():
    """Test 5: WorldModelPersistent - Intégration complète."""
    print("\n" + "="*60)
    print("TEST 5: WorldModelPersistent - Intégration complète")
    print("="*60)
    
    # Créer logs directory
    log_dir = Path(__file__).parent / "logs"
    log_dir.mkdir(exist_ok=True)
    log_path = log_dir / f"test_world_model_v36_{datetime.now().strftime('%Y%m%d_%H%M%S')}.jsonl"
    
    # Créer World Model
    world_model = WorldModelPersistent(
        grid_size=(30, 30),
        forensic_log_path=str(log_path)
    )
    
    # Simuler exploration
    print("\n📍 Simulation exploration...")
    
    # Trajectoire en spirale
    positions = []
    x, y = 15, 15
    for i in range(20):
        positions.append((x, y))
        # Spirale simple
        if i % 4 == 0:
            x += 1
        elif i % 4 == 1:
            y += 1
        elif i % 4 == 2:
            x -= 1
        else:
            y -= 1
    
    # Mettre à jour World Model
    for step, pos in enumerate(positions):
        state = {
            'agent_pos': pos,
            'grid': [[0] * 30 for _ in range(30)],
            'entities': []
        }
        world_model.update(state, action=f"move_{step}", step=step)
    
    # Vérifier état
    state = world_model.get_state()
    
    assert state['topological_map']['regions'] > 0, "Devrait avoir découvert régions"
    assert state['self_anchor']['total_moves'] == 20, "Devrait avoir 20 mouvements"
    assert state['strategic_memory']['explored'] > 0, "Devrait avoir zones explorées"
    assert state['world_inference']['observations'] == 20, "Devrait avoir 20 observations"
    
    # Tester valeur stratégique
    strategic_value = world_model.get_strategic_value((15, 15))
    assert -1.0 <= strategic_value <= 1.0, "Valeur stratégique hors limites"
    
    # Tester frontière exploration
    frontier = world_model.get_exploration_frontier()
    assert isinstance(frontier, list), "Frontière devrait être une liste"
    
    # Tester cohérence
    coherence = world_model.stats['world_coherence']
    assert 0.0 <= coherence <= 1.0, "Cohérence hors limites"
    
    print(f"✅ WorldModelPersistent validé")
    print(f"   - Régions découvertes: {state['topological_map']['regions']}")
    print(f"   - Couverture: {state['topological_map']['coverage']:.2%}")
    print(f"   - Mouvements: {state['self_anchor']['total_moves']}")
    print(f"   - Positions uniques: {state['self_anchor']['unique_positions']}")
    print(f"   - Zones explorées: {state['strategic_memory']['explored']}")
    print(f"   - Observations: {state['world_inference']['observations']}")
    print(f"   - Cohérence monde: {coherence:.2%}")
    print(f"   - Frontière exploration: {len(frontier)} positions")
    
    # Fermer
    world_model.close()
    
    # Vérifier logs forensic
    if log_path.exists():
        with open(log_path, 'r') as f:
            events = [json.loads(line) for line in f]
        
        print(f"\n📊 Événements forensic: {len(events)}")
        
        # Vérifier événements clés (utilise 'event' pas 'event_type')
        event_types = [e['event'] for e in events]
        assert 'module_initialized' in event_types, "Devrait avoir module_initialized"
        assert 'world_model_updated' in event_types, "Devrait avoir world_model_updated"
        assert 'module_closing' in event_types, "Devrait avoir module_closing"
        
        print(f"   - module_initialized: ✅")
        print(f"   - world_model_updated: {event_types.count('world_model_updated')} fois")
        print(f"   - region_discovered: {event_types.count('region_discovered')} fois")
        print(f"   - module_closing: ✅")
    
    return True


def test_world_model_reset():
    """Test 6: Reset World Model."""
    print("\n" + "="*60)
    print("TEST 6: Reset World Model")
    print("="*60)
    
    world_model = WorldModelPersistent(grid_size=(30, 30))
    
    # Ajouter données
    for step in range(5):
        state = {'agent_pos': (step, step)}
        world_model.update(state, step=step)
    
    # Vérifier données présentes
    assert world_model.stats['total_updates'] == 5, "Devrait avoir 5 updates"
    
    # Reset
    world_model.reset()
    
    # Vérifier reset
    assert world_model.stats['total_updates'] == 0, "Updates devrait être 0 après reset"
    assert len(world_model.topological_map.regions) == 0, "Régions devrait être vide"
    assert len(world_model.strategic_memory.explored_zones) == 0, "Zones explorées devrait être vide"
    
    print(f"✅ Reset validé")
    print(f"   - Stats réinitialisées: ✅")
    print(f"   - Carte topologique vidée: ✅")
    print(f"   - Mémoire stratégique vidée: ✅")
    
    return True


def run_all_tests():
    """Exécute tous les tests."""
    print("\n" + "="*60)
    print("🧪 TESTS WORLD MODEL PERSISTENT V36")
    print("="*60)
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    tests = [
        ("TopologicalMap", test_topological_map),
        ("SelfAnchor", test_self_anchor),
        ("StrategicMemory", test_strategic_memory),
        ("WorldInference", test_world_inference),
        ("WorldModelPersistent Integration", test_world_model_persistent_integration),
        ("World Model Reset", test_world_model_reset)
    ]
    
    results = []
    
    for test_name, test_func in tests:
        try:
            success = test_func()
            results.append((test_name, success, None))
        except Exception as e:
            results.append((test_name, False, str(e)))
            print(f"\n❌ ÉCHEC: {test_name}")
            print(f"   Erreur: {e}")
    
    # Résumé
    print("\n" + "="*60)
    print("📊 RÉSUMÉ DES TESTS")
    print("="*60)
    
    passed = sum(1 for _, success, _ in results if success)
    total = len(results)
    
    for test_name, success, error in results:
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} - {test_name}")
        if error:
            print(f"       Erreur: {error}")
    
    print(f"\n{'='*60}")
    print(f"Résultat: {passed}/{total} tests réussis ({passed/total*100:.1f}%)")
    print(f"{'='*60}")
    
    return passed == total


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)


# Made with Bob