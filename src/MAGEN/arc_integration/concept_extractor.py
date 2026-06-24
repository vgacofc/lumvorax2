#!/usr/bin/env python3
"""
PHASE 4.6.1 - Concept Extractor
Extraction automatique de concepts réutilisables depuis les puzzles.

Vision utilisateur:
"MAGEN ne doit pas mémoriser seulement 'Dans LS20 faire: Droite Droite ACTION4'.
Il doit mémoriser: Concept : identifier l'avatar, Concept : détecter une porte,
Concept : détecter un timer, Concept : synchronisation temporelle..."

Concepts ARC:
- Symétrie (horizontale, verticale, diagonale)
- Rotation (90°, 180°, 270°)
- Répétition (patterns récurrents)
- Scaling (agrandissement/réduction)
- Translation (déplacement)
- Color mapping (transformation couleurs)

Concepts Arcade:
- Avatar (entité contrôlable)
- Portes (passages conditionnels)
- Timers (compteurs temporels)
- Ennemis (entités hostiles)
- Objets poussables
- Cycles temporels
- Synchronisation

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.6.1
"""

import sys
import os
import numpy as np
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, field
from datetime import datetime
from collections import Counter

# Ajouter path
sys.path.insert(0, os.path.dirname(__file__))

from shared_knowledge_base import SharedKnowledgeBase, Concept
from unified_puzzle_manager import PuzzleType


@dataclass
class DetectedConcept:
    """Concept détecté dans un puzzle"""
    name: str
    type: str  # "arc" ou "arcade"
    description: str
    confidence: float
    evidence: Dict
    puzzle_id: str
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())


class ConceptExtractor:
    """
    Extracteur automatique de concepts réutilisables.
    
    Détecte et extrait des concepts généraux depuis:
    - Puzzles ARC (transformations géométriques)
    - Jeux arcade (entités et mécaniques de jeu)
    
    Permet le transfert de connaissances entre puzzles.
    """
    
    def __init__(self, 
                 knowledge_base: SharedKnowledgeBase,
                 verbose: bool = True):
        self.knowledge = knowledge_base
        self.verbose = verbose
        
        # Concepts détectés
        self.detected_concepts: List[DetectedConcept] = []
        
        # Seuils de confiance
        self.confidence_threshold = 0.7
        
        if self.verbose:
            print("\n" + "="*70)
            print("🔍 CONCEPT EXTRACTOR - PHASE 4.6.1")
            print("="*70)
            print("🎯 Détection automatique de concepts réutilisables")
            print("📊 Types supportés: ARC + Arcade")
            print("="*70)
    
    # ========================================================================
    # DÉTECTEURS ARC (Transformations géométriques)
    # ========================================================================
    
    def detect_symmetry(self, grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte symétrie dans une grille ARC"""
        h, w = grid.shape
        
        # Symétrie horizontale
        horizontal_sym = np.array_equal(grid, np.flip(grid, axis=0))
        
        # Symétrie verticale
        vertical_sym = np.array_equal(grid, np.flip(grid, axis=1))
        
        # Symétrie diagonale (si carré)
        diagonal_sym = False
        if h == w:
            diagonal_sym = np.array_equal(grid, grid.T)
        
        if horizontal_sym or vertical_sym or diagonal_sym:
            sym_type = []
            if horizontal_sym: sym_type.append("horizontale")
            if vertical_sym: sym_type.append("verticale")
            if diagonal_sym: sym_type.append("diagonale")
            
            return DetectedConcept(
                name="symmetry",
                type="arc",
                description=f"Symétrie {', '.join(sym_type)}",
                confidence=0.95,
                evidence={
                    'horizontal': horizontal_sym,
                    'vertical': vertical_sym,
                    'diagonal': diagonal_sym,
                    'grid_shape': (h, w)
                },
                puzzle_id="unknown"
            )
        
        return None
    
    def detect_rotation(self, input_grid: np.ndarray, output_grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte rotation entre input et output"""
        # Tester rotations 90°, 180°, 270°
        for angle, k in [(90, 1), (180, 2), (270, 3)]:
            rotated = np.rot90(input_grid, k)
            if np.array_equal(rotated, output_grid):
                return DetectedConcept(
                    name="rotation",
                    type="arc",
                    description=f"Rotation {angle}°",
                    confidence=1.0,
                    evidence={
                        'angle': angle,
                        'k_rotations': k,
                        'input_shape': input_grid.shape,
                        'output_shape': output_grid.shape
                    },
                    puzzle_id="unknown"
                )
        
        return None
    
    def detect_scaling(self, input_grid: np.ndarray, output_grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte scaling (agrandissement/réduction)"""
        in_h, in_w = input_grid.shape
        out_h, out_w = output_grid.shape
        
        # Vérifier si scaling uniforme
        if out_h % in_h == 0 and out_w % in_w == 0:
            scale_h = out_h // in_h
            scale_w = out_w // in_w
            
            if scale_h == scale_w and scale_h > 1:
                # Vérifier si chaque cellule est répétée
                is_scaling = True
                for i in range(in_h):
                    for j in range(in_w):
                        block = output_grid[i*scale_h:(i+1)*scale_h, 
                                          j*scale_w:(j+1)*scale_w]
                        if not np.all(block == input_grid[i, j]):
                            is_scaling = False
                            break
                    if not is_scaling:
                        break
                
                if is_scaling:
                    return DetectedConcept(
                        name="scaling",
                        type="arc",
                        description=f"Agrandissement {scale_h}x",
                        confidence=0.95,
                        evidence={
                            'scale_factor': scale_h,
                            'input_shape': (in_h, in_w),
                            'output_shape': (out_h, out_w)
                        },
                        puzzle_id="unknown"
                    )
        
        return None
    
    def detect_repetition(self, grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte patterns répétitifs"""
        h, w = grid.shape
        
        # Chercher patterns 2x2 répétés
        patterns = []
        for i in range(h-1):
            for j in range(w-1):
                pattern = tuple(grid[i:i+2, j:j+2].flatten())
                patterns.append(pattern)
        
        # Compter occurrences
        pattern_counts = Counter(patterns)
        most_common = pattern_counts.most_common(1)
        
        if most_common and most_common[0][1] >= 3:  # Au moins 3 répétitions
            return DetectedConcept(
                name="repetition",
                type="arc",
                description=f"Pattern répété {most_common[0][1]} fois",
                confidence=0.85,
                evidence={
                    'pattern': most_common[0][0],
                    'count': most_common[0][1],
                    'grid_shape': (h, w)
                },
                puzzle_id="unknown"
            )
        
        return None
    
    def detect_color_mapping(self, input_grid: np.ndarray, output_grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte transformation de couleurs"""
        if input_grid.shape != output_grid.shape:
            return None
        
        # Construire mapping couleur
        color_map = {}
        for i in range(input_grid.shape[0]):
            for j in range(input_grid.shape[1]):
                in_color = input_grid[i, j]
                out_color = output_grid[i, j]
                
                if in_color in color_map:
                    if color_map[in_color] != out_color:
                        return None  # Mapping inconsistant
                else:
                    color_map[in_color] = out_color
        
        # Vérifier si mapping non-trivial
        if len(color_map) > 1 and any(k != v for k, v in color_map.items()):
            return DetectedConcept(
                name="color_mapping",
                type="arc",
                description=f"Transformation couleurs ({len(color_map)} mappings)",
                confidence=0.9,
                evidence={
                    'mapping': color_map,
                    'num_colors': len(color_map)
                },
                puzzle_id="unknown"
            )
        
        return None
    
    # ========================================================================
    # DÉTECTEURS ARCADE (Entités et mécaniques de jeu)
    # ========================================================================
    
    def detect_avatar(self, grid: np.ndarray, previous_grid: Optional[np.ndarray] = None) -> Optional[DetectedConcept]:
        """Détecte avatar (entité contrôlable qui se déplace)"""
        if previous_grid is None:
            return None
        
        # Trouver différences entre grilles
        diff = (grid != previous_grid)
        
        # Si exactement 2 cellules changent (départ + arrivée)
        changed_positions = np.argwhere(diff)
        if len(changed_positions) == 2:
            pos1, pos2 = changed_positions
            
            # Vérifier si déplacement adjacent (1 case)
            distance = abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1])
            if distance == 1:
                return DetectedConcept(
                    name="avatar",
                    type="arcade",
                    description="Entité contrôlable détectée (déplacement 1 case)",
                    confidence=0.85,
                    evidence={
                        'from_position': tuple(pos1),
                        'to_position': tuple(pos2),
                        'distance': distance
                    },
                    puzzle_id="unknown"
                )
        
        return None
    
    def detect_door(self, grid: np.ndarray) -> Optional[DetectedConcept]:
        """Détecte portes (passages conditionnels)"""
        # Chercher patterns de porte (ex: cellule spéciale entourée de murs)
        h, w = grid.shape
        door_candidates = []
        
        for i in range(1, h-1):
            for j in range(1, w-1):
                cell = grid[i, j]
                # Si cellule différente de 0 et entourée de murs (valeur élevée)
                neighbors = [
                    grid[i-1, j], grid[i+1, j],
                    grid[i, j-1], grid[i, j+1]
                ]
                if cell > 0 and all(n >= 5 for n in neighbors):
                    door_candidates.append((i, j, cell))
        
        if door_candidates:
            return DetectedConcept(
                name="door",
                type="arcade",
                description=f"{len(door_candidates)} porte(s) détectée(s)",
                confidence=0.75,
                evidence={
                    'positions': door_candidates,
                    'count': len(door_candidates)
                },
                puzzle_id="unknown"
            )
        
        return None
    
    def detect_timer(self, grid_sequence: List[np.ndarray]) -> Optional[DetectedConcept]:
        """Détecte timer (compteur qui décrémente)"""
        if len(grid_sequence) < 3:
            return None
        
        # Chercher cellule qui décrémente régulièrement
        h, w = grid_sequence[0].shape
        
        for i in range(h):
            for j in range(w):
                values = [grid[i, j] for grid in grid_sequence]
                
                # Vérifier décrémentation
                if all(values[k] > values[k+1] for k in range(len(values)-1)):
                    if values[0] - values[-1] == len(values) - 1:
                        return DetectedConcept(
                            name="timer",
                            type="arcade",
                            description=f"Timer détecté à ({i},{j})",
                            confidence=0.9,
                            evidence={
                                'position': (i, j),
                                'initial_value': values[0],
                                'final_value': values[-1],
                                'sequence': values
                            },
                            puzzle_id="unknown"
                        )
        
        return None
    
    def detect_enemy(self, grid: np.ndarray, previous_grid: Optional[np.ndarray] = None) -> Optional[DetectedConcept]:
        """Détecte ennemis (entités hostiles qui se déplacent)"""
        if previous_grid is None:
            return None
        
        # Trouver entités qui se déplacent (similaire à avatar mais pattern différent)
        diff = (grid != previous_grid)
        changed_positions = np.argwhere(diff)
        
        # Si plusieurs entités bougent = probablement ennemis
        if len(changed_positions) >= 4:  # Au moins 2 entités (2 positions chacune)
            return DetectedConcept(
                name="enemy",
                type="arcade",
                description=f"{len(changed_positions)//2} entité(s) mobile(s) détectée(s)",
                confidence=0.7,
                evidence={
                    'num_movements': len(changed_positions),
                    'positions': changed_positions.tolist()
                },
                puzzle_id="unknown"
            )
        
        return None
    
    def detect_pushable_object(self, grid_sequence: List[np.ndarray]) -> Optional[DetectedConcept]:
        """Détecte objets poussables"""
        if len(grid_sequence) < 2:
            return None
        
        # Chercher objet qui se déplace dans la direction de l'avatar
        # TODO: Implémenter logique complète
        
        return None
    
    # ========================================================================
    # EXTRACTION UNIFIÉE
    # ========================================================================
    
    def extract_from_arc_puzzle(self, 
                                 input_grid: np.ndarray, 
                                 output_grid: np.ndarray,
                                 puzzle_id: str) -> List[DetectedConcept]:
        """Extrait tous les concepts d'un puzzle ARC"""
        concepts = []
        
        # Tester tous les détecteurs ARC
        detectors = [
            lambda: self.detect_symmetry(input_grid),
            lambda: self.detect_symmetry(output_grid),
            lambda: self.detect_rotation(input_grid, output_grid),
            lambda: self.detect_scaling(input_grid, output_grid),
            lambda: self.detect_repetition(input_grid),
            lambda: self.detect_repetition(output_grid),
            lambda: self.detect_color_mapping(input_grid, output_grid)
        ]
        
        for detector in detectors:
            try:
                concept = detector()
                if concept and concept.confidence >= self.confidence_threshold:
                    concept.puzzle_id = puzzle_id
                    concepts.append(concept)
            except Exception as e:
                if self.verbose:
                    print(f"⚠️  Erreur détecteur: {e}")
        
        return concepts
    
    def extract_from_arcade_puzzle(self,
                                    grid_sequence: List[np.ndarray],
                                    puzzle_id: str) -> List[DetectedConcept]:
        """Extrait tous les concepts d'un jeu arcade"""
        concepts = []
        
        if len(grid_sequence) < 2:
            return concepts
        
        # Tester détecteurs arcade
        current_grid = grid_sequence[-1]
        previous_grid = grid_sequence[-2] if len(grid_sequence) > 1 else None
        
        detectors = [
            lambda: self.detect_avatar(current_grid, previous_grid),
            lambda: self.detect_door(current_grid),
            lambda: self.detect_timer(grid_sequence),
            lambda: self.detect_enemy(current_grid, previous_grid)
        ]
        
        for detector in detectors:
            try:
                concept = detector()
                if concept and concept.confidence >= self.confidence_threshold:
                    concept.puzzle_id = puzzle_id
                    concepts.append(concept)
            except Exception as e:
                if self.verbose:
                    print(f"⚠️  Erreur détecteur: {e}")
        
        return concepts
    
    def add_concepts_to_knowledge_base(self, concepts: List[DetectedConcept]):
        """Ajoute concepts détectés à la base de connaissances"""
        for detected in concepts:
            # Convertir en Concept pour SharedKnowledgeBase
            kb_concept = Concept(
                name=detected.name,
                description=detected.description,
                pattern=detected.evidence,
                confidence=detected.confidence,
                puzzles_learned_from=[detected.puzzle_id]
            )
            
            self.knowledge.add_concept(kb_concept)
            self.detected_concepts.append(detected)
            
            if self.verbose:
                print(f"✅ Concept ajouté: {detected.name} ({detected.confidence:.0%})")
    
    def get_statistics(self) -> Dict:
        """Retourne statistiques d'extraction"""
        arc_concepts = [c for c in self.detected_concepts if c.type == "arc"]
        arcade_concepts = [c for c in self.detected_concepts if c.type == "arcade"]
        
        return {
            'total_concepts': len(self.detected_concepts),
            'arc_concepts': len(arc_concepts),
            'arcade_concepts': len(arcade_concepts),
            'average_confidence': np.mean([c.confidence for c in self.detected_concepts]) if self.detected_concepts else 0.0,
            'concept_types': Counter([c.name for c in self.detected_concepts])
        }


def main():
    """Test du Concept Extractor"""
    print("\n" + "="*70)
    print("🧪 TEST CONCEPT EXTRACTOR - PHASE 4.6.1")
    print("="*70)
    
    # Créer base de connaissances
    knowledge = SharedKnowledgeBase("test_concepts.pkl")
    
    # Créer extractor
    extractor = ConceptExtractor(knowledge, verbose=True)
    
    # Test 1: Puzzle ARC avec rotation
    print("\n📊 TEST 1: Puzzle ARC (Rotation)")
    input_grid = np.array([[1, 2], [3, 4]])
    output_grid = np.rot90(input_grid)
    
    concepts = extractor.extract_from_arc_puzzle(input_grid, output_grid, "test_arc_1")
    extractor.add_concepts_to_knowledge_base(concepts)
    
    # Test 2: Puzzle ARC avec symétrie
    print("\n📊 TEST 2: Puzzle ARC (Symétrie)")
    symmetric_grid = np.array([[1, 2, 1], [3, 4, 3], [1, 2, 1]])
    
    concepts = extractor.extract_from_arc_puzzle(symmetric_grid, symmetric_grid, "test_arc_2")
    extractor.add_concepts_to_knowledge_base(concepts)
    
    # Test 3: Jeu arcade avec avatar
    print("\n📊 TEST 3: Jeu Arcade (Avatar)")
    grid1 = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]])
    grid2 = np.array([[0, 0, 0], [0, 0, 1], [0, 0, 0]])
    
    concepts = extractor.extract_from_arcade_puzzle([grid1, grid2], "test_arcade_1")
    extractor.add_concepts_to_knowledge_base(concepts)
    
    # Statistiques
    stats = extractor.get_statistics()
    print("\n📊 STATISTIQUES:")
    print(f"   Total concepts: {stats['total_concepts']}")
    print(f"   ARC: {stats['arc_concepts']}")
    print(f"   Arcade: {stats['arcade_concepts']}")
    print(f"   Confiance moyenne: {stats['average_confidence']:.0%}")
    print(f"   Types: {dict(stats['concept_types'])}")
    
    print("\n✅ Test terminé avec succès")


if __name__ == "__main__":
    main()

# Made with ❤️ by Bob

# Made with Bob
