/* TEST C430 #5: MEMORY BANDWIDTH WALL
 * 
 * OBJECTIF: Valider que Surface States pointent vers vraies adresses mémoire
 * 
 * MÉTHODE: Saturer bande passante mémoire GPU
 * - Lire 1 MB de données input
 * - Écrire 1 MB de données output
 * - Mesurer temps exécution
 * - Si temps ≈ (2 MB / bande_passante_GPU) → Surface States corrects ✅
 * - Si temps << attendu → Surface States NULL ou cache ❌
 * 
 * VALIDATION:
 * - Intel UHD 620: ~25 GB/s bande passante
 * - 2 MB transfert devrait prendre ~80 µs
 * - Si temps < 10 µs → pas de transfert réel (Surface States NULL)
 * - Si temps ≈ 80 µs → transfert réel (Surface States corrects) ✅
 */

__kernel void test_memory_bandwidth(__global uint* input, __global uint* output) {
    uint gid = get_global_id(0);
    uint local_size = get_local_size(0);
    uint group_id = get_group_id(0);
    
    /* Chaque thread lit 256 uint32 (1 KB) depuis input
     * Total: 256 threads × 1 KB = 256 KB par work-group
     * Avec 4 work-groups: 1 MB total
     */
    uint sum = 0;
    for (uint i = 0; i < 256; i++) {
        uint offset = gid * 256 + i;
        sum += input[offset];
    }
    
    /* Chaque thread écrit 256 uint32 (1 KB) vers output
     * Total: 256 threads × 1 KB = 256 KB par work-group
     * Avec 4 work-groups: 1 MB total
     */
    for (uint i = 0; i < 256; i++) {
        uint offset = gid * 256 + i;
        output[offset] = sum + i;
    }
    
    /* VALIDATION:
     * 
     * Si Surface States NULL:
     * - GPU lit/écrit dans NULL
     * - Pas de transfert mémoire réel
     * - Temps exécution < 10 µs (overhead seulement)
     * 
     * Si Surface States corrects:
     * - GPU lit 1 MB depuis input GTT
     * - GPU écrit 1 MB vers output GTT
     * - Temps exécution ≈ 80 µs (2 MB / 25 GB/s)
     * 
     * Mesurer temps avec:
     * - clock_gettime(CLOCK_MONOTONIC) avant/après dispatch
     * - Si temps < 10 µs → Surface States NULL ❌
     * - Si temps ≈ 80 µs → Surface States corrects ✅
     */
}

/* PROTOCOLE TEST:
 * 
 * 1. Allouer input buffer 1 MB (256K uint32)
 * 2. Allouer output buffer 1 MB (256K uint32)
 * 3. Lancer kernel avec 1024 threads (4 work-groups × 256 threads)
 * 4. Mesurer temps exécution
 * 
 * RÉSULTATS ATTENDUS:
 * - Temps < 10 µs → Surface States NULL (bug) ❌
 * - Temps 10-50 µs → Cache hit (Surface States peut-être corrects)
 * - Temps 50-150 µs → Transfert mémoire réel (Surface States corrects) ✅
 * - Temps > 150 µs → Overhead excessif (investigation requise)
 */

; Made with Bob
