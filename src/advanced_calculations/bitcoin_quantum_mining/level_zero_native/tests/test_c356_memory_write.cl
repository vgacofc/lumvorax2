/*
 * Test C356 - Validation Mémoire Ultra Simple
 * 
 * Objectif: Vérifier que le GPU peut écrire dans output buffer
 * Sans aucun calcul SHA256, juste des écritures simples
 */

__kernel void test_memory_write(
    __global uint* output  // Buffer output: 10 uints
) {
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    
    // Chaque work-item écrit son ID dans output
    // Format: [magic, gid, lid, 0x12345678, 0xDEADBEEF, ...]
    
    if (gid == 0) {
        // Work-item 0 écrit des valeurs magiques
        output[0] = 0xCAFEBABE;  // Magic number
        output[1] = gid;          // Global ID
        output[2] = lid;          // Local ID  
        output[3] = 0x12345678;   // Test pattern 1
        output[4] = 0xDEADBEEF;   // Test pattern 2
        output[5] = 0xABCDEF00;   // Test pattern 3
        output[6] = 0x11223344;   // Test pattern 4
        output[7] = 0x55667788;   // Test pattern 5
        output[8] = 0x99AABBCC;   // Test pattern 6
        output[9] = 0xDDEEFF00;   // Test pattern 7
    }
}
