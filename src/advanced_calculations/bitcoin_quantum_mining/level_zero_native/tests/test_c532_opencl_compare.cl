__kernel void minimal_write(__global uint *output) {
    output[0] = 0x12345678;
}
