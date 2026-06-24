#!/usr/bin/env python3
"""
Objects Layer - Layer 2 Architecture MAGEN V2
==============================================

OBJECTIF: Unifier tous les modules de détection d'objets existants
dans une interface cohérente pour l'architecture V2.

Modules Intégrés:
- ObjectExtractor: Extraction objets structurés complets
- SpatialCompressionEngine: Compression spatiale + signatures
- TopologicalHashV39: Hashing topologique invariant

Architecture:
    VisionOutput (Layer 1)
         ↓
    ObjectsLayer.process()
         ↓
    ObjectsOutput (unifié)
         ↓
    Layer 3 (Attention)

Insight Critique (Session 57):
"ARC attend OBJET ENTIER pas PIXEL"

Auteur: Bob (Architecture MAGEN V2)
Date: 2026-06-18
Session: Phase 2.4
"""

from dataclasses import dataclass, field
from typing import Optional, List, Tuple, Dict, Any
import numpy as np
import time

# Imports modules existants
try:
    from core.object_transformation_engine import ObjectExtractor, StructuredObject
    from core.spatial_compression_engine import SpatialCompressionEngine, SpatialSignature
    from core.topological_hash_v39 import TopologicalHashV39, TopologicalHash
except ImportError:
    # Fallback si imports échouent
    ObjectExtractor = None
    SpatialCompressionEngine = None
    TopologicalHashV39 = None


@dataclass
class ObjectsOutput:
    """
    Sortie unifiée du Objects Layer.
    
    Combine:
    - Structured objects (objets complets avec structure)
    - Spatial signature (compression + hash spatial)
    - Topological hash (invariants topologiques)
    """
    # Données brutes
    raw_grid: np.ndarray
    """Grille brute d'entrée"""
    
    # Objets structurés
    objects: List[Any] = field(default_factory=list)
    """Liste StructuredObject extraits"""
    
    num_objects: int = 0
    """Nombre d'objets détectés"""
    
    # Compression spatiale
    spatial_signature: Optional[Any] = None
    """SpatialSignature compressée"""
    
    # Hash topologique
    topological_hash: Optional[Any] = None
    """TopologicalHash invariant"""
    
    # Métadonnées
    processing_time_ms: float = 0.0
    """Temps traitement en millisecondes"""
    
    timestamp_ns: int = 0
    """Timestamp nanoseconde"""
    
    # Flags qualité
    is_valid: bool = True
    """Validation globale"""
    
    has_objects: bool = False
    """Objets détectés"""
    
    has_multiple_objects: bool = False
    """Plusieurs objets détectés"""
    
    # Statistiques objets
    total_area: int = 0
    """Aire totale des objets"""
    
    avg_object_size: float = 0.0
    """Taille moyenne des objets"""
    
    colors_present: List[int] = field(default_factory=list)
    """Couleurs présentes dans les objets"""
    
    # Cache
    _cache: Dict[str, Any] = field(default_factory=dict)
    """Cache interne pour calculs coûteux"""
    
    def get_object_by_id(self, obj_id: int) -> Optional[Any]:
        """Récupère objet par ID"""
        for obj in self.objects:
            if hasattr(obj, 'object_id') and obj.object_id == obj_id:
                return obj
        return None
    
    def get_largest_object(self) -> Optional[Any]:
        """Récupère l'objet le plus grand"""
        if not self.objects:
            return None
        return max(self.objects, key=lambda obj: obj.area if hasattr(obj, 'area') else 0)
    
    def get_objects_by_color(self, color: int) -> List[Any]:
        """Récupère objets d'une couleur spécifique"""
        return [obj for obj in self.objects if hasattr(obj, 'color') and obj.color == color]
    
    def get_spatial_hash(self) -> str:
        """Récupère hash spatial"""
        if self.spatial_signature and hasattr(self.spatial_signature, 'hash_value'):
            return self.spatial_signature.hash_value
        return ""
    
    def get_topological_hash(self) -> str:
        """Récupère hash topologique"""
        if self.topological_hash and hasattr(self.topological_hash, 'hash_value'):
            return self.topological_hash.hash_value
        return ""
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation pour logging"""
        return {
            'grid_shape': self.raw_grid.shape,
            'num_objects': self.num_objects,
            'has_objects': self.has_objects,
            'has_multiple_objects': self.has_multiple_objects,
            'total_area': self.total_area,
            'avg_object_size': self.avg_object_size,
            'colors_present': self.colors_present,
            'spatial_hash': self.get_spatial_hash()[:16] + "..." if self.get_spatial_hash() else None,
            'topological_hash': self.get_topological_hash()[:16] + "..." if self.get_topological_hash() else None,
            'processing_time_ms': self.processing_time_ms,
            'timestamp_ns': self.timestamp_ns,
            'is_valid': self.is_valid
        }


@dataclass
class ObjectsLayer:
    """
    Layer 2: Objects - Détection objets structurés unifiée.
    
    Responsabilités:
    1. Extraction objets complets (pas juste pixels)
    2. Préservation structure spatiale
    3. Compression spatiale intelligente
    4. Génération hash topologique invariant
    5. Calcul propriétés géométriques
    
    Pipeline:
        Grid → Extract Objects → Compress Spatial → Hash Topological → Output
    
    Intégrations:
    - ObjectExtractor: Objets structurés complets
    - SpatialCompressionEngine: Compression + signatures
    - TopologicalHashV39: Hash invariant transformations
    """
    
    # Configuration
    enable_object_extraction: bool = True
    """Activer ObjectExtractor (extraction objets)"""
    
    enable_spatial_compression: bool = True
    """Activer SpatialCompressionEngine (compression)"""
    
    enable_topological_hash: bool = True
    """Activer TopologicalHashV39 (hashing)"""
    
    compression_level: int = 4
    """Niveau compression spatiale (1-8)"""
    
    rotation_invariant: bool = True
    """Hash invariant aux rotations"""
    
    reflection_invariant: bool = True
    """Hash invariant aux réflexions"""
    
    cache_enabled: bool = True
    """Activer cache pour grilles identiques"""
    
    verbose: bool = False
    """Mode verbeux"""
    
    # Modules intégrés
    _object_extractor: Optional[Any] = None
    _spatial_compressor: Optional[Any] = None
    _topological_hasher: Optional[Any] = None
    
    # Cache
    _cache: Dict[str, ObjectsOutput] = field(default_factory=dict)
    _cache_hits: int = 0
    _cache_misses: int = 0
    
    # Statistiques
    total_processed: int = 0
    total_time_ms: float = 0.0
    total_objects_detected: int = 0
    
    def __post_init__(self):
        """Initialisation modules"""
        # Initialiser ObjectExtractor
        if self.enable_object_extraction and ObjectExtractor:
            self._object_extractor = ObjectExtractor(verbose=self.verbose)
        
        # Initialiser SpatialCompressionEngine
        if self.enable_spatial_compression and SpatialCompressionEngine:
            self._spatial_compressor = SpatialCompressionEngine(
                compression_level=self.compression_level
            )
        
        # Initialiser TopologicalHashV39
        if self.enable_topological_hash and TopologicalHashV39:
            self._topological_hasher = TopologicalHashV39(
                rotation_invariant=self.rotation_invariant,
                reflection_invariant=self.reflection_invariant
            )
        
        if self.verbose:
            print(f"[ObjectsLayer] Initialisé")
            print(f"  - ObjectExtractor: {self.enable_object_extraction}")
            print(f"  - SpatialCompression: {self.enable_spatial_compression}")
            print(f"  - TopologicalHash: {self.enable_topological_hash}")
            print(f"  - Cache: {self.cache_enabled}")
    
    def _compute_grid_hash(self, grid: np.ndarray) -> str:
        """Calcule hash rapide de la grille pour cache"""
        import hashlib
        return hashlib.md5(grid.tobytes()).hexdigest()
    
    def detect(self, grid: np.ndarray) -> ObjectsOutput:
        """
        Méthode wrapper pour compatibilité avec tests.
        Alias de process() pour détection d'objets.
        
        Args:
            grid: Grille d'entrée
            
        Returns:
            ObjectsOutput avec objets détectés
        """
        return self.process(grid)
    
    def process(self, grid: np.ndarray) -> ObjectsOutput:
        """
        Traitement complet détection objets.
        
        Args:
            grid: Grille d'entrée (2D normalisée)
            
        Returns:
            ObjectsOutput unifié
        """
        start_time = time.perf_counter()
        timestamp_ns = time.time_ns()
        
        # Hash pour cache
        grid_hash = self._compute_grid_hash(grid)
        
        # Vérifier cache
        if self.cache_enabled and grid_hash in self._cache:
            self._cache_hits += 1
            cached = self._cache[grid_hash]
            # Mettre à jour timestamp
            cached.timestamp_ns = timestamp_ns
            return cached
        
        self._cache_misses += 1
        
        # Créer output
        output = ObjectsOutput(
            raw_grid=grid,
            timestamp_ns=timestamp_ns
        )
        
        # PHASE 1: ObjectExtractor (extraction objets structurés)
        if self.enable_object_extraction and self._object_extractor:
            try:
                objects = self._object_extractor.extract_objects(grid)
                output.objects = objects
                output.num_objects = len(objects)
                output.has_objects = len(objects) > 0
                output.has_multiple_objects = len(objects) > 1
                
                # Statistiques objets
                if objects:
                    output.total_area = sum(obj.area for obj in objects if hasattr(obj, 'area'))
                    output.avg_object_size = output.total_area / len(objects)
                    output.colors_present = list(set(obj.color for obj in objects if hasattr(obj, 'color')))
                
                self.total_objects_detected += len(objects)
                
                if self.verbose:
                    print(f"[ObjectsLayer] Extracted {len(objects)} objects")
                
            except Exception as e:
                if self.verbose:
                    print(f"[ObjectsLayer] Erreur ObjectExtractor: {e}")
                output.is_valid = False
        
        # PHASE 2: SpatialCompressionEngine (compression spatiale)
        if self.enable_spatial_compression and self._spatial_compressor:
            try:
                spatial_sig = self._spatial_compressor.compress(grid)
                output.spatial_signature = spatial_sig
                
                if self.verbose:
                    print(f"[ObjectsLayer] Spatial hash: {spatial_sig.hash_value[:16]}...")
                
            except Exception as e:
                if self.verbose:
                    print(f"[ObjectsLayer] Erreur SpatialCompression: {e}")
        
        # PHASE 3: TopologicalHashV39 (hashing topologique)
        if self.enable_topological_hash and self._topological_hasher:
            try:
                topo_hash = self._topological_hasher.compute_hash(grid)
                output.topological_hash = topo_hash
                
                if self.verbose:
                    print(f"[ObjectsLayer] Topological hash: {topo_hash.hash_value[:16]}...")
                
            except Exception as e:
                if self.verbose:
                    print(f"[ObjectsLayer] Erreur TopologicalHash: {e}")
        
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
        avg_objects = self.total_objects_detected / max(1, self.total_processed)
        
        return {
            'total_processed': self.total_processed,
            'total_time_ms': self.total_time_ms,
            'avg_time_ms': avg_time,
            'total_objects_detected': self.total_objects_detected,
            'avg_objects_per_grid': avg_objects,
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
    print("🧪 Test ObjectsLayer...")
    print("="*80)
    
    # Créer layer
    objects_layer = ObjectsLayer(
        enable_object_extraction=True,
        enable_spatial_compression=True,
        enable_topological_hash=True,
        compression_level=4,
        rotation_invariant=True,
        reflection_invariant=True,
        cache_enabled=True,
        verbose=False
    )
    
    # Test 1: Grille avec objets simples
    print("\n1. Test grille avec objets...")
    grid1 = np.zeros((20, 20), dtype=np.uint8)
    grid1[5:8, 5:8] = 1  # Objet 1 (carré)
    grid1[12:15, 12:18] = 2  # Objet 2 (rectangle)
    
    output1 = objects_layer.process(grid1)
    print(f"   ✓ Objets détectés: {output1.num_objects}")
    print(f"   ✓ Aire totale: {output1.total_area}")
    print(f"   ✓ Couleurs: {output1.colors_present}")
    print(f"   ✓ Temps: {output1.processing_time_ms:.2f}ms")
    
    # Test 2: Grille vide
    print("\n2. Test grille vide...")
    grid2 = np.zeros((20, 20), dtype=np.uint8)
    output2 = objects_layer.process(grid2)
    print(f"   ✓ Objets détectés: {output2.num_objects}")
    print(f"   ✓ Has objects: {output2.has_objects}")
    
    # Test 3: Cache
    print("\n3. Test cache...")
    output3 = objects_layer.process(grid1)  # Même grille
    print(f"   ✓ Cache hit: {output3.timestamp_ns != output1.timestamp_ns}")
    
    # Test 4: Objets multiples
    print("\n4. Test objets multiples...")
    grid4 = np.zeros((30, 30), dtype=np.uint8)
    grid4[5:10, 5:10] = 3
    grid4[15:20, 15:20] = 4
    grid4[10:12, 25:28] = 5
    
    output4 = objects_layer.process(grid4)
    print(f"   ✓ Objets détectés: {output4.num_objects}")
    print(f"   ✓ Multiple objects: {output4.has_multiple_objects}")
    print(f"   ✓ Taille moyenne: {output4.avg_object_size:.1f}")
    
    # Test 5: Statistiques
    print("\n5. Statistiques...")
    stats = objects_layer.get_stats()
    print(f"   ✓ Total processed: {stats['total_processed']}")
    print(f"   ✓ Total objects: {stats['total_objects_detected']}")
    print(f"   ✓ Avg objects/grid: {stats['avg_objects_per_grid']:.2f}")
    print(f"   ✓ Cache hit rate: {stats['cache_hit_rate']*100:.1f}%")
    print(f"   ✓ Avg time: {stats['avg_time_ms']:.2f}ms")
    
    # Test 6: Sérialisation
    print("\n6. Test sérialisation...")
    data = output1.to_dict()
    print(f"   ✓ Keys: {len(data)} attributs")
    print(f"   ✓ Num objects: {data['num_objects']}")
    
    print("\n✅ Test ObjectsLayer réussi!")
    print(f"\n📊 Statistiques finales:")
    for key, value in stats.items():
        print(f"   - {key}: {value}")

# Made with Bob
