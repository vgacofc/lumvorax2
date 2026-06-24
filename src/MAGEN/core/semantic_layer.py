"""
MAGEN Architecture V2 - Layer 6: Semantic Memory
=================================================

Wrapper d'intégration unifié pour la mémoire sémantique.
Intègre: SemanticMemoryV39

Auteur: Bob (Assistant IA)
Date: 2026-06-18
Version: V2.0 Architecture 10 couches

Architecture Pipeline:
    Input (concepts) → Semantic Memory → Retrieval → Output (similar concepts)

Modules Intégrés:
    1. SemanticMemoryV39 (semantic_memory_v39.py)
       - Stockage concepts avec embeddings
       - Recherche similarité sémantique
       - Clustering concepts
       - Statistiques mémoire

Design Pattern: Facade + Repository
Forensic: Logging complet intégré
"""

import time
import hashlib
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, field
from datetime import datetime
import numpy as np

# Import module existant
try:
    from .semantic_memory_v39 import SemanticMemoryV39, SemanticConcept
except ImportError:
    # Fallback pour tests standalone
    from semantic_memory_v39 import SemanticMemoryV39, SemanticConcept


@dataclass
class SemanticOutput:
    """
    Sortie unifiée de la couche Semantic Memory.
    
    Attributes:
        concept_stored: Concept stocké (si store)
        similar_concepts: Concepts similaires trouvés (si retrieve)
        similarity_scores: Scores de similarité
        clusters: Clusters de concepts
        memory_stats: Statistiques mémoire
        processing_time_ms: Temps de traitement
        cache_hit: Si résultat vient du cache
        timestamp: Timestamp de traitement
    """
    concept_stored: Optional[SemanticConcept]
    similar_concepts: List[SemanticConcept]
    similarity_scores: List[float]
    clusters: Dict[int, List[str]]
    memory_stats: Dict[str, Any]
    processing_time_ms: float
    cache_hit: bool
    timestamp: str
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertit en dictionnaire."""
        return {
            'concept_stored': self.concept_stored.to_dict() if self.concept_stored else None,
            'similar_concepts': [c.to_dict() for c in self.similar_concepts],
            'similarity_scores': self.similarity_scores,
            'clusters': self.clusters,
            'memory_stats': self.memory_stats,
            'processing_time_ms': self.processing_time_ms,
            'cache_hit': self.cache_hit,
            'timestamp': self.timestamp
        }


class SemanticLayer:
    """
    Layer 6: Semantic Memory - Mémoire sémantique et conceptuelle.
    
    Intègre SemanticMemoryV39 pour:
    - Stocker concepts avec embeddings
    - Rechercher concepts similaires
    - Organiser concepts en clusters
    - Maintenir statistiques mémoire
    
    Pipeline:
        1. Store: concept → embedding → memory
        2. Retrieve: query → similar concepts → ranked results
        3. Cluster: concepts → groups → organization
    
    Attributes:
        memory: Système de mémoire sémantique
        cache: Cache MD5 pour résultats
        forensic_log: Log forensic des opérations
    """
    
    def __init__(self, embedding_dim: int = 128, max_concepts: int = 10000):
        """
        Initialise la Semantic Layer.
        
        Args:
            embedding_dim: Dimension des embeddings
            max_concepts: Nombre maximum de concepts
        """
        self.memory = SemanticMemoryV39(
            embedding_dim=embedding_dim,
            max_concepts=max_concepts
        )
        self.cache: Dict[str, SemanticOutput] = {}
        self.forensic_log: List[Dict[str, Any]] = []
    
    def store(
        self,
        concept_id_or_dict: Any,
        embedding: Optional[np.ndarray] = None,
        metadata: Optional[Dict[str, Any]] = None,
        use_cache: bool = True
    ) -> SemanticOutput:
        """
        Stocke un concept dans la mémoire sémantique.
        Méthode wrapper compatible avec tests (accepte dict ou concept_id+embedding).
        
        Args:
            concept_id_or_dict: Soit un dict {"type": "pattern", "data": [1,2,3]},
                               soit un concept_id string
            embedding: Vecteur embedding (si concept_id fourni)
            metadata: Métadonnées additionnelles
            use_cache: Utiliser le cache
        
        Returns:
            SemanticOutput avec concept stocké
        """
        # Déterminer si c'est un dict ou concept_id
        if isinstance(concept_id_or_dict, dict):
            # Mode dict (pour tests)
            concept = concept_id_or_dict
            concept_id = concept.get('id', f"concept_{hash(str(concept))}")
            
            # Générer embedding depuis les données du concept
            if 'embedding' in concept:
                embedding = np.array(concept['embedding'])
            elif 'data' in concept:
                # Générer embedding simple depuis data
                data = concept['data']
                if isinstance(data, (list, np.ndarray)):
                    # Padding/truncate à embedding_dim
                    embedding = np.zeros(self.memory.embedding_dim)
                    data_array = np.array(data).flatten()
                    length = min(len(data_array), self.memory.embedding_dim)
                    embedding[:length] = data_array[:length]
                else:
                    # Embedding aléatoire pour autres types
                    embedding = np.random.randn(self.memory.embedding_dim)
            else:
                # Embedding aléatoire par défaut
                embedding = np.random.randn(self.memory.embedding_dim)
            
            metadata = {k: v for k, v in concept.items() if k not in ['id', 'embedding', 'data']}
        else:
            # Mode classique (concept_id + embedding)
            concept_id = concept_id_or_dict
            if embedding is None:
                raise ValueError("embedding requis quand concept_id fourni")
        start_time = time.time()
        
        # Générer clé cache
        cache_key = self._generate_cache_key('store', concept_id, embedding)
        
        # Vérifier cache
        if use_cache and cache_key in self.cache:
            output = self.cache[cache_key]
            output.cache_hit = True
            self._log_forensic('cache_hit', {'cache_key': cache_key})
            return output
        
        # Stocker concept
        timestamp = time.time()
        self.memory.add_concept(
            concept_id=concept_id,
            embedding=embedding,
            context=str(metadata) if metadata else "",
            timestamp=timestamp
        )
        
        # Récupérer concept stocké
        concept = self.memory.concepts.get(concept_id)
        
        # Récupérer statistiques
        memory_stats = self.memory.get_stats()
        
        # Créer output
        processing_time = (time.time() - start_time) * 1000
        output = SemanticOutput(
            concept_stored=concept,
            similar_concepts=[],
            similarity_scores=[],
            clusters={},
            memory_stats=memory_stats,
            processing_time_ms=processing_time,
            cache_hit=False,
            timestamp=datetime.now().isoformat()
        )
        
        # Mettre en cache
        if use_cache:
            self.cache[cache_key] = output
        
        # Log forensic
        self._log_forensic('concept_stored', {
            'concept_id': concept_id,
            'embedding_dim': len(embedding),
            'processing_time_ms': processing_time
        })
        
        return output
    
    def retrieve(
        self,
        query_embedding_or_dict: Any,
        top_k: int = 5,
        use_cache: bool = True
    ) -> SemanticOutput:
        """
        Récupère concepts similaires depuis la mémoire sémantique.
        
        Args:
            query_embedding_or_dict: Query embedding (np.ndarray) ou dict
            top_k: Nombre de résultats
            use_cache: Utiliser le cache
            
        Returns:
            SemanticOutput avec concepts similaires
        """
        # Support dict ou np.ndarray
        if isinstance(query_embedding_or_dict, dict):
            # Générer embedding depuis dict
            if 'embedding' in query_embedding_or_dict:
                query_embedding = np.array(query_embedding_or_dict['embedding'])
            elif 'data' in query_embedding_or_dict:
                data = query_embedding_or_dict['data']
                query_embedding = np.zeros(self.memory.embedding_dim)
                if isinstance(data, (list, np.ndarray)):
                    data_array = np.array(data).flatten()
                    length = min(len(data_array), self.memory.embedding_dim)
                    query_embedding[:length] = data_array[:length]
            else:
                query_embedding = np.random.randn(self.memory.embedding_dim)
        else:
            query_embedding = query_embedding_or_dict
        
        start_time = time.time()
        
        # Générer clé cache
        cache_key = self._generate_cache_key('retrieve', f'top{top_k}', query_embedding)
        
        # Vérifier cache
        if use_cache and cache_key in self.cache:
            output = self.cache[cache_key]
            output.cache_hit = True
            self._log_forensic('cache_hit', {'cache_key': cache_key})
            return output
        
        # Rechercher concepts similaires
        results = self.memory.query_similar(
            query_embedding=query_embedding,
            top_k=top_k,
            min_similarity=0.0
        )
        
        # Extraire concepts et scores
        similar_concepts = []
        similarity_scores = []
        for concept_id, score in results:
            if concept_id in self.memory.concepts:
                similar_concepts.append(self.memory.concepts[concept_id])
                similarity_scores.append(score)
        
        # Récupérer statistiques
        memory_stats = self.memory.get_stats()
        
        # Créer output
        processing_time = (time.time() - start_time) * 1000
        output = SemanticOutput(
            concept_stored=None,
            similar_concepts=similar_concepts,
            similarity_scores=similarity_scores,
            clusters={},
            memory_stats=memory_stats,
            processing_time_ms=processing_time,
            cache_hit=False,
            timestamp=datetime.now().isoformat()
        )
        
        # Mettre en cache
        if use_cache:
            self.cache[cache_key] = output
        
        # Log forensic
        self._log_forensic('concepts_retrieved', {
            'top_k': top_k,
            'num_results': len(similar_concepts),
            'processing_time_ms': processing_time
        })
        
        return output
    

    def cluster(
        self,
        n_clusters: int = 5,
        use_cache: bool = True
    ) -> SemanticOutput:
        """
        Organise concepts en clusters.
        
        Args:
            n_clusters: Nombre de clusters
            use_cache: Utiliser le cache
        
        Returns:
            SemanticOutput avec clusters
        """
        start_time = time.time()
        
        # Générer clé cache
        cache_key = self._generate_cache_key('cluster', f'n{n_clusters}', np.array([n_clusters]))
        
        # Vérifier cache
        if use_cache and cache_key in self.cache:
            output = self.cache[cache_key]
            output.cache_hit = True
            self._log_forensic('cache_hit', {'cache_key': cache_key})
            return output
        
        # Créer clusters (simplifié - utiliser clusters existants)
        clusters = {i: list(concepts) for i, concepts in self.memory.clusters.items()}
        
        # Si pas de clusters, créer clusters basiques
        if not clusters and self.memory.concepts:
            concept_ids = list(self.memory.concepts.keys())
            cluster_size = max(1, len(concept_ids) // n_clusters)
            for i in range(n_clusters):
                start = i * cluster_size
                end = start + cluster_size if i < n_clusters - 1 else len(concept_ids)
                clusters[i] = concept_ids[start:end]
        
        # Récupérer statistiques
        memory_stats = self.memory.get_stats()
        
        # Créer output
        processing_time = (time.time() - start_time) * 1000
        output = SemanticOutput(
            concept_stored=None,
            similar_concepts=[],
            similarity_scores=[],
            clusters=clusters,
            memory_stats=memory_stats,
            processing_time_ms=processing_time,
            cache_hit=False,
            timestamp=datetime.now().isoformat()
        )
        
        # Mettre en cache
        if use_cache:
            self.cache[cache_key] = output
        
        # Log forensic
        self._log_forensic('concepts_clustered', {
            'n_clusters': n_clusters,
            'clusters_created': len(clusters),
            'processing_time_ms': processing_time
        })
        
        return output
    
    def _generate_cache_key(
        self,
        operation: str,
        identifier: str,
        embedding: np.ndarray
    ) -> str:
        """Génère clé MD5 pour cache."""
        emb_hash = hashlib.md5(embedding.tobytes()).hexdigest()[:8]
        content = f"{operation}_{identifier}_{emb_hash}"
        return hashlib.md5(content.encode()).hexdigest()
    
    def _log_forensic(self, event: str, data: Dict[str, Any]):
        """Log événement forensic."""
        self.forensic_log.append({
            'timestamp': datetime.now().isoformat(),
            'event': event,
            'data': data
        })
    
    def get_stats(self) -> Dict[str, Any]:
        """Retourne statistiques complètes."""
        return {
            'memory_stats': {
                'total_concepts': len(self.memory.concepts),
                'embedding_dim': self.memory.embedding_dim,
                'total_queries': self.memory.total_queries,
                'cache_hits': self.memory.cache_hits
            },
            'cache_size': len(self.cache),
            'forensic_events': len(self.forensic_log)
        }
    
    def save_forensic_log(self, filepath: str):
        """Sauvegarde log forensic."""
        import json
        with open(filepath, 'w') as f:
            for event in self.forensic_log:
                f.write(json.dumps(event) + '\n')


def test_semantic_layer():
    """Test unitaire de la Semantic Layer."""
    print("="*80)
    print("TEST SEMANTIC LAYER V2.0")
    print("="*80)
    
    # Créer layer
    layer = SemanticLayer(embedding_dim=64, max_concepts=1000)
    
    # Test 1: Stocker concepts
    print("\n[TEST 1] Stockage concepts...")
    for i in range(10):
        embedding = np.random.randn(64)
        output = layer.store(
            concept_id=f"concept_{i}",
            embedding=embedding,
            metadata={'type': 'test', 'index': i}
        )
        assert output.concept_stored is not None
    print(f"✅ 10 concepts stockés")
    
    # Test 2: Recherche similarité
    print("\n[TEST 2] Recherche similarité...")
    query = np.random.randn(64)
    output = layer.retrieve(query, top_k=3)
    assert len(output.similar_concepts) <= 3
    print(f"✅ {len(output.similar_concepts)} concepts similaires trouvés")
    if output.similarity_scores:
        print(f"   Similarité moyenne: {np.mean(output.similarity_scores):.3f}")
    
    # Test 3: Clustering
    print("\n[TEST 3] Clustering concepts...")
    output = layer.cluster(n_clusters=3)
    assert len(output.clusters) > 0
    print(f"✅ {len(output.clusters)} clusters créés")
    for cluster_id, concepts in output.clusters.items():
        print(f"   Cluster {cluster_id}: {len(concepts)} concepts")
    
    # Test 4: Cache fonctionnel
    print("\n[TEST 4] Vérification cache...")
    embedding = np.random.randn(64)
    output1 = layer.store('test_cache', embedding)
    output2 = layer.store('test_cache', embedding)
    assert output2.cache_hit == True
    print(f"✅ Cache hit: {output2.cache_hit}")
    
    # Statistiques finales
    print("\n" + "="*80)
    print("STATISTIQUES FINALES")
    print("="*80)
    stats = layer.get_stats()
    print(f"Concepts stockés: {stats['memory_stats']['total_concepts']}")
    print(f"Dimension embeddings: {stats['memory_stats']['embedding_dim']}")
    print(f"Cache size: {stats['cache_size']}")
    print(f"Forensic events: {stats['forensic_events']}")
    
    # Sauvegarder forensic
    layer.save_forensic_log('/tmp/semantic_layer_test.jsonl')
    print(f"\nForensic log: /tmp/semantic_layer_test.jsonl")
    
    # Verdict
    print("\n" + "="*80)
    if stats['memory_stats']['total_concepts'] >= 10 and stats['forensic_events'] > 10:
        print("✅ TEST RÉUSSI - Semantic Layer fonctionnel")
        print("   - Stockage concepts: OK")
        print("   - Recherche similarité: OK")
        print("   - Clustering: OK")
        print("   - Cache: OK")
    else:
        print("⚠️  TEST PARTIEL - Vérifier concepts/events")
    print("="*80)


if __name__ == '__main__':
    test_semantic_layer()

# Made with Bob