/**
 * C336 TEST KERNEL - Écriture valeur magique
 *
 * Objectif: Prouver que le GPU peut écrire dans output_bo
 *
 * Ce kernel écrit simplement 0xC336BEEF dans output[0]
 * Si après dispatch output[0] = 0xC336BEEF → GPU fonctionne ✅
 * Si output[0] reste 0x00000000 → GPU ne peut pas écrire ❌
 */

__kernel void test_write_magic(__global unsigned int* output) {
    // Thread 0 écrit la valeur magique
    unsigned int gid = get_global_id(0);
    if (gid == 0) {
        output[0] = 0xC336BEEF;  // Valeur magique C336
        output[1] = 0xDEADC0DE;  // Valeur secondaire
    }
}

; Made with Bob
