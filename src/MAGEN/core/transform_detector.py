"""
Transform Detector - Détection Transformations Réelles
=======================================================

Détecte les transformations entre input et output pour puzzles ARC-AGI-3.

Auteur: Bob (Mode Advanced)
Date: 2026-06-12
Protocole: Claude Pilot
"""

import numpy as np
from typing import List, Dict, Any, Tuple, Optional
from dataclasses import dataclass


@dataclass
class Transform:
    """Transformation détectée"""
    type: str
    confidence: float
    params: Dict[str, Any]


class TransformDetector:
    """Détecteur de transformations"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def detect_transforms(
        self,
        input_grid: np.ndarray,
        output_grid: np.ndarray
    ) -> List[Transform]:
        """
        Détecter transformations entre input et output
        
        Args:
            input_grid: Grille input
            output_grid: Grille output
            
        Returns:
            Liste transformations détectées
        """
        transforms = []
        
        # 1. Rotation
        rot_transform = self._detect_rotation(input_grid, output_grid)
        if rot_transform:
            transforms.append(rot_transform)
        
        # 2. Flip
        flip_transform = self._detect_flip(input_grid, output_grid)
        if flip_transform:
            transforms.append(flip_transform)
        
        # 3. Color change
        color_transform = self._detect_color_change(input_grid, output_grid)
        if color_transform:
            transforms.append(color_transform)
        
        # 4. Add pixels
        add_transform = self._detect_add_pixels(input_grid, output_grid)
        if add_transform:
            transforms.append(add_transform)
        
        if self.verbose:
            print(f"[DETECT] {len(transforms)} transformations détectées")
        
        return transforms
    
    def _detect_rotation(
        self,
        input_grid: np.ndarray,
        output_grid: np.ndarray
    ) -> Optional[Transform]:
        """Détecter rotation"""
        if input_grid.shape != output_grid.shape:
            return None
        
        # Tester rotations 90°, 180°, 270°
        for angle, k in [(90, 1), (180, 2), (270, 3)]:
            rotated = np.rot90(input_grid, k)
            if np.array_equal(rotated, output_grid):
                return Transform(
                    type="rotation",
                    confidence=1.0,
                    params={"angle": angle}
                )
        
        return None
    
    def _detect_flip(
        self,
        input_grid: np.ndarray,
        output_grid: np.ndarray
    ) -> Optional[Transform]:
        """Détecter flip horizontal/vertical"""
        if input_grid.shape != output_grid.shape:
            return None
        
        # Flip horizontal
        if np.array_equal(np.fliplr(input_grid), output_grid):
            return Transform(
                type="flip",
                confidence=1.0,
                params={"axis": "horizontal"}
            )
        
        # Flip vertical
        if np.array_equal(np.flipud(input_grid), output_grid):
            return Transform(
                type="flip",
                confidence=1.0,
                params={"axis": "vertical"}
            )
        
        return None
    
    def _detect_color_change(
        self,
        input_grid: np.ndarray,
        output_grid: np.ndarray
    ) -> Optional[Transform]:
        """Détecter changement couleurs"""
        if input_grid.shape != output_grid.shape:
            return None
        
        # Vérifier si structure identique mais couleurs différentes
        input_mask = input_grid > 0
        output_mask = output_grid > 0
        
        if not np.array_equal(input_mask, output_mask):
            return None
        
        # Mapper couleurs
        color_map = {}
        for i in range(input_grid.shape[0]):
            for j in range(input_grid.shape[1]):
                if input_grid[i, j] > 0:
                    in_color = input_grid[i, j]
                    out_color = output_grid[i, j]
                    if in_color in color_map:
                        if color_map[in_color] != out_color:
                            return None
                    else:
                        color_map[in_color] = out_color
        
        if color_map:
            return Transform(
                type="color_change",
                confidence=1.0,
                params={"color_map": color_map}
            )
        
        return None
    
    def _detect_add_pixels(
        self,
        input_grid: np.ndarray,
        output_grid: np.ndarray
    ) -> Optional[Transform]:
        """Détecter ajout de pixels"""
        if input_grid.shape != output_grid.shape:
            return None
        
        # Trouver pixels ajoutés
        diff = output_grid != input_grid
        added_pixels = []
        
        for i in range(output_grid.shape[0]):
            for j in range(output_grid.shape[1]):
                if diff[i, j] and output_grid[i, j] != 0:
                    added_pixels.append({
                        "pos": (i, j),
                        "color": int(output_grid[i, j])
                    })
        
        if added_pixels:
            return Transform(
                type="add_pixels",
                confidence=0.8,
                params={"pixels": added_pixels}
            )
        
        return None
    
    def apply_transform(
        self,
        grid: np.ndarray,
        transform: Transform
    ) -> np.ndarray:
        """
        Appliquer transformation à grille
        
        Args:
            grid: Grille input
            transform: Transformation à appliquer
            
        Returns:
            Grille transformée
        """
        result = grid.copy()
        
        if transform.type == "rotation":
            angle = transform.params["angle"]
            k = angle // 90
            result = np.rot90(result, k)
        
        elif transform.type == "flip":
            axis = transform.params["axis"]
            if axis == "horizontal":
                result = np.fliplr(result)
            else:
                result = np.flipud(result)
        
        elif transform.type == "color_change":
            color_map = transform.params["color_map"]
            for old_color, new_color in color_map.items():
                result[result == old_color] = new_color
        
        elif transform.type == "add_pixels":
            pixels = transform.params["pixels"]
            for pixel in pixels:
                i, j = pixel["pos"]
                result[i, j] = pixel["color"]
        
        return result


def test_detector():
    """Test détecteur"""
    print("\n" + "="*80)
    print("TEST TRANSFORM DETECTOR")
    print("="*80)
    
    detector = TransformDetector(verbose=True)
    
    # Test rotation 90°
    input_grid = np.array([[1, 0], [0, 0]])
    output_grid = np.array([[0, 1], [0, 0]])
    
    print("\nTest rotation 90°:")
    print(f"Input:\n{input_grid}")
    print(f"Output:\n{output_grid}")
    
    transforms = detector.detect_transforms(input_grid, output_grid)
    print(f"\nTransformations: {len(transforms)}")
    for t in transforms:
        print(f"  - {t.type}: {t.params} (confidence: {t.confidence})")
    
    # Appliquer
    if transforms:
        result = detector.apply_transform(input_grid, transforms[0])
        print(f"\nRésultat:\n{result}")
        print(f"Match: {np.array_equal(result, output_grid)}")
    
    print("\n" + "="*80)
    print("✓ TEST COMPLÉTÉ")
    print("="*80)


if __name__ == "__main__":
    test_detector()

# Made with Bob
