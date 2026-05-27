/* TEST C430 #2: DEPENDENCY CHAIN (ALU intensif)
 * 
 * OBJECTIF: Prouver que le GPU exécute réellement le kernel
 * 
 * MÉTHODE: Chaîne de dépendances ALU impossible à faker
 * - Chaque thread calcule une séquence de 1000 opérations dépendantes
 * - Résultat final = f(thread_id) impossible à prédire sans exécution
 * - Si output[thread_id] == résultat_attendu → GPU a exécuté
 * 
 * VALIDATION:
 * - Cache/CPU ne peut pas faker 1000 opérations ALU par thread
 * - Résultat dépend de get_global_id(0) → chaque thread unique
 * - Pattern impossible à deviner sans exécution réelle
 */

__kernel void test_dependency_chain(__global uint* output) {
    uint gid = get_global_id(0);
    
    /* Initialisation avec thread ID */
    uint result = gid;
    
    /* Chaîne de 1000 opérations ALU dépendantes
     * Chaque opération dépend du résultat précédent
     * Impossible à optimiser ou faker
     */
    for (uint i = 0; i < 1000; i++) {
        /* Opérations ALU complexes */
        result = result * 1103515245u + 12345u;  /* LCG (Linear Congruential Generator) */
        result ^= (result >> 16);                 /* XOR shift */
        result = result * 2654435761u;            /* Knuth multiplicative hash */
        result ^= (result << 13);                 /* XOR shift inverse */
        result = result + i;                      /* Dépendance sur itération */
    }
    
    /* Écriture résultat final
     * Si GPU n'exécute pas, output[gid] restera 0
     * Si GPU exécute, output[gid] = résultat unique par thread
     */
    output[gid] = result;
}

// Made with Bob
