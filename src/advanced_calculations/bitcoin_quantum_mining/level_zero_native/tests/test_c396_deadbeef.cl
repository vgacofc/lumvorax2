// Test C396 - Validation descriptor OpenCL avec valeur fixe
// Objectif: Prouver que descriptor 0x04025E00 preserve le payload
// Basé sur: Analyse C395 comparative OpenCL vs C391

__kernel void test_c396_deadbeef(__global uint* output) {
    // Thread 0 seulement (comme C391)
    uint gid = get_global_id(0);
    
    if (gid == 0) {
        // Ecrire valeur magique DEADBEEF
        // Si on lit 0xDEADBEEF → descriptor fonctionne
        // Si on lit 0x00000000 → descriptor échoue (comme C391)
        output[0] = 0xDEADBEEF;
    }
}
