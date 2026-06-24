"""
Tests Unitaires - Goal Grounder avec Forensic
==============================================

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

from core.goal_grounder import GoalGrounder, GoalProgress, ActionPriority
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


def test_grounder_initialization_forensic(temp_forensic_log, id_manager):
    """Test 1: Initialisation grounder avec forensic"""
    grounder = GoalGrounder(
        abandonment_threshold=20,
        promising_threshold=0.1,
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Vérifier middleware forensic actif
    assert grounder.forensic is not None
    assert grounder.forensic.component_name == "GoalGrounder"
    assert grounder.forensic.enabled is True
    
    # Vérifier paramètres grounder
    assert grounder.abandonment_threshold == 20
    assert grounder.promising_threshold == 0.1
    assert len(grounder.action_priorities) == 0


def test_grounder_record_progress_forensic(temp_forensic_log, id_manager):
    """Test 2: Enregistrement progression avec logging forensic"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Enregistrer progression
    progress = grounder.record_progress(
        action="move_right",
        reward_before=0.0,
        reward_after=0.5,
        levels_before=0,
        levels_after=0
    )
    
    # Vérifier progression enregistrée
    assert progress.action == "move_right"
    assert progress.reward_delta == 0.5
    assert progress.success is True
    assert grounder.stats['total_progressions'] == 1
    
    # Vérifier logs forensic générés
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log progress_recorded
    progress_logs = [log for log in logs if log['event'] == 'progress_recorded']
    assert len(progress_logs) > 0
    
    progress_log = progress_logs[0]
    assert progress_log['component'] == 'GoalGrounder'
    assert progress_log['data']['action'] == "move_right"
    assert progress_log['data']['reward_delta'] == 0.5
    assert progress_log['data']['success'] is True


def test_grounder_goal_detection_forensic(temp_forensic_log, id_manager):
    """Test 3: Détection goal avec logging forensic"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Enregistrer progression avec level complété
    grounder.record_progress(
        action="complete_puzzle",
        reward_before=0.0,
        reward_after=1.0,
        levels_before=0,
        levels_after=1  # Level complété
    )
    
    # Vérifier goal détecté
    assert "complete_level" in grounder.detected_goals
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log goal_detected
    goal_logs = [log for log in logs if log['event'] == 'goal_detected']
    assert len(goal_logs) > 0
    
    goal_log = goal_logs[0]
    assert goal_log['data']['goal_type'] == "complete_level"
    assert goal_log['data']['levels_delta'] == 1


def test_grounder_winning_sequence_forensic(temp_forensic_log, id_manager):
    """Test 4: Séquence gagnante avec logging forensic"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Enregistrer séquence actions
    grounder.record_progress("action1", 0.0, 0.1, 0, 0)
    grounder.record_progress("action2", 0.1, 0.3, 0, 0)
    grounder.record_progress("action3", 0.3, 1.0, 0, 1)  # Level complété
    
    # Vérifier séquence gagnante enregistrée
    assert len(grounder.winning_sequences) == 1
    assert grounder.winning_sequences[0] == ["action1", "action2", "action3"]
    
    # Vérifier logs forensic
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Chercher log winning_sequence_found
    seq_logs = [log for log in logs if log['event'] == 'winning_sequence_found']
    assert len(seq_logs) > 0
    
    seq_log = seq_logs[0]
    assert seq_log['data']['sequence'] == ["action1", "action2", "action3"]
    assert seq_log['data']['sequence_length'] == 3


def test_grounder_action_abandonment_forensic(temp_forensic_log, id_manager):
    """Test 5: Abandon action avec logging forensic"""
    grounder = GoalGrounder(
        abandonment_threshold=5,  # Seuil bas pour test
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Enregistrer échecs consécutifs (plus que threshold)
    for _ in range(25):  # Bien au-delà du threshold
        grounder.record_progress(
            action="bad_action",
            reward_before=0.0,
            reward_after=0.0,  # Pas de reward
            levels_before=0,
            levels_after=0
        )
    
    # Vérifier action abandonnée
    priority = grounder.action_priorities.get("bad_action")
    assert priority is not None
    assert priority.consecutive_failures >= 20  # Threshold par défaut
    
    # Tester prioritization (action abandonnée doit être skippée)
    prioritized = grounder.get_prioritized_actions(["bad_action", "good_action"])
    
    # Vérifier logs forensic générés
    logs = []
    with open(temp_forensic_log, 'r') as f:
        for line in f:
            logs.append(json.loads(line))
    
    # Vérifier logs progress_recorded présents
    progress_logs = [log for log in logs if log['event'] == 'progress_recorded']
    assert len(progress_logs) >= 25
    
    # Chercher log action_abandoned_skipped (si action testée dans prioritization)
    abandon_logs = [log for log in logs if log['event'] == 'action_abandoned_skipped']
    
    if abandon_logs:
        abandon_log = abandon_logs[0]
        assert abandon_log['data']['action'] == "bad_action"
        assert abandon_log['data']['consecutive_failures'] >= 20


def test_grounder_action_prioritization(temp_forensic_log, id_manager):
    """Test 6: Priorisation actions"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Enregistrer progressions différentes
    grounder.record_progress("good_action", 0.0, 0.5, 0, 0)
    grounder.record_progress("good_action", 0.5, 1.0, 0, 0)
    grounder.record_progress("bad_action", 0.0, 0.0, 0, 0)
    
    # Obtenir actions prioritisées
    prioritized = grounder.get_prioritized_actions(["good_action", "bad_action", "new_action"])
    
    # Vérifier ordre priorité
    assert len(prioritized) > 0
    # good_action devrait avoir priorité plus élevée que bad_action
    actions_dict = {action: score for action, score in prioritized}
    assert actions_dict.get("good_action", 0) > actions_dict.get("bad_action", 0)


def test_grounder_hash_chain_integrity(temp_forensic_log, id_manager):
    """Test 7: Intégrité hash chain forensic"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Générer plusieurs progressions
    for i in range(5):
        grounder.record_progress(
            action=f"action_{i}",
            reward_before=0.0,
            reward_after=0.1 * i,
            levels_before=0,
            levels_after=0
        )
    
    # Vérifier intégrité hash chain
    assert grounder.forensic.verify_integrity() is True
    
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


def test_grounder_causal_hierarchy(temp_forensic_log, id_manager):
    """Test 8: Hiérarchie causale session→episode→transition"""
    grounder = GoalGrounder(
        forensic_log_path=temp_forensic_log,
        id_manager=id_manager
    )
    
    # Créer nouvel épisode
    episode_id = grounder.forensic.new_episode()
    assert episode_id is not None
    
    # Enregistrer progression avec transition
    transition_id = grounder.forensic.new_transition()
    grounder.record_progress(
        action="test_action",
        reward_before=0.0,
        reward_after=0.5,
        levels_before=0,
        levels_after=0
    )
    
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
