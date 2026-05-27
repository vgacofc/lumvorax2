/* C389 - Test écriture via Binding Table Entry (BTI)
 * 
 * PROBLÈME C388: Kernel utilisait register-pointer (r3) au lieu de BTI
 * SOLUTION: Utiliser __global pointer qui mappe automatiquement vers BTI
 * 
 * Sur Gen9, __global pointer génère:
 * - Surface State dans SSH
 * - Binding Table Entry
 * - Dataport message avec surface index
 * 
 * Au lieu de:
 *   mov r4 ← r3 (pointeur direct)
 *   sends [r4]
 * 
 * Génère:
 *   sends avec BTI index + offset
 */

typedef unsigned int uint;

__kernel void test_bti_write(__global uint* output) {
    // Écrire magic number via BTI
    // OpenCL génère automatiquement:
    // - load BTI index depuis binding table
    // - dataport write message
    output[0] = 0xC382ABCD;
    
    // Écrire pattern pour validation
    uint gid = get_global_id(0);
    if (gid < 64) {
        output[gid] = 0xC382AB00 | gid;
    }
}
