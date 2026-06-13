"""
MAGEN - Tests Unitaires Module Objects
Validation extraction d'objets et scene graphs

Tests:
1. Extraction d'objets simples
2. Détection de shapes
3. Construction de scene graphs
4. Détection de relations
5. Transformations d'objets
6. Cas réels ARC-like
"""

import numpy as np
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from objects import (
    ObjectExtractor,
    ObjectTransformer,
    ShapeType,
    RelationType,
    ObjectTransformation
)


class TestObjectsModule:
    """Suite de tests pour le module objects"""
    
    def __init__(self):
        self.extractor = ObjectExtractor(connectivity=4, min_object_size=1)
        self.transformer = ObjectTransformer()
        
        self.tests_passed = 0
        self.tests_failed = 0
        self.tests_total = 0
    
    def assert_equal(self, actual, expected, test_name: str):
        """Assert avec logging"""
        self.tests_total += 1
        if actual == expected:
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            print(f"  Expected: {expected}")
            print(f"  Got: {actual}")
            self.tests_failed += 1
            return False
    
    def assert_true(self, condition: bool, test_name: str):
        """Assert boolean avec logging"""
        self.tests_total += 1
        if condition:
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            self.tests_failed += 1
            return False
    
    # ========== TESTS EXTRACTION OBJETS ==========
    
    def test_extract_single_object(self):
        """Test extraction objet unique"""
        grid = np.array([
            [0, 0, 0, 0],
            [0, 1, 1, 0],
            [0, 1, 1, 0],
            [0, 0, 0, 0]
        ])
        
        objects = self.extractor.extract_objects(grid, background=0)
        self.assert_equal(len(objects), 1, "Extract single object - count")
        
        if len(objects) == 1:
            obj = objects[0]
            self.assert_equal(obj.color, 1, "Extract single object - color")
            self.assert_equal(len(obj.pixels), 4, "Extract single object - size")
    
    def test_extract_multiple_objects(self):
        """Test extraction objets multiples"""
        grid = np.array([
            [1, 1, 0, 2, 2],
            [1, 1, 0, 2, 2],
            [0, 0, 0, 0, 0],
            [3, 3, 0, 4, 4],
            [3, 3, 0, 4, 4]
        ])
        
        objects = self.extractor.extract_objects(grid, background=0)
        self.assert_equal(len(objects), 4, "Extract multiple objects")
        
        colors = sorted([obj.color for obj in objects])
        self.assert_equal(colors, [1, 2, 3, 4], "Extract multiple objects - colors")
    
    def test_extract_with_connectivity(self):
        """Test extraction avec connectivité"""
        grid = np.array([
            [1, 0, 1],
            [0, 1, 0],
            [1, 0, 1]
        ])
        
        # 4-connectivity: 5 objets séparés
        extractor_4 = ObjectExtractor(connectivity=4)
        objects_4 = extractor_4.extract_objects(grid, background=0)
        self.assert_equal(len(objects_4), 5, "4-connectivity: 5 objects")
        
        # 8-connectivity: 1 objet connecté
        extractor_8 = ObjectExtractor(connectivity=8)
        objects_8 = extractor_8.extract_objects(grid, background=0)
        self.assert_equal(len(objects_8), 1, "8-connectivity: 1 object")
    
    # ========== TESTS DÉTECTION SHAPES ==========
    
    def test_detect_point(self):
        """Test détection point"""
        grid = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.POINT, "Detect point")
    
    def test_detect_line_horizontal(self):
        """Test détection ligne horizontale"""
        grid = np.array([[0, 0, 0, 0], [1, 1, 1, 1], [0, 0, 0, 0]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.LINE_HORIZONTAL, "Detect horizontal line")
    
    def test_detect_line_vertical(self):
        """Test détection ligne verticale"""
        grid = np.array([[0, 1, 0], [0, 1, 0], [0, 1, 0], [0, 1, 0]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.LINE_VERTICAL, "Detect vertical line")
    
    def test_detect_rectangle(self):
        """Test détection rectangle"""
        grid = np.array([
            [0, 0, 0, 0, 0],
            [0, 1, 1, 1, 0],
            [0, 1, 1, 1, 0],
            [0, 0, 0, 0, 0]
        ])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.RECTANGLE, "Detect rectangle")
    
    def test_detect_square(self):
        """Test détection carré"""
        grid = np.array([
            [0, 0, 0, 0],
            [0, 1, 1, 0],
            [0, 1, 1, 0],
            [0, 0, 0, 0]
        ])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.SQUARE, "Detect square")
    
    def test_detect_hollow_rectangle(self):
        """Test détection rectangle creux"""
        grid = np.array([
            [1, 1, 1, 1],
            [1, 0, 0, 1],
            [1, 0, 0, 1],
            [1, 1, 1, 1]
        ])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            self.assert_equal(objects[0].shape_type, ShapeType.HOLLOW_RECTANGLE, "Detect hollow rectangle")
    
    # ========== TESTS SCENE GRAPH ==========
    
    def test_build_scene_graph(self):
        """Test construction scene graph"""
        grid = np.array([
            [1, 1, 0, 2, 2],
            [1, 1, 0, 2, 2],
            [0, 0, 0, 0, 0],
            [3, 3, 0, 4, 4],
            [3, 3, 0, 4, 4]
        ])
        
        sg = self.extractor.build_scene_graph(grid, background=0)
        
        self.assert_equal(len(sg.objects), 4, "Scene graph - objects count")
        self.assert_true(len(sg.relations) > 0, "Scene graph - has relations")
        self.assert_equal(sg.properties['num_colors'], 4, "Scene graph - num colors")
    
    def test_detect_above_below_relations(self):
        """Test détection relations above/below"""
        grid = np.array([
            [1, 1, 0],
            [0, 0, 0],
            [2, 2, 0]
        ])
        
        sg = self.extractor.build_scene_graph(grid, background=0)
        
        # Vérifier relation ABOVE
        above_relations = [r for r in sg.relations if r.type == RelationType.ABOVE]
        self.assert_true(len(above_relations) > 0, "Detect ABOVE relation")
        
        # Vérifier relation BELOW
        below_relations = [r for r in sg.relations if r.type == RelationType.BELOW]
        self.assert_true(len(below_relations) > 0, "Detect BELOW relation")
    
    def test_detect_left_right_relations(self):
        """Test détection relations left/right"""
        grid = np.array([
            [1, 0, 2],
            [1, 0, 2]
        ])
        
        sg = self.extractor.build_scene_graph(grid, background=0)
        
        # Vérifier relation LEFT_OF
        left_relations = [r for r in sg.relations if r.type == RelationType.LEFT_OF]
        self.assert_true(len(left_relations) > 0, "Detect LEFT_OF relation")
        
        # Vérifier relation RIGHT_OF
        right_relations = [r for r in sg.relations if r.type == RelationType.RIGHT_OF]
        self.assert_true(len(right_relations) > 0, "Detect RIGHT_OF relation")
    
    def test_detect_same_row_column(self):
        """Test détection same row/column"""
        grid = np.array([
            [1, 0, 2, 0, 3],
            [0, 0, 0, 0, 0],
            [4, 0, 5, 0, 6]
        ])
        
        sg = self.extractor.build_scene_graph(grid, background=0)
        
        # Vérifier SAME_ROW
        same_row = [r for r in sg.relations if r.type == RelationType.SAME_ROW]
        self.assert_true(len(same_row) > 0, "Detect SAME_ROW relation")
    
    # ========== TESTS TRANSFORMATIONS ==========
    
    def test_move_object(self):
        """Test déplacement objet"""
        grid = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            obj = objects[0]
            moved = self.transformer.move_object(obj, dx=1, dy=1)
            
            # Vérifier nouveau centre
            expected_center = (2.0, 2.0)
            self.assert_equal(moved.properties['center'], expected_center, "Move object - center")
    
    def test_recolor_object(self):
        """Test recoloration objet"""
        grid = np.array([[1, 1], [1, 1]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            obj = objects[0]
            recolored = self.transformer.recolor_object(obj, new_color=5)
            
            self.assert_equal(recolored.color, 5, "Recolor object")
    
    def test_scale_object(self):
        """Test scaling objet"""
        grid = np.array([[1, 1], [1, 1]])
        objects = self.extractor.extract_objects(grid, background=0)
        
        if len(objects) == 1:
            obj = objects[0]
            scaled = self.transformer.scale_object(obj, factor=2)
            
            # Taille devrait être 4x plus grande
            self.assert_equal(scaled.properties['size'], 16, "Scale object - size")
    
    def test_scene_graph_to_grid(self):
        """Test reconstruction grille depuis scene graph"""
        original_grid = np.array([
            [1, 1, 0, 2, 2],
            [1, 1, 0, 2, 2]
        ])
        
        sg = self.extractor.build_scene_graph(original_grid, background=0)
        reconstructed = self.transformer.scene_graph_to_grid(sg)
        
        self.assert_true(np.array_equal(original_grid, reconstructed), "Scene graph to grid")
    
    # ========== TESTS CAS RÉELS ARC-LIKE ==========
    
    def test_arc_like_object_movement(self):
        """Test cas ARC: déplacement d'objets"""
        input_grid = np.array([
            [1, 1, 0, 0],
            [1, 1, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0]
        ])
        
        output_grid = np.array([
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 1, 1],
            [0, 0, 1, 1]
        ])
        
        sg_in = self.extractor.build_scene_graph(input_grid, background=0)
        sg_out = self.extractor.build_scene_graph(output_grid, background=0)
        
        # Détecter transformation
        transforms = self.transformer.detect_object_transformation_pattern(sg_in, sg_out)
        
        self.assert_true(len(transforms) > 0, "ARC pattern: detect movement")
        
        # Vérifier qu'il y a un move
        move_transforms = [t for t in transforms if t.transform_type == 'move']
        self.assert_true(len(move_transforms) > 0, "ARC pattern: has move transform")
    
    def test_arc_like_object_recoloring(self):
        """Test cas ARC: recoloration d'objets"""
        input_grid = np.array([[1, 1], [1, 1]])
        output_grid = np.array([[5, 5], [5, 5]])
        
        sg_in = self.extractor.build_scene_graph(input_grid, background=0)
        sg_out = self.extractor.build_scene_graph(output_grid, background=0)
        
        transforms = self.transformer.detect_object_transformation_pattern(sg_in, sg_out)
        
        # Vérifier qu'il y a un recolor
        recolor_transforms = [t for t in transforms if t.transform_type == 'recolor']
        self.assert_true(len(recolor_transforms) > 0, "ARC pattern: has recolor transform")
    
    # ========== EXÉCUTION TESTS ==========
    
    def run_all_tests(self):
        """Exécuter tous les tests"""
        print("=" * 80)
        print("MAGEN - Tests Unitaires Module Objects")
        print("=" * 80)
        print()
        
        print("--- Tests Extraction Objets ---")
        self.test_extract_single_object()
        self.test_extract_multiple_objects()
        self.test_extract_with_connectivity()
        print()
        
        print("--- Tests Détection Shapes ---")
        self.test_detect_point()
        self.test_detect_line_horizontal()
        self.test_detect_line_vertical()
        self.test_detect_rectangle()
        self.test_detect_square()
        self.test_detect_hollow_rectangle()
        print()
        
        print("--- Tests Scene Graph ---")
        self.test_build_scene_graph()
        self.test_detect_above_below_relations()
        self.test_detect_left_right_relations()
        self.test_detect_same_row_column()
        print()
        
        print("--- Tests Transformations ---")
        self.test_move_object()
        self.test_recolor_object()
        self.test_scale_object()
        self.test_scene_graph_to_grid()
        print()
        
        print("--- Tests Cas Réels ARC-like ---")
        self.test_arc_like_object_movement()
        self.test_arc_like_object_recoloring()
        print()
        
        # Résultats
        print("=" * 80)
        print("RÉSULTATS")
        print("=" * 80)
        print(f"Tests passés:  {self.tests_passed}/{self.tests_total}")
        print(f"Tests échoués: {self.tests_failed}/{self.tests_total}")
        print(f"Taux succès:   {100.0 * self.tests_passed / self.tests_total:.1f}%")
        print()
        
        # Statistiques modules
        print("Statistiques modules:")
        print(f"  Extractor:   {self.extractor.get_statistics()}")
        print(f"  Transformer: {self.transformer.get_statistics()}")
        print()
        
        return self.tests_failed == 0


if __name__ == "__main__":
    tester = TestObjectsModule()
    success = tester.run_all_tests()
    
    if success:
        print("✓ TOUS LES TESTS PASSÉS")
        sys.exit(0)
    else:
        print("✗ CERTAINS TESTS ONT ÉCHOUÉ")
        sys.exit(1)


# Made with Bob - Expert en Unit Testing, Object Detection, Scene Understanding