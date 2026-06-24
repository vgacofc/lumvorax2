#!/usr/bin/env python3
"""
Test Forensic V34 - Validation Corrections
Vérifie conformité LEÇON-73.1 après corrections

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
"""

import json
import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from MAGEN.core.action_reputation_system import ActionReputationSystem
from MAGEN.core.metacognitive_controller import MetaCognitiveController


def test_action_reputation_system_forensic():
    """Test forensic ActionReputationSystem (LEÇON-73.1)"""
    print("\n" + "="*80)
    print("TEST 1: ActionReputationSystem Forensic")
    print("="*80)
    
    log_path = "logs/test_v34_corrections/action_reputation_system.jsonl"
    
    # Nettoyer ancien log
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    # Créer instance
    print(f"\n✓ Création instance avec forensic_log_path={log_path}")
    ars = ActionReputationSystem(
        exploration_bonus=0.1,
        min_attempts_threshold=10,
        verbose=False,
        forensic_log_path=log_path
    )
    
    # Vérifier API
    print("\n✓ Vérification API forensic...")
    assert hasattr(ars, 'forensic'), "❌ MANQUE: self.forensic"
    assert hasattr(ars, '_forensic_state'), "❌ MANQUE: _forensic_state property"
    assert hasattr(ars, 'get_state'), "❌ MANQUE: get_state() method"
    assert hasattr(ars, 'close'), "❌ MANQUE: close() method"
    print("  ✅ Tous les attributs forensic présents")
    
    # Tester opérations
    print("\n✓ Test opérations avec logging...")
    ars.register_action("rotate_90")
    ars.register_action("flip_horizontal")
    ars.update("rotate_90", success=True, error=0.1)
    ars.update("flip_horizontal", success=False, error=0.9)
    
    selected = ars.select_actions(
        candidates=["rotate_90", "flip_horizontal", "identity"],
        top_k=2
    )
    print(f"  ✅ Actions sélectionnées: {selected}")
    
    # Vérifier état
    print("\n✓ Vérification état forensic...")
    state = ars.get_state()
    assert 'statistics' in state, "❌ MANQUE: statistics dans state"
    assert 'forensic_state' in state, "❌ MANQUE: forensic_state dans state"
    print(f"  ✅ État forensic: {state['forensic_state']}")
    
    # Fermer proprement
    print("\n✓ Fermeture propre...")
    ars.close()
    print("  ✅ Module fermé")
    
    # Lire et valider logs
    print("\n✓ Validation logs forensic...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    
    # Vérifier événements
    event_types = [e['event'] for e in events]
    print(f"  ✅ Types événements: {event_types}")
    
    assert 'module_initialized' in event_types, "❌ MANQUE: module_initialized"
    assert 'module_closing' in event_types, "❌ MANQUE: module_closing"
    assert 'action_registered' in event_types, "❌ MANQUE: action_registered"
    assert 'select_actions_start' in event_types, "❌ MANQUE: select_actions_start"
    assert 'select_actions_complete' in event_types, "❌ MANQUE: select_actions_complete"
    
    # Vérifier structure
    print("\n✓ Vérification structure événements...")
    for i, event in enumerate(events):
        assert 'timestamp_ns' in event, f"❌ Event {i}: MANQUE timestamp_ns"
        assert 'component' in event, f"❌ Event {i}: MANQUE component"
        assert 'event' in event, f"❌ Event {i}: MANQUE event"
        assert 'data' in event, f"❌ Event {i}: MANQUE data"
        assert 'entry_hash' in event, f"❌ Event {i}: MANQUE entry_hash"
    print(f"  ✅ Structure validée pour {len(events)} événements")
    
    # Vérifier hash chain
    print("\n✓ Vérification hash chain...")
    for i in range(1, len(events)):
        assert events[i]['previous_hash'] == events[i-1]['entry_hash'], \
            f"❌ Hash chain brisée à l'événement {i}"
    print(f"  ✅ Hash chain intègre ({len(events)-1} liens)")
    
    print("\n" + "="*80)
    print("✅ TEST 1 RÉUSSI: ActionReputationSystem 100% conforme LEÇON-73.1")
    print("="*80)
    
    return True, len(events)


def test_metacognitive_controller_forensic():
    """Test forensic MetaCognitiveController (déjà conforme)"""
    print("\n" + "="*80)
    print("TEST 2: MetaCognitiveController Forensic (Validation)")
    print("="*80)
    
    log_path = "logs/test_v34_corrections/metacognitive_controller.jsonl"
    
    # Nettoyer ancien log
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    # Créer instance avec forensic minimal
    print(f"\n✓ Création instance...")
    
    # Import forensic middleware
    from MAGEN.core.forensic_middleware import ForensicMiddleware
    
    Path(log_path).parent.mkdir(parents=True, exist_ok=True)
    forensic = ForensicMiddleware(log_path, "MetaCognitiveController")
    
    controller = MetaCognitiveController(
        forensic_logger=forensic.forensic_logger,
        enable_sterility_memory=False,  # Simplifier test
        enable_progress_detection=False,
        enable_disengagement=False
    )
    
    # Vérifier API
    print("\n✓ Vérification API forensic...")
    assert hasattr(controller, 'get_state'), "❌ MANQUE: get_state() method"
    assert hasattr(controller, 'close'), "❌ MANQUE: close() method"
    print("  ✅ API forensic présente")
    
    # Vérifier état
    print("\n✓ Vérification état...")
    state = controller.get_state()
    assert 'statistics' in state, "❌ MANQUE: statistics dans state"
    print(f"  ✅ État: {state['statistics']}")
    
    # Fermer proprement
    print("\n✓ Fermeture propre...")
    controller.close()
    forensic.forensic_logger.close()
    print("  ✅ Module fermé")
    
    # Lire logs
    print("\n✓ Validation logs...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    
    event_types = [e['event'] for e in events]
    assert 'module_initialized' in event_types, "❌ MANQUE: module_initialized"
    assert 'module_closing' in event_types, "❌ MANQUE: module_closing"
    
    print("\n" + "="*80)
    print("✅ TEST 2 RÉUSSI: MetaCognitiveController déjà conforme")
    print("="*80)
    
    return True, len(events)


def main():
    """Exécute tous les tests"""
    print("\n" + "="*80)
    print("VALIDATION FORENSIC V34 - CORRECTIONS LEÇON-73.1")
    print("="*80)
    
    results = []
    
    try:
        # Test 1: ActionReputationSystem
        success1, events1 = test_action_reputation_system_forensic()
        results.append(("ActionReputationSystem", success1, events1))
    except Exception as e:
        print(f"\n❌ TEST 1 ÉCHOUÉ: {e}")
        import traceback
        traceback.print_exc()
        results.append(("ActionReputationSystem", False, 0))
    
    try:
        # Test 2: MetaCognitiveController
        success2, events2 = test_metacognitive_controller_forensic()
        results.append(("MetaCognitiveController", success2, events2))
    except Exception as e:
        print(f"\n❌ TEST 2 ÉCHOUÉ: {e}")
        import traceback
        traceback.print_exc()
        results.append(("MetaCognitiveController", False, 0))
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ VALIDATION")
    print("="*80)
    
    total_success = sum(1 for _, success, _ in results if success)
    total_events = sum(events for _, _, events in results)
    
    for module, success, events in results:
        status = "✅ RÉUSSI" if success else "❌ ÉCHOUÉ"
        print(f"{status} - {module}: {events} événements")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {total_success}/{len(results)} tests réussis")
    print(f"Total événements forensic: {total_events}")
    print(f"{'='*80}\n")
    
    # Exit code
    sys.exit(0 if total_success == len(results) else 1)


if __name__ == "__main__":
    main()

# Made with Bob
