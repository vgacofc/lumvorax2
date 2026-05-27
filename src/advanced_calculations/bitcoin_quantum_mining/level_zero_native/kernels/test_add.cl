/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_add.cl — Kernel OpenCL C simple pour tests Level Zero
 *
 * CYCLE C198 — Phase 9 — Compilation Kernel SPIR-V
 *
 * Kernel ultra-simple: addition vectorielle
 * Input: a[], b[]
 * Output: c[] = a[] + b[]
 */

__kernel void vector_add(
    __global const float* a,
    __global const float* b,
    __global float* c,
    const unsigned int n
) {
    unsigned int gid = get_global_id(0);
    
    if (gid < n) {
        c[gid] = a[gid] + b[gid];
    }
}

// Made with Bob - LumVorax C198
