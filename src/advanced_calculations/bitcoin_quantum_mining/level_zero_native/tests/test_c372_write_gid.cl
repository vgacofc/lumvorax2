__kernel void test_write_gid(__global uint* output) {
    uint gid = get_global_id(0);
    
    if (gid < 10) {
        output[gid] = gid + 0xC372;
    }
}
