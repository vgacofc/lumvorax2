"""
Tests d'intégration forensic pour State Encoder V25

Valide:
1. Logs générés correctement (AVANT/APRÈS)
2. Hash chain intègre
3. État forensic capturé
4. Métriques embedding loggées
5. Causal IDs hiérarchiques
"""

import pytest
import numpy as np
import json
import tempfile
import warnings
from pathlib import Path

from MAGEN.core.state_encoder_v25 import StateEncoderV25

# Supprimer warnings NumPy sur slices vides (comportement attendu dans tests)
warnings.filterwarnings('ignore', message='Mean of empty slice')
warnings.filterwarnings('ignore', message='invalid value encountered in scalar divide')


class TestStateEncoderV25Forensic:
    """Tests intégration forensic State Encoder V25"""
    
    def test_forensic_logs_generated(self):
        """Valide que logs forensic sont générés"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_state_encoder.jsonl"
            
            # Créer encoder avec forensic activé (256D requis par V25)
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            # Encoder un état simple
            state = np.random.randint(0, 10, size=(10, 10))
            embedding = encoder.encode(state, action="rotate_90")
            
            # Fermer pour flush logs
            encoder.close()
            
            # Vérifier fichier créé
            assert log_path.exists(), "Fichier forensic non créé"
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Valider au moins 2 logs (start + complete)
            assert len(logs) >= 2, f"Attendu >=2 logs, obtenu {len(logs)}"
            
            # Valider événements
            events = [log['event'] for log in logs]
            assert 'encode_start' in events, "Événement encode_start manquant"
            assert 'encode_complete' in events, "Événement encode_complete manquant"
    
    def test_forensic_hash_chain_integrity(self):
        """Valide intégrité hash chain"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_hash_chain.jsonl"
            
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            # Encoder 3 états
            for i in range(3):
                state = np.random.randint(0, 10, size=(10, 10))
                encoder.encode(state)
            
            encoder.close()
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Valider hash chain
            for i in range(1, len(logs)):
                expected_prev_hash = logs[i-1]['entry_hash']
                actual_prev_hash = logs[i]['previous_hash']
                assert expected_prev_hash == actual_prev_hash, \
                    f"Hash chain rompu au log {i}: {expected_prev_hash} != {actual_prev_hash}"
    
    def test_forensic_state_captured(self):
        """Valide que état forensic est capturé"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_state.jsonl"
            
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            state = np.random.randint(0, 10, size=(10, 10))
            encoder.encode(state, action="flip_horizontal")
            encoder.close()
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Trouver log encode_complete (action enregistrée APRÈS encoding)
            complete_log = next(log for log in logs if log['event'] == 'encode_complete')
            
            # Valider forensic_state présent
            assert 'forensic_state' in complete_log['data'], "forensic_state manquant"
            forensic_state = complete_log['data']['forensic_state']
            
            # Valider contenu
            assert 'total_encodings' in forensic_state
            assert 'embedding_dim' in forensic_state
            assert 'last_action' in forensic_state
            # Action enregistrée après premier encoding
            assert forensic_state['last_action'] == "flip_horizontal"
    
    def test_forensic_embedding_metrics_logged(self):
        """Valide que métriques embedding sont loggées"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_metrics.jsonl"
            
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            state = np.random.randint(0, 10, size=(10, 10))
            embedding = encoder.encode(state)
            encoder.close()
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Trouver log encode_complete
            complete_log = next(log for log in logs if log['event'] == 'encode_complete')
            
            # Valider métriques embedding
            assert 'embedding' in complete_log['data']
            emb_data = complete_log['data']['embedding']
            
            assert 'vector_shape' in emb_data
            assert 'vector_norm' in emb_data
            assert 'objects_detected' in emb_data
            assert 'motion_magnitude' in emb_data
            assert 'topological_hash' in emb_data
            
            # Valider cohérence
            assert emb_data['vector_shape'] == [256], f"Shape incorrecte: {emb_data['vector_shape']}"
            assert emb_data['objects_detected'] == embedding.objects_detected
    
    def test_forensic_causal_ids_hierarchical(self):
        """Valide hiérarchie causal IDs (session → episode → transition → operation)"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_causal.jsonl"
            
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            # Encoder 2 états (2 operations)
            for i in range(2):
                state = np.random.randint(0, 10, size=(10, 10))
                encoder.encode(state)
            
            encoder.close()
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Valider causal_context présent
            for log in logs:
                assert 'causal_context' in log
                ctx = log['causal_context']
                
                assert 'session_id' in ctx
                assert 'episode_id' in ctx
                assert 'transition_id' in ctx
                assert 'operation_id' in ctx
                
                # Valider format UUID
                assert len(ctx['session_id']) == 36  # UUID v4 format
                assert len(ctx['operation_id']) == 36
    
    def test_forensic_disabled_no_logs(self):
        """Valide que forensic désactivé ne génère pas de logs"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_disabled.jsonl"
            
            # Créer encoder SANS forensic
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=False
            )
            
            state = np.random.randint(0, 10, size=(10, 10))
            encoder.encode(state)
            encoder.close()
            
            # Vérifier fichier NON créé
            assert not log_path.exists(), "Fichier forensic créé alors que désactivé"
    
    def test_forensic_multiple_encodings_sequence(self):
        """Valide séquence multiple encodings avec forensic"""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "forensic_sequence.jsonl"
            
            encoder = StateEncoderV25(
                embedding_dim=256,
                forensic_log_path=str(log_path),
                enable_forensic=True
            )
            
            # Séquence 5 encodings
            actions = ["rotate_90", "flip_horizontal", "identity", "rotate_180", "flip_vertical"]
            for action in actions:
                state = np.random.randint(0, 10, size=(10, 10))
                encoder.encode(state, action=action)
            
            encoder.close()
            
            # Lire logs
            logs = []
            with open(log_path, 'r') as f:
                for line in f:
                    logs.append(json.loads(line))
            
            # Valider nombre logs (5 encodings × 2 events = 10 logs minimum)
            assert len(logs) >= 10, f"Attendu >=10 logs, obtenu {len(logs)}"
            
            # Valider actions loggées
            start_logs = [log for log in logs if log['event'] == 'encode_start']
            logged_actions = [log['data']['action'] for log in start_logs]
            
            assert logged_actions == actions, f"Actions incorrectes: {logged_actions}"


if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])

# Made with Bob
