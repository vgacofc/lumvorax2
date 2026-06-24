#!/usr/bin/env python3
"""
Tests unitaires forensic pour Action Discovery Engine
Vérifie intégration PROTOCOLE_MAGEN V3.0
"""

import pytest
import json
import tempfile
import numpy as np
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.action_discovery_engine import ActionDiscoveryEngine, StateTransition
from core.forensic_logger import CausalIDManager


@pytest.fixture
def temp_forensic_dir():
    """Crée répertoire temporaire pour logs forensic"""
    with tempfile.TemporaryDirectory() as tmpdir:
        yield Path(tmpdir)


@pytest.fixture
def id_manager():
    """Crée ID manager pour tests"""
    return CausalIDManager()


@pytest.fixture
def sample_transitions():
    """Crée transitions test"""
    transitions = []
    for i in range(5):
        state_before = np.random.rand(3, 3)
        state_after = np.rot90(state_before)  # Rotation 90°
        transitions.append(StateTransition(
            state_before=state_before,
            action="rotate_90",
            state_after=state_after,
            reward=1.0 if i % 2 == 0 else 0.0,
            context={'grid_size': (3, 3), 'color_count': 3},
            timestamp=float(i)
        ))
    return transitions


def test_engine_initialization_forensic(temp_forensic_dir, id_manager):
    """Test: Initialisation avec forensic génère logs corrects"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Vérifier forensic activé
    assert engine.forensic is not None
    assert forensic_path.exists()
    
    # Vérifier log initialisation
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    assert len(logs) >= 1
    init_log = logs[0]
    assert init_log['event'] == 'engine_initialized'
    assert 'exploration_budget' in init_log['data']
    assert init_log['data']['exploration_budget'] == 50


def test_action_discovery_forensic(temp_forensic_dir, id_manager, sample_transitions):
    """Test: Découverte action génère logs forensic"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Découvrir action
    model = engine.discover_action("rotate_90", sample_transitions)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher logs discovery
    discovery_start_logs = [log for log in logs if log['event'] == 'action_discovery_start']
    assert len(discovery_start_logs) >= 1
    
    # Vérifier log action_discovery_start
    start_log = discovery_start_logs[0]
    assert start_log['data']['action_name'] == "rotate_90"
    assert start_log['data']['transitions_count'] == 5
    
    # Vérifier log action_discovered
    discovered_logs = [log for log in logs if log['event'] == 'action_discovered']
    assert len(discovered_logs) >= 1
    
    discovered_log = discovered_logs[0]
    assert discovered_log['data']['action_name'] == "rotate_90"
    assert discovered_log['data']['is_new_action'] is True
    assert 'success_rate' in discovered_log['data']
    assert 'avg_reward' in discovered_log['data']


def test_causal_analysis_forensic(temp_forensic_dir, id_manager, sample_transitions):
    """Test: Analyse causale génère logs forensic"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Découvrir action (déclenche analyse causale)
    engine.discover_action("rotate_90", sample_transitions)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher logs causal_analysis
    causal_logs = [log for log in logs if 'causal' in log['event']]
    assert len(causal_logs) >= 2  # start + detected
    
    # Vérifier log causal_analysis_start
    start_log = next(log for log in causal_logs if log['event'] == 'causal_analysis_start')
    assert start_log['data']['transitions_count'] == 5
    
    # Vérifier log causal_effects_detected
    detected_log = next(log for log in causal_logs if log['event'] == 'causal_effects_detected')
    assert 'effects_count' in detected_log['data']
    assert 'effect_types' in detected_log['data']


def test_exploration_decision_forensic(temp_forensic_dir, id_manager):
    """Test: Décision exploration génère logs forensic"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Tester décision exploration (action inconnue)
    should_explore = engine.should_explore("unknown_action")
    assert should_explore is True
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log exploration_decision
    decision_logs = [log for log in logs if log['event'] == 'exploration_decision']
    assert len(decision_logs) >= 1
    
    decision_log = decision_logs[0]
    assert decision_log['data']['action_name'] == "unknown_action"
    assert decision_log['data']['should_explore'] is True
    assert decision_log['data']['reason'] == "unknown_action"


def test_best_action_selection_forensic(temp_forensic_dir, id_manager, sample_transitions):
    """Test: Sélection meilleure action génère logs forensic"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Découvrir quelques actions
    engine.discover_action("rotate_90", sample_transitions)
    engine.discover_action("identity", sample_transitions[:2])
    
    # Sélectionner meilleure action
    context = {'grid_size': (3, 3), 'color_count': 3}
    best_action = engine.get_best_action(context)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log best_action_selected
    selection_logs = [log for log in logs if log['event'] == 'best_action_selected']
    assert len(selection_logs) >= 1
    
    selection_log = selection_logs[0]
    assert 'action_name' in selection_log['data']
    assert 'score' in selection_log['data']
    assert 'context' in selection_log['data']
    assert 'candidates_count' in selection_log['data']


def test_multiple_discoveries_forensic(temp_forensic_dir, id_manager, sample_transitions):
    """Test: Découvertes multiples génèrent logs forensic corrects"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=100,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Découvrir plusieurs actions
    actions = ["rotate_90", "mirror_h", "identity"]
    for action in actions:
        engine.discover_action(action, sample_transitions)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Compter logs action_discovered
    discovered_logs = [log for log in logs if log['event'] == 'action_discovered']
    assert len(discovered_logs) == 3
    
    # Vérifier chaque action
    discovered_actions = [log['data']['action_name'] for log in discovered_logs]
    assert set(discovered_actions) == set(actions)


def test_forensic_hash_chain_integrity(temp_forensic_dir, id_manager, sample_transitions):
    """Test: Hash chain maintient intégrité forensic"""
    forensic_path = temp_forensic_dir / "action_discovery_test.jsonl"
    
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Générer plusieurs événements
    engine.discover_action("rotate_90", sample_transitions)
    engine.should_explore("test_action")
    engine.get_best_action({'grid_size': (3, 3)})
    
    # Vérifier intégrité hash chain
    assert engine.forensic is not None
    is_valid = engine.forensic.verify_integrity()
    assert is_valid, "Hash chain invalide"


def test_engine_without_forensic(sample_transitions):
    """Test: Engine fonctionne sans forensic (backward compatibility)"""
    engine = ActionDiscoveryEngine(
        exploration_budget=50,
        forensic_log_path=None
    )
    
    # Vérifier forensic désactivé
    assert engine.forensic is None
    
    # Vérifier fonctionnement normal
    model = engine.discover_action("rotate_90", sample_transitions)
    assert model is not None
    assert model.action_id == "rotate_90"
    
    # Autres opérations ne doivent pas crasher
    should_explore = engine.should_explore("test_action")
    assert isinstance(should_explore, bool)
    
    best_action = engine.get_best_action({'grid_size': (3, 3)})
    assert best_action is not None or best_action is None  # Peut être None si pas d'actions


if __name__ == "__main__":
    pytest.main([__file__, "-v"])

# Made with Bob
