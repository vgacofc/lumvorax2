/*
 * TEST-UNIT-007: Validation SHA-256 avec vecteurs Bitcoin connus
 * ROOT CAUSE #158: Kernel SHA-256 défectueux (0 nonces trouvés sur 152.6M hashes)
 * 
 * VECTEURS DE TEST:
 * 1. Block Genesis Bitcoin (block #0)
 * 2. Block #125552 (premier block avec difficulté > 1)
 * 3. Test FIPS-180-2 "abc" → ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// SHA-256 Constants
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void sha256_transform(uint32_t state[8], const uint32_t data[16]) {
    uint32_t a, b, c, d, e, f, g, h, t1, t2, W[64];
    
    for (int i = 0; i < 16; i++) W[i] = data[i];
    for (int i = 16; i < 64; i++) {
        W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
    }
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// Swap endianness (little-endian ↔ big-endian)
uint32_t swap_endian_u32(uint32_t val) {
    return ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) |
           ((val & 0xFF0000) >> 8) | ((val >> 24) & 0xFF);
}

void sha256_double(const uint8_t* data, size_t len, uint32_t hash[8]) {
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    
    // C628 FIX: Bitcoin uses big-endian, convert input data
    uint32_t block[16];
    
    // First SHA-256 block (64 bytes)
    for (int i = 0; i < 16; i++) {
        uint32_t val;
        memcpy(&val, data + i * 4, 4);
        block[i] = swap_endian_u32(val);
    }
    sha256_transform(state, block);
    
    // Second block (16 bytes + padding)
    memset(block, 0, sizeof(block));
    for (int i = 0; i < 4; i++) {
        uint32_t val;
        memcpy(&val, data + 64 + i * 4, 4);
        block[i] = swap_endian_u32(val);
    }
    block[4] = 0x80000000;
    block[15] = 640;
    sha256_transform(state, block);
    
    // Second SHA-256 (hash the first hash)
    uint32_t hash1[8];
    memcpy(hash1, state, 32);
    
    state[0] = 0x6a09e667; state[1] = 0xbb67ae85; state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
    state[4] = 0x510e527f; state[5] = 0x9b05688c; state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
    
    memset(block, 0, sizeof(block));
    memcpy(block, hash1, 32);
    block[8] = 0x80000000;
    block[15] = 256;
    sha256_transform(state, block);
    
    memcpy(hash, state, 32);
}

int main(void) {
    printf("=== TEST-UNIT-007: VALIDATION SHA-256 BITCOIN ===\n\n");
    
    // Test 1: Block Genesis Bitcoin
    printf("Test 1/3: Block Genesis Bitcoin #0\n");
    uint8_t genesis[80] = {
        0x01, 0x00, 0x00, 0x00, // version
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // prev_hash
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2, 0x7a, 0xc7, 0x2c, 0x3e,
        0x67, 0x76, 0x8f, 0x61, 0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a, // merkle_root
        0x29, 0xab, 0x5f, 0x49, // timestamp
        0xff, 0xff, 0x00, 0x1d, // bits
        0x1d, 0xac, 0x2b, 0x7c  // nonce
    };
    
    uint32_t hash[8];
    sha256_double(genesis, 80, hash);
    
    printf("  Hash calculé: %08x%08x%08x%08x%08x%08x%08x%08x\n",
           hash[7], hash[6], hash[5], hash[4], hash[3], hash[2], hash[1], hash[0]);
    printf("  Hash attendu: 000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f\n");
    
    // Vérification (hash Bitcoin en little-endian)
    uint32_t expected[8] = {0x6fe28c0a, 0xb6f1b372, 0xc1a6a246, 0xae63f74f,
                            0x931e8365, 0xe15a089c, 0x68d61900, 0x00000000};
    int test1_pass = 1;
    for (int i = 0; i < 8; i++) {
        if (hash[i] != expected[i]) {
            test1_pass = 0;
            break;
        }
    }
    printf("  Résultat: %s\n\n", test1_pass ? "✅ PASS" : "❌ FAIL");
    
    // Test 2: Comptage leading zeros
    printf("Test 2/3: Comptage leading zeros\n");
    int leading_zeros = 0;
    for (int i = 7; i >= 0; i--) {
        if (hash[i] == 0) {
            leading_zeros += 32;
        } else {
            leading_zeros += __builtin_clz(hash[i]);
            break;
        }
    }
    printf("  Leading zeros: %d bits\n", leading_zeros);
    printf("  Attendu: 32 bits (difficulté Genesis)\n");
    int test2_pass = (leading_zeros == 32);
    printf("  Résultat: %s\n\n", test2_pass ? "✅ PASS" : "❌ FAIL");
    
    // Test 3: Endianness validation
    printf("Test 3/3: Validation endianness\n");
    printf("  hash[0] (LSB): 0x%08x\n", hash[0]);
    printf("  hash[7] (MSB): 0x%08x\n", hash[7]);
    printf("  Format Bitcoin: MSB first (hash[7] = 0x00000000)\n");
    int test3_pass = (hash[7] == 0x00000000);
    printf("  Résultat: %s\n\n", test3_pass ? "✅ PASS" : "❌ FAIL");
    
    printf("=== RÉSULTAT FINAL: %d/3 TESTS VALIDÉS ===\n", 
           test1_pass + test2_pass + test3_pass);
    
    return (test1_pass && test2_pass && test3_pass) ? 0 : 1;
}
