"""
Hierarchical Planner - Planification Hiérarchique (Phase 4.5.4 - Couche 5)

Objectif: Planifier actions pour atteindre objectifs

Méthode:
- Décomposer objectif global en sous-objectifs
- Recherche A* pour trouver chemin optimal
- Utiliser WorldModel + RulesLearning pour prédire
- Gérer hiérarchie objectifs (priorités)
- Replanning si échec

Exemple ls20:
- Objectif: Atteindre porte
- Sous-objectifs: 
  1. Identifier avatar (SelfIdentification)
  2. Localiser porte (WorldModel)
  3. Calculer chemin (A*)
  4. Exécuter actions séquentielles

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.4
"""

import numpy as np
from collections import deque
from typing import Dict, List, Tuple, Optional, Set, Any
from dataclasses import dataclass, field
import heapq


@dataclass
class Goal:
    """Objectif à atteindre"""
    type: str  # "reach_position", "reach_entity", "maximize_reward", "explore"
    target: Any  # Position cible, entité cible, etc.
    priority: float = 1.0
    deadline: Optional[int] = None  # Step limite (optionnel)
    parent_goal: Optional['Goal'] = None  # Objectif parent (hiérarchie)
    sub_goals: List['Goal'] = field(default_factory=list)
    status: str = "pending"  # "pending", "active", "completed", "failed"
    
    def __repr__(self):
        return f"Goal({self.type}, target={self.target}, priority={self.priority:.2f}, status={self.status})"


@dataclass
class Plan:
    """Plan d'actions"""
    actions: List[int]  # Séquence d'actions
    expected_states: List[Dict]  # États attendus après chaque action
    cost: float  # Coût total du plan
    confidence: float  # Confiance dans le plan
    goal: Goal  # Objectif associé
    
    def __repr__(self):
        return f"Plan({len(self.actions)} actions, cost={self.cost:.2f}, conf={self.confidence:.2%})"


@dataclass
class SearchNode:
    """Nœud pour recherche A*"""
    state_hash: str
    position: Tuple[float, float]  # Position avatar
    g_cost: float  # Coût depuis départ
    h_cost: float  # Heuristique vers objectif
    parent: Optional['SearchNode'] = None
    action: Optional[int] = None
    
    @property
    def f_cost(self):
        """Coût total f = g + h"""
        return self.g_cost + self.h_cost
    
    def __lt__(self, other):
        return self.f_cost < other.f_cost
    
    def __repr__(self):
        return f"Node(pos={self.position}, f={self.f_cost:.2f})"


class HierarchicalPlanner:
    """
    Planificateur hiérarchique avec recherche A*
    
    Principe:
    1. Recevoir objectif global
    2. Décomposer en sous-objectifs
    3. Pour chaque sous-objectif, planifier avec A*
    4. Exécuter plan séquentiellement
    5. Replanning si échec
    """
    
    def __init__(self, world_model=None, rules_learning=None, world_dynamics=None):
        # Modules systémiques
        self.world_model = world_model
        self.rules_learning = rules_learning
        self.world_dynamics = world_dynamics
        
        # Objectifs
        self.goals: List[Goal] = []
        self.active_goal: Optional[Goal] = None
        
        # Plans
        self.current_plan: Optional[Plan] = None
        self.plan_history: List[Plan] = []
        
        # Statistiques
        self.plans_generated = 0
        self.plans_succeeded = 0
        self.plans_failed = 0
        self.replanning_count = 0
        
        # Paramètres A*
        self.max_search_depth = 50  # Profondeur max recherche
        self.max_nodes_explored = 1000  # Nœuds max explorés
        self.action_cost = 1.0  # Coût par action
        
        # Actions disponibles (ls20)
        self.available_actions = [0, 1, 2, 3, 4, 5]  # NOOP, RIGHT, UP, LEFT, DOWN, USE
    
    def add_goal(self, goal: Goal):
        """Ajouter objectif"""
        self.goals.append(goal)
        # Trier par priorité
        self.goals.sort(key=lambda g: g.priority, reverse=True)
    
    def decompose_goal(self, goal: Goal) -> List[Goal]:
        """
        Décomposer objectif en sous-objectifs
        
        Exemple: "reach_entity" → ["identify_avatar", "locate_target", "plan_path"]
        """
        sub_goals = []
        
        if goal.type == "reach_entity":
            # Sous-objectif 1: Identifier avatar
            sub_goals.append(Goal(
                type="identify_avatar",
                target=None,
                priority=goal.priority + 0.1,
                parent_goal=goal
            ))
            
            # Sous-objectif 2: Localiser entité cible
            sub_goals.append(Goal(
                type="locate_entity",
                target=goal.target,
                priority=goal.priority,
                parent_goal=goal
            ))
            
            # Sous-objectif 3: Atteindre position
            sub_goals.append(Goal(
                type="reach_position",
                target=None,  # Sera défini après localisation
                priority=goal.priority,
                parent_goal=goal
            ))
        
        elif goal.type == "maximize_reward":
            # Sous-objectif 1: Explorer pour trouver récompenses
            sub_goals.append(Goal(
                type="explore",
                target="high_reward_areas",
                priority=goal.priority,
                parent_goal=goal
            ))
            
            # Sous-objectif 2: Atteindre meilleure récompense
            sub_goals.append(Goal(
                type="reach_position",
                target=None,  # Sera défini après exploration
                priority=goal.priority + 0.2,
                parent_goal=goal
            ))
        
        # Ajouter sous-objectifs au goal parent
        goal.sub_goals = sub_goals
        
        return sub_goals
    
    def plan_to_goal(self, goal: Goal, current_state: Dict) -> Optional[Plan]:
        """
        Planifier pour atteindre objectif
        
        Utilise A* pour trouver chemin optimal
        """
        if goal.type == "reach_position":
            return self.plan_reach_position(goal, current_state)
        elif goal.type == "reach_entity":
            return self.plan_reach_entity(goal, current_state)
        elif goal.type == "explore":
            return self.plan_exploration(goal, current_state)
        else:
            # Type objectif non supporté
            return None
    
    def plan_reach_position(self, goal: Goal, current_state: Dict) -> Optional[Plan]:
        """
        Planifier pour atteindre position cible
        
        Utilise A* avec heuristique distance euclidienne
        """
        # Extraire position avatar actuelle
        avatar_pos = self.get_avatar_position(current_state)
        if avatar_pos is None:
            return None
        
        target_pos = goal.target
        if target_pos is None:
            return None
        
        # Recherche A*
        path = self.astar_search(avatar_pos, target_pos, current_state)
        
        if path is None:
            return None
        
        # Convertir chemin en actions
        actions = self.path_to_actions(path)
        
        # Créer plan
        plan = Plan(
            actions=actions,
            expected_states=[],  # Sera rempli si nécessaire
            cost=len(actions) * self.action_cost,
            confidence=0.8,  # Confiance par défaut
            goal=goal
        )
        
        self.plans_generated += 1
        return plan
    
    def plan_reach_entity(self, goal: Goal, current_state: Dict) -> Optional[Plan]:
        """
        Planifier pour atteindre entité
        
        1. Localiser entité
        2. Planifier vers position entité
        """
        entity_id = goal.target
        
        # Trouver position entité
        entities = current_state.get('entities', {})
        if entity_id not in entities:
            return None
        
        entity = entities[entity_id]
        target_pos = entity.centroid
        
        # Créer sous-objectif reach_position
        sub_goal = Goal(
            type="reach_position",
            target=target_pos,
            priority=goal.priority,
            parent_goal=goal
        )
        
        return self.plan_reach_position(sub_goal, current_state)
    
    def plan_exploration(self, goal: Goal, current_state: Dict) -> Optional[Plan]:
        """
        Planifier exploration
        
        Stratégie simple: aller vers zones non explorées
        """
        # Pour l'instant, exploration aléatoire dirigée
        # Sera amélioré avec IntelligentExploration (Couche 6)
        
        avatar_pos = self.get_avatar_position(current_state)
        if avatar_pos is None:
            return None
        
        # Choisir direction exploration (opposée à position actuelle)
        grid_center = (50.0, 50.0)  # Centre grille ls20 (100x100)
        
        # Aller vers centre si loin, sinon explorer bords
        distance_to_center = np.sqrt((avatar_pos[0] - grid_center[0])**2 + 
                                     (avatar_pos[1] - grid_center[1])**2)
        
        if distance_to_center > 30:
            target_pos = grid_center
        else:
            # Explorer vers bord aléatoire
            target_pos = (np.random.uniform(10, 90), np.random.uniform(10, 90))
        
        # Créer sous-objectif reach_position
        sub_goal = Goal(
            type="reach_position",
            target=target_pos,
            priority=goal.priority,
            parent_goal=goal
        )
        
        return self.plan_reach_position(sub_goal, current_state)
    
    def astar_search(self, start_pos: Tuple[float, float], 
                     goal_pos: Tuple[float, float],
                     current_state: Dict) -> Optional[List[Tuple[float, float]]]:
        """
        Recherche A* pour trouver chemin
        
        Heuristique: Distance euclidienne
        """
        # Nœud départ
        start_node = SearchNode(
            state_hash=self.hash_position(start_pos),
            position=start_pos,
            g_cost=0.0,
            h_cost=self.heuristic(start_pos, goal_pos)
        )
        
        # Files priorité et ensembles
        open_set = []
        heapq.heappush(open_set, start_node)
        closed_set: Set[str] = set()
        nodes_explored = 0
        best_node = start_node  # Meilleur nœud trouvé (plus proche objectif)
        
        while open_set and nodes_explored < self.max_nodes_explored:
            # Nœud avec f_cost minimal
            current = heapq.heappop(open_set)
            nodes_explored += 1
            
            # Mettre à jour meilleur nœud
            if current.h_cost < best_node.h_cost:
                best_node = current
            
            # Objectif atteint ?
            if self.is_goal_reached(current.position, goal_pos):
                return self.reconstruct_path(current)
            
            # Marquer comme exploré
            closed_set.add(current.state_hash)
            
            # Profondeur max atteinte ?
            if current.g_cost >= self.max_search_depth:
                continue
            
            # Explorer voisins
            for action in self.available_actions:
                neighbor_pos = self.predict_position_after_action(
                    current.position, action, current_state
                )
                
                if neighbor_pos is None:
                    continue
                
                neighbor_hash = self.hash_position(neighbor_pos)
                
                if neighbor_hash in closed_set:
                    continue
                
                # Calculer coûts
                g_cost = current.g_cost + self.action_cost
                h_cost = self.heuristic(neighbor_pos, goal_pos)
                
                neighbor = SearchNode(
                    state_hash=neighbor_hash,
                    position=neighbor_pos,
                    g_cost=g_cost,
                    h_cost=h_cost,
                    parent=current,
                    action=action
                )
                
                heapq.heappush(open_set, neighbor)
        
        # Si aucun chemin complet, retourner chemin partiel vers meilleur nœud
        if best_node != start_node:
            return self.reconstruct_path(best_node)
        
        # Aucun chemin trouvé
        return None
    
    def heuristic(self, pos1: Tuple[float, float], pos2: Tuple[float, float]) -> float:
        """Heuristique distance euclidienne"""
        return np.sqrt((pos1[0] - pos2[0])**2 + (pos1[1] - pos2[1])**2)
    
    def is_goal_reached(self, pos: Tuple[float, float], goal_pos: Tuple[float, float], 
                       threshold: float = 2.0) -> bool:
        """Vérifier si objectif atteint"""
        distance = self.heuristic(pos, goal_pos)
        return distance < threshold
    
    def reconstruct_path(self, node: SearchNode) -> List[Tuple[float, float]]:
        """Reconstruire chemin depuis nœud final"""
        path = []
        current = node
        
        while current is not None:
            path.append(current.position)
            current = current.parent
        
        path.reverse()
        return path
    
    def path_to_actions(self, path: List[Tuple[float, float]]) -> List[int]:
        """
        Convertir chemin en actions
        
        Simplifié: direction entre positions successives
        """
        actions = []
        
        for i in range(len(path) - 1):
            current_pos = path[i]
            next_pos = path[i + 1]
            
            # Calculer direction
            dx = next_pos[0] - current_pos[0]
            dy = next_pos[1] - current_pos[1]
            
            # Convertir en action
            if abs(dx) > abs(dy):
                action = 1 if dx > 0 else 3  # RIGHT ou LEFT
            else:
                action = 4 if dy > 0 else 2  # DOWN ou UP
            
            actions.append(action)
        
        return actions
    
    def predict_position_after_action(self, pos: Tuple[float, float], 
                                     action: int, 
                                     current_state: Dict) -> Optional[Tuple[float, float]]:
        """
        Prédire position après action
        
        Utilise RulesLearning si disponible, sinon modèle simple
        """
        # Modèle simple: mouvement unitaire
        dx, dy = 0.0, 0.0
        
        if action == 1:  # RIGHT
            dx = 1.0
        elif action == 2:  # UP
            dy = -1.0
        elif action == 3:  # LEFT
            dx = -1.0
        elif action == 4:  # DOWN
            dy = 1.0
        elif action == 0 or action == 5:  # NOOP ou USE
            dx, dy = 0.0, 0.0
        
        new_pos = (pos[0] + dx, pos[1] + dy)
        
        # Vérifier limites grille (0-100 pour ls20)
        new_pos = (
            max(0.0, min(100.0, new_pos[0])),
            max(0.0, min(100.0, new_pos[1]))
        )
        
        return new_pos
    
    def get_avatar_position(self, state: Dict) -> Optional[Tuple[float, float]]:
        """Extraire position avatar depuis état"""
        entities = state.get('entities', {})
        
        # Méthode 1: Chercher par ID explicite "avatar"
        for entity_id, entity in entities.items():
            if 'avatar' in entity_id.lower():
                return entity.centroid
        
        # Méthode 2: Heuristique taille ~100 pixels
        for entity_id, entity in entities.items():
            if 80 <= entity.size <= 120:
                return entity.centroid
        
        # Méthode 3: Première entité si aucune autre trouvée
        if entities:
            first_entity = next(iter(entities.values()))
            return first_entity.centroid
        
        return None
    
    def hash_position(self, pos: Tuple[float, float]) -> str:
        """Hash position pour recherche"""
        return f"{int(pos[0])}_{int(pos[1])}"
    
    def execute_plan(self, plan: Plan) -> int:
        """
        Exécuter plan (retourne prochaine action)
        
        Gestion séquentielle des actions
        """
        if not plan.actions:
            return 0  # NOOP si plan vide
        
        # Retourner première action
        next_action = plan.actions[0]
        
        # Retirer action du plan
        plan.actions = plan.actions[1:]
        
        return next_action
    
    def replan(self, current_state: Dict, reason: str = "unknown"):
        """
        Replanning si plan échoue
        
        Génère nouveau plan pour objectif actif
        """
        self.replanning_count += 1
        
        if self.active_goal is None:
            return
        
        # Générer nouveau plan
        new_plan = self.plan_to_goal(self.active_goal, current_state)
        
        if new_plan:
            self.current_plan = new_plan
            self.plans_generated += 1
        else:
            # Échec replanning
            self.active_goal.status = "failed"
            self.plans_failed += 1
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques planification"""
        return {
            'goals_total': len(self.goals),
            'goals_active': sum(1 for g in self.goals if g.status == "active"),
            'goals_completed': sum(1 for g in self.goals if g.status == "completed"),
            'goals_failed': sum(1 for g in self.goals if g.status == "failed"),
            'plans_generated': self.plans_generated,
            'plans_succeeded': self.plans_succeeded,
            'plans_failed': self.plans_failed,
            'replanning_count': self.replanning_count,
            'current_plan': str(self.current_plan) if self.current_plan else None
        }
    
    def __repr__(self):
        stats = self.get_statistics()
        return (f"HierarchicalPlanner(goals={stats['goals_total']}, "
                f"plans={stats['plans_generated']}, "
                f"replanning={stats['replanning_count']})")

# Made with Bob
