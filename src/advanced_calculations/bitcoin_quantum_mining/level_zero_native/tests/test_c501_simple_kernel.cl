__kernel void simple_write(__global uint *output)
{
    output[0] = 0xDEADBEEF;
}

; Made with Bob
