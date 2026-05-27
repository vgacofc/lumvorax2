// Test C363 - Valeurs magiques pour PROUVER ecriture GPU
// Objectif: Distinguer "GPU ecrit 0" de "GPU n'ecrit pas"

__kernel void test_magic_values(__global uint* output) {
    uint gid = get_global_id(0);
    
    // Ecrire valeur magique + GID
    // Attendu: 0xCAFE0000, 0xCAFE0001, 0xCAFE0002, etc.
    output[gid] = 0xCAFE0000 + gid;
}

