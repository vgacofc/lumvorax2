/* C342 KERNEL ULTRA SIMPLE */
__kernel void test_ultra_simple(__global uint* output) {
    output[0] = 0x12345678;
    output[1] = 0xABCDEF00;
    output[2] = 0xDEADBEEF;
    output[3] = 0xCAFEBABE;
}
