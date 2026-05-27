__kernel void test_t1_isa_signature(__global uint* output)
{
    uint gid = get_global_id(0);
    
    uint signatures[16] = {
        0x11111111, 0x22222222, 0x33333333, 0x44444444,
        0x55555555, 0x66666666, 0x77777777, 0x88888888,
        0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
        0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x12345678
    };
    
    output[gid] = signatures[gid % 16];
}

