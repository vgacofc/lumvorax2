/* C342 KERNEL ABSOLU MINIMAL - Aucune fonction, juste écriture directe */
__kernel void test_absolute_minimal(__global uint* output) {
    /* Thread 0 écrit 4 valeurs hardcodées */
    output[0] = 0x12345678;
    output[1] = 0xABCDEF00;
    output[2] = 0xDEADBEEF;
    output[3] = 0xCAFEBABE;
}
