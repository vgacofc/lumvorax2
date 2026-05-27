/*
 * C342 TEST MINIMAL - Kernel de diagnostic Gen9
 * OBJECTIF: Identifier le vrai bug dans le pipeline compute state
 * TESTS PROGRESSIFS:
 * 1. Hardcoded write (test binding/surface state)
 * 2. get_global_id() (test CURBE/payload)
 * 3. Input read (test input surface)
 * 4. Simple calcul (test ALU)
 */

// TEST #1: Hardcoded write
// Si ça échoue → binding/surface state faux
__kernel void test_hardcoded_write(__global uint* output)
{
    output[0] = 0x12345678;
    output[1] = 0xABCDEF00;
    output[2] = 0xDEADBEEF;
    output[3] = 0xCAFEBABE;
}

// TEST #2: get_global_id()
// Si toujours 0 → CURBE/payload cassé
__kernel void test_global_id(__global uint* output)
{
    uint gid = get_global_id(0);
    output[gid] = gid;
}

// TEST #3: Input read
// Si 0 → input surface cassée
__kernel void test_input_read(__global uint* input, __global uint* output)
{
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    output[3] = input[3];
}

// TEST #4: Simple calcul
// Test ALU + memory
__kernel void test_simple_calc(__global uint* input, __global uint* output)
{
    uint gid = get_global_id(0);
    uint value = input[gid];
    output[gid] = value + gid;
}

// TEST #5: Vérifier tous les threads
// Chaque thread écrit son ID
__kernel void test_all_threads(__global uint* output)
{
    uint gid = get_global_id(0);
    if (gid < 256) {
        output[gid] = gid;
    }
}

// TEST #6: Vérifier work-group ID
// Test dimensions work-group
__kernel void test_workgroup_id(__global uint* output)
{
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint wgid = get_group_id(0);
    
    if (gid < 256) {
        output[gid * 3 + 0] = gid;
        output[gid * 3 + 1] = lid;
        output[gid * 3 + 2] = wgid;
    }
}
