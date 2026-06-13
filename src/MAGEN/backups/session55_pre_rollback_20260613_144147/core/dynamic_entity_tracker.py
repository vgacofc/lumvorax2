#!/usr/bin/env python3
"""
Dynamic Entity Tracker (DET) - Identité Persistante des Entités
================================================================

MODULE CRITIQUE manquant identifié Session 54.

Problème actuel:
    case A rouge → case B bleu → système voit "objets différents"
    
Solution DET:
    case A rouge → case B bleu → MÊME entité, état différent

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
Protocole: Claude Pilot + LumVorax
Session: 54 - Architecture révolutionnaire
"""

import numpy as np
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass, field
from enum import Enum
import hashlib


class EntityRole(Enum):
    """Rôles possibles d'une entité"""
    PLAYER = "player"           # Entité contrôlée/principale
    ENEMY = "enemy"             # Entité hostile/opposée
    OBJECT = "object"           # Objet manipulable
    DECORATION = "decoration"   # Élément de décor
    UI_ELEMENT = "ui"          # Élément d'interface
    LATENT = "latent"          # Entité dormante/cachée
    ACTIVE = "active"          # Entité activée
    TRANSFORMED = "transformed" # Entité en transformation


class EntityState(Enum):
    """États possibles d'une entité"""
    ALIVE = "alive"
    DEAD = "dead"
    HIDDEN = "hidden"
    VISIBLE = "visible"
    ACTIVE = "active"
    DORMANT = "dormant"
    TRANSFORMING = "transforming"
    STABLE = "stable"


@dataclass
class EntitySnapshot:
    """Snapshot d'une entité à un instant t"""
    timestamp: int
    position: Tuple[int, int]
    shape: np.ndarray
    color: int
    bounding_box: Tuple[int, int, int, int]  # (row_min, row_max, col_min, col_max)
    state: EntityState
    role: EntityRole
    
    def compute_hash(self) -> str:
        """Hash unique du snapshot"""
        data = f"{self.position}_{self.shape.tobytes()}_{self.color}_{self.state.value}"
        return hashlib.md5(data.encode()).hexdigest()[:8]


@dataclass
class Entity:
    """
    Entité avec identité persistante
    
    CONCEPT CLÉ: Une entité RESTE LA MÊME malgré:
    - Changement de forme
    - Changement de couleur
    - Changement de position
    - Changement d'état
    - Changement de rôle
    """
    id: str
    role: EntityRole
    state: EntityState
    
    # Historique complet
    shape_history: List[np.ndarray] = field(default_factory=list)
    color_history: List[int] = field(default_factory=list)
    position_history: List[Tuple[int, int]] = field(default_factory=list)
    state_history: List[EntityState] = field(default_factory=list)
    role_history: List[EntityRole] = field(default_factory=list)
    
    # Snapshots temporels
    snapshots: List[EntitySnapshot] = field(default_factory=list)
    
    # Métadonnées
    birth_time: int = 0
    last_update: int = 0
    is_persistent: bool = True
    
    def add_snapshot(self, snapshot: EntitySnapshot):
        """Ajoute un snapshot à l'historique"""
        self.snapshots.append(snapshot)
        self.shape_history.append(snapshot.shape.copy())
        self.color_history.append(snapshot.color)
        self.position_history.append(snapshot.position)
        self.state_history.append(snapshot.state)
        self.role_history.append(snapshot.role)
        self.last_update = snapshot.timestamp
    
    def get_current_snapshot(self) -> Optional[EntitySnapshot]:
        """Retourne le snapshot le plus récent"""
        return self.snapshots[-1] if self.snapshots else None
    
    def has_transformed(self) -> bool:
        """Vérifie si l'entité a subi une transformation"""
        if len(self.shape_history) < 2:
            return False
        return not np.array_equal(self.shape_history[0], self.shape_history[-1])
    
    def has_moved(self) -> bool:
        """Vérifie si l'entité s'est déplacée"""
        if len(self.position_history) < 2:
            return False
        return self.position_history[0] != self.position_history[-1]
    
    def has_changed_color(self) -> bool:
        """Vérifie si l'entité a changé de couleur"""
        if len(self.color_history) < 2:
            return False
        return self.color_history[0] != self.color_history[-1]
    
    def get_transformation_pattern(self) -> str:
        """Identifie le pattern de transformation"""
        patterns = []
        if self.has_moved():
            patterns.append("MOVE")
        if self.has_transformed():
            patterns.append("SHAPE")
        if self.has_changed_color():
            patterns.append("COLOR")
        return "+".join(patterns) if patterns else "STABLE"


class DynamicEntityTracker:
    """
    Tracker d'entités avec identité persistante
    
    RÉVOLUTION CONCEPTUELLE:
    Au lieu de voir des grilles statiques, on track des ENTITÉS
    qui évoluent dans le temps tout en gardant leur identité.
    
    Exemple:
        Frame 0: Carré rouge en (0,0)
        Frame 1: Cercle bleu en (2,2)
        
        Système actuel: 2 objets différents
        DET: MÊME entité, transformée
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.entities: Dict[str, Entity] = {}
        self.current_time = 0
        self.entity_counter = 0
        
        # Graphe de relations entre entités
        self.entity_graph: Dict[str, Set[str]] = {}
        
        if self.verbose:
            print("[DET] Dynamic Entity Tracker initialisé")
    
    def _generate_entity_id(self) -> str:
        """Génère un ID unique pour une entité"""
        self.entity_counter += 1
        return f"entity_{self.entity_counter:04d}"
    
    def extract_entities_from_grid(self, grid: np.ndarray, timestamp: int = None) -> List[Entity]:
        """
        Extrait les entités d'une grille
        
        IMPORTANT: Utilise connected components pour identifier objets
        """
        if timestamp is None:
            timestamp = self.current_time
            self.current_time += 1
        
        entities = []
        
        # Identifier les composantes connexes (objets distincts)
        from scipy import ndimage
        
        # Pour chaque couleur non-nulle
        unique_colors = np.unique(grid)
        unique_colors = unique_colors[unique_colors != 0]
        
        for color in unique_colors:
            # Masque pour cette couleur
            mask = (grid == color)
            
            # Trouver composantes connexes
            labeled, num_features = ndimage.label(mask)
            
            for label_id in range(1, num_features + 1):
                # Extraire cette composante
                component_mask = (labeled == label_id)
                rows, cols = np.where(component_mask)
                
                if len(rows) == 0:
                    continue
                
                # Bounding box
                row_min, row_max = rows.min(), rows.max()
                col_min, col_max = cols.min(), cols.max()
                
                # Extraire shape
                shape = grid[row_min:row_max+1, col_min:col_max+1].copy()
                shape = np.where(component_mask[row_min:row_max+1, col_min:col_max+1], color, 0)
                
                # Position (centre de masse)
                position = (int(rows.mean()), int(cols.mean()))
                
                # Créer snapshot
                snapshot = EntitySnapshot(
                    timestamp=timestamp,
                    position=position,
                    shape=shape,
                    color=int(color),
                    bounding_box=(row_min, row_max, col_min, col_max),
                    state=EntityState.VISIBLE,
                    role=EntityRole.OBJECT  # Par défaut
                )
                
                # Créer ou retrouver entité
                entity_id = self._match_or_create_entity(snapshot)
                entity = self.entities[entity_id]
                entity.add_snapshot(snapshot)
                entities.append(entity)
        
        return entities
    
    def _match_or_create_entity(self, snapshot: EntitySnapshot) -> str:
        """
        Match un snapshot avec une entité existante ou en crée une nouvelle
        
        LOGIQUE CLÉ: Identité persistante basée sur:
        1. Proximité spatiale
        2. Similarité de forme
        3. Continuité temporelle
        """
        best_match_id = None
        best_match_score = 0.0
        
        for entity_id, entity in self.entities.items():
            current = entity.get_current_snapshot()
            if current is None:
                continue
            
            # Score de matching
            score = self._compute_match_score(current, snapshot)
            
            if score > best_match_score and score > 0.5:  # Seuil
                best_match_score = score
                best_match_id = entity_id
        
        if best_match_id:
            return best_match_id
        
        # Créer nouvelle entité
        new_id = self._generate_entity_id()
        new_entity = Entity(
            id=new_id,
            role=snapshot.role,
            state=snapshot.state,
            birth_time=snapshot.timestamp
        )
        self.entities[new_id] = new_entity
        
        if self.verbose:
            print(f"[DET] Nouvelle entité créée: {new_id}")
        
        return new_id
    
    def _compute_match_score(self, snap1: EntitySnapshot, snap2: EntitySnapshot) -> float:
        """
        Calcule score de matching entre 2 snapshots
        
        Critères:
        - Distance spatiale (plus proche = meilleur)
        - Similarité de forme (IoU)
        - Continuité couleur
        """
        # Distance spatiale (normalisée)
        dist = np.sqrt((snap1.position[0] - snap2.position[0])**2 + 
                      (snap1.position[1] - snap2.position[1])**2)
        spatial_score = 1.0 / (1.0 + dist)
        
        # Similarité de forme (IoU approximatif)
        size1 = snap1.shape.size
        size2 = snap2.shape.size
        size_score = min(size1, size2) / max(size1, size2) if max(size1, size2) > 0 else 0
        
        # Continuité couleur
        color_score = 1.0 if snap1.color == snap2.color else 0.3
        
        # Score combiné
        score = 0.4 * spatial_score + 0.4 * size_score + 0.2 * color_score
        
        return score
    
    def track_sequence(self, grids: List[np.ndarray]) -> Dict[str, Entity]:
        """
        Track une séquence de grilles (train pairs)
        
        Retourne: Dict d'entités avec leur historique complet
        """
        for t, grid in enumerate(grids):
            self.extract_entities_from_grid(grid, timestamp=t)
        
        if self.verbose:
            print(f"[DET] Tracking terminé: {len(self.entities)} entités identifiées")
            for entity_id, entity in self.entities.items():
                pattern = entity.get_transformation_pattern()
                print(f"  {entity_id}: {pattern} ({len(entity.snapshots)} frames)")
        
        return self.entities
    
    def get_entity_graph(self) -> Dict[str, Set[str]]:
        """
        Construit graphe de relations entre entités
        
        Relations basées sur:
        - Proximité spatiale
        - Interactions temporelles
        - Transformations liées
        """
        graph = {}
        
        entity_ids = list(self.entities.keys())
        for i, id1 in enumerate(entity_ids):
            graph[id1] = set()
            for id2 in entity_ids[i+1:]:
                if self._are_related(id1, id2):
                    graph[id1].add(id2)
                    if id2 not in graph:
                        graph[id2] = set()
                    graph[id2].add(id1)
        
        return graph
    
    def _are_related(self, id1: str, id2: str) -> bool:
        """Vérifie si 2 entités sont reliées"""
        entity1 = self.entities[id1]
        entity2 = self.entities[id2]
        
        # Vérifier proximité spatiale dans au moins un frame
        for snap1 in entity1.snapshots:
            for snap2 in entity2.snapshots:
                if snap1.timestamp == snap2.timestamp:
                    dist = np.sqrt((snap1.position[0] - snap2.position[0])**2 + 
                                 (snap1.position[1] - snap2.position[1])**2)
                    if dist < 5:  # Seuil de proximité
                        return True
        
        return False
    
    def get_transformation_rules(self) -> List[Dict]:
        """
        Extrait les règles de transformation des entités
        
        CRUCIAL: Identifie les patterns de transformation
        qui sont STABLES à travers les train pairs
        """
        rules = []
        
        for entity_id, entity in self.entities.items():
            if len(entity.snapshots) < 2:
                continue
            
            rule = {
                'entity_id': entity_id,
                'pattern': entity.get_transformation_pattern(),
                'initial_state': entity.snapshots[0],
                'final_state': entity.snapshots[-1],
                'num_steps': len(entity.snapshots),
                'is_stable': not entity.has_transformed()
            }
            rules.append(rule)
        
        return rules


# Made with Bob - Session 54 Revolutionary Architecture