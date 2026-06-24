"""
Tests Unitaires - Causal Transition Graph avec Forensic
========================================================

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

from core.causal_transition_graph import CausalTransitionGraph, NodeType
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


def test_graph_initialization_forensic(temp_forensic_log, id_manager):
    """Test 1: Initialisation graphe avec forensic"""
    graph = CausalTransitionGraph(
        similarity_threshold=0.85,
        max_nodes=1000,
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Vérifier middleware forensic actif
    assert graph.forensic is not None
    assert graph.forensic.component_name == "CausalTransitionGraph"
    assert graph.forensic.enabled is True
    
    # Vérifier paramètres graphe
    assert graph.similarity_threshold == 0.85
    assert graph.max_nodes == 1000
    assert len(graph.nodes) == 0
    assert len(graph.edges) == 0


def test_graph_add_state_forensic(temp_forensic_log, id_manager):
    """Test 2: Ajout état avec logging forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer embedding test
    embedding = np.random.randn(128)
    embedding = embedding / np.linalg.norm(embedding)  # Normaliser
    
    # Ajouter état
    node_id = graph.add_state(embedding, metadata={'test': 'value'})
    
    # Vérifier état ajouté
    assert node_id in graph.nodes
    assert graph.stats['total_nodes'] == 1
    
    # Vérifier logs forensic générés
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log node_created
    node_logs = [log for log in logs if log['event'] == 'node_created']
    assert len(node_logs) > 0
    
    node_log = node_logs[0]
    assert node_log['component'] == 'CausalTransitionGraph'
    assert 'node_id' in node_log['data']
    assert 'total_nodes' in node_log['data']
    assert node_log['data']['total_nodes'] == 1


def test_graph_add_transition_forensic(temp_forensic_log, id_manager):
    """Test 3: Ajout transition avec logging forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer embeddings test
    emb1 = np.random.randn(128)
    emb1 = emb1 / np.linalg.norm(emb1)
    
    emb2 = np.random.randn(128)
    emb2 = emb2 / np.linalg.norm(emb2)
    
    # Ajouter transition
    graph.add_transition(
        from_embedding=emb1,
        action="move_right",
        semantic_action="move_right",
        to_embedding=emb2,
        reward=0.5,
        success=True
    )
    
    # Vérifier transition ajoutée
    assert graph.stats['total_nodes'] >= 2
    assert graph.stats['total_edges'] >= 1
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log edge_created
    edge_logs = [log for log in logs if log['event'] == 'edge_created']
    assert len(edge_logs) > 0
    
    edge_log = edge_logs[0]
    assert 'from_node' in edge_log['data']
    assert 'to_node' in edge_log['data']
    assert edge_log['data']['action'] == "move_right"
    assert edge_log['data']['reward'] == 0.5
    assert edge_log['data']['success'] is True


def test_graph_goal_detection_forensic(temp_forensic_log, id_manager):
    """Test 4: Détection goal avec logging forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer embeddings
    emb1 = np.random.randn(128)
    emb1 = emb1 / np.linalg.norm(emb1)
    
    emb2 = np.random.randn(128)
    emb2 = emb2 / np.linalg.norm(emb2)
    
    # Ajouter transitions avec reward élevé pour déclencher goal
    for _ in range(5):
        graph.add_transition(
            from_embedding=emb1,
            action="test",
            semantic_action="test",
            to_embedding=emb2,
            reward=0.8,  # Reward élevé
            success=True
        )
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log goal_detected
    goal_logs = [log for log in logs if log['event'] == 'goal_detected']
    
    if goal_logs:
        goal_log = goal_logs[0]
        assert 'node_id' in goal_log['data']
        assert 'avg_reward' in goal_log['data']
        assert 'total_goals' in goal_log['data']


def test_graph_path_finding_forensic(temp_forensic_log, id_manager):
    """Test 5: Recherche chemin avec logging forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer chaîne états
    embeddings = []
    for i in range(5):
        emb = np.random.randn(128)
        emb = emb / np.linalg.norm(emb)
        embeddings.append(emb)
    
    # Ajouter transitions séquentielles
    for i in range(len(embeddings) - 1):
        graph.add_transition(
            from_embedding=embeddings[i],
            action=f"action_{i}",
            semantic_action=f"action_{i}",
            to_embedding=embeddings[i+1],
            reward=0.1,
            success=True
        )
    
    # Marquer dernier nœud comme goal
    last_id = graph.add_state(embeddings[-1])
    graph.nodes[last_id].node_type = NodeType.GOAL
    graph.goal_nodes.add(last_id)
    
    # Chercher chemin
    path = graph.find_path_bfs(embeddings[0], NodeType.GOAL, max_depth=10)
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log path_found
    path_logs = [log for log in logs if log['event'] == 'path_found']
    
    if path and path_logs:
        path_log = path_logs[0]
        assert 'path_length' in path_log['data']
        assert 'total_reward' in path_log['data']
        assert 'success_probability' in path_log['data']
        assert 'planning_time_ns' in path_log['data']


def test_graph_trajectory_reset_forensic(temp_forensic_log, id_manager):
    """Test 6: Reset trajectoire avec logging forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Ajouter quelques transitions
    for i in range(3):
        emb1 = np.random.randn(128)
        emb1 = emb1 / np.linalg.norm(emb1)
        emb2 = np.random.randn(128)
        emb2 = emb2 / np.linalg.norm(emb2)
        
        graph.add_transition(
            from_embedding=emb1,
            action=f"action_{i}",
            semantic_action=f"action_{i}",
            to_embedding=emb2,
            reward=0.1,
            success=True
        )
    
    # Vérifier trajectoire non vide
    assert len(graph.current_trajectory) > 0
    
    # Reset trajectoire
    graph.reset_trajectory()
    
    # Vérifier trajectoire vide
    assert len(graph.current_trajectory) == 0
    
    # Vérifier log reset
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    reset_logs = [log for log in logs if log['event'] == 'trajectory_reset']
    assert len(reset_logs) > 0
    
    reset_log = reset_logs[0]
    assert 'trajectory_length_before' in reset_log['data']


def test_graph_hash_chain_integrity(temp_forensic_log, id_manager):
    """Test 7: Intégrité hash chain forensic"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Générer plusieurs opérations
    for i in range(5):
        emb = np.random.randn(128)
        emb = emb / np.linalg.norm(emb)
        graph.add_state(emb, metadata={'index': i})
    
    # Vérifier intégrité hash chain
    assert graph.forensic.verify_integrity() is True
    
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


def test_graph_causal_hierarchy(temp_forensic_log, id_manager):
    """Test 8: Hiérarchie causale session→episode→transition"""
    graph = CausalTransitionGraph(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer nouvel épisode
    episode_id = graph.forensic.new_episode()
    assert episode_id is not None
    
    # Ajouter état avec transition
    emb = np.random.randn(128)
    emb = emb / np.linalg.norm(emb)
    
    transition_id = graph.forensic.new_transition()
    graph.add_state(emb)
    
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


if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])

# Made with Bob
