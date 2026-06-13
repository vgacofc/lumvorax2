"""
MAGEN - Tests Unitaires Module Memory
Validation mémoire épisodique et analogical reasoning

Tests:
1. Encodage épisodes en vecteurs
2. Calcul de similarité
3. Stockage et retrieval
4. Analogical transfer
5. Extraction de patterns conceptuels
6. Compression mémoire
"""

import numpy as np
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from memory import Episode, MemoryEncoder, EpisodicMemory, ConceptualPattern
from objects import ObjectExtractor
from symbolic import Transform, TransformType


class TestMemoryModule:
    """Suite de tests pour le module memory"""
    
    def __init__(self):
        self.encoder = MemoryEncoder(embedding_dim=128)
        self.memory = EpisodicMemory(self.encoder, max_episodes=100)
        self.extractor = ObjectExtractor()
        
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
    
    def assert_close(self, actual: float, expected: float, tolerance: float, test_name: str):
        """Assert avec tolérance"""
        self.tests_total += 1
        if abs(actual - expected) <= tolerance:
            print(f"✓ {test_name}")
            self.tests_passed += 1
            return True
        else:
            print(f"✗ {test_name}")
            print(f"  Expected: {expected} ± {tolerance}")
            print(f"  Got: {actual}")
            self.tests_failed += 1
            return False
    
    # ========== TESTS ENCODAGE ==========
    
    def test_encode_simple_episode(self):
        """Test encodage épisode simple"""
        grid_in = np.array([[1, 2], [3, 4]])
        grid_out = np.array([[4, 3], [2, 1]])
        
        episode = Episode(
            id=1,
            input_grid=grid_in,
            output_grid=grid_out,
            success=True,
            reward=1.0
        )
        
        embedding = self.encoder.encode_episode(episode)
        
        self.assert_equal(len(embedding), 128, "Encode episode - dimension")
        self.assert_true(isinstance(embedding, np.ndarray), "Encode episode - type")
        
        # Vérifier normalisation L2
        norm = float(np.linalg.norm(embedding))
        self.assert_close(norm, 1.0, 0.01, "Encode episode - L2 normalized")
    
    def test_encode_with_scene_graphs(self):
        """Test encodage avec scene graphs"""
        grid_in = np.array([
            [1, 1, 0, 2, 2],
            [1, 1, 0, 2, 2]
        ])
        grid_out = np.array([
            [2, 2, 0, 1, 1],
            [2, 2, 0, 1, 1]
        ])
        
        sg_in = self.extractor.build_scene_graph(grid_in, background=0)
        sg_out = self.extractor.build_scene_graph(grid_out, background=0)
        
        episode = Episode(
            id=2,
            input_grid=grid_in,
            output_grid=grid_out,
            input_scene_graph=sg_in,
            output_scene_graph=sg_out,
            success=True
        )
        
        embedding = self.encoder.encode_episode(episode)
        
        self.assert_equal(len(embedding), 128, "Encode with scene graphs - dimension")
        self.assert_close(float(np.linalg.norm(embedding)), 1.0, 0.01, "Encode with scene graphs - normalized")
    
    def test_encode_with_transformations(self):
        """Test encodage avec transformations"""
        grid_in = np.array([[1, 2], [3, 4]])
        grid_out = np.array([[3, 1], [4, 2]])
        
        transforms = [
            Transform(type=TransformType.ROTATE_90, parameters={})
        ]
        
        episode = Episode(
            id=3,
            input_grid=grid_in,
            output_grid=grid_out,
            transformations=transforms,
            success=True
        )
        
        embedding = self.encoder.encode_episode(episode)
        
        self.assert_equal(len(embedding), 128, "Encode with transforms - dimension")
    
    # ========== TESTS SIMILARITÉ ==========
    
    def test_similarity_identical(self):
        """Test similarité entre vecteurs identiques"""
        vec = np.random.rand(128)
        vec = vec / np.linalg.norm(vec)
        
        sim = self.encoder.compute_similarity(vec, vec, metric='cosine')
        
        self.assert_close(sim, 1.0, 0.01, "Similarity identical - cosine")
    
    def test_similarity_orthogonal(self):
        """Test similarité entre vecteurs orthogonaux"""
        vec1 = np.zeros(128)
        vec1[0] = 1.0
        
        vec2 = np.zeros(128)
        vec2[1] = 1.0
        
        sim = self.encoder.compute_similarity(vec1, vec2, metric='cosine')
        
        self.assert_close(sim, 0.5, 0.1, "Similarity orthogonal - cosine")
    
    def test_similarity_metrics(self):
        """Test différentes métriques de similarité"""
        vec1 = np.random.rand(128)
        vec1 = vec1 / np.linalg.norm(vec1)
        
        vec2 = vec1 + np.random.rand(128) * 0.1
        vec2 = vec2 / np.linalg.norm(vec2)
        
        sim_cosine = self.encoder.compute_similarity(vec1, vec2, metric='cosine')
        sim_euclidean = self.encoder.compute_similarity(vec1, vec2, metric='euclidean')
        sim_manhattan = self.encoder.compute_similarity(vec1, vec2, metric='manhattan')
        
        self.assert_true(0.0 <= sim_cosine <= 1.0, "Similarity metrics - cosine range")
        self.assert_true(0.0 <= sim_euclidean <= 1.0, "Similarity metrics - euclidean range")
        self.assert_true(0.0 <= sim_manhattan <= 1.0, "Similarity metrics - manhattan range")
    
    # ========== TESTS STOCKAGE & RETRIEVAL ==========
    
    def test_store_episode(self):
        """Test stockage épisode"""
        grid = np.array([[1, 2], [3, 4]])
        episode = Episode(id=0, input_grid=grid, output_grid=grid, success=True)
        
        episode_id = self.memory.store_episode(episode)
        
        self.assert_true(episode_id > 0, "Store episode - ID assigned")
        self.assert_equal(len(self.memory.episodes), 1, "Store episode - count")
        self.assert_true(episode.embedding is not None, "Store episode - embedding created")
    
    def test_store_multiple_episodes(self):
        """Test stockage épisodes multiples"""
        for i in range(5):
            grid = np.random.randint(0, 10, (3, 3))
            episode = Episode(id=0, input_grid=grid, output_grid=grid, success=(i % 2 == 0))
            self.memory.store_episode(episode)
        
        self.assert_equal(len(self.memory.episodes), 6, "Store multiple - count")  # 1 from previous + 5 new
        self.assert_equal(self.memory.successful_episodes, 4, "Store multiple - successful count")  # 1 + 3 new
    
    def test_retrieve_similar(self):
        """Test retrieval par similarité"""
        # Créer épisodes similaires
        base_grid = np.array([[1, 2], [3, 4]])
        
        for i in range(3):
            grid = base_grid + i  # Légère variation
            episode = Episode(id=0, input_grid=grid, output_grid=grid, success=True)
            self.memory.store_episode(episode)
        
        # Query similaire
        query_grid = base_grid + 0.5
        query = Episode(id=0, input_grid=query_grid, output_grid=query_grid)
        
        similar = self.memory.retrieve_similar(query, k=2)
        
        self.assert_equal(len(similar), 2, "Retrieve similar - count")
        self.assert_true(all(0.0 <= sim <= 1.0 for _, sim in similar), "Retrieve similar - similarity range")
        
        # Vérifier ordre décroissant
        if len(similar) >= 2:
            self.assert_true(similar[0][1] >= similar[1][1], "Retrieve similar - sorted")
    
    # ========== TESTS ANALOGICAL TRANSFER ==========
    
    def test_analogical_transfer_simple(self):
        """Test analogical transfer simple"""
        # Source: grille 2x2 avec transformation
        source_input = np.array([[1, 2], [3, 4]])
        source_output = np.array([[3, 1], [4, 2]])
        
        transforms = [Transform(type=TransformType.ROTATE_90, parameters={})]
        
        source_episode = Episode(
            id=0,
            input_grid=source_input,
            output_grid=source_output,
            transformations=transforms,
            success=True
        )
        
        # Target: grille 4x4
        target_input = np.random.randint(0, 10, (4, 4))
        
        adapted = self.memory.analogical_transfer(source_episode, target_input)
        
        self.assert_true(len(adapted) > 0, "Analogical transfer - has transforms")
        self.assert_equal(len(adapted), len(transforms), "Analogical transfer - count")
    
    def test_analogical_transfer_scaling(self):
        """Test adaptation scaling dans analogical transfer"""
        # Source: petite grille avec déplacement
        source_input = np.array([[1, 0], [0, 0]])
        source_output = np.array([[0, 0], [0, 1]])
        
        from objects import ObjectTransformation
        transforms = [
            ObjectTransformation(
                object_id=0,
                transform_type='move',
                parameters={'dx': 1, 'dy': 1}
            )
        ]
        
        source_episode = Episode(
            id=0,
            input_grid=source_input,
            output_grid=source_output,
            transformations=transforms,
            success=True
        )
        
        # Target: grille 2x plus grande
        target_input = np.zeros((4, 4))
        
        adapted = self.memory.analogical_transfer(source_episode, target_input)
        
        self.assert_true(len(adapted) > 0, "Analogical transfer scaling - has transforms")
        
        # Vérifier adaptation paramètres
        if len(adapted) > 0 and hasattr(adapted[0], 'parameters'):
            # dx devrait être scalé
            self.assert_true('dx' in adapted[0].parameters, "Analogical transfer scaling - dx exists")
    
    # ========== TESTS PATTERNS CONCEPTUELS ==========
    
    def test_extract_patterns(self):
        """Test extraction patterns conceptuels"""
        # Créer épisodes avec pattern récurrent
        for i in range(5):
            grid = np.random.randint(0, 10, (3, 3))
            transforms = [
                Transform(type=TransformType.ROTATE_90, parameters={}),
                Transform(type=TransformType.FLIP_HORIZONTAL, parameters={})
            ]
            episode = Episode(
                id=0,
                input_grid=grid,
                output_grid=grid,
                transformations=transforms,
                success=(i < 4)  # 4 succès, 1 échec
            )
            self.memory.store_episode(episode)
        
        patterns = self.memory.extract_conceptual_patterns(min_frequency=3)
        
        self.assert_true(len(patterns) > 0, "Extract patterns - found patterns")
        
        if len(patterns) > 0:
            pattern = patterns[0]
            self.assert_true(pattern.frequency >= 3, "Extract patterns - frequency")
            self.assert_true(0.0 <= pattern.success_rate <= 1.0, "Extract patterns - success rate")
            self.assert_true(pattern.embedding is not None, "Extract patterns - has embedding")
    
    # ========== TESTS COMPRESSION ==========
    
    def test_memory_compression(self):
        """Test compression mémoire"""
        # Remplir mémoire au-delà de max_episodes
        memory_small = EpisodicMemory(self.encoder, max_episodes=10)
        
        for i in range(15):
            grid = np.random.randint(0, 10, (2, 2))
            episode = Episode(
                id=0,
                input_grid=grid,
                output_grid=grid,
                success=(i % 3 == 0),  # 1/3 succès
                timestamp=float(i)
            )
            memory_small.store_episode(episode)
        
        # Vérifier compression
        self.assert_true(len(memory_small.episodes) <= 10, "Memory compression - size limit")
        
        # Vérifier que succès sont gardés
        successful = [ep for ep in memory_small.episodes if ep.success]
        self.assert_true(len(successful) > 0, "Memory compression - keeps successful")
    
    # ========== TESTS STATISTIQUES ==========
    
    def test_statistics(self):
        """Test statistiques mémoire"""
        stats = self.memory.get_statistics()
        
        self.assert_true('total_episodes' in stats, "Statistics - has total_episodes")
        self.assert_true('stored_episodes' in stats, "Statistics - has stored_episodes")
        self.assert_true('successful_episodes' in stats, "Statistics - has successful_episodes")
        self.assert_true('success_rate' in stats, "Statistics - has success_rate")
        self.assert_true('patterns_extracted' in stats, "Statistics - has patterns_extracted")
        self.assert_true('retrievals' in stats, "Statistics - has retrievals")
        self.assert_true('analogical_transfers' in stats, "Statistics - has analogical_transfers")
    
    # ========== EXÉCUTION TESTS ==========
    
    def run_all_tests(self):
        """Exécuter tous les tests"""
        print("=" * 80)
        print("MAGEN - Tests Unitaires Module Memory")
        print("=" * 80)
        print()
        
        print("--- Tests Encodage ---")
        self.test_encode_simple_episode()
        self.test_encode_with_scene_graphs()
        self.test_encode_with_transformations()
        print()
        
        print("--- Tests Similarité ---")
        self.test_similarity_identical()
        self.test_similarity_orthogonal()
        self.test_similarity_metrics()
        print()
        
        print("--- Tests Stockage & Retrieval ---")
        self.test_store_episode()
        self.test_store_multiple_episodes()
        self.test_retrieve_similar()
        print()
        
        print("--- Tests Analogical Transfer ---")
        self.test_analogical_transfer_simple()
        self.test_analogical_transfer_scaling()
        print()
        
        print("--- Tests Patterns Conceptuels ---")
        self.test_extract_patterns()
        print()
        
        print("--- Tests Compression ---")
        self.test_memory_compression()
        print()
        
        print("--- Tests Statistiques ---")
        self.test_statistics()
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
        print(f"  Encoder:  {self.encoder.get_statistics()}")
        print(f"  Memory:   {self.memory.get_statistics()}")
        print()
        
        return self.tests_failed == 0


if __name__ == "__main__":
    tester = TestMemoryModule()
    success = tester.run_all_tests()
    
    if success:
        print("✓ TOUS LES TESTS PASSÉS")
        sys.exit(0)
    else:
        print("✗ CERTAINS TESTS ONT ÉCHOUÉ")
        sys.exit(1)


# Made with Bob - Expert en Unit Testing, Memory Systems, Analogical Reasoning