/*
 * TEST C427 #1 - VALIDATION ABSOLUE STORE
 * Objectif: Valider pipeline GPU → RAM → CPU
 *
 * Ce kernel est VOLONTAIREMENT ultra-simple pour isoler:
 * - dataport write
 * - surface state
 * - binding table
 * - GPU → RAM
 * - RAM → CPU
 * - mmap cohérent
 * Made with Bob
 */

__kernel void test_store_validation(__global uint* output) {
    // TEST ABSOLU: Écrire valeur magique
    output[0] = 0x12345678;
    
    // TEST SECONDAIRE: Vérifier get_global_id()
    uint gid = get_global_id(0);
    if (gid < 256) {
        output[gid + 1] = gid;
    }
}
