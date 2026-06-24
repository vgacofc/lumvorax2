"""
MAGEN V34 - SterilityMemory
Mémoire des Zones et Actions Stériles

Mission: Stocker et gérer la mémoire des régions/actions qui n'ont jamais produit de progrès
Architecture: Composant critique du MetaCognitiveController

NOUVEAUTÉ V34: Capacité de dire "ça ne marche pas"

Auteur: Bob (LumVorax AI)
Date: 2026-06-15
Version: V34.0.0
"""

import time
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from collections import defaultdict
import json


@dataclass
class SterilityRecord:
    """Enregistrement de stérilité pour une zone/action"""
    region_id: str
    action: str
    visits: int = 0
    total_reward: float = 0.0
    last_visit_step: int = 0
    sterility_score: float = 0.0
    first_visit_step: int = 0
    
    def to_dict(self) -> Dict:
        return {
            "region_id": self.region_id,
            "action": self.action,
            "visits": self.visits,
            "total_reward": self.total_reward,
            "last_visit_step": self.last_visit_step,
            "sterility_score": self.sterility_score,
            "first_visit_step": self.first_visit_step
        }


class SterilityMemory:
    """
    Mémoire des zones et actions stériles.
    
    Principe fondamental:
        Si une (région, action) est visitée N fois sans reward,
        elle est marquée comme stérile et pénalisée dans le scoring.
    
    Règles:
        1. 0 reward après N visites → +stérilité
        2. Répétition sans progrès → +stérilité
        3. Temps passé sans gain → +stérilité
        4. Stérilité > seuil → pénalité forte
    
    CRITIQUE: Ce module transforme exploration aveugle en exploration mémoire-guidée
    """
    
    def __init__(
        self,
        forensic_logger=None,
        sterility_threshold: float = 0.8,
        max_visits_before_sterile: int = 10,
        decay_rate: float = 0.95
    ):
        """
        Initialise la mémoire de stérilité
        
        Args:
            forensic_logger: Logger forensic pour traçabilité
            sterility_threshold: Seuil au-delà duquel zone considérée stérile
            max_visits_before_sterile: Nombre visites max avant stérilité
            decay_rate: Taux de décroissance stérilité (oubli progressif)
        """
        self.forensic_logger = forensic_logger
        
        # Configuration
        self.sterility_threshold = sterility_threshold
        self.max_visits_before_sterile = max_visits_before_sterile
        self.decay_rate = decay_rate
        
        # Mémoires principales
        self.region_records: Dict[str, SterilityRecord] = {}
        self.action_records: Dict[str, SterilityRecord] = {}
        self.region_action_records: Dict[Tuple[str, str], SterilityRecord] = {}
        
        # Statistiques
        self.total_updates: int = 0
        self.sterile_regions: Set[str] = set()
        self.sterile_actions: Set[str] = set()
        self.sterile_combinations: Set[Tuple[str, str]] = set()
        
        # Forensic
        self.creation_time = time.time_ns()
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_initialized",
                "SterilityMemory",
                {
                    "timestamp_ns": self.creation_time,
                    "version": "v34.0.0",
                    "sterility_threshold": self.sterility_threshold,
                    "max_visits_before_sterile": self.max_visits_before_sterile,
                    "decay_rate": self.decay_rate
                }
            )
    
    def update(
        self,
        region_id: str,
        action: str,
        reward: float,
        current_step: int
    ) -> None:
        """
        Met à jour la mémoire de stérilité après une action
        
        Args:
            region_id: Identifiant de la région
            action: Action exécutée
            reward: Reward obtenu
            current_step: Numéro du step actuel
        """
        self.total_updates += 1
        
        # Mettre à jour enregistrement région
        self._update_region(region_id, reward, current_step)
        
        # Mettre à jour enregistrement action
        self._update_action(action, reward, current_step)
        
        # Mettre à jour enregistrement combinaison
        self._update_combination(region_id, action, reward, current_step)
        
        # Appliquer décroissance (oubli progressif)
        if self.total_updates % 100 == 0:
            self._apply_decay()
        
        # Log forensic
        if self.forensic_logger:
            self.forensic_logger.log(
                "sterility_updated",
                "SterilityMemory",
                {
                    "step": current_step,
                    "region_id": region_id,
                    "action": action,
                    "reward": reward,
                    "region_sterility": self.get_region_sterility(region_id),
                    "action_sterility": self.get_action_sterility(action),
                    "combination_sterility": self.get_combination_sterility(region_id, action)
                }
            )
    
    def _update_region(self, region_id: str, reward: float, step: int) -> None:
        """Met à jour stérilité d'une région"""
        if region_id not in self.region_records:
            self.region_records[region_id] = SterilityRecord(
                region_id=region_id,
                action="",
                first_visit_step=step
            )
        
        record = self.region_records[region_id]
        record.visits += 1
        record.total_reward += reward
        record.last_visit_step = step
        
        # Calculer stérilité
        if record.visits >= self.max_visits_before_sterile:
            avg_reward = record.total_reward / record.visits
            
            if avg_reward <= 0.0:
                # Aucun reward → augmenter stérilité
                record.sterility_score = min(
                    1.0,
                    record.sterility_score + 0.1
                )
                
                # Marquer comme stérile si seuil dépassé
                if record.sterility_score >= self.sterility_threshold:
                    self.sterile_regions.add(region_id)
    
    def _update_action(self, action: str, reward: float, step: int) -> None:
        """Met à jour stérilité d'une action"""
        if action not in self.action_records:
            self.action_records[action] = SterilityRecord(
                region_id="",
                action=action,
                first_visit_step=step
            )
        
        record = self.action_records[action]
        record.visits += 1
        record.total_reward += reward
        record.last_visit_step = step
        
        # Calculer stérilité
        if record.visits >= self.max_visits_before_sterile:
            avg_reward = record.total_reward / record.visits
            
            if avg_reward <= 0.0:
                record.sterility_score = min(
                    1.0,
                    record.sterility_score + 0.05  # Moins agressif que région
                )
                
                if record.sterility_score >= self.sterility_threshold:
                    self.sterile_actions.add(action)
    
    def _update_combination(
        self,
        region_id: str,
        action: str,
        reward: float,
        step: int
    ) -> None:
        """Met à jour stérilité d'une combinaison (région, action)"""
        key = (region_id, action)
        
        if key not in self.region_action_records:
            self.region_action_records[key] = SterilityRecord(
                region_id=region_id,
                action=action,
                first_visit_step=step
            )
        
        record = self.region_action_records[key]
        record.visits += 1
        record.total_reward += reward
        record.last_visit_step = step
        
        # Calculer stérilité
        if record.visits >= self.max_visits_before_sterile:
            avg_reward = record.total_reward / record.visits
            
            if avg_reward <= 0.0:
                record.sterility_score = min(
                    1.0,
                    record.sterility_score + 0.15  # Plus agressif (combinaison spécifique)
                )
                
                if record.sterility_score >= self.sterility_threshold:
                    self.sterile_combinations.add(key)
    
    def _apply_decay(self) -> None:
        """Applique décroissance (oubli progressif) à tous les scores"""
        # Décroissance régions
        for record in self.region_records.values():
            record.sterility_score *= self.decay_rate
            
            # Retirer de la liste stérile si score trop bas
            if record.sterility_score < self.sterility_threshold:
                self.sterile_regions.discard(record.region_id)
        
        # Décroissance actions
        for record in self.action_records.values():
            record.sterility_score *= self.decay_rate
            
            if record.sterility_score < self.sterility_threshold:
                self.sterile_actions.discard(record.action)
        
        # Décroissance combinaisons
        for key, record in self.region_action_records.items():
            record.sterility_score *= self.decay_rate
            
            if record.sterility_score < self.sterility_threshold:
                self.sterile_combinations.discard(key)
    
    def is_sterile(self, region_id: str, action: str) -> bool:
        """
        Vérifie si une combinaison (région, action) est stérile
        
        Args:
            region_id: Identifiant région
            action: Action
            
        Returns:
            True si stérile, False sinon
        """
        # Vérifier combinaison spécifique
        if (region_id, action) in self.sterile_combinations:
            return True
        
        # Vérifier région seule
        if region_id in self.sterile_regions:
            return True
        
        # Vérifier action seule
        if action in self.sterile_actions:
            return True
        
        return False
    
    def get_sterility_penalty(self, region_id: str, action: str) -> float:
        """
        Retourne pénalité de stérilité pour scoring
        
        Args:
            region_id: Identifiant région
            action: Action
            
        Returns:
            Pénalité entre 0.0 et 1.0
        """
        # Récupérer scores individuels
        region_sterility = self.get_region_sterility(region_id)
        action_sterility = self.get_action_sterility(action)
        combination_sterility = self.get_combination_sterility(region_id, action)
        
        # Prendre le maximum (pire cas)
        max_sterility = max(region_sterility, action_sterility, combination_sterility)
        
        return max_sterility
    
    def get_region_sterility(self, region_id: str) -> float:
        """Retourne score stérilité d'une région"""
        if region_id in self.region_records:
            return self.region_records[region_id].sterility_score
        return 0.0
    
    def get_action_sterility(self, action: str) -> float:
        """Retourne score stérilité d'une action"""
        if action in self.action_records:
            return self.action_records[action].sterility_score
        return 0.0
    
    def get_combination_sterility(self, region_id: str, action: str) -> float:
        """Retourne score stérilité d'une combinaison"""
        key = (region_id, action)
        if key in self.region_action_records:
            return self.region_action_records[key].sterility_score
        return 0.0
    
    def get_sterile_regions(self) -> List[str]:
        """Retourne liste des régions stériles"""
        return list(self.sterile_regions)
    
    def get_sterile_actions(self) -> List[str]:
        """Retourne liste des actions stériles"""
        return list(self.sterile_actions)
    
    def get_sterile_combinations(self) -> List[Tuple[str, str]]:
        """Retourne liste des combinaisons stériles"""
        return list(self.sterile_combinations)
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques globales"""
        return {
            "total_updates": self.total_updates,
            "total_regions": len(self.region_records),
            "total_actions": len(self.action_records),
            "total_combinations": len(self.region_action_records),
            "sterile_regions_count": len(self.sterile_regions),
            "sterile_actions_count": len(self.sterile_actions),
            "sterile_combinations_count": len(self.sterile_combinations),
            "sterility_threshold": self.sterility_threshold,
            "max_visits_before_sterile": self.max_visits_before_sterile
        }
    
    def get_state(self) -> Dict:
        """Retourne état complet pour forensic"""
        return {
            "statistics": self.get_statistics(),
            "sterile_regions": self.get_sterile_regions(),
            "sterile_actions": self.get_sterile_actions(),
            "sterile_combinations_count": len(self.sterile_combinations)
        }
    
    def reset(self) -> None:
        """Reset complet de la mémoire"""
        self.region_records.clear()
        self.action_records.clear()
        self.region_action_records.clear()
        self.sterile_regions.clear()
        self.sterile_actions.clear()
        self.sterile_combinations.clear()
        self.total_updates = 0
        
        if self.forensic_logger:
            self.forensic_logger.log(
                "memory_reset",
                "SterilityMemory",
                {
                    "timestamp_ns": time.time_ns(),
                    "reason": "manual_reset"
                }
            )
    
    def close(self) -> None:
        """Ferme le module et log statistiques finales"""
        if self.forensic_logger:
            self.forensic_logger.log(
                "module_closing",
                "SterilityMemory",
                {
                    "timestamp_ns": time.time_ns(),
                    "final_statistics": self.get_statistics(),
                    "final_state": self.get_state()
                }
            )


# Export
__all__ = ['SterilityMemory', 'SterilityRecord']

# Made with Bob
