#!/usr/bin/env python3
"""
PerceptionFrame - Structure Unifiée de Perception
==================================================

Colonne vertébrale épistémologique de MAGEN V27.1+
Garantit continuité, traçabilité, causalité et cohérence temporelle.

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14
Session: 73
Phase: 1A - Restauration Continuité Perceptive
"""

from dataclasses import dataclass, field
from typing import Optional, List, Tuple, Dict, Any
import numpy as np
import time


@dataclass
class Entity:
    """Entité détectée dans la grille."""
    id: int
    position: Tuple[int, int]
    color: int
    size: int
    is_mobile: bool
    confidence: float


@dataclass
class PerceptionFrame:
    """
    Structure normalisée pour TOUS les modules MAGEN.
    
    Garantit:
    - Continuité perceptive (before → after)
    - Traçabilité forensic (timestamps, source)
    - Causalité (action_context, transition_valid)
    - Cohérence temporelle (identité perceptive)
    - Intégrité (validation automatique)
    
    Principe Fondamental:
    ---------------------
    Perception Réelle = Observation + Changement + Causalité
    
    Usage:
    ------
    # Perception initiale
    frame_before = PerceptionFrame.from_observation(obs, source="initial")
    
    # Action
    action = agent.decide(frame_before)
    obs_after = env.step(action)
    
    # Perception après action
    frame_after = PerceptionFrame.from_observation(
        obs_after, 
        source="after_action",
        action_context=action
    )
    
    # Calcul delta
    delta = frame_after.compute_delta(frame_before)
    
    # Validation perception réelle
    is_real = delta.has_change and delta.has_causality
    """
    
    # ========== RAW DATA ==========
    raw_grid: np.ndarray
    """Grille brute de l'environnement (H, W) ou (C, H, W)"""
    
    processed_grid: Optional[np.ndarray] = None
    """Grille après preprocessing (normalisation, etc.)"""
    
    # ========== EMBEDDINGS ==========
    embedding_vector: Optional[np.ndarray] = None
    """Vecteur embedding (256D pour V25)"""
    
    topological_hash: Optional[str] = None
    """Hash topologique de l'état (hex string)"""
    
    # ========== SPATIAL ==========
    motion_map: Optional[np.ndarray] = None
    """Carte de mouvement (différence avec état précédent)"""
    
    spatial_relations: List[Tuple] = field(default_factory=list)
    """Relations spatiales entre objets [(obj1, obj2, relation, distance)]"""
    
    # ========== ENTITIES ==========
    entities: List[Entity] = field(default_factory=list)
    """Entités détectées dans la grille"""
    
    agent_detected: bool = False
    """Agent/joueur détecté (inférence causale)"""
    
    agent_position: Optional[Tuple[int, int]] = None
    """Position estimée de l'agent"""
    
    agent_confidence: float = 0.0
    """Confiance détection agent [0.0-1.0]"""
    
    # ========== METADATA ==========
    timestamp_ns: int = field(default_factory=lambda: int(time.time_ns()))
    """Timestamp nanoseconde (traçabilité)"""
    
    action_context: Optional[str] = None
    """Action qui a produit cet état (None si initial)"""
    
    transition_valid: bool = True
    """Transition valide depuis état précédent"""
    
    # ========== FORENSIC ==========
    source: str = "unknown"
    """Source: 'initial' | 'after_action' | 'replay' | 'synthetic'"""
    
    integrity_checks: Dict[str, bool] = field(default_factory=dict)
    """Résultats vérifications intégrité"""
    
    error_log: List[str] = field(default_factory=list)
    """Erreurs détectées lors création/validation"""
    
    # ========== COMPUTED PROPERTIES ==========
    
    @property
    def grid_shape(self) -> Tuple[int, ...]:
        """Shape de la grille."""
        return self.raw_grid.shape if self.raw_grid is not None else ()
    
    @property
    def grid_dtype(self) -> str:
        """Type de données de la grille."""
        return str(self.raw_grid.dtype) if self.raw_grid is not None else "None"
    
    @property
    def is_valid(self) -> bool:
        """Frame valide si grid non None et checks OK."""
        return (
            self.raw_grid is not None and
            len(self.raw_grid.shape) >= 2 and
            all(self.integrity_checks.values())
        )
    
    @property
    def has_embedding(self) -> bool:
        """Embedding calculé."""
        return self.embedding_vector is not None
    
    @property
    def has_entities(self) -> bool:
        """Entités détectées."""
        return len(self.entities) > 0
    
    # ========== FACTORY METHODS ==========
    
    @classmethod
    def from_observation(
        cls,
        obs: Any,
        source: str = "unknown",
        action_context: Optional[str] = None
    ) -> 'PerceptionFrame':
        """
        Créer PerceptionFrame depuis observation environnement.
        
        Args:
            obs: Observation (structure variable selon env)
            source: Source de l'observation
            action_context: Action qui a produit cet état
        
        Returns:
            frame: PerceptionFrame validé
        """
        frame = cls(
            raw_grid=np.array([]),  # Placeholder
            source=source,
            action_context=action_context
        )
        
        # Extraire grid (structure variable)
        grid = None
        if hasattr(obs, 'grid'):
            grid = obs.grid
        elif hasattr(obs, 'observation'):
            grid = obs.observation
        elif hasattr(obs, 'state'):
            grid = obs.state
        elif hasattr(obs, 'pixels'):
            grid = obs.pixels
        elif isinstance(obs, np.ndarray):
            grid = obs
        elif isinstance(obs, dict) and 'grid' in obs:
            grid = obs['grid']
        
        # Validation grid
        if grid is None:
            frame.error_log.append("Grid extraction failed: obs has no grid attribute")
            frame.integrity_checks['grid_extracted'] = False
            frame.transition_valid = False
        elif not isinstance(grid, np.ndarray):
            frame.error_log.append(f"Grid is not ndarray: {type(grid)}")
            frame.integrity_checks['grid_type'] = False
            frame.transition_valid = False
        elif len(grid.shape) < 2:
            frame.error_log.append(f"Grid dimensions invalid: {grid.shape}")
            frame.integrity_checks['grid_dimensions'] = False
            frame.transition_valid = False
        else:
            frame.raw_grid = grid
            frame.integrity_checks['grid_extracted'] = True
            frame.integrity_checks['grid_type'] = True
            frame.integrity_checks['grid_dimensions'] = True
        
        # Extraire metadata si disponible
        if hasattr(obs, 'state'):
            frame.integrity_checks['env_state_present'] = True
        
        return frame
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PerceptionFrame':
        """
        Créer PerceptionFrame depuis dict (perception agent V27).
        
        Args:
            data: Dict perception agent
        
        Returns:
            frame: PerceptionFrame
        """
        frame = cls(
            raw_grid=data.get('grid', np.array([])),
            source="agent_perception"
        )
        
        # Embedding
        if 'embedding' in data:
            emb = data['embedding']
            if isinstance(emb, dict):
                frame.embedding_vector = emb.get('vector')
                frame.topological_hash = emb.get('topological_hash')
                frame.spatial_relations = emb.get('spatial_relations', [])
            else:
                frame.embedding_vector = emb
        
        # Entities
        frame.agent_detected = data.get('agent_detected_causal', False)
        frame.agent_position = data.get('agent_position_causal')
        frame.agent_confidence = data.get('agent_confidence', 0.0)
        
        # Metadata
        frame.action_context = data.get('action_context')
        
        return frame
    
    # ========== DELTA COMPUTATION ==========
    
    def compute_delta(self, frame_before: 'PerceptionFrame') -> 'PerceptionDelta':
        """
        Calculer différence perceptible entre deux frames.
        
        Args:
            frame_before: Frame avant action
        
        Returns:
            delta: PerceptionDelta
        """
        return PerceptionDelta.compute(frame_before, self)
    
    # ========== VALIDATION ==========
    
    def validate(self) -> bool:
        """
        Valider intégrité complète du frame.
        
        Returns:
            valid: True si tous les checks passent
        """
        checks = {}
        
        # Grid
        checks['grid_not_none'] = self.raw_grid is not None
        checks['grid_is_ndarray'] = isinstance(self.raw_grid, np.ndarray)
        checks['grid_has_dimensions'] = len(self.grid_shape) >= 2 if checks['grid_is_ndarray'] else False
        
        # Embedding
        if self.embedding_vector is not None:
            checks['embedding_is_ndarray'] = isinstance(self.embedding_vector, np.ndarray)
            checks['embedding_has_shape'] = len(self.embedding_vector.shape) == 1
        
        # Hash
        if self.topological_hash is not None:
            checks['hash_is_string'] = isinstance(self.topological_hash, str)
            checks['hash_is_hex'] = all(c in '0123456789abcdef' for c in self.topological_hash)
        
        # Timestamp
        checks['timestamp_valid'] = self.timestamp_ns > 0
        
        # Source
        checks['source_valid'] = self.source in ['initial', 'after_action', 'replay', 'synthetic', 'agent_perception', 'unknown']
        
        self.integrity_checks.update(checks)
        return all(checks.values())
    
    # ========== FORENSIC ==========
    
    def to_forensic_dict(self) -> Dict[str, Any]:
        """
        Exporter frame pour logging forensic.
        
        Returns:
            data: Dict sérialisable JSON
        """
        return {
            'timestamp_ns': self.timestamp_ns,
            'source': self.source,
            'action_context': self.action_context,
            'grid_shape': list(self.grid_shape),
            'grid_dtype': self.grid_dtype,
            'topological_hash': self.topological_hash,
            'agent_detected': self.agent_detected,
            'agent_position': self.agent_position,
            'agent_confidence': self.agent_confidence,
            'entities_count': len(self.entities),
            'spatial_relations_count': len(self.spatial_relations),
            'transition_valid': self.transition_valid,
            'integrity_checks': self.integrity_checks,
            'errors': self.error_log,
            'is_valid': self.is_valid
        }


@dataclass
class PerceptionDelta:
    """
    Différence perceptible entre deux PerceptionFrames.
    
    Mesure:
    - Changement spatial (motion)
    - Changement topologique (hash)
    - Changement entités (détection, position)
    - Causalité (action → changement)
    """
    
    # ========== HASHES ==========
    hash_before: Optional[str] = None
    hash_after: Optional[str] = None
    hash_changed: bool = False
    
    # ========== SPATIAL ==========
    delta_spatial: float = 0.0
    """Distance L2 entre grids normalisés"""
    
    motion_detected: bool = False
    """Motion > seuil (0.01)"""
    
    # ========== TOPOLOGICAL ==========
    delta_topological: float = 0.0
    """Différence topologique (Hamming distance hashes)"""
    
    topology_changed: bool = False
    """Topologie a évolué"""
    
    # ========== ENTITIES ==========
    delta_entities: int = 0
    """Différence nombre entités"""
    
    entities_moved: bool = False
    """Au moins une entité a bougé"""
    
    agent_appeared: bool = False
    """Agent détecté après action (pas avant)"""
    
    agent_disappeared: bool = False
    """Agent perdu après action"""
    
    # ========== CAUSAL ==========
    delta_causal: float = 0.0
    """Score causalité action→changement"""
    
    causality_detected: bool = False
    """Causalité établie"""
    
    # ========== METADATA ==========
    action_context: Optional[str] = None
    """Action qui a produit ce delta"""
    
    duration_ns: int = 0
    """Durée transition (ns)"""
    
    # ========== COMPUTED ==========
    
    @property
    def has_change(self) -> bool:
        """Changement détecté (spatial OU topologique OU entités)."""
        return (
            self.motion_detected or
            self.topology_changed or
            self.entities_moved or
            self.agent_appeared or
            self.agent_disappeared
        )
    
    @property
    def has_causality(self) -> bool:
        """Causalité établie (action → changement)."""
        return self.causality_detected and self.action_context is not None
    
    @property
    def is_real_perception(self) -> bool:
        """
        Perception réelle = Observation + Changement + Causalité.
        
        Returns:
            real: True si perception réelle (pas illusion)
        """
        return self.has_change and self.has_causality
    
    # ========== FACTORY ==========
    
    @classmethod
    def compute(
        cls,
        frame_before: PerceptionFrame,
        frame_after: PerceptionFrame
    ) -> 'PerceptionDelta':
        """
        Calculer delta entre deux frames.
        
        Args:
            frame_before: Frame avant action
            frame_after: Frame après action
        
        Returns:
            delta: PerceptionDelta
        """
        delta = cls(
            action_context=frame_after.action_context,
            duration_ns=frame_after.timestamp_ns - frame_before.timestamp_ns
        )
        
        # Hashes
        delta.hash_before = frame_before.topological_hash
        delta.hash_after = frame_after.topological_hash
        delta.hash_changed = (
            delta.hash_before is not None and
            delta.hash_after is not None and
            delta.hash_before != delta.hash_after
        )
        
        # Spatial
        if frame_before.raw_grid is not None and frame_after.raw_grid is not None:
            if frame_before.raw_grid.shape == frame_after.raw_grid.shape:
                diff = frame_after.raw_grid.astype(float) - frame_before.raw_grid.astype(float)
                delta.delta_spatial = float(np.linalg.norm(diff))
                delta.motion_detected = delta.delta_spatial > 0.01
        
        # Topological
        if delta.hash_before and delta.hash_after:
            # Hamming distance (approximation)
            delta.delta_topological = sum(
                c1 != c2 for c1, c2 in zip(delta.hash_before, delta.hash_after)
            ) / len(delta.hash_before)
            delta.topology_changed = delta.delta_topological > 0.0
        
        # Entities
        delta.delta_entities = len(frame_after.entities) - len(frame_before.entities)
        delta.entities_moved = delta.delta_entities != 0  # Simplification
        
        # Agent
        delta.agent_appeared = (not frame_before.agent_detected) and frame_after.agent_detected
        delta.agent_disappeared = frame_before.agent_detected and (not frame_after.agent_detected)
        
        # Causalité (score simple)
        if delta.has_change and frame_after.action_context:
            delta.delta_causal = 1.0  # Simplification: action + changement = causalité
            delta.causality_detected = True
        
        return delta
    
    # ========== FORENSIC ==========
    
    def to_forensic_dict(self) -> Dict[str, Any]:
        """Export forensic."""
        return {
            'action_context': self.action_context,
            'duration_ns': self.duration_ns,
            'hash_before': self.hash_before,
            'hash_after': self.hash_after,
            'hash_changed': self.hash_changed,
            'delta_spatial': self.delta_spatial,
            'motion_detected': self.motion_detected,
            'delta_topological': self.delta_topological,
            'topology_changed': self.topology_changed,
            'delta_entities': self.delta_entities,
            'entities_moved': self.entities_moved,
            'agent_appeared': self.agent_appeared,
            'agent_disappeared': self.agent_disappeared,
            'delta_causal': self.delta_causal,
            'causality_detected': self.causality_detected,
            'has_change': self.has_change,
            'has_causality': self.has_causality,
            'is_real_perception': self.is_real_perception
        }


# ========== TESTS UNITAIRES ==========

if __name__ == "__main__":
    print("="*80)
    print("TESTS UNITAIRES PerceptionFrame")
    print("="*80)
    
    # Test 1: Création depuis ndarray
    print("\n[TEST 1] Création depuis ndarray")
    grid = np.random.randint(0, 10, (64, 64), dtype=np.uint8)
    frame = PerceptionFrame(raw_grid=grid, source="test")
    frame.validate()
    print(f"  Grid shape: {frame.grid_shape}")
    print(f"  Is valid: {frame.is_valid}")
    print(f"  Checks: {frame.integrity_checks}")
    
    # Test 2: Création depuis observation
    print("\n[TEST 2] Création depuis observation mock")
    class MockObs:
        def __init__(self):
            self.grid = np.random.randint(0, 10, (32, 32), dtype=np.uint8)
            self.state = "NOT_FINISHED"
    
    obs = MockObs()
    frame2 = PerceptionFrame.from_observation(obs, source="initial")
    frame2.validate()
    print(f"  Grid shape: {frame2.grid_shape}")
    print(f"  Is valid: {frame2.is_valid}")
    print(f"  Errors: {frame2.error_log}")
    
    # Test 3: Calcul delta
    print("\n[TEST 3] Calcul delta")
    grid_before = np.zeros((10, 10), dtype=np.uint8)
    grid_after = np.ones((10, 10), dtype=np.uint8)
    
    frame_before = PerceptionFrame(
        raw_grid=grid_before,
        topological_hash="abc123",
        source="initial"
    )
    
    frame_after = PerceptionFrame(
        raw_grid=grid_after,
        topological_hash="def456",
        source="after_action",
        action_context="ACTION1"
    )
    
    delta = frame_after.compute_delta(frame_before)
    print(f"  Hash changed: {delta.hash_changed}")
    print(f"  Motion detected: {delta.motion_detected}")
    print(f"  Delta spatial: {delta.delta_spatial:.4f}")
    print(f"  Has change: {delta.has_change}")
    print(f"  Has causality: {delta.has_causality}")
    print(f"  Is real perception: {delta.is_real_perception}")
    
    print("\n✅ Tests unitaires complétés")

# Made with Bob
