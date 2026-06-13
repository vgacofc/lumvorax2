"""
CognitiveLogger - Traçabilité forensique du raisonnement cognitif
Capture CHAQUE étape du processus de raisonnement pour analyse authentique
"""

import time
import json
from typing import Dict, List, Any, Optional
from pathlib import Path
from dataclasses import dataclass, asdict
from enum import Enum


class CognitiveEventType(Enum):
    """Types d'événements cognitifs"""
    HYPOTHESIS_GENERATED = "hypothesis_generated"
    HYPOTHESIS_TESTED = "hypothesis_tested"
    HYPOTHESIS_REJECTED = "hypothesis_rejected"
    HYPOTHESIS_ACCEPTED = "hypothesis_accepted"
    CONFIDENCE_UPDATED = "confidence_updated"
    REASONING_STEP = "reasoning_step"
    DECISION_MADE = "decision_made"
    ALTERNATIVE_CONSIDERED = "alternative_considered"
    VALIDATION_CHECK = "validation_check"
    META_COGNITION = "meta_cognition"
    ACTION_PLANNED = "action_planned"
    PREDICTION_GENERATED = "prediction_generated"


@dataclass
class CognitiveEvent:
    """Événement cognitif tracé"""
    timestamp_ns: int  # Nanoseconde
    event_type: CognitiveEventType
    description: str
    data: Dict[str, Any]
    confidence: Optional[float] = None
    reasoning: Optional[str] = None
    alternatives: Optional[List[str]] = None
    
    def to_dict(self):
        return {
            "timestamp_ns": self.timestamp_ns,
            "event_type": self.event_type.value,
            "description": self.description,
            "data": self.data,
            "confidence": self.confidence,
            "reasoning": self.reasoning,
            "alternatives": self.alternatives
        }


class CognitiveLogger:
    """
    Logger forensique pour traçabilité cognitive complète
    
    Capture:
    - Chaque hypothèse générée/testée/rejetée
    - Évolution de confiance nanoseconde par nanoseconde
    - Arbre de décision complet
    - Raisonnement par élimination
    - Méta-cognition (réflexion sur le raisonnement)
    """
    
    def __init__(self, log_dir: Path, game_id: str):
        self.log_dir = Path(log_dir)
        self.game_id = game_id
        self.events: List[CognitiveEvent] = []
        self.start_time_ns = time.time_ns()
        
        # Créer répertoire
        self.log_dir.mkdir(parents=True, exist_ok=True)
        
        # Fichier de log
        self.log_file = self.log_dir / f"cognitive_trace_{game_id}_{self.start_time_ns}.json"
        
    def log_hypothesis_generated(self, 
                                 hypothesis_type: str,
                                 confidence: float,
                                 parameters: Dict,
                                 reasoning: str,
                                 alternatives_considered: List[str]):
        """Log génération d'hypothèse avec raisonnement complet"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.HYPOTHESIS_GENERATED,
            description=f"Hypothèse générée: {hypothesis_type}",
            data={
                "hypothesis_type": hypothesis_type,
                "parameters": parameters,
                "generation_method": "rule_extraction"
            },
            confidence=confidence,
            reasoning=reasoning,
            alternatives=alternatives_considered
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_hypothesis_tested(self,
                             hypothesis_type: str,
                             test_input_shape: tuple,
                             validation_score: float,
                             test_confidence: float,
                             reasoning: str):
        """Log test d'hypothèse avec résultats détaillés"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.HYPOTHESIS_TESTED,
            description=f"Test hypothèse: {hypothesis_type}",
            data={
                "hypothesis_type": hypothesis_type,
                "test_input_shape": test_input_shape,
                "validation_score": validation_score,
                "test_confidence": test_confidence
            },
            confidence=test_confidence,
            reasoning=reasoning
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_confidence_evolution(self,
                                hypothesis_type: str,
                                old_confidence: float,
                                new_confidence: float,
                                reason: str):
        """Log évolution de confiance avec justification"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.CONFIDENCE_UPDATED,
            description=f"Confiance mise à jour: {old_confidence:.2f} → {new_confidence:.2f}",
            data={
                "hypothesis_type": hypothesis_type,
                "old_confidence": old_confidence,
                "new_confidence": new_confidence,
                "delta": new_confidence - old_confidence
            },
            confidence=new_confidence,
            reasoning=reason
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_reasoning_step(self,
                          step_number: int,
                          description: str,
                          inputs: Dict,
                          outputs: Dict,
                          reasoning: str):
        """Log étape de raisonnement détaillée"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.REASONING_STEP,
            description=f"Étape {step_number}: {description}",
            data={
                "step_number": step_number,
                "inputs": inputs,
                "outputs": outputs
            },
            reasoning=reasoning
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_decision(self,
                    decision_type: str,
                    chosen_option: str,
                    alternatives: List[str],
                    reasoning: str,
                    confidence: float):
        """Log décision avec alternatives et justification"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.DECISION_MADE,
            description=f"Décision: {chosen_option}",
            data={
                "decision_type": decision_type,
                "chosen_option": chosen_option,
                "num_alternatives": len(alternatives)
            },
            confidence=confidence,
            reasoning=reasoning,
            alternatives=alternatives
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_action_plan(self,
                       predicted_output_shape: tuple,
                       action_sequence: List[int],
                       reasoning: str,
                       confidence: float):
        """Log planification d'actions"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.ACTION_PLANNED,
            description=f"Plan d'actions: {len(action_sequence)} actions",
            data={
                "predicted_output_shape": predicted_output_shape,
                "action_sequence": action_sequence,
                "num_actions": len(action_sequence)
            },
            confidence=confidence,
            reasoning=reasoning
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_meta_cognition(self,
                          reflection: str,
                          confidence_in_reasoning: float,
                          identified_gaps: List[str]):
        """Log méta-cognition (réflexion sur le raisonnement)"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.META_COGNITION,
            description="Méta-cognition: réflexion sur le raisonnement",
            data={
                "identified_gaps": identified_gaps,
                "num_gaps": len(identified_gaps)
            },
            confidence=confidence_in_reasoning,
            reasoning=reflection
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_prediction_generated(self,
                                input_shape: tuple,
                                output_shape: tuple,
                                confidence: float,
                                validation_score: float,
                                reasoning: str):
        """Log génération de prédiction"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.PREDICTION_GENERATED,
            description=f"Prédiction générée: {input_shape} → {output_shape}",
            data={
                "input_shape": input_shape,
                "output_shape": output_shape,
                "validation_score": validation_score
            },
            confidence=confidence,
            reasoning=reasoning
        )
        self.events.append(event)
        self._write_event(event)
    
    def log_action_plan_generated(self,
                                 plan: List[int],
                                 subgoals: List[str],
                                 confidence: float,
                                 reasoning: str):
        """Log génération de plan d'actions"""
        event = CognitiveEvent(
            timestamp_ns=time.time_ns(),
            event_type=CognitiveEventType.ACTION_PLANNED,
            description=f"Plan d'actions généré: {len(plan)} actions",
            data={
                "plan": plan,
                "num_actions": len(plan),
                "subgoals": subgoals,
                "num_subgoals": len(subgoals)
            },
            confidence=confidence,
            reasoning=reasoning
        )
        self.events.append(event)
        self._write_event(event)
    
    def _write_event(self, event: CognitiveEvent):
        """Écrire événement dans fichier JSON (append)"""
        with open(self.log_file, 'a') as f:
            json.dump(event.to_dict(), f)
            f.write('\n')
    
    def generate_summary(self) -> Dict:
        """Générer résumé de la trace cognitive"""
        total_duration_ns = time.time_ns() - self.start_time_ns
        
        # Compter événements par type
        event_counts = {}
        for event in self.events:
            event_type = event.event_type.value
            event_counts[event_type] = event_counts.get(event_type, 0) + 1
        
        # Analyser évolution confiance
        confidence_evolution = []
        for event in self.events:
            if event.confidence is not None:
                confidence_evolution.append({
                    "timestamp_ns": event.timestamp_ns,
                    "confidence": event.confidence,
                    "event_type": event.event_type.value
                })
        
        summary = {
            "game_id": self.game_id,
            "total_events": len(self.events),
            "total_duration_ns": total_duration_ns,
            "total_duration_ms": total_duration_ns / 1_000_000,
            "event_counts": event_counts,
            "confidence_evolution": confidence_evolution,
            "log_file": str(self.log_file)
        }
        
        # Sauvegarder résumé
        summary_file = self.log_dir / f"cognitive_summary_{self.game_id}_{self.start_time_ns}.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        return summary
    
    def get_reasoning_percentage(self) -> float:
        """
        Calculer pourcentage de raisonnement capturé
        
        Critères:
        - Hypothèses générées avec alternatives
        - Évolution de confiance tracée
        - Raisonnement explicite fourni
        - Méta-cognition présente
        """
        total_score = 0
        max_score = 0
        
        # 1. Hypothèses avec alternatives (25%)
        hypothesis_events = [e for e in self.events if e.event_type == CognitiveEventType.HYPOTHESIS_GENERATED]
        if hypothesis_events:
            with_alternatives = sum(1 for e in hypothesis_events if e.alternatives and len(e.alternatives) > 0)
            total_score += (with_alternatives / len(hypothesis_events)) * 25
        max_score += 25
        
        # 2. Évolution confiance (25%)
        confidence_events = [e for e in self.events if e.event_type == CognitiveEventType.CONFIDENCE_UPDATED]
        if confidence_events:
            total_score += min(len(confidence_events) / 5, 1.0) * 25  # Au moins 5 updates
        max_score += 25
        
        # 3. Raisonnement explicite (25%)
        with_reasoning = sum(1 for e in self.events if e.reasoning and len(e.reasoning) > 20)
        if self.events:
            total_score += (with_reasoning / len(self.events)) * 25
        max_score += 25
        
        # 4. Méta-cognition (25%)
        meta_events = [e for e in self.events if e.event_type == CognitiveEventType.META_COGNITION]
        if meta_events:
            total_score += min(len(meta_events) / 3, 1.0) * 25  # Au moins 3 réflexions
        max_score += 25
        
        return (total_score / max_score) * 100 if max_score > 0 else 0.0

# Made with Bob
