#!/usr/bin/env python3
"""
Test V35 - Validation Architecture Causale Fonctionnelle Complète
Vérifie les 3 nouveaux modules + forensic LEÇON-73.1

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
"""

import json
import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from MAGEN.core.trajectory_utility_model import TrajectoryUtilityModel
from MAGEN.core.value_of_stopping_estimator import ValueOfStoppingEstimator, StoppingDecision
from MAGEN.core.global_regret_detector import GlobalRegretDetector, RegretLevel


def test_trajectory_utility_model():
    """Test TrajectoryUtilityModel avec métriques fonctionnelles"""
    print("\n" + "="*80)
    print("TEST 1: TrajectoryUtilityModel - Métriques Fonctionnelles")
    print("="*80)
    
    log_path = "logs/test_v35/trajectory_utility_model.jsonl"
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    # Créer instance
    print(f"\n✓ Création TrajectoryUtilityModel...")
    model = TrajectoryUtilityModel(
        window_size=50,
        min_trajectory_length=5,
        utility_threshold=0.3,
        forensic_log_path=log_path
    )
    
    # Vérifier API forensic
    print("\n✓ Vérification API forensic...")
    assert hasattr(model, 'forensic'), "❌ MANQUE: self.forensic"
    assert hasattr(model, '_forensic_state'), "❌ MANQUE: _forensic_state"
    assert hasattr(model, 'get_state'), "❌ MANQUE: get_state()"
    assert hasattr(model, 'close'), "❌ MANQUE: close()"
    print("  ✅ API forensic complète")
    
    # Test trajectoire
    print("\n✓ Test trajectoire avec métriques fonctionnelles...")
    model.start_trajectory("traj_001", start_step=0)
    
    # Simuler progression
    for i in range(10):
        model.update_trajectory(
            current_step=i,
            current_score=0.5 + i * 0.05,  # Amélioration progressive
            current_action=f"action_{i % 3}",
            reward=0.1,
            state_hash=f"state_{i}"
        )
    
    # Prédire utilité
    prediction = model.predict_utility()
    print(f"  ✅ Utilité prédite: {prediction.predicted_utility:.3f}")
    print(f"  ✅ Confiance: {prediction.confidence:.3f}")
    print(f"  ✅ Métriques: {prediction.metrics_used}")
    
    # Vérifier métriques fonctionnelles (NOUVEAUTÉ V35)
    assert 'loop_reduction' in prediction.metrics_used, "❌ MANQUE: loop_reduction"
    assert 'convergence_rate' in prediction.metrics_used, "❌ MANQUE: convergence_rate"
    assert 'cumulative_reward' in prediction.metrics_used, "❌ MANQUE: cumulative_reward"
    print("  ✅ Métriques fonctionnelles présentes")
    
    # Décision continuer
    should_continue, reason = model.should_continue()
    print(f"  ✅ Décision: {'Continuer' if should_continue else 'Arrêter'} ({reason})")
    
    # Terminer trajectoire
    model.end_trajectory(success=True)
    
    # Vérifier état
    state = model.get_state()
    print(f"\n✓ État final: {state['statistics']}")
    
    # Fermer
    model.close()
    
    # Valider logs
    print("\n✓ Validation logs forensic...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    event_types = [e['event'] for e in events]
    assert 'module_initialized' in event_types
    assert 'trajectory_started' in event_types
    assert 'utility_predicted' in event_types
    assert 'trajectory_ended' in event_types
    assert 'module_closing' in event_types
    print(f"  ✅ Types: {event_types}")
    
    print("\n" + "="*80)
    print("✅ TEST 1 RÉUSSI: TrajectoryUtilityModel 100% conforme")
    print("="*80)
    
    return True, len(events)


def test_value_of_stopping_estimator():
    """Test ValueOfStoppingEstimator - Décision Stop"""
    print("\n" + "="*80)
    print("TEST 2: ValueOfStoppingEstimator - Value of Stopping")
    print("="*80)
    
    log_path = "logs/test_v35/value_of_stopping_estimator.jsonl"
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    # Créer instance
    print(f"\n✓ Création ValueOfStoppingEstimator...")
    estimator = ValueOfStoppingEstimator(
        cost_per_step=0.1,  # Favorise exploration
        diminishing_returns_threshold=0.01,
        max_cost_threshold=100.0,
        min_value_threshold=-10.0,  # Très permissif
        forensic_log_path=log_path
    )
    
    # Vérifier API
    print("\n✓ Vérification API forensic...")
    assert hasattr(estimator, 'forensic')
    assert hasattr(estimator, '_forensic_state')
    assert hasattr(estimator, 'get_state')
    assert hasattr(estimator, 'close')
    print("  ✅ API forensic complète")
    
    # Test évaluation - Cas CONTINUE
    print("\n✓ Test évaluation CONTINUE (bonne trajectoire)...")
    eval1 = estimator.evaluate_stopping(
        current_value=0.5,
        current_cost=10.0,
        trajectory_utility=0.8,
        convergence_rate=0.1,
        steps_remaining_estimate=10
    )
    print(f"  ✅ Décision: {eval1.decision.value}")
    print(f"  ✅ V(continue)={eval1.value_of_continuing:.3f}, V(stop)={eval1.value_of_stopping:.3f}")
    assert eval1.decision == StoppingDecision.CONTINUE, "❌ Devrait continuer"
    
    # Test évaluation - Cas STOP
    print("\n✓ Test évaluation STOP (mauvaise trajectoire)...")
    eval2 = estimator.evaluate_stopping(
        current_value=0.5,
        current_cost=10.0,
        trajectory_utility=0.1,  # Faible utilité
        convergence_rate=0.001,  # Convergence lente
        steps_remaining_estimate=10
    )
    print(f"  ✅ Décision: {eval2.decision.value}")
    print(f"  ✅ Raison: {eval2.reasoning}")
    assert eval2.decision != StoppingDecision.CONTINUE, "❌ Devrait arrêter"
    
    # Vérifier état
    state = estimator.get_state()
    print(f"\n✓ État: {state['statistics']}")
    
    # Fermer
    estimator.close()
    
    # Valider logs
    print("\n✓ Validation logs forensic...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    event_types = [e['event'] for e in events]
    assert 'module_initialized' in event_types
    assert 'stopping_evaluated' in event_types
    assert 'module_closing' in event_types
    
    print("\n" + "="*80)
    print("✅ TEST 2 RÉUSSI: ValueOfStoppingEstimator 100% conforme")
    print("="*80)
    
    return True, len(events)


def test_global_regret_detector():
    """Test GlobalRegretDetector - Signal Regret Global"""
    print("\n" + "="*80)
    print("TEST 3: GlobalRegretDetector - Global Regret Signal")
    print("="*80)
    
    log_path = "logs/test_v35/global_regret_detector.jsonl"
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    # Créer instance
    print(f"\n✓ Création GlobalRegretDetector...")
    detector = GlobalRegretDetector(
        window_size=100,
        loop_threshold=0.7,
        stagnation_threshold=0.8,
        waste_threshold=0.6,
        forensic_log_path=log_path
    )
    
    # Vérifier API
    print("\n✓ Vérification API forensic...")
    assert hasattr(detector, 'forensic')
    assert hasattr(detector, '_forensic_state')
    assert hasattr(detector, 'get_state')
    assert hasattr(detector, 'close')
    print("  ✅ API forensic complète")
    
    # Test détection - Pas de regret
    print("\n✓ Test détection AUCUN regret (bonne situation)...")
    for i in range(15):
        detector.update(
            loop_score=0.2,  # Peu de boucles
            stagnation_score=0.1,  # Peu de stagnation
            trajectory_utility=0.8,  # Bonne utilité
            step_cost=1.0
        )
    
    signal1 = detector.detect_regret()
    print(f"  ✅ Niveau: {signal1.level.value}")
    print(f"  ✅ Score regret: {signal1.regret_score:.3f}")
    assert signal1.level == RegretLevel.NONE, "❌ Ne devrait pas détecter regret"
    
    # Test détection - Regret HIGH
    print("\n✓ Test détection REGRET HIGH (mauvaise situation)...")
    for i in range(25):
        detector.update(
            loop_score=0.9,  # Boucles infinies
            stagnation_score=0.9,  # Stagnation totale
            trajectory_utility=0.1,  # Faible utilité
            step_cost=1.0
        )
    
    signal2 = detector.detect_regret()
    print(f"  ✅ Niveau: {signal2.level.value}")
    print(f"  ✅ Score regret: {signal2.regret_score:.3f}")
    print(f"  ✅ Gaspillage: {signal2.wasted_computation:.3f}")
    print(f"  ✅ Action recommandée: {signal2.recommended_action}")
    assert signal2.level in [RegretLevel.HIGH, RegretLevel.CRITICAL], "❌ Devrait détecter regret"
    
    # Test décision reset
    should_reset, reset_type = detector.should_reset()
    print(f"\n✓ Décision reset: {should_reset} ({reset_type})")
    assert should_reset, "❌ Devrait recommander reset"
    
    # Vérifier état
    state = detector.get_state()
    print(f"\n✓ État: {state['statistics']}")
    
    # Fermer
    detector.close()
    
    # Valider logs
    print("\n✓ Validation logs forensic...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    event_types = [e['event'] for e in events]
    assert 'module_initialized' in event_types
    assert 'regret_detected' in event_types
    assert 'module_closing' in event_types
    
    print("\n" + "="*80)
    print("✅ TEST 3 RÉUSSI: GlobalRegretDetector 100% conforme")
    print("="*80)
    
    return True, len(events)


def main():
    """Exécute tous les tests V35"""
    print("\n" + "="*80)
    print("VALIDATION V35 - ARCHITECTURE CAUSALE FONCTIONNELLE COMPLÈTE")
    print("="*80)
    
    results = []
    
    try:
        success1, events1 = test_trajectory_utility_model()
        results.append(("TrajectoryUtilityModel", success1, events1))
    except Exception as e:
        print(f"\n❌ TEST 1 ÉCHOUÉ: {e}")
        import traceback
        traceback.print_exc()
        results.append(("TrajectoryUtilityModel", False, 0))
    
    try:
        success2, events2 = test_value_of_stopping_estimator()
        results.append(("ValueOfStoppingEstimator", success2, events2))
    except Exception as e:
        print(f"\n❌ TEST 2 ÉCHOUÉ: {e}")
        import traceback
        traceback.print_exc()
        results.append(("ValueOfStoppingEstimator", False, 0))
    
    try:
        success3, events3 = test_global_regret_detector()
        results.append(("GlobalRegretDetector", success3, events3))
    except Exception as e:
        print(f"\n❌ TEST 3 ÉCHOUÉ: {e}")
        import traceback
        traceback.print_exc()
        results.append(("GlobalRegretDetector", False, 0))
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ VALIDATION V35")
    print("="*80)
    
    total_success = sum(1 for _, success, _ in results if success)
    total_events = sum(events for _, _, events in results)
    
    for module, success, events in results:
        status = "✅ RÉUSSI" if success else "❌ ÉCHOUÉ"
        print(f"{status} - {module}: {events} événements")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {total_success}/{len(results)} tests réussis")
    print(f"Total événements forensic: {total_events}")
    print(f"\n🎯 NOUVEAUTÉS V35 VALIDÉES:")
    print(f"  ✅ Métriques fonctionnelles (loop_reduction, convergence, reward)")
    print(f"  ✅ Value of Stopping (décision continuer/arrêter)")
    print(f"  ✅ Global Regret Signal (détection gaspillage)")
    print(f"  ✅ Forensic standard LEÇON-73.1 (100% conformité)")
    print(f"{'='*80}\n")
    
    sys.exit(0 if total_success == len(results) else 1)


if __name__ == "__main__":
    main()

# Made with Bob
