"""
Test de validation intégration forensic V32
Vérifie que TOUS les modules génèrent bien des logs forensic
"""

import sys
import os
from pathlib import Path
import json
import time

# Ajouter le chemin pour imports
sys.path.insert(0, str(Path(__file__).parent))

def test_module_forensic(module_name, module_class, test_method=None):
    """Tester qu'un module génère bien des logs forensic."""
    print(f"\n{'='*60}")
    print(f"TEST: {module_name}")
    print(f"{'='*60}")
    
    try:
        # 1. Importer le module
        print(f"✓ Import {module_name}...")
        
        # 2. Nettoyer ancien log si existe
        log_path = f"logs/magen_v32/test_{module_name.lower()}.jsonl"
        if Path(log_path).exists():
            Path(log_path).unlink()
            print(f"✓ Ancien log nettoyé: {log_path}")
        
        # 3. Créer instance avec forensic
        print(f"✓ Création instance avec log: {log_path}")
        
        instance = module_class(forensic_log_path=log_path)
        
        # 3. Vérifier que forensic est initialisé
        assert hasattr(instance, 'forensic'), f"❌ {module_name} n'a pas d'attribut 'forensic'"
        print(f"✓ Attribut 'forensic' présent")
        
        # 4. Vérifier que _forensic_state existe
        assert hasattr(instance, '_forensic_state'), f"❌ {module_name} n'a pas de propriété '_forensic_state'"
        state = instance._forensic_state
        print(f"✓ Propriété '_forensic_state' présente: {list(state.keys())}")
        
        # 5. Vérifier que get_state() existe
        assert hasattr(instance, 'get_state'), f"❌ {module_name} n'a pas de méthode 'get_state()'"
        state = instance.get_state()
        print(f"✓ Méthode 'get_state()' présente")
        
        # 6. Vérifier que close() existe
        assert hasattr(instance, 'close'), f"❌ {module_name} n'a pas de méthode 'close()'"
        print(f"✓ Méthode 'close()' présente")
        
        # 7. Exécuter méthode de test si fournie
        if test_method:
            print(f"✓ Exécution méthode de test...")
            test_method(instance)
        
        # 8. Fermer proprement
        instance.close()
        print(f"✓ Fermeture propre")
        
        # 9. Vérifier que le fichier log existe
        if Path(log_path).exists():
            # Lire et compter les événements
            with open(log_path, 'r') as f:
                events = [json.loads(line) for line in f if line.strip()]
            
            print(f"✓ Fichier log créé: {len(events)} événements")
            
            # Vérifier événements minimum
            event_types = [e.get('event') for e in events]  # 'event' pas 'event_type'!
            print(f"  DEBUG: event_types = {event_types}")
            assert 'module_initialized' in event_types, f"❌ Événement 'module_initialized' manquant. Trouvé: {event_types}"
            assert 'module_closing' in event_types, f"❌ Événement 'module_closing' manquant. Trouvé: {event_types}"
            
            print(f"✓ Événements forensic: {event_types}")
            
            # Vérifier structure événements
            for event in events:
                assert 'timestamp_ns' in event, "❌ 'timestamp_ns' manquant"
                assert 'component' in event, "❌ 'component' manquant"
                assert 'event' in event, "❌ 'event' manquant"
            
            print(f"✓ Structure événements conforme")
            
            return True, len(events)
        else:
            print(f"❌ ERREUR: Fichier log non créé: {log_path}")
            return False, 0
            
    except Exception as e:
        print(f"❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        return False, 0

def main():
    """Tester tous les modules V32."""
    print("\n" + "="*60)
    print("VALIDATION INTÉGRATION FORENSIC V32")
    print("="*60)
    
    results = {}
    total_events = 0
    
    # Liste des modules à tester
    modules_to_test = [
        ("DecisionKernelV32Spatial", "core.decision_kernel_v32_spatial", "DecisionKernelV32Spatial"),
        ("MentalMapBuilder", "core.mental_map_builder", "MentalMapBuilder"),
        ("SpatialCompressor", "core.spatial_compressor", "SpatialCompressor"),
        ("RegionalStrategist", "core.regional_strategist", "RegionalStrategist"),
        ("CognitiveStabilizer", "core.cognitive_stabilizer", "CognitiveStabilizer"),
        ("MetaCognitiveMonitor", "core.metacognitive_monitor", "MetaCognitiveMonitor"),
        ("SelfSpatialIdentity", "core.self_spatial_identity", "SelfSpatialIdentity"),
        ("GoalHypothesisEngine", "core.goal_hypothesis_engine", "GoalHypothesisEngine"),
        ("TrajectoryMeaningSystem", "core.trajectory_meaning_system", "TrajectoryMeaningSystem"),
        ("SpatialKnowledgeGraph", "core.spatial_knowledge_graph", "SpatialKnowledgeGraph"),
    ]
    
    for module_name, module_path, class_name in modules_to_test:
        try:
            # Import dynamique
            module = __import__(module_path, fromlist=[class_name])
            module_class = getattr(module, class_name)
            
            # Test
            success, events = test_module_forensic(module_name, module_class)
            results[module_name] = (success, events)
            total_events += events
            
        except Exception as e:
            print(f"\n❌ ERREUR IMPORT {module_name}: {e}")
            results[module_name] = (False, 0)
    
    # Rapport final
    print("\n" + "="*60)
    print("RAPPORT FINAL")
    print("="*60)
    
    success_count = sum(1 for success, _ in results.values() if success)
    total_count = len(results)
    
    print(f"\nModules testés: {total_count}")
    print(f"Modules OK: {success_count}")
    print(f"Modules KO: {total_count - success_count}")
    print(f"Taux succès: {success_count/total_count*100:.1f}%")
    print(f"Total événements forensic: {total_events}")
    
    print("\nDétail par module:")
    for module_name, (success, events) in results.items():
        status = "✅" if success else "❌"
        print(f"  {status} {module_name}: {events} événements")
    
    if success_count == total_count:
        print("\n🎉 SUCCÈS: Tous les modules ont le forensic opérationnel!")
        return 0
    else:
        print(f"\n⚠️ ÉCHEC: {total_count - success_count} modules ont des problèmes")
        return 1

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
