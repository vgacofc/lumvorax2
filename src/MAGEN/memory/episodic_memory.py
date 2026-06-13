"""
MAGEN - Episodic Memory System
Mémoire épisodique vectorisée avec analogical reasoning

PRIORITÉ 3 identifiée par analyse experte:
- Memory épisodique (pas juste hiérarchique)
- Retrieval par similarité vectorielle
- Analogical transfer entre épisodes
- Compression conceptuelle

Architecture:
- Encodage épisodes → vecteurs denses
- Index vectoriel pour retrieval rapide
- Analogical reasoning (source → target)
- Conceptual compression (patterns récurrents)
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional, Set
from dataclasses import dataclass, field
from collections import defaultdict
import copy

# Import modules MAGEN
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic import Transform, ComposedTransform, Hypothesis
from objects import Object, SceneGraph, ObjectTransformation


@dataclass
class Episode:
    """
    Épisode mémorisé: expérience complète d'un puzzle
    
    Contient:
    - Input/output grids
    - Scene graphs
    - Transformations appliquées
    - Résultat (succès/échec)
    - Embedding vectoriel
    """
    id: int
    input_grid: np.ndarray
    output_grid: np.ndarray
    input_scene_graph: Optional[SceneGraph] = None
    output_scene_graph: Optional[SceneGraph] = None
    transformations: List[Any] = field(default_factory=list)
    success: bool = False
    reward: float = 0.0
    embedding: Optional[np.ndarray] = None
    metadata: Dict[str, Any] = field(default_factory=dict)
    timestamp: float = 0.0
    
    def __repr__(self):
        status = "✓" if self.success else "✗"
        return f"Episode({self.id}, {status}, reward={self.reward:.2f})"


@dataclass
class ConceptualPattern:
    """
    Pattern conceptuel extrait de multiples épisodes
    
    Représente un concept abstrait réutilisable:
    - "Rotation 90° + flip" apparaît dans 5 épisodes
    - "Déplacer objets rouges vers droite" apparaît dans 3 épisodes
    """
    id: int
    name: str
    description: str
    frequency: int = 0
    success_rate: float = 0.0
    episodes: List[int] = field(default_factory=list)
    embedding: Optional[np.ndarray] = None
    properties: Dict[str, Any] = field(default_factory=dict)
    
    def __repr__(self):
        return f"Pattern({self.name}, freq={self.frequency}, success={self.success_rate:.2f})"


class MemoryEncoder:
    """
    Encodeur d'épisodes en vecteurs denses
    
    Stratégies d'encodage:
    1. Scene graph features (objets, relations, shapes)
    2. Transformation features (types, paramètres)
    3. Grid features (taille, couleurs, densité)
    4. Structural features (symétries, patterns)
    """
    
    def __init__(self, embedding_dim: int = 128):
        self.embedding_dim = embedding_dim
        self.encodings_count = 0
    
    def encode_episode(self, episode: Episode) -> np.ndarray:
        """
        Encoder épisode complet en vecteur dense
        
        Returns:
            Vecteur de dimension embedding_dim
        """
        features = []
        
        # 1. Grid features (32 dims)
        grid_features = self._encode_grid_features(episode.input_grid, episode.output_grid)
        features.extend(grid_features)
        
        # 2. Scene graph features (48 dims)
        if episode.input_scene_graph and episode.output_scene_graph:
            sg_features = self._encode_scene_graph_features(
                episode.input_scene_graph, episode.output_scene_graph
            )
            features.extend(sg_features)
        else:
            features.extend([0.0] * 48)
        
        # 3. Transformation features (32 dims)
        transform_features = self._encode_transformation_features(episode.transformations)
        features.extend(transform_features)
        
        # 4. Outcome features (16 dims)
        outcome_features = self._encode_outcome_features(episode)
        features.extend(outcome_features)
        
        # Normaliser et padding/truncate à embedding_dim
        features = np.array(features, dtype=np.float32)
        
        if len(features) < self.embedding_dim:
            # Padding avec zéros
            features = np.pad(features, (0, self.embedding_dim - len(features)))
        elif len(features) > self.embedding_dim:
            # Truncate
            features = features[:self.embedding_dim]
        
        # Normalisation L2
        norm = np.linalg.norm(features)
        if norm > 0:
            features = features / norm
        
        self.encodings_count += 1
        return features
    
    def _encode_grid_features(self, input_grid: np.ndarray, output_grid: np.ndarray) -> List[float]:
        """Encoder features des grilles (32 dims)"""
        features = []
        
        for grid in [input_grid, output_grid]:
            # Taille (2)
            features.append(float(grid.shape[0]) / 30.0)  # Normaliser par taille max ARC
            features.append(float(grid.shape[1]) / 30.0)
            
            # Couleurs (10)
            grid_int = grid.astype(int)  # Convertir en int pour bincount
            color_counts = np.bincount(grid_int.flatten(), minlength=10)
            color_freqs = color_counts / grid.size
            features.extend(color_freqs.tolist())
            
            # Densité non-background (1)
            non_bg = np.sum(grid != 0) / grid.size
            features.append(float(non_bg))
            
            # Entropie (1)
            probs = color_freqs[color_freqs > 0]
            entropy = -np.sum(probs * np.log2(probs + 1e-10))
            features.append(float(entropy) / 3.32)  # Normaliser par log2(10)
        
        # Différence input→output (4)
        size_change = (output_grid.shape[0] * output_grid.shape[1]) / (input_grid.shape[0] * input_grid.shape[1])
        features.append(float(size_change))
        
        color_change = len(set(output_grid.flatten()) - set(input_grid.flatten()))
        features.append(float(color_change) / 10.0)
        
        # Similarité structurelle (2)
        if input_grid.shape == output_grid.shape:
            pixel_similarity = np.sum(input_grid == output_grid) / input_grid.size
            features.append(float(pixel_similarity))
        else:
            features.append(0.0)
        
        features.append(1.0 if input_grid.shape == output_grid.shape else 0.0)
        
        return features
    
    def _encode_scene_graph_features(self, sg_input: SceneGraph, sg_output: SceneGraph) -> List[float]:
        """Encoder features des scene graphs (48 dims)"""
        features = []
        
        for sg in [sg_input, sg_output]:
            # Nombre d'objets (1)
            features.append(float(len(sg.objects)) / 20.0)  # Normaliser
            
            # Distribution de shapes (13)
            shape_counts = defaultdict(int)
            for obj in sg.objects:
                shape_counts[obj.shape_type.value] += 1
            
            shape_types = ['point', 'line_horizontal', 'line_vertical', 'line_diagonal',
                          'rectangle', 'square', 'hollow_rectangle', 'l_shape', 't_shape',
                          'cross', 'pattern', 'blob', 'unknown']
            for shape_type in shape_types:
                count = shape_counts.get(shape_type, 0)
                features.append(float(count) / max(len(sg.objects), 1))
            
            # Distribution de couleurs objets (10)
            color_counts = defaultdict(int)
            for obj in sg.objects:
                color_counts[obj.color] += 1
            for color in range(10):
                count = color_counts.get(color, 0)
                features.append(float(count) / max(len(sg.objects), 1))
        
        # Relations (12)
        relation_counts = defaultdict(int)
        for rel in sg_output.relations:
            relation_counts[rel.type.value] += 1
        
        relation_types = ['above', 'below', 'left_of', 'right_of', 'inside', 'contains',
                         'adjacent', 'overlaps', 'same_row', 'same_column',
                         'aligned_horizontal', 'aligned_vertical']
        for rel_type in relation_types:
            count = relation_counts.get(rel_type, 0)
            features.append(float(count) / max(len(sg_output.relations), 1))
        
        return features
    
    def _encode_transformation_features(self, transformations: List[Any]) -> List[float]:
        """Encoder features des transformations (32 dims)"""
        features = []
        
        # Nombre de transformations (1)
        features.append(float(len(transformations)) / 10.0)
        
        # Types de transformations (20)
        transform_types = [
            'rotate_90', 'rotate_180', 'rotate_270', 'flip_horizontal', 'flip_vertical',
            'transpose', 'crop', 'expand', 'translate', 'scale_up', 'scale_down',
            'recolor', 'color_map', 'invert_colors', 'extract_objects', 'flood_fill',
            'detect_symmetry', 'complete_symmetry', 'repeat_pattern', 'identity'
        ]
        
        transform_counts = defaultdict(int)
        for t in transformations:
            if hasattr(t, 'type'):
                transform_counts[t.type.value] += 1
            elif hasattr(t, 'transform_type'):
                transform_counts[t.transform_type] += 1
        
        for ttype in transform_types:
            count = transform_counts.get(ttype, 0)
            features.append(float(count) / max(len(transformations), 1))
        
        # Complexité (1)
        complexity = len(transformations) / 10.0
        features.append(float(complexity))
        
        # Padding (10)
        features.extend([0.0] * 10)
        
        return features
    
    def _encode_outcome_features(self, episode: Episode) -> List[float]:
        """Encoder features du résultat (16 dims)"""
        features = []
        
        # Succès (1)
        features.append(1.0 if episode.success else 0.0)
        
        # Reward (1)
        features.append(float(episode.reward))
        
        # Metadata features (14)
        metadata_keys = ['attempts', 'time_taken', 'confidence', 'difficulty']
        for key in metadata_keys:
            if key in episode.metadata:
                features.append(float(episode.metadata[key]))
            else:
                features.append(0.0)
        
        # Padding
        features.extend([0.0] * (16 - len(features)))
        
        return features
    
    def compute_similarity(self, embedding1: np.ndarray, embedding2: np.ndarray,
                          metric: str = 'cosine') -> float:
        """
        Calculer similarité entre deux embeddings
        
        Args:
            embedding1, embedding2: Vecteurs à comparer
            metric: 'cosine', 'euclidean', 'manhattan'
        
        Returns:
            Similarité [0, 1] (1 = identique)
        """
        if metric == 'cosine':
            # Cosine similarity
            dot = np.dot(embedding1, embedding2)
            norm1 = np.linalg.norm(embedding1)
            norm2 = np.linalg.norm(embedding2)
            if norm1 == 0 or norm2 == 0:
                return 0.0
            similarity = dot / (norm1 * norm2)
            # Normaliser à [0, 1]
            return float((similarity + 1.0) / 2.0)
        
        elif metric == 'euclidean':
            # Euclidean distance → similarity
            dist = np.linalg.norm(embedding1 - embedding2)
            # Normaliser: similarity = 1 / (1 + distance)
            return float(1.0 / (1.0 + dist))
        
        elif metric == 'manhattan':
            # Manhattan distance → similarity
            dist = np.sum(np.abs(embedding1 - embedding2))
            return float(1.0 / (1.0 + dist))
        
        else:
            raise ValueError(f"Unknown metric: {metric}")
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'encodings': self.encodings_count
        }


class EpisodicMemory:
    """
    Système de mémoire épisodique vectorisée
    
    Fonctionnalités:
    - Stockage épisodes avec embeddings
    - Retrieval par similarité (k-NN)
    - Analogical transfer (source → target)
    - Conceptual compression (extraction patterns)
    - Hierarchical organization
    """
    
    def __init__(self, encoder: MemoryEncoder, max_episodes: int = 10000,
                 similarity_threshold: float = 0.7, verbose: bool = False):
        self.encoder = encoder
        self.max_episodes = max_episodes
        self.similarity_threshold = similarity_threshold
        self.verbose = verbose
        
        # Storage
        self.episodes: List[Episode] = []
        self.episode_index: Dict[int, Episode] = {}
        self.embeddings: Optional[np.ndarray] = None
        
        # Conceptual patterns
        self.patterns: List[ConceptualPattern] = []
        self.pattern_index: Dict[int, ConceptualPattern] = {}
        
        # Statistics
        self.total_episodes = 0
        self.successful_episodes = 0
        self.retrievals = 0
        self.analogical_transfers = 0
    
    def store_episode(self, episode: Episode) -> int:
        """
        Stocker un épisode en mémoire
        
        Returns:
            ID de l'épisode stocké
        """
        # Assigner ID si pas déjà fait
        if episode.id == 0:
            episode.id = self.total_episodes + 1
        
        # Encoder en vecteur
        if episode.embedding is None:
            episode.embedding = self.encoder.encode_episode(episode)
        
        # Stocker
        self.episodes.append(episode)
        self.episode_index[episode.id] = episode
        
        # Mettre à jour matrice embeddings
        if self.embeddings is None:
            self.embeddings = episode.embedding.reshape(1, -1)
        else:
            self.embeddings = np.vstack([self.embeddings, episode.embedding])
        
        # Statistics
        self.total_episodes += 1
        if episode.success:
            self.successful_episodes += 1
        
        # Compression si trop d'épisodes
        if len(self.episodes) > self.max_episodes:
            self._compress_memory()
        
        return episode.id
    
    def retrieve_similar(self, query_episode: Episode, k: int = 5,
                        metric: str = 'cosine') -> List[Tuple[Episode, float]]:
        """
        Retrieval des k épisodes les plus similaires
        
        Args:
            query_episode: Épisode requête
            k: Nombre d'épisodes à retourner
            metric: Métrique de similarité
        
        Returns:
            Liste de (episode, similarity) triée par similarité décroissante
        """
        if len(self.episodes) == 0:
            return []
        
        # Encoder query
        if query_episode.embedding is None:
            query_episode.embedding = self.encoder.encode_episode(query_episode)
        
        # Calculer similarités avec tous les épisodes
        similarities = []
        for i, episode in enumerate(self.episodes):
            if episode.embedding is not None:
                sim = self.encoder.compute_similarity(
                    query_episode.embedding, episode.embedding, metric
                )
                similarities.append((episode, float(sim)))
        
        # Trier par similarité décroissante
        similarities.sort(key=lambda x: x[1], reverse=True)
        
        self.retrievals += 1
        return similarities[:k]
    
    def analogical_transfer(self, source_episode: Episode, target_input: np.ndarray) -> List[Any]:
        """
        Analogical reasoning: transférer solution de source vers target
        
        Stratégie:
        1. Identifier transformations dans source
        2. Adapter transformations au contexte target
        3. Retourner transformations adaptées
        
        Args:
            source_episode: Épisode source (avec solution connue)
            target_input: Grille input du problème target
        
        Returns:
            Liste de transformations adaptées
        """
        if not source_episode.transformations:
            return []
        
        # Analyser différences input source vs target
        source_input = source_episode.input_grid
        
        # Facteurs d'adaptation
        scale_factor_h = target_input.shape[0] / source_input.shape[0]
        scale_factor_w = target_input.shape[1] / source_input.shape[1]
        
        # Adapter transformations
        adapted_transforms = []
        
        for transform in source_episode.transformations:
            # Copier transformation
            adapted = copy.deepcopy(transform)
            
            # Adapter paramètres spatiaux
            if hasattr(adapted, 'parameters'):
                if 'dx' in adapted.parameters:
                    adapted.parameters['dx'] = int(adapted.parameters['dx'] * scale_factor_w)
                if 'dy' in adapted.parameters:
                    adapted.parameters['dy'] = int(adapted.parameters['dy'] * scale_factor_h)
                if 'x' in adapted.parameters:
                    adapted.parameters['x'] = int(adapted.parameters['x'] * scale_factor_w)
                if 'y' in adapted.parameters:
                    adapted.parameters['y'] = int(adapted.parameters['y'] * scale_factor_h)
            
            adapted_transforms.append(adapted)
        
        self.analogical_transfers += 1
        return adapted_transforms
    
    def extract_conceptual_patterns(self, min_frequency: int = 3) -> List[ConceptualPattern]:
        """
        Extraire patterns conceptuels récurrents
        
        Identifie séquences de transformations qui apparaissent fréquemment
        et ont un bon taux de succès
        
        Args:
            min_frequency: Fréquence minimale pour considérer un pattern
        
        Returns:
            Liste de patterns conceptuels
        """
        # Grouper épisodes par séquence de transformations
        transform_sequences = defaultdict(list)
        
        for episode in self.episodes:
            if not episode.transformations:
                continue
            
            # Créer signature de la séquence
            signature = tuple([
                t.type.value if hasattr(t, 'type') else t.transform_type
                for t in episode.transformations
            ])
            
            transform_sequences[signature].append(episode)
        
        # Créer patterns pour séquences fréquentes
        patterns = []
        pattern_id = len(self.patterns) + 1
        
        for signature, episodes in transform_sequences.items():
            if len(episodes) >= min_frequency:
                # Calculer taux de succès
                successes = sum(1 for ep in episodes if ep.success)
                success_rate = successes / len(episodes)
                
                # Créer pattern
                pattern = ConceptualPattern(
                    id=pattern_id,
                    name=f"Pattern_{pattern_id}",
                    description=" → ".join(signature),
                    frequency=len(episodes),
                    success_rate=success_rate,
                    episodes=[ep.id for ep in episodes]
                )
                
                # Encoder pattern (moyenne des embeddings)
                embeddings = [ep.embedding for ep in episodes if ep.embedding is not None]
                if embeddings:
                    pattern.embedding = np.mean(embeddings, axis=0)
                
                patterns.append(pattern)
                self.patterns.append(pattern)
                self.pattern_index[pattern_id] = pattern
                pattern_id += 1
        
        return patterns
    
    def _compress_memory(self):
        """
        Compression de la mémoire quand trop d'épisodes
        
        Stratégie:
        1. Garder tous les épisodes réussis
        2. Garder épisodes récents
        3. Supprimer épisodes anciens échoués
        """
        # Séparer succès et échecs
        successful = [ep for ep in self.episodes if ep.success]
        failed = [ep for ep in self.episodes if not ep.success]
        
        # Trier échecs par timestamp (plus récents d'abord)
        failed.sort(key=lambda ep: ep.timestamp, reverse=True)
        
        # Garder: tous succès + échecs récents
        keep_failed = failed[:self.max_episodes // 2]
        
        # Nouvelle liste
        self.episodes = successful + keep_failed
        
        # Reconstruire index et embeddings
        self.episode_index = {ep.id: ep for ep in self.episodes}
        valid_embeddings = [ep.embedding for ep in self.episodes if ep.embedding is not None]
        if valid_embeddings:
            self.embeddings = np.vstack(valid_embeddings)
        else:
            self.embeddings = None
        
        if self.verbose:
            print(f"Memory compressed: {len(successful)} successful + {len(keep_failed)} recent failed")
    
    def get_statistics(self) -> Dict[str, Any]:
        """Statistiques complètes"""
        return {
            'total_episodes': self.total_episodes,
            'stored_episodes': len(self.episodes),
            'successful_episodes': self.successful_episodes,
            'success_rate': self.successful_episodes / self.total_episodes if self.total_episodes > 0 else 0.0,
            'patterns_extracted': len(self.patterns),
            'retrievals': self.retrievals,
            'analogical_transfers': self.analogical_transfers,
            'encoder_stats': self.encoder.get_statistics()
        }


# Made with Bob - Expert en Machine Learning, Cognitive Science, Information Retrieval, Memory Systems