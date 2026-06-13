"""
MAGEN DSL - Region Extraction for Semantic Reasoning

Extrait régions candidates et score leur pertinence.
Basé sur analyse forensique 5bd6f4ac et 88a62173.

Architecture: Region extraction devient hypothèse primaire, pas étape secondaire.
"""

import numpy as np
from typing import List, Tuple, Optional, Dict
from .program import Program


class RegionExtractor:
    """
    Extrait régions candidates et génère programmes d'extraction
    
    Régions supportées:
    - Quadrants (4): top_left, top_right, bottom_left, bottom_right
    - Center
    - Borders
    - Custom crops basés sur analyse train pairs
    """
    
    def extract_quadrant(self, grid: np.ndarray, quadrant: str) -> np.ndarray:
        """
        Extrait un quadrant spécifique
        
        Args:
            grid: Grille d'entrée
            quadrant: 'top_left', 'top_right', 'bottom_left', 'bottom_right'
            
        Returns:
            Sous-grille extraite
        """
        h, w = grid.shape
        mid_h = h // 2
        mid_w = w // 2
        
        if quadrant == 'top_left':
            return grid[:mid_h, :mid_w]
        elif quadrant == 'top_right':
            return grid[:mid_h, mid_w:]
        elif quadrant == 'bottom_left':
            return grid[mid_h:, :mid_w]
        elif quadrant == 'bottom_right':
            return grid[mid_h:, mid_w:]
        else:
            raise ValueError(f"Unknown quadrant: {quadrant}")
    
    def extract_region(self, grid: np.ndarray, 
                      start_y: int, start_x: int,
                      height: int, width: int) -> np.ndarray:
        """
        Extrait région rectangulaire
        
        Args:
            grid: Grille d'entrée
            start_y, start_x: Position de départ
            height, width: Dimensions de la région
            
        Returns:
            Sous-grille extraite
        """
        h, w = grid.shape
        
        # Assurer limites
        end_y = min(start_y + height, h)
        end_x = min(start_x + width, w)
        
        return grid[start_y:end_y, start_x:end_x]
    
    def detect_best_region(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> Optional[Dict]:
        """
        Détecte la meilleure région à extraire basée sur train pairs
        
        Stratégie:
        1. Pour chaque train pair, tester toutes les régions candidates
        2. Scorer chaque région (match avec output)
        3. Retourner région avec meilleur score moyen
        
        Returns:
            Dict avec 'type', 'params', 'score' ou None
        """
        if not train_pairs:
            return None
        
        # Candidats à tester
        candidates = []
        
        # 1. Quadrants
        for quadrant in ['top_left', 'top_right', 'bottom_left', 'bottom_right']:
            candidates.append({
                'type': 'quadrant',
                'params': {'quadrant': quadrant},
                'scores': []
            })
        
        # 2. Régions basées sur ratio output/input
        first_inp, first_out = train_pairs[0]
        out_h, out_w = first_out.shape
        inp_h, inp_w = first_inp.shape
        
        # Si output plus petit, tester extraction de coins
        if out_h < inp_h and out_w < inp_w:
            # Coin haut-gauche
            candidates.append({
                'type': 'corner',
                'params': {'start_y': 0, 'start_x': 0, 'height': out_h, 'width': out_w},
                'scores': []
            })
            
            # Coin haut-droit
            candidates.append({
                'type': 'corner',
                'params': {'start_y': 0, 'start_x': inp_w - out_w, 'height': out_h, 'width': out_w},
                'scores': []
            })
            
            # Coin bas-gauche
            candidates.append({
                'type': 'corner',
                'params': {'start_y': inp_h - out_h, 'start_x': 0, 'height': out_h, 'width': out_w},
                'scores': []
            })
            
            # Coin bas-droit
            candidates.append({
                'type': 'corner',
                'params': {'start_y': inp_h - out_h, 'start_x': inp_w - out_w, 'height': out_h, 'width': out_w},
                'scores': []
            })
        
        # 3. Scorer chaque candidat sur tous les train pairs
        for inp, out in train_pairs:
            for candidate in candidates:
                try:
                    if candidate['type'] == 'quadrant':
                        extracted = self.extract_quadrant(inp, candidate['params']['quadrant'])
                    elif candidate['type'] == 'corner':
                        extracted = self.extract_region(
                            inp,
                            candidate['params']['start_y'],
                            candidate['params']['start_x'],
                            candidate['params']['height'],
                            candidate['params']['width']
                        )
                    else:
                        continue
                    
                    # Score: pourcentage de pixels matchant
                    if extracted.shape == out.shape:
                        matches = np.sum(extracted == out)
                        total = extracted.size
                        score = matches / total if total > 0 else 0.0
                    else:
                        score = 0.0
                    
                    candidate['scores'].append(score)
                except:
                    candidate['scores'].append(0.0)
        
        # 4. Trouver meilleur candidat
        best_candidate = None
        best_avg_score = 0.0
        
        for candidate in candidates:
            if candidate['scores']:
                avg_score = sum(candidate['scores']) / len(candidate['scores'])
                if avg_score > best_avg_score:
                    best_avg_score = avg_score
                    best_candidate = {
                        'type': candidate['type'],
                        'params': candidate['params'],
                        'score': avg_score
                    }
        
        # Retourner seulement si score > 0.8 (80% match)
        if best_candidate and best_candidate['score'] > 0.8:
            return best_candidate
        
        return None
    
    def generate_region_programs(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                                test_input: np.ndarray) -> List[Program]:
        """
        Génère programmes d'extraction de région
        
        Returns:
            Liste de programmes candidats
        """
        programs = []
        
        # 1. Détecter meilleure région
        best_region = self.detect_best_region(train_pairs)
        
        if best_region:
            # Créer programme pour région détectée
            prog = Program()
            
            if best_region['type'] == 'quadrant':
                from .primitives_advanced import extract_semantic_region
                # Mapper quadrant vers region_type
                quadrant_map = {
                    'top_left': 'top_left_quadrant',
                    'top_right': 'top_right_quadrant',
                    'bottom_left': 'bottom_left_quadrant',
                    'bottom_right': 'bottom_right_quadrant'
                }
                region_type = quadrant_map.get(best_region['params']['quadrant'], 'top_left_quadrant')
                prog.add_operation('extract_semantic_region', extract_semantic_region, args=(region_type,))
            
            elif best_region['type'] == 'corner':
                # Créer primitive crop custom
                params = best_region['params']
                
                def crop_region(grid):
                    return self.extract_region(
                        grid,
                        params['start_y'],
                        params['start_x'],
                        params['height'],
                        params['width']
                    )
                
                prog.add_operation('crop_region', crop_region)
            
            programs.append(prog)
        
        # 2. Générer programmes pour tous les quadrants (fallback)
        from .primitives_advanced import extract_semantic_region
        
        for region_type in ['top_left_quadrant', 'top_right_quadrant', 
                           'bottom_left_quadrant', 'bottom_right_quadrant']:
            prog = Program()
            prog.add_operation('extract_semantic_region', extract_semantic_region, args=(region_type,))
            programs.append(prog)
        
        return programs


# Made with Bob - Session 32