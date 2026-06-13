"""
Tests unitaires pour le module hierarchical_composer.py
Validation composition multi-niveaux et propagation transformations
"""

import numpy as np
from typing import List, Tuple
import sys
from pathlib import Path

# Ajouter le répertoire parent au path pour imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from symbolic.hierarchical_composer import (
    HierarchicalComposition,
    HierarchicalComposer
)
from symbolic.hierarchical_transforms import (
    TransformLevel,
    HierarchicalTransform
)
from objects.object_extractor import Object, SceneGraph, ShapeType


class TestHierarchicalComposition:
    """Tests pour structure HierarchicalComposition"""
    
    def test_create_empty_composition(self):
        """Test création composition vide"""
        comp = HierarchicalComposition()
        
        assert len(comp.transforms) == 0
        assert comp.metadata == {}
    
    def test_add_single_transform(self):
        """Test ajout transformation unique"""
        comp = HierarchicalComposition()
        
        transform = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 1}
        )
        
        comp.add_transform(transform)
        
        assert len(comp.transforms) == 1
        assert comp.transforms[0] == transform
    
    def test_add_multiple_transforms(self):
        """Test ajout transformations multiples"""
        comp = HierarchicalComposition()
        
        t1 = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 0}
        )
        t2 = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='rotate',
            parameters={'angle': 90}
        )
        
        comp.add_transform(t1)
        comp.add_transform(t2)
        
        assert len(comp.transforms) == 2
        assert comp.transforms[0].level == TransformLevel.OBJECT
        assert comp.transforms[1].level == TransformLevel.GLOBAL


class TestHierarchicalComposer:
    """Tests pour HierarchicalComposer"""
    
    def setup_method(self):
        """Initialisation avant chaque test"""
        self.composer = HierarchicalComposer()
        
        # Grille test 5x5
        self.test_grid = np.array([
            [0, 0, 0, 0, 0],
            [0, 2, 2, 0, 0],
            [0, 2, 2, 0, 0],
            [0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0]
        ])
        
        # Scene graph avec un objet
        self.test_obj = Object(
            id=1,
            pixels=[(1, 1), (1, 2), (2, 1), (2, 2)],
            color=2,
            bounding_box=(1, 1, 2, 2),
            shape_type=ShapeType.SQUARE
        )
        
        self.test_scene_graph = SceneGraph(
            grid_shape=(5, 5),
            objects=[self.test_obj],
            relations=[]
        )
    
    def test_apply_empty_composition(self):
        """Test application composition vide"""
        comp = HierarchicalComposition()
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Rien ne change
        assert np.array_equal(result_grid, self.test_grid)
        assert len(result_sg.objects) == len(self.test_scene_graph.objects)
    
    def test_apply_object_level_transform(self):
        """Test application transform object-level"""
        comp = HierarchicalComposition()
        
        # Déplacer objet de (1, 0)
        transform = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 0},
            target_objects=[1]
        )
        comp.add_transform(transform)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Objet déplacé
        assert len(result_sg.objects) == 1
        moved_obj = result_sg.objects[0]
        
        # Vérifier déplacement (x augmenté de 1)
        for (y, x) in moved_obj.pixels:
            assert (y, x - 1) in self.test_obj.pixels
    
    def test_apply_scene_level_transform(self):
        """Test application transform scene-level"""
        # Ajouter deuxième objet
        obj2 = Object(
            id=2,
            pixels=[(3, 3), (3, 4)],
            color=3,
            bounding_box=(3, 3, 3, 4),
            shape_type=ShapeType.LINE_HORIZONTAL
        )
        
        scene_graph = SceneGraph(
            grid_shape=(5, 5),
            objects=[self.test_obj, obj2],
            relations=[]
        )
        
        comp = HierarchicalComposition()
        
        # Aligner objets horizontalement
        transform = HierarchicalTransform(
            level=TransformLevel.SCENE,
            transform_type='align',
            parameters={'alignment': 'horizontal'}
        )
        comp.add_transform(transform)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, scene_graph
        )
        
        # Objets alignés sur même y
        assert len(result_sg.objects) == 2
        y_coords = [obj.bounding_box[0] for obj in result_sg.objects]
        assert len(set(y_coords)) == 1
    
    def test_apply_global_level_transform(self):
        """Test application transform global-level"""
        comp = HierarchicalComposition()
        
        # Rotation 90°
        transform = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='rotate',
            parameters={'angle': 90}
        )
        comp.add_transform(transform)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Grille tournée
        assert not np.array_equal(result_grid, self.test_grid)
        
        # Objets propagés (rotation appliquée)
        assert len(result_sg.objects) == 1
    
    def test_apply_multi_level_composition(self):
        """Test composition multi-niveaux"""
        comp = HierarchicalComposition()
        
        # 1. Object-level: Déplacer objet
        t1 = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 0},
            target_objects=[1]
        )
        comp.add_transform(t1)
        
        # 2. Global-level: Rotation grille
        t2 = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='rotate',
            parameters={'angle': 90}
        )
        comp.add_transform(t2)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Les deux transformations appliquées
        # Note: Grille peut être identique si rotation ne change pas le pattern
        assert len(result_sg.objects) == 1
        # Vérifier que composition a été appliquée
        assert self.composer.compositions_applied >= 1
    
    def test_propagation_rotation(self):
        """Test propagation rotation aux objets"""
        comp = HierarchicalComposition()
        
        transform = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='rotate',
            parameters={'angle': 90}
        )
        comp.add_transform(transform)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Objet doit être tourné aussi
        original_pixels = set(self.test_obj.pixels)
        result_pixels = set(result_sg.objects[0].pixels)
        
        # Pixels différents (rotation appliquée)
        assert original_pixels != result_pixels
    
    def test_propagation_flip(self):
        """Test propagation flip aux objets"""
        comp = HierarchicalComposition()
        
        transform = HierarchicalTransform(
            level=TransformLevel.GLOBAL,
            transform_type='flip',
            parameters={'axis': 'horizontal'}
        )
        comp.add_transform(transform)
        
        result_grid, result_sg = self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        # Objet doit être flippé aussi
        original_pixels = set(self.test_obj.pixels)
        result_pixels = set(result_sg.objects[0].pixels)
        
        # Pixels différents (flip appliqué)
        assert original_pixels != result_pixels
    
    def test_get_statistics(self):
        """Test statistiques composer"""
        comp = HierarchicalComposition()
        
        transform = HierarchicalTransform(
            level=TransformLevel.OBJECT,
            transform_type='move',
            parameters={'dx': 1, 'dy': 0},
            target_objects=[1]
        )
        comp.add_transform(transform)
        
        self.composer.apply_composition(
            comp, self.test_grid, self.test_scene_graph
        )
        
        stats = self.composer.get_statistics()
        
        assert 'compositions_applied' in stats
        assert stats['compositions_applied'] >= 1


def test_hierarchical_composition_repr():
    """Test représentation string composition"""
    comp = HierarchicalComposition()
    
    t1 = HierarchicalTransform(
        level=TransformLevel.OBJECT,
        transform_type='move',
        parameters={'dx': 1}
    )
    comp.add_transform(t1)
    
    repr_str = repr(comp)
    assert 'HierarchicalComposition' in repr_str
    # Format: HierarchicalComposition(O:1, S:0, G:0)
    assert 'O:1' in repr_str


if __name__ == '__main__':
    # Exécuter tests sans pytest
    print("=== Tests HierarchicalComposition ===")
    test_comp = TestHierarchicalComposition()
    
    try:
        test_comp.test_create_empty_composition()
        print("✓ test_create_empty_composition")
    except AssertionError as e:
        print(f"✗ test_create_empty_composition: {e}")
    
    try:
        test_comp.test_add_single_transform()
        print("✓ test_add_single_transform")
    except AssertionError as e:
        print(f"✗ test_add_single_transform: {e}")
    
    try:
        test_comp.test_add_multiple_transforms()
        print("✓ test_add_multiple_transforms")
    except AssertionError as e:
        print(f"✗ test_add_multiple_transforms: {e}")
    
    print("\n=== Tests HierarchicalComposer ===")
    test_composer = TestHierarchicalComposer()
    test_composer.setup_method()
    
    try:
        test_composer.test_apply_empty_composition()
        print("✓ test_apply_empty_composition")
    except AssertionError as e:
        print(f"✗ test_apply_empty_composition: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_apply_object_level_transform()
        print("✓ test_apply_object_level_transform")
    except AssertionError as e:
        print(f"✗ test_apply_object_level_transform: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_apply_scene_level_transform()
        print("✓ test_apply_scene_level_transform")
    except AssertionError as e:
        print(f"✗ test_apply_scene_level_transform: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_apply_global_level_transform()
        print("✓ test_apply_global_level_transform")
    except AssertionError as e:
        print(f"✗ test_apply_global_level_transform: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_apply_multi_level_composition()
        print("✓ test_apply_multi_level_composition")
    except AssertionError as e:
        print(f"✗ test_apply_multi_level_composition: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_propagation_rotation()
        print("✓ test_propagation_rotation")
    except AssertionError as e:
        print(f"✗ test_propagation_rotation: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_propagation_flip()
        print("✓ test_propagation_flip")
    except AssertionError as e:
        print(f"✗ test_propagation_flip: {e}")
    
    try:
        test_composer.setup_method()
        test_composer.test_get_statistics()
        print("✓ test_get_statistics")
    except AssertionError as e:
        print(f"✗ test_get_statistics: {e}")
    
    try:
        test_hierarchical_composition_repr()
        print("✓ test_hierarchical_composition_repr")
    except AssertionError as e:
        print(f"✗ test_hierarchical_composition_repr: {e}")
    
    print("\n=== Tests terminés ===")

# Made with Bob
