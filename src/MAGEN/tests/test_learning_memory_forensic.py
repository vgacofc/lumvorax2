#!/usr/bin/env python3
"""
Tests unitaires forensic pour Learning Memory Manager
Vérifie intégration PROTOCOLE_MAGEN V3.0
"""

import pytest
import json
import tempfile
import numpy as np
from pathlib import Path
from datetime import datetime

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.learning_memory import LearningMemory, PuzzleExperience
from core.forensic_logger import CausalIDManager


@pytest.fixture
def temp_dirs():
    """Crée répertoires temporaires pour tests"""
    with tempfile.TemporaryDirectory() as memory_dir, \
         tempfile.TemporaryDirectory() as forensic_dir:
        yield Path(memory_dir), Path(forensic_dir)


@pytest.fixture
def id_manager():
    """Crée ID manager pour tests"""
    return CausalIDManager()


def test_memory_initialization_forensic(temp_dirs, id_manager):
    """Test: Initialisation avec forensic génère logs corrects"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Vérifier forensic activé
    assert memory.forensic is not None
    assert forensic_path.exists()
    
    # Vérifier log initialisation
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    assert len(logs) >= 1
    init_log = logs[0]
    assert init_log['event'] == 'memory_initialized'
    assert 'memory_dir' in init_log['data']
    assert 'archive_exists' in init_log['data']


def test_store_experience_forensic(temp_dirs, id_manager):
    """Test: Stockage expérience génère logs forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Créer expérience test
    experience = PuzzleExperience(
        puzzle_id="test_001",
        timestamp=datetime.now().isoformat(),
        success=True,
        classification="rotation",
        confidence=0.95,
        programs_tried=5,
        best_score=1.0,
        solution_program="rotate_90",
        failure_reason=None,
        train_pairs_hash="abc123"
    )
    
    memory.store_experience(experience)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher logs store
    store_logs = [log for log in logs if 'store' in log['event']]
    assert len(store_logs) >= 2  # start + stored
    
    # Vérifier log experience_store_start
    start_log = next(log for log in store_logs if log['event'] == 'experience_store_start')
    assert start_log['data']['puzzle_id'] == "test_001"
    assert start_log['data']['success'] is True
    assert start_log['data']['classification'] == "rotation"
    
    # Vérifier log experience_stored
    stored_log = next(log for log in store_logs if log['event'] == 'experience_stored')
    assert stored_log['data']['puzzle_id'] == "test_001"
    assert stored_log['data']['short_term_size'] == 1


def test_failure_pattern_update_forensic(temp_dirs, id_manager):
    """Test: Mise à jour pattern échec génère logs forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Créer expérience échec
    experience = PuzzleExperience(
        puzzle_id="test_002",
        timestamp=datetime.now().isoformat(),
        success=False,
        classification="color_detection",
        confidence=0.3,
        programs_tried=10,
        best_score=0.2,
        solution_program=None,
        failure_reason="color_mismatch",
        train_pairs_hash="def456"
    )
    
    memory.store_experience(experience)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log failure_pattern_updated
    pattern_logs = [log for log in logs if log['event'] == 'failure_pattern_updated']
    assert len(pattern_logs) >= 1
    
    pattern_log = pattern_logs[0]
    assert pattern_log['data']['pattern_type'] == "color_mismatch"
    assert pattern_log['data']['is_new_pattern'] is True
    assert pattern_log['data']['frequency'] == 1
    assert pattern_log['data']['puzzle_id'] == "test_002"


def test_success_pattern_update_forensic(temp_dirs, id_manager):
    """Test: Mise à jour pattern succès génère logs forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Créer expérience succès
    experience = PuzzleExperience(
        puzzle_id="test_003",
        timestamp=datetime.now().isoformat(),
        success=True,
        classification="symmetry",
        confidence=0.98,
        programs_tried=3,
        best_score=1.0,
        solution_program="mirror_horizontal",
        failure_reason=None,
        train_pairs_hash="ghi789"
    )
    
    memory.store_experience(experience)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log success_pattern_updated
    pattern_logs = [log for log in logs if log['event'] == 'success_pattern_updated']
    assert len(pattern_logs) >= 1
    
    pattern_log = pattern_logs[0]
    assert pattern_log['data']['classification'] == "symmetry"
    assert pattern_log['data']['is_new_classification'] is True
    assert pattern_log['data']['is_new_puzzle'] is True
    assert pattern_log['data']['puzzle_id'] == "test_003"


def test_memory_consolidation_forensic(temp_dirs, id_manager):
    """Test: Consolidation mémoire génère logs forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Ajouter >50 expériences pour déclencher consolidation
    for i in range(55):
        experience = PuzzleExperience(
            puzzle_id=f"test_{i:03d}",
            timestamp=datetime.now().isoformat(),
            success=(i % 2 == 0),
            classification="test_class",
            confidence=0.8,
            programs_tried=5,
            best_score=0.9,
            solution_program="test_prog" if i % 2 == 0 else None,
            failure_reason="test_fail" if i % 2 == 1 else None,
            train_pairs_hash=f"hash_{i}"
        )
        memory.store_experience(experience)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log memory_consolidated
    consolidation_logs = [log for log in logs if log['event'] == 'memory_consolidated']
    assert len(consolidation_logs) >= 1
    
    consol_log = consolidation_logs[0]
    assert 'short_to_mid' in consol_log['data']
    assert 'short_term_size' in consol_log['data']
    assert 'mid_term_size' in consol_log['data']


def test_correction_attempt_forensic(temp_dirs, id_manager):
    """Test: Tentative correction génère logs forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Créer pattern échec d'abord
    experience = PuzzleExperience(
        puzzle_id="test_004",
        timestamp=datetime.now().isoformat(),
        success=False,
        classification="test",
        confidence=0.5,
        programs_tried=5,
        best_score=0.3,
        solution_program=None,
        failure_reason="region_extraction",
        train_pairs_hash="jkl012"
    )
    memory.store_experience(experience)
    
    # Enregistrer tentative correction
    memory.record_correction_attempt("region_extraction", success=True)
    
    # Vérifier logs forensic
    with open(forensic_path) as f:
        logs = [json.loads(line) for line in f]
    
    # Chercher log correction_attempted
    correction_logs = [log for log in logs if log['event'] == 'correction_attempted']
    assert len(correction_logs) >= 1
    
    correction_log = correction_logs[0]
    assert correction_log['data']['pattern_type'] == "region_extraction"
    assert correction_log['data']['success'] is True
    assert 'pattern_frequency' in correction_log['data']
    assert 'causal_sequences_count' in correction_log['data']


def test_forensic_hash_chain_integrity(temp_dirs, id_manager):
    """Test: Hash chain maintient intégrité forensic"""
    memory_dir, forensic_dir = temp_dirs
    forensic_path = forensic_dir / "learning_memory_test.jsonl"
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=str(forensic_path),
        id_manager=id_manager
    )
    
    # Générer plusieurs événements
    for i in range(5):
        experience = PuzzleExperience(
            puzzle_id=f"test_{i}",
            timestamp=datetime.now().isoformat(),
            success=True,
            classification="test",
            confidence=0.9,
            programs_tried=3,
            best_score=1.0,
            solution_program="test",
            failure_reason=None,
            train_pairs_hash=f"hash_{i}"
        )
        memory.store_experience(experience)
    
    # Vérifier intégrité hash chain
    assert memory.forensic is not None
    is_valid = memory.forensic.verify_integrity()
    assert is_valid, "Hash chain invalide"


def test_memory_without_forensic(temp_dirs):
    """Test: Memory fonctionne sans forensic (backward compatibility)"""
    memory_dir, _ = temp_dirs
    
    memory = LearningMemory(
        memory_dir=memory_dir,
        forensic_log_path=None
    )
    
    # Vérifier forensic désactivé
    assert memory.forensic is None
    
    # Vérifier fonctionnement normal
    experience = PuzzleExperience(
        puzzle_id="test_005",
        timestamp=datetime.now().isoformat(),
        success=True,
        classification="test",
        confidence=0.9,
        programs_tried=3,
        best_score=1.0,
        solution_program="test",
        failure_reason=None,
        train_pairs_hash="hash_test"
    )
    
    # Ne doit pas crasher
    memory.store_experience(experience)
    assert len(memory.short_term) == 1


if __name__ == "__main__":
    pytest.main([__file__, "-v"])

# Made with Bob
