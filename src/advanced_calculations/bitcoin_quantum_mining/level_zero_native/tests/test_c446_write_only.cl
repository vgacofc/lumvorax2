/*
 * TEST C446 - KERNEL ULTRA-MINIMAL WRITE-ONLY
 * Objectif: Isoler le write path GPU → RAM
 */

__kernel void write_test(__global uint* output)
{
    int gid = get_global_id(0);
    output[gid] = 0xAABBCCDD;
}
