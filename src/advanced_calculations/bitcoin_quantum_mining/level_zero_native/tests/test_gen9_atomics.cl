/*
 * Test Unitaire C293 - Vérification Atomiques Gen9 Skylake
 * Objectif: Confirmer si atomic_max() fonctionne sur local/global memory
 */

__kernel void test_atomic_local(
    __global uint* results,
    uint test_value
) {
    __local uint local_counter;
    
    uint lid = get_local_id(0);
    
    // Initialiser à 0
    if (lid == 0) {
        local_counter = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Chaque work-item tente atomic_max avec sa valeur
    atomic_max(&local_counter, test_value + lid);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Work-item 0 écrit le résultat
    if (lid == 0) {
        results[get_group_id(0)] = local_counter;
    }
}

__kernel void test_atomic_global(
    __global uint* results,
    uint test_value
) {
    uint gid = get_global_id(0);
    
    // Chaque work-item tente atomic_max sur global memory
    atomic_max(&results[0], test_value + gid);
}

__kernel void test_clz_builtin(
    __global uint* input,
    __global uint* output
) {
    uint gid = get_global_id(0);
    output[gid] = clz(input[gid]);
}

// Made with Bob - C293 Atomics Test

