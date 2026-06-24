        print(f"  Temps: {elapsed:.3f}s")
    
    print(f"\n{'='*80}")
    print(f"RÉSULTAT FINAL: {successes}/11 ({successes/11*100:.1f}%)")
    print(f"{'='*80}")
    
    # Anti-regression check
    regression = baseline_solved - current_solved
    improvement = current_solved - baseline_solved
    
    if regression:
        print(f"⚠️ RÉGRESSION DÉTECTÉE: {len(regression)} puzzles perdus")
        print(f"   Puzzles perdus: {regression}")
    
    if improvement:
        print(f"✅ AMÉLIORATION: +{len(improvement)} puzzles récupérés!")
        print(f"   Puzzles récupérés: {improvement}")
    
    if successes == 11:
        print("✓✓✓ OBJECTIF ATTEINT: 11/11 (100%) ✓✓✓")
        print("V2 Session 33 = V1 sur les 11 puzzles!")
    elif successes >= 9:
        print(f"✓ PROGRESSION MAJEURE: {successes}/11 (+{successes-8} vs Session 31)")
        print("Corrections Session 32 efficaces!")
    elif successes == 8:
        print("⚠️ Stable: 8/11 (pas de régression vs Session 31)")
    else:
        print(f"✗ Régression: {successes}/11 (baseline: 8/11)")
    
    print(f"\nGain vs Session 31 (8/11): +{successes-8} puzzles")
    print(f"Gain vs V2 baseline (7/11): +{successes-7} puzzles")


if __name__ == "__main__":
    main()