"""
Tests Unitaires - Agent V27 COGNITIVE Forensic Integration
============================================================

Valide l'intégration forensic conforme PROTOCOLE_MAGEN V3.0:
- Hash chain blockchain-style
- Causal IDs hiérarchiques
- Logs perception/decision/learning
- Gestion épisodes/transitions

Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 71 (Forensic Integration)
"""

import pytest
import numpy as np
import os
import json
import tempfile
from pathlib import Path

# Import agent V27
import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from magen_arc_agi_agent_v27_cognitive import MAGENArcAgiAgentV27Cognitive, GridObservation
from core.state_encoder_v25 import StateEmbedding


class MockObs:
    """Mock observation pour tests"""
    def __init__(self, grid: np.ndarray):
        self.frame = [grid]  # ARC-AGI-3 format


@pytest.fixture
def temp_log_path():
    """Créer fichier log temporaire"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl', delete=False) as f:
        log_path = f.name
    yield log_path
    # Cleanup
    if os.path.exists(log_path):
        os.unlink(log_path)


@pytest.fixture
def agent_v27(temp_log_path):
    """Créer agent V27 avec forensic"""
    agent = MAGENArcAgiAgentV27Cognitive(forensic_log_path=temp_log_path)
    yield agent
    # Cleanup
    if hasattr(agent, 'forensic') and agent.forensic:
        agent.forensic.close()


def test_agent_v27_initialization_forensic(agent_v27, temp_log_path):
    """Test 1: Initialisation agent avec forensic"""
    # Vérifier forensic activé
    assert agent_v27.forensic is not None
    assert agent_v27.forensic_enabled is True
    
    # Vérifier log initialization généré
    assert os.path.exists(temp_log_path)
    
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    assert len(logs) >= 1
    init_log = logs[0]
    
    # Vérifier structure log
    assert 'event' in init_log
    assert init_log['event'] == 'initialization'
    assert 'data' in init_log
    assert 'version' in init_log['data']
    assert init_log['data']['version'] == 'V27_COGNITIVE'
    
    # Vérifier causal IDs (dans causal_context)
    assert 'causal_context' in init_log
    assert 'session_id' in init_log['causal_context']
    
    # Vérifier hash chain
    assert 'previous_hash' in init_log
    assert 'entry_hash' in init_log


def test_agent_v27_episode_management(agent_v27, temp_log_path):
    """Test 2: Gestion épisodes/transitions"""
    # Démarrer épisode
    agent_v27.start_episode()
    
    # Démarrer transition
    agent_v27.start_transition()
    
    # Terminer transition
    agent_v27.end_transition()
    
    # Terminer épisode
    agent_v27.end_episode()
    
    # Vérifier logs générés
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    # Trouver logs épisode/transition
    episode_start = [l for l in logs if l['event'] == 'episode_start']
    transition_start = [l for l in logs if l['event'] == 'transition_start']
    transition_end = [l for l in logs if l['event'] == 'transition_end']
    episode_end = [l for l in logs if l['event'] == 'episode_end']
    
    assert len(episode_start) >= 1
    assert len(transition_start) >= 1
    assert len(transition_end) >= 1
    assert len(episode_end) >= 1


def test_agent_v27_perceive_forensic(agent_v27, temp_log_path):
    """Test 3: Perception avec forensic"""
    # Créer observation mock
    grid = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    obs = MockObs(grid)
    
    # Démarrer épisode
    agent_v27.start_episode()
    
    # Percevoir
    grid_obs = agent_v27.perceive(obs)
    
    # Vérifier retour
    assert isinstance(grid_obs, GridObservation)
    assert grid_obs.grid is not None
    assert grid_obs.embedding is not None
    
    # Vérifier logs forensic
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    perception_logs = [l for l in logs if l['event'] == 'perception_complete']
    assert len(perception_logs) >= 1
    
    perc_log = perception_logs[-1]
    assert 'data' in perc_log
    assert 'grid_shape' in perc_log['data']
    assert 'embedding_dim' in perc_log['data']
    assert 'perception_time_ns' in perc_log['data']


def test_agent_v27_decide_forensic(agent_v27, temp_log_path):
    """Test 4: Décision avec forensic"""
    # Créer observation mock
    grid = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    obs = MockObs(grid)
    
    # Démarrer épisode
    agent_v27.start_episode()
    
    # Percevoir
    grid_obs = agent_v27.perceive(obs)
    
    # Décider
    available_actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
    decision = agent_v27.decide(grid_obs, available_actions)
    
    # Vérifier retour
    assert decision is not None
    assert decision.action_name in available_actions
    
    # Vérifier logs forensic
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    # Au moins un log de décision (grid_aware, reputation, ou exploration)
    decision_logs = [l for l in logs if 'decision' in l['event']]
    assert len(decision_logs) >= 1


def test_agent_v27_learn_forensic(agent_v27, temp_log_path):
    """Test 5: Apprentissage avec forensic"""
    # Créer observations mock
    grid_before = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    grid_after = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    
    obs_before = MockObs(grid_before)
    obs_after = MockObs(grid_after)
    
    # Démarrer épisode
    agent_v27.start_episode()
    
    # Percevoir avant/après
    grid_obs_before = agent_v27.perceive(obs_before)
    grid_obs_after = agent_v27.perceive(obs_after)
    
    # Apprendre
    action = 'ACTION1'
    reward = 1.0
    done = False
    
    agent_v27.learn(grid_obs_before, action, grid_obs_after, reward, done)
    
    # Vérifier logs forensic
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    learning_logs = [l for l in logs if l['event'] == 'learning_complete']
    assert len(learning_logs) >= 1
    
    learn_log = learning_logs[-1]
    assert 'data' in learn_log
    assert 'action' in learn_log['data']
    assert learn_log['data']['action'] == action
    assert 'env_reward' in learn_log['data']
    assert 'intrinsic_reward' in learn_log['data']
    assert 'total_reward' in learn_log['data']


def test_agent_v27_hash_chain_integrity(agent_v27, temp_log_path):
    """Test 6: Intégrité hash chain"""
    # Générer plusieurs logs
    grid = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    obs = MockObs(grid)
    
    agent_v27.start_episode()
    
    for _ in range(5):
        agent_v27.perceive(obs)
    
    # Vérifier hash chain
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    # Vérifier chaînage
    for i in range(1, len(logs)):
        prev_log = logs[i-1]
        curr_log = logs[i]
        
        # Hash précédent doit correspondre (entry_hash au lieu de current_hash)
        assert curr_log['previous_hash'] == prev_log['entry_hash']


def test_agent_v27_causal_hierarchy(agent_v27, temp_log_path):
    """Test 7: Hiérarchie causale (Session→Episode→Transition→Operation)"""
    # Générer logs avec hiérarchie
    grid = np.random.randint(0, 10, (10, 10), dtype=np.uint8)
    obs = MockObs(grid)
    
    agent_v27.start_episode()
    agent_v27.start_transition()
    agent_v27.perceive(obs)
    agent_v27.end_transition()
    agent_v27.end_episode()
    
    # Vérifier hiérarchie
    with open(temp_log_path, 'r') as f:
        logs = [json.loads(line) for line in f]
    
    # Tous les logs doivent avoir session_id dans causal_context
    session_ids = set(l['causal_context']['session_id'] for l in logs)
    assert len(session_ids) == 1  # Même session
    
    # Logs épisode doivent avoir episode_id dans data
    episode_logs = [l for l in logs if 'episode' in l['event']]
    if episode_logs:
        episode_ids = set(l['data'].get('episode_id') for l in episode_logs if 'episode_id' in l['data'])
        assert len(episode_ids) >= 1


if __name__ == '__main__':
    pytest.main([__file__, '-v', '-W', 'ignore::RuntimeWarning'])

# Made with Bob
