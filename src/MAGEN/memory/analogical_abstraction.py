"""
MAGEN - Analogical Abstraction System
Amélioration CRITIQUE du module Memory (identifiée par analyse experte)

PROBLÈME IDENTIFIÉ:
- Retrieval actuel = simple k-NN sur embeddings
- Pas de structural similarity (scene graphs)
- Pas d'abstract patterns inter-puzzles
- Pas de causal understanding

SOLUTION:
- Structural similarity (graph matching)
- Abstract pattern extraction
- Causal pattern recognition
- Multi-level analogical reasoning

Architecture:
1. StructuralMatcher: Compare scene graphs structurellement
2. AbstractPatternExtractor: Extrait patterns abstraits
3. CausalPatternRecognizer: Identifie causalité
4. AnalogicalReasoner: Raisonnement analogique multi-niveaux
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

from symbolic import Transform, TransformType, ComposedTransform
from objects import Object, SceneGraph, Relation, ShapeType, RelationType
from memory.episodic_memory import Episode, ConceptualPattern


@dataclass
class StructuralMapping:
    """
    Mapping structurel entre deux scene graphs
    
    Représente correspondances:
    - Objets source → objets target
    - Relations source → relations target
    - Score de similarité structurelle
    """
    source_episode_id: int
    target_episode_id: int
    object_mappings: Dict[int, int] = field(default_factory=dict)  # source_obj_id → target_obj_id
    relation_mappings: List[Tuple[int, int]] = field(default_factory=list)
    structural_similarity: float = 0.0
    shape_similarity: float = 0.0
    relation_similarity: float = 0.0
    spatial_similarity: float = 0.0
    
    def __repr__(self):
        return f"StructuralMapping(sim={self.structural_similarity:.3f}, objects={len(self.object_mappings)})"


@dataclass
class AbstractPattern:
    """
    Pattern abstrait extrait de multiples épisodes
    
    Représente concept de haut niveau:
    - "Objets de même couleur → grouper ensemble"
    - "Symétrie détectée → compléter symétrie"
    - "Pattern répété → extrapoler pattern"
    """
    id: int
    name: str
    description: str
    abstraction_level: str  # 'object', 'scene', 'global'
    preconditions: List[str] = field(default_factory=list)
    transformations: List[str] = field(default_factory=list)
    postconditions: List[str] = field(default_factory=list)
    frequency: int = 0
    success_rate: float = 0.0
    episodes: List[int] = field(default_factory=list)
    causal_score: float = 0.0  # Pourquoi ce pattern fonctionne
    
    def __repr__(self):
        return f"AbstractPattern({self.name}, level={self.abstraction_level}, freq={self.frequency}, causal={self.causal_score:.2f})"


class StructuralMatcher:
    """
    Comparaison structurelle de scene graphs
    
    Méthodes:
    - Graph isomorphism (exact matching)
    - Subgraph matching (partial matching)
    - Structural similarity scoring
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.comparisons = 0
    
    def compute_structural_similarity(self, sg1: SceneGraph, sg2: SceneGraph) -> StructuralMapping:
        """
        Calculer similarité structurelle entre deux scene graphs
        
        Stratégie:
        1. Comparer objets (shapes, couleurs, tailles)
        2. Comparer relations spatiales
        3. Trouver meilleur mapping
        4. Scorer similarité globale
        
        Returns:
            StructuralMapping avec score et correspondances
        """
        self.comparisons += 1
        
        # 1. Similarité objets
        object_mappings, shape_sim = self._match_objects(sg1, sg2)
        
        # 2. Similarité relations
        relation_mappings, relation_sim = self._match_relations(sg1, sg2, object_mappings)
        
        # 3. Similarité spatiale (positions relatives)
        spatial_sim = self._compute_spatial_similarity(sg1, sg2, object_mappings)
        
        # 4. Score global (moyenne pondérée)
        structural_sim = (
            0.4 * shape_sim +
            0.3 * relation_sim +
            0.3 * spatial_sim
        )
        
        mapping = StructuralMapping(
            source_episode_id=0,
            target_episode_id=0,
            object_mappings=object_mappings,
            relation_mappings=relation_mappings,
            structural_similarity=structural_sim,
            shape_similarity=shape_sim,
            relation_similarity=relation_sim,
            spatial_similarity=spatial_sim
        )
        
        return mapping
    
    def _match_objects(self, sg1: SceneGraph, sg2: SceneGraph) -> Tuple[Dict[int, int], float]:
        """
        Matcher objets entre deux scene graphs
        
        Returns:
            (mappings, similarity_score)
        """
        if not sg1.objects or not sg2.objects:
            return {}, 0.0
        
        # Matrice de similarité objets
        n1, n2 = len(sg1.objects), len(sg2.objects)
        similarity_matrix = np.zeros((n1, n2))
        
        for i, obj1 in enumerate(sg1.objects):
            for j, obj2 in enumerate(sg2.objects):
                sim = self._object_similarity(obj1, obj2)
                similarity_matrix[i, j] = sim
        
        # Greedy matching (meilleur match pour chaque objet)
        mappings = {}
        used_targets = set()
        
        # Trier par similarité décroissante
        matches = []
        for i in range(n1):
            for j in range(n2):
                matches.append((similarity_matrix[i, j], i, j))
        matches.sort(reverse=True)
        
        for sim, i, j in matches:
            if i not in mappings and j not in used_targets:
                if sim > 0.3:  # Seuil minimum
                    mappings[i] = j
                    used_targets.add(j)
        
        # Score global
        if mappings:
            total_sim = sum(similarity_matrix[i, j] for i, j in mappings.items())
            avg_sim = total_sim / max(n1, n2)
        else:
            avg_sim = 0.0
        
        return mappings, float(avg_sim)
    
    def _object_similarity(self, obj1: Object, obj2: Object) -> float:
        """Similarité entre deux objets"""
        score = 0.0
        
        # Shape (40%)
        if obj1.shape_type == obj2.shape_type:
            score += 0.4
        
        # Couleur (30%)
        if obj1.color == obj2.color:
            score += 0.3
        
        # Taille relative (20%)
        size1 = len(obj1.pixels)
        size2 = len(obj2.pixels)
        if size1 > 0 and size2 > 0:
            size_ratio = min(size1, size2) / max(size1, size2)
            score += 0.2 * size_ratio
        
        # Bounding box aspect ratio (10%)
        if obj1.bounding_box and obj2.bounding_box:
            h1, w1 = obj1.bounding_box[2] - obj1.bounding_box[0], obj1.bounding_box[3] - obj1.bounding_box[1]
            h2, w2 = obj2.bounding_box[2] - obj2.bounding_box[0], obj2.bounding_box[3] - obj2.bounding_box[1]
            if h1 > 0 and w1 > 0 and h2 > 0 and w2 > 0:
                ratio1 = h1 / w1
                ratio2 = h2 / w2
                ratio_sim = 1.0 - abs(ratio1 - ratio2) / max(ratio1, ratio2)
                score += 0.1 * ratio_sim
        
        return score
    
    def _match_relations(self, sg1: SceneGraph, sg2: SceneGraph,
                        object_mappings: Dict[int, int]) -> Tuple[List[Tuple[int, int]], float]:
        """
        Matcher relations entre scene graphs
        
        Returns:
            (relation_mappings, similarity_score)
        """
        if not sg1.relations or not sg2.relations or not object_mappings:
            return [], 0.0
        
        # Créer index relations par type
        relations1_by_type = defaultdict(list)
        for i, rel in enumerate(sg1.relations):
            relations1_by_type[rel.type].append((i, rel))
        
        relations2_by_type = defaultdict(list)
        for i, rel in enumerate(sg2.relations):
            relations2_by_type[rel.type].append((i, rel))
        
        # Matcher relations de même type
        mappings = []
        matched_count = 0
        
        for rel_type in relations1_by_type:
            if rel_type in relations2_by_type:
                rels1 = relations1_by_type[rel_type]
                rels2 = relations2_by_type[rel_type]
                
                for i1, rel1 in rels1:
                    for i2, rel2 in rels2:
                        # Vérifier si objets correspondent
                        obj1_idx = sg1.objects.index(rel1.object1) if rel1.object1 in sg1.objects else -1
                        obj2_idx = sg1.objects.index(rel1.object2) if rel1.object2 in sg1.objects else -1
                        
                        if obj1_idx in object_mappings and obj2_idx in object_mappings:
                            target_obj1_idx = object_mappings[obj1_idx]
                            target_obj2_idx = object_mappings[obj2_idx]
                            
                            # Vérifier correspondance dans sg2
                            if (target_obj1_idx < len(sg2.objects) and 
                                target_obj2_idx < len(sg2.objects)):
                                target_obj1 = sg2.objects[target_obj1_idx]
                                target_obj2 = sg2.objects[target_obj2_idx]
                                
                                if rel2.object1 == target_obj1 and rel2.object2 == target_obj2:
                                    mappings.append((i1, i2))
                                    matched_count += 1
                                    break
        
        # Score
        total_relations = max(len(sg1.relations), len(sg2.relations))
        similarity = matched_count / total_relations if total_relations > 0 else 0.0
        
        return mappings, float(similarity)
    
    def _compute_spatial_similarity(self, sg1: SceneGraph, sg2: SceneGraph,
                                    object_mappings: Dict[int, int]) -> float:
        """
        Calculer similarité spatiale (positions relatives)
        """
        if not object_mappings:
            return 0.0
        
        # Comparer positions relatives des objets mappés
        similarities = []
        
        for i1, i2 in object_mappings.items():
            if i1 >= len(sg1.objects) or i2 >= len(sg2.objects):
                continue
            
            obj1 = sg1.objects[i1]
            obj2 = sg2.objects[i2]
            
            if not obj1.bounding_box or not obj2.bounding_box:
                continue
            
            # Position relative dans la grille
            y1, x1 = (obj1.bounding_box[0] + obj1.bounding_box[2]) / 2, (obj1.bounding_box[1] + obj1.bounding_box[3]) / 2
            y2, x2 = (obj2.bounding_box[0] + obj2.bounding_box[2]) / 2, (obj2.bounding_box[1] + obj2.bounding_box[3]) / 2
            
            # Normaliser par taille grille
            if hasattr(sg1, 'grid_shape') and hasattr(sg2, 'grid_shape'):
                y1_norm = y1 / sg1.grid_shape[0]
                x1_norm = x1 / sg1.grid_shape[1]
                y2_norm = y2 / sg2.grid_shape[0]
                x2_norm = x2 / sg2.grid_shape[1]
                
                # Distance euclidienne normalisée
                dist = np.sqrt((y1_norm - y2_norm)**2 + (x1_norm - x2_norm)**2)
                sim = 1.0 - min(dist, 1.0)
                similarities.append(sim)
        
        return float(np.mean(similarities)) if similarities else 0.0
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'comparisons': self.comparisons
        }


class AbstractPatternExtractor:
    """
    Extraction de patterns abstraits inter-puzzles
    
    Identifie concepts de haut niveau:
    - Object-level: "Objets rouges → déplacer droite"
    - Scene-level: "Symétrie → compléter"
    - Global-level: "Pattern répété → extrapoler"
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.patterns_extracted = 0
    
    def extract_abstract_patterns(self, episodes: List[Episode],
                                  structural_mappings: List[StructuralMapping],
                                  min_frequency: int = 2) -> List[AbstractPattern]:
        """
        Extraire patterns abstraits de multiples épisodes
        
        Stratégie:
        1. Grouper épisodes par similarité structurelle
        2. Identifier préconditions communes
        3. Identifier transformations communes
        4. Identifier postconditions communes
        5. Créer pattern abstrait
        
        Returns:
            Liste de patterns abstraits
        """
        patterns = []
        pattern_id = 1
        
        # Grouper épisodes similaires structurellement
        episode_groups = self._group_similar_episodes(episodes, structural_mappings)
        
        for group in episode_groups:
            if len(group) < min_frequency:
                continue
            
            # Analyser groupe
            pattern = self._analyze_episode_group(group, pattern_id)
            
            if pattern:
                patterns.append(pattern)
                pattern_id += 1
                self.patterns_extracted += 1
        
        return patterns
    
    def _group_similar_episodes(self, episodes: List[Episode],
                               mappings: List[StructuralMapping]) -> List[List[Episode]]:
        """Grouper épisodes similaires"""
        # Créer graphe de similarité
        similarity_graph = defaultdict(list)
        
        for mapping in mappings:
            if mapping.structural_similarity > 0.6:  # Seuil
                similarity_graph[mapping.source_episode_id].append(mapping.target_episode_id)
                similarity_graph[mapping.target_episode_id].append(mapping.source_episode_id)
        
        # Trouver composantes connexes (groupes)
        visited = set()
        groups = []
        
        episode_dict = {ep.id: ep for ep in episodes}
        
        for ep_id in similarity_graph:
            if ep_id not in visited:
                group = self._dfs_group(ep_id, similarity_graph, visited)
                group_episodes = [episode_dict[eid] for eid in group if eid in episode_dict]
                if group_episodes:
                    groups.append(group_episodes)
        
        return groups
    
    def _dfs_group(self, start: int, graph: Dict[int, List[int]], visited: Set[int]) -> List[int]:
        """DFS pour trouver composante connexe"""
        stack = [start]
        group = []
        
        while stack:
            node = stack.pop()
            if node not in visited:
                visited.add(node)
                group.append(node)
                stack.extend(graph[node])
        
        return group
    
    def _analyze_episode_group(self, episodes: List[Episode], pattern_id: int) -> Optional[AbstractPattern]:
        """Analyser groupe d'épisodes pour extraire pattern"""
        if not episodes:
            return None
        
        # Préconditions communes (scene graph features)
        preconditions = self._extract_common_preconditions(episodes)
        
        # Transformations communes
        transformations = self._extract_common_transformations(episodes)
        
        # Postconditions communes
        postconditions = self._extract_common_postconditions(episodes)
        
        # Déterminer niveau d'abstraction
        abstraction_level = self._determine_abstraction_level(transformations)
        
        # Calculer métriques
        successes = sum(1 for ep in episodes if ep.success)
        success_rate = successes / len(episodes)
        
        # Causal score (pourquoi ça marche)
        causal_score = self._compute_causal_score(preconditions, transformations, postconditions)
        
        pattern = AbstractPattern(
            id=pattern_id,
            name=f"AbstractPattern_{pattern_id}",
            description=f"{abstraction_level}: {' → '.join(transformations[:3])}",
            abstraction_level=abstraction_level,
            preconditions=preconditions,
            transformations=transformations,
            postconditions=postconditions,
            frequency=len(episodes),
            success_rate=success_rate,
            episodes=[ep.id for ep in episodes],
            causal_score=causal_score
        )
        
        return pattern
    
    def _extract_common_preconditions(self, episodes: List[Episode]) -> List[str]:
        """Extraire préconditions communes"""
        preconditions = []
        
        # Analyser scene graphs input
        if all(ep.input_scene_graph for ep in episodes):
            # Shapes communes
            shape_counts = defaultdict(int)
            for ep in episodes:
                if ep.input_scene_graph:
                    for obj in ep.input_scene_graph.objects:
                        shape_counts[obj.shape_type.value] += 1
            
            common_shapes = [shape for shape, count in shape_counts.items() 
                           if count >= len(episodes) * 0.7]
            
            for shape in common_shapes:
                preconditions.append(f"has_{shape}")
        
        return preconditions
    
    def _extract_common_transformations(self, episodes: List[Episode]) -> List[str]:
        """Extraire transformations communes"""
        # Compter types de transformations
        transform_counts = defaultdict(int)
        
        for ep in episodes:
            for t in ep.transformations:
                if hasattr(t, 'type'):
                    transform_counts[t.type.value] += 1
                elif hasattr(t, 'transform_type'):
                    transform_counts[t.transform_type] += 1
        
        # Garder transformations fréquentes
        common_transforms = [t for t, count in transform_counts.items()
                           if count >= len(episodes) * 0.5]
        
        return common_transforms
    
    def _extract_common_postconditions(self, episodes: List[Episode]) -> List[str]:
        """Extraire postconditions communes"""
        postconditions = []
        
        # Analyser outputs
        if all(ep.output_scene_graph for ep in episodes):
            # Nombre d'objets
            obj_counts = [len(ep.output_scene_graph.objects) for ep in episodes if ep.output_scene_graph]
            avg_count = np.mean(obj_counts)
            postconditions.append(f"objects_count≈{int(avg_count)}")
        
        return postconditions
    
    def _determine_abstraction_level(self, transformations: List[str]) -> str:
        """Déterminer niveau d'abstraction"""
        object_level = ['recolor', 'translate', 'scale_up', 'scale_down']
        scene_level = ['extract_objects', 'flood_fill', 'detect_symmetry']
        global_level = ['rotate_90', 'flip_horizontal', 'repeat_pattern']
        
        object_count = sum(1 for t in transformations if t in object_level)
        scene_count = sum(1 for t in transformations if t in scene_level)
        global_count = sum(1 for t in transformations if t in global_level)
        
        if global_count >= scene_count and global_count >= object_count:
            return 'global'
        elif scene_count >= object_count:
            return 'scene'
        else:
            return 'object'
    
    def _compute_causal_score(self, preconditions: List[str],
                             transformations: List[str],
                             postconditions: List[str]) -> float:
        """
        Calculer score causal (pourquoi ce pattern fonctionne)
        
        Heuristiques:
        - Préconditions claires → +0.3
        - Transformations cohérentes → +0.4
        - Postconditions vérifiables → +0.3
        """
        score = 0.0
        
        # Préconditions
        if len(preconditions) >= 2:
            score += 0.3
        
        # Transformations
        if len(transformations) >= 1 and len(transformations) <= 5:
            score += 0.4
        
        # Postconditions
        if len(postconditions) >= 1:
            score += 0.3
        
        return score
    
    def get_statistics(self) -> Dict[str, int]:
        """Statistiques"""
        return {
            'patterns_extracted': self.patterns_extracted
        }


# Made with Bob - Expert en Analogical Reasoning, Cognitive Science, Graph Theory, Causal Inference