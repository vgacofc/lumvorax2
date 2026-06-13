"""
MAGEN - Dynamic Identity Tracker
=================================

Module révolutionnaire basé sur l'analyse cognitive de l'utilisateur:
"Les entités changent de forme, couleur, rôle, comportement, position
mais gardent une identité logique"

Inspiré du modèle des couleurs primaires:
- Rouge, Bleu, Jaune = invariants fondamentaux
- Violet, Orange, Vert = compositions stables
- Noir/Blanc = modulations d'état
- Nuances = variations continues

Architecture: Continuité Compositionnelle Dynamique
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set, Any
from dataclasses import dataclass, field
from collections import defaultdict
import hashlib


@dataclass
class EntityIdentity:
    """
    Représentation d'une entité avec continuité logique
    malgré mutations visuelles
    """
    # Identifiant unique persistant
    entity_id: str
    
    # Traits fondamentaux (invariants déterministes)
    core_traits: Dict[str, Any] = field(default_factory=dict)
    
    # Traits mutables (peuvent changer)
    mutable_traits: Dict[str, Any] = field(default_factory=dict)
    
    # Traits hérités (conservés lors transformations)
    inherited_traits: Dict[str, Any] = field(default_factory=dict)
    
    # Historique transformations
    transformation_history: List[Dict] = field(default_factory=list)
    
    # Chaîne causale
    causal_chain: List[str] = field(default_factory=list)
    
    # États futurs possibles
    possible_future_states: List[Dict] = field(default_factory=list)
    
    # Famille conceptuelle
    family: str = "unknown"
    
    # Distance cognitive depuis état initial
    cognitive_distance: float = 0.0
    
    # Intensité dernière transformation
    last_transformation_magnitude: float = 0.0


@dataclass
class ColorComposition:
    """
    Modèle compositionnel des couleurs
    Inspiré: Rouge+Bleu=Violet, +Noir=Assombri, +Blanc=Éclairci
    """
    # Couleurs primaires (déterministes)
    primaries: Dict[int, str] = field(default_factory=lambda: {
        1: "red", 2: "blue", 3: "yellow"
    })
    
    # Compositions secondaires stables
    compositions: Dict[Tuple[int, ...], str] = field(default_factory=lambda: {
        (1, 2): "violet",
        (1, 3): "orange", 
        (2, 3): "green"
    })
    
    # Modulateurs d'état
    modulators: Dict[int, str] = field(default_factory=lambda: {
        0: "black",  # Assombrit
        9: "white"   # Éclaircit
    })


class DynamicIdentityTracker:
    """
    Suit les entités malgré mutations visuelles
    Maintient continuité logique compositionnelle
    """
    
    def __init__(self):
        self.entities: Dict[str, EntityIdentity] = {}
        self.color_model = ColorComposition()
        self.transformation_graph: Dict[str, List[Tuple[str, float]]] = defaultdict(list)
        self.family_hierarchies: Dict[str, Set[str]] = defaultdict(set)
        
    def extract_entities(self, grid: np.ndarray) -> List[EntityIdentity]:
        """
        Extrait entités avec traits fondamentaux et mutables
        """
        entities = []
        
        # Détection connected components
        from scipy.ndimage import label
        labeled, num_features = label(grid > 0)
        
        for entity_idx in range(1, num_features + 1):
            mask = labeled == entity_idx
            entity_pixels = grid[mask]
            positions = np.argwhere(mask)
            
            # Traits fondamentaux (invariants)
            core_traits = {
                "topology": self._compute_topology(mask),
                "connectivity": self._compute_connectivity(mask),
                "role": self._infer_role(entity_pixels, positions)
            }
            
            # Traits mutables
            mutable_traits = {
                "colors": set(entity_pixels.flatten()),
                "shape": mask.shape,
                "size": len(positions),
                "position": tuple(positions.mean(axis=0).astype(int)),
                "bounding_box": self._compute_bbox(positions)
            }
            
            # Générer ID basé sur traits fondamentaux
            entity_id = self._generate_entity_id(core_traits)
            
            # Famille conceptuelle
            family = self._classify_family(core_traits, mutable_traits)
            
            entity = EntityIdentity(
                entity_id=entity_id,
                core_traits=core_traits,
                mutable_traits=mutable_traits,
                family=family
            )
            
            entities.append(entity)
            self.entities[entity_id] = entity
            self.family_hierarchies[family].add(entity_id)
            
        return entities
    
    def track_transformation(
        self,
        entity_before: EntityIdentity,
        entity_after: EntityIdentity
    ) -> Dict:
        """
        Analyse transformation entre deux états
        Détermine: continuité, magnitude, réversibilité
        """
        # Distance cognitive
        cognitive_distance = self._compute_cognitive_distance(
            entity_before, entity_after
        )
        
        # Magnitude transformation
        magnitude = self._compute_transformation_magnitude(
            entity_before, entity_after
        )
        
        # Traits conservés (héritage)
        inherited = self._identify_inherited_traits(
            entity_before, entity_after
        )
        
        # Réversibilité
        reversible = self._is_reversible_transformation(
            entity_before, entity_after
        )
        
        # Type transformation
        transform_type = self._classify_transformation_type(
            entity_before, entity_after
        )
        
        transformation = {
            "from_id": entity_before.entity_id,
            "to_id": entity_after.entity_id,
            "cognitive_distance": cognitive_distance,
            "magnitude": magnitude,
            "inherited_traits": inherited,
            "reversible": reversible,
            "type": transform_type,
            "continuity_preserved": cognitive_distance < 0.5
        }
        
        # Mettre à jour graphe transformations
        self.transformation_graph[entity_before.entity_id].append(
            (entity_after.entity_id, magnitude)
        )
        
        # Mettre à jour entité
        entity_after.transformation_history.append(transformation)
        entity_after.inherited_traits.update(inherited)
        entity_after.cognitive_distance = cognitive_distance
        entity_after.last_transformation_magnitude = magnitude
        
        return transformation
    
    def predict_future_states(
        self,
        entity: EntityIdentity,
        context: Dict
    ) -> List[EntityIdentity]:
        """
        Prédit états futurs possibles basés sur:
        - Historique transformations
        - Graphe transformations
        - Contexte actuel
        """
        future_states = []
        
        # États accessibles depuis graphe
        if entity.entity_id in self.transformation_graph:
            for next_id, prob in self.transformation_graph[entity.entity_id]:
                if next_id in self.entities:
                    future_states.append(self.entities[next_id])
        
        # États latents (non encore observés)
        latent_states = self._predict_latent_states(entity, context)
        future_states.extend(latent_states)
        
        return future_states
    
    def compute_identity_distance(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> float:
        """
        Distance cognitive entre deux entités
        Proche = même famille logique
        Éloigné = familles différentes
        """
        # Distance traits fondamentaux (poids fort)
        core_distance = self._trait_distance(
            entity1.core_traits, entity2.core_traits
        ) * 2.0
        
        # Distance traits mutables (poids faible)
        mutable_distance = self._trait_distance(
            entity1.mutable_traits, entity2.mutable_traits
        ) * 0.5
        
        # Bonus si même famille
        family_bonus = 0.0 if entity1.family == entity2.family else 0.3
        
        return core_distance + mutable_distance + family_bonus
    
    def analyze_color_composition(self, colors: Set[int]) -> Dict:
        """
        Analyse compositionnelle des couleurs
        Identifie: primaires, compositions, modulations
        """
        primaries = colors & set(self.color_model.primaries.keys())
        modulators = colors & set(self.color_model.modulators.keys())
        
        # Détecter compositions
        compositions_found = []
        for comp_colors, comp_name in self.color_model.compositions.items():
            if set(comp_colors).issubset(colors):
                compositions_found.append(comp_name)
        
        # État modulation
        modulation_state = "neutral"
        if 0 in modulators:
            modulation_state = "darkened"
        elif 9 in modulators:
            modulation_state = "lightened"
        
        return {
            "primaries": list(primaries),
            "compositions": compositions_found,
            "modulators": list(modulators),
            "modulation_state": modulation_state,
            "is_pure": len(primaries) == 1 and len(modulators) == 0,
            "is_composed": len(compositions_found) > 0
        }
    
    # === Méthodes Internes ===
    
    def _compute_topology(self, mask: np.ndarray) -> str:
        """Topologie: compact, elongated, fragmented"""
        from scipy.ndimage import binary_fill_holes
        filled = binary_fill_holes(mask)
        fill_ratio = mask.sum() / (filled.sum() + 1e-6)
        
        if fill_ratio > 0.9:
            return "compact"
        elif fill_ratio > 0.5:
            return "elongated"
        else:
            return "fragmented"
    
    def _compute_connectivity(self, mask: np.ndarray) -> int:
        """Nombre composantes connexes"""
        from scipy.ndimage import label
        _, num = label(mask)
        return num
    
    def _infer_role(self, pixels: np.ndarray, positions: np.ndarray) -> str:
        """Rôle tactique inféré"""
        # Heuristiques simples
        if len(positions) < 5:
            return "small_entity"
        elif len(positions) > 50:
            return "large_entity"
        else:
            return "medium_entity"
    
    def _compute_bbox(self, positions: np.ndarray) -> Tuple:
        """Bounding box"""
        min_y, min_x = positions.min(axis=0)
        max_y, max_x = positions.max(axis=0)
        return (min_y, min_x, max_y, max_x)
    
    def _generate_entity_id(self, core_traits: Dict) -> str:
        """ID unique basé sur traits fondamentaux"""
        trait_str = str(sorted(core_traits.items()))
        return hashlib.md5(trait_str.encode()).hexdigest()[:8]
    
    def _classify_family(self, core_traits: Dict, mutable_traits: Dict) -> str:
        """Famille conceptuelle"""
        # Heuristiques
        if core_traits["topology"] == "compact":
            return "solid_entity"
        elif core_traits["topology"] == "elongated":
            return "linear_entity"
        else:
            return "complex_entity"
    
    def _compute_cognitive_distance(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> float:
        """Distance cognitive entre états"""
        return self.compute_identity_distance(entity1, entity2)
    
    def _compute_transformation_magnitude(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> float:
        """Intensité transformation (0=minime, 1=majeure)"""
        # Changements traits mutables
        changes = 0
        total = 0
        
        for key in entity1.mutable_traits:
            total += 1
            if key in entity2.mutable_traits:
                if entity1.mutable_traits[key] != entity2.mutable_traits[key]:
                    changes += 1
            else:
                changes += 1
        
        return changes / (total + 1e-6)
    
    def _identify_inherited_traits(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> Dict:
        """Traits conservés lors transformation"""
        inherited = {}
        
        for key, val in entity1.core_traits.items():
            if key in entity2.core_traits:
                if val == entity2.core_traits[key]:
                    inherited[key] = val
        
        return inherited
    
    def _is_reversible_transformation(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> bool:
        """Transformation réversible ou permanente"""
        # Heuristique: si traits fondamentaux identiques → réversible
        return entity1.core_traits == entity2.core_traits
    
    def _classify_transformation_type(
        self,
        entity1: EntityIdentity,
        entity2: EntityIdentity
    ) -> str:
        """Type: color_change, shape_change, state_change, etc."""
        if entity1.mutable_traits.get("colors") != entity2.mutable_traits.get("colors"):
            return "color_transformation"
        elif entity1.mutable_traits.get("shape") != entity2.mutable_traits.get("shape"):
            return "shape_transformation"
        else:
            return "state_transformation"
    
    def _predict_latent_states(
        self,
        entity: EntityIdentity,
        context: Dict
    ) -> List[EntityIdentity]:
        """États latents non encore observés"""
        # Placeholder: à implémenter avec modèle prédictif
        return []
    
    def _trait_distance(self, traits1: Dict, traits2: Dict) -> float:
        """Distance entre deux ensembles de traits"""
        keys = set(traits1.keys()) | set(traits2.keys())
        if not keys:
            return 0.0
        
        differences = sum(
            1 for k in keys
            if traits1.get(k) != traits2.get(k)
        )
        
        return differences / len(keys)


def test_dynamic_identity_tracker():
    """Test module avec exemple simple"""
    tracker = DynamicIdentityTracker()
    
    # Grille test: entité qui change
    grid1 = np.array([
        [0, 1, 1, 0],
        [0, 1, 1, 0],
        [0, 0, 0, 0]
    ])
    
    grid2 = np.array([
        [0, 2, 2, 0],
        [0, 2, 2, 2],
        [0, 0, 0, 0]
    ])
    
    # Extraction
    entities1 = tracker.extract_entities(grid1)
    entities2 = tracker.extract_entities(grid2)
    
    print(f"Entités grid1: {len(entities1)}")
    print(f"Entités grid2: {len(entities2)}")
    
    if entities1 and entities2:
        # Analyse transformation
        transform = tracker.track_transformation(entities1[0], entities2[0])
        print(f"\nTransformation:")
        print(f"  Distance cognitive: {transform['cognitive_distance']:.3f}")
        print(f"  Magnitude: {transform['magnitude']:.3f}")
        print(f"  Type: {transform['type']}")
        print(f"  Continuité préservée: {transform['continuity_preserved']}")
        
        # Analyse couleurs
        colors1 = entities1[0].mutable_traits["colors"]
        colors2 = entities2[0].mutable_traits["colors"]
        
        comp1 = tracker.analyze_color_composition(colors1)
        comp2 = tracker.analyze_color_composition(colors2)
        
        print(f"\nComposition couleurs grid1: {comp1}")
        print(f"Composition couleurs grid2: {comp2}")


if __name__ == "__main__":
    test_dynamic_identity_tracker()

# Made with Bob
