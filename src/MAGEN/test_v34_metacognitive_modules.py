"""
Test de Validation V34 - Modules Métacognitifs
Tests unitaires pour SterilityMemory, NonProgressDetector, DisengagementEngine

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V34.0.0
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from src.MAGEN.core.sterility_memory import SterilityMemory
from src.MAGEN.core.non_progress_detector import NonProgressDetector
from src.MAGEN.core.disengagement_engine import DisengagementEngine
from src.MAGEN.core.metacognitive_controller import MetaCognitiveController


def test_sterility_memory():
    """Test SterilityMemory - Mémoire des zones stériles"""
    print("\n" + "="*80)
    print("TEST 1: SterilityMemory - Détection zones stériles")
    print("="*80)
    
    memory = SterilityMemory()
    
    # Simuler 15 visites sans reward dans région A
    for step in range(15):
        memory.update(
            region_id="region_A",
            action="rotate_90",
            reward=0.0,
            current_step=step
        )
    
    # Vérifier stérilité
    is_sterile = memory.is_sterile("region_A", "rotate_90")
    sterility_score = memory.get_sterility_penalty("region_A", "rotate_90")
    
    print(f"\nRésultat après 15 visites sans reward:")
    print(f"  - Région stérile: {is_sterile}")
    print(f"  - Score stérilité: {sterility_score:.3f}")
    print(f"  - Seuil: {memory.sterility_threshold}")
    
    # Vérifier statistiques
    stats = memory.get_statistics()
    print(f"\nStatistiques:")
    print(f"  - Total updates: {stats['total_updates']}")
    print(f"  - Régions stériles: {stats['sterile_regions_count']}")
    print(f"  - Actions stériles: {stats['sterile_actions_count']}")
    print(f"  - Combinaisons stériles: {stats['sterile_combinations_count']}")
    
    # Validation
    assert is_sterile, "❌ Région devrait être stérile après 15 visites sans reward"
    assert sterility_score >= memory.sterility_threshold, "❌ Score stérilité trop bas"
    
    print("\n✅ TEST 1 RÉUSSI - SterilityMemory fonctionne correctement")
    
    memory.close()
    return True


def test_non_progress_detector():
    """Test NonProgressDetector - Détection stagnation"""
    print("\n" + "="*80)
    print("TEST 2: NonProgressDetector - Détection stagnation globale")
    print("="*80)
    
    detector = NonProgressDetector(window_size=20)
    
    # Simuler 30 steps avec score gelé
    base_score = 0.5
    for step in range(30):
        detector.update(
            current_step=step,
            current_score=base_score,  # Score gelé
            current_action="wait" if step % 2 == 0 else "rotate_90",
            current_health=0.3
        )
    
    # Vérifier stagnation après 25 steps
    stagnation = detector.check_stagnation(current_step=25)
    is_stagnating = detector.is_stagnating()
    severity = detector.get_stagnation_severity()
    
    print(f"\nRésultat après 30 steps avec score gelé:")
    print(f"  - Stagnation détectée: {stagnation is not None}")
    print(f"  - Est en stagnation: {is_stagnating}")
    print(f"  - Sévérité: {severity:.3f}")
    
    if stagnation:
        print(f"  - Raison: {stagnation.reason}")
        print(f"  - Delta score: {stagnation.score_delta:.3f}")
        print(f"  - Diversité actions: {stagnation.action_diversity:.3f}")
    
    # Vérifier statistiques
    stats = detector.get_statistics()
    print(f"\nStatistiques:")
    print(f"  - Total checks: {stats['total_checks']}")
    print(f"  - Stagnation count: {stats['stagnation_count']}")
    print(f"  - Stagnation rate: {stats['stagnation_rate']:.3f}")
    
    # Validation
    assert stagnation is not None, "❌ Stagnation devrait être détectée"
    assert is_stagnating, "❌ Devrait être en stagnation"
    assert severity > 0.5, "❌ Sévérité devrait être élevée"
    
    print("\n✅ TEST 2 RÉUSSI - NonProgressDetector fonctionne correctement")
    
    detector.close()
    return True


def test_disengagement_engine():
    """Test DisengagementEngine - Décision d'abandon"""
    print("\n" + "="*80)
    print("TEST 3: DisengagementEngine - Décision désengagement")
    print("="*80)
    
    engine = DisengagementEngine(min_steps_before_disengage=10)
    
    # Démarrer trajectoire
    engine.start_trajectory(step=0, score=0.5)
    
    # Simuler 25 steps avec stérilité et stagnation élevées
    for step in range(25):
        engine.update_trajectory_cost(cost_increment=1.0)
    
    # Évaluer désengagement avec conditions critiques
    decision = engine.evaluate_disengagement(
        current_step=25,
        current_score=0.5,  # Pas de progrès
        sterility_score=0.85,  # Très stérile
        stagnation_severity=0.75,  # Forte stagnation
        future_value_prediction=0.1  # Valeur future faible
    )
    
    print(f"\nRésultat évaluation désengagement:")
    print(f"  - Devrait se désengager: {decision.should_disengage}")
    print(f"  - Raison: {decision.reason.value}")
    print(f"  - Confiance: {decision.confidence:.3f}")
    print(f"  - Action alternative: {decision.alternative_action}")
    print(f"  - Explication: {decision.explanation}")
    
    # Vérifier statistiques
    stats = engine.get_statistics()
    print(f"\nStatistiques:")
    print(f"  - Total évaluations: {stats['total_evaluations']}")
    print(f"  - Total désengagements: {stats['total_disengagements']}")
    print(f"  - Taux désengagement: {stats['disengagement_rate']:.3f}")
    print(f"  - Coût trajectoire: {stats['current_trajectory_cost']}")
    
    # Validation
    assert decision.should_disengage, "❌ Devrait décider de se désengager"
    assert decision.confidence > 0.6, "❌ Confiance devrait être élevée"
    assert decision.alternative_action is not None, "❌ Devrait suggérer alternative"
    
    print("\n✅ TEST 3 RÉUSSI - DisengagementEngine fonctionne correctement")
    
    engine.close()
    return True


def test_metacognitive_controller():
    """Test MetaCognitiveController - Orchestration complète"""
    print("\n" + "="*80)
    print("TEST 4: MetaCognitiveController - Orchestration V34")
    print("="*80)
    
    controller = MetaCognitiveController()
    
    # Démarrer trajectoire
    controller.start_trajectory(step=0, score=0.5)
    
    # Simuler 30 steps avec conditions pathologiques
    for step in range(30):
        state = controller.update(
            current_step=step,
            current_score=0.5,  # Score gelé
            current_action="wait",  # Action répétitive
            current_health=0.25,  # Santé basse
            region_id="region_dead",
            reward=0.0,  # Pas de reward
            action_cost=1.0
        )
    
    # Vérifier état final
    print(f"\nÉtat métacognitif après 30 steps:")
    print(f"  - Score: {state.score}")
    print(f"  - Santé: {state.health}")
    print(f"  - En stagnation: {state.is_stagnating}")
    print(f"  - Sévérité stagnation: {state.stagnation_severity:.3f}")
    print(f"  - Pénalité stérilité: {state.sterility_penalty:.3f}")
    print(f"  - Devrait se désengager: {state.should_disengage}")
    if state.disengagement_reason:
        print(f"  - Raison désengagement: {state.disengagement_reason}")
    
    # Vérifier intervention
    should_intervene = controller.should_intervene()
    intervention_action = controller.get_intervention_action()
    
    print(f"\nDécision d'intervention:")
    print(f"  - Intervention nécessaire: {should_intervene}")
    print(f"  - Action recommandée: {intervention_action}")
    
    # Vérifier statistiques globales
    stats = controller.get_statistics()
    print(f"\nStatistiques globales:")
    print(f"  - Total updates: {stats['total_updates']}")
    print(f"  - Total interventions: {stats['total_interventions']}")
    print(f"  - Taux intervention: {stats['intervention_rate']:.3f}")
    
    # Validation
    assert state.is_stagnating, "❌ Devrait détecter stagnation"
    assert state.sterility_penalty > 0.5, "❌ Pénalité stérilité devrait être élevée"
    assert state.should_disengage, "❌ Devrait décider désengagement"
    assert should_intervene, "❌ Intervention devrait être nécessaire"
    
    print("\n✅ TEST 4 RÉUSSI - MetaCognitiveController fonctionne correctement")
    
    controller.close()
    return True


def test_integration_complete():
    """Test d'intégration complète V34"""
    print("\n" + "="*80)
    print("TEST 5: Intégration Complète V34 - Scénario Réaliste")
    print("="*80)
    
    controller = MetaCognitiveController()
    
    # Scénario: Exploration normale puis zone stérile
    print("\nPhase 1: Exploration normale (steps 0-20)")
    controller.start_trajectory(step=0, score=0.0)
    
    for step in range(20):
        state = controller.update(
            current_step=step,
            current_score=0.0 + step * 0.01,  # Progrès lent
            current_action=f"action_{step % 5}",  # Diversité
            current_health=0.6,
            region_id="region_normal",
            reward=0.01,
            action_cost=1.0
        )
    
    print(f"  - Après 20 steps: score={state.score:.3f}, stagnation={state.is_stagnating}")
    
    # Phase 2: Entrée en zone stérile
    print("\nPhase 2: Zone stérile (steps 20-50)")
    
    for step in range(20, 50):
        state = controller.update(
            current_step=step,
            current_score=0.20,  # Score gelé
            current_action="wait",  # Action répétitive
            current_health=0.3,  # Santé décroissante
            region_id="region_sterile",
            reward=0.0,  # Pas de reward
            action_cost=1.0
        )
        
        # Vérifier si intervention déclenchée
        if controller.should_intervene():
            print(f"\n  ⚠️  INTERVENTION DÉCLENCHÉE au step {step}")
            print(f"      - Raison: {state.disengagement_reason}")
            print(f"      - Action recommandée: {controller.get_intervention_action()}")
            break
    
    # Vérifier état final
    final_state = controller.get_state()
    print(f"\nÉtat final:")
    print(f"  - Total updates: {final_state['statistics']['total_updates']}")
    print(f"  - Total interventions: {final_state['statistics']['total_interventions']}")
    
    # Accès sécurisé aux stats modules
    sterility_stats = final_state.get('sterility_memory', {}).get('statistics', {})
    progress_stats = final_state.get('progress_detector', {})
    disengagement_stats = final_state.get('disengagement_engine', {})
    
    print(f"  - Régions stériles: {sterility_stats.get('sterile_regions_count', 0)}")
    print(f"  - Stagnation détectée: {progress_stats.get('is_stagnating', False)}")
    print(f"  - Désengagement actif: {disengagement_stats.get('should_disengage_now', False)}")
    
    # Validation
    assert final_state['statistics']['total_interventions'] > 0, "❌ Devrait avoir au moins 1 intervention"
    assert sterility_stats.get('sterile_regions_count', 0) > 0 or sterility_stats.get('sterile_combinations_count', 0) > 0, "❌ Devrait détecter zones stériles"
    
    print("\n✅ TEST 5 RÉUSSI - Intégration complète V34 validée")
    
    controller.close()
    return True


def main():
    """Exécute tous les tests V34"""
    print("\n" + "="*80)
    print("VALIDATION MODULES MÉTACOGNITIFS V34")
    print("="*80)
    print("\nTests:")
    print("  1. SterilityMemory - Mémoire zones stériles")
    print("  2. NonProgressDetector - Détection stagnation")
    print("  3. DisengagementEngine - Décision abandon")
    print("  4. MetaCognitiveController - Orchestration")
    print("  5. Intégration complète - Scénario réaliste")
    
    results = []
    
    try:
        results.append(("SterilityMemory", test_sterility_memory()))
    except Exception as e:
        print(f"\n❌ TEST 1 ÉCHOUÉ: {e}")
        results.append(("SterilityMemory", False))
    
    try:
        results.append(("NonProgressDetector", test_non_progress_detector()))
    except Exception as e:
        print(f"\n❌ TEST 2 ÉCHOUÉ: {e}")
        results.append(("NonProgressDetector", False))
    
    try:
        results.append(("DisengagementEngine", test_disengagement_engine()))
    except Exception as e:
        print(f"\n❌ TEST 3 ÉCHOUÉ: {e}")
        results.append(("DisengagementEngine", False))
    
    try:
        results.append(("MetaCognitiveController", test_metacognitive_controller()))
    except Exception as e:
        print(f"\n❌ TEST 4 ÉCHOUÉ: {e}")
        results.append(("MetaCognitiveController", False))
    
    try:
        results.append(("Intégration Complète", test_integration_complete()))
    except Exception as e:
        print(f"\n❌ TEST 5 ÉCHOUÉ: {e}")
        results.append(("Intégration Complète", False))
    
    # Rapport final
    print("\n" + "="*80)
    print("RAPPORT FINAL")
    print("="*80)
    
    passed = sum(1 for _, success in results if success)
    total = len(results)
    
    print(f"\nRésultats: {passed}/{total} tests réussis")
    print("\nDétail:")
    for name, success in results:
        status = "✅ RÉUSSI" if success else "❌ ÉCHOUÉ"
        print(f"  - {name}: {status}")
    
    if passed == total:
        print("\n" + "="*80)
        print("🎉 VALIDATION V34 COMPLÈTE - TOUS LES TESTS RÉUSSIS")
        print("="*80)
        print("\nModules V34 opérationnels:")
        print("  ✅ SterilityMemory - Mémoire zones stériles")
        print("  ✅ NonProgressDetector - Détection stagnation")
        print("  ✅ DisengagementEngine - Abandon stratégique")
        print("  ✅ MetaCognitiveController - Orchestration")
        print("\nMAGEN V34 prêt pour intégration production")
        return 0
    else:
        print("\n⚠️  VALIDATION PARTIELLE - Certains tests ont échoué")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
