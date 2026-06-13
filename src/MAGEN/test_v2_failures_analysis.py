#!/usr/bin/env python3
"""
Analyse détaillée des 4 échecs V2 vs succès V1
"""

import json
import numpy as np
from pathlib import Path
from typing import Dict, List

# Import MAGEN
from core.magen_memory import MAGENMemory
from synthesis.synthesizer import ProgramSynthesizer

def load_puzzle(puzzle_id: str) -> Dict:
    """Charge un puzzle depuis le dataset"""
    dataset_path = Path("data/arc-agi_training_challenges.json")
    with open(dataset_path) as f:
        dataset = json.load(f)
    return dataset[puzzle_id]

def analyze_v1_solution(puzzle_id: str, puzzle: Dict) -> Dict:
    """Analyse comment V1 a résolu ce puzzle"""
    print(f"\n{'='*80}")
    print(f"ANALYSE V1 - Puzzle {puzzle_id}")
    print(f"{'='*80}")
    
    # Charger V1
    magen = MAGENMemory()
    
    # Résoudre avec V1
    train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                   for p in puzzle['train']]
    test_input = np.array(puzzle['test'][0]['input'])
    
    result = magen.solve_puzzle(train_pairs, test_input)
    
    print(f"\n✓ V1 Solution trouvée:")
    print(f"  Heuristique: {result.get('heuristic', 'unknown')}")
    print(f"  Confiance: {result.get('confidence', 0):.2%}")
    print(f"  Temps: {result.get('elapsed', 0):.3f}s")
    
    # Analyser les transformations
    print(f"\n📊 Analyse transformations:")
    for i, (inp, out) in enumerate(train_pairs):
        print(f"\n  Train pair {i+1}:")
        print(f"    Input shape:  {inp.shape}")
        print(f"    Output shape: {out.shape}")
        print(f"    Input colors:  {sorted(set(inp.flatten()))}")
        print(f"    Output colors: {sorted(set(out.flatten()))}")
        
        # Vérifier transformations géométriques simples
        if inp.shape == out.shape:
            if np.array_equal(inp, out):
                print(f"    ⚠️  Identity (input == output)")
            elif np.array_equal(np.rot90(inp, 2), out):
                print(f"    ✓ rotate180")
            elif np.array_equal(np.rot90(inp, 1), out):
                print(f"    ✓ rotate90")
            elif np.array_equal(np.rot90(inp, 3), out):
                print(f"    ✓ rotate270")
            elif np.array_equal(np.fliplr(inp), out):
                print(f"    ✓ mirror_horizontal")
            elif np.array_equal(np.flipud(inp), out):
                print(f"    ✓ mirror_vertical")
            else:
                print(f"    ⚠️  Complex transformation (same shape)")
        else:
            # Vérifier scaling
            h_ratio = out.shape[0] / inp.shape[0]
            w_ratio = out.shape[1] / inp.shape[1]
            if h_ratio == w_ratio and h_ratio == int(h_ratio):
                print(f"    ✓ scale({int(h_ratio)})")
            elif h_ratio == w_ratio and h_ratio < 1:
                factor = int(1 / h_ratio)
                print(f"    ✓ downscale({factor})")
            else:
                print(f"    ⚠️  Complex transformation (shape change)")
    
    return result

def analyze_v2_failure(puzzle_id: str, puzzle: Dict) -> Dict:
    """Analyse pourquoi V2 échoue sur ce puzzle"""
    print(f"\n{'='*80}")
    print(f"ANALYSE V2 - Puzzle {puzzle_id}")
    print(f"{'='*80}")
    
    # Charger V2
    synthesizer = ProgramSynthesizer(max_depth=7, max_candidates=100)
    
    # Préparer données
    train_pairs = [(np.array(p['input']), np.array(p['output'])) 
                   for p in puzzle['train']]
    test_input = np.array(puzzle['test'][0]['input'])
    
    # Tenter synthesis
    program = synthesizer.synthesize(train_pairs, test_input)
    
    if program:
        print(f"\n✓ V2 Programme trouvé:")
        print(f"  {program}")
        
        # Tester sur train
        train_correct = 0
        for inp, expected_out in train_pairs:
            try:
                actual_out = program.execute(inp)
                if np.array_equal(actual_out, expected_out):
                    train_correct += 1
            except:
                pass
        
        print(f"  Train accuracy: {train_correct}/{len(train_pairs)}")
        
        # Tester sur test
        try:
            test_output = program.execute(test_input)
            test_expected = np.array(puzzle['test'][0]['output'])
            test_correct = np.array_equal(test_output, test_expected)
            print(f"  Test correct: {test_correct}")
        except Exception as e:
            print(f"  Test error: {e}")
    else:
        print(f"\n❌ V2 Aucun programme synthétisé")
        print(f"\n🔍 Diagnostic:")
        
        # Analyser pourquoi aucun candidat n'a passé
        from dsl.composer import Composer
        composer = Composer(max_depth=7, max_candidates=100)
        
        # Générer candidats
        candidates = composer.generate_candidates(train_pairs)
        print(f"  Candidats générés: {len(candidates)}")
        
        if candidates:
            # Tester quelques candidats
            print(f"\n  Test des 5 premiers candidats:")
            for i, prog in enumerate(candidates[:5]):
                try:
                    # Tester sur premier train pair
                    inp, expected = train_pairs[0]
                    result = prog.execute(inp)
                    match = np.array_equal(result, expected)
                    print(f"    {i+1}. {prog.operations[0].name if prog.operations else 'empty'}: {'✓' if match else '✗'}")
                except Exception as e:
                    print(f"    {i+1}. Error: {e}")
        
        # Vérifier si les primitives nécessaires existent
        print(f"\n  Primitives disponibles dans composer:")
        for category, prims in composer.primitives.items():
            print(f"    {category}: {len(prims)} primitives")
    
    return {"program": program}

def main():
    """Analyse les 4 échecs"""
    failures = ["2dee498d", "3aa6fb7a", "5bd6f4ac", "88a62173"]
    
    print("="*80)
    print("ANALYSE COMPARATIVE V1 vs V2 - 4 ÉCHECS")
    print("="*80)
    
    for puzzle_id in failures:
        puzzle = load_puzzle(puzzle_id)
        
        # Analyser V1
        v1_result = analyze_v1_solution(puzzle_id, puzzle)
        
        # Analyser V2
        v2_result = analyze_v2_failure(puzzle_id, puzzle)
        
        print(f"\n{'='*80}")
        print(f"CONCLUSION - {puzzle_id}")
        print(f"{'='*80}")
        print(f"V1: ✓ Succès avec {v1_result.get('heuristic', 'unknown')}")
        print(f"V2: ✗ Échec - {v2_result.get('program', 'No program')}")
        print()
        
        input("Appuyez sur Entrée pour continuer...")

if __name__ == "__main__":
    main()

# Made with Bob
