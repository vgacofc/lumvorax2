/*
 * Test C355 - Bitcoin Header Format Validation
 * Compare CPU (OpenSSL) vs GPU on SAME Bitcoin header
 * 
 * This test uses the EXACT header from C353 where CPU found nonce=32
 * to identify if GPU calculates different hash due to:
 * - Endianness issues
 * - Padding problems
 * - Double SHA-256 chaining
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

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void sha256_transform(uint *state, const uchar *data) {
    uint a, b, c, d, e, f, g, h, t1, t2, m[64];
    
    /* Prepare message schedule - BIG ENDIAN */
    for (int i = 0, j = 0; i < 16; i++, j += 4) {
        m[i] = ((uint)data[j] << 24) | ((uint)data[j+1] << 16) |
               ((uint)data[j+2] << 8) | ((uint)data[j+3]);
    }
    for (int i = 16; i < 64; i++) {
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    }
    
    /* Initialize working variables */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    /* Main loop */
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    /* Add to state */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

__kernel void test_bitcoin_header_c355(
    __global uint *output,      /* [0-7]=hash1, [8-15]=hash2, [16]=nonce_tested */
    uint test_nonce             /* Nonce to test (32 from C353) */
) {
    /* Bitcoin block header from C353 test (80 bytes) */
    uchar header[80];
    
    /* Version: 0x20000000 (little-endian in memory) */
    header[0] = 0x00; header[1] = 0x00; header[2] = 0x00; header[3] = 0x20;
    
    /* Previous block hash (32 bytes) - all zeros for test */
    for (int i = 4; i < 36; i++) header[i] = 0x00;
    
    /* Merkle root (32 bytes) - all zeros for test */
    for (int i = 36; i < 68; i++) header[i] = 0x00;
    
    /* Timestamp: 0x00000000 */
    header[68] = 0x00; header[69] = 0x00; header[70] = 0x00; header[71] = 0x00;
    
    /* Bits (difficulty): 0x1d00ffff (little-endian) */
    header[72] = 0xff; header[73] = 0xff; header[74] = 0x00; header[75] = 0x1d;
    
    /* Nonce: test_nonce (little-endian) */
    header[76] = (test_nonce) & 0xFF;
    header[77] = (test_nonce >> 8) & 0xFF;
    header[78] = (test_nonce >> 16) & 0xFF;
    header[79] = (test_nonce >> 24) & 0xFF;
    
    /* First SHA-256: hash1 = SHA256(header) */
    uint hash1[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Process 80 bytes = 64 bytes + 16 bytes */
    sha256_transform(hash1, header);
    
    /* Prepare second block with padding */
    uchar block2[64];
    for (int i = 0; i < 16; i++) block2[i] = header[64 + i];
    block2[16] = 0x80;  /* Padding bit */
    for (int i = 17; i < 56; i++) block2[i] = 0x00;
    /* Length: 80 bytes = 640 bits (big-endian) */
    for (int i = 56; i < 62; i++) block2[i] = 0x00;
    block2[62] = 0x02;  /* 640 >> 8 = 2 */
    block2[63] = 0x80;  /* 640 & 0xFF = 128 */
    
    sha256_transform(hash1, block2);
    
    /* Save hash1 to output[0-7] */
    for (int i = 0; i < 8; i++) {
        output[i] = hash1[i];
    }
    
    /* Second SHA-256: hash2 = SHA256(hash1) */
    uint hash2[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Prepare hash1 as input (32 bytes in big-endian) */
    uchar hash1_bytes[64];
    for (int i = 0; i < 8; i++) {
        hash1_bytes[i*4 + 0] = (hash1[i] >> 24) & 0xFF;
        hash1_bytes[i*4 + 1] = (hash1[i] >> 16) & 0xFF;
        hash1_bytes[i*4 + 2] = (hash1[i] >> 8) & 0xFF;
        hash1_bytes[i*4 + 3] = hash1[i] & 0xFF;
    }
    
    /* Padding for 32 bytes */
    hash1_bytes[32] = 0x80;
    for (int i = 33; i < 56; i++) hash1_bytes[i] = 0x00;
    /* Length: 32 bytes = 256 bits */
    for (int i = 56; i < 62; i++) hash1_bytes[i] = 0x00;
    hash1_bytes[62] = 0x01;  /* 256 >> 8 = 1 */
    hash1_bytes[63] = 0x00;  /* 256 & 0xFF = 0 */
    
    sha256_transform(hash2, hash1_bytes);
    
    /* Save hash2 to output[8-15] */
    for (int i = 0; i < 8; i++) {
        output[8 + i] = hash2[i];
    }
    
    /* Save nonce tested */
    output[16] = test_nonce;
}
