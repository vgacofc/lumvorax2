__kernel void test_t11_compute_minimal(__global uint* output)
{
    uint gid = get_global_id(0);
    output[gid] = gid + 0xCAFE0000;
}

