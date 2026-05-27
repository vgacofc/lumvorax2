__kernel void test_minimal_write(__global unsigned int* output) {
    if (get_global_id(0) == 0) {
        output[0] = 0x12345678;
    }
}
; Made with Bob
