// C307 TEST MINIMAL: Kernel qui écrit TOUJOURS pour prouver exécution GPU

__kernel void bitcoin_mine_sha256(
    __global const uint* input_buffer,
    __global uint* output_buffer
) {
    uint gid = get_global_id(0);
    
    // PREUVE: Écrire signature si gid==0
    if (gid == 0) {
        output_buffer[0] = 0x12345678;
        output_buffer[1] = gid;
    }
}
