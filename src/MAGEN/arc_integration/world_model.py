"""
World Model - Cartographie Complète du Monde (Phase 4.5.1)

Objectif: Comprendre le niveau comme un SYSTÈME UNIQUE, pas une collection d'entités isolées.

Approche:
- Observer TOUT simultanément (pas timer seul, puis porte seule, etc.)
- Construire graphe spatial complet
- Détecter invariants et patterns temporels
- Catégoriser entités automatiquement

Auteur: Bob (MAGEN Project)
Date: 2026-06-20
"""

import numpy as np
import networkx as nx
from collections import defaultdict
from typing import Dict, List, Tuple, Set, Optional
from dataclasses import dataclass, field
from scipy.ndimage import label


@dataclass
class Entity:
    """Représentation d'une entité dans le monde"""
    id: str
    value: int
    pixels: List[Tuple[int, int]]
    centroid: Tuple[float, float]
    size: int
    category: str = "unknown"
    is_mobile: bool = False
    position_history: List[Tuple[float, float]] = field(default_factory=list)
    
    def __post_init__(self):
        self.position_history.append(self.centroid)
    
    def update_position(self, new_centroid: Tuple[float, float]):
        """Mettre à jour position et historique"""
        self.centroid = new_centroid
        self.position_history.append(new_centroid)
        
        # Détecter mobilité
        if len(self.position_history) > 1:
            last_pos = self.position_history[-2]
            distance = np.sqrt((new_centroid[0] - last_pos[0])**2 + 
                             (new_centroid[1] - last_pos[1])**2)
            if distance > 0.5:
                self.is_mobile = True


class WorldModel:
    """
    Modèle mental global du monde
    
    Construit une représentation complète du niveau:
    - Toutes les entités
    - Relations spatiales (graphe)
    - Patterns temporels (cycles, invariants)
    - Catégorisation automatique
    """
    
    def __init__(self):
        # Entités du monde
        self.entities: Dict[str, Entity] = {}
        self.entities_by_value: Dict[int, List[Entity]] = defaultdict(list)
        
        # Graphe spatial
        self.spatial_graph = nx.Graph()
        
        # Patterns temporels
        self.temporal_patterns: List[Dict] = []
        self.invariants: List[Dict] = []
        self.cycles: List[Dict] = []
        
        # Historique observations
        self.observation_history: List[np.ndarray] = []
        self.step_count = 0
        
        # Statistiques
        self.entity_stats: Dict[str, Dict] = defaultdict(lambda: {
            'size_history': [],
            'position_history': [],
            'first_seen': None,
            'last_seen': None,
            'stable': True
        })
    
    def observe_complete_state(self, grid: np.ndarray) -> Dict[str, Entity]:
        """
        Observer système complet en une fois
        
        Pas: observer timer, puis porte, puis avatar
        Mais: observer système complet simultanément
        
        Args:
            grid: Grille d'observation (H x W)
            
        Returns:
            Dict des entités détectées
        """
        self.step_count += 1
        self.observation_history.append(grid.copy())
        
        # Extraire toutes les entités (composantes connexes)
        entities = self.extract_connected_components(grid)
        
        # Mettre à jour entités existantes ou créer nouvelles
        self.update_entities(entities)
        
        # Construire graphe spatial
        self.build_spatial_graph()
        
        # Détecter invariants
        self.detect_invariants()
        
        # Détecter patterns temporels
        if len(self.observation_history) > 5:
            self.detect_temporal_patterns()
        
        return self.entities
    
    def extract_connected_components(self, grid: np.ndarray) -> List[Entity]:
        """
        Extraire composantes connexes (entités)
        
        Utilise scipy.ndimage.label pour détecter toutes les entités
        """
        entities = []
        
        # Pour chaque valeur unique (sauf 0 = background)
        unique_values = np.unique(grid)
        unique_values = unique_values[unique_values != 0]
        
        for value in unique_values:
            # Masque pour cette valeur
            mask = (grid == value)
            
            # Composantes connexes
            labeled, num_components = label(mask)
            
            # Créer entité pour chaque composante
            for comp_id in range(1, num_components + 1):
                comp_mask = (labeled == comp_id)
                pixels = list(zip(*np.where(comp_mask)))
                
                if len(pixels) > 0:
                    # Calculer centroïde
                    centroid = (
                        float(np.mean([p[0] for p in pixels])),
                        float(np.mean([p[1] for p in pixels]))
                    )
                    
                    # Créer entité
                    entity_id = f"{value}#{comp_id}"
                    entity = Entity(
                        id=entity_id,
                        value=int(value),
                        pixels=pixels,
                        centroid=centroid,
                        size=len(pixels)
                    )
                    
                    entities.append(entity)
        
        return entities
    
    def update_entities(self, new_entities: List[Entity]):
        """
        Mettre à jour entités existantes ou créer nouvelles
        
        Utilise matching spatial pour identifier entités entre frames
        """
        # Créer mapping par valeur pour matching rapide
        new_by_value = defaultdict(list)
        for entity in new_entities:
            new_by_value[entity.value].append(entity)
        
        # Matcher avec entités existantes
        matched_ids = set()
        
        for value, new_ents in new_by_value.items():
            old_ents = self.entities_by_value.get(value, [])
            
            # Matcher par proximité spatiale
            for new_ent in new_ents:
                best_match = None
                best_distance = float('inf')
                
                for old_ent in old_ents:
                    if old_ent.id in matched_ids:
                        continue
                    
                    distance = np.sqrt(
                        (new_ent.centroid[0] - old_ent.centroid[0])**2 +
                        (new_ent.centroid[1] - old_ent.centroid[1])**2
                    )
                    
                    if distance < best_distance and distance < 10.0:
                        best_match = old_ent
                        best_distance = distance
                
                if best_match:
                    # Mettre à jour entité existante
                    best_match.update_position(new_ent.centroid)
                    best_match.pixels = new_ent.pixels
                    best_match.size = new_ent.size
                    matched_ids.add(best_match.id)
                    
                    # Mettre à jour stats
                    self.entity_stats[best_match.id]['size_history'].append(new_ent.size)
                    self.entity_stats[best_match.id]['position_history'].append(new_ent.centroid)
                    self.entity_stats[best_match.id]['last_seen'] = self.step_count
                else:
                    # Nouvelle entité
                    self.entities[new_ent.id] = new_ent
                    self.entities_by_value[value].append(new_ent)
                    
                    # Initialiser stats
                    self.entity_stats[new_ent.id]['first_seen'] = self.step_count
                    self.entity_stats[new_ent.id]['last_seen'] = self.step_count
                    self.entity_stats[new_ent.id]['size_history'].append(new_ent.size)
                    self.entity_stats[new_ent.id]['position_history'].append(new_ent.centroid)
        
        # Catégoriser toutes les entités
        for entity in self.entities.values():
            if entity.category == "unknown":
                entity.category = self.categorize_entity(entity)
    
    def categorize_entity(self, entity: Entity) -> str:
        """
        Catégoriser entité automatiquement
        
        Heuristiques basées sur:
        - Taille (pixels)
        - Position
        - Mobilité observée
        - Valeur
        - Historique
        """
        stats = self.entity_stats[entity.id]
        
        # Très grande entité statique = mur/porte
        if entity.size > 500 and not entity.is_mobile:
            return "wall_or_door"
        
        # Petite entité statique = goal/marker
        if entity.size < 20 and not entity.is_mobile:
            return "goal_or_marker"
        
        # Entité mobile = avatar/bloc/ennemi
        if entity.is_mobile:
            if entity.size < 50:
                return "avatar_candidate"
            else:
                return "mobile_block"
        
        # Taille variable = timer/compteur
        if len(stats['size_history']) > 2:
            size_variance = np.var(stats['size_history'])
            if size_variance > 10:
                return "timer_or_counter"
        
        return "unknown"
    
    def build_spatial_graph(self):
        """
        Construire graphe spatial des entités
        
        Arêtes = proximité spatiale ou relations causales
        """
        self.spatial_graph.clear()
        
        # Ajouter tous les nœuds
        for entity_id, entity in self.entities.items():
            self.spatial_graph.add_node(
                entity_id,
                value=entity.value,
                category=entity.category,
                position=entity.centroid,
                size=entity.size
            )
        
        # Ajouter arêtes basées sur proximité
        entities_list = list(self.entities.values())
        for i, entity1 in enumerate(entities_list):
            for entity2 in entities_list[i+1:]:
                distance = np.sqrt(
                    (entity1.centroid[0] - entity2.centroid[0])**2 +
                    (entity1.centroid[1] - entity2.centroid[1])**2
                )
                
                # Arête si distance < seuil
                if distance < 20.0:
                    self.spatial_graph.add_edge(
                        entity1.id,
                        entity2.id,
                        distance=distance,
                        relation="proximity"
                    )
    
    def detect_invariants(self):
        """
        Détecter invariants du monde
        
        Invariants = propriétés qui ne changent jamais:
        - Entités statiques (position fixe)
        - Tailles constantes
        - Relations spatiales fixes
        """
        for entity_id, entity in self.entities.items():
            stats = self.entity_stats[entity_id]
            
            # Vérifier stabilité position
            if len(stats['position_history']) > 5:
                positions = np.array(stats['position_history'])
                position_variance = np.var(positions, axis=0).sum()
                
                if position_variance < 0.1:
                    # Position stable = invariant
                    invariant = {
                        'type': 'stable_position',
                        'entity_id': entity_id,
                        'position': entity.centroid,
                        'confidence': 1.0 - position_variance
                    }
                    
                    # Ajouter si pas déjà présent
                    if not any(inv['entity_id'] == entity_id and inv['type'] == 'stable_position' 
                              for inv in self.invariants):
                        self.invariants.append(invariant)
            
            # Vérifier stabilité taille
            if len(stats['size_history']) > 5:
                size_variance = np.var(stats['size_history'])
                
                if size_variance < 1.0:
                    # Taille stable = invariant
                    invariant = {
                        'type': 'stable_size',
                        'entity_id': entity_id,
                        'size': entity.size,
                        'confidence': 1.0 / (1.0 + size_variance)
                    }
                    
                    if not any(inv['entity_id'] == entity_id and inv['type'] == 'stable_size' 
                              for inv in self.invariants):
                        self.invariants.append(invariant)
    
    def detect_temporal_patterns(self):
        """
        Détecter patterns temporels (cycles, périodicités)
        
        Analyse historique observations pour trouver:
        - Cycles (événements répétitifs)
        - Périodicités (patterns réguliers)
        - Transitions d'états
        """
        if len(self.observation_history) < 10:
            return
        
        # Analyser changements entre frames
        changes = []
        for i in range(1, len(self.observation_history)):
            prev_grid = self.observation_history[i-1]
            curr_grid = self.observation_history[i]
            
            # Compter pixels modifiés
            diff = np.sum(prev_grid != curr_grid)
            changes.append(diff)
        
        # Détecter pics (événements majeurs)
        if len(changes) > 5:
            mean_change = np.mean(changes)
            std_change = np.std(changes)
            
            for i, change in enumerate(changes):
                if change > mean_change + 2 * std_change:
                    # Événement majeur détecté
                    pattern = {
                        'type': 'major_event',
                        'step': i + 1,
                        'magnitude': change,
                        'threshold': mean_change + 2 * std_change
                    }
                    
                    if not any(p['step'] == pattern['step'] for p in self.temporal_patterns):
                        self.temporal_patterns.append(pattern)
        
        # Détecter cycles (événements répétitifs)
        if len(self.temporal_patterns) > 2:
            events = sorted([p['step'] for p in self.temporal_patterns 
                           if p['type'] == 'major_event'])
            
            if len(events) >= 2:
                # Calculer intervalles
                intervals = [events[i+1] - events[i] for i in range(len(events)-1)]
                
                if len(intervals) > 1:
                    mean_interval = np.mean(intervals)
                    std_interval = np.std(intervals)
                    
                    # Si intervalles réguliers = cycle
                    if std_interval < mean_interval * 0.1:
                        cycle = {
                            'type': 'cycle',
                            'period': int(mean_interval),
                            'confidence': 1.0 - (std_interval / mean_interval),
                            'events': events
                        }
                        
                        if not any(c['type'] == 'cycle' for c in self.cycles):
                            self.cycles.append(cycle)
    
    def get_summary(self) -> Dict:
        """
        Obtenir résumé du modèle monde
        
        Returns:
            Dict avec statistiques et insights
        """
        return {
            'step_count': self.step_count,
            'total_entities': len(self.entities),
            'entities_by_category': {
                category: len([e for e in self.entities.values() if e.category == category])
                for category in set(e.category for e in self.entities.values())
            },
            'mobile_entities': len([e for e in self.entities.values() if e.is_mobile]),
            'invariants_detected': len(self.invariants),
            'temporal_patterns': len(self.temporal_patterns),
            'cycles_detected': len(self.cycles),
            'spatial_graph_nodes': self.spatial_graph.number_of_nodes(),
            'spatial_graph_edges': self.spatial_graph.number_of_edges()
        }
    
    def __repr__(self):
        summary = self.get_summary()
        return (f"WorldModel(steps={summary['step_count']}, "
                f"entities={summary['total_entities']}, "
                f"mobile={summary['mobile_entities']}, "
                f"invariants={summary['invariants_detected']}, "
                f"cycles={summary['cycles_detected']})")

# Made with Bob
