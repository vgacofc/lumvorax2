#!/usr/bin/env python3
"""
Test du Puzzle Classifier sur les 11 puzzles réussis
"""

import json
import numpy as np
from pathlib import Path
from classification import PuzzleClassifier, TransformationClass

def load_puzzle(puzzle_id: str):
    """Charge un puzzle"""
    dataset_path = Path("arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]

def test_classifier():
    """Test le classifier sur les 11 puzzles"""
    
    # Les 11 puzzles réussis par V1
    puzzles = [
        "2dee498d",  # Pattern répété
        "3aa6fb7a",  # Object relational
        "3c9b0459",  # Geometric
        "5bd6f4ac",  # Semantic region
        "6150a2bd",  # Geometric
        "67a3c6ac",  # Geometric
        "68b16354",  # Geometric
        "88a62173",  # Spatial sampling
        "9172f3a0",  # Geometric
        "c59eb873",  # Geometric
        "ed36ccf7",  # Geometric
    ]
    
    classifier = PuzzleClassifier()
    
    print("="*80)
    print("TEST PUZZLE CLASSIFIER - 11 PUZZLES")
    print("="*80)
    
    results = []
    
    for puzzle_id in puzzles:
        print(f"\n{'='*80}")
        print(f"Puzzle: {puzzle_id}")
        print(f"{'='*80}")
        
        # Charger puzzle
        puzzle = load_puzzle(puzzle_id)
        train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                       for p in puzzle['train']]
        
        # Classifier
        result = classifier.classify(train_pairs)
        
        print(f"\n✓ Classification:")
        print(f"  Classe primaire: {result.primary_class.value}")
        print(f"  Confiance: {result.confidence:.2%}")
        
        if result.secondary_classes:
            print(f"\n  Classes secondaires:")
            for cls, score in result.secondary_classes:
                print(f"    - {cls.value}: {score:.2%}")
        
        print(f"\n  Features détectées:")
        for class_name, features in result.features.items():
            if features:  # Afficher seulement si non vide
                print(f"    {class_name}:")
                for key, value in features.items():
                    if value not in [False, [], 1.0, True]:  # Filtrer valeurs par défaut
                        print(f"      - {key}: {value}")
        
        results.append({
            'puzzle_id': puzzle_id,
            'class': result.primary_class.value,
            'confidence': result.confidence
        })
    
    # Résumé
    print(f"\n{'='*80}")
    print("RÉSUMÉ CLASSIFICATION")
    print(f"{'='*80}")
    
    # Compter par classe
    class_counts = {}
    for r in results:
        cls = r['class']
        class_counts[cls] = class_counts.get(cls, 0) + 1
    
    print(f"\nDistribution des classes:")
    for cls, count in sorted(class_counts.items(), key=lambda x: x[1], reverse=True):
        print(f"  {cls}: {count}/11 ({count/11*100:.1f}%)")
    
    print(f"\nConfiance moyenne: {sum(r['confidence'] for r in results)/len(results):.2%}")
    
    # Identifier puzzles mal classifiés
    expected_classes = {
        "2dee498d": "periodic",
        "3aa6fb7a": "object_relational",
        "5bd6f4ac": "semantic_region",
        "88a62173": "semantic_region",  # ou unknown
    }
    
    print(f"\nVérification classifications attendues:")
    for puzzle_id, expected in expected_classes.items():
        actual = next(r['class'] for r in results if r['puzzle_id'] == puzzle_id)
        match = "✓" if actual == expected else "✗"
        print(f"  {match} {puzzle_id}: attendu={expected}, obtenu={actual}")

if __name__ == "__main__":
    test_classifier()

# Made with Bob
