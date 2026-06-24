"""
CAUSAL TRANSITION GRAPH - MAGEN V25
Graphe transitions causales pour planification dirigée objectif

PROBLÈME V24: Agent explore sans mémoire trajectoire → cycles + exploration aveugle
SOLUTION V25: Graphe états-actions-effets + planification BFS/MCTS

Architecture:
- Nodes: États environnement (embeddings causaux)
- Edges: Actions + effets observés
- Goals: États cibles (reward élevé, niveau complété)
- Planning: BFS, MCTS, planification hiérarchique

FORENSIC: Intégré via ForensicMiddleware (PROTOCOLE_MAGEN V3.0)
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass, field
from collections import defaultdict, deque
import time
from enum import Enum

from .forensic_middleware import ForensicMiddleware, CausalIDManager

class NodeType(Enum):
    """Type nœud graphe"""
    NORMAL = "normal"
    GOAL = "goal"
    DEAD_END = "dead_end"
    CHECKPOINT = "checkpoint"

@dataclass
class GraphNode:
    """Nœud graphe = état environnement"""
    id: str  # Hash embedding
    embedding: np.ndarray  # (128,) embedding causal
    visit_count: int = 0
    total_reward: float = 0.0
    avg_reward: float = 0.0
    node_type: NodeType = NodeType.NORMAL
    metadata: Dict = field(default_factory=dict)
    timestamp_ns: int = 0
    
    def update_reward(self, reward: float):
        """Mise à jour reward après visite"""
        self.visit_count += 1
        self.total_reward += reward
        self.avg_reward = self.total_reward / self.visit_count
        
        # Détection goal (reward élevé)
        if self.avg_reward > 0.5 and self.visit_count >= 3:
            self.node_type = NodeType.GOAL
        # Détection dead-end (reward négatif persistant)
        elif self.avg_reward < -0.3 and self.visit_count >= 5:
            self.node_type = NodeType.DEAD_END

@dataclass
class GraphEdge:
    """Arête graphe = transition causale"""
    from_node: str  # ID nœud source
    to_node: str  # ID nœud destination
    action: str  # Action exécutée
    semantic_action: str  # Nom sémantique action
    reward: float  # Reward observé
    success: bool  # Si transition réussie
    visit_count: int = 0
    success_count: int = 0
    success_rate: float = 0.0
    avg_reward: float = 0.0
    reputation: float = 1.0  # Réputation transition (0.0 = mauvaise, 1.0 = bonne)
    penalty_count: int = 0  # Nombre pénalités appliquées
    timestamp_ns: int = 0
    
    def update(self, reward: float, success: bool):
        """Mise à jour statistiques après utilisation"""
        self.visit_count += 1
        if success:
            self.success_count += 1
        self.success_rate = self.success_count / self.visit_count if self.visit_count > 0 else 0.0
        self.avg_reward = ((self.avg_reward * (self.visit_count - 1)) + reward) / self.visit_count
    
    def apply_penalty(self, penalty_factor: float = 0.1):
        """
        Applique pénalité réputation pour transition invalide
        
        Args:
            penalty_factor: Facteur pénalité (0.1 = -10% réputation)
        """
        self.reputation = max(0.0, self.reputation - penalty_factor)
        self.penalty_count += 1
    
    def boost_reputation(self, boost_factor: float = 0.05):
        """
        Augmente réputation pour transition réussie
        
        Args:
            boost_factor: Facteur boost (0.05 = +5% réputation)
        """
        self.reputation = min(1.0, self.reputation + boost_factor)

@dataclass
class Path:
    """Chemin dans graphe"""
    nodes: List[str]  # Séquence IDs nœuds
    actions: List[str]  # Séquence actions
    total_reward: float = 0.0
    length: int = 0
    success_probability: float = 0.0
    
    def __post_init__(self):
        self.length = len(self.actions)

class CausalTransitionGraph:
    """
    Graphe transitions causales pour planification
    
    Remplace exploration aveugle (V24) par planification dirigée (V25)
    """
    
    def __init__(
        self,
        similarity_threshold: float = 0.85,
        max_nodes: int = 10000,
        max_edges_per_node: int = 50,
        forensic_log_path: Optional[str] = None,
        id_manager: Optional[CausalIDManager] = None
    ):
        self.similarity_threshold = similarity_threshold
        self.max_nodes = max_nodes
        self.max_edges_per_node = max_edges_per_node
        
        # Graphe
        self.nodes: Dict[str, GraphNode] = {}
        self.edges: Dict[Tuple[str, str], GraphEdge] = {}  # (from_id, to_id) → edge
        self.adjacency: Dict[str, List[str]] = defaultdict(list)  # from_id → [to_ids]
        
        # Goals détectés
        self.goal_nodes: Set[str] = set()
        self.checkpoint_nodes: Set[str] = set()
        
        # Trajectoire courante
        self.current_trajectory: List[Tuple[str, str]] = []  # [(node_id, action)]
        
        # Statistiques forensiques
        self.stats = {
            'total_nodes': 0,
            'total_edges': 0,
            'goal_nodes_count': 0,
            'dead_end_nodes_count': 0,
            'avg_node_degree': 0.0,
            'max_path_length': 0,
            'total_paths_found': 0,
            'avg_path_length': 0.0,
            'planning_time_ns': []
        }
        
        # FORENSIC: Middleware standardisé (PROTOCOLE_MAGEN V3.0)
        self.forensic: Optional[ForensicMiddleware] = None
        if forensic_log_path:
            self.forensic = ForensicMiddleware(
                forensic_log_path,
                "CausalTransitionGraph",
                id_manager
            )
        
        print("[CAUSAL GRAPH] Initialisé")
        print(f"  Similarity threshold: {similarity_threshold}")
        print(f"  Max nodes: {max_nodes}")
        print(f"  Max edges per node: {max_edges_per_node}")
    
    def _compute_node_id(self, embedding: np.ndarray) -> str:
        """Calcule ID unique nœud depuis embedding"""
        # Hash embedding pour ID unique
        # Utilise quantization pour regrouper embeddings similaires
        quantized = (embedding * 100).astype(np.int32)
        return str(hash(quantized.tobytes()))
    
    def _find_similar_node(self, embedding: np.ndarray) -> Optional[str]:
        """Trouve nœud similaire existant (si existe)"""
        for node_id, node in self.nodes.items():
            similarity = np.dot(embedding, node.embedding)
            if similarity >= self.similarity_threshold:
                return node_id
        return None
    
    def add_state(
        self,
        embedding: np.ndarray,
        metadata: Optional[Dict] = None
    ) -> str:
        """
        Ajoute état au graphe (ou retourne ID si existe)
        
        Args:
            embedding: Embedding causal état (128D)
            metadata: Métadonnées additionnelles
        
        Returns:
            ID nœud
        """
        # Chercher nœud similaire existant
        node_id = self._find_similar_node(embedding)
        
        if node_id is None:
            # Créer nouveau nœud
            node_id = self._compute_node_id(embedding)
            
            # Vérifier limite
            if len(self.nodes) >= self.max_nodes:
                # Supprimer nœud le moins visité
                least_visited = min(self.nodes.values(), key=lambda n: n.visit_count)
                del self.nodes[least_visited.id]
                self.stats['total_nodes'] -= 1
            
            self.nodes[node_id] = GraphNode(
                id=node_id,
                embedding=embedding.copy(),
                metadata=metadata or {},
                timestamp_ns=time.perf_counter_ns()
            )
            
            self.stats['total_nodes'] += 1
            
            # FORENSIC: Log node creation
            if self.forensic:
                self.forensic.log_event('node_created', {
                    'node_id': node_id,
                    'timestamp_ns': int(time.perf_counter_ns()),
                    'total_nodes': self.stats['total_nodes'],
                    'embedding_shape': list(embedding.shape),
                    'metadata': metadata or {}
                })
        
        return node_id
    
    def add_transition(
        self,
        from_embedding: np.ndarray,
        action: str,
        semantic_action: str,
        to_embedding: np.ndarray,
        reward: float,
        success: bool,
        metadata: Optional[Dict] = None
    ):
        """
        Ajoute transition causale au graphe
        
        Args:
            from_embedding: État avant action
            action: Action exécutée
            semantic_action: Nom sémantique action
            to_embedding: État après action
            reward: Reward observé
            success: Si transition réussie
            metadata: Métadonnées additionnelles
        """
        # Ajouter/récupérer nœuds
        from_id = self.add_state(from_embedding, metadata)
        to_id = self.add_state(to_embedding, metadata)
        
        # Mettre à jour rewards nœuds
        self.nodes[from_id].update_reward(reward)
        self.nodes[to_id].update_reward(reward)
        
        # Créer/mettre à jour arête
        edge_key = (from_id, to_id)
        
        if edge_key in self.edges:
            # Mettre à jour arête existante
            edge = self.edges[edge_key]
            edge.update(reward, success)
            
            # Gestion réputation basée sur résultat
            if success and reward > 0:
                # Boost réputation pour succès
                edge.boost_reputation(boost_factor=0.05)
            elif not success or reward < -0.5:
                # Pénalité pour échec ou reward très négatif
                edge.apply_penalty(penalty_factor=0.1)
            
            # Pénalité supplémentaire si mène à dead-end
            if self.nodes[to_id].node_type == NodeType.DEAD_END:
                edge.apply_penalty(penalty_factor=0.2)
                
                # FORENSIC: Log penalty application
                if self.forensic:
                    self.forensic.log_event('transition_penalty_applied', {
                        'from_node': from_id,
                        'to_node': to_id,
                        'action': action,
                        'reason': 'leads_to_dead_end',
                        'new_reputation': float(edge.reputation),
                        'penalty_count': edge.penalty_count,
                        'timestamp_ns': int(time.perf_counter_ns())
                    })
        else:
            # Créer nouvelle arête
            self.edges[edge_key] = GraphEdge(
                from_node=from_id,
                to_node=to_id,
                action=action,
                semantic_action=semantic_action,
                reward=reward,
                success=success,
                visit_count=1,
                success_count=1 if success else 0,
                success_rate=1.0 if success else 0.0,
                avg_reward=reward,
                timestamp_ns=time.perf_counter_ns()
            )
            
            # Mettre à jour adjacence
            self.adjacency[from_id].append(to_id)
            
            # Limiter edges par nœud
            if len(self.adjacency[from_id]) > self.max_edges_per_node:
                # Supprimer edge le moins utilisé
                edges_from_node = [(to, self.edges[(from_id, to)]) for to in self.adjacency[from_id]]
                least_used = min(edges_from_node, key=lambda x: x[1].visit_count)
                self.adjacency[from_id].remove(least_used[0])
                del self.edges[(from_id, least_used[0])]
            
            self.stats['total_edges'] += 1
            
            # FORENSIC: Log edge creation
            if self.forensic:
                self.forensic.log_event('edge_created', {
                    'from_node': from_id,
                    'to_node': to_id,
                    'action': action,
                    'semantic_action': semantic_action,
                    'reward': float(reward),
                    'success': bool(success),
                    'timestamp_ns': int(time.perf_counter_ns()),
                    'total_edges': self.stats['total_edges']
                })
        
        # Mettre à jour trajectoire courante
        self.current_trajectory.append((from_id, action))
        
        # Détecter goals
        if self.nodes[to_id].node_type == NodeType.GOAL:
            self.goal_nodes.add(to_id)
            self.stats['goal_nodes_count'] = len(self.goal_nodes)
            
            # FORENSIC: Log goal detection
            if self.forensic:
                self.forensic.log_event('goal_detected', {
                    'node_id': to_id,
                    'timestamp_ns': int(time.perf_counter_ns()),
                    'avg_reward': float(self.nodes[to_id].avg_reward),
                    'visit_count': int(self.nodes[to_id].visit_count),
                    'total_goals': len(self.goal_nodes)
                })
        
        # Détecter dead-ends
        if self.nodes[to_id].node_type == NodeType.DEAD_END:
            self.stats['dead_end_nodes_count'] += 1
            
            # FORENSIC: Log dead-end detection
            if self.forensic:
                self.forensic.log_event('dead_end_detected', {
                    'node_id': to_id,
                    'timestamp_ns': int(time.perf_counter_ns()),
                    'avg_reward': float(self.nodes[to_id].avg_reward),
                    'visit_count': int(self.nodes[to_id].visit_count)
                })
    
    def find_path_bfs(
        self,
        start_embedding: np.ndarray,
        goal_type: NodeType = NodeType.GOAL,
        max_depth: int = 50
    ) -> Optional[Path]:
        """
        Recherche chemin vers objectif via BFS
        
        Args:
            start_embedding: État départ
            goal_type: Type nœud cible
            max_depth: Profondeur max recherche
        
        Returns:
            Path si trouvé, None sinon
        """
        start_ns = time.perf_counter_ns()
        
        # Trouver nœud départ
        start_id = self._find_similar_node(start_embedding)
        if start_id is None:
            return None
        
        # BFS
        queue = deque([(start_id, [], [])])  # (node_id, path_nodes, path_actions)
        visited = {start_id}
        
        while queue:
            current_id, path_nodes, path_actions = queue.popleft()
            
            # Vérifier profondeur
            if len(path_actions) >= max_depth:
                continue
            
            # Vérifier si goal atteint
            if self.nodes[current_id].node_type == goal_type:
                # Calculer statistiques chemin
                total_reward = sum(
                    self.edges.get((path_nodes[i], path_nodes[i+1]), GraphEdge("", "", "", "", 0, False)).avg_reward
                    for i in range(len(path_nodes)-1)
                )
                
                success_prob = np.prod([
                    self.edges.get((path_nodes[i], path_nodes[i+1]), GraphEdge("", "", "", "", 0, False)).success_rate
                    for i in range(len(path_nodes)-1)
                ]) if len(path_nodes) > 1 else 1.0
                
                path = Path(
                    nodes=path_nodes + [current_id],
                    actions=path_actions,
                    total_reward=total_reward,
                    success_probability=float(success_prob)  # Conversion explicite np.floating → float
                )
                
                # Statistiques
                planning_time_ns = time.perf_counter_ns() - start_ns
                self.stats['planning_time_ns'].append(planning_time_ns)
                self.stats['total_paths_found'] += 1
                self.stats['max_path_length'] = max(self.stats['max_path_length'], path.length)
                
                if self.stats['total_paths_found'] > 0:
                    self.stats['avg_path_length'] = (
                        (self.stats['avg_path_length'] * (self.stats['total_paths_found'] - 1) + path.length)
                        / self.stats['total_paths_found']
                    )
                
                # FORENSIC: Log path found
                if self.forensic:
                    self.forensic.log_event('path_found', {
                        'timestamp_ns': int(time.perf_counter_ns()),
                        'path_length': path.length,
                        'total_reward': float(total_reward),
                        'success_probability': float(success_prob),
                        'planning_time_ns': planning_time_ns,
                        'nodes_visited': len(visited)
                    })
                
                print(f"[CAUSAL GRAPH] Chemin trouvé: {path.length} actions, reward={total_reward:.2f}, prob={success_prob:.2%}")
                
                return path
            
            # Explorer voisins
            for neighbor_id in self.adjacency.get(current_id, []):
                if neighbor_id not in visited:
                    visited.add(neighbor_id)
                    
                    # Récupérer action
                    edge = self.edges.get((current_id, neighbor_id))
                    if edge:
                        queue.append((
                            neighbor_id,
                            path_nodes + [current_id],
                            path_actions + [edge.action]
                        ))
        
        # Aucun chemin trouvé
        planning_time_ns = time.perf_counter_ns() - start_ns
        self.stats['planning_time_ns'].append(planning_time_ns)
        
        return None
    
    def get_best_action(
        self,
        current_embedding: np.ndarray,
        available_actions: List[str]
    ) -> Optional[str]:
        """
        Retourne meilleure action depuis état actuel
        
        Utilise statistiques edges pour choisir action prometteuse
        
        Args:
            current_embedding: État actuel
            available_actions: Actions disponibles
        
        Returns:
            Action recommandée ou None
        """
        # Trouver nœud actuel
        current_id = self._find_similar_node(current_embedding)
        if current_id is None:
            return None
        
        # Évaluer actions disponibles
        action_scores = {}
        
        for neighbor_id in self.adjacency.get(current_id, []):
            edge = self.edges.get((current_id, neighbor_id))
            if edge and edge.action in available_actions:
                # Score = success_rate × avg_reward × reputation × (1 + visit_bonus)
                # Réputation pénalise transitions vers dead-ends
                visit_bonus = np.log1p(edge.visit_count) / 10.0
                score = edge.success_rate * edge.avg_reward * edge.reputation * (1 + visit_bonus)
                action_scores[edge.action] = score
        
        if action_scores:
            best_action = max(action_scores.items(), key=lambda x: x[1])[0]
            return best_action
        
        return None
    
    def detect_cycles(self, window: int = 10) -> List[Dict]:
        """
        Détecte cycles dans trajectoire courante
        
        Args:
            window: Taille fenêtre analyse
        
        Returns:
            Liste cycles détectés
        """
        if len(self.current_trajectory) < window:
            return []
        
        cycles = []
        recent_trajectory = self.current_trajectory[-window:]
        
        # Chercher patterns répétitifs
        for period in range(2, window // 2):
            is_cycle = True
            for i in range(period, len(recent_trajectory)):
                if recent_trajectory[i] != recent_trajectory[i % period]:
                    is_cycle = False
                    break
            
            if is_cycle:
                cycles.append({
                    'period': period,
                    'pattern': recent_trajectory[:period],
                    'repetitions': len(recent_trajectory) // period
                })
        
        return cycles
    
    def reset_trajectory(self):
        """Réinitialise trajectoire courante"""
        # FORENSIC: Log trajectory reset
        if self.forensic:
            self.forensic.log_event('trajectory_reset', {
                'timestamp_ns': int(time.perf_counter_ns()),
                'trajectory_length_before': len(self.current_trajectory)
            })
        
        self.current_trajectory = []
    
    def get_stats(self) -> Dict:
        """Statistiques forensiques"""
        stats = self.stats.copy()
        
        # Calcul degré moyen
        if self.nodes:
            stats['avg_node_degree'] = sum(len(neighbors) for neighbors in self.adjacency.values()) / len(self.nodes)
        
        # Temps planification moyen
        if stats['planning_time_ns']:
            stats['avg_planning_time_us'] = np.mean(stats['planning_time_ns']) / 1000
            stats['max_planning_time_us'] = np.max(stats['planning_time_ns']) / 1000
        
        # Statistiques réputation
        if self.edges:
            reputations = [edge.reputation for edge in self.edges.values()]
            penalties = [edge.penalty_count for edge in self.edges.values()]
            stats['avg_edge_reputation'] = float(np.mean(reputations))
            stats['min_edge_reputation'] = float(np.min(reputations))
            stats['total_penalties_applied'] = int(np.sum(penalties))
            stats['edges_with_penalties'] = sum(1 for p in penalties if p > 0)
        
        return stats
    
    def print_stats(self):
        """Affiche statistiques forensiques"""
        stats = self.get_stats()
        print("\n[CAUSAL GRAPH] Statistiques:")
        print(f"  Total nodes: {stats['total_nodes']}")
        print(f"  Total edges: {stats['total_edges']}")
        print(f"  Goal nodes: {stats['goal_nodes_count']}")
        print(f"  Dead-end nodes: {stats['dead_end_nodes_count']}")
        print(f"  Avg node degree: {stats['avg_node_degree']:.2f}")
        print(f"  Total paths found: {stats['total_paths_found']}")
        if stats['total_paths_found'] > 0:
            print(f"  Avg path length: {stats['avg_path_length']:.1f}")
            print(f"  Max path length: {stats['max_path_length']}")
        if 'avg_planning_time_us' in stats:
            print(f"  Avg planning time: {stats['avg_planning_time_us']:.1f}µs")
        if 'avg_edge_reputation' in stats:
            print(f"  Avg edge reputation: {stats['avg_edge_reputation']:.2f}")
            print(f"  Total penalties applied: {stats['total_penalties_applied']}")
            print(f"  Edges with penalties: {stats['edges_with_penalties']}/{stats['total_edges']}")

# Made with Bob