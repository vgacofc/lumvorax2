"""
Semantic Memory V39 - Mémoire Sémantique pour MAGEN
Layer 6: Semantic Memory dans Architecture V2

Objectif:
- Stocker et récupérer des concepts abstraits
- Créer des embeddings sémantiques des patterns
- Permettre le raisonnement par analogie
- Supporter l'apprentissage par transfert

Auteur: Bob (Assistant IA)
Date: 2026-06-18
Version: 39
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from collections import defaultdict
import json
from pathlib import Path


@dataclass
class SemanticConcept:
    """
    Représente un concept sémantique abstrait.
    
    Attributes:
        concept_id: Identifiant unique du concept
        embedding: Vecteur d'embedding (256D)
        frequency: Nombre d'occurrences
        contexts: Contextes d'apparition
        related_concepts: Concepts liés (similarité)
        creation_time: Timestamp de création
        last_access: Dernier accès
    """
    concept_id: str
    embedding: np.ndarray
    frequency: int = 0
    contexts: List[str] = field(default_factory=list)
    related_concepts: Dict[str, float] = field(default_factory=dict)
    creation_time: float = 0.0
    last_access: float = 0.0
    
    def to_dict(self) -> Dict:
        """Sérialise le concept."""
        return {
            'concept_id': self.concept_id,
            'embedding': self.embedding.tolist(),
            'frequency': self.frequency,
            'contexts': self.contexts[-10:],  # Garde 10 derniers contextes
            'related_concepts': self.related_concepts,
            'creation_time': self.creation_time,
            'last_access': self.last_access
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> 'SemanticConcept':
        """Désérialise le concept."""
        return cls(
            concept_id=data['concept_id'],
            embedding=np.array(data['embedding']),
            frequency=data['frequency'],
            contexts=data['contexts'],
            related_concepts=data['related_concepts'],
            creation_time=data['creation_time'],
            last_access=data['last_access']
        )


class SemanticMemoryV39:
    """
    Mémoire Sémantique pour MAGEN V39.
    
    Fonctionnalités:
    - Stockage de concepts abstraits
    - Recherche par similarité
    - Clustering automatique
    - Raisonnement par analogie
    - Apprentissage par transfert
    """
    
    def __init__(self,
                 embedding_dim: int = 256,
                 max_concepts: int = 10000,
                 similarity_threshold: float = 0.7,
                 decay_rate: float = 0.99):
        """
        Initialise la mémoire sémantique.
        
        Args:
            embedding_dim: Dimension des embeddings
            max_concepts: Nombre maximum de concepts
            similarity_threshold: Seuil de similarité pour liens
            decay_rate: Taux de décroissance de la fréquence
        """
        self.embedding_dim = embedding_dim
        self.max_concepts = max_concepts
        self.similarity_threshold = similarity_threshold
        self.decay_rate = decay_rate
        
        # Stockage des concepts
        self.concepts: Dict[str, SemanticConcept] = {}
        
        # Index pour recherche rapide
        self.embedding_matrix: Optional[np.ndarray] = None
        self.concept_ids: List[str] = []
        
        # Clusters de concepts
        self.clusters: Dict[int, Set[str]] = defaultdict(set)
        
        # Statistiques
        self.total_queries = 0
        self.cache_hits = 0
        
        # Cache de similarité
        self.similarity_cache: Dict[Tuple[str, str], float] = {}
        
    def add_concept(self,
                   concept_id: str,
                   embedding: np.ndarray,
                   context: str = "",
                   timestamp: float = 0.0) -> bool:
        """
        Ajoute ou met à jour un concept.
        
        Args:
            concept_id: Identifiant du concept
            embedding: Vecteur d'embedding
            context: Contexte d'apparition
            timestamp: Timestamp
            
        Returns:
            True si ajouté/mis à jour avec succès
        """
        # Normaliser l'embedding
        embedding = embedding / (np.linalg.norm(embedding) + 1e-8)
        
        if concept_id in self.concepts:
            # Mise à jour concept existant
            concept = self.concepts[concept_id]
            
            # Moyenne pondérée des embeddings
            alpha = 0.1  # Taux d'apprentissage
            concept.embedding = (1 - alpha) * concept.embedding + alpha * embedding
            concept.embedding /= (np.linalg.norm(concept.embedding) + 1e-8)
            
            concept.frequency += 1
            if context:
                concept.contexts.append(context)
            concept.last_access = timestamp
            
        else:
            # Nouveau concept
            if len(self.concepts) >= self.max_concepts:
                # Supprimer le concept le moins fréquent
                self._evict_least_frequent()
            
            concept = SemanticConcept(
                concept_id=concept_id,
                embedding=embedding,
                frequency=1,
                contexts=[context] if context else [],
                creation_time=timestamp,
                last_access=timestamp
            )
            self.concepts[concept_id] = concept
            
            # Invalider l'index
            self.embedding_matrix = None
        
        # Mettre à jour les relations
        self._update_relations(concept_id)
        
        return True
    
    def query_similar(self,
                     query_embedding: np.ndarray,
                     top_k: int = 5,
                     min_similarity: float = 0.5) -> List[Tuple[str, float]]:
        """
        Recherche les concepts les plus similaires.
        
        Args:
            query_embedding: Vecteur de requête
            top_k: Nombre de résultats
            min_similarity: Similarité minimale
            
        Returns:
            Liste de (concept_id, similarité)
        """
        self.total_queries += 1
        
        if not self.concepts:
            return []
        
        # Reconstruire l'index si nécessaire
        if self.embedding_matrix is None:
            self._rebuild_index()
        
        # Normaliser la requête
        query_embedding = query_embedding / (np.linalg.norm(query_embedding) + 1e-8)
        
        # Calcul de similarité cosinus
        similarities = np.dot(self.embedding_matrix, query_embedding)
        
        # Filtrer et trier
        valid_indices = np.where(similarities >= min_similarity)[0]
        if len(valid_indices) == 0:
            return []
        
        sorted_indices = valid_indices[np.argsort(-similarities[valid_indices])]
        top_indices = sorted_indices[:top_k]
        
        results = [
            (self.concept_ids[idx], float(similarities[idx]))
            for idx in top_indices
        ]
        
        return results
    
    def find_analogies(self,
                      concept_a: str,
                      concept_b: str,
                      concept_c: str,
                      top_k: int = 3) -> List[Tuple[str, float]]:
        """
        Raisonnement par analogie: A est à B ce que C est à ?
        
        Args:
            concept_a: Premier concept de la relation
            concept_b: Second concept de la relation
            concept_c: Concept source pour l'analogie
            top_k: Nombre de résultats
            
        Returns:
            Liste de (concept_id, score)
        """
        if not all(c in self.concepts for c in [concept_a, concept_b, concept_c]):
            return []
        
        # Vecteur de relation A -> B
        emb_a = self.concepts[concept_a].embedding
        emb_b = self.concepts[concept_b].embedding
        emb_c = self.concepts[concept_c].embedding
        
        relation_vector = emb_b - emb_a
        
        # Appliquer la relation à C
        target_embedding = emb_c + relation_vector
        
        # Rechercher les concepts similaires (exclure A, B, C)
        results = self.query_similar(target_embedding, top_k=top_k + 3)
        
        # Filtrer les concepts source
        filtered = [
            (cid, score) for cid, score in results
            if cid not in {concept_a, concept_b, concept_c}
        ]
        
        return filtered[:top_k]
    
    def get_concept_cluster(self, concept_id: str) -> Set[str]:
        """
        Retourne le cluster d'un concept.
        
        Args:
            concept_id: Identifiant du concept
            
        Returns:
            Ensemble des concepts du même cluster
        """
        if concept_id not in self.concepts:
            return set()
        
        # Trouver le cluster
        for cluster_id, members in self.clusters.items():
            if concept_id in members:
                return members.copy()
        
        return {concept_id}
    
    def decay_frequencies(self):
        """Applique la décroissance aux fréquences."""
        for concept in self.concepts.values():
            concept.frequency = int(concept.frequency * self.decay_rate)
    
    def _update_relations(self, concept_id: str):
        """Met à jour les relations d'un concept."""
        if concept_id not in self.concepts:
            return
        
        concept = self.concepts[concept_id]
        
        # Calculer similarités avec autres concepts
        for other_id, other_concept in self.concepts.items():
            if other_id == concept_id:
                continue
            
            # Vérifier cache
            cache_key = tuple(sorted([concept_id, other_id]))
            if cache_key in self.similarity_cache:
                similarity = self.similarity_cache[cache_key]
            else:
                similarity = float(np.dot(concept.embedding, other_concept.embedding))
                self.similarity_cache[cache_key] = similarity
            
            # Ajouter relation si similarité suffisante
            if similarity >= self.similarity_threshold:
                concept.related_concepts[other_id] = similarity
                other_concept.related_concepts[concept_id] = similarity
    
    def _rebuild_index(self):
        """Reconstruit l'index de recherche."""
        self.concept_ids = list(self.concepts.keys())
        self.embedding_matrix = np.vstack([
            self.concepts[cid].embedding
            for cid in self.concept_ids
        ])
    
    def _evict_least_frequent(self):
        """Supprime le concept le moins fréquent."""
        if not self.concepts:
            return
        
        # Trouver le concept le moins fréquent
        min_concept_id = min(
            self.concepts.keys(),
            key=lambda cid: self.concepts[cid].frequency
        )
        
        # Supprimer
        del self.concepts[min_concept_id]
        
        # Nettoyer les relations
        for concept in self.concepts.values():
            concept.related_concepts.pop(min_concept_id, None)
        
        # Invalider l'index
        self.embedding_matrix = None
    
    def save(self, filepath: str):
        """Sauvegarde la mémoire sémantique."""
        data = {
            'config': {
                'embedding_dim': self.embedding_dim,
                'max_concepts': self.max_concepts,
                'similarity_threshold': self.similarity_threshold,
                'decay_rate': self.decay_rate
            },
            'concepts': {
                cid: concept.to_dict()
                for cid, concept in self.concepts.items()
            },
            'stats': {
                'total_queries': self.total_queries,
                'cache_hits': self.cache_hits,
                'num_concepts': len(self.concepts)
            }
        }
        
        Path(filepath).parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def load(self, filepath: str):
        """Charge la mémoire sémantique."""
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        # Restaurer config
        config = data['config']
        self.embedding_dim = config['embedding_dim']
        self.max_concepts = config['max_concepts']
        self.similarity_threshold = config['similarity_threshold']
        self.decay_rate = config['decay_rate']
        
        # Restaurer concepts
        self.concepts = {
            cid: SemanticConcept.from_dict(cdata)
            for cid, cdata in data['concepts'].items()
        }
        
        # Restaurer stats
        stats = data['stats']
        self.total_queries = stats['total_queries']
        self.cache_hits = stats['cache_hits']
        
        # Invalider l'index
        self.embedding_matrix = None
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            'num_concepts': len(self.concepts),
            'total_queries': self.total_queries,
            'cache_hits': self.cache_hits,
            'cache_hit_rate': self.cache_hits / max(1, self.total_queries),
            'avg_frequency': np.mean([c.frequency for c in self.concepts.values()]) if self.concepts else 0,
            'num_clusters': len(self.clusters)
        }


def test_semantic_memory():
    """Test de la mémoire sémantique."""
    print("=== Test Semantic Memory V39 ===\n")
    
    memory = SemanticMemoryV39(embedding_dim=256)
    
    # Créer des concepts de test
    concepts = {
        'move_up': np.random.randn(256),
        'move_down': np.random.randn(256),
        'move_left': np.random.randn(256),
        'move_right': np.random.randn(256),
        'rotate_cw': np.random.randn(256),
        'rotate_ccw': np.random.randn(256)
    }
    
    # Ajouter les concepts
    for cid, emb in concepts.items():
        memory.add_concept(cid, emb, context=f"test_{cid}")
    
    print(f"✓ {len(concepts)} concepts ajoutés")
    
    # Test recherche similarité
    query = concepts['move_up'] + 0.1 * np.random.randn(256)
    results = memory.query_similar(query, top_k=3)
    print(f"\n✓ Recherche similarité (top 3):")
    for cid, score in results:
        print(f"  - {cid}: {score:.3f}")
    
    # Test analogie
    analogies = memory.find_analogies('move_up', 'move_down', 'move_left', top_k=2)
    print(f"\n✓ Analogie (up:down :: left:?):")
    for cid, score in analogies:
        print(f"  - {cid}: {score:.3f}")
    
    # Stats
    stats = memory.get_stats()
    print(f"\n✓ Statistiques:")
    print(f"  - Concepts: {stats['num_concepts']}")
    print(f"  - Requêtes: {stats['total_queries']}")
    print(f"  - Fréquence moyenne: {stats['avg_frequency']:.2f}")
    
    # Sauvegarde
    memory.save('/tmp/semantic_memory_test.json')
    print(f"\n✓ Mémoire sauvegardée: /tmp/semantic_memory_test.json")


if __name__ == '__main__':
    test_semantic_memory()

# Made with Bob
