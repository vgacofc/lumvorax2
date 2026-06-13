"""
MAGEN V2 - Rule Detector
Détecte règles contextuelles et génère programmes ciblés
Extrait de V1 ContextualRuleDetector
"""

import numpy as np
from typing import List, Tuple, Optional
from .program import Program


class RuleDetector:
    """Détecte règles contextuelles (formes L, patterns, etc.)"""
    
    def __init__(self):
        self.rules_detected = []
    
    def detect_L_shapes(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> bool:
        """
        Détecte si puzzle contient formes L avec coins marqués
        
        Vérifie si:
        1. Input contient formes en L (3 cellules formant angle droit)
        2. Output marque les coins intérieurs des L avec nouvelle couleur
        """
        for inp, out in train_pairs:
            # P1 CORRECTION: Vérifier dimensions input/output compatibles
            if inp.shape != out.shape:
                continue
            
            # Vérifier si nouvelle couleur apparaît dans output
            inp_colors = set(inp.flatten())
            out_colors = set(out.flatten())
            new_colors = out_colors - inp_colors
            
            if not new_colors:
                continue
            
            # Vérifier si pixels ajoutés sont aux coins de formes L
            h, w = inp.shape
            corners_marked = 0
            
            # Patterns de formes L (relatif au coin)
            l_patterns = [
                [(0, 1), (1, 0)],   # ┌ (coin haut-gauche)
                [(0, -1), (1, 0)],  # ┐ (coin haut-droite)
                [(0, 1), (-1, 0)],  # └ (coin bas-gauche)
                [(0, -1), (-1, 0)]  # ┘ (coin bas-droite)
            ]
            
            for y in range(h):
                for x in range(w):
                    # Vérifier si pixel ajouté dans output
                    if inp[y, x] == 0 and out[y, x] != 0:
                        # Vérifier si c'est un coin de L
                        for pattern in l_patterns:
                            is_L_corner = True
                            base_color = None
                            
                            for dy, dx in pattern:
                                ny, nx = y + dy, x + dx
                                if not (0 <= ny < h and 0 <= nx < w):
                                    is_L_corner = False
                                    break
                                
                                cell_color = inp[ny, nx]
                                if cell_color == 0:
                                    is_L_corner = False
                                    break
                                
                                if base_color is None:
                                    base_color = cell_color
                                elif cell_color != base_color:
                                    is_L_corner = False
                                    break
                            
                            if is_L_corner:
                                corners_marked += 1
                                break
            
            # Si au moins 2 coins marqués, c'est probablement une règle L
            if corners_marked >= 2:
                return True
        
        return False
    
    def generate_L_shape_program(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[Program]:
        """
        Génère programme pour marquer coins de formes L
        
        Returns:
            Programme detect_L + recolor ou None
        """
        if not self.detect_L_shapes(train_pairs):
            return None
        
        # Déterminer couleurs source et cible
        source_color = None
        target_color = None
        
        for inp, out in train_pairs:
            inp_colors = set(inp.flatten())
            out_colors = set(out.flatten())
            new_colors = out_colors - inp_colors
            
            if new_colors:
                target_color = list(new_colors)[0]
                
                # Trouver couleur source (couleur des L)
                for color in inp_colors:
                    if color != 0:
                        source_color = color
                        break
                
                break
        
        if source_color is None or target_color is None:
            return None
        
        # Créer programme: detect_L_shapes + recolor optionnel
        from .primitives_advanced import detect_and_mark_L_shapes
        
        prog = Program()
        prog.add_operation('detect_and_mark_L_shapes', detect_and_mark_L_shapes, 
                         args=(source_color, target_color))
        
        return prog
    
    def generate_rule_programs(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> List[Program]:
        """
        Génère tous les programmes basés sur règles détectées
        
        Returns:
            Liste de programmes candidats
        """
        programs = []
        
        # Règle 1: Formes L
        l_program = self.generate_L_shape_program(train_pairs)
        if l_program:
            programs.append(l_program)
            self.rules_detected.append('L_shape_corner_marker')
        
        # TODO: Ajouter autres règles (bounding box, duplication, etc.)
        
        return programs

# Made with Bob
