/*
 * LumVorax C390 - Test Surface States Gen9 Complets
 * 
 * OBJECTIF: Valider que Surface States + Binding Table fonctionnent
 * TEST: Écrire magic number 0xC390ABCD dans output buffer
 * VALIDATION: output[0] doit contenir 0xC390ABCD après exécution
 */

__kernel void test_surface_states(__global uint* output) {
    // Test 1: Écrire magic number à output[0]
    output[0] = 0xC390ABCD;
    
    // Test 2: Écrire pattern basé sur global_id
    uint gid = get_global_id(0);
    if (gid < 64) {
        output[gid] = 0xC390AB00 | gid;
    }
    
    // Test 3: Vérifier que toutes les work-items peuvent écrire
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    // Test 4: Work-item 0 écrit le count total
    if (gid == 0) {
        output[64] = get_global_size(0);
    }
}

/* Made with Bob */
