/*
 * Test C354 - SHA-256 FIPS 180-4 Test Vector
 * Input: "abc" (3 bytes)
 * Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
 */

/* SHA-256 Constants */
__constant uint K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define H0_INIT 0x6a09e667
#define H1_INIT 0xbb67ae85
#define H2_INIT 0x3c6ef372
#define H3_INIT 0xa54ff53a
#define H4_INIT 0x510e527f
#define H5_INIT 0x9b05688c
#define H6_INIT 0x1f83d9ab
#define H7_INIT 0x5be0cd19

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

__kernel void test_sha256_fips_abc(__global uint* output) {
    uint W[64];
    uint a, b, c, d, e, f, g, h, t1, t2;
    
    /* Initialize hash */
    output[0] = H0_INIT;
    output[1] = H1_INIT;
    output[2] = H2_INIT;
    output[3] = H3_INIT;
    output[4] = H4_INIT;
    output[5] = H5_INIT;
    output[6] = H6_INIT;
    output[7] = H7_INIT;
    
    /* Prepare W[0..15] for "abc" */
    W[0] = 0x61626380;  /* 'a' 'b' 'c' 0x80 (big-endian) */
    for(int i = 1; i < 15; i++) W[i] = 0;
    W[15] = 24;  /* length in bits */
    
    /* Extend W[16..63] */
    for(int i = 16; i < 64; i++) {
        W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
    }
    
    /* Initialize working variables */
    a = output[0]; b = output[1]; c = output[2]; d = output[3];
    e = output[4]; f = output[5]; g = output[6]; h = output[7];
    
    /* 64 rounds */
    for(int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    /* Add to hash */
    output[0] += a; output[1] += b; output[2] += c; output[3] += d;
    output[4] += e; output[5] += f; output[6] += g; output[7] += h;
}
