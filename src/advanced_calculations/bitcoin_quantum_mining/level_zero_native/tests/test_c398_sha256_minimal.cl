// C398: SHA256 Minimal Implementation for Bitcoin Mining
// Objective: Calculate real SHA256 hash on GPU

// SHA256 Constants (first 32 bits of fractional parts of cube roots of first 64 primes)
__constant uint K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// SHA256 Functions
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

// Bitcoin block header structure (80 bytes)
typedef struct {
    uint version;        // 4 bytes
    uint prev_hash[8];   // 32 bytes
    uint merkle_root[8]; // 32 bytes
    uint timestamp;      // 4 bytes
    uint bits;           // 4 bytes
    uint nonce;          // 4 bytes
} BlockHeader;

// SHA256 single block (512 bits = 64 bytes)
void sha256_transform(uint state[8], const uint data[16]) {
    uint a, b, c, d, e, f, g, h, t1, t2;
    uint W[64];
    
    // Prepare message schedule
    for (int i = 0; i < 16; i++) {
        W[i] = data[i];
    }
    for (int i = 16; i < 64; i++) {
        W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
    }
    
    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    
    // Main loop (64 rounds)
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    // Add compressed chunk to current hash value
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Count leading zeros in hash
uint count_leading_zeros(const uint hash[8]) {
    uint zeros = 0;
    for (int i = 0; i < 8; i++) {
        uint val = hash[i];
        if (val == 0) {
            zeros += 32;
        } else {
            // Count leading zeros in this word
            zeros += clz(val);
            break;
        }
    }
    return zeros;
}

__kernel void bitcoin_mine(
    __global const uint* block_header,  // 20 uints (80 bytes)
    __global uint* results,             // Output: hash results
    uint nonce_start
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;
    
    // Prepare block header with current nonce
    uint header[20];
    for (int i = 0; i < 19; i++) {
        header[i] = block_header[i];
    }
    header[19] = nonce;  // Set nonce
    
    // SHA256 initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    // First SHA256: hash the 80-byte block header
    // Block 1: first 64 bytes (16 uints)
    sha256_transform(state, header);
    
    // Block 2: remaining 16 bytes + padding
    uint block2[16] = {
        header[16], header[17], header[18], header[19],
        0x80000000,  // Padding bit
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,           // Length high
        640          // Length low (80 bytes = 640 bits)
    };
    sha256_transform(state, block2);
    
    // Second SHA256: hash the result (double SHA256 for Bitcoin)
    uint hash1[8];
    for (int i = 0; i < 8; i++) {
        hash1[i] = state[i];
    }
    
    // Reset state for second hash
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
    
    // Hash the 32-byte result + padding
    uint block3[16] = {
        hash1[0], hash1[1], hash1[2], hash1[3],
        hash1[4], hash1[5], hash1[6], hash1[7],
        0x80000000,  // Padding
        0, 0, 0, 0, 0,
        0,           // Length high
        256          // Length low (32 bytes = 256 bits)
    };
    sha256_transform(state, block3);
    
    // Count leading zeros
    uint leading_zeros = count_leading_zeros(state);
    
    // Write result: leading_zeros in high 16 bits, nonce in low 16 bits
    // This allows CPU to find best result easily
    results[gid] = (leading_zeros << 16) | (nonce & 0xFFFF);
}
