// Test C362 - FIX ROOT CAUSE: 2 arguments pour BTI correct
// PROBLÈME C361: Kernel avec 1 arg → ocloc génère BTI=0 → GPU écrit sur Input!
// SOLUTION C362: Kernel avec 2 args → ocloc génère BTI=1 pour output → GPU écrit sur Output!

__kernel void test_two_args(__global uint* input, __global uint* output) {
    uint gid = get_global_id(0);
    
    // Écrire GID dans output (BTI=1, pas BTI=0!)
    // Attendu: output[0]=0, output[1]=1, output[2]=2, etc.
    output[gid] = gid;
}
