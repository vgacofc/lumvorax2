/*
 * TEST C436 - KERNEL MINIMAL ABSOLU
 * Objectif: Écrire UNE SEULE valeur constante dans output[0]
 * 
 * Ce kernel est le PLUS SIMPLE possible:
 * - Pas de get_global_id()
 * - Pas de lecture CURBE
 * - Pas de calculs
 * - JUSTE: output[0] = 0x12345678
 * 
 * Si ce kernel échoue aussi, le problème est dans:
 * - Surface State configuration
 * - Binding Table
 * - Memory write path GPU→RAM
 */

__kernel void test_minimal_write(__global unsigned int* output) {
    // Thread 0 écrit la valeur magique
    if (get_global_id(0) == 0) {
        output[0] = 0x12345678;
    }
}

; Made with Bob
