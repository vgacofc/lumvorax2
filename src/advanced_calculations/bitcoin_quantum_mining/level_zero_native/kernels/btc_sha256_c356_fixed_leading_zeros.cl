/*
 * LumVorax Bitcoin Mining - Kernel C356 FIX LEADING ZEROS
 * 
 * ROOT CAUSE: clz() compte depuis MSB, mais Bitcoin utilise little-endian
 * SOLUTION: Utiliser ctz() (count trailing zeros) ou inverser byte order
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

__constant uint H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)      (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

void sha256_transform_opt(uint* state, __private const uint* data) {
    uint a, b, c, d, e, f, g, h, t1, t2;
    uint w[16];
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    #pragma unroll 4
    for (int i = 0; i < 64; ++i) {
        if (i < 16) {
            w[i] = data[i];
        } else {
            int idx = i & 15;
            w[idx] = SIG1(w[(idx + 14) & 15]) + w[(idx + 9) & 15] + 
                     SIG0(w[(idx + 1) & 15]) + w[idx];
        }
        
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i & 15];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_hash_btc(__private const uint* data, uint* hash) {
    uint state[8];
    
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    sha256_transform_opt(state, data);
    
    uint block2[16];
    block2[0] = data[16]; block2[1] = data[17];
    block2[2] = data[18]; block2[3] = data[19];
    block2[4] = 0x80000000;
    #pragma unroll
    for (int i = 5; i < 15; i++) {
        block2[i] = 0;
    }
    block2[15] = 640;
    
    sha256_transform_opt(state, block2);
    
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

void sha256_hash_32(__private const uint* data, uint* hash) {
    uint state[8];
    uint block[16];
    
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        block[i] = data[i];
    }
    block[8] = 0x80000000;
    #pragma unroll
    for (int i = 9; i < 15; i++) {
        block[i] = 0;
    }
    block[15] = 256;
    
    sha256_transform_opt(state, block);
    
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * C356 FIX: Compte leading zeros CORRECTEMENT pour Bitcoin
 * 
 * Bitcoin utilise little-endian pour difficulty check:
 * - Les leading zeros sont dans les BYTES DE POIDS FAIBLE
 * - hash[7] contient les MSB, hash[0] contient les LSB
 * - Il faut parcourir de hash[7] vers hash[0] (ordre inversé)
 * - Et utiliser clz() sur chaque word (car uint32 est big-endian en mémoire)
 */
uint count_leading_zeros_bitcoin(const uint* hash) {
    uint zeros = 0;
    
    /* Parcourir de hash[7] à hash[0] (MSB à LSB en little-endian Bitcoin) */
    for (int i = 7; i >= 0; i--) {
        uint word = hash[i];
        
        if (word == 0) {
            zeros += 32;
        } else {
            /* clz() compte depuis MSB du uint32, ce qui est correct ici */
            zeros += clz(word);
            break;
        }
    }
    
    return zeros;
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__attribute__((vec_type_hint(uint4)))
__kernel void btc_sha256_mining_c356_fixed(
    __global const uint* block_header,
    __global const uint* target,
    uint nonce_start,
    __global uint* output_buffer
) {
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint nonce = nonce_start + gid;
    
    uint header[20];
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        header[i] = block_header[i];
    }
    header[19] = nonce;
    
    uint hash1[8];
    sha256_hash_btc(header, hash1);
    
    uint hash2[8];
    sha256_hash_32(hash1, hash2);
    
    /* C356 FIX: Utiliser la fonction corrigée */
    uint leading_zeros = count_leading_zeros_bitcoin(hash2);
    
    __local uint best_nonce_local;
    __local uint best_zeros_local;
    
    if (lid == 0) {
        best_nonce_local = 0;
        best_zeros_local = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    uint old_zeros = atomic_max(&best_zeros_local, leading_zeros);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (leading_zeros > old_zeros || (leading_zeros == old_zeros && leading_zeros == best_zeros_local && lid == 0)) {
        best_nonce_local = nonce;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (lid == 0) {
        uint old_zeros = atomic_max(&output_buffer[1], best_zeros_local);
        
        if (best_zeros_local >= old_zeros) {
            output_buffer[0] = best_nonce_local;
            
            #pragma unroll
            for (int i = 0; i < 8; i++) {
                output_buffer[2 + i] = hash2[i];
            }
        }
        
        atomic_add(&output_buffer[10], (uint)get_local_size(0));
    }
}
