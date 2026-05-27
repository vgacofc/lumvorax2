/* C342 KERNEL DIAGNOSTIC CRITIQUE - Test Surface Access */
__kernel void test_diagnostic(__global uint* output) {
    /* Test #1: Écrire get_global_id(0) pour vérifier payload/CURBE */
    uint gid = get_global_id(0);
    output[0] = gid;
    
    /* Test #2: Écrire constante pour vérifier accès mémoire GPU */
    output[1] = 0x12345678;
    
    /* Test #3: Écrire get_local_id(0) pour vérifier thread dispatch */
    output[2] = get_local_id(0);
    
    /* Test #4: Écrire get_group_id(0) pour vérifier work-group */
    output[3] = get_group_id(0);
}
