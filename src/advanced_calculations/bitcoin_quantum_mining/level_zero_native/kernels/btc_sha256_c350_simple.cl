/*
 * LumVorax C350 - Kernel Bitcoin Mining ULTRA SIMPLE
 * 
 * CORRECTION BUG C349 : Pas d'atomics, chaque thread écrit son résultat
 * 
 * OUTPUT FORMAT (par thread) :
 * - output[gid*3 + 0] = nonce
 * - output[gid*3 + 1] = leading_zeros
 * - output[gid*3 + 2] = hash[0] (premier uint32 du hash pour validation)
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

/* SHA-256 Initial Hash Values */
__constant uint H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/* Rotate right */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* SHA-256 Functions */
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)      (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/**
 * SHA-256 Transform
 */
void sha256_transform(__private uint* state, __private const uint* data) {
    uint a, b, c, d, e, f, g, h, t1, t2;
    uint w[64];
    
    /* Initialize working variables */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    /* Prepare message schedule */
    for (int i = 0; i < 16; i++) {
        w[i] = data[i];
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Main loop */
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    /* Add compressed chunk to current hash value */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/**
 * SHA-256 Hash pour Bitcoin (80 bytes header)
 */
void sha256_hash_btc(__private const uint* data, __private uint* hash) {
    uint state[8];
    
    /* Initialize state */
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    /* Process first block (64 bytes) */
    sha256_transform(state, data);
    
    /* Process second block (16 bytes + padding) */
    uint block2[16];
    block2[0] = data[16]; block2[1] = data[17];
    block2[2] = data[18]; block2[3] = data[19];
    block2[4] = 0x80000000;  // Padding
    for (int i = 5; i < 15; i++) {
        block2[i] = 0;
    }
    block2[15] = 640;  // Length in bits (80 * 8)
    
    sha256_transform(state, block2);
    
    /* Copy state to output */
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * SHA-256 Hash pour 32 bytes (second hash)
 */
void sha256_hash_32(__private const uint* data, __private uint* hash) {
    uint state[8];
    uint block[16];
    
    /* Initialize state */
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    /* Prepare block (32 bytes + padding) */
    for (int i = 0; i < 8; i++) {
        block[i] = data[i];
    }
    block[8] = 0x80000000;  // Padding
    for (int i = 9; i < 15; i++) {
        block[i] = 0;
    }
    block[15] = 256;  // Length in bits (32 * 8)
    
    sha256_transform(state, block);
    
    /* Copy state to output */
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * Compte les leading zeros dans un hash SHA-256
 * C350 FIX: Implémentation manuelle sans clz() qui peut bugger
 */
uint count_leading_zeros(__private const uint* hash) {
    uint zeros = 0;
    
    /* Bitcoin utilise little-endian, parcourir de l'index 0 à 7 */
    for (int i = 0; i < 8; i++) {
        uint word = hash[i];
        
        if (word == 0) {
            /* Tous les bits sont à 0 */
            zeros += 32;
        } else {
            /* Compter les bits à 0 manuellement */
            for (int bit = 31; bit >= 0; bit--) {
                if ((word & (1u << bit)) == 0) {
                    zeros++;
                } else {
                    return zeros;
                }
            }
            break;
        }
    }
    
    return zeros;
}

/**
 * Bitcoin Mining Kernel C350 ULTRA SIMPLE
 * 
 * Chaque thread écrit son résultat à sa propre position
 * Pas d'atomics, pas de synchronisation
 */
__kernel void btc_sha256_mining_c350_simple(
    __global const uint* block_header,   /* 20 uint32 (80 bytes) */
    uint nonce_start,                    /* Nonce de départ */
    __global uint* output_buffer         /* Format: [nonce, leading_zeros, hash[0]] par thread */
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;
    
    /* Copier block header en registres privés */
    uint header[20];
    for (int i = 0; i < 19; i++) {
        header[i] = block_header[i];
    }
    
    /* Insérer nonce à la position 19 (bytes 76-79) */
    header[19] = nonce;
    
    /* Premier SHA-256 */
    uint hash1[8];
    sha256_hash_btc(header, hash1);
    
    /* Second SHA-256 */
    uint hash2[8];
    sha256_hash_32(hash1, hash2);
    
    /* Compter leading zeros */
    uint leading_zeros = count_leading_zeros(hash2);
    
    /* Écrire résultat à la position du thread (pas d'atomics) */
    uint output_idx = gid * 3;
    output_buffer[output_idx + 0] = nonce;
    output_buffer[output_idx + 1] = leading_zeros;
    output_buffer[output_idx + 2] = hash2[0];  /* Premier uint32 du hash pour validation */
}

// Made with Bob - C350 Simple Fix


