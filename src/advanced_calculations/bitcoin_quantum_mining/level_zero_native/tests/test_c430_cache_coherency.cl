/* TEST C430 #7: CACHE COHERENCY
 * 
 * OBJECTIF: Valider que CPU cache invalidation fonctionne
 * 
 * MÉTHODE: Forcer CPU à cacher output, puis GPU écrit, puis CPU relit
 * - CPU écrit pattern A dans output
 * - CPU lit output (mise en cache L1/L2/L3)
 * - GPU écrit pattern B dans output
 * - CPU relit output SANS invalidation cache
 * - Si CPU lit pattern A → cache stale ❌
 * - CPU relit output AVEC invalidation cache (CLFLUSH+MFENCE)
 * - Si CPU lit pattern B → cache coherency OK ✅
 * 
 * VALIDATION:
 * - Pattern A: 0xAAAAAAAA
 * - Pattern B: 0xBBBBBBBB
 * - Lecture sans flush: devrait lire A (cache stale)
 * - Lecture avec flush: devrait lire B (cache invalidé) ✅
 */

__kernel void test_cache_coherency(__global uint* output, uint pattern) {
    uint gid = get_global_id(0);
    
    /* GPU écrit pattern fourni en paramètre
     * 
     * Si pattern = 0xBBBBBBBB:
     * - GPU écrit 0xBBBBBBBB dans output[gid]
     * - CPU cache contient encore 0xAAAAAAAA (stale)
     * - CPU doit invalider cache pour voir 0xBBBBBBBB
     */
    output[gid] = pattern;
    
    /* Écriture additionnelle pour forcer flush GPU
     * Certains GPU bufferisent écritures, cette 2ème écriture
     * force le flush du write buffer GPU
     */
    output[gid] = pattern ^ gid;  /* XOR avec gid pour unicité */
}

/* PROTOCOLE TEST:
 * 
 * PHASE 1: Préparation cache CPU
 * 1. CPU écrit 0xAAAAAAAA dans output[0..255]
 * 2. CPU lit output[0..255] (mise en cache L1/L2/L3)
 * 3. Vérifier que CPU lit bien 0xAAAAAAAA
 * 
 * PHASE 2: GPU écrit nouveau pattern
 * 4. Lancer kernel avec pattern=0xBBBBBBBB
 * 5. Attendre fin exécution GPU (GEM_WAIT)
 * 
 * PHASE 3: Test cache stale (SANS invalidation)
 * 6. CPU lit output[0] SANS CLFLUSH
 * 7. ATTENDU: CPU lit 0xAAAAAAAA (cache stale) ❌
 * 8. Si CPU lit 0xBBBBBBBB → cache auto-invalidé (rare)
 * 
 * PHASE 4: Test cache coherency (AVEC invalidation)
 * 9. CPU exécute CLFLUSH sur output[0..255]
 * 10. CPU exécute MFENCE (memory fence)
 * 11. CPU lit output[0..255]
 * 12. ATTENDU: CPU lit 0xBBBBBBBB ^ gid ✅
 * 13. Si CPU lit 0xAAAAAAAA → cache coherency ÉCHEC ❌
 * 
 * RÉSULTATS ATTENDUS:
 * 
 * CAS 1: Cache coherency parfait (rare sur x86) ✅
 * - Phase 3: CPU lit 0xBBBBBBBB (cache auto-invalidé)
 * - Phase 4: CPU lit 0xBBBBBBBB (confirmé)
 * 
 * CAS 2: Cache coherency manuel requis (normal x86) ✅
 * - Phase 3: CPU lit 0xAAAAAAAA (cache stale attendu)
 * - Phase 4: CPU lit 0xBBBBBBBB (CLFLUSH+MFENCE fonctionne) ✅
 * 
 * CAS 3: Cache coherency ÉCHEC ❌
 * - Phase 3: CPU lit 0xAAAAAAAA (cache stale)
 * - Phase 4: CPU lit 0xAAAAAAAA (CLFLUSH+MFENCE ne fonctionne pas) ❌
 * 
 * CAS 4: GPU n'écrit pas ❌
 * - Phase 3: CPU lit 0xAAAAAAAA
 * - Phase 4: CPU lit 0xAAAAAAAA (GPU n'a rien écrit) ❌
 */

; Made with Bob
