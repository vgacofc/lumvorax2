/**
 * C369 TEST 1: Kernel de validation écriture GPU
 * 
 * OBJECTIF: Vérifier que le GPU peut écrire dans un buffer
 * PATTERN: Chaque thread écrit son GID + 0xC369A000
 * VALIDATION: CPU vérifie que output[gid] == gid + 0xC369A000
 */

__kernel void test_gpu_write(__global uint* output) {
    uint gid = get_global_id(0);
    
    /* Pattern simple: GID + magic number */
    output[gid] = gid + 0xC369A000;
}

