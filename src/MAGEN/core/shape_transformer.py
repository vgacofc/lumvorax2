"""
Shape Transformer - Gestion transformations géométriques
Gère les puzzles où output_shape != input_shape (34.5% du dataset)
"""

import numpy as np
from typing import List, Tuple, Dict, Optional

class ShapeTransformer:
    """Détecte et applique transformations de shape"""
    
    def __init__(self):
        self.transformation_types = [
            'same',      # input_shape == output_shape
            'upscale',   # output plus grand (répétition)
            'downscale', # output plus petit (extraction)
            'crop',      # extraction zone spécifique
            'extend'     # ajout lignes/colonnes
        ]
    
    def detect_transformation_type(self, train_pairs: List[Dict]) -> str:
        """
        Détecte le type de transformation shape à partir des train pairs
        
        Args:
            train_pairs: Liste de paires (input, output)
            
        Returns:
            Type transformation: 'same', 'upscale', 'downscale', 'crop', 'extend'
        """
        if not train_pairs:
            return 'same'
        
        # Analyser toutes les paires
        shape_changes = []
        for pair in train_pairs:
            input_grid = np.array(pair['input'])
            output_grid = np.array(pair['output'])
            
            in_h, in_w = input_grid.shape
            out_h, out_w = output_grid.shape
            
            shape_changes.append({
                'input': (in_h, in_w),
                'output': (out_h, out_w),
                'ratio_h': out_h / in_h if in_h > 0 else 1,
                'ratio_w': out_w / in_w if in_w > 0 else 1
            })
        
        # Vérifier si toutes les paires ont le même type de transformation
        first = shape_changes[0]
        
        # Same shape
        if all(sc['input'] == sc['output'] for sc in shape_changes):
            return 'same'
        
        # Upscaling (facteur constant)
        ratios_h = [sc['ratio_h'] for sc in shape_changes]
        ratios_w = [sc['ratio_w'] for sc in shape_changes]
        
        if (all(r > 1 for r in ratios_h) and all(r > 1 for r in ratios_w) and
            len(set(ratios_h)) == 1 and len(set(ratios_w)) == 1):
            return 'upscale'
        
        # Downscaling
        if (all(r < 1 for r in ratios_h) or all(r < 1 for r in ratios_w)):
            return 'downscale'
        
        # Crop (extraction zone)
        if all(sc['output'][0] <= sc['input'][0] and 
               sc['output'][1] <= sc['input'][1] for sc in shape_changes):
            return 'crop'
        
        # Extension (ajout lignes/colonnes)
        if any(sc['output'][0] > sc['input'][0] or 
               sc['output'][1] > sc['input'][1] for sc in shape_changes):
            return 'extend'
        
        return 'same'
    
    def predict_output_shape(self, input_shape: Tuple[int, int], 
                            train_pairs: List[Dict]) -> Tuple[int, int]:
        """
        Prédit la shape de l'output à partir de l'input et des train pairs
        
        Args:
            input_shape: Shape de l'input test
            train_pairs: Paires d'entraînement
            
        Returns:
            Shape prédite pour l'output
        """
        if not train_pairs:
            return input_shape
        
        # Calculer ratios moyens
        ratios_h = []
        ratios_w = []
        
        for pair in train_pairs:
            in_grid = np.array(pair['input'])
            out_grid = np.array(pair['output'])
            
            in_h, in_w = in_grid.shape
            out_h, out_w = out_grid.shape
            
            if in_h > 0 and in_w > 0:
                ratios_h.append(out_h / in_h)
                ratios_w.append(out_w / in_w)
        
        if not ratios_h or not ratios_w:
            return input_shape
        
        # Ratio moyen
        avg_ratio_h = np.mean(ratios_h)
        avg_ratio_w = np.mean(ratios_w)
        
        # Prédire output shape
        pred_h = int(input_shape[0] * avg_ratio_h)
        pred_w = int(input_shape[1] * avg_ratio_w)
        
        return (max(1, pred_h), max(1, pred_w))
    
    def apply_upscaling(self, grid: np.ndarray, target_shape: Tuple[int, int]) -> np.ndarray:
        """
        Applique upscaling en répétant pixels
        
        Args:
            grid: Grid input
            target_shape: Shape cible
            
        Returns:
            Grid upscalé
        """
        in_h, in_w = grid.shape
        out_h, out_w = target_shape
        
        # Calculer facteurs
        factor_h = out_h // in_h if in_h > 0 else 1
        factor_w = out_w // in_w if in_w > 0 else 1
        
        # Répéter pixels
        result = np.repeat(np.repeat(grid, factor_h, axis=0), factor_w, axis=1)
        
        # Ajuster si nécessaire
        if result.shape[0] > out_h:
            result = result[:out_h, :]
        if result.shape[1] > out_w:
            result = result[:, :out_w]
        
        # Padding si nécessaire
        if result.shape[0] < out_h or result.shape[1] < out_w:
            padded = np.zeros(target_shape, dtype=grid.dtype)
            padded[:result.shape[0], :result.shape[1]] = result
            result = padded
        
        return result
    
    def apply_downscaling(self, grid: np.ndarray, target_shape: Tuple[int, int],
                         train_pairs: Optional[List[Dict]] = None) -> np.ndarray:
        """
        Applique downscaling intelligent avec plusieurs heuristiques
        
        Args:
            grid: Grid input
            target_shape: Shape cible
            train_pairs: Paires pour analyser pattern extraction
            
        Returns:
            Grid downscalé
        """
        in_h, in_w = grid.shape
        out_h, out_w = target_shape
        
        # CAS 1: Extraction vers 1x1 (valeur unique)
        if out_h == 1 and out_w == 1:
            return self._extract_single_value(grid, train_pairs)
        
        # CAS 2: Extraction pattern répété (détection répétition)
        if train_pairs:
            repeated_pattern = self._detect_repeated_pattern(grid, target_shape, train_pairs)
            if repeated_pattern is not None:
                return repeated_pattern
        
        # CAS 3: Extraction zone spécifique (analyse train pairs)
        if train_pairs:
            specific_zone = self._extract_specific_zone(grid, target_shape, train_pairs)
            if specific_zone is not None:
                return specific_zone
        
        # CAS 4: Fallback - extraire centre (comportement original)
        return self._extract_center(grid, target_shape)
    
    def _extract_single_value(self, grid: np.ndarray,
                             train_pairs: Optional[List[Dict]] = None) -> np.ndarray:
        """
        Extrait valeur unique (1x1) avec scoring multi-heuristiques
        
        Teste 6 heuristiques (dont 1 composite) et choisit celle avec meilleur score sur train pairs
        """
        # Définir toutes les heuristiques
        heuristics = {}
        
        # Heuristique 1: Couleur dominante globale
        unique, counts = np.unique(grid, return_counts=True)
        heuristics['dominant_global'] = unique[np.argmax(counts)]
        
        # Heuristique 2: Couleur non-zéro dominante
        non_zero = grid[grid != 0]
        if len(non_zero) > 0:
            unique_nz, counts_nz = np.unique(non_zero, return_counts=True)
            heuristics['dominant_nonzero'] = unique_nz[np.argmax(counts_nz)]
        else:
            heuristics['dominant_nonzero'] = heuristics['dominant_global']
        
        # Heuristique 3: Pixel central
        center_h = grid.shape[0] // 2
        center_w = grid.shape[1] // 2
        heuristics['center_pixel'] = grid[center_h, center_w]
        
        # Heuristique 4: Couleur la plus fréquente (excluant 0)
        if len(non_zero) > 0:
            heuristics['most_frequent_nonzero'] = heuristics['dominant_nonzero']
        else:
            heuristics['most_frequent_nonzero'] = heuristics['dominant_global']
        
        # Heuristique 5: Première couleur non-zéro rencontrée
        if len(non_zero) > 0:
            heuristics['first_nonzero'] = non_zero[0]
        else:
            heuristics['first_nonzero'] = 0
        
        # Heuristique 6: COMPOSITE INTELLIGENTE (pour puzzles comme 239be575)
        # Si dominant_global = 0 ET compte(0) > 50% → output = 0
        # Sinon → output = dominant_nonzero
        total_pixels = grid.size
        zero_count = np.sum(grid == 0)
        zero_ratio = zero_count / total_pixels
        
        if heuristics['dominant_global'] == 0 and zero_ratio > 0.5:
            heuristics['composite_zero_dominant'] = 0
        else:
            heuristics['composite_zero_dominant'] = heuristics['dominant_nonzero']
        
        # Heuristique 7: ALL_ROWS_HAVE_DOMINANT_NONZERO
        # Si toutes les lignes contiennent au moins un pixel de la couleur non-zéro dominante → output = dominant_nonzero
        # Sinon → output = 0
        rows_with_dominant_nz = np.sum(np.any(grid == heuristics['dominant_nonzero'], axis=1))
        total_rows = grid.shape[0]
        
        if rows_with_dominant_nz == total_rows:
            heuristics['all_rows_have_dominant_nonzero'] = heuristics['dominant_nonzero']
        else:
            heuristics['all_rows_have_dominant_nonzero'] = 0
        
        # Si pas de train pairs, retourner couleur non-zéro dominante (meilleure heuristique générale)
        if not train_pairs:
            return np.array([[heuristics['dominant_nonzero']]])
        
        # Scorer chaque heuristique sur train pairs
        scores = {name: 0 for name in heuristics}
        
        for pair in train_pairs:
            in_grid = np.array(pair['input'])
            out_grid = np.array(pair['output'])
            
            if out_grid.shape != (1, 1):
                continue
            
            expected_output = out_grid[0, 0]
            
            # Calculer valeur pour chaque heuristique sur ce train pair
            pair_heuristics = {}
            
            # Heuristique 1: Couleur dominante globale
            unique_in, counts_in = np.unique(in_grid, return_counts=True)
            pair_heuristics['dominant_global'] = unique_in[np.argmax(counts_in)]
            
            # Heuristique 2: Couleur non-zéro dominante
            non_zero_in = in_grid[in_grid != 0]
            if len(non_zero_in) > 0:
                unique_nz_in, counts_nz_in = np.unique(non_zero_in, return_counts=True)
                pair_heuristics['dominant_nonzero'] = unique_nz_in[np.argmax(counts_nz_in)]
            else:
                pair_heuristics['dominant_nonzero'] = pair_heuristics['dominant_global']
            
            # Heuristique 3: Pixel central
            center_h_in = in_grid.shape[0] // 2
            center_w_in = in_grid.shape[1] // 2
            pair_heuristics['center_pixel'] = in_grid[center_h_in, center_w_in]
            
            # Heuristique 4: Couleur la plus fréquente (excluant 0)
            pair_heuristics['most_frequent_nonzero'] = pair_heuristics['dominant_nonzero']
            
            # Heuristique 5: Première couleur non-zéro
            if len(non_zero_in) > 0:
                pair_heuristics['first_nonzero'] = non_zero_in[0]
            else:
                pair_heuristics['first_nonzero'] = 0
            
            # Heuristique 6: COMPOSITE INTELLIGENTE
            total_pixels_in = in_grid.size
            zero_count_in = np.sum(in_grid == 0)
            zero_ratio_in = zero_count_in / total_pixels_in
            
            if pair_heuristics['dominant_global'] == 0 and zero_ratio_in > 0.5:
                pair_heuristics['composite_zero_dominant'] = 0
            else:
                pair_heuristics['composite_zero_dominant'] = pair_heuristics['dominant_nonzero']
            
            # Heuristique 7: ALL_ROWS_HAVE_DOMINANT_NONZERO
            rows_with_dominant_nz_in = np.sum(np.any(in_grid == pair_heuristics['dominant_nonzero'], axis=1))
            total_rows_in = in_grid.shape[0]
            
            if rows_with_dominant_nz_in == total_rows_in:
                pair_heuristics['all_rows_have_dominant_nonzero'] = pair_heuristics['dominant_nonzero']
            else:
                pair_heuristics['all_rows_have_dominant_nonzero'] = 0
            
            # Heuristique 8: Couleur avec ratio le plus proche de 50%
            total_pixels_in = in_grid.size
            ratios_in = {c: count/total_pixels_in for c, count in zip(unique_in, counts_in)}
            closest_to_50_in = min(ratios_in.items(), key=lambda x: abs(x[1] - 0.5))
            pair_heuristics['closest_to_50_percent'] = closest_to_50_in[0]
            
            # Heuristique 9: Couleur dominante par ligne (majorité des lignes)
            row_dominants_in = []
            for row in in_grid:
                if len(row[row != 0]) > 0:
                    unique_row, counts_row = np.unique(row[row != 0], return_counts=True)
                    row_dominants_in.append(unique_row[np.argmax(counts_row)])
            
            if row_dominants_in:
                unique_rd_in, counts_rd_in = np.unique(row_dominants_in, return_counts=True)
                pair_heuristics['row_dominant_majority'] = unique_rd_in[np.argmax(counts_rd_in)]
            else:
                pair_heuristics['row_dominant_majority'] = 0
            
            # Heuristique 10: Couleur dominante par colonne (majorité des colonnes)
            col_dominants_in = []
            for col_idx in range(in_grid.shape[1]):
                col = in_grid[:, col_idx]
                if len(col[col != 0]) > 0:
                    unique_col, counts_col = np.unique(col[col != 0], return_counts=True)
                    col_dominants_in.append(unique_col[np.argmax(counts_col)])
            
            if col_dominants_in:
                unique_cd_in, counts_cd_in = np.unique(col_dominants_in, return_counts=True)
                pair_heuristics['col_dominant_majority'] = unique_cd_in[np.argmax(counts_cd_in)]
            else:
                pair_heuristics['col_dominant_majority'] = 0
            
            # Heuristique 11: Couleur avec le plus grand cluster continu
            try:
                from scipy import ndimage
                max_cluster_size_in = 0
                max_cluster_color_in = 0
                for color in unique_in:
                    if color == 0:
                        continue
                    binary = (in_grid == color).astype(int)
                    label_result = ndimage.label(binary)
                    labeled = label_result[0]
                    num_features = label_result[1]
                    if num_features > 0:
                        sizes = ndimage.sum(binary, labeled, range(1, num_features + 1))
                        max_size = int(np.max(sizes))
                        if max_size > max_cluster_size_in:
                            max_cluster_size_in = max_size
                            max_cluster_color_in = color
                pair_heuristics['largest_cluster'] = max_cluster_color_in
            except:
                pair_heuristics['largest_cluster'] = 0
            
            # Heuristique 12: Couleur avec le plus de touches aux bords
            border_touches_in = {}
            for color in unique_in:
                if color == 0:
                    continue
                touches = 0
                touches += np.sum(in_grid[0, :] == color)
                touches += np.sum(in_grid[-1, :] == color)
                touches += np.sum(in_grid[:, 0] == color)
                touches += np.sum(in_grid[:, -1] == color)
                border_touches_in[color] = touches
            
            if border_touches_in:
                pair_heuristics['most_border_touches'] = max(border_touches_in.items(), key=lambda x: x[1])[0]
            else:
                pair_heuristics['most_border_touches'] = 0
            
            # Heuristique 13: COMPOSITE (H9 AND H10 AND H11 AND H12)
            h9_in = pair_heuristics.get('row_dominant_majority', 0)
            h10_in = pair_heuristics.get('col_dominant_majority', 0)
            h11_in = pair_heuristics.get('largest_cluster', 0)
            h12_in = pair_heuristics.get('most_border_touches', 0)
            
            if h9_in != 0 and h9_in == h10_in == h11_in == h12_in:
                pair_heuristics['composite_h9_h10_h11_h12'] = h9_in
            else:
                pair_heuristics['composite_h9_h10_h11_h12'] = 0
            
            # Scorer chaque heuristique
            for name, value in pair_heuristics.items():
                if name not in scores:
                    scores[name] = 0
                if value == expected_output:
                    scores[name] += 1
        
        # Choisir heuristique avec meilleur score
        best_heuristic = max(scores.keys(), key=lambda k: scores[k])
        best_score = scores[best_heuristic]
        
        # Si aucune heuristique ne matche, retourner couleur non-zéro dominante
        if best_score == 0:
            return np.array([[heuristics['dominant_nonzero']]])
        
        return np.array([[heuristics[best_heuristic]]])
    
    def _detect_repeated_pattern(self, grid: np.ndarray, target_shape: Tuple[int, int],
                                train_pairs: List[Dict]) -> Optional[np.ndarray]:
        """
        Détecte si grid contient pattern répété et extrait 1ère occurrence
        
        Exemple: [A|A|A] → [A] (pattern répété 3x horizontalement)
        """
        out_h, out_w = target_shape
        in_h, in_w = grid.shape
        
        # Vérifier si dimensions sont multiples
        if in_w % out_w == 0 and in_h == out_h:
            # Pattern répété horizontalement
            repetitions = in_w // out_w
            
            # Extraire 1ère occurrence
            pattern = grid[:, :out_w]
            
            # Vérifier si pattern se répète
            is_repeated = True
            for i in range(1, repetitions):
                segment = grid[:, i*out_w:(i+1)*out_w]
                if not np.array_equal(pattern, segment):
                    is_repeated = False
                    break
            
            if is_repeated:
                return pattern
        
        if in_h % out_h == 0 and in_w == out_w:
            # Pattern répété verticalement
            repetitions = in_h // out_h
            
            # Extraire 1ère occurrence
            pattern = grid[:out_h, :]
            
            # Vérifier si pattern se répète
            is_repeated = True
            for i in range(1, repetitions):
                segment = grid[i*out_h:(i+1)*out_h, :]
                if not np.array_equal(pattern, segment):
                    is_repeated = False
                    break
            
            if is_repeated:
                return pattern
        
        return None
    
    def _extract_specific_zone(self, grid: np.ndarray, target_shape: Tuple[int, int],
                              train_pairs: List[Dict]) -> Optional[np.ndarray]:
        """
        Analyse train pairs pour détecter quelle zone est extraite
        
        Teste: top-left, top-right, bottom-left, bottom-right, center
        """
        out_h, out_w = target_shape
        in_h, in_w = grid.shape
        
        # Tester différentes zones
        zones = {
            'top_left': (0, 0),
            'top_right': (0, max(0, in_w - out_w)),
            'bottom_left': (max(0, in_h - out_h), 0),
            'bottom_right': (max(0, in_h - out_h), max(0, in_w - out_w)),
            'center': (max(0, (in_h - out_h) // 2), max(0, (in_w - out_w) // 2))
        }
        
        # Analyser train pairs pour identifier zone
        zone_scores = {zone: 0 for zone in zones}
        
        for pair in train_pairs:
            in_grid = np.array(pair['input'])
            out_grid = np.array(pair['output'])
            
            if out_grid.shape != target_shape:
                continue
            
            pair_in_h, pair_in_w = in_grid.shape
            
            # Tester chaque zone
            for zone_name, (start_h, start_w) in zones.items():
                # Adapter position pour cette paire
                if zone_name == 'top_left':
                    test_start_h, test_start_w = 0, 0
                elif zone_name == 'top_right':
                    test_start_h, test_start_w = 0, max(0, pair_in_w - out_w)
                elif zone_name == 'bottom_left':
                    test_start_h, test_start_w = max(0, pair_in_h - out_h), 0
                elif zone_name == 'bottom_right':
                    test_start_h, test_start_w = max(0, pair_in_h - out_h), max(0, pair_in_w - out_w)
                else:  # center
                    test_start_h = max(0, (pair_in_h - out_h) // 2)
                    test_start_w = max(0, (pair_in_w - out_w) // 2)
                
                # Extraire zone
                extracted = in_grid[test_start_h:test_start_h+out_h,
                                   test_start_w:test_start_w+out_w]
                
                # Comparer avec output
                if extracted.shape == out_grid.shape and np.array_equal(extracted, out_grid):
                    zone_scores[zone_name] += 1
        
        # Trouver zone avec meilleur score
        best_zone = max(zone_scores.keys(), key=lambda k: zone_scores[k])
        if zone_scores[best_zone] > 0:
            start_h, start_w = zones[best_zone]
            result = grid[start_h:start_h+out_h, start_w:start_w+out_w]
            
            # Padding si nécessaire
            if result.shape != target_shape:
                padded = np.zeros(target_shape, dtype=grid.dtype)
                padded[:result.shape[0], :result.shape[1]] = result
                return padded
            
            return result
        
        return None
    
    def _extract_center(self, grid: np.ndarray, target_shape: Tuple[int, int]) -> np.ndarray:
        """
        Extrait zone centrale (comportement original - fallback)
        """
        in_h, in_w = grid.shape
        out_h, out_w = target_shape
        
        # Extraire centre
        start_h = (in_h - out_h) // 2
        start_w = (in_w - out_w) // 2
        
        # Bounds checking
        start_h = max(0, start_h)
        start_w = max(0, start_w)
        end_h = min(in_h, start_h + out_h)
        end_w = min(in_w, start_w + out_w)
        
        result = grid[start_h:end_h, start_w:end_w]
        
        # Padding si nécessaire
        if result.shape != target_shape:
            padded = np.zeros(target_shape, dtype=grid.dtype)
            padded[:result.shape[0], :result.shape[1]] = result
            result = padded
        
        return result
    
    def apply_crop(self, grid: np.ndarray, target_shape: Tuple[int, int], 
                   train_pairs: List[Dict]) -> np.ndarray:
        """
        Applique crop en extrayant zone spécifique
        
        Args:
            grid: Grid input
            target_shape: Shape cible
            train_pairs: Paires pour détecter zone
            
        Returns:
            Grid croppé
        """
        # Analyser train pairs pour détecter zone à extraire
        # Utiliser downscaling intelligent avec train_pairs
        return self.apply_downscaling(grid, target_shape, train_pairs)
    
    def apply_extension(self, grid: np.ndarray, target_shape: Tuple[int, int], 
                       train_pairs: List[Dict]) -> np.ndarray:
        """
        Applique extension en ajoutant lignes/colonnes
        
        Args:
            grid: Grid input
            target_shape: Shape cible
            train_pairs: Paires pour détecter pattern extension
            
        Returns:
            Grid étendu
        """
        in_h, in_w = grid.shape
        out_h, out_w = target_shape
        
        # Créer grid étendu
        result = np.zeros(target_shape, dtype=grid.dtype)
        
        # Copier grid original
        result[:min(in_h, out_h), :min(in_w, out_w)] = grid[:min(in_h, out_h), :min(in_w, out_w)]
        
        # Analyser train pairs pour pattern extension
        if train_pairs:
            # Détecter si extension répète dernière ligne/colonne
            pair = train_pairs[0]
            in_grid = np.array(pair['input'])
            out_grid = np.array(pair['output'])
            
            # Si output plus grand, vérifier si répétition
            if out_grid.shape[0] > in_grid.shape[0]:
                # Extension verticale - répéter dernière ligne
                if in_h < out_h:
                    last_row = grid[-1, :]
                    for i in range(in_h, out_h):
                        result[i, :min(in_w, out_w)] = last_row[:min(in_w, out_w)]
            
            if out_grid.shape[1] > in_grid.shape[1]:
                # Extension horizontale - répéter dernière colonne
                if in_w < out_w:
                    last_col = grid[:, -1]
                    for j in range(in_w, out_w):
                        result[:min(in_h, out_h), j] = last_col[:min(in_h, out_h)]
        
        return result
    
    def apply_transformation(self, grid: np.ndarray, transform_type: str,
                            target_shape: Tuple[int, int],
                            train_pairs: List[Dict]) -> np.ndarray:
        """
        Applique la transformation appropriée
        
        Args:
            grid: Grid input
            transform_type: Type de transformation
            target_shape: Shape cible
            train_pairs: Paires d'entraînement
            
        Returns:
            Grid transformé
        """
        if transform_type == 'same':
            return grid.copy()
        elif transform_type == 'upscale':
            return self.apply_upscaling(grid, target_shape)
        elif transform_type == 'downscale':
            return self.apply_downscaling(grid, target_shape, train_pairs)
        elif transform_type == 'crop':
            return self.apply_crop(grid, target_shape, train_pairs)
        elif transform_type == 'extend':
            return self.apply_extension(grid, target_shape, train_pairs)
        else:
            return grid.copy()

# Made with Bob
