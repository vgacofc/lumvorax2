/*
 * TEST C580 - KERNEL MINIMAL STORE IMMÉDIAT
 * Objectif: Premier write mémoire GPU natif i915 (sans OpenCL runtime)
 * 
 * Recommandation utilisateur: Commencer par kernel ISA minimal
 * qui fait uniquement mov/store immédiat avant SHA256.
 * 
 * Ce kernel isole:
 * - ISA validity Gen9
 * - Surface state validity
 * - Memory write correctness
 * - EU execution réelle
 * 
 * MILESTONE HISTORIQUE: Premier write mémoire GPU natif correct
 */

__kernel void minimal_store(__global uint *output)
{
    output[0] = 0x12345678;
}
