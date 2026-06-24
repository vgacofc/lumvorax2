"""
Test MDBAI/PILOT Workflow Engine

Valide le fonctionnement du workflow engine central qui orchestre
l'exécution structurée de MAGEN V35+.

Version: 1.0.0
Date: 2026-06-15
Auteur: Bob (LVX AI Systems)
"""

import json
import sys
from pathlib import Path

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent.parent))

from MAGEN.core.mdbai_pilot_engine import (
    MDBAIPilotEngine,
    WorkflowPhase,
    WorkflowStatus
)


class MockModule:
    """Module mock pour tests"""
    
    def __init__(self, name: str):
        self.name = name
        self.executions = 0
    
    def execute(self, input_data):
        """Exécute le module"""
        self.executions += 1
        return {"result": f"{self.name}_output", "input": input_data}
    
    def get_state(self):
        """Retourne état"""
        return {"executions": self.executions}


def test_mdbai_pilot_engine():
    """Test complet du MDBAI/PILOT Engine"""
    
    print("\n" + "="*80)
    print("TEST MDBAI/PILOT WORKFLOW ENGINE")
    print("="*80)
    
    # Créer engine
    print("\n✓ Création MDBAIPilotEngine...")
    log_path = "logs/test_mdbai_pilot/engine_test.jsonl"
    if Path(log_path).exists():
        Path(log_path).unlink()
    
    engine = MDBAIPilotEngine(
        max_workflow_duration_s=60.0,
        enable_auto_abort=True,
        forensic_log_path=log_path
    )
    
    # Vérifier API forensic
    print("\n✓ Vérification API forensic...")
    assert hasattr(engine, 'forensic')
    assert hasattr(engine, '_forensic_state')
    assert hasattr(engine, 'get_state')
    assert hasattr(engine, 'close')
    print("  ✅ API forensic complète")
    
    # Enregistrer modules mock
    print("\n✓ Enregistrement modules...")
    modules = {
        WorkflowPhase.AFFORDANCE_DISCOVERY: MockModule("affordance"),
        WorkflowPhase.CANDIDATE_FILTERING: MockModule("filtering"),
        WorkflowPhase.REPUTATION_PRIORITIZATION: MockModule("reputation"),
        WorkflowPhase.BUDGET_ALLOCATION: MockModule("budget"),
        WorkflowPhase.TRAJECTORY_MONITORING: MockModule("trajectory"),
        WorkflowPhase.GLOBAL_REGRET_DETECTION: MockModule("regret"),
        WorkflowPhase.SYMBOLIC_VALIDATION: MockModule("validation")
    }
    
    for phase, module in modules.items():
        engine.register_module(phase, module)
    
    print(f"  ✅ {len(modules)} modules enregistrés")
    
    # Démarrer workflow
    print("\n✓ Démarrage workflow...")
    workflow_id = engine.start_workflow("test_puzzle_001")
    print(f"  ✅ Workflow ID: {workflow_id}")
    assert engine.current_workflow is not None
    assert engine.current_workflow.status == WorkflowStatus.RUNNING
    
    # Exécuter phases
    print("\n✓ Exécution phases...")
    input_data = {"puzzle": "test_data"}
    
    for phase in WorkflowPhase:
        result = engine.execute_phase(phase, input_data)
        print(f"  ✅ Phase {phase.value}: {result.status}")
        assert result.status == "success"
        input_data = result.output  # Chaîner les phases
    
    # Compléter workflow
    print("\n✓ Complétion workflow...")
    solution = {"grid": [[1, 2], [3, 4]]}
    engine.complete_workflow(solution)
    
    assert engine.current_workflow is None
    assert engine.successful_workflows == 1
    assert len(engine.workflow_history) == 1
    print("  ✅ Workflow complété avec succès")
    
    # Vérifier état
    print("\n✓ Vérification état...")
    state = engine.get_state()
    print(f"  ✅ Total workflows: {state['statistics']['total_workflows']}")
    print(f"  ✅ Success rate: {state['statistics']['success_rate']:.2%}")
    assert state['statistics']['total_workflows'] == 1
    assert state['statistics']['successful_workflows'] == 1
    assert state['statistics']['success_rate'] == 1.0
    
    # Test workflow aborté
    print("\n✓ Test workflow aborté...")
    workflow_id2 = engine.start_workflow("test_puzzle_002")
    engine.abort_workflow("Test abort")
    
    assert engine.current_workflow is None
    assert engine.aborted_workflows == 1
    print("  ✅ Workflow aborté correctement")
    
    # Fermer engine
    engine.close()
    
    # Valider logs forensic
    print("\n✓ Validation logs forensic...")
    with open(log_path) as f:
        events = [json.loads(line) for line in f]
    
    print(f"  ✅ {len(events)} événements générés")
    
    event_types = [e['event'] for e in events]
    assert 'module_initialized' in event_types
    assert 'workflow_started' in event_types
    assert 'phase_executed' in event_types
    assert 'workflow_completed' in event_types
    assert 'workflow_aborted' in event_types
    assert 'module_closing' in event_types
    
    print(f"  ✅ Types: {set(event_types)}")
    
    print("\n" + "="*80)
    print("✅ TEST RÉUSSI: MDBAI/PILOT Engine 100% fonctionnel")
    print("="*80)
    
    return True, len(events)


def main():
    """Point d'entrée"""
    try:
        success, events = test_mdbai_pilot_engine()
        
        print("\n" + "="*80)
        print("RÉSUMÉ TEST MDBAI/PILOT")
        print("="*80)
        print(f"✅ Test réussi: {success}")
        print(f"✅ Événements forensic: {events}")
        print(f"✅ Workflow engine: OPÉRATIONNEL")
        print("="*80)
        
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True


if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)


# Made with ❤️ by Bob (LVX AI Systems)
# Protocole: MDBAI/PILOT + LUMVORAX + LEÇON-73.1

# Made with Bob
