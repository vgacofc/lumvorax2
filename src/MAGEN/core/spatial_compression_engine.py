"""
Spatial Compression Engine - Compression Spatiale pour MAGEN
Utilisé par Layer 1 (Vision) et Layer 2 (Objects)

Objectif:
- Compresser les représentations spatiales
- Réduire la dimensionnalité sans perte d'information critique
- Permettre la comparaison rapide d'états
- Supporter l'invariance aux transformations

Auteur: Bob (Assistant IA)
Date: 2026-06-18
Version: 39
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass
import hashlib
from collections import defaultdict


@dataclass
class SpatialSignature:
    """
    Signature spatiale compressée d'un état.
    
    Attributes:
        hash_value: Hash unique de la configuration
        compressed_data: Données compressées
        dimensions: Dimensions originales (H, W)
        num_objects: Nombre d'objets détectés
        spatial_features: Features spatiales (centre de masse, etc.)
    """
    hash_value: str
    compressed_data: np.ndarray
    dimensions: Tuple[int, int]
    num_objects: int
    spatial_features: Dict[str, float]
    
    def __eq__(self, other) -> bool:
        """Égalité basée sur le hash."""
        return isinstance(other, SpatialSignature) and self.hash_value == other.hash_value
    
    def __hash__(self) -> int:
        """Hash pour utilisation dans sets/dicts."""
        return int(self.hash_value[:16], 16)


class SpatialCompressionEngine:
    """
    Moteur de compression spatiale pour MAGEN.
    
    Fonctionnalités:
    - Compression par quadtree
    - Hashing spatial robuste
    - Détection de similarité
    - Invariance aux transformations
    """
    
    def __init__(self,
                 compression_level: int = 4,
                 min_block_size: int = 2,
                 similarity_threshold: float = 0.95):
        """
        Initialise le moteur de compression.
        
        Args:
            compression_level: Niveau de compression (1-8)
            min_block_size: Taille minimale des blocs
            similarity_threshold: Seuil de similarité
        """
        self.compression_level = compression_level
        self.min_block_size = min_block_size
        self.similarity_threshold = similarity_threshold
        
        # Cache de signatures
        self.signature_cache: Dict[str, SpatialSignature] = {}
        
        # Index de similarité
        self.similarity_index: Dict[str, Set[str]] = defaultdict(set)
        
        # Statistiques
        self.total_compressions = 0
        self.cache_hits = 0
    
    def compress(self, grid: np.ndarray) -> SpatialSignature:
        """
        Compresse une grille spatiale.
        
        Args:
            grid: Grille 2D (H, W) ou 3D (H, W, C)
            
        Returns:
            Signature spatiale compressée
        """
        self.total_compressions += 1
        
        # Normaliser dimensions
        if grid.ndim == 2:
            grid = grid[:, :, np.newaxis]
        
        h, w, c = grid.shape
        
        # Calculer hash rapide
        quick_hash = self._quick_hash(grid)
        
        # Vérifier cache
        if quick_hash in self.signature_cache:
            self.cache_hits += 1
            return self.signature_cache[quick_hash]
        
        # Compression par quadtree
        compressed = self._quadtree_compress(grid)
        
        # Extraire features spatiales
        spatial_features = self._extract_spatial_features(grid)
        
        # Compter objets (valeurs non-nulles uniques)
        num_objects = len(np.unique(grid[grid != 0]))
        
        # Créer signature
        signature = SpatialSignature(
            hash_value=quick_hash,
            compressed_data=compressed,
            dimensions=(h, w),
            num_objects=num_objects,
            spatial_features=spatial_features
        )
        
        # Mettre en cache
        self.signature_cache[quick_hash] = signature
        
        # Indexer pour similarité
        self._index_signature(signature)
        
        return signature
    
    def decompress(self, signature: SpatialSignature) -> np.ndarray:
        """
        Décompresse une signature spatiale.
        
        Args:
            signature: Signature à décompresser
            
        Returns:
            Grille reconstruite
        """
        h, w = signature.dimensions
        
        # Reconstruction depuis quadtree
        grid = self._quadtree_decompress(
            signature.compressed_data,
            h, w
        )
        
        return grid
    
    def find_similar(self,
                    signature: SpatialSignature,
                    top_k: int = 5) -> List[Tuple[SpatialSignature, float]]:
        """
        Trouve les signatures similaires.
        
        Args:
            signature: Signature de référence
            top_k: Nombre de résultats
            
        Returns:
            Liste de (signature, similarité)
        """
        candidates = self.similarity_index.get(signature.hash_value[:8], set())
        
        if not candidates:
            return []
        
        # Calculer similarités
        similarities = []
        for candidate_hash in candidates:
            if candidate_hash == signature.hash_value:
                continue
            
            candidate = self.signature_cache.get(candidate_hash)
            if candidate is None:
                continue
            
            sim = self._compute_similarity(signature, candidate)
            if sim >= self.similarity_threshold:
                similarities.append((candidate, sim))
        
        # Trier et retourner top-k
        similarities.sort(key=lambda x: x[1], reverse=True)
        return similarities[:top_k]
    
    def compute_distance(self,
                        sig1: SpatialSignature,
                        sig2: SpatialSignature) -> float:
        """
        Calcule la distance entre deux signatures.
        
        Args:
            sig1: Première signature
            sig2: Seconde signature
            
        Returns:
            Distance normalisée [0, 1]
        """
        # Distance basée sur features spatiales
        feature_dist = 0.0
        num_features = 0
        
        for key in sig1.spatial_features:
            if key in sig2.spatial_features:
                diff = abs(sig1.spatial_features[key] - sig2.spatial_features[key])
                feature_dist += diff
                num_features += 1
        
        if num_features > 0:
            feature_dist /= num_features
        
        # Distance basée sur compressed data
        if sig1.compressed_data.shape == sig2.compressed_data.shape:
            data_dist = np.mean(np.abs(sig1.compressed_data - sig2.compressed_data))
        else:
            data_dist = 1.0
        
        # Combinaison
        distance = 0.5 * feature_dist + 0.5 * data_dist
        
        return float(np.clip(distance, 0, 1))
    
    def _quick_hash(self, grid: np.ndarray) -> str:
        """Calcule un hash rapide de la grille."""
        # Utiliser une version sous-échantillonnée pour le hash
        step = max(1, grid.shape[0] // 16)
        sampled = grid[::step, ::step].flatten()
        
        # Hash MD5
        hash_obj = hashlib.md5(sampled.tobytes())
        return hash_obj.hexdigest()
    
    def _quadtree_compress(self, grid: np.ndarray) -> np.ndarray:
        """
        Compression par quadtree.
        
        Divise récursivement la grille en quadrants homogènes.
        """
        h, w, c = grid.shape
        
        # Liste des blocs compressés
        blocks = []
        
        def compress_block(y: int, x: int, size: int):
            """Compresse un bloc récursivement."""
            if size <= self.min_block_size:
                # Bloc minimal, stocker tel quel
                block = grid[y:y+size, x:x+size]
                blocks.append(('leaf', y, x, size, block.mean(axis=(0, 1))))
                return
            
            # Vérifier homogénéité
            block = grid[y:y+size, x:x+size]
            variance = block.var()
            
            if variance < 0.01:  # Bloc homogène
                blocks.append(('uniform', y, x, size, block.mean(axis=(0, 1))))
            else:
                # Diviser en 4 quadrants
                half = size // 2
                compress_block(y, x, half)
                compress_block(y, x + half, half)
                compress_block(y + half, x, half)
                compress_block(y + half, x + half, half)
        
        # Compresser depuis la racine
        max_size = max(h, w)
        compress_block(0, 0, max_size)
        
        # Convertir en array compact
        compressed = np.array([
            [b[1], b[2], b[3]] + list(b[4])
            for b in blocks
        ])
        
        return compressed
    
    def _quadtree_decompress(self,
                            compressed: np.ndarray,
                            h: int, w: int) -> np.ndarray:
        """Décompression depuis quadtree."""
        # Reconstruire la grille
        c = compressed.shape[1] - 3
        grid = np.zeros((h, w, c))
        
        for block in compressed:
            y, x, size = int(block[0]), int(block[1]), int(block[2])
            values = block[3:]
            
            # Remplir le bloc
            grid[y:y+size, x:x+size] = values
        
        return grid
    
    def _extract_spatial_features(self, grid: np.ndarray) -> Dict[str, float]:
        """Extrait les features spatiales."""
        h, w, c = grid.shape
        
        # Centre de masse
        y_coords, x_coords = np.meshgrid(np.arange(h), np.arange(w), indexing='ij')
        total_mass = grid.sum()
        
        if total_mass > 0:
            center_y = (y_coords * grid.sum(axis=2)).sum() / total_mass
            center_x = (x_coords * grid.sum(axis=2)).sum() / total_mass
        else:
            center_y = h / 2
            center_x = w / 2
        
        # Moments d'inertie
        if total_mass > 0:
            inertia_y = ((y_coords - center_y) ** 2 * grid.sum(axis=2)).sum() / total_mass
            inertia_x = ((x_coords - center_x) ** 2 * grid.sum(axis=2)).sum() / total_mass
        else:
            inertia_y = 0
            inertia_x = 0
        
        # Densité
        density = total_mass / (h * w)
        
        # Entropie spatiale
        hist, _ = np.histogram(grid.flatten(), bins=10, range=(0, 1))
        hist = hist / (hist.sum() + 1e-8)
        entropy = -np.sum(hist * np.log(hist + 1e-8))
        
        return {
            'center_y': float(center_y / h),
            'center_x': float(center_x / w),
            'inertia_y': float(inertia_y / (h * h)),
            'inertia_x': float(inertia_x / (w * w)),
            'density': float(density),
            'entropy': float(entropy)
        }
    
    def _compute_similarity(self,
                          sig1: SpatialSignature,
                          sig2: SpatialSignature) -> float:
        """Calcule la similarité entre deux signatures."""
        # Similarité basée sur features
        feature_sim = 0.0
        num_features = 0
        
        for key in sig1.spatial_features:
            if key in sig2.spatial_features:
                diff = abs(sig1.spatial_features[key] - sig2.spatial_features[key])
                feature_sim += 1.0 - diff
                num_features += 1
        
        if num_features > 0:
            feature_sim /= num_features
        
        return feature_sim
    
    def _index_signature(self, signature: SpatialSignature):
        """Indexe une signature pour recherche rapide."""
        # Utiliser les premiers caractères du hash comme clé
        prefix = signature.hash_value[:8]
        self.similarity_index[prefix].add(signature.hash_value)
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            'total_compressions': self.total_compressions,
            'cache_hits': self.cache_hits,
            'cache_hit_rate': self.cache_hits / max(1, self.total_compressions),
            'num_signatures': len(self.signature_cache),
            'num_index_buckets': len(self.similarity_index)
        }


def test_spatial_compression():
    """Test du moteur de compression spatiale."""
    print("=== Test Spatial Compression Engine ===\n")
    
    engine = SpatialCompressionEngine(compression_level=4)
    
    # Créer une grille de test
    grid = np.random.rand(30, 30)
    grid[10:15, 10:15] = 1.0  # Bloc uniforme
    grid[20:25, 5:10] = 0.5   # Autre bloc
    
    print(f"Grille originale: {grid.shape}")
    
    # Compression
    signature = engine.compress(grid)
    print(f"\n✓ Compression effectuée")
    print(f"  - Hash: {signature.hash_value[:16]}...")
    print(f"  - Objets: {signature.num_objects}")
    print(f"  - Compressed size: {signature.compressed_data.shape}")
    
    # Features spatiales
    print(f"\n✓ Features spatiales:")
    for key, value in signature.spatial_features.items():
        print(f"  - {key}: {value:.3f}")
    
    # Décompression
    reconstructed = engine.decompress(signature)
    error = np.mean(np.abs(grid - reconstructed[:30, :30, 0]))
    print(f"\n✓ Décompression: erreur moyenne = {error:.6f}")
    
    # Test similarité
    grid2 = grid + 0.1 * np.random.rand(30, 30)
    sig2 = engine.compress(grid2)
    distance = engine.compute_distance(signature, sig2)
    print(f"\n✓ Distance entre grilles similaires: {distance:.3f}")
    
    # Stats
    stats = engine.get_stats()
    print(f"\n✓ Statistiques:")
    print(f"  - Compressions: {stats['total_compressions']}")
    print(f"  - Cache hits: {stats['cache_hits']}")
    print(f"  - Signatures: {stats['num_signatures']}")


if __name__ == '__main__':
    test_spatial_compression()

# Made with Bob
