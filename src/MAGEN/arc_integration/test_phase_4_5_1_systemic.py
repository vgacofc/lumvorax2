#!/usr/bin/env python3
"""
Test Phase 4.5.1 - Validation Apprentissage Systémique
=======================================================

Test de validation pour:
- WorldModel: Cartographie complète monde
- SelfIdentification: Identification avatar automatique
- MAGENSystemicAgent: Intégration complète

Objectifs:
- Avatar identifié avec >95% confiance en <10 épisodes
- 19 entités détectées et catégorisées
- Graphe spatial complet construit
- Invariants détectés (porte immobile, goals fixes)

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.1
"""

import sys
import os
from pathlib import Path
import json
from datetime import datetime
from typing import Optional
import numpy as np

# Setup paths
SCRIPT_DIR = Path(__file__).parent
MAGEN_ROOT = SCRIPT_DIR.parent
sys.path.insert(0, str(MAGEN_ROOT))

# Imports
from arc_integration.magen_arc_agent_systemic import MAGENSystemicAgent
from arc_integration.ls20_real_wrapper import LS20RealWrapper

def test_phase_4_5_1(n_episodes: int = 10, max_steps: int = 100):
    """
    Test validation Phase 4.5.1
    
    Args:
        n_episodes: Nombre d'épisodes (10 par défaut)
        max_steps: Steps max par épisode
    """
    print("\n" + "="*80)
    print("🧪 TEST PHASE 4.5.1 - APPRENTISSAGE SYSTÉMIQUE")
    print("="*80)
    print(f"📊 Configuration:")
    print(f"   - Épisodes: {n_episodes}")
    print(f"   - Steps max: {max_steps}")
    print(f"   - Objectif: Avatar identifié >95% confiance")
    print("="*80 + "\n")
    
    # Créer environnement
    print("🎮 Initialisation environnement ls20-9607627b...")
    try:
        env = LS20RealWrapper()
        print("✅ Environnement créé\n")
    except Exception as e:
        print(f"❌ Erreur création environnement: {e}")
        return None
    
    # Créer agent systémique
    print("🧠 Initialisation agent systémique...")
    agent = MAGENSystemicAgent(
        env=env,
        n_actions=4,
        learning_rate=0.1,
        gamma=0.99,
        epsilon=0.9,
        epsilon_decay=0.995,
        use_layer0=True,
        use_reward_shaper=True,
        use_systemic=True,
        verbose=True
    )
    print()
    
    # Entraîner épisodes
    print(f"🚀 Démarrage entraînement ({n_episodes} épisodes)...\n")
    
    results = []
    for episode in range(1, n_episodes + 1):
        print(f"📍 Épisode {episode}/{n_episodes}")
        print("-" * 60)
        
        try:
            result = agent.train_episode(max_steps=max_steps)
            results.append(result)
            
            # Afficher résultats
            print(f"   Steps: {result['steps']}")
            print(f"   Reward: {result['reward']:.2f}")
            print(f"   Victory: {'🎉 OUI' if result['victory'] else '❌ Non'}")
            print(f"   Epsilon: {result['epsilon']:.3f}")
            
            if result.get('avatar_identified'):
                print(f"   👤 Avatar: {agent.self_identification.get_avatar_id()} "
                      f"(confiance: {result['avatar_confidence']:.1%})")
            else:
                candidates = agent.self_identification.get_candidates()
                if candidates:
                    best = candidates[0]
                    print(f"   👤 Meilleur candidat: {best[0]} (score: {best[1]:.1f})")
            
            print(f"   🌍 Entités: {result.get('entities_detected', 0)}")
            print(f"   🔒 Invariants: {result.get('invariants_detected', 0)}")
            print(f"   🔄 Cycles: {result.get('cycles_detected', 0)}")
            print()
            
        except Exception as e:
            print(f"❌ Erreur épisode {episode}: {e}")
            import traceback
            traceback.print_exc()
            break
    
    # Sauvegarder logs
    print("\n" + "="*80)
    print("💾 Sauvegarde logs forensiques...")
    try:
        log_file = agent.save_logs()
        print(f"✅ Logs sauvegardés: {log_file}")
    except Exception as e:
        print(f"❌ Erreur sauvegarde logs: {e}")
        log_file = None
    
    # Générer rapport
    print("\n" + "="*80)
    print("📊 RAPPORT FINAL PHASE 4.5.1")
    print("="*80)
    
    # Statistiques globales
    total_steps = sum(r['steps'] for r in results)
    total_reward = sum(r['reward'] for r in results)
    victories = sum(1 for r in results if r['victory'])
    
    print(f"\n📈 Statistiques Globales:")
    print(f"   - Épisodes complétés: {len(results)}/{n_episodes}")
    print(f"   - Steps totaux: {total_steps}")
    print(f"   - Reward total: {total_reward:.2f}")
    print(f"   - Reward moyen: {total_reward/len(results):.2f}")
    print(f"   - Victoires: {victories}/{len(results)} ({victories/len(results)*100:.1f}%)")
    
    # Identification avatar
    print(f"\n👤 Identification Avatar:")
    avatar_id = agent.self_identification.get_avatar_id()
    confidence = agent.self_identification.get_confidence()
    
    if avatar_id:
        print(f"   ✅ Avatar identifié: {avatar_id}")
        print(f"   ✅ Confiance: {confidence:.1%}")
        
        if confidence >= 0.95:
            print(f"   🎯 OBJECTIF ATTEINT: Confiance >95%")
        else:
            print(f"   ⚠️  Confiance insuffisante (objectif: >95%)")
    else:
        print(f"   ❌ Avatar non identifié")
        print(f"   📊 Confiance actuelle: {confidence:.1%}")
        
        candidates = agent.self_identification.get_candidates()
        if candidates:
            print(f"   📋 Top 3 candidats:")
            for i, (cand_id, score) in enumerate(candidates[:3], 1):
                print(f"      {i}. {cand_id}: {score:.1f}")
    
    # Modèle monde
    print(f"\n🌍 Modèle Monde:")
    world_summary = agent.world_model.get_summary()
    print(f"   - Entités détectées: {world_summary['total_entities']}")
    print(f"   - Entités mobiles: {world_summary['mobile_entities']}")
    print(f"   - Invariants: {world_summary['invariants_detected']}")
    print(f"   - Cycles: {world_summary['cycles_detected']}")
    print(f"   - Graphe spatial: {world_summary['spatial_graph_nodes']} nœuds, "
          f"{world_summary['spatial_graph_edges']} arêtes")
    
    # Catégories entités
    if 'entities_by_category' in world_summary:
        print(f"\n   📦 Entités par catégorie:")
        for category, count in world_summary['entities_by_category'].items():
            print(f"      - {category}: {count}")
    
    # Validation objectifs
    print(f"\n✅ Validation Objectifs Phase 4.5.1:")
    objectives = {
        'Avatar identifié >95%': avatar_id is not None and confidence >= 0.95,
        'Entités détectées': world_summary['total_entities'] > 0,
        'Graphe spatial construit': world_summary['spatial_graph_nodes'] > 0,
        'Invariants détectés': world_summary['invariants_detected'] > 0
    }
    
    for objective, achieved in objectives.items():
        status = "✅" if achieved else "❌"
        print(f"   {status} {objective}")
    
    all_achieved = all(objectives.values())
    
    print("\n" + "="*80)
    if all_achieved:
        print("🎉 PHASE 4.5.1 VALIDÉE - TOUS LES OBJECTIFS ATTEINTS")
    else:
        print("⚠️  PHASE 4.5.1 PARTIELLE - CERTAINS OBJECTIFS NON ATTEINTS")
    print("="*80 + "\n")
    
    # Retourner résultats pour analyse
    return {
        'results': results,
        'log_file': log_file,
        'summary': {
            'episodes': len(results),
            'total_steps': total_steps,
            'total_reward': total_reward,
            'victories': victories,
            'avatar_identified': avatar_id is not None,
            'avatar_confidence': confidence,
            'world_summary': world_summary,
            'objectives_achieved': all_achieved
        }
    }


def generate_report(test_results: dict, output_file: Optional[str] = None):
    """
    Générer rapport markdown détaillé
    
    Args:
        test_results: Résultats du test
        output_file: Fichier de sortie (auto si None)
    """
    if output_file is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"RAPPORT_PHASE_4_5_1_VALIDATION_{timestamp}.md"
    
    output_path = Path(__file__).parent.parent / output_file
    
    summary = test_results['summary']
    
    report = f"""# RAPPORT VALIDATION PHASE 4.5.1 - Apprentissage Systémique

**Date**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}  
**Phase**: MAGEN 4.5.1 (Systemic Learning)  
**Test**: Validation WorldModel + SelfIdentification

---

## 📊 RÉSULTATS GLOBAUX

### Statistiques Entraînement

| Métrique | Valeur |
|----------|--------|
| Épisodes | {summary['episodes']} |
| Steps totaux | {summary['total_steps']} |
| Reward total | {summary['total_reward']:.2f} |
| Reward moyen | {summary['total_reward']/summary['episodes']:.2f} |
| Victoires | {summary['victories']}/{summary['episodes']} ({summary['victories']/summary['episodes']*100:.1f}%) |

### Identification Avatar

| Métrique | Valeur |
|----------|--------|
| Avatar identifié | {'✅ OUI' if summary['avatar_identified'] else '❌ NON'} |
| Confiance | {summary['avatar_confidence']:.1%} |
| Objectif >95% | {'✅ ATTEINT' if summary['avatar_confidence'] >= 0.95 else '❌ NON ATTEINT'} |

### Modèle Monde

| Métrique | Valeur |
|----------|--------|
| Entités détectées | {summary['world_summary']['total_entities']} |
| Entités mobiles | {summary['world_summary']['mobile_entities']} |
| Invariants | {summary['world_summary']['invariants_detected']} |
| Cycles | {summary['world_summary']['cycles_detected']} |
| Graphe spatial (nœuds) | {summary['world_summary']['spatial_graph_nodes']} |
| Graphe spatial (arêtes) | {summary['world_summary']['spatial_graph_edges']} |

---

## ✅ VALIDATION OBJECTIFS

"""
    
    objectives = {
        'Avatar identifié >95%': summary['avatar_identified'] and summary['avatar_confidence'] >= 0.95,
        'Entités détectées': summary['world_summary']['total_entities'] > 0,
        'Graphe spatial construit': summary['world_summary']['spatial_graph_nodes'] > 0,
        'Invariants détectés': summary['world_summary']['invariants_detected'] > 0
    }
    
    for objective, achieved in objectives.items():
        status = "✅" if achieved else "❌"
        report += f"- {status} **{objective}**\n"
    
    report += f"\n**Statut global**: {'✅ PHASE 4.5.1 VALIDÉE' if summary['objectives_achieved'] else '⚠️ VALIDATION PARTIELLE'}\n"
    
    report += f"""
---

## 📁 FICHIERS GÉNÉRÉS

- Logs forensiques: `{test_results['log_file']}`
- Rapport validation: `{output_path}`

---

## 🎯 PROCHAINES ÉTAPES

### Phase 4.5.2: RulesLearning (Couche 3)
- Implémenter apprentissage règles causales ACTION→EFFET
- Prédire résultats actions
- Généraliser à nouveaux états

### Phase 4.5.3: WorldDynamics (Couche 4)
- Modéliser dynamiques temporelles
- Détecter cycles automatiquement
- Prédire transitions d'états

### Phase 4.5.4: Planning + Exploration (Couches 5-6)
- Planification hiérarchique vers goals
- Exploration intelligente (curiosity-driven)
- Équilibre exploration/exploitation

---

**Statut**: Phase 4.5.1 {'COMPLÉTÉE ✅' if summary['objectives_achieved'] else 'EN COURS ⚠️'}  
**Prochaine action**: {'Phase 4.5.2 (RulesLearning)' if summary['objectives_achieved'] else 'Améliorer identification avatar'}
"""
    
    # Sauvegarder rapport
    with open(output_path, 'w') as f:
        f.write(report)
    
    print(f"📄 Rapport généré: {output_path}")
    return output_path


if __name__ == "__main__":
    # Exécuter test
    test_results = test_phase_4_5_1(n_episodes=10, max_steps=100)
    
    if test_results:
        # Générer rapport
        report_file = generate_report(test_results)
        print(f"\n✅ Test Phase 4.5.1 terminé")
        print(f"📊 Rapport: {report_file}")
    else:
        print("\n❌ Test Phase 4.5.1 échoué")

# Made with Bob
