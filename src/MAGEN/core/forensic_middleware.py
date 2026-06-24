"""
MAGEN Forensic Middleware - Adaptateur Générique Réutilisable
Conforme PROTOCOLE_MAGEN V3.0 - FORENSIC TOTAL

Principe: Centraliser le wrapping forensic pour éviter duplication structurelle
"""

import functools
import time
from typing import Any, Callable, Dict, Optional, TypeVar, cast
from pathlib import Path

from .forensic_logger import CausalIDManager, ForensicLogger, SilentErrorDetector


T = TypeVar('T')


class ForensicMiddleware:
    """
    Middleware forensic générique pour instrumentation modules MAGEN
    
    Fonctionnalités:
    - Wrapping automatique méthodes (AVANT/APRÈS)
    - Gestion erreurs avec logging
    - Détection anomalies temps réel
    - Interface unique standardisée
    - Zéro duplication code
    
    Usage:
        middleware = ForensicMiddleware("logs/module.jsonl", "ModuleName")
        
        @middleware.instrument()
        def my_method(self, arg1, arg2):
            return result
    """
    
    def __init__(
        self,
        log_path: str,
        component_name: str,
        id_manager: Optional[CausalIDManager] = None,
        enable_anomaly_detection: bool = True
    ):
        """
        Initialiser middleware forensic
        
        Args:
            log_path: Chemin fichier log JSONL
            component_name: Nom composant (e.g., "StateEncoderV25")
            id_manager: Manager IDs causaux (créé si None)
            enable_anomaly_detection: Activer détection anomalies
        """
        self.component_name = component_name
        self.id_manager = id_manager or CausalIDManager()
        self.forensic_logger = ForensicLogger(log_path, self.id_manager)
        self.anomaly_detector = SilentErrorDetector() if enable_anomaly_detection else None
        self.enabled = True
    
    def instrument(
        self,
        operation_name: Optional[str] = None,
        capture_args: bool = True,
        capture_result: bool = True,
        capture_state: bool = False,
        state_getter: Optional[Callable[[Any], Dict[str, Any]]] = None
    ) -> Callable[[Callable[..., T]], Callable[..., T]]:
        """
        Décorateur pour instrumenter méthode avec forensic AVANT/APRÈS
        
        Args:
            operation_name: Nom opération (défaut: nom fonction)
            capture_args: Capturer arguments
            capture_result: Capturer résultat
            capture_state: Capturer état objet
            state_getter: Fonction pour extraire état (si capture_state=True)
        
        Returns:
            Décorateur fonction
        
        Example:
            @middleware.instrument(capture_state=True, state_getter=lambda self: {'counter': self.counter})
            def process(self, data):
                return result
        """
        def decorator(func: Callable[..., T]) -> Callable[..., T]:
            @functools.wraps(func)
            def wrapper(*args: Any, **kwargs: Any) -> T:
                if not self.enabled:
                    return func(*args, **kwargs)
                
                # Nom opération
                op_name = operation_name or func.__name__
                
                # Créer operation ID
                op_id = self.id_manager.new_operation()
                start_ns = time.perf_counter_ns()
                
                # Préparer données AVANT
                data_before: Dict[str, Any] = {}
                
                if capture_args:
                    # Capturer args (skip self si méthode)
                    if args and hasattr(args[0].__class__, func.__name__):
                        data_before['args'] = args[1:]  # Skip self
                    else:
                        data_before['args'] = args
                    data_before['kwargs'] = kwargs
                
                if capture_state and state_getter and args:
                    try:
                        data_before['state_before'] = state_getter(args[0])
                    except Exception as e:
                        data_before['state_before_error'] = str(e)
                
                # Log AVANT
                self.forensic_logger.log(
                    f'{op_name}_start',
                    self.component_name,
                    data_before
                )
                
                # Exécution
                error_occurred = False
                result = None
                error = None
                
                try:
                    result = func(*args, **kwargs)
                except Exception as e:
                    error_occurred = True
                    error = e
                    
                    # Log erreur
                    end_ns = time.perf_counter_ns()
                    self.forensic_logger.log(
                        f'{op_name}_error',
                        self.component_name,
                        {
                            'error_type': type(e).__name__,
                            'error_message': str(e),
                            'duration_ns': end_ns - start_ns
                        }
                    )
                    raise
                
                finally:
                    if not error_occurred:
                        # Préparer données APRÈS
                        end_ns = time.perf_counter_ns()
                        data_after: Dict[str, Any] = {
                            'duration_ns': end_ns - start_ns
                        }
                        
                        if capture_result:
                            data_after['result'] = result
                        
                        if capture_state and state_getter and args:
                            try:
                                data_after['state_after'] = state_getter(args[0])
                            except Exception as e:
                                data_after['state_after_error'] = str(e)
                        
                        # Log APRÈS
                        self.forensic_logger.log(
                            f'{op_name}_complete',
                            self.component_name,
                            data_after
                        )
                        
                        # Détection anomalies
                        if self.anomaly_detector:
                            log_entry = {
                                'event': f'{op_name}_complete',
                                'component': self.component_name,
                                'data': data_after
                            }
                            anomalies = self.anomaly_detector.check_anomalies(log_entry)
                            
                            if anomalies:
                                self.forensic_logger.log(
                                    f'{op_name}_anomalies_detected',
                                    self.component_name,
                                    {
                                        'anomalies': anomalies,
                                        'operation': op_name
                                    }
                                )
                
                return cast(T, result)
            
            return wrapper
        return decorator
    
    def log_event(self, event_name: str, data: Dict[str, Any]):
        """
        Logger événement custom
        
        Args:
            event_name: Nom événement
            data: Données à logger
        """
        if self.enabled:
            self.forensic_logger.log(event_name, self.component_name, data)
    
    def new_episode(self) -> str:
        """Créer nouvel épisode"""
        return self.id_manager.new_episode()
    
    def new_transition(self) -> str:
        """Créer nouvelle transition"""
        return self.id_manager.new_transition()
    
    def verify_integrity(self) -> bool:
        """Vérifier intégrité hash chain"""
        return self.forensic_logger.verify_chain()
    
    def enable(self):
        """Activer forensic"""
        self.enabled = True
    
    def disable(self):
        """Désactiver forensic (pour benchmarks)"""
        self.enabled = False
    
    def close(self):
        """Fermer logger"""
        self.forensic_logger.close()


class ForensicMixin:
    """
    Mixin pour ajouter capacités forensic à une classe
    
    Usage:
        class MyModule(ForensicMixin):
            def __init__(self):
                self.setup_forensic("logs/mymodule.jsonl", "MyModule")
            
            @property
            def _forensic_state(self):
                return {'counter': self.counter}
            
            def my_method(self, arg):
                return self._forensic_call('my_method', lambda: self._my_method_impl(arg))
    """
    
    def setup_forensic(
        self,
        log_path: str,
        component_name: str,
        id_manager: Optional[CausalIDManager] = None
    ):
        """
        Initialiser forensic pour cette instance
        
        Args:
            log_path: Chemin fichier log
            component_name: Nom composant
            id_manager: Manager IDs causaux (partagé si fourni)
        """
        self._forensic_middleware = ForensicMiddleware(
            log_path,
            component_name,
            id_manager
        )
    
    @property
    def _forensic_state(self) -> Dict[str, Any]:
        """
        Override pour fournir état à capturer
        
        Returns:
            Dict état actuel
        """
        return {}
    
    def _forensic_call(
        self,
        operation_name: str,
        func: Callable[[], T],
        capture_state: bool = True
    ) -> T:
        """
        Wrapper forensic pour appel fonction
        
        Args:
            operation_name: Nom opération
            func: Fonction à exécuter
            capture_state: Capturer état avant/après
        
        Returns:
            Résultat fonction
        """
        if not hasattr(self, '_forensic_middleware'):
            return func()
        
        middleware = self._forensic_middleware
        
        if not middleware.enabled:
            return func()
        
        # Créer operation ID
        op_id = middleware.id_manager.new_operation()
        start_ns = time.perf_counter_ns()
        
        # Log AVANT
        data_before = {}
        if capture_state:
            data_before['state_before'] = self._forensic_state
        
        middleware.forensic_logger.log(
            f'{operation_name}_start',
            middleware.component_name,
            data_before
        )
        
        # Exécution
        try:
            result = func()
        except Exception as e:
            # Log erreur
            end_ns = time.perf_counter_ns()
            middleware.forensic_logger.log(
                f'{operation_name}_error',
                middleware.component_name,
                {
                    'error_type': type(e).__name__,
                    'error_message': str(e),
                    'duration_ns': end_ns - start_ns
                }
            )
            raise
        
        # Log APRÈS
        end_ns = time.perf_counter_ns()
        data_after = {
            'result': result,
            'duration_ns': end_ns - start_ns
        }
        if capture_state:
            data_after['state_after'] = self._forensic_state
        
        middleware.forensic_logger.log(
            f'{operation_name}_complete',
            middleware.component_name,
            data_after
        )
        
        return result
    
    def _forensic_log_event(self, event_name: str, data: Dict[str, Any]):
        """Logger événement custom"""
        if hasattr(self, '_forensic_middleware'):
            self._forensic_middleware.log_event(event_name, data)
    
    def _forensic_new_episode(self) -> str:
        """Créer nouvel épisode"""
        if hasattr(self, '_forensic_middleware'):
            return self._forensic_middleware.new_episode()
        return ""
    
    def _forensic_new_transition(self) -> str:
        """Créer nouvelle transition"""
        if hasattr(self, '_forensic_middleware'):
            return self._forensic_middleware.new_transition()
        return ""


def create_forensic_wrapper(
    module_class: type,
    log_path: str,
    component_name: str,
    methods_to_instrument: list[str],
    id_manager: Optional[CausalIDManager] = None
) -> type:
    """
    Factory pour créer wrapper forensic d'une classe existante
    
    Args:
        module_class: Classe à wrapper
        log_path: Chemin fichier log
        component_name: Nom composant
        methods_to_instrument: Liste noms méthodes à instrumenter
        id_manager: Manager IDs causaux (partagé si fourni)
    
    Returns:
        Classe wrappée avec forensic
    
    Example:
        StateEncoderForensic = create_forensic_wrapper(
            StateEncoderV25,
            "logs/encoder.jsonl",
            "StateEncoderV25",
            ["encode", "decode"]
        )
        encoder = StateEncoderForensic()
    """
    middleware = ForensicMiddleware(log_path, component_name, id_manager)
    
    class ForensicWrapper(module_class):  # type: ignore
        def __init__(self, *args: Any, **kwargs: Any):
            super().__init__(*args, **kwargs)
            self._forensic_middleware = middleware
    
    # Instrumenter méthodes spécifiées
    for method_name in methods_to_instrument:
        if hasattr(module_class, method_name):
            original_method = getattr(module_class, method_name)
            
            @middleware.instrument(operation_name=method_name)
            @functools.wraps(original_method)
            def wrapped_method(self: Any, *args: Any, **kwargs: Any) -> Any:
                return original_method(self, *args, **kwargs)
            
            setattr(ForensicWrapper, method_name, wrapped_method)
    
    ForensicWrapper.__name__ = f"{module_class.__name__}Forensic"
    ForensicWrapper.__qualname__ = f"{module_class.__qualname__}Forensic"
    
    return ForensicWrapper

# Made with Bob
