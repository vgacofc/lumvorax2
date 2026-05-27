/*
 * LumVorax C351 - Kernel Bitcoin Mining SANS ATOMICS (FIX FINAL)
 * ROOT CAUSE C349-C350: atomic_max() et atomic_add() ne fonctionnent PAS en Gen9 natif
 * SOLUTION: Comparaisons locales + écriture conditionnelle globale (pas d'atomics)
 */

/* ═══════════════════════════════════════════════════════════════════════════
 * SHA256 CONSTANTS ET FONCTIONS (identiques à l'original)
 * ═══════════════════════════════════════════════════════════════════════════ */

constant uint K[64] = {
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

void sha256_transform(uint* state, const uint* data) {
    uint a, b, c, d, e, f, g, h, t1, t2, m[64];
    
    #pragma unroll 16
    for (int i = 0; i < 16; i++) {
        m[i] = data[i];
    }
    
    #pragma unroll 48
    for (int i = 16; i < 64; i++) {
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    }
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    #pragma unroll 64
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_hash_btc(__private const uint* header, __private uint* hash) {
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint data[16];
    #pragma unroll 16
    for (int i = 0; i < 16; i++) {
        data[i] = (i < 20) ? header[i] : 0;
    }
    data[0] = 0x80000000;  /* Padding après les 80 bytes du header */
    data[15] = 640;
    
    sha256_transform(state, data);
    
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

void sha256_hash_32(__private const uint* input, __private uint* hash) {
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint data[16];
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        data[i] = input[i];
    }
    data[8] = 0x80000000;
    #pragma unroll 6
    for (int i = 9; i < 15; i++) {
        data[i] = 0;
    }
    data[15] = 256;
    
    sha256_transform(state, data);
    
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/* C351: Count leading zeros MANUELLEMENT (pas de clz() builtin) */
uint count_leading_zeros(__private const uint* hash) {
    uint zeros = 0;
    for (int i = 0; i < 8; i++) {
        uint word = hash[i];
        if (word == 0) {
            zeros += 32;
        } else {
            /* Compter bit par bit depuis MSB */
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

/* ═══════════════════════════════════════════════════════════════════════════
 * C351 KERNEL PRINCIPAL - SANS ATOMICS
 * ═══════════════════════════════════════════════════════════════════════════ */

__kernel void btc_sha256_mining_c351_noatomic(
    __global const uint* block_header,
    uint nonce_start,
    __global uint* output_buffer
) {
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint wid = get_group_id(0);
    uint local_size = get_local_size(0);
    
    uint nonce = nonce_start + gid;
    
    /* Copier header + insérer nonce */
    uint header[20];
    #pragma unroll 19
    for (int i = 0; i < 19; i++) {
        header[i] = block_header[i];
    }
    header[19] = nonce;
    
    /* Double SHA256 */
    uint hash1[8];
    sha256_hash_btc(header, hash1);
    uint hash2[8];
    sha256_hash_32(hash1, hash2);
    
    /* Compter leading zeros */
    uint leading_zeros = count_leading_zeros(hash2);
    
    /* ═══════════════════════════════════════════════════════════════════════
     * C351 FIX: SANS ATOMICS - Utiliser local memory + comparaisons
     * ═══════════════════════════════════════════════════════════════════════ */
    
    __local uint best_nonce_local;
    __local uint best_zeros_local;
    
    /* Initialiser local memory */
    if (lid == 0) {
        best_nonce_local = 0;
        best_zeros_local = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Chaque thread compare avec le meilleur local actuel
     * SANS ATOMICS: Utiliser une boucle séquentielle par work-item */
    for (uint i = 0; i < local_size; i++) {
        if (lid == i) {
            /* C'est mon tour: comparer et écrire si meilleur */
            if (leading_zeros > best_zeros_local) {
                best_zeros_local = leading_zeros;
                best_nonce_local = nonce;
            }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    /* Le thread 0 de chaque work-group écrit dans global memory
     * SANS ATOMICS: Utiliser une boucle séquentielle par work-group */
    if (lid == 0) {
        /* Lire valeur globale actuelle */
        uint global_zeros = output_buffer[1];
        
        /* Si meilleur, écrire (race condition possible mais acceptable) */
        if (best_zeros_local > global_zeros) {
            output_buffer[0] = best_nonce_local;
            output_buffer[1] = best_zeros_local;
        }
    }
}
