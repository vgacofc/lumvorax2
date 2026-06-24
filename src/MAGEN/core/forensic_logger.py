"""
MAGEN Forensic Logger - Standards Industriels Complets
Conforme PROTOCOLE_MAGEN V3.0 - FORENSIC TOTAL

Principe Fondamental: FORENSIC TOTAL, TOUJOURS, SANS COMPROMIS
"""

import json
import hashlib
import time
import uuid
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional
from pathlib import Path
import numpy as np


class CausalIDManager:
    """
    Gestionnaire IDs causaux hiérarchiques (UUID v4)
    
    Hiérarchie:
        session_id (UUID v4)
          └─ episode_id (UUID v4)
              └─ transition_id (UUID v4)
                  └─ operation_id (UUID v4)
    """
    
    def __init__(self):
        self.session_id = str(uuid.uuid4())
        self.episode_id = None
        self.transition_id = None
        self.operation_id = None
        
        self.episode_counter = 0
        self.transition_counter = 0
        self.operation_counter = 0
    
    def new_episode(self) -> str:
        """Créer nouvel épisode"""
        self.episode_counter += 1
        self.episode_id = str(uuid.uuid4())
        self.transition_counter = 0
        return self.episode_id
    
    def new_transition(self) -> str:
        """Créer nouvelle transition"""
        self.transition_counter += 1
        self.transition_id = str(uuid.uuid4())
        self.operation_counter = 0
        return self.transition_id
    
    def new_operation(self) -> str:
        """Créer nouvelle opération"""
        self.operation_counter += 1
        self.operation_id = str(uuid.uuid4())
        return self.operation_id
    
    def get_full_context(self) -> Dict[str, Any]:
        """Obtenir contexte causal complet"""
        return {
            'session_id': self.session_id,
            'episode_id': self.episode_id,
            'episode_number': self.episode_counter,
            'transition_id': self.transition_id,
            'transition_number': self.transition_counter,
            'operation_id': self.operation_id,
            'operation_number': self.operation_counter
        }


class ForensicLogger:
    """
    Logger forensic avec hash chain blockchain-style
    
    Caractéristiques:
    - Hash chain: Chaque log N contient hash log N-1
    - Timestamps doubles: monotonic + wall clock
    - Hashing SHA256 cryptographique
    - Sérialisation complète bit-level
    - Vérification intégrité chaîne
    """
    
    def __init__(self, log_path: str, id_manager: CausalIDManager):
        self.log_path = Path(log_path)
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        
        self.log_file = open(self.log_path, 'a')
        self.id_manager = id_manager
        self.previous_hash = "0" * 64  # Genesis hash
        self.log_counter = 0
    
    def log(self, event: str, component: str, data: Dict[str, Any]):
        """
        Logger événement avec hash chain
        
        Args:
            event: Nom événement (e.g., 'encoding_start', 'transition_complete')
            component: Nom module (e.g., 'StateEncoderV25', 'AgentV27')
            data: Données complètes à logger
        """
        self.log_counter += 1
        
        entry = {
            'log_number': self.log_counter,
            'previous_hash': self.previous_hash,
            'causal_context': self.id_manager.get_full_context(),
            'timestamp_ns': time.perf_counter_ns(),
            'timestamp_wall_ns': time.time_ns(),
            'timestamp_iso': datetime.now(timezone.utc).isoformat(),
            'event': event,
            'component': component,
            'data': self._serialize_complete(data)
        }
        
        # Calculer hash de cette entrée
        entry_json = json.dumps(entry, sort_keys=True)
        entry_hash = hashlib.sha256(entry_json.encode()).hexdigest()
        entry['entry_hash'] = entry_hash
        
        # Écrire
        self.log_file.write(json.dumps(entry) + '\n')
        self.log_file.flush()
        
        # Mettre à jour previous_hash
        self.previous_hash = entry_hash
    
    def _serialize_complete(self, data: Any) -> Any:
        """
        Sérialisation COMPLÈTE sans perte
        
        INTERDIT: Sample, truncation, approximation
        OBLIGATOIRE: data_full complet
        """
        if isinstance(data, np.ndarray):
            return {
                'type': 'ndarray',
                'shape': list(data.shape),
                'dtype': str(data.dtype),
                'hash_sha256': hashlib.sha256(data.tobytes()).hexdigest(),
                'data_full': data.tolist(),  # ✅ COMPLET, pas sample!
                'min': float(data.min()),
                'max': float(data.max()),
                'mean': float(data.mean()),
                'std': float(data.std()),
                'size_bytes': data.nbytes
            }
        elif isinstance(data, dict):
            return {k: self._serialize_complete(v) for k, v in data.items()}
        elif isinstance(data, (list, tuple)):
            return [self._serialize_complete(v) for v in data]
        elif isinstance(data, (int, float, str, bool, type(None))):
            return data
        else:
            return str(data)
    
    def verify_chain(self, log_path: Optional[str] = None) -> bool:
        """
        Vérifier intégrité chaîne forensic
        
        Returns:
            True si chaîne intègre, False sinon
        """
        path_to_verify = log_path if log_path is not None else str(self.log_path)
        
        try:
            with open(path_to_verify) as f:
                previous_hash = "0" * 64
                for line in f:
                    entry = json.loads(line)
                    
                    # Vérifier chaînage
                    if entry['previous_hash'] != previous_hash:
                        return False
                    
                    # Vérifier hash entrée
                    entry_copy = entry.copy()
                    claimed_hash = entry_copy.pop('entry_hash')
                    entry_json = json.dumps(entry_copy, sort_keys=True)
                    computed_hash = hashlib.sha256(entry_json.encode()).hexdigest()
                    
                    if claimed_hash != computed_hash:
                        return False
                    
                    previous_hash = claimed_hash
            
            return True
        except Exception as e:
            print(f"Error verifying chain: {e}")
            return False
    
    def close(self):
        """Fermer fichier log"""
        if hasattr(self, 'log_file') and self.log_file:
            self.log_file.close()
    
    def __del__(self):
        """Destructor: fermer fichier"""
        self.close()


class SilentErrorDetector:
    """
    Détecteur erreurs silencieuses
    
    Anomalies détectées:
    1. HASH_CONSTANT: Hash constant (collision massive)
    2. EMBEDDING_CONSTANT: Embedding constant (stagnation)
    3. REWARD_BLOCKED: Reward bloqué
    4. ACTION_LOOP: Action loop
    5. NAN_PROPAGATION: NaN propagation
    6. GRADIENT_EXPLOSION: Gradient explosion
    7. ENTROPY_COLLAPSE: Entropy collapse
    """
    
    def __init__(self):
        self.hash_history = []
        self.embedding_history = []
        self.reward_history = []
        self.action_history = []
    
    def check_anomalies(self, log_entry: Dict[str, Any]) -> List[str]:
        """
        Vérifier anomalies dans log entry
        
        Returns:
            Liste anomalies détectées
        """
        anomalies = []
        
        # 1. Hash constant (collision massive)
        if 'topological_hash' in log_entry.get('data', {}):
            self.hash_history.append(log_entry['data']['topological_hash'])
            if len(self.hash_history) > 10:
                unique_hashes = len(set(self.hash_history[-10:]))
                if unique_hashes < 3:
                    anomalies.append('HASH_CONSTANT')
        
        # 2. Embedding constant (stagnation)
        if 'embedding' in log_entry.get('data', {}):
            emb_data = log_entry['data']['embedding']
            if isinstance(emb_data, dict) and 'data_full' in emb_data:
                emb = np.array(emb_data['data_full'])
                self.embedding_history.append(emb)
                if len(self.embedding_history) > 5:
                    variations = [
                        np.linalg.norm(self.embedding_history[i] - self.embedding_history[i-1])
                        for i in range(-4, 0)
                    ]
                    if max(variations) < 0.01:
                        anomalies.append('EMBEDDING_CONSTANT')
        
        # 3. Reward bloqué
        if 'reward' in log_entry.get('data', {}):
            self.reward_history.append(log_entry['data']['reward'])
            if len(self.reward_history) > 20:
                if len(set(self.reward_history[-20:])) == 1:
                    anomalies.append('REWARD_BLOCKED')
        
        # 4. Action loop
        if 'action' in log_entry.get('data', {}):
            self.action_history.append(str(log_entry['data']['action']))
            if len(self.action_history) > 10:
                last_10 = self.action_history[-10:]
                if len(set(last_10)) <= 2:
                    anomalies.append('ACTION_LOOP')
        
        # 5. NaN propagation
        if 'embedding' in log_entry.get('data', {}):
            emb_data = log_entry['data']['embedding']
            if isinstance(emb_data, dict) and 'data_full' in emb_data:
                emb = np.array(emb_data['data_full'])
                if np.isnan(emb).any():
                    anomalies.append('NAN_PROPAGATION')
        
        # 6. Gradient explosion
        if 'embedding' in log_entry.get('data', {}):
            emb_data = log_entry['data']['embedding']
            if isinstance(emb_data, dict) and 'data_full' in emb_data:
                emb = np.array(emb_data['data_full'])
                if np.abs(emb).max() > 1e6:
                    anomalies.append('GRADIENT_EXPLOSION')
        
        # 7. Entropy collapse
        if 'action_probs' in log_entry.get('data', {}):
            probs = np.array(log_entry['data']['action_probs'], dtype=np.float64)
            entropy = float(-np.sum(probs * np.log(probs + 1e-10)))
            if entropy < 0.1:
                anomalies.append('ENTROPY_COLLAPSE')
        
        return anomalies


class ForensicReplay:
    """
    Système replay forensic
    
    Modes:
    1. replay_exact: Replay exact avec même seed
    2. replay_accelerated: Replay accéléré pour debugging
    3. replay_differential: Comparer deux exécutions
    4. replay_causal: Rejouer jusqu'à état cible
    """
    
    def __init__(self, log_path: str):
        self.log_path = Path(log_path)
        self.logs = self._load_logs()
        self.verify_integrity()
    
    def _load_logs(self) -> List[Dict[str, Any]]:
        """Charger logs depuis fichier"""
        logs = []
        with open(self.log_path) as f:
            for line in f:
                logs.append(json.loads(line))
        return logs
    
    def verify_integrity(self) -> bool:
        """Vérifier intégrité chaîne"""
        previous_hash = "0" * 64
        for entry in self.logs:
            if entry['previous_hash'] != previous_hash:
                raise ValueError(f"Hash chain broken at log {entry['log_number']}")
            
            entry_copy = entry.copy()
            claimed_hash = entry_copy.pop('entry_hash')
            entry_json = json.dumps(entry_copy, sort_keys=True)
            computed_hash = hashlib.sha256(entry_json.encode()).hexdigest()
            
            if claimed_hash != computed_hash:
                raise ValueError(f"Hash mismatch at log {entry['log_number']}")
            
            previous_hash = claimed_hash
        
        return True
    
    def replay_exact(self):
        """Replay exact avec même seed"""
        for log in self.logs:
            # TODO: Implémenter exécution opération
            pass
    
    def replay_accelerated(self, speed: float = 10.0):
        """Replay accéléré pour debugging"""
        for i, log in enumerate(self.logs):
            if i > 0:
                duration_ns = log['timestamp_ns'] - self.logs[i-1]['timestamp_ns']
                time.sleep(duration_ns / 1e9 / speed)
            # TODO: Implémenter exécution opération
    
    def replay_differential(self, other_log_path: str) -> List[Dict[str, Any]]:
        """Comparer deux exécutions"""
        other_replay = ForensicReplay(other_log_path)
        differences = []
        
        for i, (log1, log2) in enumerate(zip(self.logs, other_replay.logs)):
            if log1['data'] != log2['data']:
                differences.append({
                    'step': i,
                    'log1': log1,
                    'log2': log2
                })
        
        return differences
    
    def replay_causal(self, target_state: str):
        """Rejouer jusqu'à état cible"""
        for log in self.logs:
            # TODO: Implémenter exécution opération
            # TODO: Vérifier si état cible atteint
            pass


def compute_sha256(data: bytes) -> str:
    """Calculer hash SHA256"""
    return hashlib.sha256(data).hexdigest()


def compute_sha256_array(array: np.ndarray) -> str:
    """Calculer hash SHA256 d'un ndarray"""
    return hashlib.sha256(array.tobytes()).hexdigest()

# Made with Bob
