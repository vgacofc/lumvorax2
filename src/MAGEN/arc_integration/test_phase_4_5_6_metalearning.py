"""
Tests Unitaires - MetaLearning (Couche 7)
Phase 4.5.6 - Architecture Systémique MAGEN

Tests:
1. Enregistrement épisodes
2. Sélection stratégies
3. Adaptation paramètres
4. Identification patterns
5. Statistiques méta-apprentissage
6. Meilleures pratiques

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.6-TEST
"""

import sys
import os
from datetime import datetime

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

# Imports
from meta_learning import MetaLearning, EpisodeMemory, Strategy


def test_1_episode_recording():
    """Test 1: Enregistrement épisodes"""
    print("\n" + "="*60)
    print("TEST 1: Enregistrement Épisodes")
    print("="*60)
    
    meta = MetaLearning()
    
    # Enregistrer épisode succès
    meta.record_episode(
        episode_id=1,
        total_reward=50.0,
        steps=30,
        victory=True,
        strategies_used=['balanced'],
        performance_metrics={'overall': 0.8}
    )
    
    assert len(meta.episode_memories) == 1, "Épisode non enregistré"
    assert meta.total_episodes == 1, "Compteur épisodes incorrect"
    assert meta.total_victories == 1, "Compteur victoires incorrect"
    print(f"✓ Épisode enregistré: {len(meta.episode_memories)}")
    print(f"✓ Total épisodes: {meta.total_episodes}")
    print(f"✓ Total victoires: {meta.total_victories}")
    
    # Enregistrer épisode échec
    meta.record_episode(
        episode_id=2,
        total_reward=-10.0,
        steps=100,
        victory=False,
        strategies_used=['aggressive_exploration'],
        performance_metrics={'overall': 0.2}
    )
    
    assert len(meta.episode_memories) == 2, "Deuxième épisode non enregistré"
    assert meta.total_victories == 1, "Compteur victoires devrait rester à 1"
    print(f"✓ Deuxième épisode enregistré")
    print(f"✓ Taux victoire: {meta.total_victories / meta.total_episodes * 100:.1f}%")
    
    print("✅ Test 1 RÉUSSI")
    return True


def test_2_strategy_selection():
    """Test 2: Sélection stratégies"""
    print("\n" + "="*60)
    print("TEST 2: Sélection Stratégies")
    print("="*60)
    
    meta = MetaLearning()
    
    # Simuler performance mauvaise
    for _ in range(5):
        meta.performance_history.append(0.2)
    
    strategy = meta.select_strategy({})
    assert strategy == 'aggressive_exploration', "Devrait sélectionner exploration agressive"
    print(f"✓ Performance mauvaise → {strategy}")
    
    # Simuler performance bonne
    meta.performance_history.clear()
    for _ in range(5):
        meta.performance_history.append(0.8)
    
    strategy = meta.select_strategy({})
    assert strategy == 'focused_exploitation', "Devrait sélectionner exploitation ciblée"
    print(f"✓ Performance bonne → {strategy}")
    
    # Simuler performance moyenne
    meta.performance_history.clear()
    for _ in range(5):
        meta.performance_history.append(0.5)
    
    strategy = meta.select_strategy({})
    assert strategy in meta.strategies, "Stratégie invalide"
    print(f"✓ Performance moyenne → {strategy}")
    
    print("✅ Test 2 RÉUSSI")
    return True


def test_3_parameter_adaptation():
    """Test 3: Adaptation paramètres"""
    print("\n" + "="*60)
    print("TEST 3: Adaptation Paramètres")
    print("="*60)
    
    meta = MetaLearning()
    
    # Paramètres initiaux
    initial_exploration = meta.adaptive_params['exploration_weight']
    print(f"✓ Exploration initiale: {initial_exploration}")
    
    # Simuler performance mauvaise
    for _ in range(10):
        meta.performance_history.append(0.2)
        meta.victory_history.append(0.0)
    
    meta.adapt_parameters()
    
    # Vérifier augmentation exploration
    assert meta.adaptive_params['exploration_weight'] > initial_exploration, \
        "Exploration devrait augmenter avec mauvaise performance"
    print(f"✓ Exploration après adaptation: {meta.adaptive_params['exploration_weight']}")
    
    # Simuler performance bonne
    meta.performance_history.clear()
    meta.victory_history.clear()
    for _ in range(10):
        meta.performance_history.append(0.8)
        meta.victory_history.append(1.0)
    
    current_exploration = meta.adaptive_params['exploration_weight']
    meta.adapt_parameters()
    
    # Vérifier réduction exploration
    assert meta.adaptive_params['exploration_weight'] < current_exploration, \
        "Exploration devrait diminuer avec bonne performance"
    print(f"✓ Exploration après bonne performance: {meta.adaptive_params['exploration_weight']}")
    
    print("✅ Test 3 RÉUSSI")
    return True


def test_4_pattern_identification():
    """Test 4: Identification patterns"""
    print("\n" + "="*60)
    print("TEST 4: Identification Patterns")
    print("="*60)
    
    meta = MetaLearning()
    
    # Enregistrer succès
    for i in range(3):
        meta.record_episode(
            episode_id=i,
            total_reward=50.0,
            steps=30,
            victory=True,
            strategies_used=['balanced'],
            performance_metrics={'overall': 0.8}
        )
    
    assert len(meta.success_patterns) == 3, "Patterns succès non identifiés"
    print(f"✓ Patterns succès: {len(meta.success_patterns)}")
    
    # Enregistrer échecs
    for i in range(2):
        meta.record_episode(
            episode_id=i+3,
            total_reward=-10.0,
            steps=100,
            victory=False,
            strategies_used=['aggressive_exploration'],
            performance_metrics={'overall': 0.2}
        )
    
    assert len(meta.failure_patterns) == 2, "Patterns échec non identifiés"
    print(f"✓ Patterns échec: {len(meta.failure_patterns)}")
    
    print("✅ Test 4 RÉUSSI")
    return True


def test_5_statistics():
    """Test 5: Statistiques méta-apprentissage"""
    print("\n" + "="*60)
    print("TEST 5: Statistiques Méta-Apprentissage")
    print("="*60)
    
    meta = MetaLearning()
    
    # Enregistrer plusieurs épisodes
    for i in range(10):
        victory = i % 3 == 0  # 1 victoire sur 3
        meta.record_episode(
            episode_id=i,
            total_reward=50.0 if victory else -10.0,
            steps=30 if victory else 100,
            victory=victory,
            strategies_used=['balanced'],
            performance_metrics={'overall': 0.8 if victory else 0.2}
        )
    
    stats = meta.get_statistics()
    
    # Vérifier statistiques
    assert 'total_episodes' in stats, "Statistique manquante"
    assert 'total_victories' in stats, "Statistique manquante"
    assert 'victory_rate' in stats, "Statistique manquante"
    assert 'recent_performance' in stats, "Statistique manquante"
    
    assert stats['total_episodes'] == 10, "Compteur épisodes incorrect"
    assert stats['total_victories'] == 4, "Compteur victoires incorrect"  # 0,3,6,9
    
    print(f"✓ Total épisodes: {stats['total_episodes']}")
    print(f"✓ Total victoires: {stats['total_victories']}")
    print(f"✓ Taux victoire: {stats['victory_rate']*100:.1f}%")
    print(f"✓ Performance récente: {stats['recent_performance']:.3f}")
    print(f"✓ Patterns succès: {stats['success_patterns_identified']}")
    print(f"✓ Patterns échec: {stats['failure_patterns_identified']}")
    
    print("✅ Test 5 RÉUSSI")
    return True


def test_6_best_practices():
    """Test 6: Meilleures pratiques"""
    print("\n" + "="*60)
    print("TEST 6: Meilleures Pratiques")
    print("="*60)
    
    meta = MetaLearning()
    
    # Enregistrer succès avec stratégie spécifique
    for i in range(5):
        meta.record_episode(
            episode_id=i,
            total_reward=50.0,
            steps=30,
            victory=True,
            strategies_used=['balanced', 'focused_exploitation'],
            performance_metrics={'overall': 0.8}
        )
    
    practices = meta.get_best_practices()
    
    assert isinstance(practices, list), "Devrait retourner liste"
    print(f"✓ Meilleures pratiques identifiées: {len(practices)}")
    
    if practices:
        for i, practice in enumerate(practices):
            print(f"  {i+1}. {practice.get('recommendation', 'N/A')}")
            print(f"     Evidence: {practice.get('evidence', 'N/A')}")
    
    print("✅ Test 6 RÉUSSI")
    return True


def generate_report(results: dict):
    """Génère rapport de test"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"RAPPORT_PHASE_4_5_6_METALEARNING_TEST_{timestamp}.md"
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write("# RAPPORT TEST PHASE 4.5.6 - META LEARNING\n\n")
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
            f.write("✅ **Tous les tests sont passés** - MetaLearning validé\n")
            f.write("\n**Architecture Systémique COMPLÈTE (7 couches)**\n")
        else:
            f.write(f"⚠️ **{total-passed} test(s) échoué(s)** - Corrections nécessaires\n")
    
    return filename


def main():
    """Exécution tests"""
    print("="*60)
    print("TESTS PHASE 4.5.6 - META LEARNING")
    print("="*60)
    
    results = {}
    
    try:
        results['Test 1: Enregistrement Épisodes'] = test_1_episode_recording()
    except Exception as e:
        print(f"❌ Test 1 ÉCHOUÉ: {e}")
        results['Test 1: Enregistrement Épisodes'] = False
    
    try:
        results['Test 2: Sélection Stratégies'] = test_2_strategy_selection()
    except Exception as e:
        print(f"❌ Test 2 ÉCHOUÉ: {e}")
        results['Test 2: Sélection Stratégies'] = False
    
    try:
        results['Test 3: Adaptation Paramètres'] = test_3_parameter_adaptation()
    except Exception as e:
        print(f"❌ Test 3 ÉCHOUÉ: {e}")
        results['Test 3: Adaptation Paramètres'] = False
    
    try:
        results['Test 4: Identification Patterns'] = test_4_pattern_identification()
    except Exception as e:
        print(f"❌ Test 4 ÉCHOUÉ: {e}")
        results['Test 4: Identification Patterns'] = False
    
    try:
        results['Test 5: Statistiques'] = test_5_statistics()
    except Exception as e:
        print(f"❌ Test 5 ÉCHOUÉ: {e}")
        results['Test 5: Statistiques'] = False
    
    try:
        results['Test 6: Meilleures Pratiques'] = test_6_best_practices()
    except Exception as e:
        print(f"❌ Test 6 ÉCHOUÉ: {e}")
        results['Test 6: Meilleures Pratiques'] = False
    
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
        print("🎉 ARCHITECTURE SYSTÉMIQUE COMPLÈTE (7 COUCHES)")
        return 0
    else:
        print(f"\n⚠️ {total-passed} TEST(S) ÉCHOUÉ(S)")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
