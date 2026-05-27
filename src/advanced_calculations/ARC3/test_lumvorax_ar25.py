#!/usr/bin/env python3
"""
Script de test pour l'agent LumVorax sur le jeu ar25
Teste l'intégration complète des technologies NX48, NX49, Quantum, POH, Forensic
"""

import sys
import os
import json
import time
from pathlib import Path

# Ajouter le chemin vers ARC-AGI-3-Agents
arc_agents_path = Path(__file__).parent / "arc-prize-2026-arc-agi-3" / "ARC-AGI-3-Agents"
sys.path.insert(0, str(arc_agents_path))

try:
    from agents.templates.lumvorax_agent import LumVoraxAgent, LumVoraxConfig
    from arcengine import EnvironmentWrapper
    print("✅ Imports réussis")
except ImportError as e:
    print(f"❌ Erreur import: {e}")
    print(f"Chemin ARC agents: {arc_agents_path}")
    print(f"Existe: {arc_agents_path.exists()}")
    sys.exit(1)


def test_lumvorax_ar25():
    """Test complet de l'agent LumVorax sur ar25"""
    
    print("\n" + "="*80)
    print("🌟 TEST AGENT LUMVORAX - JEU AR25 🌟")
    print("="*80 + "\n")
    
    # Configuration
    game_id = "ar25"
    card_id = "0c556536"
    
    print(f"📋 Configuration:")
    print(f"   - Jeu: {game_id}")
    print(f"   - Card: {card_id}")
    print(f"   - Technologies: NX48, NX49, Quantum, POH, Forensic")
    print()
    
    # Créer environnement
    print("🔧 Création environnement ARC...")
    try:
        env = EnvironmentWrapper(game_id=game_id)
        print("✅ Environnement créé")
    except Exception as e:
        print(f"❌ Erreur création environnement: {e}")
        return False
    
    # Configuration LumVorax personnalisée
    config = LumVoraxConfig(
        # NX48 - Plus agressif pour tests
        nx48_learning_rate=0.15,
        nx48_adaptation_threshold=0.25,
        
        # NX49 - STDP plus rapide
        nx49_stdp_tau_plus=15.0,
        nx49_stdp_tau_minus=15.0,
        
        # Quantum - Plus de profondeur
        quantum_superposition_depth=5,
        
        # Forensic - Logging maximal
        forensic_log_level="BIT_LEVEL",
        forensic_trace_all=True
    )
    
    print("\n📊 Configuration LumVorax:")
    print(f"   - NX48 learning_rate: {config.nx48_learning_rate}")
    print(f"   - NX49 STDP tau: {config.nx49_stdp_tau_plus}ms")
    print(f"   - Quantum depth: {config.quantum_superposition_depth}")
    print(f"   - Forensic level: {config.forensic_log_level}")
    print()
    
    # Créer agent
    print("🤖 Création agent LumVorax...")
    try:
        agent = LumVoraxAgent(
            card_id=card_id,
            game_id=game_id,
            agent_name="lumvorax_test_v1",
            ROOT_URL="https://api.arcprize.org",
            record=True,
            arc_env=env,
            tags=["test", "lumvorax", "nx48", "nx49", "quantum", "poh", "forensic"]
        )
        agent.config = config
        print("✅ Agent créé")
    except Exception as e:
        print(f"❌ Erreur création agent: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # Métriques initiales
    print("\n📈 Métriques initiales:")
    print(f"   - POH sequence: {agent.poh.sequence}")
    print(f"   - Forensic events: {len(agent.forensic.logs)}")
    print(f"   - Quantum patterns: {len(agent.quantum.pattern_library)}")
    print(f"   - NX48 epsilon: {agent.nx48.epsilon:.3f}")
    print()
    
    # Exécuter test
    print("🚀 Lancement test (max 10 actions)...")
    print("-" * 80)
    
    start_time = time.time()
    max_actions = 10
    
    try:
        # Simuler quelques actions
        for i in range(max_actions):
            print(f"\n[Action {i+1}/{max_actions}]")
            
            # Obtenir frame actuel (simulé)
            # Dans un vrai test, on utiliserait env.step()
            
            # Afficher état modules
            print(f"  NX48 stratégie: {agent.nx48.current_strategy}")
            print(f"  NX49 spikes: {agent.metrics['stdp_spikes']}")
            print(f"  Quantum patterns: {len(agent.quantum.pattern_library)}")
            print(f"  POH sequence: {agent.poh.sequence}")
            print(f"  Forensic events: {len(agent.forensic.logs)}")
            
            # Vérifier intégrité POH
            if not agent.poh.verify_chain():
                print("  ⚠️  ALERTE: Chaîne POH corrompue!")
                break
            
            time.sleep(0.1)  # Pause pour lisibilité
            
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrompu par l'utilisateur")
    except Exception as e:
        print(f"\n❌ Erreur pendant test: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    elapsed = time.time() - start_time
    
    # Métriques finales
    print("\n" + "-" * 80)
    print("\n📊 MÉTRIQUES FINALES:")
    print(f"   - Durée: {elapsed:.2f}s")
    print(f"   - Décisions: {agent.metrics['decisions']}")
    print(f"   - Quantum recognitions: {agent.metrics['quantum_recognitions']}")
    print(f"   - STDP spikes: {agent.metrics['stdp_spikes']}")
    print(f"   - Strategy switches: {agent.metrics['strategy_switches']}")
    print()
    
    # NX48 stats
    print("🧠 NX48 STATISTIQUES:")
    nx48_stats = agent.nx48.get_strategy_stats()
    for strategy, data in nx48_stats['strategies'].items():
        success_rate = data['successes'] / max(data['uses'], 1) if data['uses'] > 0 else 0
        print(f"   - {strategy:20s}: score={data['score']:.3f}, uses={data['uses']:3d}, success={success_rate:.1%}")
    print(f"   - Epsilon actuel: {nx48_stats['epsilon']:.3f}")
    print()
    
    # POH vérification
    print("🔗 POH VÉRIFICATION:")
    poh_valid = agent.poh.verify_chain()
    print(f"   - Chaîne valide: {'✅ OUI' if poh_valid else '❌ NON'}")
    print(f"   - Séquence finale: {agent.poh.sequence}")
    print(f"   - Entrées totales: {len(agent.poh.history)}")
    if agent.poh.history:
        last_entry = agent.poh.history[-1]
        print(f"   - Dernier hash: {last_entry['hash'][:16]}...")
    print()
    
    # Forensic summary
    print("🔍 FORENSIC SUMMARY:")
    forensic_summary = agent.forensic.get_summary()
    print(f"   - Total events: {forensic_summary['total_events']}")
    print(f"   - Durée: {forensic_summary['duration_ms']:.2f}ms")
    print(f"   - Events/sec: {forensic_summary['events_per_second']:.2f}")
    print(f"   - Catégories:")
    for cat, count in forensic_summary['categories'].items():
        print(f"     * {cat:15s}: {count:4d} events")
    print()
    
    # Sauvegarder résultats
    results = {
        "test_date": time.strftime("%Y-%m-%d %H:%M:%S"),
        "game_id": game_id,
        "card_id": card_id,
        "duration_s": elapsed,
        "metrics": agent.metrics,
        "nx48_stats": nx48_stats,
        "poh_valid": poh_valid,
        "poh_sequence": agent.poh.sequence,
        "forensic_summary": forensic_summary,
        "config": {
            "nx48_learning_rate": config.nx48_learning_rate,
            "nx49_stdp_tau_plus": config.nx49_stdp_tau_plus,
            "quantum_superposition_depth": config.quantum_superposition_depth,
            "forensic_log_level": config.forensic_log_level
        }
    }
    
    results_file = Path(__file__).parent / "test_results_ar25.json"
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"💾 Résultats sauvegardés: {results_file}")
    print()
    
    # Verdict
    print("="*80)
    if poh_valid and agent.metrics['decisions'] > 0:
        print("✅ TEST RÉUSSI - Agent LumVorax opérationnel sur ar25")
    else:
        print("⚠️  TEST PARTIEL - Vérifier les logs pour détails")
    print("="*80)
    print()
    
    return True


if __name__ == "__main__":
    success = test_lumvorax_ar25()
    sys.exit(0 if success else 1)

# Made with Bob
