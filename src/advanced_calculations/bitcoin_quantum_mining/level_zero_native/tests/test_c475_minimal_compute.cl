/*
 * KERNEL MINIMAL C475 - PROOF OF COMPUTE EXECUTION
 * 
 * Objectif: Prouver que le GPU exécute réellement du code compute
 * Stratégie: Écrire une signature unique en mémoire
 * 
 * Ce kernel est VOLONTAIREMENT trivial pour isoler l'exécution compute
 * des problèmes de SHA256, CURBE complexe, etc.
 */

__kernel void minimal_write(__global unsigned int* output) {
    output[0] = 0xDEADBEEF;
    output[1] = 0xCAFEBABE;
    output[2] = 0x12345678;
    output[3] = 0xABCDEF00;
}
