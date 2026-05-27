// Test C361 - Pattern GID pour identifier offset exact ecriture GPU
// Objectif: Prouver ou GPU ecrit reellement (BTI/SIMD/offset)

__kernel void test_gid_pattern(__global uint* output) {
    uint gid = get_global_id(0);
    
    // Chaque thread ecrit son GID a son offset
    // Si GPU fonctionne correctement:
    //   output[0] = 0
    //   output[1] = 1
    //   output[2] = 2
    //   etc.
    output[gid] = gid;
}
