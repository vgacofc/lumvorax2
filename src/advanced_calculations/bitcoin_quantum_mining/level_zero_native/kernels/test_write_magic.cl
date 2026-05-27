/*
 * LumVorax C345 — Kernel de validation relocations
 * test_write_magic.cl — Écrit une valeur magique pour valider GPU write
 *
 * Ce kernel écrit simplement une valeur magique (0xC345BEEF) dans output[0]
 * pour prouver que le GPU peut écrire correctement via les relocations.
 */

__kernel void write_magic(__global uint* output) {
    uint gid = get_global_id(0);
    
    /* Seul le premier thread écrit */
    if (gid == 0) {
        output[0] = 0xC345BEEF;  /* Valeur magique C345 */
        output[1] = 0x12345678;  /* Valeur test */
        output[2] = gid;         /* ID thread (devrait être 0) */
    }
}
