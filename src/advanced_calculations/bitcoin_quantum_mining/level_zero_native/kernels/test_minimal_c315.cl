// LumVorax C315 - Kernel Minimal pour Test GPU
// Objectif: Prouver que le GPU peut ecrire dans un buffer
// Architecture: 100% i915 DRM natif, 0% OpenCL

__kernel void test_minimal_write(
    __global uint* output_buffer,
    uint magic_value
) {
    /* Récupérer l'ID global du work-item */
    uint gid = get_global_id(0);
    
    /* Écriture simple: gid + magic_value */
    output_buffer[gid] = gid + magic_value;
}

__kernel void test_sentinel_overwrite(
    __global uint* output_buffer
) {
    /* Récupérer l'ID global du work-item */
    uint gid = get_global_id(0);
    
    /* Écraser le sentinel avec une valeur connue */
    if (gid == 0) {
        output_buffer[0] = 0x12345678;  /* Valeur de test */
        output_buffer[1] = 0xABCDEF00;  /* Valeur de test */
    }
}

__kernel void test_atomic_write(
    __global uint* output_buffer
) {
    /* Récupérer l'ID global du work-item */
    uint gid = get_global_id(0);
    
    /* Test atomique: incrémenter un compteur */
    atomic_inc(&output_buffer[0]);
    
    /* Écrire l'ID dans le buffer */
    output_buffer[gid + 1] = gid;
}
