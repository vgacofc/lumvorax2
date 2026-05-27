/* TEST C430 #3: IMPOSSIBLE ENTROPY
 * 
 * OBJECTIF: Détecter cache stale ou buffers non mis à jour
 * 
 * MÉTHODE: Générer entropie impossible à prédire
 * - Utilise get_global_id(0) + timestamp simulé
 * - Chaque exécution produit pattern différent
 * - Si 2 exécutions identiques → cache stale détecté
 * 
 * VALIDATION:
 * - Exécuter 2 fois le kernel
 * - Si output_run1 == output_run2 → cache stale ❌
 * - Si output_run1 != output_run2 → GPU exécute réellement ✅
 */

__kernel void test_impossible_entropy(__global uint* output, uint seed) {
    uint gid = get_global_id(0);
    
    /* Générateur pseudo-aléatoire basé sur:
     * - Thread ID (gid)
     * - Seed externe (change à chaque exécution)
     * - Opérations non-linéaires
     */
    uint state = gid ^ seed;
    
    /* 100 itérations de génération pseudo-aléatoire */
    for (uint i = 0; i < 100; i++) {
        /* Xorshift128+ (générateur haute qualité) */
        uint s1 = state;
        uint s0 = seed;
        state = s0;
        s1 ^= s1 << 23;
        s1 ^= s1 >> 17;
        s1 ^= s0;
        s1 ^= s0 >> 26;
        seed = s1;
        
        /* Accumulation non-linéaire */
        state += s1 * 0x9e3779b9u;  /* Golden ratio */
    }
    
    /* Résultat final dépend de:
     * - gid (unique par thread)
     * - seed (change à chaque exécution)
     * - 100 itérations non-linéaires
     * 
     * Impossible d'avoir même résultat sur 2 exécutions
     * sauf si cache stale ou GPU n'exécute pas
     */
    output[gid] = state;
}

; Made with Bob
