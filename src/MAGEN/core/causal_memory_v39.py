"""
PHASE 4 V39: Causal Memory System
Mémoire des transformations et de leurs effets causaux.

Auteur: Bob (Assistant IA)
Date: 2026-06-16
"""

import numpy as np
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, field
from datetime import datetime
import json
from collections import defaultdict


@dataclass
class CausalEvent:
    """Événement causal (transformation + effet)."""
    event_id: str
    timestamp: float
    transformation: str
    input_state: Dict
    output_state: Dict
    success: bool
    error: float
    context: Dict = field(default_factory=dict)
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'event_id': self.event_id,
            'timestamp': self.timestamp,
            'transformation': self.transformation,
            'success': self.success,
            'error': self.error,
            'context': self.context
        }


@dataclass
class CausalChain:
    """Chaîne causale (séquence de transformations)."""
    chain_id: str
    events: List[CausalEvent] = field(default_factory=list)
    total_error: float = 0.0
    success_rate: float = 0.0
    
    def add_event(self, event: CausalEvent):
        """Ajoute un événement à la chaîne."""
        self.events.append(event)
        self._update_stats()
    
    def _update_stats(self):
        """Met à jour les statistiques."""
        if not self.events:
            return
        
        successes = sum(1 for e in self.events if e.success)
        self.success_rate = successes / len(self.events)
        self.total_error = sum(e.error for e in self.events)
    
    def length(self) -> int:
        """Retourne la longueur de la chaîne."""
        return len(self.events)
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'chain_id': self.chain_id,
            'length': self.length(),
            'success_rate': self.success_rate,
            'total_error': self.total_error,
            'events': [e.to_dict() for e in self.events]
        }


class CausalMemorySystem:
    """
    Système de mémoire causale pour MAGEN.
    Mémorise les transformations et leurs effets.
    """
    
    def __init__(self, max_events: int = 10000):
        """
        Initialise le système de mémoire causale.
        
        Args:
            max_events: Nombre maximum d'événements à mémoriser
        """
        self.max_events = max_events
        
        # Événements causaux
        self.events: List[CausalEvent] = []
        
        # Chaînes causales
        self.chains: Dict[str, CausalChain] = {}
        
        # Index par transformation
        self.transformation_index: Dict[str, List[CausalEvent]] = defaultdict(list)
        
        # Statistiques causales
        self.causal_stats: Dict[str, Dict] = {}
        
        # Statistiques globales
        self.stats = {
            'total_events': 0,
            'total_chains': 0,
            'successful_transformations': 0,
            'failed_transformations': 0,
            'avg_chain_length': 0.0
        }
        
        # Forensic logging
        self.forensic_events: List[Dict] = []
    
    def record_transformation(self, transformation: str, input_state: Dict,
                            output_state: Dict, success: bool, error: float,
                            context: Optional[Dict] = None) -> CausalEvent:
        """
        Enregistre une transformation et son effet.
        
        Args:
            transformation: Nom de la transformation
            input_state: État avant transformation
            output_state: État après transformation
            success: Succès de la transformation
            error: Erreur de la transformation
            context: Contexte additionnel (optionnel)
        
        Returns:
            Événement causal créé
        """
        # Créer événement
        event = CausalEvent(
            event_id=f"event_{self.stats['total_events']:06d}",
            timestamp=datetime.now().timestamp(),
            transformation=transformation,
            input_state=input_state,
            output_state=output_state,
            success=success,
            error=error,
            context=context or {}
        )
        
        # Ajouter à la mémoire
        self.events.append(event)
        self.transformation_index[transformation].append(event)
        
        # Limiter taille mémoire
        if len(self.events) > self.max_events:
            old_event = self.events.pop(0)
            self.transformation_index[old_event.transformation].remove(old_event)
        
        # Mettre à jour statistiques
        self.stats['total_events'] += 1
        if success:
            self.stats['successful_transformations'] += 1
        else:
            self.stats['failed_transformations'] += 1
        
        # Mettre à jour statistiques causales
        self._update_causal_stats(transformation, success, error)
        
        # V40.1: Créer chaînes causales automatiquement
        self._auto_create_chains()
        
        # Log forensic
        self._log_forensic_event({
            'event': 'transformation_recorded',
            'transformation': transformation,
            'success': success,
            'error': error
        })
        
        return event
    
    def create_causal_chain(self, chain_id: str, events: List[CausalEvent]) -> CausalChain:
        """
        Crée une chaîne causale à partir d'événements.
        
        Args:
            chain_id: ID de la chaîne
            events: Liste d'événements
        
        Returns:
            Chaîne causale créée
        """
        chain = CausalChain(chain_id=chain_id)
        for event in events:
            chain.add_event(event)
        
        self.chains[chain_id] = chain
        self.stats['total_chains'] += 1
        
        # Mettre à jour longueur moyenne
        if self.chains:
            self.stats['avg_chain_length'] = np.mean([c.length() for c in self.chains.values()])
        
        # Log forensic
        self._log_forensic_event({
            'event': 'chain_created',
            'chain_id': chain_id,
            'length': chain.length(),
            'success_rate': chain.success_rate
        })
        
        return chain
    
    def get_transformation_history(self, transformation: str) -> List[CausalEvent]:
        """Retourne l'historique d'une transformation."""
        return self.transformation_index.get(transformation, [])
    
    def get_transformation_success_rate(self, transformation: str) -> float:
        """Retourne le taux de succès d'une transformation."""
        events = self.get_transformation_history(transformation)
        if not events:
            return 0.0
        successes = sum(1 for e in events if e.success)
        return successes / len(events)
    
    def get_transformation_avg_error(self, transformation: str) -> float:
        """Retourne l'erreur moyenne d'une transformation."""
        events = self.get_transformation_history(transformation)
        if not events:
            return 1.0
        return np.mean([e.error for e in events])
    
    def get_best_transformations(self, top_k: int = 5) -> List[Tuple[str, float]]:
        """
        Retourne les meilleures transformations.
        
        Args:
            top_k: Nombre de transformations à retourner
        
        Returns:
            Liste de (transformation, score)
        """
        scores = []
        for transformation in self.transformation_index.keys():
            success_rate = self.get_transformation_success_rate(transformation)
            avg_error = self.get_transformation_avg_error(transformation)
            # Score = success_rate * (1 - avg_error)
            score = success_rate * (1.0 - avg_error)
            scores.append((transformation, score))
        
        # Trier par score décroissant
        scores.sort(key=lambda x: x[1], reverse=True)
        return scores[:top_k]
    
    def get_causal_patterns(self) -> Dict[str, List[str]]:
        """
        Détecte les patterns causaux (séquences fréquentes).
        
        Returns:
            Dictionnaire de patterns
        """
        patterns = defaultdict(list)
        
        # Analyser chaînes pour détecter patterns
        for chain in self.chains.values():
            if chain.length() < 2:
                continue
            
            # Extraire séquences de transformations
            sequence = [e.transformation for e in chain.events]
            
            # Patterns de longueur 2
            for i in range(len(sequence) - 1):
                pattern = f"{sequence[i]} -> {sequence[i+1]}"
                patterns['length_2'].append(pattern)
            
            # Patterns de longueur 3
            for i in range(len(sequence) - 2):
                pattern = f"{sequence[i]} -> {sequence[i+1]} -> {sequence[i+2]}"
                patterns['length_3'].append(pattern)
        
        return dict(patterns)
    
    def _auto_create_chains(self, window_size: int = 10):
        """
        Crée automatiquement des chaînes causales à partir des événements récents.
        
        Args:
            window_size: Taille de la fenêtre pour grouper événements
        """
        # Prendre les derniers événements
        recent_events = self.events[-window_size:] if len(self.events) >= window_size else self.events
        
        if len(recent_events) < 2:
            return
        
        # Créer chaîne avec événements récents
        chain_id = f"chain_{len(self.chains):06d}"
        
        # Vérifier si chaîne existe déjà
        if chain_id in self.chains:
            return
        
        # Créer nouvelle chaîne
        chain = CausalChain(chain_id=chain_id)
        for event in recent_events:
            chain.add_event(event)
        
        # Sauvegarder chaîne
        self.chains[chain_id] = chain
        self.stats['total_chains'] += 1
        
        # Mettre à jour longueur moyenne
        if self.chains:
            self.stats['avg_chain_length'] = float(np.mean([c.length() for c in self.chains.values()]))
    
    def _update_causal_stats(self, transformation: str, success: bool, error: float):
        """Met à jour les statistiques causales."""
        if transformation not in self.causal_stats:
            self.causal_stats[transformation] = {
                'count': 0,
                'successes': 0,
                'failures': 0,
                'total_error': 0.0,
                'avg_error': 0.0,
                'success_rate': 0.0
            }
        
        stats = self.causal_stats[transformation]
        stats['count'] += 1
        if success:
            stats['successes'] += 1
        else:
            stats['failures'] += 1
        stats['total_error'] += error
        stats['avg_error'] = stats['total_error'] / stats['count']
        stats['success_rate'] = stats['successes'] / stats['count']
    
    def get_stats(self) -> Dict:
        """Retourne les statistiques."""
        return {
            **self.stats,
            'transformations_tracked': len(self.transformation_index),
            'memory_usage': len(self.events)
        }
    
    def to_dict(self) -> Dict:
        """Convertit en dictionnaire."""
        return {
            'stats': self.get_stats(),
            'causal_stats': self.causal_stats,
            'chains_sample': [
                self.chains[cid].to_dict()
                for cid in list(self.chains.keys())[:5]
            ]
        }
    
    def _log_forensic_event(self, event: Dict):
        """Log événement forensic."""
        event['timestamp'] = datetime.now().isoformat()
        self.forensic_events.append(event)
    
    def save_forensic_log(self, filepath: str):
        """Sauvegarde le log forensic."""
        with open(filepath, 'w') as f:
            for event in self.forensic_events:
                f.write(json.dumps(event) + '\n')


def test_causal_memory():
    """Test unitaire du Causal Memory System."""
    print("="*80)
    print("TEST CAUSAL MEMORY SYSTEM V39")
    print("="*80)
    
    # Créer système
    memory = CausalMemorySystem(max_events=1000)
    
    # Simuler transformations
    print("\nSimulation transformations...")
    transformations = ['rotate_90', 'flip_h', 'flip_v', 'identity', 'transpose']
    
    for i in range(100):
        transformation = np.random.choice(transformations)
        success = np.random.random() > 0.3  # 70% succès
        error = np.random.random() * (0.5 if success else 1.0)
        
        input_state = {'step': i, 'value': np.random.randint(0, 10)}
        output_state = {'step': i+1, 'value': np.random.randint(0, 10)}
        
        memory.record_transformation(
            transformation, input_state, output_state, success, error
        )
    
    # Créer chaînes causales
    print("\nCréation chaînes causales...")
    for i in range(5):
        chain_events = memory.events[i*10:(i+1)*10]
        chain = memory.create_causal_chain(f"chain_{i}", chain_events)
        print(f"  Chaîne {i}: {chain.length()} événements, succès {chain.success_rate:.1%}")
    
    # Statistiques
    print("\n" + "="*80)
    print("STATISTIQUES")
    print("="*80)
    
    stats = memory.get_stats()
    print(f"Total événements: {stats['total_events']}")
    print(f"Transformations trackées: {stats['transformations_tracked']}")
    print(f"Succès: {stats['successful_transformations']}")
    print(f"Échecs: {stats['failed_transformations']}")
    print(f"Chaînes créées: {stats['total_chains']}")
    print(f"Longueur moyenne chaînes: {stats['avg_chain_length']:.1f}")
    
    # Meilleures transformations
    print("\nMeilleures transformations:")
    for transformation, score in memory.get_best_transformations(top_k=3):
        print(f"  {transformation}: {score:.3f}")
    
    # Patterns causaux
    patterns = memory.get_causal_patterns()
    print(f"\nPatterns causaux détectés: {sum(len(v) for v in patterns.values())}")
    
    # Sauvegarder forensic
    memory.save_forensic_log('/tmp/causal_memory_test.jsonl')
    print(f"\nForensic log sauvegardé: /tmp/causal_memory_test.jsonl")
    
    # Verdict
    print("\n" + "="*80)
    if stats['total_events'] > 50 and stats['total_chains'] > 0:
        print("✅ TEST RÉUSSI - Causal Memory fonctionnel")
    else:
        print("⚠️  TEST PARTIEL - Événements ou chaînes insuffisants")
    print("="*80)


if __name__ == '__main__':
    test_causal_memory()

# Made with Bob
