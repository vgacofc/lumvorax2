// C414 - Kernel OpenCL ULTRA SIMPLE pour test
// Écrit simplement le thread ID dans le buffer output
// Pattern: output[tid] = (4 << 16) | tid

__kernel void simple_write(__global uint *output) {
    uint tid = get_global_id(0);
    output[tid] = (4 << 16) | tid;
}

