"""
Contextual Action Validator - MAGEN V27.1
==========================================

Principe:
    Erreur API ≠ Erreur technique
    Erreur API = Information sur le monde
    
    500 → Action invalide dans ce contexte
    400 → Précondition non satisfaite

Objectif:
    Apprendre validité contextuelle des actions
    Éviter erreurs répétées
    Construire modèle affordances

Author: MAGEN V27.1 CAUSAL
Date: 2026-06-14
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set
from dataclasses import dataclass, field
from collections import defaultdict
import time
import hashlib
import json


@dataclass
class ActionContext:
    """Contexte d'exécution d'une action"""
    grid_hash: str
    agent_position: Optional[Tuple[int, int]]
    goal_position: Optional[Tuple[int, int]]
    entities_count: int
    state_name: str
    timestamp: int


@dataclass
class ActionValidityRecord:
    """Enregistrement validité action"""
    action: str
    context_hash: str
    is_valid: bool
    error_code: Optional[int]
    error_message: Optional[str]
    timestamp: int
    attempts: int = 1


class ContextualActionValidator:
    """
    Validateur contextuel d'actions.
    
    Capacités:
    1. Apprendre validité actions par contexte
    2. Transformer erreurs API en signal cognitif
    3. Construire modèle affordances
    4. Prédire validité avant exécution
    5. Blacklist intelligent contextuel
    """
    
    def __init__(
        self,
        confidence_threshold: float = 0.7,
        min_samples: int = 3,
        error_penalty: float = 0.5,
        success_bonus: float = 0.3
    ):
        """
        Args:
            confidence_threshold: Seuil confiance pour prédiction
            min_samples: Échantillons min pour prédiction fiable
            error_penalty: Pénalité score après erreur
            success_bonus: Bonus score après succès
        """
        self.confidence_threshold = confidence_threshold
        self.min_samples = min_samples
        self.error_penalty = error_penalty
        self.success_bonus = success_bonus
        
        # Modèle validité: {action: {context_hash: validity_score}}
        self.validity_model: Dict[str, Dict[str, float]] = defaultdict(lambda: defaultdict(float))
        
        # Historique: {action: {context_hash: [records]}}
        self.history: Dict[str, Dict[str, List[ActionValidityRecord]]] = defaultdict(lambda: defaultdict(list))
        
        # Statistiques globales par action
        self.action_stats: Dict[str, Dict] = defaultdict(lambda: {
            'total_attempts': 0,
            'total_successes': 0,
            'total_errors': 0,
            'error_types': defaultdict(int),
            'success_rate': 0.0
        })
        
        # Blacklist temporaire: {action: {context_hash: expiry_timestamp}}
        self.blacklist: Dict[str, Dict[str, int]] = defaultdict(dict)
        self.blacklist_duration_ns = 300 * 1e9  # 5 minutes
        
        # Forensic bit-level nanoseconde
        self.learning_log: List[Dict] = []
        self.forensic_enabled = True
        self.forensic_log_path: Optional[str] = None
    
    def learn_from_execution(
        self,
        action: str,
        context: ActionContext,
        success: bool,
        error_code: Optional[int] = None,
        error_message: Optional[str] = None
    ):
        """
        Apprend de l'exécution d'une action.
        
        Args:
            action: Action exécutée
            context: Contexte d'exécution
            success: Succès ou échec
            error_code: Code erreur si échec
            error_message: Message erreur si échec
        """
        timestamp_start = int(time.time_ns())
        context_hash = self._hash_context(context)
        
        # Forensic: Log learning start
        if self.forensic_enabled:
            self._log_forensic('learning_start', {
                'timestamp_ns': timestamp_start,
                'action': action,
                'context_hash': context_hash,
                'success': success,
                'error_code': error_code
            })
        
        # Créer enregistrement
        record = ActionValidityRecord(
            action=action,
            context_hash=context_hash,
            is_valid=success,
            error_code=error_code,
            error_message=error_message,
            timestamp=int(time.time_ns())
        )
        
        # Ajouter à historique
        self.history[action][context_hash].append(record)
        
        # Mettre à jour modèle validité
        current_score = self.validity_model[action][context_hash]
        
        if success:
            # Succès → augmenter score
            new_score = min(1.0, current_score + self.success_bonus)
            self.action_stats[action]['total_successes'] += 1
        else:
            # Échec → diminuer score
            new_score = max(0.0, current_score - self.error_penalty)
            self.action_stats[action]['total_errors'] += 1
            
            if error_code:
                self.action_stats[action]['error_types'][error_code] += 1
            
            # Blacklist si erreur critique
            if error_code in [500, 400]:
                self.blacklist[action][context_hash] = int(time.time_ns() + self.blacklist_duration_ns)
        
        self.validity_model[action][context_hash] = new_score
        
        # Mettre à jour statistiques globales
        self.action_stats[action]['total_attempts'] += 1
        total = self.action_stats[action]['total_attempts']
        successes = self.action_stats[action]['total_successes']
        self.action_stats[action]['success_rate'] = successes / total if total > 0 else 0.0
        
        # Log apprentissage
        self.learning_log.append({
            'timestamp': record.timestamp,
            'action': action,
            'context_hash': context_hash,
            'success': success,
            'error_code': error_code,
            'validity_score': new_score,
            'blacklisted': context_hash in self.blacklist.get(action, {})
        })
        
        # Limiter log
        if len(self.learning_log) > 1000:
            self.learning_log.pop(0)
        
        # Forensic: Log learning complete
        timestamp_end = int(time.time_ns())
        if self.forensic_enabled:
            self._log_forensic('learning_complete', {
                'timestamp_ns': timestamp_end,
                'duration_ns': timestamp_end - timestamp_start,
                'action': action,
                'context_hash': context_hash,
                'validity_score': new_score,
                'blacklisted': context_hash in self.blacklist.get(action, {}),
                'total_attempts': self.action_stats[action]['total_attempts'],
                'success_rate': self.action_stats[action]['success_rate']
            })
    
    def is_valid(
        self,
        action: str,
        context: ActionContext
    ) -> Tuple[bool, float]:
        """
        Prédit si action est valide dans contexte.
        
        Args:
            action: Action à valider
            context: Contexte actuel
        
        Returns:
            (is_valid, confidence): Prédiction + confiance
        """
        context_hash = self._hash_context(context)
        
        # Vérifier blacklist
        if action in self.blacklist:
            if context_hash in self.blacklist[action]:
                expiry = self.blacklist[action][context_hash]
                if int(time.time_ns()) < expiry:
                    # Encore blacklisté
                    return False, 1.0
                else:
                    # Expiration blacklist
                    del self.blacklist[action][context_hash]
        
        # Vérifier modèle validité
        if action in self.validity_model:
            if context_hash in self.validity_model[action]:
                score = self.validity_model[action][context_hash]
                samples = len(self.history[action][context_hash])
                
                if samples >= self.min_samples:
                    # Prédiction fiable
                    is_valid = score >= self.confidence_threshold
                    confidence = min(1.0, samples / (self.min_samples * 2))
                    return is_valid, confidence
        
        # Pas assez d'information → optimiste par défaut
        # Mais avec confiance faible
        global_success_rate = self.action_stats[action]['success_rate']
        
        # Forensic: Log validation query
        if self.forensic_enabled:
            self._log_forensic('validation_query', {
                'timestamp_ns': int(time.time_ns()),
                'action': action,
                'context_hash': context_hash,
                'is_valid': True,
                'confidence': global_success_rate,
                'reason': 'insufficient_data'
            })
        
        return True, global_success_rate
    
    def get_valid_actions(
        self,
        actions: List[str],
        context: ActionContext
    ) -> List[Tuple[str, float]]:
        """
        Filtre actions valides dans contexte.
        
        Args:
            actions: Liste actions candidates
            context: Contexte actuel
        
        Returns:
            valid_actions: Liste (action, confidence) triée par confiance
        """
        valid = []
        
        for action in actions:
            is_valid, confidence = self.is_valid(action, context)
            if is_valid:
                valid.append((action, confidence))
        
        # Trier par confiance décroissante
        valid.sort(key=lambda x: x[1], reverse=True)
        
        return valid
    
    def _hash_context(self, context: ActionContext) -> str:
        """Hash contexte pour indexation"""
        # Créer signature contexte
        signature = f"{context.grid_hash}_{context.agent_position}_{context.goal_position}_{context.entities_count}_{context.state_name}"
        
        # Hash MD5
        return hashlib.md5(signature.encode()).hexdigest()[:16]
    
    def _hash_grid(self, grid: np.ndarray) -> str:
        """Hash rapide grille"""
        flat = grid.flatten()
        signature = f"{grid.shape}_{np.sum(flat)}_{np.std(flat):.2f}"
        return hashlib.md5(signature.encode()).hexdigest()[:8]
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques validation"""
        total_contexts = sum(
            len(contexts)
            for action_contexts in self.validity_model.values()
            for contexts in [action_contexts]
        )
        
        blacklisted_count = sum(
            len(contexts)
            for action_blacklist in self.blacklist.values()
            for contexts in [action_blacklist]
        )
        
        return {
            'total_actions_tracked': len(self.action_stats),
            'total_contexts_learned': total_contexts,
            'total_blacklisted': blacklisted_count,
            'action_stats': dict(self.action_stats),
            'learning_events': len(self.learning_log)
        }
    
    def get_action_report(self, action: str) -> Dict:
        """Rapport détaillé pour une action"""
        if action not in self.action_stats:
            return {
                'action': action,
                'tracked': False
            }
        
        stats = self.action_stats[action]
        contexts_count = len(self.validity_model.get(action, {}))
        blacklisted_contexts = len(self.blacklist.get(action, {}))
        
        return {
            'action': action,
            'tracked': True,
            'total_attempts': stats['total_attempts'],
            'total_successes': stats['total_successes'],
            'total_errors': stats['total_errors'],
            'success_rate': stats['success_rate'],
            'error_types': dict(stats['error_types']),
            'contexts_learned': contexts_count,
            'contexts_blacklisted': blacklisted_contexts
        }
    
    def reset(self):
        """Réinitialise pour nouveau jeu"""
        # Ne pas réinitialiser complètement le modèle
        # Car apprentissage cross-game est utile
        # Seulement nettoyer blacklist temporaire
        self.blacklist.clear()
        
        # Optionnel: décroissance scores pour oublier ancien contexte
        for action in self.validity_model:
            for context_hash in self.validity_model[action]:
                self.validity_model[action][context_hash] *= 0.9
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensic bit-level nanoseconde"""
        if not self.forensic_enabled:
            return
        
        event = {
            'timestamp_ns': int(time.time_ns()),
            'component': 'ContextualActionValidator',
            'event_type': event_type,
            'data': data
        }
        
        # Écrire dans fichier si path défini
        if self.forensic_log_path:
            try:
                with open(self.forensic_log_path, 'a') as f:
                    f.write(json.dumps(event) + '\n')
            except Exception as e:
                print(f"[FORENSIC ERROR] {e}")
    
    def set_forensic_log_path(self, path: str):
        """Configure chemin fichier log forensic"""
        self.forensic_log_path = path

# Made with Bob
