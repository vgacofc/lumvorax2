"""
MAGEN V32 - Module C3: RegionalStrategist
Spatial Cognitive System - Évaluation Stratégique des Régions

Mission: Donner une valeur STRATÉGIQUE aux régions du monde
"""

import time
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass
import json
from pathlib import Path

from .forensic_middleware import ForensicMiddleware


@dataclass
class RegionStrategicAssessment:
    """Évaluation stratégique d'une région."""
    region_id: str
    
    # Valeurs stratégiques (0.0-1.0)
    strategic_value: float = 0.5
    learning_potential: float = 0.5
    novelty: float = 0.5
    reward_probability: float = 0.5
    loop_pressure: float = 0.0
    exploration_priority: float = 0.5
    
    # Statut cognitif
    cognitive_status: str = "unknown"  # sterile, promising, neutral, explored, goal_candidate
    confidence: float = 0.0
    
    # Justification
    reasoning: Optional[List[str]] = None
    
    def __post_init__(self):
        if self.reasoning is None:
            self.reasoning = []
    
    def to_dict(self) -> Dict:
        """Sérialisation pour forensic."""
        return {
            'region_id': self.region_id,
            'strategic_value': self.strategic_value,
            'learning_potential': self.learning_potential,
            'novelty': self.novelty,
            'reward_probability': self.reward_probability,
            'loop_pressure': self.loop_pressure,
            'exploration_priority': self.exploration_priority,
            'cognitive_status': self.cognitive_status,
            'confidence': self.confidence,
            'reasoning': self.reasoning
        }


class RegionalStrategist:
    """
    Stratège Régional.
    
    Évalue l'utilité stratégique des régions pour guider l'exploration.
    """
    
    def __init__(self, logger=None, forensic_log_path: Optional[str] = None):
        self.logger = logger
        
        # Évaluations par région
        self.assessments: Dict[str, RegionStrategicAssessment] = {}
        
        # Historique des évaluations
        self.assessment_history: List[Dict] = []
        
        # Statistiques
        self.total_assessments: int = 0
        self.sterile_regions_identified: int = 0
        self.promising_regions_identified: int = 0
        
        # Forensic standard LumVorax
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/regional_strategist.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(forensic_log_path, "RegionalStrategist")
        
        # Forensic
        self.creation_time = time.time_ns()
        
        # Log initialization avec forensic standard
        self.forensic.forensic_logger.log(
            "module_initialized",
            "RegionalStrategist",
            {
                "timestamp_ns": self.creation_time,
                "version": "v32.0.0",
                "forensic_log_path": forensic_log_path,
                "initial_state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "regional_strategist.initialized",
                "regional_strategist",
                {
                    "timestamp": self.creation_time,
                    "version": "v32.0.0"
                }
            )
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'total_assessments': self.total_assessments,
            'regions_evaluated': len(self.assessments),
            'sterile_regions': self.sterile_regions_identified,
            'promising_regions': self.promising_regions_identified,
            'avg_strategic_value': sum(a.strategic_value for a in self.assessments.values()) / len(self.assessments) if self.assessments else 0.0,
            'history_size': len(self.assessment_history)
        }
    
    def evaluate_region(self,
                       region_id: str,
                       region_data: Dict,
                       step: int) -> RegionStrategicAssessment:
        """
        Évaluer stratégiquement une région.
        
        Args:
            region_id: ID de la région
            region_data: Données de la région (from MentalMapBuilder)
            step: Step actuel
            
        Returns:
            Évaluation stratégique complète
        """
        start_ns = time.time_ns()
        
        # Log début évaluation
        self.forensic.forensic_logger.log(
            "evaluation_start",
            "RegionalStrategist",
            {
                "region_id": region_id,
                "step": step,
                "region_type": region_data.get('type', 'unknown'),
                "state": self._forensic_state
            }
        )
        
        # 1. Calculer les métriques stratégiques
        strategic_value = self._calculate_strategic_value(region_data)
        learning_potential = self._calculate_learning_potential(region_data)
        novelty = self._calculate_novelty(region_data, step)
        reward_probability = self._calculate_reward_probability(region_data)
        loop_pressure = self._calculate_loop_pressure(region_data)
        exploration_priority = self._calculate_exploration_priority(
            strategic_value, learning_potential, novelty, loop_pressure
        )
        
        # 2. Déterminer le statut cognitif
        cognitive_status = self._determine_cognitive_status(
            strategic_value, learning_potential, reward_probability, loop_pressure
        )
        
        # 3. Calculer la confiance
        confidence = self._calculate_confidence(region_data)
        
        # 4. Générer le raisonnement
        reasoning = self._generate_reasoning(
            region_data, strategic_value, learning_potential, 
            novelty, reward_probability, loop_pressure, cognitive_status
        )
        
        # 5. Créer l'évaluation
        assessment = RegionStrategicAssessment(
            region_id=region_id,
            strategic_value=strategic_value,
            learning_potential=learning_potential,
            novelty=novelty,
            reward_probability=reward_probability,
            loop_pressure=loop_pressure,
            exploration_priority=exploration_priority,
            cognitive_status=cognitive_status,
            confidence=confidence,
            reasoning=reasoning
        )
        
        # 6. Stocker l'évaluation
        self.assessments[region_id] = assessment
        self.total_assessments += 1
        
        # 7. Mettre à jour statistiques
        if cognitive_status == "sterile":
            self.sterile_regions_identified += 1
        elif cognitive_status == "promising":
            self.promising_regions_identified += 1
        
        # 8. Historique
        self.assessment_history.append({
            'step': step,
            'region_id': region_id,
            'assessment': assessment.to_dict()
        })
        
        # 9. Forensic logging standard
        end_ns = time.time_ns()
        self.forensic.forensic_logger.log(
            "evaluation_complete",
            "RegionalStrategist",
            {
                "region_id": region_id,
                "step": step,
                "assessment": assessment.to_dict(),
                "execution_time_ns": end_ns - start_ns,
                "success": True,
                "state": self._forensic_state
            }
        )
        
        if self.logger:
            self.logger.log_event(
                "regional_strategist.region_evaluated",
                "regional_strategist",
                {
                    'step': step,
                    'region_id': region_id,
                    'cognitive_status': cognitive_status,
                    'strategic_value': strategic_value,
                    'exploration_priority': exploration_priority
                }
            )
        
        return assessment
    
    def _calculate_strategic_value(self, region_data: Dict) -> float:
        """Calculer la valeur stratégique globale."""
        # Composantes:
        # 1. Reward moyen (40%)
        avg_reward = region_data.get('avg_reward', 0.0)
        reward_component = max(0.0, min(1.0, (avg_reward + 1.0) / 2.0))
        
        # 2. Taux de succès (30%)
        success_rate = region_data.get('success_rate', 0.5)
        
        # 3. Type de région (20%)
        region_type = region_data.get('type', 'unknown')
        type_values = {
            'frontier': 0.9,
            'cluster': 0.7,
            'corridor': 0.6,
            'area': 0.5,
            'loop': 0.2,
            'dead_end': 0.1,
            'single': 0.3,
            'unknown': 0.5
        }
        type_component = type_values.get(region_type, 0.5)
        
        # 4. Termination (10%)
        termination = region_data.get('termination', None)
        termination_values = {
            'frontier': 0.9,
            'goal': 1.0,
            'wall': 0.2,
            'loop': 0.1,
            None: 0.5
        }
        termination_component = termination_values.get(termination, 0.5)
        
        strategic_value = (
            0.4 * reward_component +
            0.3 * success_rate +
            0.2 * type_component +
            0.1 * termination_component
        )
        
        return strategic_value
    
    def _calculate_learning_potential(self, region_data: Dict) -> float:
        """Calculer le potentiel d'apprentissage."""
        # Potentiel basé sur:
        # 1. Nombre de visites (moins = plus de potentiel)
        visit_count = region_data.get('visit_count', 0)
        visit_factor = max(0.0, 1.0 - (visit_count / 20.0))
        
        # 2. Taille de la région (plus = plus de potentiel)
        positions_count = len(region_data.get('positions', []))
        size_factor = min(1.0, positions_count / 10.0)
        
        # 3. Variance des rewards (plus = plus d'information)
        rewards = region_data.get('rewards_collected', [])
        if len(rewards) > 1:
            mean_reward = sum(rewards) / len(rewards)
            variance = sum((r - mean_reward) ** 2 for r in rewards) / len(rewards)
            variance_factor = min(1.0, variance)
        else:
            variance_factor = 0.5
        
        learning_potential = (
            0.4 * visit_factor +
            0.3 * size_factor +
            0.3 * variance_factor
        )
        
        return learning_potential
    
    def _calculate_novelty(self, region_data: Dict, current_step: int) -> float:
        """Calculer la nouveauté de la région."""
        # Nouveauté basée sur:
        # 1. Temps depuis découverte
        discovery_step = region_data.get('discovery_step', current_step)
        steps_since_discovery = current_step - discovery_step
        time_factor = max(0.0, 1.0 - (steps_since_discovery / 100.0))
        
        # 2. Temps depuis dernière visite
        last_visit = region_data.get('last_visit_step', current_step)
        steps_since_visit = current_step - last_visit
        recency_factor = max(0.0, 1.0 - (steps_since_visit / 50.0))
        
        # 3. Nombre de visites (moins = plus nouveau)
        visit_count = region_data.get('visit_count', 0)
        visit_factor = max(0.0, 1.0 - (visit_count / 10.0))
        
        novelty = (
            0.3 * time_factor +
            0.3 * recency_factor +
            0.4 * visit_factor
        )
        
        return novelty
    
    def _calculate_reward_probability(self, region_data: Dict) -> float:
        """Calculer la probabilité de reward."""
        # Basé sur l'historique des rewards
        rewards = region_data.get('rewards_collected', [])
        
        if not rewards:
            return 0.5  # Incertitude
        
        # Proportion de rewards positifs
        positive_rewards = sum(1 for r in rewards if r > 0)
        probability = positive_rewards / len(rewards)
        
        # Ajuster par la moyenne
        avg_reward = sum(rewards) / len(rewards)
        if avg_reward > 0:
            probability = min(1.0, probability * 1.2)
        
        return probability
    
    def _calculate_loop_pressure(self, region_data: Dict) -> float:
        """Calculer la pression de boucle."""
        # Pression basée sur:
        # 1. Ratio visites/positions
        visit_count = region_data.get('visit_count', 0)
        positions_count = len(region_data.get('positions', []))
        
        if positions_count == 0:
            return 0.0
        
        revisit_ratio = visit_count / positions_count
        
        # 2. Type de région
        region_type = region_data.get('type', 'unknown')
        if region_type == 'loop':
            type_pressure = 1.0
        elif region_type == 'dead_end':
            type_pressure = 0.8
        else:
            type_pressure = 0.0
        
        # 3. Termination
        termination = region_data.get('termination', None)
        if termination == 'loop':
            termination_pressure = 1.0
        else:
            termination_pressure = 0.0
        
        loop_pressure = (
            0.5 * min(1.0, revisit_ratio / 3.0) +
            0.3 * type_pressure +
            0.2 * termination_pressure
        )
        
        return loop_pressure
    
    def _calculate_exploration_priority(self,
                                       strategic_value: float,
                                       learning_potential: float,
                                       novelty: float,
                                       loop_pressure: float) -> float:
        """Calculer la priorité d'exploration."""
        # Priorité = combinaison pondérée
        priority = (
            0.3 * strategic_value +
            0.3 * learning_potential +
            0.2 * novelty -
            0.2 * loop_pressure  # Pénalité pour loops
        )
        
        return max(0.0, min(1.0, priority))
    
    def _determine_cognitive_status(self,
                                    strategic_value: float,
                                    learning_potential: float,
                                    reward_probability: float,
                                    loop_pressure: float) -> str:
        """Déterminer le statut cognitif de la région."""
        # Sterile: faible valeur ET faible potentiel ET haute pression loop
        if (strategic_value < 0.2 and 
            learning_potential < 0.2 and 
            loop_pressure > 0.7):
            return "sterile"
        
        # Promising: haute valeur OU haut potentiel
        if strategic_value > 0.7 or learning_potential > 0.7:
            return "promising"
        
        # Goal candidate: haute probabilité de reward
        if reward_probability > 0.7:
            return "goal_candidate"
        
        # Explored: faible potentiel mais pas sterile
        if learning_potential < 0.3:
            return "explored"
        
        # Neutral: tout le reste
        return "neutral"
    
    def _calculate_confidence(self, region_data: Dict) -> float:
        """Calculer la confiance dans l'évaluation."""
        # Confiance basée sur le nombre de visites
        visit_count = region_data.get('visit_count', 0)
        confidence = min(1.0, visit_count / 10.0)
        
        # Bonus si région bien définie
        region_confidence = region_data.get('confidence', 0.0)
        confidence = 0.7 * confidence + 0.3 * region_confidence
        
        return confidence
    
    def _generate_reasoning(self,
                           region_data: Dict,
                           strategic_value: float,
                           learning_potential: float,
                           novelty: float,
                           reward_probability: float,
                           loop_pressure: float,
                           cognitive_status: str) -> List[str]:
        """Générer le raisonnement de l'évaluation."""
        reasoning = []
        
        # Statut cognitif
        reasoning.append(f"cognitive_status={cognitive_status}")
        
        # Valeur stratégique
        if strategic_value > 0.7:
            reasoning.append("high_strategic_value")
        elif strategic_value < 0.3:
            reasoning.append("low_strategic_value")
        
        # Potentiel d'apprentissage
        if learning_potential > 0.7:
            reasoning.append("high_learning_potential")
        elif learning_potential < 0.3:
            reasoning.append("low_learning_potential")
        
        # Nouveauté
        if novelty > 0.7:
            reasoning.append("high_novelty")
        elif novelty < 0.3:
            reasoning.append("well_explored")
        
        # Reward
        if reward_probability > 0.7:
            reasoning.append("high_reward_probability")
        elif reward_probability < 0.3:
            reasoning.append("low_reward_probability")
        
        # Loop pressure
        if loop_pressure > 0.7:
            reasoning.append("high_loop_pressure")
        
        # Type de région
        region_type = region_data.get('type', 'unknown')
        reasoning.append(f"type={region_type}")
        
        # Termination
        termination = region_data.get('termination', None)
        if termination:
            reasoning.append(f"termination={termination}")
        
        return reasoning
    
    def get_assessment(self, region_id: str) -> Optional[RegionStrategicAssessment]:
        """Obtenir l'évaluation d'une région."""
        return self.assessments.get(region_id)
    
    def get_top_priority_regions(self, n: int = 5) -> List[Tuple[str, RegionStrategicAssessment]]:
        """Obtenir les n régions avec la plus haute priorité."""
        sorted_regions = sorted(
            self.assessments.items(),
            key=lambda x: x[1].exploration_priority,
            reverse=True
        )
        return sorted_regions[:n]
    
    def get_sterile_regions(self) -> List[str]:
        """Obtenir toutes les régions stériles."""
        return [
            rid for rid, assessment in self.assessments.items()
            if assessment.cognitive_status == "sterile"
        ]
    
    def get_promising_regions(self) -> List[str]:
        """Obtenir toutes les régions prometteuses."""
        return [
            rid for rid, assessment in self.assessments.items()
            if assessment.cognitive_status == "promising"
        ]
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques du stratège."""
        if not self.assessments:
            return {
                "total_assessments": 0,
                "sterile_regions": 0,
                "promising_regions": 0
            }
        
        # Compter par statut
        status_counts = {}
        for assessment in self.assessments.values():
            status = assessment.cognitive_status
            status_counts[status] = status_counts.get(status, 0) + 1
        
        # Moyennes
        avg_strategic_value = sum(a.strategic_value for a in self.assessments.values()) / len(self.assessments)
        avg_learning_potential = sum(a.learning_potential for a in self.assessments.values()) / len(self.assessments)
        avg_exploration_priority = sum(a.exploration_priority for a in self.assessments.values()) / len(self.assessments)
        
        return {
            "total_assessments": self.total_assessments,
            "current_regions": len(self.assessments),
            "sterile_regions": self.sterile_regions_identified,
            "promising_regions": self.promising_regions_identified,
            "status_distribution": status_counts,
            "avg_strategic_value": avg_strategic_value,
            "avg_learning_potential": avg_learning_potential,
            "avg_exploration_priority": avg_exploration_priority
        }
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
    
    def export_to_json(self, filepath: str) -> None:
        """Exporter évaluations en JSON."""
        data = {
            "statistics": self.get_statistics(),
            "assessments": {
                rid: assessment.to_dict()
                for rid, assessment in self.assessments.items()
            },
            "history": self.assessment_history[-50:]  # 50 dernières
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    def close(self) -> None:
        """Fermeture propre du module avec forensic."""
        self.forensic.forensic_logger.log(
            "module_closing",
            "RegionalStrategist",
            {
                "final_state": self._forensic_state,
                "total_assessments": self.total_assessments,
                "regions_evaluated": len(self.assessments),
                "uptime_ns": time.time_ns() - self.creation_time
            }
        )
        self.forensic.forensic_logger.close()

# Made with Bob
