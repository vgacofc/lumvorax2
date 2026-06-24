"""
IntelligentExploration - Couche 6 Architecture Systémique MAGEN

Exploration intelligente dirigée par la curiosité pour découvrir activement
l'environnement et maximiser l'apprentissage.

Principes:
- Curiosité basée sur l'incertitude (zones inexplorées)
- Équilibrage exploration/exploitation (epsilon-greedy adaptatif)
- Priorisation zones à fort potentiel d'apprentissage
- Évitement comportements répétitifs (détection cycles)

Intégration:
- Utilise WorldModel pour identifier zones inexplorées
- Utilise WorldDynamics pour prédire nouveauté
- Utilise HierarchicalPlanner pour planifier exploration
- Fournit actions d'exploration au système de décision

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.5
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass
from collections import defaultdict


@dataclass
class ExplorationZone:
    """Zone d'exploration potentielle"""
    position: Tuple[int, int]
    uncertainty: float  # Incertitude (0-1)
    visit_count: int
    last_visit_step: int
    predicted_novelty: float  # Nouveauté prédite (0-1)
    priority: float  # Priorité globale (0-1)


class IntelligentExploration:
    """
    Exploration intelligente dirigée par curiosité
    
    Stratégies:
    1. Curiosité intrinsèque (zones jamais visitées)
    2. Curiosité épistémique (zones à forte incertitude)
    3. Curiosité prédictive (zones avec dynamiques inconnues)
    4. Évitement cycles (détection comportements répétitifs)
    """
    
    def __init__(self, world_model, world_dynamics, planner):
        """
        Args:
            world_model: WorldModel pour cartographie
            world_dynamics: WorldDynamics pour prédictions
            planner: HierarchicalPlanner pour planification
        """
        self.world_model = world_model
        self.world_dynamics = world_dynamics
        self.planner = planner
        
        # Paramètres exploration
        self.epsilon_initial = 0.9  # Exploration initiale élevée
        self.epsilon_min = 0.1  # Exploration minimale
        self.epsilon_decay = 0.995  # Décroissance epsilon
        self.epsilon = self.epsilon_initial
        
        # Seuils curiosité
        self.uncertainty_threshold = 0.3  # Seuil incertitude significative
        self.novelty_threshold = 0.5  # Seuil nouveauté intéressante
        self.visit_penalty_factor = 0.1  # Pénalité revisites
        
        # Historique exploration
        self.position_visits: Dict[Tuple[int, int], int] = defaultdict(int)
        self.position_last_visit: Dict[Tuple[int, int], int] = {}
        self.exploration_zones: List[ExplorationZone] = []
        
        # Statistiques
        self.total_explorations = 0
        self.successful_discoveries = 0
        self.cycles_detected = 0
        self.current_step = 0
        
        # Historique récent pour détection cycles
        self.recent_positions: List[Tuple[int, int]] = []
        self.cycle_detection_window = 10
        
    def should_explore(self) -> bool:
        """
        Décide si exploration ou exploitation
        
        Utilise epsilon-greedy adaptatif
        
        CORRECTION PHASE 4.5.9: Détection cycles conditionnelle
        - Cycles détectés uniquement si epsilon > 0.2
        - Évite exploration forcée quand epsilon faible
        
        Returns:
            True si exploration, False si exploitation
        """
        # Epsilon-greedy STRICT
        if np.random.random() < self.epsilon:
            return True
        
        # Détection cycles UNIQUEMENT si exploration élevée (epsilon > 0.2)
        # CORRECTION: Évite de forcer exploration quand epsilon faible
        if self.epsilon > 0.2 and self._is_in_cycle():
            return True
        
        return False
    
    def select_exploration_action(self, current_state: Dict) -> Optional[int]:
        """
        Sélectionne action d'exploration optimale
        
        Args:
            current_state: État actuel (avec position avatar)
            
        Returns:
            Action d'exploration (0-4) ou None si pas d'exploration
        """
        self.current_step += 1
        
        # Mettre à jour zones d'exploration
        self._update_exploration_zones(current_state)
        
        # Sélectionner zone cible
        target_zone = self._select_target_zone(current_state)
        
        if target_zone is None:
            # Exploration aléatoire si pas de zone identifiée
            return self._random_exploration_action()
        
        # Planifier chemin vers zone cible
        current_pos = current_state.get('avatar_position')
        if current_pos is None:
            return self._random_exploration_action()
        
        # Utiliser planner pour atteindre zone
        try:
            path = self.planner.astar_search(
                start_pos=current_pos,
                goal_pos=target_zone.position,
                current_state=current_state
            )
            
            if path and len(path) > 1:
                # Convertir prochaine position en action
                next_pos = path[1]
                action = self._position_to_action(current_pos, next_pos)
                
                # Enregistrer exploration
                self.total_explorations += 1
                self._record_visit(current_pos)
                
                return action
        except Exception:
            pass
        
        # Fallback: exploration aléatoire
        return self._random_exploration_action()
    
    def _update_exploration_zones(self, current_state: Dict):
        """Met à jour zones d'exploration potentielles"""
        self.exploration_zones = []
        
        # Obtenir grille depuis WorldModel
        grid = current_state.get('grid')
        if grid is None:
            return
        
        height, width = grid.shape
        current_pos = current_state.get('avatar_position', (0, 0))
        
        # Identifier zones accessibles non visitées ou peu visitées
        for y in range(height):
            for x in range(width):
                pos = (y, x)
                
                # Ignorer position actuelle
                if pos == current_pos:
                    continue
                
                # Vérifier si accessible (pas mur)
                if grid[y, x] == 0:  # Mur
                    continue
                
                # Calculer métriques curiosité
                uncertainty = self._calculate_uncertainty(pos, current_state)
                visit_count = self.position_visits.get(pos, 0)
                last_visit = self.position_last_visit.get(pos, -1000)
                novelty = self._predict_novelty(pos, current_state)
                
                # Calculer priorité
                priority = self._calculate_priority(
                    uncertainty, visit_count, last_visit, novelty
                )
                
                # Créer zone si priorité suffisante
                if priority > 0.1:  # Seuil minimal
                    zone = ExplorationZone(
                        position=pos,
                        uncertainty=uncertainty,
                        visit_count=visit_count,
                        last_visit_step=last_visit,
                        predicted_novelty=novelty,
                        priority=priority
                    )
                    self.exploration_zones.append(zone)
        
        # Trier par priorité décroissante
        self.exploration_zones.sort(key=lambda z: z.priority, reverse=True)
    
    def _calculate_uncertainty(self, pos: Tuple[int, int], state: Dict) -> float:
        """
        Calcule incertitude d'une position
        
        Incertitude élevée = zone jamais/peu visitée
        """
        visit_count = self.position_visits.get(pos, 0)
        
        if visit_count == 0:
            return 1.0  # Incertitude maximale
        
        # Décroissance logarithmique
        uncertainty = 1.0 / (1.0 + np.log1p(visit_count))
        
        return float(uncertainty)
    
    def _predict_novelty(self, pos: Tuple[int, int], state: Dict) -> float:
        """
        Prédit nouveauté d'une position
        
        Utilise WorldDynamics pour estimer si position
        contient dynamiques intéressantes
        """
        # Vérifier si position contient entité
        grid = state.get('grid')
        if grid is not None:
            cell_value = grid[pos[0], pos[1]]
            
            # Entités spéciales (porte, objets) = haute nouveauté
            if cell_value in [5, 6, 7, 8, 9]:  # Couleurs spéciales
                return 0.9
            
            # Sol vide = faible nouveauté
            if cell_value == 4:
                return 0.2
        
        # Nouveauté par défaut
        return 0.5
    
    def _calculate_priority(
        self,
        uncertainty: float,
        visit_count: int,
        last_visit: int,
        novelty: float
    ) -> float:
        """
        Calcule priorité globale d'exploration
        
        Combine:
        - Incertitude (curiosité intrinsèque)
        - Nouveauté prédite (curiosité épistémique)
        - Pénalité revisites récentes
        - Bonus zones jamais visitées
        """
        # Composante incertitude (poids 0.4)
        uncertainty_score = uncertainty * 0.4
        
        # Composante nouveauté (poids 0.3)
        novelty_score = novelty * 0.3
        
        # Bonus jamais visité (poids 0.2)
        never_visited_bonus = 0.2 if visit_count == 0 else 0.0
        
        # Pénalité revisites récentes (poids 0.1)
        recency_penalty = 0.0
        if last_visit >= 0:
            steps_since_visit = self.current_step - last_visit
            if steps_since_visit < 20:  # Visite très récente
                recency_penalty = 0.1 * (1.0 - steps_since_visit / 20.0)
        
        # Priorité finale
        priority = (
            uncertainty_score +
            novelty_score +
            never_visited_bonus -
            recency_penalty
        )
        
        return max(0.0, min(1.0, priority))
    
    def _select_target_zone(self, state: Dict) -> Optional[ExplorationZone]:
        """Sélectionne zone cible pour exploration"""
        if not self.exploration_zones:
            return None
        
        # Sélectionner zone avec priorité maximale
        # (déjà triées par priorité décroissante)
        return self.exploration_zones[0]
    
    def _random_exploration_action(self) -> int:
        """Action d'exploration aléatoire"""
        # Actions: 0=NOOP, 1=UP, 2=DOWN, 3=LEFT, 4=RIGHT
        return np.random.randint(1, 5)
    
    def _position_to_action(
        self,
        current: Tuple[int, int],
        next_pos: Tuple[int, int]
    ) -> int:
        """Convertit déplacement en action"""
        dy = next_pos[0] - current[0]
        dx = next_pos[1] - current[1]
        
        if dy < 0:
            return 1  # UP
        elif dy > 0:
            return 2  # DOWN
        elif dx < 0:
            return 3  # LEFT
        elif dx > 0:
            return 4  # RIGHT
        else:
            return 0  # NOOP
    
    def _record_visit(self, pos: Tuple[int, int]):
        """Enregistre visite d'une position"""
        self.position_visits[pos] += 1
        self.position_last_visit[pos] = self.current_step
        self.recent_positions.append(pos)
        
        # Limiter historique récent
        if len(self.recent_positions) > self.cycle_detection_window:
            self.recent_positions.pop(0)
    
    def _is_in_cycle(self) -> bool:
        """
        Détecte si agent est dans un cycle
        
        Cycle = retour répété aux mêmes positions
        """
        if len(self.recent_positions) < self.cycle_detection_window:
            return False
        
        # Vérifier si positions récentes se répètent
        unique_positions = len(set(self.recent_positions))
        
        # Cycle détecté si moins de 50% positions uniques
        if unique_positions < self.cycle_detection_window * 0.5:
            self.cycles_detected += 1
            return True
        
        return False
    
    def update_epsilon(self):
        """Met à jour epsilon (décroissance progressive)"""
        self.epsilon = max(
            self.epsilon_min,
            self.epsilon * self.epsilon_decay
        )
    
    def record_discovery(self, discovery_type: str):
        """Enregistre découverte réussie"""
        self.successful_discoveries += 1
    
    def get_statistics(self) -> Dict:
        """Statistiques d'exploration"""
        return {
            'total_explorations': self.total_explorations,
            'successful_discoveries': self.successful_discoveries,
            'cycles_detected': self.cycles_detected,
            'epsilon': self.epsilon,
            'unique_positions_visited': len(self.position_visits),
            'exploration_zones_identified': len(self.exploration_zones),
            'average_visits_per_position': (
                np.mean(list(self.position_visits.values()))
                if self.position_visits else 0.0
            )
        }
    
    def reset(self):
        """Reset pour nouvel épisode"""
        self.epsilon = self.epsilon_initial
        self.position_visits.clear()
        self.position_last_visit.clear()
        self.exploration_zones.clear()
        self.recent_positions.clear()
        self.current_step = 0


if __name__ == "__main__":
    print("IntelligentExploration - Couche 6 Architecture Systémique")
    print("=" * 60)
    print()
    print("Fonctionnalités:")
    print("- Curiosité intrinsèque (zones inexplorées)")
    print("- Curiosité épistémique (incertitude)")
    print("- Curiosité prédictive (nouveauté)")
    print("- Détection cycles")
    print("- Epsilon-greedy adaptatif")
    print()
    print("Intégration: WorldModel + WorldDynamics + HierarchicalPlanner")

# Made with Bob
