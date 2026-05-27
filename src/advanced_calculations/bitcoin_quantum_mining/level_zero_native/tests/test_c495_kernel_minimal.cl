__kernel void minimal_write(__global uint *output) {
    output[0] = 0xDEADBEEF;
    output[1] = 0xCAFEBABE;
    output[2] = 0x12345678;
    output[3] = 0xABCDEF00;
}

