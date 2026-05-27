// Test OpenCL de référence C601
// Kernel ultra-simple pour validation

__kernel void test_write(__global uint *output) {
    output[0] = 0x12345678;
}
