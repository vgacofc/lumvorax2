/* 
 * TEST C341 - Kernel Compteur Atomique
 * Objectif: PROUVER combien de threads GPU s'exécutent réellement
 * 
 * Si GPGPU_WALKER correct: counter ≈ 10,000,000
 * Si GPGPU_WALKER bugué: counter = 256, 512, 1024, etc.
 */

__kernel void test_thread_counter(
    __global uint* counter,      // Compteur atomique global
    __global uint* thread_ids,   // Array pour stocker les thread IDs
    uint max_threads             // Nombre max de threads attendus
) {
    uint gid = get_global_id(0);
    
    // Test 1: Incrémenter compteur atomique
    atomic_inc(counter);
    
    // Test 2: Stocker notre thread ID (si dans limite)
    if (gid < max_threads) {
        thread_ids[gid] = gid;
    }
    
    // Test 3: Écrire une valeur magique pour prouver exécution
    if (gid == 0) {
        counter[1] = 0xCAFEBABE;  // Thread 0 écrit signature
    }
    if (gid == max_threads - 1) {
        counter[2] = 0xDEADBEEF;  // Dernier thread écrit signature
    }
}

; Made with Bob
