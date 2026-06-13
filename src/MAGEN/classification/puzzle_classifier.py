"""
MAGEN Puzzle Classifier - Classification pré-synthesis

Classifie les puzzles AVANT génération de programmes pour router
vers des synthesizers spécialisés et prioriser les bonnes primitives.
"""

import numpy as np
from typing import List, Tuple, Dict, Set, Any
from dataclasses import dataclass
from enum import Enum


class TransformationClass(Enum):
    """Classes de transformations ARC"""
    GEOMETRIC = "geometric"              # rotate, mirror, transpose
    PERIODIC = "periodic"                # compression, répétition
    OBJECT_RELATIONAL = "object_relational"  # formes, relations
    TOPOLOGICAL = "topological"          # connectivité, frontières
    SEMANTIC_REGION = "semantic_region"  # sélection de régions
    COUNTING = "counting"                # ajout/suppression pixels
    PROPAGATION = "propagation"          # diffusion, spreading
    SYMMETRY = "symmetry"                # complétion symétrie
    UNKNOWN = "unknown"                  # non classifié


@dataclass
class ClassificationResult:
    """Résultat de classification"""
    primary_class: TransformationClass
    confidence: float
    secondary_classes: List[Tuple[TransformationClass, float]]
    features: Dict[str, Any]


class PuzzleClassifier:
    """
    Classifieur de puzzles ARC
    
    Analyse les train pairs pour déterminer le type de transformation
    avant de lancer la synthesis de programmes.
    """
    
    def __init__(self):
        """Initialise le classifieur"""
        self.transformation_features = {
            TransformationClass.GEOMETRIC: self._check_geometric,
            TransformationClass.PERIODIC: self._check_periodic,
            TransformationClass.OBJECT_RELATIONAL: self._check_object_relational,
            TransformationClass.SEMANTIC_REGION: self._check_semantic_region,
            TransformationClass.TOPOLOGICAL: self._check_topological,
            TransformationClass.COUNTING: self._check_counting,
            TransformationClass.PROPAGATION: self._check_propagation,
            TransformationClass.SYMMETRY: self._check_symmetry,
        }
    
    def classify(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> ClassificationResult:
        """
        Classifie un puzzle basé sur ses train pairs
        
        Args:
            train_pairs: Liste de (input, output) pairs
            
        Returns:
            ClassificationResult avec classe primaire et features
        """
        if not train_pairs:
            return ClassificationResult(
                primary_class=TransformationClass.UNKNOWN,
                confidence=0.0,
                secondary_classes=[],
                features={}
            )
        
        # Calculer scores pour chaque classe
        class_scores = {}
        all_features = {}
        
        for trans_class, check_func in self.transformation_features.items():
            score, features = check_func(train_pairs)
            class_scores[trans_class] = score
            all_features[trans_class.value] = features
        
        # Trier par score
        sorted_classes = sorted(class_scores.items(), key=lambda x: x[1], reverse=True)
        
        # Classe primaire
        primary_class, primary_score = sorted_classes[0]
        
        # Classes secondaires (score > 0.3)
        secondary_classes = [
            (cls, score) for cls, score in sorted_classes[1:]
            if score > 0.3
        ]
        
        # PRIORITÉ SPÉCIALE: Si periodic détecté avec compression_ratio, prioriser
        periodic_score = class_scores.get(TransformationClass.PERIODIC, 0.0)
        periodic_features = all_features.get('periodic', {})
        if periodic_score > 0.5 and periodic_features.get('compression_ratio', 1.0) > 1.0:
            # Periodic avec compression détectée → priorité absolue
            primary_class = TransformationClass.PERIODIC
            primary_score = periodic_score
            # Recalculer secondaires
            secondary_classes = [
                (cls, score) for cls, score in sorted_classes
                if cls != TransformationClass.PERIODIC and score > 0.3
            ]
        
        # Si score primaire trop faible, marquer comme UNKNOWN
        elif primary_score < 0.4:
            primary_class = TransformationClass.UNKNOWN
        
        return ClassificationResult(
            primary_class=primary_class,
            confidence=primary_score,
            secondary_classes=secondary_classes,
            features=all_features
        )
    
    def _check_geometric(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie transformations géométriques"""
        score = 0.0
        features = {
            'same_shape': True,
            'rotation_detected': False,
            'mirror_detected': False,
            'transpose_detected': False,
            'scale_detected': False,
            'scale_factor': 1.0
        }
        
        for inp, out in train_pairs:
            # Vérifier scale (changement de taille proportionnel)
            if inp.shape != out.shape:
                h_in, w_in = inp.shape
                h_out, w_out = out.shape
                
                # Scale up (agrandissement)
                if h_out > h_in and w_out > w_in:
                    h_ratio = h_out / h_in
                    w_ratio = w_out / w_in
                    if h_ratio == w_ratio and h_ratio == int(h_ratio):
                        features['scale_detected'] = True
                        features['scale_factor'] = h_ratio
                        features['same_shape'] = False
                        score += 1.0
                        continue
                
                # Scale down (réduction)
                elif h_out < h_in and w_out < w_in:
                    h_ratio = h_in / h_out
                    w_ratio = w_in / w_out
                    if h_ratio == w_ratio and h_ratio == int(h_ratio):
                        features['scale_detected'] = True
                        features['scale_factor'] = 1.0 / h_ratio
                        features['same_shape'] = False
                        score += 0.8  # Légèrement moins de confiance pour downscale
                        continue
                
                features['same_shape'] = False
                continue
            
            # Tester rotations
            if np.array_equal(np.rot90(inp, 1), out):
                features['rotation_detected'] = True
                score += 1.0
            elif np.array_equal(np.rot90(inp, 2), out):
                features['rotation_detected'] = True
                score += 1.0
            elif np.array_equal(np.rot90(inp, 3), out):
                features['rotation_detected'] = True
                score += 1.0
            # Tester miroirs
            elif np.array_equal(np.fliplr(inp), out):
                features['mirror_detected'] = True
                score += 1.0
            elif np.array_equal(np.flipud(inp), out):
                features['mirror_detected'] = True
                score += 1.0
            # Tester transpose
            elif np.array_equal(inp.T, out):
                features['transpose_detected'] = True
                score += 1.0
        
        # Normaliser par nombre de pairs
        score = score / len(train_pairs) if train_pairs else 0.0
        
        return score, features
    
    def _check_periodic(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie patterns périodiques"""
        score = 0.0
        features = {
            'horizontal_repetition': False,
            'vertical_repetition': False,
            'compression_ratio': 1.0
        }
        
        for inp, out in train_pairs:
            h_in, w_in = inp.shape
            h_out, w_out = out.shape
            
            # Vérifier compression horizontale
            if h_in == h_out and w_in > w_out and w_in % w_out == 0:
                ratio = w_in / w_out
                # Vérifier si input est répétition de output
                is_repeated = True
                for i in range(h_in):
                    for rep in range(int(ratio)):
                        for j in range(w_out):
                            if inp[i, rep * w_out + j] != out[i, j]:
                                is_repeated = False
                                break
                        if not is_repeated:
                            break
                    if not is_repeated:
                        break
                
                if is_repeated:
                    features['horizontal_repetition'] = True
                    features['compression_ratio'] = ratio
                    score += 1.0
            
            # Vérifier compression verticale
            elif w_in == w_out and h_in > h_out and h_in % h_out == 0:
                ratio = h_in / h_out
                is_repeated = True
                for rep in range(int(ratio)):
                    for i in range(h_out):
                        if not np.array_equal(inp[rep * h_out + i], out[i]):
                            is_repeated = False
                            break
                    if not is_repeated:
                        break
                
                if is_repeated:
                    features['vertical_repetition'] = True
                    features['compression_ratio'] = ratio
                    score += 1.0
        
        score = score / len(train_pairs) if train_pairs else 0.0
        return score, features
    
    def _check_object_relational(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie raisonnement relationnel sur objets"""
        score = 0.0
        features = {
            'color_added': False,
            'color_removed': False,
            'same_shape': True,
            'new_colors': []
        }
        
        for inp, out in train_pairs:
            if inp.shape != out.shape:
                features['same_shape'] = False
                continue
            
            # Vérifier ajout de couleurs
            inp_colors = set(inp.flatten())
            out_colors = set(out.flatten())
            
            new_colors = out_colors - inp_colors
            removed_colors = inp_colors - out_colors
            
            if new_colors:
                features['color_added'] = True
                features['new_colors'].extend(list(new_colors))
                score += 0.8
            
            if removed_colors:
                features['color_removed'] = True
                score += 0.5
        
        score = score / len(train_pairs) if train_pairs else 0.0
        return score, features
    
    def _check_semantic_region(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie sélection de régions sémantiques"""
        score = 0.0
        features = {
            'size_reduction': False,
            'ratio_h': 1.0,
            'ratio_w': 1.0,
            'is_crop': False
        }
        
        for inp, out in train_pairs:
            h_in, w_in = inp.shape
            h_out, w_out = out.shape
            
            # Vérifier réduction de taille
            if h_out < h_in or w_out < w_in:
                features['size_reduction'] = True
                features['ratio_h'] = h_out / h_in
                features['ratio_w'] = w_out / w_in
                
                # Vérifier si output est sous-région de input
                is_subregion = False
                for i in range(h_in - h_out + 1):
                    for j in range(w_in - w_out + 1):
                        if np.array_equal(inp[i:i+h_out, j:j+w_out], out):
                            is_subregion = True
                            features['is_crop'] = True
                            break
                    if is_subregion:
                        break
                
                if is_subregion:
                    score += 1.0
                else:
                    # Downscale possible
                    score += 0.6
        
        score = score / len(train_pairs) if train_pairs else 0.0
        return score, features
    
    def _check_topological(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie transformations topologiques"""
        # TODO: Implémenter détection connectivité, frontières
        return 0.0, {}
    
    def _check_counting(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie opérations de comptage"""
        # TODO: Implémenter détection ajout/suppression pixels
        return 0.0, {}
    
    def _check_propagation(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie propagation cellulaire"""
        # TODO: Implémenter détection diffusion, spreading
        return 0.0, {}
    
    def _check_symmetry(self, train_pairs: List[Tuple]) -> Tuple[float, Dict]:
        """Vérifie complétion de symétrie"""
        # TODO: Implémenter détection symétrie
        return 0.0, {}


# Export
__all__ = ['PuzzleClassifier', 'TransformationClass', 'ClassificationResult']

# Made with Bob
