"""
MAGEN - Simple Logger pour tests Arcade
Version simplifiée sans tous les paramètres forensiques
"""

import time
import json
from pathlib import Path
from typing import Dict, Any, List


class SimpleLogger:
    """Logger simplifié pour tests Arcade"""
    
    def __init__(self, log_dir: str):
        """
        Initialisation logger simple
        
        Args:
            log_dir: Répertoire logs
        """
        self.log_dir = Path(log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)
        
        # Buffer événements
        self.events: List[Dict[str, Any]] = []
        
        # Timestamps
        self.session_start = time.time()
        
        print(f"[SimpleLogger] Logger initialisé: {self.log_dir}")
    
    def log_event(self, event_type: str, data: Dict[str, Any]) -> None:
        """
        Logger un événement simple
        
        Args:
            event_type: Type événement
            data: Données événement
        """
        event = {
            "timestamp": time.time(),
            "timestamp_ns": time.time_ns(),
            "event_type": event_type,
            "data": data
        }
        
        self.events.append(event)
    
    def save_logs(self) -> Path:
        """
        Sauvegarder logs en JSON
        
        Returns:
            Chemin fichier sauvegardé
        """
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        log_file = self.log_dir / f"simple_log_{timestamp}.json"
        
        with open(log_file, 'w') as f:
            json.dump({
                "session_start": self.session_start,
                "total_events": len(self.events),
                "events": self.events
            }, f, indent=2)
        
        print(f"[SimpleLogger] Logs sauvegardés: {log_file}")
        return log_file
    
    def get_stats(self) -> Dict[str, Any]:
        """Obtenir statistiques"""
        return {
            "total_events": len(self.events),
            "session_duration": time.time() - self.session_start,
            "events_by_type": self._count_by_type()
        }
    
    def _count_by_type(self) -> Dict[str, int]:
        """Compter événements par type"""
        counts = {}
        for event in self.events:
            event_type = event["event_type"]
            counts[event_type] = counts.get(event_type, 0) + 1
        return counts

# Made with Bob
