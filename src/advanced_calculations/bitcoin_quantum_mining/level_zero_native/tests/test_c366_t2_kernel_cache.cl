__kernel void test_t2_kernel_a(__global uint* output)
{
    uint gid = get_global_id(0);
    output[gid] = 0xAAAA0000 + gid;
}

__kernel void test_t2_kernel_b(__global uint* output)
{
    uint gid = get_global_id(0);
    output[gid] = 0xBBBB0000 + gid;
}

