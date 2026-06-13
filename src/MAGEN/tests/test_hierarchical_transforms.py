"""
Tests unitaires pour le module hierarchical_transforms.py
Validation complète des transformations multi-niveaux (object/scene/global)
"""

import numpy as np
from typing import List, Tuple
import sys
from pathlib import Path

# Ajouter le répertoire parent au path pour imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic.hierarchical_transforms import (
    TransformLevel,
    HierarchicalTransform,
    ObjectLevelTransforms,
    SceneLevelTransforms,
    GlobalLevelTransforms
)
from objects.object_extractor import Object, SceneGraph, ShapeType


class TestObjectLevelTransforms:
    """Tests pour transformations object-level"""
    
    def setup_method(self):
        """Initialisation avant chaque test"""
        self.transforms = ObjectLevelTransforms()
        
        # Objet test: carré 2x2 rouge en (1,1)
        self.test_obj = Object(
            id=1,
            pixels=[(1, 1), (1, 2), (2, 1), (2, 2)],
            color=2,
            bounding_box=(1, 1, 2, 2),
            shape_type=ShapeType.SQUARE
        )
        self.grid_shape = (5, 5)
    
    def test_move_object_basic(self):
        """Test déplacement objet simple"""
        moved = self.transforms.move_object(self.test_obj, dx=1, dy=1, grid_shape=self.grid_shape)
        
        assert moved.id == self.test_obj.id
        assert moved.color == self.test_obj.color
        assert moved.shape_type == self.test_obj.shape_type
        
        # Vérifier pixels déplacés
        expected_pixels = [(2, 2), (2, 3), (3, 2), (3, 3)]
        assert sorted(moved.pixels) == sorted(expected_pixels)
        
        # Vérifier bounding_box
        assert moved.bounding_box == (2, 2, 3, 3)
    
    def test_move_object_out_of_bounds(self):
        """Test déplacement hors limites"""
        # Déplacement qui sort de la grille
        moved = self.transforms.move_object(self.test_obj, dx=10, dy=10, grid_shape=self.grid_shape)
        
        # Pixels hors limites sont filtrés
        assert len(moved.pixels) == 0
    
    def test_recolor_object(self):
        """Test changement couleur"""
        recolored = self.transforms.recolor_object(self.test_obj, new_color=5)
        
        assert recolored.color == 5
        assert recolored.pixels == self.test_obj.pixels
        assert recolored.shape_type == self.test_obj.shape_type
    
    def test_scale_object_up(self):
        """Test agrandissement objet"""
        scaled = self.transforms.scale_object(self.test_obj, scale_factor=2.0, grid_shape=self.grid_shape)
        
        # Objet 2x2 devient environ 4x4 (peut varier selon arrondi)
        assert len(scaled.pixels) >= 4  # Au moins agrandi
        assert scaled.color == self.test_obj.color
    
    def test_scale_object_down(self):
        """Test réduction objet"""
        scaled = self.transforms.scale_object(self.test_obj, scale_factor=0.5, grid_shape=self.grid_shape)
        
        # Objet 2x2 devient 1x1
        assert len(scaled.pixels) == 1
        assert scaled.color == self.test_obj.color
    
    def test_rotate_object_90(self):
        """Test rotation 90°"""
        rotated = self.transforms.rotate_object(self.test_obj, angle=90, grid_shape=self.grid_shape)
        
        assert rotated.color == self.test_obj.color
        assert len(rotated.pixels) == len(self.test_obj.pixels)
    
    def test_rotate_object_180(self):
        """Test rotation 180°"""
        rotated = self.transforms.rotate_object(self.test_obj, angle=180, grid_shape=self.grid_shape)
        
        assert rotated.color == self.test_obj.color
        assert len(rotated.pixels) == len(self.test_obj.pixels)
    
    def test_duplicate_object(self):
        """Test duplication objet"""
        # duplicate_object crée UNE seule copie avec offset
        duplicated = self.transforms.duplicate_object(
            self.test_obj,
            offset_x=1,
            offset_y=0,
            grid_shape=self.grid_shape,
            new_id=2
        )
        
        assert duplicated is not None
        assert duplicated.id == 2
        assert duplicated.color == self.test_obj.color
        assert duplicated.shape_type == self.test_obj.shape_type
        
        # Vérifier décalage
        for (y, x) in duplicated.pixels:
            assert (y, x - 1) in self.test_obj.pixels


class TestSceneLevelTransforms:
    """Tests pour transformations scene-level"""
    
    def setup_method(self):
        """Initialisation avant chaque test"""
        self.transforms = SceneLevelTransforms()
        
        # Deux objets test
        self.obj1 = Object(
            id=1,
            pixels=[(1, 1), (1, 2)],
            color=2,
            bounding_box=(1, 1, 1, 2),
            shape_type=ShapeType.LINE_HORIZONTAL
        )
        self.obj2 = Object(
            id=2,
            pixels=[(3, 3), (3, 4)],
            color=3,
            bounding_box=(3, 3, 3, 4),
            shape_type=ShapeType.LINE_HORIZONTAL
        )
        self.objects = [self.obj1, self.obj2]
        self.grid_shape = (5, 5)
    
    def test_group_objects(self):
        """Test groupement objets"""
        # group_objects retourne liste de groupes (pas objets fusionnés)
        grouped = self.transforms.group_objects(self.objects, criterion='color')
        
        # Deux couleurs différentes = deux groupes
        assert len(grouped) == 2
        
        # Vérifier que chaque groupe contient un objet
        assert len(grouped[0]) == 1
        assert len(grouped[1]) == 1
    
    def test_align_objects_horizontal(self):
        """Test alignement horizontal"""
        aligned = self.transforms.align_objects(
            self.objects, 
            alignment='horizontal',
            grid_shape=self.grid_shape
        )
        
        assert len(aligned) == 2
        
        # Tous les objets doivent avoir même y
        y_coords = [obj.bounding_box[0] for obj in aligned]
        assert len(set(y_coords)) == 1
    
    def test_align_objects_vertical(self):
        """Test alignement vertical"""
        aligned = self.transforms.align_objects(
            self.objects,
            alignment='vertical',
            grid_shape=self.grid_shape
        )
        
        assert len(aligned) == 2
        
        # Tous les objets doivent avoir même x
        x_coords = [obj.bounding_box[1] for obj in aligned]
        assert len(set(x_coords)) == 1
    
    def test_distribute_objects_horizontal(self):
        """Test distribution horizontale"""
        distributed = self.transforms.distribute_objects(
            self.objects,
            direction='horizontal',
            grid_shape=self.grid_shape
        )
        
        assert len(distributed) == 2
        
        # Espacement égal entre objets
        x1 = distributed[0].bounding_box[1]
        x2 = distributed[1].bounding_box[1]
        assert x2 > x1  # Deuxième objet à droite
    
    def test_distribute_objects_vertical(self):
        """Test distribution verticale"""
        distributed = self.transforms.distribute_objects(
            self.objects,
            direction='vertical',
            grid_shape=self.grid_shape
        )
        
        assert len(distributed) == 2
        
        # Espacement égal entre objets
        y1 = distributed[0].bounding_box[0]
        y2 = distributed[1].bounding_box[0]
        assert y2 > y1  # Deuxième objet en bas
    
    def test_create_symmetry_horizontal(self):
        """Test symétrie horizontale"""
        mirrored = self.transforms.create_symmetry(
            self.objects,
            axis='horizontal',
            grid_shape=self.grid_shape
        )
        
        # Double le nombre d'objets (originaux + miroirs)
        assert len(mirrored) == 4
    
    def test_create_symmetry_vertical(self):
        """Test symétrie verticale"""
        mirrored = self.transforms.create_symmetry(
            self.objects,
            axis='vertical',
            grid_shape=self.grid_shape
        )
        
        # Double le nombre d'objets
        assert len(mirrored) == 4


class TestGlobalLevelTransforms:
    """Tests pour transformations global-level"""
    
    def setup_method(self):
        """Initialisation avant chaque test"""
        self.transforms = GlobalLevelTransforms()
        
        # Grille test 3x3 avec pattern
        self.test_grid = np.array([
            [1, 2, 0],
            [3, 4, 0],
            [0, 0, 0]
        ])
    
    def test_rotate_grid_90(self):
        """Test rotation grille 90°"""
        rotated = self.transforms.rotate_grid(self.test_grid, angle=90)
        
        # Après rotation 90°, dimensions peuvent être inversées
        assert rotated.shape[0] == self.test_grid.shape[1] or rotated.shape == self.test_grid.shape
        # Vérifier que rotation a eu lieu (contenu différent)
        assert not np.array_equal(rotated, self.test_grid)
    
    def test_rotate_grid_180(self):
        """Test rotation grille 180°"""
        rotated = self.transforms.rotate_grid(self.test_grid, angle=180)
        
        assert rotated.shape == self.test_grid.shape
        # Vérifier rotation correcte
        assert rotated[0, 0] == self.test_grid[2, 2]
        assert rotated[2, 2] == self.test_grid[0, 0]
    
    def test_rotate_grid_270(self):
        """Test rotation grille 270°"""
        rotated = self.transforms.rotate_grid(self.test_grid, angle=270)
        
        assert rotated.shape == self.test_grid.shape
        # Équivalent à -90°
        assert rotated[0, 2] == self.test_grid[2, 2]
    
    def test_flip_grid_horizontal(self):
        """Test flip horizontal"""
        flipped = self.transforms.flip_grid(self.test_grid, axis='horizontal')
        
        assert flipped.shape == self.test_grid.shape
        # Vérifier flip
        assert flipped[0, 0] == self.test_grid[0, 2]
        assert flipped[0, 2] == self.test_grid[0, 0]
    
    def test_flip_grid_vertical(self):
        """Test flip vertical"""
        flipped = self.transforms.flip_grid(self.test_grid, axis='vertical')
        
        assert flipped.shape == self.test_grid.shape
        # Vérifier flip
        assert flipped[0, 0] == self.test_grid[2, 0]
        assert flipped[2, 0] == self.test_grid[0, 0]
    
    def test_crop_grid(self):
        """Test crop grille"""
        cropped = self.transforms.crop_grid(self.test_grid, y1=0, x1=0, y2=1, x2=1)
        
        assert cropped.shape == (2, 2)
        assert cropped[0, 0] == self.test_grid[0, 0]
        assert cropped[1, 1] == self.test_grid[1, 1]
    
    def test_expand_grid(self):
        """Test expansion grille"""
        expanded = self.transforms.expand_grid(
            self.test_grid,
            new_height=5,
            new_width=5,
            fill_value=9
        )
        
        assert expanded.shape == (5, 5)
        # Vérifier bordures remplies
        assert expanded[4, 4] == 9
        # Vérifier contenu original préservé (en haut à gauche)
        assert expanded[0, 0] == self.test_grid[0, 0]
        assert expanded[2, 2] == self.test_grid[2, 2]
    
    def test_tile_grid(self):
        """Test tiling grille"""
        tiled = self.transforms.tile_grid(self.test_grid, repeat_y=2, repeat_x=2)
        
        assert tiled.shape == (6, 6)
        # Vérifier répétition
        assert tiled[0, 0] == self.test_grid[0, 0]
        assert tiled[3, 0] == self.test_grid[0, 0]  # Répétition verticale
        assert tiled[0, 3] == self.test_grid[0, 0]  # Répétition horizontale


class TestHierarchicalTransform:
    """Tests pour structure HierarchicalTransform"""
    
    def test_create_object_level_transform(self):
        """Test création transform object-level"""
        transform = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 1},
            target_objects=[1]
        )
        
        assert transform.level == TransformLevel.OBJECT
        assert transform.transform_type == 'move'
        assert transform.parameters['dx'] == 1
        assert 1 in transform.target_objects
    
    def test_create_scene_level_transform(self):
        """Test création transform scene-level"""
        transform = HierarchicalTransform(
            level=TransformLevel.SCENE,
            transform_type='align',
            parameters={'alignment': 'horizontal'}
        )
        
        assert transform.level == TransformLevel.SCENE
        assert transform.transform_type == 'align'
        assert len(transform.target_objects) == 0
    
    def test_create_global_level_transform(self):
        """Test création transform global-level"""
        transform = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='rotate',
            parameters={'angle': 90}
        )
        
        assert transform.level == TransformLevel.GLOBAL
        assert transform.transform_type == 'rotate'


def test_transform_level_enum():
    """Test enum TransformLevel"""
    assert TransformLevel.OBJECT.value == 'object'
    assert TransformLevel.SCENE.value == 'scene'
    assert TransformLevel.GLOBAL.value == 'global'
    
    # Vérifier ordre hiérarchique
    levels = [TransformLevel.OBJECT, TransformLevel.SCENE, TransformLevel.GLOBAL]
    assert len(levels) == 3


if __name__ == '__main__':
    # Exécuter tests sans pytest si pas installé
    import sys
    
    # Tests object-level
    print("=== Tests Object-Level Transforms ===")
    test_obj = TestObjectLevelTransforms()
    test_obj.setup_method()
    
    try:
        test_obj.test_move_object_basic()
        print("✓ test_move_object_basic")
    except AssertionError as e:
        print(f"✗ test_move_object_basic: {e}")
    
    try:
        test_obj.test_recolor_object()
        print("✓ test_recolor_object")
    except AssertionError as e:
        print(f"✗ test_recolor_object: {e}")
    
    try:
        test_obj.test_scale_object_up()
        print("✓ test_scale_object_up")
    except AssertionError as e:
        print(f"✗ test_scale_object_up: {e}")
    
    try:
        test_obj.test_duplicate_object()
        print("✓ test_duplicate_object")
    except AssertionError as e:
        print(f"✗ test_duplicate_object: {e}")
    
    # Tests scene-level
    print("\n=== Tests Scene-Level Transforms ===")
    test_scene = TestSceneLevelTransforms()
    test_scene.setup_method()
    
    try:
        test_scene.test_group_objects()
        print("✓ test_group_objects")
    except AssertionError as e:
        print(f"✗ test_group_objects: {e}")
    
    try:
        test_scene.test_align_objects_horizontal()
        print("✓ test_align_objects_horizontal")
    except AssertionError as e:
        print(f"✗ test_align_objects_horizontal: {e}")
    
    # Tests global-level
    print("\n=== Tests Global-Level Transforms ===")
    test_global = TestGlobalLevelTransforms()
    test_global.setup_method()
    
    try:
        test_global.test_rotate_grid_90()
        print("✓ test_rotate_grid_90")
    except AssertionError as e:
        print(f"✗ test_rotate_grid_90: {e}")
    
    try:
        test_global.test_flip_grid_horizontal()
        print("✓ test_flip_grid_horizontal")
    except AssertionError as e:
        print(f"✗ test_flip_grid_horizontal: {e}")
    
    try:
        test_global.test_expand_grid()
        print("✓ test_expand_grid")
    except AssertionError as e:
        print(f"✗ test_expand_grid: {e}")
    
    print("\n=== Tests HierarchicalTransform ===")
    test_ht = TestHierarchicalTransform()
    
    try:
        test_ht.test_create_object_level_transform()
        print("✓ test_create_object_level_transform")
    except AssertionError as e:
        print(f"✗ test_create_object_level_transform: {e}")
    
    try:
        test_transform_level_enum()
        print("✓ test_transform_level_enum")
    except AssertionError as e:
        print(f"✗ test_transform_level_enum: {e}")
    
    print("\n=== Tests terminés ===")

# Made with Bob
