/*
 * LumVorax C391 - Test TRIVIAL Sans Optimisation
 * Objectif: Écrire 0xDEADBEEF sans aucune logique complexe
 */

__kernel void test_trivial(__global unsigned int* output) {
    output[0] = 0xDEADBEEF;
}
