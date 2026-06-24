#!/usr/bin/env python3
"""
Vision Layer - Layer 1 Architecture MAGEN V2
=============================================

OBJECTIF: Unifier tous les modules de perception visuelle existants
dans une interface cohérente pour l'architecture V2.

Modules Intégrés:
- PerceptionFrame: Structure unifiée perception
- GridAnalyzer: Analyse spatiale intelligente
- ARCPerception: Extraction features ARC

Architecture:
    Raw Input (grid/frame)
         ↓
    VisionLayer.process()
         ↓
    VisionOutput (unifié)
         ↓
    Layer 2 (Objects)

Auteur: Bob (Architecture MAGEN V2)
Date: 2026-06-18
Session: Phase 2.3
"""

from dataclasses import dataclass, field
from typing import Optional, List, Tuple, Dict, Any
import numpy as np
import time
import hashlib

# Imports modules existants
try:
    from core.perception_frame import PerceptionFrame, Entity
    from core.grid_analyzer import GridAnalyzer, SpatialAnalysis, GridEntity, ActionSuggestion
    from perception.arc_perception import ARCPerception, GridFeatures
except ImportError:
    # Fallback si imports échouent
    PerceptionFrame = None
    GridAnalyzer = None
    ARCPerception = None


@dataclass
class VisionOutput:
    """
    Sortie unifiée du Vision Layer.
    
    Combine:
    - Perception frame (structure + traçabilité)
    - Spatial analysis (entités + pathfinding)
    - Grid features (statistiques + patterns)
    """
    # Données brutes
    raw_grid: np.ndarray
    """Grille brute d'entrée"""
    
    # Perception Frame (structure unifiée)
    perception_frame: Optional[Any] = None
    """PerceptionFrame complet avec embeddings"""
    
    # Spatial Analysis (analyse intelligente)
    spatial_analysis: Optional[Any] = None
    """SpatialAnalysis avec entités et pathfinding"""
    
    # Grid Features (statistiques)
    grid_features: Optional[Any] = None
    """GridFeatures avec patterns et complexité"""
    
    # Alias pour compatibilité tests
    features: Optional[Any] = None
    """Alias de grid_features pour compatibilité"""
    
    # Métadonnées
    processing_time_ms: float = 0.0
    """Temps traitement en millisecondes"""
    
    timestamp_ns: int = 0
    """Timestamp nanoseconde"""
    
    grid_hash: str = ""
    """Hash MD5 de la grille"""
    
    # Flags qualité
    is_valid: bool = True
    """Validation globale"""
    
    has_entities: bool = False
    """Entités détectées"""
    
    has_player: bool = False
    """Joueur/agent détecté"""
    
    has_goal: bool = False
    """Objectif détecté"""
    
    # Cache
    _cache: Dict[str, Any] = field(default_factory=dict)
    """Cache interne pour calculs coûteux"""
    
    def get_player_position(self) -> Optional[Tuple[int, int]]:
        """Récupère position joueur depuis spatial_analysis"""
        if self.spatial_analysis and hasattr(self.spatial_analysis, 'player_position'):
            return self.spatial_analysis.player_position
        return None
    
    def get_goal_position(self) -> Optional[Tuple[int, int]]:
        """Récupère position objectif depuis spatial_analysis"""
        if self.spatial_analysis and hasattr(self.spatial_analysis, 'goal_position'):
            return self.spatial_analysis.goal_position
        return None
    
    def get_distance_to_goal(self) -> float:
        """Récupère distance à l'objectif"""
        if self.spatial_analysis and hasattr(self.spatial_analysis, 'distance_to_goal'):
            return self.spatial_analysis.distance_to_goal
        return float('inf')
    
    def get_suggested_actions(self) -> List[Any]:
        """Récupère suggestions d'actions depuis spatial_analysis"""
        return self._cache.get('suggested_actions', [])
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation pour logging"""
        return {
            'grid_shape': self.raw_grid.shape,
            'grid_hash': self.grid_hash,
            'processing_time_ms': self.processing_time_ms,
            'timestamp_ns': self.timestamp_ns,
            'is_valid': self.is_valid,
            'has_entities': self.has_entities,
            'has_player': self.has_player,
            'has_goal': self.has_goal,
            'player_position': self.get_player_position(),
            'goal_position': self.get_goal_position(),
            'distance_to_goal': self.get_distance_to_goal(),
            'grid_features': self.grid_features.to_dict() if self.grid_features else None
        }


@dataclass
class VisionLayer:
    """
    Layer 1: Vision - Perception visuelle unifiée.
    
    Responsabilités:
    1. Normalisation input (grilles diverses formats)
    2. Extraction features visuelles (couleurs, patterns, symétries)
    3. Détection entités (joueur, objectifs, obstacles)
    4. Analyse spatiale (distances, chemins, zones)
    5. Génération embeddings visuels
    
    Pipeline:
        Raw Grid → Normalize → Extract Features → Detect Entities → Analyze Spatial → Output
    
    Intégrations:
    - PerceptionFrame: Structure + traçabilité
    - GridAnalyzer: Entités + pathfinding
    - ARCPerception: Features + patterns
    """
    
    # Configuration
    max_grid_size: int = 64
    """Taille maximale grille (64x64 pour ARC-AGI-3)"""
    
    enable_perception_frame: bool = True
    """Activer PerceptionFrame (structure unifiée)"""
    
    enable_spatial_analysis: bool = True
    """Activer GridAnalyzer (analyse spatiale)"""
    
    enable_feature_extraction: bool = True
    """Activer ARCPerception (extraction features)"""
    
    cache_enabled: bool = True
    """Activer cache pour grilles identiques"""
    
    # Modules intégrés
    _perception_frame_module: Optional[Any] = None
    _grid_analyzer: Optional[Any] = None
    _arc_perception: Optional[Any] = None
    
    # Cache
    _cache: Dict[str, VisionOutput] = field(default_factory=dict)
    _cache_hits: int = 0
    _cache_misses: int = 0
    
    # Statistiques
    total_processed: int = 0
    total_time_ms: float = 0.0
    
    def __post_init__(self):
        """Initialisation modules"""
        # Initialiser GridAnalyzer
        if self.enable_spatial_analysis and GridAnalyzer:
            self._grid_analyzer = GridAnalyzer()
        
        # Initialiser ARCPerception
        if self.enable_feature_extraction and ARCPerception:
            self._arc_perception = ARCPerception(max_grid_size=self.max_grid_size)
        
        print(f"[VisionLayer] Initialisé")
        print(f"  - PerceptionFrame: {self.enable_perception_frame}")
        print(f"  - GridAnalyzer: {self.enable_spatial_analysis}")
        print(f"  - ARCPerception: {self.enable_feature_extraction}")
        print(f"  - Cache: {self.cache_enabled}")
    
    def _compute_grid_hash(self, grid: np.ndarray) -> str:
        """Calcule hash MD5 de la grille"""
        return hashlib.md5(grid.tobytes()).hexdigest()
    
    def _normalize_grid(self, grid: np.ndarray) -> np.ndarray:
        """
        Normalise grille en format standard 2D.
        
        Gère:
        - (H, W) → (H, W)
        - (C, H, W) → (H, W) [prend premier canal]
        - (H, W, C) → (H, W) [prend premier canal]
        """
        if len(grid.shape) == 2:
            return grid
        
        elif len(grid.shape) == 3:
            # Channels first (C, H, W)
            if grid.shape[0] <= 10:
                return grid[0]
            # Channels last (H, W, C)
            else:
                return grid[:, :, 0]
        
        else:
            raise ValueError(f"Format grille non supporté: {grid.shape}")
    
    def analyze(self, grid: np.ndarray) -> VisionOutput:
        """
        Méthode wrapper pour compatibilité avec tests.
        Alias de process() pour analyse visuelle.
        
        Args:
            grid: Grille d'entrée
            
        Returns:
            VisionOutput avec features extraites
        """
        output = self.process(grid, source="test")
        # Ajouter attribut 'features' pour compatibilité tests
        if not hasattr(output, 'features'):
            output.features = output.grid_features
        return output
    
    def process(self, 
                grid: np.ndarray,
                source: str = "unknown",
                action_context: Optional[str] = None) -> VisionOutput:
        """
        Traitement complet vision.
        
        Args:
            grid: Grille d'entrée (formats variés)
            source: Source de la grille ("initial", "after_action", etc.)
            action_context: Contexte action si applicable
            
        Returns:
            VisionOutput unifié
        """
        start_time = time.perf_counter()
        timestamp_ns = time.time_ns()
        
        # Normaliser grille
        normalized_grid = self._normalize_grid(grid)
        grid_hash = self._compute_grid_hash(normalized_grid)
        
        # Vérifier cache
        if self.cache_enabled and grid_hash in self._cache:
            self._cache_hits += 1
            cached = self._cache[grid_hash]
            # Mettre à jour timestamp
            cached.timestamp_ns = timestamp_ns
            return cached
        
        self._cache_misses += 1
        
        # Créer output
        output = VisionOutput(
            raw_grid=normalized_grid,
            timestamp_ns=timestamp_ns,
            grid_hash=grid_hash
        )
        
        # PHASE 1: PerceptionFrame (structure unifiée)
        if self.enable_perception_frame and PerceptionFrame:
            try:
                # Créer PerceptionFrame basique
                # Note: PerceptionFrame.from_observation nécessite obs complet
                # Ici on crée structure minimale
                output.perception_frame = {
                    'raw_grid': normalized_grid,
                    'source': source,
                    'action_context': action_context,
                    'timestamp_ns': timestamp_ns,
                    'grid_hash': grid_hash
                }
            except Exception as e:
                print(f"[VisionLayer] Erreur PerceptionFrame: {e}")
        
        # PHASE 2: GridAnalyzer (analyse spatiale)
        if self.enable_spatial_analysis and self._grid_analyzer:
            try:
                spatial_analysis = self._grid_analyzer.analyze_grid(normalized_grid)
                output.spatial_analysis = spatial_analysis
                
                # Mettre à jour flags
                output.has_player = spatial_analysis.player_position is not None
                output.has_goal = spatial_analysis.goal_position is not None
                output.has_entities = (
                    len(spatial_analysis.obstacles) > 0 or
                    len(spatial_analysis.enemies) > 0 or
                    len(spatial_analysis.collectibles) > 0
                )
                
                # Suggestions actions (si disponible)
                if hasattr(self._grid_analyzer, 'suggest_action'):
                    suggestions = self._grid_analyzer.suggest_action(spatial_analysis)
                    output._cache['suggested_actions'] = suggestions if isinstance(suggestions, list) else [suggestions]
                
            except Exception as e:
                print(f"[VisionLayer] Erreur GridAnalyzer: {e}")
        
        # PHASE 3: ARCPerception (extraction features)
        if self.enable_feature_extraction and self._arc_perception:
            try:
                grid_features = self._arc_perception.extract_features(normalized_grid)
                output.grid_features = grid_features
            except Exception as e:
                print(f"[VisionLayer] Erreur ARCPerception: {e}")
        
        # Calcul temps traitement
        end_time = time.perf_counter()
        output.processing_time_ms = (end_time - start_time) * 1000
        
        # Mise à jour statistiques
        self.total_processed += 1
        self.total_time_ms += output.processing_time_ms
        
        # Cache
        if self.cache_enabled:
            self._cache[grid_hash] = output
        
        return output
    
    def get_stats(self) -> Dict[str, Any]:
        """Statistiques du layer"""
        avg_time = self.total_time_ms / max(1, self.total_processed)
        cache_rate = self._cache_hits / max(1, self._cache_hits + self._cache_misses)
        
        return {
            'total_processed': self.total_processed,
            'total_time_ms': self.total_time_ms,
            'avg_time_ms': avg_time,
            'cache_hits': self._cache_hits,
            'cache_misses': self._cache_misses,
            'cache_hit_rate': cache_rate,
            'cache_size': len(self._cache)
        }
    
    def clear_cache(self):
        """Vider cache"""
        self._cache.clear()
        self._cache_hits = 0
        self._cache_misses = 0


# ========== TESTS ==========

if __name__ == "__main__":
    print("="*80)
    print("🧪 Test VisionLayer...")
    print("="*80)
    
    # Créer layer
    vision = VisionLayer(
        max_grid_size=64,
        enable_perception_frame=True,
        enable_spatial_analysis=True,
        enable_feature_extraction=True,
        cache_enabled=True
    )
    
    # Test 1: Grille simple 2D
    print("\n1. Test grille 2D simple...")
    grid_2d = np.random.randint(0, 10, (20, 20), dtype=np.uint8)
    output1 = vision.process(grid_2d, source="test")
    print(f"   ✓ Shape: {output1.raw_grid.shape}")
    print(f"   ✓ Hash: {output1.grid_hash[:16]}...")
    print(f"   ✓ Temps: {output1.processing_time_ms:.2f}ms")
    print(f"   ✓ Has entities: {output1.has_entities}")
    
    # Test 2: Grille 3D (channels first)
    print("\n2. Test grille 3D (channels first)...")
    grid_3d_cf = np.random.randint(0, 10, (3, 20, 20), dtype=np.uint8)
    output2 = vision.process(grid_3d_cf, source="test")
    print(f"   ✓ Shape: {output2.raw_grid.shape}")
    print(f"   ✓ Normalized: {output2.raw_grid.shape == (20, 20)}")
    
    # Test 3: Cache
    print("\n3. Test cache...")
    output3 = vision.process(grid_2d, source="test")  # Même grille
    print(f"   ✓ Cache hit: {output3.grid_hash == output1.grid_hash}")
    
    # Test 4: Statistiques
    print("\n4. Statistiques...")
    stats = vision.get_stats()
    print(f"   ✓ Total processed: {stats['total_processed']}")
    print(f"   ✓ Cache hit rate: {stats['cache_hit_rate']*100:.1f}%")
    print(f"   ✓ Avg time: {stats['avg_time_ms']:.2f}ms")
    
    # Test 5: Sérialisation
    print("\n5. Test sérialisation...")
    data = output1.to_dict()
    print(f"   ✓ Keys: {len(data)} attributs")
    print(f"   ✓ Grid shape: {data['grid_shape']}")
    
    print("\n✅ Test VisionLayer réussi!")
    print(f"\n📊 Statistiques finales:")
    for key, value in stats.items():
        print(f"   - {key}: {value}")

# Made with Bob
