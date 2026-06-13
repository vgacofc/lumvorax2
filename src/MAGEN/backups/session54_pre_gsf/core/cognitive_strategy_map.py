#!/usr/bin/env python3
"""
Cognitive Strategy Map - Représentation spatiale simultanée des futurs possibles
Inspiré par visualisation mentale humaine: "voir" toutes les solutions en même temps
"""

import numpy as np
from typing import Dict, List, Tuple, Set, Optional, Any
from dataclasses import dataclass, field
from enum import Enum
from collections import defaultdict


class FutureStatus(Enum):
    """Statut d'un futur possible"""
    PROMISING = "promising"      # Zone verte - trajectoire prometteuse
    UNCERTAIN = "uncertain"      # Zone jaune - incertain
    DEAD_END = "dead_end"        # Zone rouge - impasse détectée
    VALIDATED = "validated"      # Zone bleue - validé sur train pairs
    EXECUTING = "executing"      # En cours d'exécution


@dataclass
class FutureNode:
    """
    Nœud représentant un futur possible
    
    Inspiré par perception jeux arcade:
    - Chaque nœud = un état futur possible
    - Couleur = confiance/statut
    - Position = similarité avec autres futurs
    """
    node_id: int
    program: Any
    parent_id: Optional[int]
    
    # Métriques de confiance
    confidence: float  # 0.0 (faible) à 1.0 (haute)
    predicted_accuracy: float
    
    # Statut
    status: FutureStatus
    
    # Analyse prédictive
    predicted_divergence: Optional[Tuple[int, int]]  # (row, col) où divergence prédite
    dead_end_reason: Optional[str]
    
    # Métriques spatiales
    position: Tuple[float, float]  # Position dans carte mentale (x, y)
    similarity_to_neighbors: Dict[int, float]  # Similarité avec autres nœuds
    
    # Historique
    execution_history: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class StrategyPath:
    """Chemin stratégique dans la carte mentale"""
    path_id: int
    nodes: List[int]  # Liste de node_ids
    total_confidence: float
    estimated_success_rate: float
    priority: float  # Score de priorité pour exploration


class CognitiveStrategyMap:
    """
    Carte Mentale Cognitive des Stratégies
    
    Inspiré par visualisation humaine:
    - Générer TOUS les futurs possibles
    - Les représenter spatialement
    - Détecter impasses AVANT exécution
    - Éliminer trajectoires mortes
    - Focus sur zones prometteuses
    
    Architecture:
    - Monte-Carlo Cognitive Search
    - Beam Search Spatial
    - Predictive Dead-End Detection
    - Visual Strategy Representation
    """
    
    def __init__(self, max_futures: int = 100, beam_width: int = 10):
        self.max_futures = max_futures
        self.beam_width = beam_width
        
        # Carte mentale
        self.nodes: Dict[int, FutureNode] = {}
        self.next_node_id = 0
        
        # Chemins stratégiques
        self.paths: List[StrategyPath] = []
        
        # Zones de la carte
        self.promising_zone: Set[int] = set()  # Zone verte
        self.dead_end_zone: Set[int] = set()   # Zone rouge
        self.uncertain_zone: Set[int] = set()  # Zone jaune
        
        # Statistiques
        self.total_futures_generated = 0
        self.dead_ends_detected = 0
        self.early_eliminations = 0
    
    def generate_cognitive_map(self,
                               input_grid: np.ndarray,
                               train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                               program_generators: List[Any],
                               invariants: Any) -> Dict[str, Any]:
        """
        Génère la carte mentale cognitive complète
        
        Args:
            input_grid: Grille d'entrée
            train_pairs: Paires d'entraînement
            program_generators: Liste de générateurs de programmes
            invariants: Profil d'invariants
            
        Returns:
            Carte mentale avec tous les futurs possibles
        """
        print(f"  🧠 Génération carte mentale cognitive...")
        
        # Phase 1: Génération massive de futurs
        futures = self._generate_all_futures(
            input_grid, train_pairs, program_generators, invariants
        )
        print(f"    → {len(futures)} futurs générés")
        
        # Phase 2: Prédiction trajectoires
        self._predict_trajectories(futures, train_pairs, invariants)
        print(f"    → Trajectoires prédites")
        
        # Phase 3: Détection impasses précoce
        dead_ends = self._detect_dead_ends_early(futures, invariants)
        print(f"    → {len(dead_ends)} impasses détectées AVANT exécution")
        
        # Phase 4: Représentation spatiale
        self._create_spatial_representation(futures)
        print(f"    → Représentation spatiale créée")
        
        # Phase 5: Élimination précoce
        eliminated = self._eliminate_dead_ends(dead_ends)
        print(f"    → {eliminated} trajectoires éliminées")
        
        # Phase 6: Focus zones prometteuses
        promising = self._focus_promising_zones(futures)
        print(f"    → {len(promising)} futurs prometteurs identifiés")
        
        # Phase 7: Sélection stratégie optimale
        best_strategy = self._select_optimal_strategy(promising)
        
        return {
            'total_futures': len(futures),
            'promising_futures': len(promising),
            'dead_ends_detected': len(dead_ends),
            'early_eliminations': eliminated,
            'best_strategy': best_strategy,
            'cognitive_map': self.nodes,
            'zones': {
                'promising': list(self.promising_zone),
                'dead_end': list(self.dead_end_zone),
                'uncertain': list(self.uncertain_zone)
            }
        }
    
    def _generate_all_futures(self,
                             input_grid: np.ndarray,
                             train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                             generators: List[Any],
                             invariants: Any) -> List[FutureNode]:
        """
        Génère TOUS les futurs possibles (jusqu'à max_futures)
        
        Returns:
            Liste de FutureNode
        """
        futures = []
        
        for generator in generators:
            try:
                # Générer programmes avec ce générateur
                programs = generator.generate(train_pairs, max_programs=self.max_futures // len(generators))
                
                for program in programs:
                    if len(futures) >= self.max_futures:
                        break
                    
                    # Créer nœud futur
                    node = FutureNode(
                        node_id=self.next_node_id,
                        program=program,
                        parent_id=None,
                        confidence=0.5,  # Initial
                        predicted_accuracy=0.0,
                        status=FutureStatus.UNCERTAIN,
                        predicted_divergence=None,
                        dead_end_reason=None,
                        position=(0.0, 0.0),  # À calculer
                        similarity_to_neighbors={}
                    )
                    
                    futures.append(node)
                    self.nodes[self.next_node_id] = node
                    self.next_node_id += 1
                    self.total_futures_generated += 1
            
            except Exception as e:
                continue
        
        return futures
    
    def _predict_trajectories(self,
                             futures: List[FutureNode],
                             train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                             invariants: Any):
        """
        Prédit les trajectoires de chaque futur
        
        Utilise invariants pour prédire si trajectoire va diverger
        """
        for node in futures:
            try:
                # Exécuter sur premier train pair pour prédiction
                first_input, first_output = train_pairs[0]
                result = node.program.execute(first_input)
                
                if result is None:
                    node.status = FutureStatus.DEAD_END
                    node.dead_end_reason = "execution_failed"
                    node.confidence = 0.0
                    continue
                
                # Vérifier cohérence avec invariants
                if invariants.preserve_shape and result.shape != first_output.shape:
                    node.status = FutureStatus.DEAD_END
                    node.dead_end_reason = "shape_mismatch_predicted"
                    node.confidence = 0.1
                    continue
                
                # Calculer accuracy prédite
                if result.shape == first_output.shape:
                    matches = np.sum(result == first_output)
                    total = first_output.size
                    node.predicted_accuracy = matches / total if total > 0 else 0.0
                else:
                    node.predicted_accuracy = 0.0
                
                # Mettre à jour confiance
                node.confidence = node.predicted_accuracy
                
                # Mettre à jour statut
                if node.predicted_accuracy >= 0.95:
                    node.status = FutureStatus.PROMISING
                elif node.predicted_accuracy >= 0.5:
                    node.status = FutureStatus.UNCERTAIN
                else:
                    node.status = FutureStatus.DEAD_END
                    node.dead_end_reason = "low_predicted_accuracy"
            
            except Exception as e:
                node.status = FutureStatus.DEAD_END
                node.dead_end_reason = f"prediction_error: {str(e)}"
                node.confidence = 0.0
    
    def _detect_dead_ends_early(self,
                                futures: List[FutureNode],
                                invariants: Any) -> List[int]:
        """
        Détecte les impasses AVANT exécution complète
        
        Returns:
            Liste de node_ids identifiés comme impasses
        """
        dead_ends = []
        
        for node in futures:
            # Déjà marqué comme impasse
            if node.status == FutureStatus.DEAD_END:
                dead_ends.append(node.node_id)
                self.dead_ends_detected += 1
                continue
            
            # Vérifications prédictives supplémentaires
            # TODO: Ajouter plus de heuristiques
            
            if node.confidence < 0.3:
                node.status = FutureStatus.DEAD_END
                node.dead_end_reason = "low_confidence"
                dead_ends.append(node.node_id)
                self.dead_ends_detected += 1
        
        return dead_ends
    
    def _create_spatial_representation(self, futures: List[FutureNode]):
        """
        Crée représentation spatiale des futurs
        
        Position basée sur:
        - Similarité entre programmes
        - Confiance
        - Type de transformation
        """
        # Version simplifiée: position basée sur confiance et type
        for i, node in enumerate(futures):
            # X = confiance
            x = node.confidence
            
            # Y = index (à améliorer avec clustering)
            y = i / len(futures) if futures else 0.0
            
            node.position = (x, y)
    
    def _eliminate_dead_ends(self, dead_end_ids: List[int]) -> int:
        """
        Élimine les trajectoires mortes de la carte
        
        Returns:
            Nombre de trajectoires éliminées
        """
        eliminated = 0
        
        for node_id in dead_end_ids:
            if node_id in self.nodes:
                self.dead_end_zone.add(node_id)
                eliminated += 1
                self.early_eliminations += 1
        
        return eliminated
    
    def _focus_promising_zones(self, futures: List[FutureNode]) -> List[FutureNode]:
        """
        Focus sur les zones prometteuses (zone verte)
        
        Returns:
            Liste de futurs prometteurs
        """
        promising = []
        
        for node in futures:
            if node.status == FutureStatus.PROMISING:
                self.promising_zone.add(node.node_id)
                promising.append(node)
            elif node.status == FutureStatus.UNCERTAIN:
                self.uncertain_zone.add(node.node_id)
        
        # Trier par confiance
        promising.sort(key=lambda n: n.confidence, reverse=True)
        
        # Garder seulement beam_width meilleurs
        return promising[:self.beam_width]
    
    def _select_optimal_strategy(self, promising: List[FutureNode]) -> Optional[FutureNode]:
        """
        Sélectionne la stratégie optimale parmi les futurs prometteurs
        
        Returns:
            Meilleur FutureNode ou None
        """
        if not promising:
            return None
        
        # Sélectionner celui avec plus haute confiance
        best = max(promising, key=lambda n: n.confidence)
        
        return best
    
    def visualize_map(self) -> str:
        """
        Génère visualisation ASCII de la carte mentale
        
        Returns:
            Représentation ASCII
        """
        lines = []
        lines.append("=" * 60)
        lines.append("COGNITIVE STRATEGY MAP")
        lines.append("=" * 60)
        lines.append(f"Total futures: {len(self.nodes)}")
        lines.append(f"Promising (🟢): {len(self.promising_zone)}")
        lines.append(f"Uncertain (🟡): {len(self.uncertain_zone)}")
        lines.append(f"Dead ends (🔴): {len(self.dead_end_zone)}")
        lines.append(f"Early eliminations: {self.early_eliminations}")
        lines.append("=" * 60)
        
        # Afficher top 5 prometteurs
        promising_nodes = [self.nodes[nid] for nid in self.promising_zone]
        promising_nodes.sort(key=lambda n: n.confidence, reverse=True)
        
        lines.append("\nTop 5 Promising Futures:")
        for i, node in enumerate(promising_nodes[:5], 1):
            lines.append(f"  {i}. Node {node.node_id}: confidence={node.confidence:.2f}, accuracy={node.predicted_accuracy:.2f}")
        
        return "\n".join(lines)

# Made with Bob
