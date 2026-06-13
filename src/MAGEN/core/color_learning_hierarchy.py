"""
MAGEN - Color Learning Hierarchy
=================================

Système d'apprentissage hiérarchique des couleurs basé sur l'analyse de l'utilisateur.

PRINCIPE FONDAMENTAL:
La couleur est la BASE ABSOLUE de la vision. Sans compréhension des couleurs,
un système ne peut pas réellement comprendre ce qu'il voit.

ARCHITECTURE HIÉRARCHIQUE:
1. Niveau 1 (DÉTERMINISTE): Couleurs primaires (Rouge, Bleu, Jaune)
2. Niveau 2 (DÉTERMINISTE): Blanc et Noir (luminosité, contraste)
3. Niveau 3 (PROBABILISTE): Mélanges et variations
4. Niveau 4 (PROBABILISTE): Nuances complexes
5. Niveau 5 (CONTEXTUEL): Millions de variations

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot
Basé sur: Analyse utilisateur sur apprentissage visuel biologique
"""

import numpy as np
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from enum import Enum


class ColorCategory(Enum):
    """Catégories de couleurs hiérarchiques"""
    # Niveau 1: Primaires (DÉTERMINISTE 100%)
    RED = "red"
    BLUE = "blue"
    YELLOW = "yellow"
    
    # Niveau 2: Modificateurs (DÉTERMINISTE 100%)
    WHITE = "white"
    BLACK = "black"
    
    # Niveau 3: Secondaires (mélange primaires)
    VIOLET = "violet"      # Rouge + Bleu
    GREEN = "green"        # Bleu + Jaune
    ORANGE = "orange"      # Rouge + Jaune
    
    # Niveau 4: Tertiaires et variations
    CYAN = "cyan"
    MAGENTA = "magenta"
    BROWN = "brown"
    GRAY = "gray"
    
    # Niveau 5: Contextuelles
    UNKNOWN = "unknown"


@dataclass
class ColorKnowledge:
    """Connaissance d'une couleur"""
    category: ColorCategory
    confidence: float  # 1.0 = déterministe, <1.0 = probabiliste
    level: int  # 1-5 (hiérarchie)
    rgb_range: Tuple[Tuple[int, int], Tuple[int, int], Tuple[int, int]]
    description: str
    is_deterministic: bool


class ColorLearningHierarchy:
    """
    Système d'apprentissage hiérarchique des couleurs
    
    PHILOSOPHIE:
    - Niveau 1-2: DÉTERMINISTE (100% certitude)
    - Niveau 3-5: PROBABILISTE (variations et contexte)
    
    La couleur donne:
    - Séparation des objets
    - Profondeur
    - Contrastes
    - Limites des formes
    - Textures
    - Matériaux
    - États physiques
    - Informations contextuelles
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        
        # Base de connaissances hiérarchique
        self.color_knowledge: Dict[ColorCategory, ColorKnowledge] = {}
        
        # Statistiques apprentissage
        self.colors_learned = 0
        self.deterministic_colors = 0
        self.probabilistic_colors = 0
        
        # Initialiser hiérarchie
        self._initialize_hierarchy()
    
    def _initialize_hierarchy(self):
        """Initialiser hiérarchie d'apprentissage des couleurs"""
        
        # NIVEAU 1: COULEURS PRIMAIRES (DÉTERMINISTE 100%)
        self.color_knowledge[ColorCategory.RED] = ColorKnowledge(
            category=ColorCategory.RED,
            confidence=1.0,
            level=1,
            rgb_range=((150, 255), (0, 100), (0, 100)),
            description="Rouge primaire - Base fondamentale",
            is_deterministic=True
        )
        
        self.color_knowledge[ColorCategory.BLUE] = ColorKnowledge(
            category=ColorCategory.BLUE,
            confidence=1.0,
            level=1,
            rgb_range=((0, 100), (0, 150), (150, 255)),
            description="Bleu primaire - Base fondamentale",
            is_deterministic=True
        )
        
        self.color_knowledge[ColorCategory.YELLOW] = ColorKnowledge(
            category=ColorCategory.YELLOW,
            confidence=1.0,
            level=1,
            rgb_range=((200, 255), (200, 255), (0, 100)),
            description="Jaune primaire - Base fondamentale",
            is_deterministic=True
        )
        
        # NIVEAU 2: BLANC ET NOIR (DÉTERMINISTE 100%)
        self.color_knowledge[ColorCategory.WHITE] = ColorKnowledge(
            category=ColorCategory.WHITE,
            confidence=1.0,
            level=2,
            rgb_range=((200, 255), (200, 255), (200, 255)),
            description="Blanc - Modificateur luminosité",
            is_deterministic=True
        )
        
        self.color_knowledge[ColorCategory.BLACK] = ColorKnowledge(
            category=ColorCategory.BLACK,
            confidence=1.0,
            level=2,
            rgb_range=((0, 50), (0, 50), (0, 50)),
            description="Noir - Modificateur obscurité",
            is_deterministic=True
        )
        
        # NIVEAU 3: COULEURS SECONDAIRES (PROBABILISTE)
        self.color_knowledge[ColorCategory.VIOLET] = ColorKnowledge(
            category=ColorCategory.VIOLET,
            confidence=0.9,
            level=3,
            rgb_range=((100, 200), (0, 100), (150, 255)),
            description="Violet - Mélange Rouge + Bleu",
            is_deterministic=False
        )
        
        self.color_knowledge[ColorCategory.GREEN] = ColorKnowledge(
            category=ColorCategory.GREEN,
            confidence=0.9,
            level=3,
            rgb_range=((0, 100), (150, 255), (0, 150)),
            description="Vert - Mélange Bleu + Jaune",
            is_deterministic=False
        )
        
        self.color_knowledge[ColorCategory.ORANGE] = ColorKnowledge(
            category=ColorCategory.ORANGE,
            confidence=0.9,
            level=3,
            rgb_range=((200, 255), (100, 200), (0, 100)),
            description="Orange - Mélange Rouge + Jaune",
            is_deterministic=False
        )
        
        # Compter couleurs apprises
        self.colors_learned = len(self.color_knowledge)
        self.deterministic_colors = sum(1 for c in self.color_knowledge.values() if c.is_deterministic)
        self.probabilistic_colors = self.colors_learned - self.deterministic_colors
        
        if self.verbose:
            print(f"[ColorHierarchy] Hiérarchie initialisée:")
            print(f"  - Couleurs apprises: {self.colors_learned}")
            print(f"  - Déterministes (Niveau 1-2): {self.deterministic_colors}")
            print(f"  - Probabilistes (Niveau 3+): {self.probabilistic_colors}")
    
    def recognize_color(self, rgb: Tuple[int, int, int]) -> Tuple[ColorCategory, float]:
        """
        Reconnaître couleur depuis RGB
        
        LOGIQUE:
        1. Tester d'abord couleurs déterministes (Niveau 1-2)
        2. Si aucune correspondance, tester probabilistes (Niveau 3+)
        3. Retourner catégorie + confiance
        
        Args:
            rgb: Tuple (R, G, B) avec valeurs 0-255
            
        Returns:
            (ColorCategory, confidence)
        """
        r, g, b = rgb
        
        # Tester couleurs par niveau (déterministes d'abord)
        candidates = []
        
        for color_cat, knowledge in self.color_knowledge.items():
            # Vérifier si RGB dans range
            r_min, r_max = knowledge.rgb_range[0]
            g_min, g_max = knowledge.rgb_range[1]
            b_min, b_max = knowledge.rgb_range[2]
            
            if (r_min <= r <= r_max and
                g_min <= g <= g_max and
                b_min <= b <= b_max):
                candidates.append((color_cat, knowledge.confidence, knowledge.level))
        
        if not candidates:
            return (ColorCategory.UNKNOWN, 0.0)
        
        # Trier par niveau (déterministes d'abord) puis confiance
        candidates.sort(key=lambda x: (x[2], -x[1]))
        
        best_category, best_confidence, _ = candidates[0]
        return (best_category, best_confidence)
    
    def recognize_arc_color(self, arc_color: int) -> Tuple[ColorCategory, float]:
        """
        Reconnaître couleur ARC-AGI (0-9)
        
        Mapping ARC-AGI standard:
        0 = Noir (background)
        1 = Bleu
        2 = Rouge
        3 = Vert
        4 = Jaune
        5 = Gris
        6 = Magenta
        7 = Orange
        8 = Cyan
        9 = Marron
        
        Args:
            arc_color: Valeur 0-9
            
        Returns:
            (ColorCategory, confidence)
        """
        # Mapping déterministe ARC → Catégories
        arc_mapping = {
            0: (ColorCategory.BLACK, 1.0),      # Background
            1: (ColorCategory.BLUE, 1.0),       # Primaire
            2: (ColorCategory.RED, 1.0),        # Primaire
            3: (ColorCategory.GREEN, 0.9),      # Secondaire
            4: (ColorCategory.YELLOW, 1.0),     # Primaire
            5: (ColorCategory.GRAY, 0.8),       # Tertiaire
            6: (ColorCategory.MAGENTA, 0.8),    # Tertiaire
            7: (ColorCategory.ORANGE, 0.9),     # Secondaire
            8: (ColorCategory.CYAN, 0.8),       # Tertiaire
            9: (ColorCategory.BROWN, 0.7),      # Tertiaire
        }
        
        if arc_color in arc_mapping:
            return arc_mapping[arc_color]
        else:
            return (ColorCategory.UNKNOWN, 0.0)
    
    def get_color_separation(self, grid: np.ndarray) -> Dict[ColorCategory, List[Tuple[int, int]]]:
        """
        Séparer grille par couleurs
        
        PRINCIPE FONDAMENTAL:
        La couleur donne la séparation des objets.
        
        Args:
            grid: Grille ARC (valeurs 0-9)
            
        Returns:
            Dict {ColorCategory: [(y, x), ...]}
        """
        separation = {}
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                arc_color = int(grid[i, j])
                category, _ = self.recognize_arc_color(arc_color)
                
                if category not in separation:
                    separation[category] = []
                separation[category].append((i, j))
        
        return separation
    
    def get_color_depth_map(self, grid: np.ndarray) -> np.ndarray:
        """
        Créer carte de profondeur basée sur couleurs
        
        PRINCIPE:
        - Couleurs claires = avant-plan
        - Couleurs foncées = arrière-plan
        - Blanc = maximum profondeur
        - Noir = minimum profondeur
        
        Args:
            grid: Grille ARC
            
        Returns:
            Carte profondeur (valeurs 0.0-1.0)
        """
        depth_map = np.zeros_like(grid, dtype=float)
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                arc_color = int(grid[i, j])
                category, confidence = self.recognize_arc_color(arc_color)
                
                # Assigner profondeur selon catégorie
                if category == ColorCategory.WHITE:
                    depth_map[i, j] = 1.0
                elif category == ColorCategory.BLACK:
                    depth_map[i, j] = 0.0
                elif category in [ColorCategory.YELLOW]:
                    depth_map[i, j] = 0.8
                elif category in [ColorCategory.RED, ColorCategory.BLUE]:
                    depth_map[i, j] = 0.6
                elif category in [ColorCategory.GREEN, ColorCategory.ORANGE]:
                    depth_map[i, j] = 0.5
                else:
                    depth_map[i, j] = 0.4
        
        return depth_map
    
    def get_statistics(self) -> Dict[str, any]:
        """Statistiques apprentissage"""
        return {
            'colors_learned': self.colors_learned,
            'deterministic_colors': self.deterministic_colors,
            'probabilistic_colors': self.probabilistic_colors,
            'hierarchy_levels': 5,
            'primary_colors': 3,
            'secondary_colors': 3
        }


# Made with Bob - Expert en Color Theory, Visual Learning, Hierarchical AI, Cognitive Science