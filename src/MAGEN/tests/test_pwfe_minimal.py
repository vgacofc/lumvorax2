#!/usr/bin/env python3
"""
MAGEN - Test Unitaire Minimal PWFE
===================================

SESSION 51 - CORRECTION #2
Objectif: Diagnostiquer pourquoi PWFE ne fonctionne pas

Test minimal pour vérifier:
1. PWFE accepte-t-il des programmes déjà générés ?
2. PWFE génère-t-il des mondes ?
3. PWFE retourne-t-il des résultats valides ?
"""

import numpy as np
import sys
from pathlib import Path

# Ajouter parent directory au path
sys.path.insert(0, str(Path(__file__).parent.parent))

from core.parallel_world_frame_engine import ParallelWorldFrameEngine
from dsl.program import Program


def test_pwfe_with_programs():
    """Test PWFE avec programmes déjà générés (cas actuel)"""
    print("="*80)
    print("TEST 1: PWFE avec programmes déjà générés")
    print("="*80)
    
    # Créer PWFE
    pwfe = ParallelWorldFrameEngine(
        max_hypotheses=5,
        max_frames=2,
        pruning_threshold=0.3,
        verbose=True
    )
    
    # Créer programmes simples
    programs = []
    for i in range(3):
        prog = Program()
        prog.add_operation(f"identity_{i}", lambda x: x, ())
        programs.append(prog)
    
    print(f"\n📊 Programmes créés: {len(programs)}")
    print(f"   Type: {type(programs[0])}")
    
    # Créer données test
    input_grid = np.array([[1, 2], [3, 4]])
    train_pairs = [
        (np.array([[1, 2], [3, 4]]), np.array([[1, 2], [3, 4]])),
        (np.array([[5, 6], [7, 8]]), np.array([[5, 6], [7, 8]]))
    ]
    
    # Tester PWFE
    try:
        result = pwfe.generate_parallel_worlds(
            input_grid=input_grid,
            train_pairs=train_pairs,
            program_generators=programs,  # ❌ Ce sont des PROGRAMMES, pas des GÉNÉRATEURS
            invariants={}
        )
        
        print(f"\n✅ PWFE exécuté sans erreur")
        print(f"   Success: {result.get('success', False)}")
        print(f"   Mondes générés: {result.get('total_worlds', 0)}")
        print(f"   Best trajectory: {result.get('best_trajectory') is not None}")
        
        if result.get('total_worlds', 0) == 0:
            print(f"\n⚠️  PROBLÈME: Aucun monde généré !")
            print(f"   Cause probable: Type mismatch (Program vs Callable)")
            return False
        
        return result.get('success', False)
    
    except Exception as e:
        print(f"\n❌ ERREUR PWFE: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_pwfe_with_generators():
    """Test PWFE avec générateurs de programmes (cas attendu)"""
    print("\n" + "="*80)
    print("TEST 2: PWFE avec générateurs de programmes")
    print("="*80)
    
    # Créer PWFE
    pwfe = ParallelWorldFrameEngine(
        max_hypotheses=5,
        max_frames=2,
        pruning_threshold=0.3,
        verbose=True
    )
    
    # Créer générateurs (fonctions qui retournent programmes)
    def make_identity_generator(i):
        def generator():
            prog = Program()
            prog.add_operation(f"identity_{i}", lambda x: x, ())
            return prog
        return generator
    
    generators = [make_identity_generator(i) for i in range(3)]
    
    print(f"\n📊 Générateurs créés: {len(generators)}")
    print(f"   Type: {type(generators[0])}")
    print(f"   Callable: {callable(generators[0])}")
    
    # Créer données test
    input_grid = np.array([[1, 2], [3, 4]])
    train_pairs = [
        (np.array([[1, 2], [3, 4]]), np.array([[1, 2], [3, 4]])),
        (np.array([[5, 6], [7, 8]]), np.array([[5, 6], [7, 8]]))
    ]
    
    # Tester PWFE
    try:
        result = pwfe.generate_parallel_worlds(
            input_grid=input_grid,
            train_pairs=train_pairs,
            program_generators=generators,  # ✅ Ce sont des GÉNÉRATEURS
            invariants={}
        )
        
        print(f"\n✅ PWFE exécuté sans erreur")
        print(f"   Success: {result.get('success', False)}")
        print(f"   Mondes générés: {result.get('total_worlds', 0)}")
        print(f"   Best trajectory: {result.get('best_trajectory') is not None}")
        
        return result.get('success', False)
    
    except Exception as e:
        print(f"\n❌ ERREUR PWFE: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Exécute tests diagnostiques PWFE"""
    print("\n🔬 DIAGNOSTIC PWFE - SESSION 51")
    print("Objectif: Identifier pourquoi PWFE ne fonctionne pas\n")
    
    # Test 1: Avec programmes (cas actuel - devrait échouer)
    test1_passed = test_pwfe_with_programs()
    
    # Test 2: Avec générateurs (cas attendu - devrait fonctionner)
    test2_passed = test_pwfe_with_generators()
    
    # Résumé
    print("\n" + "="*80)
    print("RÉSUMÉ DIAGNOSTIC")
    print("="*80)
    print(f"Test 1 (Programmes): {'✅ PASSÉ' if test1_passed else '❌ ÉCHOUÉ'}")
    print(f"Test 2 (Générateurs): {'✅ PASSÉ' if test2_passed else '❌ ÉCHOUÉ'}")
    
    if not test1_passed and test2_passed:
        print("\n🎯 DIAGNOSTIC CONFIRMÉ:")
        print("   PWFE attend des GÉNÉRATEURS, pas des PROGRAMMES")
        print("   Solution: Refonte PWFE pour accepter programmes déjà générés")
    elif test1_passed:
        print("\n⚠️  DIAGNOSTIC INATTENDU:")
        print("   PWFE fonctionne avec programmes ?")
        print("   Vérifier intégration dans pipeline principal")
    else:
        print("\n🚨 PROBLÈME PLUS PROFOND:")
        print("   PWFE ne fonctionne ni avec programmes ni avec générateurs")
        print("   Vérifier architecture PWFE complète")
    
    print("="*80)


if __name__ == "__main__":
    main()

# Made with Bob
