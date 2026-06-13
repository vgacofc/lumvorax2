"""
MAGEN - LumVorax Forensic Logger
=================================

Système de logging forensique bit-level avec tracking mémoire nanoseconde.
Conforme au protocole LumVorax pour traçabilité complète.

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
Protocole: Claude Pilot + LumVorax
"""

import time
import json
import hashlib
import sys
from typing import Dict, Any, List, Optional
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class LumVoraxEvent:
    """Événement forensique avec timestamp nanoseconde"""
    timestamp_ns: int
    elapsed_ns: int
    event_type: str
    component: str
    operation: str
    data: Dict[str, Any]
    memory_snapshot: Optional[Dict[str, Any]] = None
    hash_chain: Optional[str] = None


class LumVoraxLogger:
    """
    Logger forensique bit-level avec tracking nanoseconde
    
    Fonctionnalités:
    - Timestamp nanoseconde pour chaque événement
    - Hash chain pour intégrité
    - Memory snapshots périodiques
    - Export JSON forensique
    - Analyse post-mortem
    """
    
    def __init__(self, session_id: Optional[str] = None):
        """
        Initialiser logger LumVorax
        
        Args:
            session_id: Identifiant unique session (auto-généré si None)
        """
        self.session_id = session_id or self._generate_session_id()
        self.start_ns = time.time_ns()
        self.events: List[LumVoraxEvent] = []
        self.last_hash = "0" * 64  # Genesis hash
        self.event_count = 0
        
        # Compteurs par type
        self.counters = {
            'pattern_detection': 0,
            'transformation': 0,
            'validation': 0,
            'error': 0,
            'memory_snapshot': 0
        }
        
        # Log événement initial
        self.log_event(
            event_type="session_start",
            component="lumvorax_logger",
            operation="initialize",
            data={
                'session_id': self.session_id,
                'start_timestamp_ns': self.start_ns,
                'protocol': 'LumVorax v1.0',
                'mode': 'forensic_nanosecond'
            }
        )
    
    def _generate_session_id(self) -> str:
        """Générer ID session unique"""
        timestamp = time.time_ns()
        random_data = f"{timestamp}_{id(self)}"
        return hashlib.sha256(random_data.encode()).hexdigest()[:16]
    
    def _compute_hash(self, event: LumVoraxEvent) -> str:
        """
        Calculer hash événement pour chain integrity
        
        Hash = SHA256(last_hash + event_data)
        """
        event_str = json.dumps(asdict(event), sort_keys=True)
        chain_data = f"{self.last_hash}{event_str}"
        return hashlib.sha256(chain_data.encode()).hexdigest()
    
    def _get_memory_snapshot(self) -> Dict[str, Any]:
        """Capturer snapshot mémoire actuel"""
        import psutil
        import os
        
        process = psutil.Process(os.getpid())
        mem_info = process.memory_info()
        
        return {
            'rss_bytes': mem_info.rss,
            'vms_bytes': mem_info.vms,
            'percent': process.memory_percent(),
            'available_bytes': psutil.virtual_memory().available
        }
    
    def log_event(
        self,
        event_type: str,
        component: str,
        operation: str,
        data: Dict[str, Any],
        include_memory: bool = False
    ):
        """
        Logger événement forensique avec timestamp nanoseconde
        
        Args:
            event_type: Type événement (pattern_detection, transformation, etc.)
            component: Composant source (pattern_matcher, pwfe, etc.)
            operation: Opération effectuée
            data: Données événement
            include_memory: Inclure snapshot mémoire
        """
        timestamp_ns = time.time_ns()
        elapsed_ns = timestamp_ns - self.start_ns
        
        # Memory snapshot si demandé
        memory_snapshot = None
        if include_memory:
            try:
                memory_snapshot = self._get_memory_snapshot()
                self.counters['memory_snapshot'] += 1
            except Exception as e:
                memory_snapshot = {'error': str(e)}
        
        # Créer événement
        event = LumVoraxEvent(
            timestamp_ns=timestamp_ns,
            elapsed_ns=elapsed_ns,
            event_type=event_type,
            component=component,
            operation=operation,
            data=data,
            memory_snapshot=memory_snapshot,
            hash_chain=None  # Sera calculé après
        )
        
        # Calculer hash chain
        event.hash_chain = self._compute_hash(event)
        self.last_hash = event.hash_chain
        
        # Stocker événement
        self.events.append(event)
        self.event_count += 1
        
        # Incrémenter compteur
        if event_type in self.counters:
            self.counters[event_type] += 1
    
    def log_pattern_detection(
        self,
        pattern_name: str,
        confidence: float,
        puzzle_id: str,
        train_size: int,
        detection_time_ns: int
    ):
        """Logger détection pattern"""
        self.log_event(
            event_type="pattern_detection",
            component="pattern_matcher",
            operation="detect",
            data={
                'pattern_name': pattern_name,
                'confidence': confidence,
                'puzzle_id': puzzle_id,
                'train_size': train_size,
                'detection_time_ns': detection_time_ns
            }
        )
    
    def log_transformation(
        self,
        pattern_name: str,
        puzzle_id: str,
        input_shape: tuple,
        output_shape: tuple,
        transform_time_ns: int,
        success: bool
    ):
        """Logger transformation appliquée"""
        self.log_event(
            event_type="transformation",
            component="pattern_matcher",
            operation="transform",
            data={
                'pattern_name': pattern_name,
                'puzzle_id': puzzle_id,
                'input_shape': list(input_shape),
                'output_shape': list(output_shape),
                'transform_time_ns': transform_time_ns,
                'success': success
            }
        )
    
    def log_validation(
        self,
        puzzle_id: str,
        predicted_correct: bool,
        validation_time_ns: int,
        details: Optional[Dict[str, Any]] = None
    ):
        """Logger validation résultat"""
        self.log_event(
            event_type="validation",
            component="validator",
            operation="validate",
            data={
                'puzzle_id': puzzle_id,
                'predicted_correct': predicted_correct,
                'validation_time_ns': validation_time_ns,
                'details': details or {}
            }
        )
    
    def log_error(
        self,
        component: str,
        operation: str,
        error_type: str,
        error_message: str,
        puzzle_id: Optional[str] = None
    ):
        """Logger erreur"""
        self.log_event(
            event_type="error",
            component=component,
            operation=operation,
            data={
                'error_type': error_type,
                'error_message': error_message,
                'puzzle_id': puzzle_id
            }
        )
    
    def save_forensic_log(self, filepath: str):
        """
        Sauvegarder log forensique complet
        
        Args:
            filepath: Chemin fichier JSON
        """
        # Créer répertoire si nécessaire
        Path(filepath).parent.mkdir(parents=True, exist_ok=True)
        
        # Préparer données forensiques
        forensic_data = {
            'session_id': self.session_id,
            'start_timestamp_ns': self.start_ns,
            'end_timestamp_ns': time.time_ns(),
            'total_duration_ns': time.time_ns() - self.start_ns,
            'event_count': self.event_count,
            'counters': self.counters,
            'hash_chain_valid': self._verify_hash_chain(),
            'events': [asdict(event) for event in self.events]
        }
        
        # Sauvegarder
        with open(filepath, 'w') as f:
            json.dump(forensic_data, f, indent=2)
        
        print(f"[LumVorax] 💾 Log forensique sauvegardé: {filepath}")
        print(f"[LumVorax] 📊 {self.event_count} événements, hash chain: {'✅ VALIDE' if forensic_data['hash_chain_valid'] else '❌ INVALIDE'}")
    
    def _verify_hash_chain(self) -> bool:
        """Vérifier intégrité hash chain"""
        if len(self.events) == 0:
            return True
        
        last_hash = "0" * 64
        for event in self.events:
            # Recalculer hash
            event_copy = LumVoraxEvent(
                timestamp_ns=event.timestamp_ns,
                elapsed_ns=event.elapsed_ns,
                event_type=event.event_type,
                component=event.component,
                operation=event.operation,
                data=event.data,
                memory_snapshot=event.memory_snapshot,
                hash_chain=None
            )
            
            event_str = json.dumps(asdict(event_copy), sort_keys=True)
            chain_data = f"{last_hash}{event_str}"
            expected_hash = hashlib.sha256(chain_data.encode()).hexdigest()
            
            if expected_hash != event.hash_chain:
                return False
            
            last_hash = event.hash_chain
        
        return True
    
    def get_statistics(self) -> Dict[str, Any]:
        """Obtenir statistiques session"""
        duration_ns = time.time_ns() - self.start_ns
        
        return {
            'session_id': self.session_id,
            'duration_ns': duration_ns,
            'duration_ms': duration_ns / 1_000_000,
            'duration_s': duration_ns / 1_000_000_000,
            'event_count': self.event_count,
            'events_per_second': self.event_count / (duration_ns / 1_000_000_000) if duration_ns > 0 else 0,
            'counters': self.counters,
            'hash_chain_valid': self._verify_hash_chain()
        }
    
    def print_summary(self):
        """Afficher résumé session"""
        stats = self.get_statistics()
        
        print("\n" + "="*60)
        print("LUMVORAX FORENSIC SESSION SUMMARY")
        print("="*60)
        print(f"Session ID: {stats['session_id']}")
        print(f"Duration: {stats['duration_s']:.2f}s ({stats['duration_ms']:.0f}ms)")
        print(f"Total Events: {stats['event_count']}")
        print(f"Events/sec: {stats['events_per_second']:.1f}")
        print(f"\nEvent Counters:")
        for event_type, count in stats['counters'].items():
            print(f"  - {event_type}: {count}")
        print(f"\nHash Chain: {'✅ VALIDE' if stats['hash_chain_valid'] else '❌ INVALIDE'}")
        print("="*60 + "\n")


# Instance globale (singleton pattern)
_global_logger: Optional[LumVoraxLogger] = None


def get_logger(session_id: Optional[str] = None) -> LumVoraxLogger:
    """
    Obtenir instance globale logger LumVorax
    
    Args:
        session_id: ID session (utilisé uniquement à la première création)
    
    Returns:
        Instance LumVoraxLogger
    """
    global _global_logger
    
    if _global_logger is None:
        _global_logger = LumVoraxLogger(session_id=session_id)
    
    return _global_logger


def reset_logger():
    """Réinitialiser logger global (pour tests)"""
    global _global_logger
    _global_logger = None


# Made with Bob
