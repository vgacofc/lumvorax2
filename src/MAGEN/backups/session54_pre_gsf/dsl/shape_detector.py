"""
MAGEN V2 - Shape Detector
Détecte et génère programmes pour transformations shape
Extrait de V1 shape_transformer.py
"""

import numpy as np
from typing import List, Tuple, Dict, Optional
from .program import Program


class ShapeDetector:
    """Détecte transformations shape et génère programmes ciblés"""
    
    def __init__(self):
        self.transformation_types = [
            'same', 'upscale', 'downscale', 'crop', 'extend'
        ]
    
    def detect_transformation_type(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> str:
        """
        Détecte le type de transformation shape
        
        Returns:
            'same', 'upscale', 'downscale', 'crop', 'extend'
        """
        if not train_pairs:
            return 'same'
        
        shape_changes = []
        for inp, out in train_pairs:
            in_h, in_w = inp.shape
            out_h, out_w = out.shape
            
            shape_changes.append({
                'input': (in_h, in_w),
                'output': (out_h, out_w),
                'ratio_h': out_h / in_h if in_h > 0 else 1,
                'ratio_w': out_w / in_w if in_w > 0 else 1
            })
        
        # Same shape
        if all(sc['input'] == sc['output'] for sc in shape_changes):
            return 'same'
        
        # Upscaling
        ratios_h = [sc['ratio_h'] for sc in shape_changes]
        ratios_w = [sc['ratio_w'] for sc in shape_changes]
        
        if (all(r > 1 for r in ratios_h) and all(r > 1 for r in ratios_w) and
            len(set(ratios_h)) == 1 and len(set(ratios_w)) == 1):
            return 'upscale'
        
        # Downscaling
        if (all(r < 1 for r in ratios_h) or all(r < 1 for r in ratios_w)):
            return 'downscale'
        
        # Crop
        if all(sc['output'][0] <= sc['input'][0] and 
               sc['output'][1] <= sc['input'][1] for sc in shape_changes):
            return 'crop'
        
        # Extension
        if any(sc['output'][0] > sc['input'][0] or 
               sc['output'][1] > sc['input'][1] for sc in shape_changes):
            return 'extend'
        
        return 'same'
    
    def is_periodic_pattern(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> bool:
        """Vérifie si downscale est dû à pattern répété"""
        for inp, out in train_pairs:
            in_h, in_w = inp.shape
            out_h, out_w = out.shape
            
            # Vérifier compression horizontale
            if in_h == out_h and in_w > out_w:
                ratio = in_w // out_w
                if ratio > 1 and in_w % out_w == 0:
                    # Vérifier si pattern répété
                    pattern_width = out_w
                    first_pattern = inp[:, :pattern_width]
                    
                    is_repeated = True
                    for i in range(1, ratio):
                        start = i * pattern_width
                        end = start + pattern_width
                        current = inp[:, start:end]
                        if not np.array_equal(first_pattern, current):
                            is_repeated = False
                            break
                    
                    if is_repeated:
                        return True
        
        return False
    
    def calculate_downscale_factor(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[float]:
        """Calcule facteur de downscale"""
        if not train_pairs:
            return None
        
        factors = []
        for inp, out in train_pairs:
            in_h, in_w = inp.shape
            out_h, out_w = out.shape
            
            if out_h > 0 and out_w > 0:
                factor_h = in_h / out_h
                factor_w = in_w / out_w
                
                # Prendre facteur moyen si similaire
                if abs(factor_h - factor_w) < 0.1:
                    factors.append((factor_h + factor_w) / 2)
                else:
                    # Facteurs différents
                    factors.append(max(factor_h, factor_w))
        
        if factors:
            return sum(factors) / len(factors)
        return None
    
    def predict_output_shape(self, test_input: np.ndarray, 
                            train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Tuple[int, int]:
        """Prédit shape output pour test input"""
        if not train_pairs:
            return (test_input.shape[0], test_input.shape[1])
        
        # Calculer ratios moyens
        ratios_h = []
        ratios_w = []
        
        for inp, out in train_pairs:
            in_h, in_w = inp.shape
            out_h, out_w = out.shape
            
            if in_h > 0 and in_w > 0:
                ratios_h.append(out_h / in_h)
                ratios_w.append(out_w / in_w)
        
        if ratios_h and ratios_w:
            avg_ratio_h = sum(ratios_h) / len(ratios_h)
            avg_ratio_w = sum(ratios_w) / len(ratios_w)
            
            test_h, test_w = test_input.shape
            pred_h = int(test_h * avg_ratio_h)
            pred_w = int(test_w * avg_ratio_w)
            
            return (pred_h, pred_w)
        
        return (test_input.shape[0], test_input.shape[1])
    
    def generate_shape_programs(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]], 
                                test_input: np.ndarray) -> List[Program]:
        """
        Génère programmes ciblés pour transformations shape
        
        Returns:
            Liste de programmes candidats
        """
        programs = []
        shape_type = self.detect_transformation_type(train_pairs)
        
        if shape_type == 'same':
            return programs  # Pas de programmes shape
        
        elif shape_type == 'downscale':
            # PRIORITÉ 1: Extraction périodique (plus robuste que compression)
            factor = self.calculate_downscale_factor(train_pairs)
            if factor and factor == int(factor):
                from .primitives_advanced import extract_periodic_columns
                prog = Program()
                prog.add_operation('extract_periodic_columns', extract_periodic_columns,
                                 args=(int(factor),))
                programs.append(prog)
            
            # PRIORITÉ 2: Compression périodique (vérification stricte)
            if self.is_periodic_pattern(train_pairs):
                from .primitives_advanced import compress_periodic_pattern
                prog = Program()
                prog.add_operation('compress_periodic_pattern', compress_periodic_pattern)
                programs.append(prog)
            
            # PRIORITÉ 3: Downscale entier (moyenne)
            if factor and factor == int(factor):
                from .primitives import downscale
                prog = Program()
                prog.add_operation('downscale', downscale, args=(int(factor),))
                programs.append(prog)
            
            # PRIORITÉ 4: Downscale non-entier (échantillonnage)
            target_shape = self.predict_output_shape(test_input, train_pairs)
            if target_shape != test_input.shape:
                from .primitives_advanced import sample_grid_non_integer
                prog = Program()
                prog.add_operation('sample_grid_non_integer', sample_grid_non_integer,
                                 args=(target_shape,))
                programs.append(prog)
        
        elif shape_type == 'upscale':
            factor = self.calculate_downscale_factor(train_pairs)
            if factor and 1/factor == int(1/factor):
                from .primitives import scale
                prog = Program()
                prog.add_operation('scale', scale, args=(int(1/factor),))
                programs.append(prog)
        
        return programs

# Made with Bob
