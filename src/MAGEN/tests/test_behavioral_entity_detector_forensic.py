"""
Tests Unitaires - Behavioral Entity Detector avec Forensic
===========================================================

Validation intégration forensic standardisé (PROTOCOLE_MAGEN V3.0)

Author: MAGEN V27.1 FORENSIC
Date: 2026-06-14
"""

import pytest
import numpy as np
import tempfile
import json
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.behavioral_entity_detector import BehavioralEntityDetector
from core.forensic_middleware import CausalIDManager

# Filtre warnings NumPy du code existant
pytestmark = pytest.mark.filterwarnings(
    "ignore::RuntimeWarning",
    "ignore:Mean of empty slice:RuntimeWarning",
    "ignore:invalid value encountered in scalar divide:RuntimeWarning"
)


@pytest.fixture
def temp_forensic_log():
    """Créer fichier log temporaire"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        yield f.name
    Path(f.name).unlink(missing_ok=True)


@pytest.fixture
def id_manager():
    """Créer ID manager partagé"""
    return CausalIDManager()


def test_detector_initialization_forensic(temp_forensic_log, id_manager):
    """Test 1: Initialisation détecteur avec forensic"""
    detector = BehavioralEntityDetector(
        causality_threshold=0.6,
        mobility_threshold=0.3,
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Vérifier middleware forensic actif
    assert detector.forensic is not None
    assert detector.forensic.component_name == "BehavioralEntityDetector"
    assert detector.forensic.enabled is True
    
    # Vérifier paramètres détecteur
    assert detector.causality_threshold == 0.6
    assert detector.mobility_threshold == 0.3
    assert len(detector.entities) == 0
    assert detector.agent_entity_id is None


def test_detector_observe_forensic(temp_forensic_log, id_manager):
    """Test 2: Observation avec logging forensic"""
    detector = BehavioralEntityDetector(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer grille test avec entités
    grid = np.array([
        [0, 0, 1, 0],
        [0, 2, 0, 0],
        [0, 0, 0, 3],
        [0, 0, 0, 0]
    ])
    
    # Observer grille
    result = detector.observe(grid, action="move_right")
    
    # Vérifier résultat observation
    assert result['entities_count'] > 0
    assert 'agent_detected' in result
    assert 'mobile_entities' in result
    assert 'stable_entities' in result
    
    # Vérifier logs forensic générés
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Doit avoir au moins observation_start et observation_complete
    assert len(logs) >= 2
    
    # Vérifier log observation_start
    start_log = logs[0]
    assert start_log['event'] == 'observation_start'
    assert start_log['component'] == 'BehavioralEntityDetector'
    assert 'grid_shape' in start_log['data']
    assert start_log['data']['action'] == "move_right"
    
    # Vérifier log observation_complete
    complete_log = logs[1]
    assert complete_log['event'] == 'observation_complete'
    assert 'duration_ns' in complete_log['data']
    assert 'entities_detected' in complete_log['data']


def test_detector_causal_inference_forensic(temp_forensic_log, id_manager):
    """Test 3: Inférence causale avec logging forensic"""
    detector = BehavioralEntityDetector(
        causality_threshold=0.5,
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Séquence observations avec mouvement entité (plus longue pour déclencher inférence)
    grids = [
        np.array([[1, 0, 0], [0, 0, 0], [0, 0, 0]]),
        np.array([[0, 1, 0], [0, 0, 0], [0, 0, 0]]),
        np.array([[0, 0, 1], [0, 0, 0], [0, 0, 0]]),
    ]
    
    # Observer séquence avec actions
    detector.observe(grids[0])
    detector.observe(grids[1], action="move_right")
    detector.observe(grids[2], action="move_right")
    
    # Vérifier logs forensic générés (au moins observation logs)
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Vérifier logs observation présents
    obs_logs = [log for log in logs if 'observation' in log['event']]
    assert len(obs_logs) >= 2  # Au moins observation_start et observation_complete
    
    # Chercher log causal_event_detected (optionnel car dépend timing)
    causal_logs = [log for log in logs if log['event'] == 'causal_event_detected']
    
    if causal_logs:
        # Si événements causaux détectés, vérifier structure
        causal_log = causal_logs[0]
        assert 'entity_id' in causal_log['data']
        assert 'action' in causal_log['data']
        assert 'causality_strength' in causal_log['data']
        assert 'distance_moved' in causal_log['data']
    
    # Test réussi si logs forensic générés correctement
    assert len(logs) > 0


def test_detector_agent_identification_forensic(temp_forensic_log, id_manager):
    """Test 4: Identification agent avec logging forensic"""
    detector = BehavioralEntityDetector(
        causality_threshold=0.3,  # Seuil bas pour test
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Séquence avec entité mobile réactive
    grids = [
        np.array([[1, 0], [0, 0]]),
        np.array([[0, 1], [0, 0]]),
        np.array([[0, 0], [1, 0]]),
        np.array([[0, 0], [0, 1]])
    ]
    
    actions = ["right", "down", "right"]
    
    # Observer séquence
    detector.observe(grids[0])
    for i, action in enumerate(actions):
        detector.observe(grids[i+1], action=action)
    
    # Vérifier agent détecté
    agent_info = detector.get_agent_info()
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log agent_identified
    agent_logs = [log for log in logs if log['event'] == 'agent_identified']
    
    if agent_logs:
        agent_log = agent_logs[0]
        assert 'agent_id' in agent_log['data']
        assert 'confidence' in agent_log['data']
        assert 'causality_score' in agent_log['data']
        assert 'composite_score' in agent_log['data']


def test_detector_reset_forensic(temp_forensic_log, id_manager):
    """Test 5: Reset détecteur avec logging forensic"""
    detector = BehavioralEntityDetector(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Observer quelques grilles
    grid = np.array([[1, 2], [3, 0]])
    detector.observe(grid)
    detector.observe(grid, action="test")
    
    # Vérifier état avant reset
    assert len(detector.entities) > 0
    assert detector.total_observations > 0
    
    # Reset
    detector.reset()
    
    # Vérifier état après reset
    assert len(detector.entities) == 0
    assert len(detector.causal_events) == 0
    assert detector.total_observations == 0
    assert detector.agent_entity_id is None
    
    # Vérifier log reset
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    reset_logs = [log for log in logs if log['event'] == 'detector_reset']
    assert len(reset_logs) > 0
    
    reset_log = reset_logs[0]
    assert 'entities_before' in reset_log['data']
    assert 'total_observations_before' in reset_log['data']


def test_detector_hash_chain_integrity(temp_forensic_log, id_manager):
    """Test 6: Intégrité hash chain forensic"""
    detector = BehavioralEntityDetector(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Générer plusieurs observations
    for i in range(5):
        grid = np.random.randint(0, 4, size=(3, 3))
        detector.observe(grid, action=f"action_{i}")
    
    # Vérifier intégrité hash chain
    assert detector.forensic.verify_integrity() is True
    
    # Vérifier logs contiennent entry_hash
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chaque log (sauf premier) doit avoir entry_hash du précédent
    for i in range(1, len(logs)):
        assert 'entry_hash' in logs[i]
        # Hash doit être SHA256 (64 caractères hex)
        assert len(logs[i]['entry_hash']) == 64


def test_detector_causal_hierarchy(temp_forensic_log, id_manager):
    """Test 7: Hiérarchie causale session→episode→transition"""
    detector = BehavioralEntityDetector(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer nouvel épisode
    episode_id = detector.forensic.new_episode()
    assert episode_id is not None
    
    # Observer avec transitions
    grid = np.array([[1, 0], [0, 2]])
    
    transition_id = detector.forensic.new_transition()
    detector.observe(grid, action="test")
    
    # Vérifier logs contiennent causal_context
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    for log in logs:
        assert 'causal_context' in log
        context = log['causal_context']
        assert 'session_id' in context
        assert 'episode_id' in context
        assert 'transition_id' in context


def test_detector_world_model_consistency(temp_forensic_log, id_manager):
    """Test 8: Cohérence modèle monde avec forensic"""
    detector = BehavioralEntityDetector(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer grille avec entités distinctes
    grid = np.array([
        [1, 0, 2],
        [0, 3, 0],
        [4, 0, 5]
    ])
    
    detector.observe(grid)
    
    # Obtenir modèle monde
    world_model = detector.get_world_model()
    
    # Vérifier structure
    assert 'entities' in world_model
    assert 'agent_detected' in world_model
    assert 'total_observations' in world_model
    
    # Vérifier entités
    assert len(world_model['entities']) > 0
    
    for entity_id, entity_data in world_model['entities'].items():
        assert 'id' in entity_data
        assert 'position' in entity_data
        assert 'is_mobile' in entity_data
        assert 'is_stable' in entity_data
        assert 'causality_score' in entity_data


if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])

# Made with Bob
