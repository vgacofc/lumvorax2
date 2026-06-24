"""
STATE ENCODER V25 - MAGEN
Encodage états enrichi avec relations causales + historique

PROBLÈME V24: Embeddings trop similaires (motion≈0, regions=0)
→ Graphe causal effondré (1 node) → Planification impossible

SOLUTION V25: Enrichir signature d'état
- Relations spatiales objets↔objets
- Hash topologique local  
- Historique court (état + action précédente)
- Différences structurelles grille
- Signatures d'interaction

Architecture:
- Object Detection: Identifier entités + positions
- Spatial Relations: Distances, alignements, containment
- Topological Hash: Structure locale invariante
- Motion Analysis: Vecteurs déplacement temporel
- Region Attention: Zones actives/statiques
- Action Context: Embedding(state_t, delta_t-1, action_t-1)
- Embedding Compression: État enrichi → vecteur normalisé (256D)

Forensic: Logs bit-level extraction + timestamps nanoseconde
"""

import numpy as np
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import time
from collections import defaultdict
import hashlib

# Forensic middleware V3.0
from .forensic_middleware import ForensicMiddleware

@dataclass
class StateEmbedding:
    """Embedding état enrichi avec métadonnées causales"""
    vector: np.ndarray  # (256,) normalisé - AUGMENTÉ de 128→256
    objects_detected: int
    motion_magnitude: float
    active_regions: List[Tuple[int, int, int, int]]  # (x, y, w, h)
    spatial_relations_count: int  # NOUVEAU V25
    topological_hash: str  # NOUVEAU V25
    action_context: Optional[str]  # NOUVEAU V25
    timestamp_ns: int
    raw_shape: Tuple[int, ...]
    
    def __post_init__(self):
        """Validation embedding"""
        assert self.vector.shape == (256,), f"Embedding shape {self.vector.shape} != (256,)"
        assert np.allclose(np.linalg.norm(self.vector), 1.0, atol=1e-6), "Embedding non normalisé"


class StateEncoderV25:
    """
    Encodeur états environnementaux → embeddings causaux enrichis
    
    V25: Ajoute relations spatiales + historique pour augmenter séparabilité
    """
    
    def __init__(
        self,
        embedding_dim: int = 256,
        motion_threshold: float = 0.1,
        forensic_log_path: str = "logs/state_encoder_v25.jsonl",
        enable_forensic: bool = True
    ):
        self.embedding_dim = embedding_dim
        self.motion_threshold = motion_threshold
        
        # Forensic middleware V3.0
        self.forensic = ForensicMiddleware(
            forensic_log_path,
            "StateEncoderV25"
        ) if enable_forensic else None
        
        # Historique pour motion analysis + action context
        self.previous_state: Optional[np.ndarray] = None
        self.previous_embedding: Optional[StateEmbedding] = None
        self.previous_action: Optional[str] = None  # NOUVEAU V25
        
        # Statistiques forensiques
        self.stats = {
            'total_encodings': 0,
            'avg_objects_detected': 0.0,
            'avg_motion_magnitude': 0.0,
            'avg_active_regions': 0.0,
            'avg_spatial_relations': 0.0,  # NOUVEAU V25
            'unique_topological_hashes': set(),  # NOUVEAU V25
            'encoding_times_ns': [],
            'shape_distribution': defaultdict(int)
        }
        
        print("[STATE ENCODER V25] Initialisé")
        print(f"  Embedding dim: {embedding_dim} (augmenté de 128→256)")
        print(f"  Motion threshold: {motion_threshold}")
        print(f"  Features: Spatial Relations + Topological Hash + Action Context")
        if enable_forensic:
            print(f"  Forensic: ENABLED → {forensic_log_path}")
    
    @property
    def _forensic_state(self) -> Dict:
        """État pour forensic (capture état interne)"""
        return {
            'embedding_dim': self.embedding_dim,
            'has_previous_state': self.previous_state is not None,
            'has_previous_embedding': self.previous_embedding is not None,
            'previous_action': self.previous_action,
            'last_action': self.previous_action,
            'total_encodings': self.stats['total_encodings'],
            'unique_hashes': len(self.stats['unique_topological_hashes'])
        }
    
    def close(self):
        """Fermer forensic logger"""
        if self.forensic:
            self.forensic.close()
    
    def encode(self, state: np.ndarray, action: Optional[str] = None) -> StateEmbedding:
        """
        Encode état brut → embedding causal enrichi normalisé
        
        Pipeline V25:
        1. Object Detection (blobs connexes + positions)
        2. Spatial Relations (distances, alignements)
        3. Topological Hash (structure locale)
        4. Motion Analysis (delta temporel)
        5. Region Attention (heatmap activité)
        6. Action Context (action précédente)
        7. Embedding Compression (PCA-like enrichi)
        8. Normalisation L2
        
        Args:
            state: État brut (H, W) ou (H, W, C)
            action: Action précédente (optionnel)
        
        Returns:
            StateEmbedding avec vecteur normalisé + métadonnées enrichies
        """
        # Forensic AVANT
        if self.forensic:
            self.forensic.id_manager.new_operation()
            self.forensic.log_event('encode_start', {
                'state_shape': state.shape,
                'state_dtype': str(state.dtype),
                'action': action,
                'forensic_state': self._forensic_state
            })
        
        start_ns = time.perf_counter_ns()
        
        # Normaliser shape (H, W, C)
        if state.ndim == 2:
            state = state[:, :, np.newaxis]
        elif state.ndim == 3 and state.shape[0] < state.shape[2]:
            # (C, H, W) → (H, W, C)
            state = np.transpose(state, (1, 2, 0))
        
        raw_shape = state.shape
        self.stats['shape_distribution'][str(raw_shape)] += 1
        
        # 1. Object Detection + Positions
        objects_detected, object_positions = self._detect_objects_with_positions(state)
        
        # 2. Spatial Relations (NOUVEAU V25)
        spatial_relations = self._compute_spatial_relations(object_positions)
        
        # 3. Topological Hash (NOUVEAU V25)
        topological_hash = self._compute_topological_hash(state)
        
        # 4. Motion Analysis
        motion_magnitude = self._analyze_motion(state)
        
        # 5. Region Attention
        active_regions = self._detect_active_regions(state)
        
        # 6. Action Context (NOUVEAU V25)
        action_context = action if action else self.previous_action
        
        # 7. Embedding Compression (enrichi)
        embedding_vector = self._compress_to_embedding_v25(
            state, 
            objects_detected, 
            object_positions,
            spatial_relations,
            topological_hash,
            motion_magnitude, 
            active_regions,
            action_context
        )
        
        # 8. Normalisation L2
        norm = np.linalg.norm(embedding_vector)
        if norm > 1e-10:
            embedding_vector = embedding_vector / norm
        else:
            embedding_vector = np.zeros(self.embedding_dim, dtype=np.float32)
            embedding_vector[0] = 1.0  # Vecteur par défaut
        
        # Créer embedding
        embedding = StateEmbedding(
            vector=embedding_vector,
            objects_detected=objects_detected,
            motion_magnitude=motion_magnitude,
            active_regions=active_regions,
            spatial_relations_count=len(spatial_relations),
            topological_hash=topological_hash,
            action_context=action_context,
            timestamp_ns=time.perf_counter_ns(),
            raw_shape=raw_shape
        )
        
        # Mise à jour historique
        self.previous_state = state.copy()
        self.previous_embedding = embedding
        self.previous_action = action
        
        # Statistiques forensiques
        encoding_time_ns = time.perf_counter_ns() - start_ns
        self.stats['total_encodings'] += 1
        self.stats['encoding_times_ns'].append(encoding_time_ns)
        self.stats['avg_objects_detected'] = (
            (self.stats['avg_objects_detected'] * (self.stats['total_encodings'] - 1) + objects_detected)
            / self.stats['total_encodings']
        )
        self.stats['avg_motion_magnitude'] = (
            (self.stats['avg_motion_magnitude'] * (self.stats['total_encodings'] - 1) + motion_magnitude)
            / self.stats['total_encodings']
        )
        self.stats['avg_active_regions'] = (
            (self.stats['avg_active_regions'] * (self.stats['total_encodings'] - 1) + len(active_regions))
            / self.stats['total_encodings']
        )
        self.stats['avg_spatial_relations'] = (
            (self.stats['avg_spatial_relations'] * (self.stats['total_encodings'] - 1) + len(spatial_relations))
            / self.stats['total_encodings']
        )
        self.stats['unique_topological_hashes'].add(topological_hash)
        
        # Forensic APRÈS
        if self.forensic:
            self.forensic.log_event('encode_complete', {
                'embedding': {
                    'vector_shape': embedding.vector.shape,
                    'vector_norm': float(np.linalg.norm(embedding.vector)),
                    'objects_detected': embedding.objects_detected,
                    'motion_magnitude': float(embedding.motion_magnitude),
                    'active_regions_count': len(embedding.active_regions),
                    'spatial_relations_count': embedding.spatial_relations_count,
                    'topological_hash': embedding.topological_hash,
                    'action_context': embedding.action_context
                },
                'forensic_state': self._forensic_state,
                'duration_ns': encoding_time_ns
            })
        
        # Log forensique
        if self.stats['total_encodings'] % 10 == 0:
            avg_time_us = np.mean(self.stats['encoding_times_ns'][-10:]) / 1000
            print(f"[STATE ENCODER V25] Encoding #{self.stats['total_encodings']}: "
                  f"objects={objects_detected}, motion={motion_magnitude:.3f}, "
                  f"regions={len(active_regions)}, relations={len(spatial_relations)}, "
                  f"unique_hashes={len(self.stats['unique_topological_hashes'])}, "
                  f"time={avg_time_us:.1f}µs")
        
        return embedding
    
    def _detect_objects_with_positions(self, state: np.ndarray) -> Tuple[int, List[Tuple[int, int]]]:
        """
        Détecte objets via connected components + positions
        
        Returns:
            (count, [(x, y), ...]) positions centroids
        """
        # Convertir grayscale si RGB
        if state.shape[2] > 1:
            gray = np.mean(state, axis=2)
        else:
            gray = state[:, :, 0]
        
        # Binarisation adaptative
        threshold = np.mean(gray) + 0.5 * np.std(gray)
        binary = (gray > threshold).astype(np.uint8)
        
        # Trouver positions objets (approximation: peaks locaux)
        h, w = gray.shape
        positions = []
        
        # Diviser en grille 4x4 et trouver max local
        for i in range(4):
            for j in range(4):
                y1 = i * h // 4
                y2 = (i + 1) * h // 4
                x1 = j * w // 4
                x2 = (j + 1) * w // 4
                
                cell = gray[y1:y2, x1:x2]
                if np.max(cell) > threshold:
                    # Position relative max dans cellule
                    local_max = np.unravel_index(np.argmax(cell), cell.shape)
                    positions.append((x1 + local_max[1], y1 + local_max[0]))
        
        objects = len(positions)
        return max(0, min(objects, 50)), positions  # Clamp [0, 50]
    
    def _compute_spatial_relations(self, positions: List[Tuple[int, int]]) -> List[Dict]:
        """
        Calcule relations spatiales entre objets
        
        Relations:
        - Distance euclidienne
        - Alignement horizontal/vertical
        - Quadrant relatif
        
        Returns:
            Liste relations [(obj1, obj2, distance, alignment), ...]
        """
        relations = []
        
        for i, (x1, y1) in enumerate(positions):
            for j, (x2, y2) in enumerate(positions[i+1:], start=i+1):
                # Distance euclidienne
                dist = np.sqrt((x2 - x1)**2 + (y2 - y1)**2)
                
                # Alignement
                dx = abs(x2 - x1)
                dy = abs(y2 - y1)
                alignment = 'horizontal' if dx > dy * 2 else ('vertical' if dy > dx * 2 else 'diagonal')
                
                # Quadrant
                quadrant = (1 if x2 > x1 else -1, 1 if y2 > y1 else -1)
                
                relations.append({
                    'obj1': i,
                    'obj2': j,
                    'distance': dist,
                    'alignment': alignment,
                    'quadrant': quadrant
                })
        
        return relations
    
    def _compute_topological_hash(self, state: np.ndarray, grid_size: int = 4) -> str:
        """
        SOLUTION A: Hash pixel-level avec downsampling
        
        Remplace hash topologique grossier par hash précis du contenu.
        Détecte TOUS changements pixels sans collision massive.
        
        Performance: Downsampling 64x64 → 16x16 = 256 pixels
        Hash: SHA256 (stable, cryptographique)
        
        Returns:
            Hash SHA256 (8 premiers caractères)
        """
        # Convertir grayscale
        if len(state.shape) == 3 and state.shape[2] > 1:
            gray = np.mean(state, axis=2)
        elif len(state.shape) == 3:
            gray = state[:, :, 0]
        else:
            gray = state
        
        # Downsampling pour performance (64x64 → 16x16)
        try:
            from skimage.transform import resize
            downsampled = resize(gray, (16, 16), anti_aliasing=True, preserve_range=True)
            downsampled = downsampled.astype(np.uint8)
        except ImportError:
            # Fallback: simple binning si skimage absent
            h, w = gray.shape
            bin_h, bin_w = 16, 16
            cell_h = h // bin_h
            cell_w = w // bin_w
            downsampled = np.zeros((bin_h, bin_w), dtype=np.uint8)
            for i in range(bin_h):
                for j in range(bin_w):
                    y1 = i * cell_h
                    y2 = min((i + 1) * cell_h, h)
                    x1 = j * cell_w
                    x2 = min((j + 1) * cell_w, w)
                    # Utiliser nanmean pour éviter warnings sur slices vides
                    cell_mean = np.nanmean(gray[y1:y2, x1:x2])
                    # Gérer NaN (slice vide) - nanmean retourne NaN si tout est NaN
                    downsampled[i, j] = 0 if np.isnan(cell_mean) else int(cell_mean)
        
        # Hash SHA256 (stable, cryptographique)
        return hashlib.sha256(downsampled.tobytes()).hexdigest()[:8]
    
    def _analyze_motion(self, state: np.ndarray) -> float:
        """
        Analyse mouvement via différence temporelle
        
        Returns:
            Magnitude mouvement normalisée [0, 1]
        """
        if self.previous_state is None:
            return 0.0
        
        # Vérifier compatibilité shapes
        if self.previous_state.shape != state.shape:
            return 0.0
        
        # Delta temporel
        delta = np.abs(state.astype(np.float32) - self.previous_state.astype(np.float32))
        motion = np.mean(delta) / 255.0  # Normaliser [0, 1]
        
        return float(motion)
    
    def _detect_active_regions(self, state: np.ndarray, grid_size: int = 8) -> List[Tuple[int, int, int, int]]:
        """
        Détecte régions actives via heatmap activité
        
        Divise image en grille, identifie cellules avec variance élevée
        
        Returns:
            Liste (x, y, w, h) régions actives
        """
        h, w = state.shape[:2]
        cell_h = h // grid_size
        cell_w = w // grid_size
        
        active_regions = []
        
        for i in range(grid_size):
            for j in range(grid_size):
                y1 = i * cell_h
                y2 = min((i + 1) * cell_h, h)
                x1 = j * cell_w
                x2 = min((j + 1) * cell_w, w)
                
                cell = state[y1:y2, x1:x2]
                variance = np.var(cell)
                
                # Région active si variance > threshold
                if variance > 100.0:  # Threshold empirique
                    active_regions.append((x1, y1, x2 - x1, y2 - y1))
        
        return active_regions
    
    def _compress_to_embedding_v25(
        self,
        state: np.ndarray,
        objects: int,
        object_positions: List[Tuple[int, int]],
        spatial_relations: List[Dict],
        topological_hash: str,
        motion: float,
        regions: List[Tuple[int, int, int, int]],
        action_context: Optional[str]
    ) -> np.ndarray:
        """
        Compresse état enrichi + métadonnées → embedding (256D)
        
        Stratégie V25:
        - 64D: Spatial features (downsampled state)
        - 32D: Object features (positions, sizes)
        - 32D: Spatial relations (distances, alignements) [NOUVEAU]
        - 32D: Topological features (hash, structure) [NOUVEAU]
        - 32D: Motion features (direction, magnitude)
        - 32D: Attention features (active regions)
        - 32D: Action context (previous action embedding) [NOUVEAU]
        """
        embedding = np.zeros(self.embedding_dim, dtype=np.float32)
        
        # 1. Spatial features (64D): Downsample state
        h, w = state.shape[:2]
        target_h, target_w = 8, 8
        
        # Resize via binning
        bin_h = h // target_h
        bin_w = w // target_w
        
        spatial_features = []
        for i in range(target_h):
            for j in range(target_w):
                y1 = i * bin_h
                y2 = min((i + 1) * bin_h, h)
                x1 = j * bin_w
                x2 = min((j + 1) * bin_w, w)
                
                cell = state[y1:y2, x1:x2]
                # Utiliser nanmean pour éviter warnings sur slices vides
                spatial_features.append(np.nanmean(cell) / 255.0 if cell.size > 0 else 0.0)
        
        embedding[:64] = np.array(spatial_features[:64])
        
        # 2. Object features (32D): Positions normalisées
        embedding[64] = objects / 50.0  # Count normalisé
        for idx, (x, y) in enumerate(object_positions[:10]):  # Max 10 objets
            embedding[65 + idx * 2] = x / w  # X normalisé
            embedding[66 + idx * 2] = y / h  # Y normalisé
        
        # 3. Spatial relations (32D) [NOUVEAU V25]
        embedding[96] = len(spatial_relations) / 100.0  # Count normalisé
        for idx, rel in enumerate(spatial_relations[:10]):  # Max 10 relations
            embedding[97 + idx * 3] = rel['distance'] / (h + w)  # Distance normalisée
            embedding[98 + idx * 3] = {'horizontal': 0.0, 'vertical': 0.5, 'diagonal': 1.0}[rel['alignment']]
            embedding[99 + idx * 3] = (rel['quadrant'][0] + rel['quadrant'][1]) / 4.0  # Quadrant normalisé
        
        # 4. Topological features (32D) [NOUVEAU V25]
        # Encoder hash comme features numériques
        hash_bytes = bytes.fromhex(topological_hash)
        for idx, byte in enumerate(hash_bytes[:32]):
            embedding[128 + idx] = byte / 255.0
        
        # 5. Motion features (32D)
        embedding[160] = motion
        # Reste: padding zeros (TODO: optical flow si nécessaire)
        
        # 6. Attention features (32D)
        embedding[192] = len(regions) / 64.0  # Count normalisé
        if regions:
            # Encoder première région
            x, y, w_r, h_r = regions[0]
            embedding[193] = x / w
            embedding[194] = y / h
            embedding[195] = w_r / w
            embedding[196] = h_r / h
        
        # 7. Action context (32D) [NOUVEAU V25]
        if action_context:
            # Encoder action comme one-hot
            action_map = {
                'ACTION1': 0, 'ACTION2': 1, 'ACTION3': 2, 'ACTION4': 3,
                'ACTION5': 4, 'ACTION6': 5, 'ACTION7': 6, 'RESET': 7
            }
            if action_context in action_map:
                embedding[224 + action_map[action_context]] = 1.0
        
        return embedding

# Made with Bob
