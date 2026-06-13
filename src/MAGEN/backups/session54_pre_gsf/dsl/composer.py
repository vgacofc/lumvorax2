"""
MAGEN DSL Composer - Composition automatique de programmes

Ce module génère automatiquement des programmes candidats en composant
des primitives DSL de manière intelligente.
"""

import numpy as np
from typing import List, Callable, Tuple, Optional, Set
from itertools import product, combinations
import random

from .program import Program, Operation
from .primitives import *
from .primitives_advanced import *
from .shape_corner_marker import mark_shape_corners
from .quadrant_extractor import extract_bottom_right_quadrant, extract_common_pattern


class Composer:
    """
    Compositeur de programmes DSL
    
    Génère des programmes candidats en combinant des primitives de manière
    intelligente basée sur l'analyse des train pairs.
    """
    
    def __init__(self, max_depth: int = 5, max_candidates: int = 100):
        """
        Args:
            max_depth: Profondeur maximale de composition
            max_candidates: Nombre maximum de candidats à générer
        """
        self.max_depth = max_depth
        self.max_candidates = max_candidates
        
        # Catalogue de primitives disponibles
        self.primitives = self._build_primitive_catalog()
    
    def _build_primitive_catalog(self) -> dict:
        """
        Construit le catalogue de primitives disponibles
        
        Returns:
            Dict {catégorie: [(nom, fonction, générateur_args)]}
        """
        return {
            'geometric': [
                ('rotate90', rotate90, lambda g: [()]),
                ('rotate180', rotate180, lambda g: [()]),
                ('rotate270', rotate270, lambda g: [()]),
                ('mirror_horizontal', mirror_horizontal, lambda g: [()]),
                ('mirror_vertical', mirror_vertical, lambda g: [()]),
                ('transpose', transpose, lambda g: [()]),
            ],
            
            'scale': [
                ('scale', scale, lambda g: [(2,), (3,), (4,)]),
                ('downscale', downscale, lambda g: self._generate_downscale_factors(g)),
            ],
            
            'spatial': [
                ('crop', crop, lambda g: self._generate_crop_boxes(g)),
                ('pad', pad, lambda g: [(1, 1, 1, 1, 0), (2, 2, 2, 2, 0)]),
            ],
            
            'color': [
                ('recolor', recolor, lambda g: self._generate_recolor_pairs(g)),
                ('floodfill', floodfill, lambda g: self._generate_floodfill_ops(g)),
            ],
            
            'pattern': [
                ('repeat_pattern', repeat_pattern, lambda g: [(2, 2), (3, 3), (2, 3)]),
                ('extract_repeated_pattern', extract_repeated_pattern, lambda g: [()]),
                ('compress_periodic_pattern', compress_periodic_pattern, gen_compress_args),
            ],
            
            'advanced': [
                ('detect_and_mark_L_shapes', detect_and_mark_L_shapes, gen_detect_L_args),
                ('extract_semantic_region', extract_semantic_region, gen_extract_region_args),
                ('sample_grid_non_integer', sample_grid_non_integer, gen_sample_non_integer_args),
                ('downsample_by_majority', downsample_by_majority, gen_downsample_majority_args),
                ('mark_shape_corners', mark_shape_corners, lambda g: [(8, 1)]),  # Pour 3aa6fb7a
                ('extract_bottom_right_quadrant', extract_bottom_right_quadrant, lambda g: [()]),  # Pour 88a62173
                ('extract_common_pattern', extract_common_pattern, lambda g: [()]),  # Alternative 88a62173
            ],
        }
    
    def _generate_downscale_factors(self, grid: np.ndarray) -> List[tuple]:
        """Génère facteurs de downscale valides pour une grille"""
        h, w = grid.shape
        factors = []
        
        for f in [2, 3, 4, 5]:
            if h % f == 0 and w % f == 0:
                factors.append((f,))
        
        return factors if factors else [(1,)]
    
    def _generate_crop_boxes(self, grid: np.ndarray) -> List[tuple]:
        """Génère boîtes de crop candidates"""
        h, w = grid.shape
        boxes = []
        
        # Crop au contenu non-zero
        bbox = get_bounding_box(grid, background=0)
        if bbox:
            boxes.append((bbox,))
        
        # Quelques crops standards
        if h > 2 and w > 2:
            boxes.append((BoundingBox(1, 1, w-2, h-2),))  # Enlever bordure
        
        return boxes if boxes else [(BoundingBox(0, 0, w, h),)]
    
    def _generate_recolor_pairs(self, grid: np.ndarray) -> List[tuple]:
        """Génère paires de recoloration candidates"""
        colors = count_colors(grid)
        pairs = []
        
        # Essayer de remplacer chaque couleur par chaque autre
        for from_color in colors.keys():
            for to_color in range(10):  # ARC utilise couleurs 0-9
                if from_color != to_color:
                    pairs.append((from_color, to_color))
        
        return pairs[:20]  # Limiter le nombre
    
    def _generate_floodfill_ops(self, grid: np.ndarray) -> List[tuple]:
        """Génère opérations floodfill candidates"""
        h, w = grid.shape
        ops = []
        
        # Positions intéressantes: coins et centre
        positions = [
            (0, 0), (0, w-1), (h-1, 0), (h-1, w-1),  # Coins
            (h//2, w//2)  # Centre
        ]
        
        for pos in positions:
            for color in range(10):
                ops.append((pos, color))
        
        return ops[:15]  # Limiter
    
    def generate_single_op_programs(self, input_grid: np.ndarray) -> List[Program]:
        """
        Génère programmes à une seule opération
        
        Args:
            input_grid: Grille d'entrée pour guider la génération
            
        Returns:
            Liste de programmes candidats
        """
        programs = []
        
        for category, primitives in self.primitives.items():
            for name, func, arg_generator in primitives:
                # Générer arguments possibles
                try:
                    arg_sets = arg_generator(input_grid)
                except Exception:
                    arg_sets = [()]
                
                # Créer un programme pour chaque combinaison d'arguments
                for args in arg_sets:
                    program = Program()
                    program.add_operation(name, func, args=args)
                    programs.append(program)
                    
                    if len(programs) >= self.max_candidates:
                        return programs
        
        return programs
    
    def generate_two_op_programs(self, input_grid: np.ndarray,
                                 categories: Optional[List[str]] = None) -> List[Program]:
        """
        Génère programmes à deux opérations
        
        Args:
            input_grid: Grille d'entrée
            categories: Catégories de primitives à utiliser (None = toutes)
            
        Returns:
            Liste de programmes candidats
        """
        if categories is None:
            categories = list(self.primitives.keys())
        
        programs = []
        
        # Combinaisons de catégories intéressantes
        interesting_combos = [
            ('geometric', 'geometric'),  # Double transformation géométrique
            ('geometric', 'scale'),      # Rotation puis scale
            ('spatial', 'color'),        # Crop puis recolor
            ('scale', 'color'),          # Scale puis recolor
            ('geometric', 'color'),      # Rotation puis recolor
        ]
        
        for cat1, cat2 in interesting_combos:
            if cat1 not in categories or cat2 not in categories:
                continue
            
            prims1 = self.primitives[cat1]
            prims2 = self.primitives[cat2]
            
            # Limiter combinaisons pour éviter explosion
            for (name1, func1, gen1), (name2, func2, gen2) in product(
                random.sample(prims1, min(3, len(prims1))),
                random.sample(prims2, min(3, len(prims2)))
            ):
                try:
                    args1_sets = gen1(input_grid)[:2]  # Max 2 arg sets
                    
                    for args1 in args1_sets:
                        # Simuler première opération
                        try:
                            intermediate = func1(input_grid, *args1)
                        except Exception:
                            continue
                        
                        args2_sets = gen2(intermediate)[:2]
                        
                        for args2 in args2_sets:
                            program = Program()
                            program.add_operation(name1, func1, args=args1)
                            program.add_operation(name2, func2, args=args2)
                            programs.append(program)
                            
                            if len(programs) >= self.max_candidates:
                                return programs
                
                except Exception:
                    continue
        
        return programs
    
    def generate_programs_from_analysis(self, 
                                       input_grid: np.ndarray,
                                       output_grid: np.ndarray,
                                       max_programs: int = 50) -> List[Program]:
        """
        Génère programmes basés sur l'analyse input→output
        
        Args:
            input_grid: Grille d'entrée
            output_grid: Grille de sortie attendue
            max_programs: Nombre maximum de programmes
            
        Returns:
            Liste de programmes candidats ordonnés par pertinence
        """
        programs = []
        
        # Analyse des différences
        shape_changed = input_grid.shape != output_grid.shape
        colors_in = set(np.unique(input_grid))
        colors_out = set(np.unique(output_grid))
        color_changed = colors_in != colors_out
        
        # Stratégie 1: Changement de forme
        if shape_changed:
            h_in, w_in = input_grid.shape
            h_out, w_out = output_grid.shape
            
            # Downscale?
            if h_out < h_in and w_out < w_in:
                if h_in % h_out == 0 and w_in % w_out == 0:
                    factor_h = h_in // h_out
                    factor_w = w_in // w_out
                    
                    # Vérifier si facteurs égaux (downscale uniforme)
                    if factor_h == factor_w:
                        # Vérifier si input est répétition de output (pattern tiling)
                        if factor_h > 1:
                            pattern = input_grid[:h_out, :w_out]
                            is_repeated = True
                            
                            for i in range(factor_h):
                                for j in range(factor_w):
                                    block = input_grid[
                                        i*h_out:(i+1)*h_out,
                                        j*w_out:(j+1)*w_out
                                    ]
                                    if not np.array_equal(block, pattern):
                                        is_repeated = False
                                        break
                                if not is_repeated:
                                    break
                            
                            if is_repeated:
                                # Pattern répété détecté! Priorité haute
                                prog = Program()
                                prog.add_operation('downscale', downscale, args=(factor_h,))
                                programs.insert(0, prog)  # Priorité absolue
                            else:
                                # Downscale simple sans répétition
                                prog = Program()
                                prog.add_operation('downscale', downscale, args=(factor_h,))
                                programs.append(prog)
                        else:
                            # Factor = 1, pas de downscale
                            pass
            
            # Upscale?
            elif h_out > h_in and w_out > w_in:
                if h_out % h_in == 0 and w_out % w_in == 0:
                    factor = h_out // h_in
                    prog = Program()
                    prog.add_operation('scale', scale, args=(factor,))
                    programs.append(prog)
            
            # Crop?
            elif h_out <= h_in and w_out <= w_in:
                # Essayer crop au contenu
                bbox = get_bounding_box(input_grid, background=0)
                if bbox and bbox.width == w_out and bbox.height == h_out:
                    prog = Program()
                    prog.add_operation('crop', crop, args=(bbox,))
                    programs.append(prog)
        
        # Stratégie 2: Même forme, couleurs différentes
        elif color_changed:
            # Essayer recolorations simples
            for from_color in colors_in:
                for to_color in colors_out:
                    if from_color != to_color:
                        prog = Program()
                        prog.add_operation('recolor', recolor, 
                                         args=(from_color, to_color))
                        programs.append(prog)
                        
                        if len(programs) >= max_programs:
                            return programs
        
        # Stratégie 3: Transformations géométriques
        if input_grid.shape == output_grid.shape:
            # Tester rotations
            for name, func in [
                ('rotate90', rotate90),
                ('rotate180', rotate180),
                ('rotate270', rotate270),
                ('mirror_horizontal', mirror_horizontal),
                ('mirror_vertical', mirror_vertical),
                ('transpose', transpose)
            ]:
                prog = Program()
                prog.add_operation(name, func)
                programs.append(prog)
        
        # Compléter avec programmes génériques si nécessaire
        if len(programs) < max_programs:
            generic = self.generate_single_op_programs(input_grid)
            programs.extend(generic[:max_programs - len(programs)])
        
        return programs[:max_programs]
    
    def compose_incrementally(self, 
                            base_program: Program,
                            input_grid: np.ndarray,
                            target_output: np.ndarray,
                            max_additions: int = 3) -> List[Program]:
        """
        Composition incrémentale: ajoute des opérations à un programme existant
        
        Args:
            base_program: Programme de base
            input_grid: Grille d'entrée
            target_output: Sortie cible
            max_additions: Nombre maximum d'opérations à ajouter
            
        Returns:
            Liste de programmes étendus
        """
        programs = [base_program.copy()]
        
        # Exécuter programme de base
        try:
            current_output = base_program.execute(input_grid)
        except Exception:
            return []
        
        # Si déjà correct, retourner
        if np.array_equal(current_output, target_output):
            return [base_program]
        
        # Sinon, essayer d'ajouter des opérations
        for depth in range(1, max_additions + 1):
            new_programs = []
            
            for prog in programs:
                # Générer opérations candidates basées sur l'écart actuel
                candidates = self.generate_programs_from_analysis(
                    current_output, target_output, max_programs=10
                )
                
                for candidate in candidates:
                    if candidate.length() > 0:
                        extended = prog.extend(candidate)
                        new_programs.append(extended)
            
            programs.extend(new_programs)
            
            if len(programs) >= self.max_candidates:
                break
        
        return programs[:self.max_candidates]


def generate_program_variants(program: Program, max_variants: int = 10) -> List[Program]:
    """
    Génère variantes d'un programme en modifiant légèrement les paramètres
    
    Args:
        program: Programme de base
        max_variants: Nombre maximum de variantes
        
    Returns:
        Liste de programmes variantes
    """
    variants = [program.copy()]
    
    for i, op in enumerate(program.operations):
        # Variantes pour scale
        if op.name == 'scale' and op.args:
            factor = op.args[0]
            for new_factor in [factor - 1, factor + 1]:
                if new_factor > 0:
                    variant = program.copy()
                    variant.operations[i] = Operation(
                        op.name, op.func, args=(new_factor,), kwargs=op.kwargs
                    )
                    variants.append(variant)
        
        # Variantes pour recolor
        elif op.name == 'recolor' and len(op.args) >= 2:
            from_color, to_color = op.args[0], op.args[1]
            for new_to in range(10):
                if new_to != to_color:
                    variant = program.copy()
                    variant.operations[i] = Operation(
                        op.name, op.func, 
                        args=(from_color, new_to), 
                        kwargs=op.kwargs
                    )
                    variants.append(variant)
                    
                    if len(variants) >= max_variants:
                        return variants
    
    return variants[:max_variants]


__all__ = [
    'Composer',
    'generate_program_variants'
]

# Made with Bob
