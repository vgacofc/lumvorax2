"""
MAGEN V32 - Module C7: GoalHypothesisEngine
Spatial Cognitive System - Moteur d'Hypothèses de But

Mission: Générer et évaluer des hypothèses sur les objectifs du jeu
Problème V31: Aucune hypothèse goal formée
Solution V32: Génération multi-critères avec évaluation bayésienne
"""

import time
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass
from collections import defaultdict
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class GoalHypothesis:
    """Hypothèse sur l'objectif du jeu."""
    hypothesis_id: str
    goal_type: str  # "reach_position", "collect_items", "avoid_hazards", "transform_state"
    target: Optional[Tuple[int, int]]  # Position cible si applicable
    description: str
    confidence: float  # 0-1
    evidence: List[str]
    counter_evidence: List[str]
    first_proposed_step: int
    last_updated_step: int
    validation_attempts: int
    success_indicators: float  # 0-1


@dataclass
class GoalEvidence:
    """Preuve pour/contre une hypothèse."""
    evidence_type: str
    strength: float  # 0-1
    description: str
    step: int


class GoalHypothesisEngine:
    """
    Moteur d'Hypothèses de But.
    
    Génère et évalue des hypothèses sur l'objectif du jeu
    en analysant les observations, récompenses, et patterns.
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Configuration
        self.min_confidence_threshold = 0.6  # Seuil pour hypothèse viable
        self.max_active_hypotheses = 5  # Maximum d'hypothèses actives
        self.evidence_decay = 0.95  # Décroissance des preuves anciennes
        
        # Hypothèses actives
        self.active_hypotheses: Dict[str, GoalHypothesis] = {}
        self.hypothesis_counter: int = 0
        
        # Historique
        self.all_hypotheses: List[GoalHypothesis] = []
        self.validated_goals: List[GoalHypothesis] = []
        self.rejected_goals: List[GoalHypothesis] = []
        
        # Observations pour génération
        self.reward_history: List[Tuple[int, float]] = []  # (step, reward)
        self.special_positions: Set[Tuple[int, int]] = set()  # Positions remarquables
        self.item_positions: Dict[str, List[Tuple[int, int]]] = defaultdict(list)
        
        # Statistiques
        self.total_hypotheses_generated: int = 0
        self.hypotheses_validated: int = 0
        self.hypotheses_rejected: int = 0
        
        # État actuel
        self.current_best_hypothesis: Optional[GoalHypothesis] = None
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/goal_hypothesis_engine.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "GoalHypothesisEngine")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "GoalHypothesisEngine",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "goal_hypothesis_engine.initialized",
                "goal_hypothesis_engine",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_generated': self.total_hypotheses_generated,
            'active_count': len(self.active_hypotheses),
            'validated_count': self.hypotheses_validated,
            'rejected_count': self.hypotheses_rejected,
            'best_hypothesis_id': self.current_best_hypothesis.hypothesis_id if self.current_best_hypothesis else None,
            'best_confidence': self.current_best_hypothesis.confidence if self.current_best_hypothesis else 0.0
        }
    
    def update(self,
               observation: Dict,
               reward: float,
               mental_map_state: Dict,
               strategic_state: Dict,
               step: int) -> List[GoalHypothesis]:
        """
        Mettre à jour les hypothèses de but.
        
        Args:
            observation: Observation actuelle
            reward: Récompense reçue
            mental_map_state: État de la carte mentale
            strategic_state: État stratégique
            step: Step actuel
            
        Returns:
            Liste des hypothèses actives
        """
        start_ns = time.time_ns()
        
        # Log début update
        self.forensic.forensic_logger.log(
            "update_start",
            "GoalHypothesisEngine",
            {
                "step": step,
                "reward": reward,
                "state": self._forensic_state
            }
        )
        
        # 1. Enregistrer observations
        self._record_observations(observation, reward, step)
        
        # 2. Générer nouvelles hypothèses si nécessaire
        if len(self.active_hypotheses) < self.max_active_hypotheses:
            self._generate_hypotheses(observation, mental_map_state, strategic_state, step)
        
        # 3. Évaluer hypothèses existantes
        self._evaluate_hypotheses(observation, reward, mental_map_state, step)
        
        # 4. Nettoyer hypothèses faibles
        self._prune_weak_hypotheses(step)
        
        # 5. Sélectionner meilleure hypothèse
        self._select_best_hypothesis()
        
        # 6. Forensic logging standard
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "update_complete",
            "GoalHypothesisEngine",
            {
                "step": step,
                "execution_time_ns": end_ns - start_ns,
                "success": True,
                "state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "goal_hypothesis_engine.updated",
                "goal_hypothesis_engine",
                {
                    "step": step,
                    "num_active": len(self.active_hypotheses),
                    "best_hypothesis": self.current_best_hypothesis.hypothesis_id if self.current_best_hypothesis else None,
                    "best_confidence": self.current_best_hypothesis.confidence if self.current_best_hypothesis else 0.0
                }
            )
        
        return list(self.active_hypotheses.values())
    
    def _record_observations(self, observation: Dict, reward: float, step: int) -> None:
        """Enregistrer observations pour génération future."""
        # Enregistrer récompense
        self.reward_history.append((step, reward))
        
        # Limiter historique
        if len(self.reward_history) > 100:
            self.reward_history = self.reward_history[-100:]
        
        # Identifier positions spéciales dans la grille
        if 'grid' in observation:
            grid = observation['grid']
            for y, row in enumerate(grid):
                for x, cell in enumerate(row):
                    # Positions remarquables: items, objectifs, etc.
                    if cell not in [' ', '.', '#', 'W']:  # Pas vide, pas mur
                        self.special_positions.add((x, y))
                        
                        # Catégoriser par type
                        if cell in ['G', 'T', '★', '◆']:  # Symboles de goal
                            self.item_positions['goal'].append((x, y))
                        elif cell in ['C', 'I', '○', '◇']:  # Symboles d'items
                            self.item_positions['collectible'].append((x, y))
                        elif cell in ['H', 'X', '☠', '⚠']:  # Symboles de danger
                            self.item_positions['hazard'].append((x, y))
    
    def _generate_hypotheses(self,
                            observation: Dict,
                            mental_map_state: Dict,
                            strategic_state: Dict,
                            step: int) -> None:
        """Générer nouvelles hypothèses de but."""
        new_hypotheses = []
        
        # Type 1: Reach Position (atteindre une position)
        if self.item_positions['goal']:
            for pos in self.item_positions['goal'][-3:]:  # 3 dernières positions goal
                hyp = self._create_reach_position_hypothesis(pos, step)
                new_hypotheses.append(hyp)
        
        # Type 2: Collect Items (collecter des items)
        if self.item_positions['collectible']:
            hyp = self._create_collect_items_hypothesis(step)
            new_hypotheses.append(hyp)
        
        # Type 3: Avoid Hazards (éviter dangers)
        if self.item_positions['hazard']:
            hyp = self._create_avoid_hazards_hypothesis(step)
            new_hypotheses.append(hyp)
        
        # Type 4: Maximize Reward (maximiser récompense)
        if len(self.reward_history) >= 10:
            hyp = self._create_maximize_reward_hypothesis(step)
            new_hypotheses.append(hyp)
        
        # Type 5: Explore Territory (explorer territoire)
        if mental_map_state.get('num_regions', 0) > 0:
            hyp = self._create_explore_territory_hypothesis(mental_map_state, step)
            new_hypotheses.append(hyp)
        
        # Ajouter hypothèses si place disponible
        for hyp in new_hypotheses:
            if len(self.active_hypotheses) < self.max_active_hypotheses:
                self.active_hypotheses[hyp.hypothesis_id] = hyp
                self.all_hypotheses.append(hyp)
                self.total_hypotheses_generated += 1
                
                if self.logger:
                    self.logger.log_event(
                        "goal_hypothesis_engine.hypothesis_generated",
                        "goal_hypothesis_engine",
                        {
                            "step": step,
                            "hypothesis_id": hyp.hypothesis_id,
                            "goal_type": hyp.goal_type,
                            "description": hyp.description
                        }
                    )
    
    def _create_reach_position_hypothesis(self, target: Tuple[int, int], step: int) -> GoalHypothesis:
        """Créer hypothèse 'atteindre position'."""
        self.hypothesis_counter += 1
        return GoalHypothesis(
            hypothesis_id=f"reach_pos_{self.hypothesis_counter}",
            goal_type="reach_position",
            target=target,
            description=f"Atteindre position {target}",
            confidence=0.5,
            evidence=["Position marquée comme goal dans grille"],
            counter_evidence=[],
            first_proposed_step=step,
            last_updated_step=step,
            validation_attempts=0,
            success_indicators=0.0
        )
    
    def _create_collect_items_hypothesis(self, step: int) -> GoalHypothesis:
        """Créer hypothèse 'collecter items'."""
        self.hypothesis_counter += 1
        num_items = len(self.item_positions['collectible'])
        return GoalHypothesis(
            hypothesis_id=f"collect_{self.hypothesis_counter}",
            goal_type="collect_items",
            target=None,
            description=f"Collecter {num_items} items",
            confidence=0.4,
            evidence=[f"{num_items} items collectibles détectés"],
            counter_evidence=[],
            first_proposed_step=step,
            last_updated_step=step,
            validation_attempts=0,
            success_indicators=0.0
        )
    
    def _create_avoid_hazards_hypothesis(self, step: int) -> GoalHypothesis:
        """Créer hypothèse 'éviter dangers'."""
        self.hypothesis_counter += 1
        num_hazards = len(self.item_positions['hazard'])
        return GoalHypothesis(
            hypothesis_id=f"avoid_{self.hypothesis_counter}",
            goal_type="avoid_hazards",
            target=None,
            description=f"Éviter {num_hazards} dangers",
            confidence=0.3,
            evidence=[f"{num_hazards} dangers détectés"],
            counter_evidence=[],
            first_proposed_step=step,
            last_updated_step=step,
            validation_attempts=0,
            success_indicators=0.0
        )
    
    def _create_maximize_reward_hypothesis(self, step: int) -> GoalHypothesis:
        """Créer hypothèse 'maximiser récompense'."""
        self.hypothesis_counter += 1
        recent_rewards = [r for _, r in self.reward_history[-10:]]
        avg_reward = sum(recent_rewards) / len(recent_rewards) if recent_rewards else 0.0
        
        return GoalHypothesis(
            hypothesis_id=f"max_reward_{self.hypothesis_counter}",
            goal_type="maximize_reward",
            target=None,
            description=f"Maximiser récompense (avg: {avg_reward:.2f})",
            confidence=0.6,
            evidence=["Récompenses variables observées"],
            counter_evidence=[],
            first_proposed_step=step,
            last_updated_step=step,
            validation_attempts=0,
            success_indicators=0.0
        )
    
    def _create_explore_territory_hypothesis(self, mental_map_state: Dict, step: int) -> GoalHypothesis:
        """Créer hypothèse 'explorer territoire'."""
        self.hypothesis_counter += 1
        num_regions = mental_map_state.get('num_regions', 0)
        
        return GoalHypothesis(
            hypothesis_id=f"explore_{self.hypothesis_counter}",
            goal_type="explore_territory",
            target=None,
            description=f"Explorer territoire ({num_regions} régions)",
            confidence=0.4,
            evidence=[f"{num_regions} régions identifiées"],
            counter_evidence=[],
            first_proposed_step=step,
            last_updated_step=step,
            validation_attempts=0,
            success_indicators=0.0
        )
    
    def _evaluate_hypotheses(self,
                            observation: Dict,
                            reward: float,
                            mental_map_state: Dict,
                            step: int) -> None:
        """Évaluer toutes les hypothèses actives."""
        for hyp in self.active_hypotheses.values():
            # Collecter preuves
            evidence_for, evidence_against = self._collect_evidence(
                hyp, observation, reward, mental_map_state, step
            )
            
            # Mettre à jour hypothèse
            hyp.evidence.extend(evidence_for)
            hyp.counter_evidence.extend(evidence_against)
            hyp.last_updated_step = step
            hyp.validation_attempts += 1
            
            # Recalculer confiance (Bayésien simplifié)
            prior = hyp.confidence
            likelihood_for = len(evidence_for) * 0.15
            likelihood_against = len(evidence_against) * 0.15
            
            # Mise à jour bayésienne
            posterior = prior + likelihood_for - likelihood_against
            hyp.confidence = max(0.0, min(1.0, posterior))
            
            # Calculer indicateurs de succès
            hyp.success_indicators = self._compute_success_indicators(hyp, reward)
            
            # Décroissance temporelle
            age = step - hyp.first_proposed_step
            if age > 50:
                hyp.confidence *= self.evidence_decay
    
    def _collect_evidence(self,
                         hypothesis: GoalHypothesis,
                         observation: Dict,
                         reward: float,
                         mental_map_state: Dict,
                         step: int) -> Tuple[List[str], List[str]]:
        """Collecter preuves pour/contre une hypothèse."""
        evidence_for = []
        evidence_against = []
        
        if hypothesis.goal_type == "reach_position":
            # Vérifier si on se rapproche de la cible
            if hypothesis.target and 'avatar_position' in observation:
                avatar_pos = tuple(observation['avatar_position'])
                distance = abs(avatar_pos[0] - hypothesis.target[0]) + abs(avatar_pos[1] - hypothesis.target[1])
                
                if distance < 3:
                    evidence_for.append(f"Proche de cible (distance={distance})")
                
                if reward > 0.5:
                    evidence_for.append(f"Récompense positive en approchant")
                elif reward < -0.1:
                    evidence_against.append(f"Récompense négative en approchant")
        
        elif hypothesis.goal_type == "collect_items":
            # Vérifier si items diminuent
            current_items = len(self.item_positions['collectible'])
            if current_items < len(hypothesis.evidence):
                evidence_for.append("Nombre d'items a diminué")
            
            if reward > 0.3:
                evidence_for.append("Récompense positive (possible collection)")
        
        elif hypothesis.goal_type == "avoid_hazards":
            # Vérifier si on évite les dangers
            if reward < -0.5:
                evidence_against.append("Récompense très négative (danger touché?)")
            elif reward >= 0:
                evidence_for.append("Pas de pénalité (dangers évités)")
        
        elif hypothesis.goal_type == "maximize_reward":
            # Vérifier tendance des récompenses
            if len(self.reward_history) >= 5:
                recent = [r for _, r in self.reward_history[-5:]]
                if sum(recent) > 0:
                    evidence_for.append("Récompenses positives récentes")
                else:
                    evidence_against.append("Récompenses négatives récentes")
        
        elif hypothesis.goal_type == "explore_territory":
            # Vérifier si exploration progresse
            num_regions = mental_map_state.get('num_regions', 0)
            if num_regions > 3:
                evidence_for.append(f"Exploration active ({num_regions} régions)")
            
            if reward > 0.2:
                evidence_for.append("Récompense pour exploration")
        
        return evidence_for, evidence_against
    
    def _compute_success_indicators(self, hypothesis: GoalHypothesis, current_reward: float) -> float:
        """Calculer indicateurs de succès d'une hypothèse."""
        # Facteurs:
        # 1. Confiance actuelle
        # 2. Ratio preuves pour/contre
        # 3. Récompense actuelle
        # 4. Durée de vie
        
        confidence_factor = hypothesis.confidence
        
        evidence_ratio = 0.5
        if len(hypothesis.evidence) + len(hypothesis.counter_evidence) > 0:
            evidence_ratio = len(hypothesis.evidence) / (len(hypothesis.evidence) + len(hypothesis.counter_evidence))
        
        reward_factor = max(0.0, min(1.0, (current_reward + 1.0) / 2.0))
        
        age = hypothesis.last_updated_step - hypothesis.first_proposed_step
        longevity_factor = min(1.0, age / 30.0)
        
        success = (
            0.4 * confidence_factor +
            0.3 * evidence_ratio +
            0.2 * reward_factor +
            0.1 * longevity_factor
        )
        
        return success
    
    def _prune_weak_hypotheses(self, step: int) -> None:
        """Nettoyer hypothèses faibles."""
        to_remove = []
        
        for hyp_id, hyp in self.active_hypotheses.items():
            # Critères de rejet:
            # 1. Confiance trop faible après plusieurs validations
            if hyp.validation_attempts > 10 and hyp.confidence < 0.2:
                to_remove.append(hyp_id)
                self.rejected_goals.append(hyp)
                self.hypotheses_rejected += 1
            
            # 2. Trop de contre-preuves
            elif len(hyp.counter_evidence) > len(hyp.evidence) * 2:
                to_remove.append(hyp_id)
                self.rejected_goals.append(hyp)
                self.hypotheses_rejected += 1
            
            # 3. Trop vieille sans amélioration
            elif (step - hyp.first_proposed_step) > 100 and hyp.confidence < 0.3:
                to_remove.append(hyp_id)
                self.rejected_goals.append(hyp)
                self.hypotheses_rejected += 1
        
        for hyp_id in to_remove:
            del self.active_hypotheses[hyp_id]
            
            if self.logger:
                self.logger.log_event(
                    "goal_hypothesis_engine.hypothesis_rejected",
                    "goal_hypothesis_engine",
                    {
                        "step": step,
                        "hypothesis_id": hyp_id
                    }
                )
    
    def _select_best_hypothesis(self) -> None:
        """Sélectionner la meilleure hypothèse."""
        if not self.active_hypotheses:
            self.current_best_hypothesis = None
            return
        
        best = max(
            self.active_hypotheses.values(),
            key=lambda h: h.confidence * 0.6 + h.success_indicators * 0.4
        )
        
        self.current_best_hypothesis = best
        
        # Valider si confiance très élevée
        if best.confidence > 0.85 and best not in self.validated_goals:
            self.validated_goals.append(best)
            self.hypotheses_validated += 1
            
            if self.logger:
                self.logger.log_event(
                    "goal_hypothesis_engine.hypothesis_validated",
                    "goal_hypothesis_engine",
                    {
                        "hypothesis_id": best.hypothesis_id,
                        "goal_type": best.goal_type,
                        "confidence": best.confidence
                    }
                )
    
    def get_best_hypothesis(self) -> Optional[GoalHypothesis]:
        """Obtenir la meilleure hypothèse actuelle."""
        return self.current_best_hypothesis
    
    def get_active_hypotheses(self) -> List[GoalHypothesis]:
        """Obtenir toutes les hypothèses actives."""
        return list(self.active_hypotheses.values())
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques."""
        return {
            "total_generated": self.total_hypotheses_generated,
            "active": len(self.active_hypotheses),
            "validated": self.hypotheses_validated,
            "rejected": self.hypotheses_rejected,
            "best_hypothesis": {
                "id": self.current_best_hypothesis.hypothesis_id,
                "type": self.current_best_hypothesis.goal_type,
                "confidence": self.current_best_hypothesis.confidence
            } if self.current_best_hypothesis else None
        }
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter état en JSON."""
        data = {
            "statistics": self.get_statistics(),
            "active_hypotheses": [
                {
                    "id": h.hypothesis_id,
                    "type": h.goal_type,
                    "target": list(h.target) if h.target else None,
                    "description": h.description,
                    "confidence": h.confidence,
                    "evidence_count": len(h.evidence),
                    "counter_evidence_count": len(h.counter_evidence),
                    "success_indicators": h.success_indicators
                }
                for h in self.active_hypotheses.values()
            ],
            "validated_goals": [
                {
                    "id": h.hypothesis_id,
                    "type": h.goal_type,
                    "confidence": h.confidence
                }
                for h in self.validated_goals
            ]
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "GoalHypothesisEngine",
            {
                "final_state": self._forensic_state,
                "total_generated": self.total_hypotheses_generated,
                "validated": self.hypotheses_validated,
                "rejected": self.hypotheses_rejected,
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()


# Made with Bob
