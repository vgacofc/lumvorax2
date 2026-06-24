"""
Tests unitaires pour forensic_middleware.py
Validation middleware générique réutilisable
"""

import pytest
import tempfile
import json
from pathlib import Path
import sys

# Ajouter le chemin parent pour imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.forensic_middleware import (
    ForensicMiddleware,
    ForensicMixin,
    create_forensic_wrapper
)
from core.forensic_logger import CausalIDManager


class TestForensicMiddleware:
    """Tests ForensicMiddleware"""
    
    def test_init(self):
        """Test initialisation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            assert middleware.component_name == "TestComponent"
            assert middleware.enabled == True
            assert middleware.id_manager is not None
            assert middleware.forensic_logger is not None
            
            middleware.close()
    
    def test_instrument_decorator(self):
        """Test décorateur instrument"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            @middleware.instrument()
            def test_function(x, y):
                return x + y
            
            result = test_function(2, 3)
            assert result == 5
            
            middleware.close()
            
            # Vérifier logs générés
            with open(log_path) as f:
                lines = f.readlines()
                assert len(lines) == 2  # start + complete
                
                log_start = json.loads(lines[0])
                assert log_start['event'] == 'test_function_start'
                assert log_start['component'] == 'TestComponent'
                
                log_complete = json.loads(lines[1])
                assert log_complete['event'] == 'test_function_complete'
                assert 'duration_ns' in log_complete['data']
    
    def test_instrument_with_state(self):
        """Test instrument avec capture état"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            class TestClass:
                def __init__(self):
                    self.counter = 0
                
                @middleware.instrument(
                    capture_state=True,
                    state_getter=lambda self: {'counter': self.counter}
                )
                def increment(self):
                    self.counter += 1
                    return self.counter
            
            obj = TestClass()
            result = obj.increment()
            assert result == 1
            
            middleware.close()
            
            # Vérifier état capturé
            with open(log_path) as f:
                lines = f.readlines()
                
                log_start = json.loads(lines[0])
                assert log_start['data']['state_before'] == {'counter': 0}
                
                log_complete = json.loads(lines[1])
                assert log_complete['data']['state_after'] == {'counter': 1}
    
    def test_instrument_error_handling(self):
        """Test gestion erreurs"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            @middleware.instrument()
            def failing_function():
                raise ValueError("Test error")
            
            with pytest.raises(ValueError):
                failing_function()
            
            middleware.close()
            
            # Vérifier log erreur
            with open(log_path) as f:
                lines = f.readlines()
                assert len(lines) == 2  # start + error
                
                log_error = json.loads(lines[1])
                assert log_error['event'] == 'failing_function_error'
                assert log_error['data']['error_type'] == 'ValueError'
                assert log_error['data']['error_message'] == 'Test error'
    
    def test_enable_disable(self):
        """Test activation/désactivation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            @middleware.instrument()
            def test_function():
                return 42
            
            # Désactiver
            middleware.disable()
            result = test_function()
            assert result == 42
            
            middleware.close()
            
            # Vérifier aucun log généré
            with open(log_path) as f:
                lines = f.readlines()
                assert len(lines) == 0
    
    def test_log_event(self):
        """Test log événement custom"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            middleware = ForensicMiddleware(str(log_path), "TestComponent")
            
            middleware.log_event('custom_event', {'value': 123})
            
            middleware.close()
            
            # Vérifier log
            with open(log_path) as f:
                log = json.loads(f.readline())
                assert log['event'] == 'custom_event'
                assert log['data']['value'] == 123


class TestForensicMixin:
    """Tests ForensicMixin"""
    
    def test_mixin_basic(self):
        """Test mixin basique"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            
            class TestModule(ForensicMixin):
                def __init__(self):
                    self.counter = 0
                    self.setup_forensic(str(log_path), "TestModule")
                
                @property
                def _forensic_state(self):
                    return {'counter': self.counter}
                
                def increment(self):
                    return self._forensic_call(
                        'increment',
                        lambda: self._increment_impl()
                    )
                
                def _increment_impl(self):
                    self.counter += 1
                    return self.counter
            
            module = TestModule()
            result = module.increment()
            assert result == 1
            
            module._forensic_middleware.close()
            
            # Vérifier logs
            with open(log_path) as f:
                lines = f.readlines()
                assert len(lines) == 2  # start + complete
                
                log_complete = json.loads(lines[1])
                assert log_complete['event'] == 'increment_complete'
                assert log_complete['data']['result'] == 1
    
    def test_mixin_episode_transition(self):
        """Test gestion épisodes/transitions"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            
            class TestModule(ForensicMixin):
                def __init__(self):
                    self.setup_forensic(str(log_path), "TestModule")
            
            module = TestModule()
            
            episode_id = module._forensic_new_episode()
            assert episode_id is not None
            assert len(episode_id) == 36  # UUID format
            
            transition_id = module._forensic_new_transition()
            assert transition_id is not None
            assert len(transition_id) == 36
            
            module._forensic_middleware.close()


class TestCreateForensicWrapper:
    """Tests create_forensic_wrapper factory"""
    
    def test_wrapper_creation(self):
        """Test création wrapper"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.jsonl"
            
            class OriginalClass:
                def __init__(self):
                    self.value = 0
                
                def increment(self):
                    self.value += 1
                    return self.value
                
                def get_value(self):
                    return self.value
            
            # Créer wrapper
            WrappedClass = create_forensic_wrapper(
                OriginalClass,
                str(log_path),
                "OriginalClass",
                ["increment"]  # Instrumenter seulement increment
            )
            
            # Utiliser wrapper
            obj = WrappedClass()
            result = obj.increment()
            assert result == 1
            
            # get_value non instrumenté
            value = obj.get_value()
            assert value == 1
            
            obj._forensic_middleware.close()
            
            # Vérifier logs (seulement increment)
            with open(log_path) as f:
                lines = f.readlines()
                assert len(lines) == 2  # start + complete pour increment
                
                log_start = json.loads(lines[0])
                assert log_start['event'] == 'increment_start'
    
    def test_wrapper_shared_id_manager(self):
        """Test wrapper avec ID manager partagé"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path1 = Path(tmpdir) / "test1.jsonl"
            log_path2 = Path(tmpdir) / "test2.jsonl"
            
            # ID manager partagé
            shared_id_manager = CausalIDManager()
            
            class ClassA:
                def method_a(self):
                    return "A"
            
            class ClassB:
                def method_b(self):
                    return "B"
            
            # Créer wrappers avec ID manager partagé
            WrappedA = create_forensic_wrapper(
                ClassA,
                str(log_path1),
                "ClassA",
                ["method_a"],
                shared_id_manager
            )
            
            WrappedB = create_forensic_wrapper(
                ClassB,
                str(log_path2),
                "ClassB",
                ["method_b"],
                shared_id_manager
            )
            
            # Utiliser
            obj_a = WrappedA()
            obj_b = WrappedB()
            
            obj_a.method_a()
            obj_b.method_b()
            
            obj_a._forensic_middleware.close()
            obj_b._forensic_middleware.close()
            
            # Vérifier session_id identique
            with open(log_path1) as f:
                log_a = json.loads(f.readline())
            
            with open(log_path2) as f:
                log_b = json.loads(f.readline())
            
            assert log_a['causal_context']['session_id'] == log_b['causal_context']['session_id']


if __name__ == '__main__':
    pytest.main([__file__, '-v'])

# Made with Bob
