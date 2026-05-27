/* TEST C430 #6: THREAD UNIQUENESS
 * 
 * OBJECTIF: Valider que GPGPU_WALKER lance réellement N threads distincts
 * 
 * MÉTHODE: Chaque thread écrit son ID unique
 * - Thread 0 écrit 0
 * - Thread 1 écrit 1
 * - Thread N écrit N
 * - Si output[i] == i pour tout i → N threads distincts ✅
 * - Si output[i] != i → GPGPU_WALKER mal configuré ❌
 * 
 * VALIDATION:
 * - Lancer avec 256 threads
 * - Vérifier output[0]=0, output[1]=1, ..., output[255]=255
 * - Si pattern correct → GPGPU_WALKER correct ✅
 * - Si pattern incorrect → GPGPU_WALKER bug (threads dupliqués/manquants) ❌
 */

__kernel void test_thread_uniqueness(__global uint* output) {
    uint gid = get_global_id(0);
    
    /* Chaque thread écrit son ID global unique
     * 
     * Si GPGPU_WALKER correct:
     * - Thread 0 écrit output[0] = 0
     * - Thread 1 écrit output[1] = 1
     * - Thread N écrit output[N] = N
     * 
     * Si GPGPU_WALKER incorrect:
     * - Threads dupliqués: output[i] écrasé plusieurs fois
     * - Threads manquants: output[i] reste 0
     * - IDs incorrects: output[i] != i
     */
    output[gid] = gid;
    
    /* VALIDATION ADDITIONNELLE: Écrire aussi dans position miroir
     * pour détecter collisions d'adresses
     */
    uint mirror_offset = get_global_size(0) + gid;
    output[mirror_offset] = gid ^ 0xFFFFFFFF;  /* Complément pour détecter overwrites */
}

/* PROTOCOLE TEST:
 * 
 * 1. Allouer output buffer 2× taille (512 uint32 pour 256 threads)
 * 2. Initialiser output à 0xDEADBEEF (pattern détectable)
 * 3. Lancer kernel avec 256 threads
 * 4. Vérifier:
 *    a) output[0..255] == [0, 1, 2, ..., 255]
 *    b) output[256..511] == [0xFFFFFFFF, 0xFFFFFFFE, ..., 0xFFFFFF00]
 * 
 * RÉSULTATS ATTENDUS:
 * 
 * CAS 1: GPGPU_WALKER correct ✅
 * - output[i] = i pour i ∈ [0, 255]
 * - output[256+i] = i ^ 0xFFFFFFFF pour i ∈ [0, 255]
 * - Aucun 0xDEADBEEF restant
 * 
 * CAS 2: GPGPU_WALKER lance 1 seul thread ❌
 * - output[0] = 0
 * - output[1..255] = 0xDEADBEEF (non modifiés)
 * - output[256] = 0xFFFFFFFF
 * - output[257..511] = 0xDEADBEEF (non modifiés)
 * 
 * CAS 3: GPGPU_WALKER lance N threads mais IDs incorrects ❌
 * - output[i] != i pour certains i
 * - Certains output[i] = 0xDEADBEEF (threads manquants)
 * - Certains output[i] écrasés plusieurs fois (threads dupliqués)
 * 
 * CAS 4: GPU n'exécute pas du tout ❌
 * - Tous output[i] = 0xDEADBEEF (aucune modification)
 */

; Made with Bob
