"""
Tests unitaires pour forensic_logger.py
Conforme PROTOCOLE_MAGEN V3.0 - FORENSIC TOTAL
"""

import pytest
import tempfile
import json
import numpy as np
from pathlib import Path
import sys
import os

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.forensic_logger import (
    CausalIDManager,
    ForensicLogger,
    SilentErrorDetector,
    ForensicReplay,
    compute_sha256,
    compute_sha256_array
)


class TestCausalIDManager:
    """Tests CausalIDManager"""
    
    def test_init(self):
        """Test initialisation"""
        manager = CausalIDManager()
        
        assert manager.session_id is not None
        assert len(manager.session_id) == 36  # UUID v4 format
        assert manager.episode_id is None
        assert manager.episode_counter == 0
    
    def test_new_episode(self):
        """Test création épisode"""
        manager = CausalIDManager()
        
        episode_id = manager.new_episode()
        assert episode_id is not None
        assert manager.episode_counter == 1
        assert manager.transition_counter == 0
    
    def test_new_transition(self):
        """Test création transition"""
        manager = CausalIDManager()
        manager.new_episode()
        
        transition_id = manager.new_transition()
        assert transition_id is not None
        assert manager.transition_counter == 1
        assert manager.operation_counter == 0
    
    def test_new_operation(self):
        """Test création opération"""
        manager = CausalIDManager()
        manager.new_episode()
        manager.new_transition()
        
        operation_id = manager.new_operation()
        assert operation_id is not None
        assert manager.operation_counter == 1
    
    def test_get_full_context(self):
        """Test contexte complet"""
        manager = CausalIDManager()
        manager.new_episode()
        manager.new_transition()
        manager.new_operation()
        
        context = manager.get_full_context()
        
        assert 'session_id' in context
        assert 'episode_id' in context
        assert 'episode_number' in context
        assert 'transition_id' in context
        assert 'transition_number' in context
        assert 'operation_id' in context
        assert 'operation_number' in context
        
        assert context['episode_number'] == 1
        assert context['transition_number'] == 1
        assert context['operation_number'] == 1


class TestForensicLogger:
    """Tests ForensicLogger"""
    
    def test_init(self):
        """Test initialisation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            logger = ForensicLogger(str(log_path), manager)
            
            assert logger.log_path == log_path
            assert logger.log_counter == 0
            assert logger.previous_hash == "0" * 64
            
            logger.close()
    
    def test_log_simple(self):
        """Test log simple"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            manager.new_episode()
            manager.new_transition()
            manager.new_operation()
            
            logger = ForensicLogger(str(log_path), manager)
            
            logger.log('test_event', 'TestComponent', {
                'value': 42,
                'text': 'hello'
            })
            
            logger.close()
            
            # Vérifier fichier créé
            assert log_path.exists()
            
            # Lire et vérifier contenu
            with open(log_path) as f:
                line = f.readline()
                entry = json.loads(line)
                
                assert entry['log_number'] == 1
                assert entry['previous_hash'] == "0" * 64
                assert entry['event'] == 'test_event'
                assert entry['component'] == 'TestComponent'
                assert entry['data']['value'] == 42
                assert entry['data']['text'] == 'hello'
                assert 'entry_hash' in entry
                assert len(entry['entry_hash']) == 64
    
    def test_log_ndarray(self):
        """Test log avec ndarray"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            manager.new_operation()
            
            logger = ForensicLogger(str(log_path), manager)
            
            array = np.array([[1, 2], [3, 4]])
            logger.log('test_array', 'TestComponent', {
                'array': array
            })
            
            logger.close()
            
            # Vérifier sérialisation complète
            with open(log_path) as f:
                entry = json.loads(f.readline())
                
                array_data = entry['data']['array']
                assert array_data['type'] == 'ndarray'
                assert array_data['shape'] == [2, 2]
                assert 'hash_sha256' in array_data
                assert 'data_full' in array_data
                assert array_data['data_full'] == [[1, 2], [3, 4]]
                assert 'min' in array_data
                assert 'max' in array_data
                assert 'mean' in array_data
                assert 'std' in array_data
    
    def test_hash_chain(self):
        """Test hash chain"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            manager.new_operation()
            
            logger = ForensicLogger(str(log_path), manager)
            
            # Logger 3 événements
            logger.log('event1', 'Test', {'value': 1})
            logger.log('event2', 'Test', {'value': 2})
            logger.log('event3', 'Test', {'value': 3})
            
            logger.close()
            
            # Vérifier chaînage
            with open(log_path) as f:
                entries = [json.loads(line) for line in f]
            
            assert len(entries) == 3
            
            # Entry 1: previous_hash = genesis
            assert entries[0]['previous_hash'] == "0" * 64
            
            # Entry 2: previous_hash = hash entry 1
            assert entries[1]['previous_hash'] == entries[0]['entry_hash']
            
            # Entry 3: previous_hash = hash entry 2
            assert entries[2]['previous_hash'] == entries[1]['entry_hash']
    
    def test_verify_chain_valid(self):
        """Test vérification chaîne valide"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            manager.new_operation()
            
            logger = ForensicLogger(str(log_path), manager)
            
            logger.log('event1', 'Test', {'value': 1})
            logger.log('event2', 'Test', {'value': 2})
            
            # Vérifier chaîne valide
            assert logger.verify_chain() == True
            
            logger.close()
    
    def test_verify_chain_corrupted(self):
        """Test détection chaîne corrompue"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            manager = CausalIDManager()
            manager.new_operation()
            
            logger = ForensicLogger(str(log_path), manager)
            
            logger.log('event1', 'Test', {'value': 1})
            logger.log('event2', 'Test', {'value': 2})
            
            logger.close()
            
            # Corrompre fichier
            with open(log_path, 'r') as f:
                lines = f.readlines()
            
            # Modifier entry 2
            entry2 = json.loads(lines[1])
            entry2['data']['value'] = 999  # Corruption
            lines[1] = json.dumps(entry2) + '\n'
            
            with open(log_path, 'w') as f:
                f.writelines(lines)
            
            # Vérifier détection corruption
            logger2 = ForensicLogger(str(log_path), manager)
            assert logger2.verify_chain() == False
            logger2.close()


class TestSilentErrorDetector:
    """Tests SilentErrorDetector"""
    
    def test_hash_constant(self):
        """Test détection hash constant"""
        detector = SilentErrorDetector()
        
        # Simuler 15 logs avec même hash
        anomalies = []
        for i in range(15):
            log_entry = {
                'data': {
                    'topological_hash': 'constant_hash'
                }
            }
            anomalies = detector.check_anomalies(log_entry)
        
        # Après 10+ logs, devrait détecter HASH_CONSTANT
        assert 'HASH_CONSTANT' in anomalies
    
    def test_action_loop(self):
        """Test détection action loop"""
        detector = SilentErrorDetector()
        
        # Simuler 15 logs avec 2 actions alternées
        anomalies = []
        for i in range(15):
            log_entry = {
                'data': {
                    'action': 'action_A' if i % 2 == 0 else 'action_B'
                }
            }
            anomalies = detector.check_anomalies(log_entry)
        
        # Après 10+ logs, devrait détecter ACTION_LOOP
        assert 'ACTION_LOOP' in anomalies
    
    def test_nan_propagation(self):
        """Test détection NaN"""
        detector = SilentErrorDetector()
        
        log_entry = {
            'data': {
                'embedding': {
                    'data_full': [1.0, 2.0, float('nan'), 4.0]
                }
            }
        }
        
        anomalies = detector.check_anomalies(log_entry)
        assert 'NAN_PROPAGATION' in anomalies


class TestHashFunctions:
    """Tests fonctions hash"""
    
    def test_compute_sha256(self):
        """Test hash SHA256 bytes"""
        data = b"hello world"
        hash_result = compute_sha256(data)
        
        assert len(hash_result) == 64
        assert all(c in '0123456789abcdef' for c in hash_result)
        
        # Vérifier stabilité
        assert compute_sha256(data) == hash_result
    
    def test_compute_sha256_array(self):
        """Test hash SHA256 ndarray"""
        array = np.array([[1, 2], [3, 4]])
        hash_result = compute_sha256_array(array)
        
        assert len(hash_result) == 64
        assert all(c in '0123456789abcdef' for c in hash_result)
        
        # Vérifier stabilité
        assert compute_sha256_array(array) == hash_result
        
        # Vérifier différence si array différent
        array2 = np.array([[1, 2], [3, 5]])
        assert compute_sha256_array(array2) != hash_result


if __name__ == '__main__':
    pytest.main([__file__, '-v'])

# Made with Bob
