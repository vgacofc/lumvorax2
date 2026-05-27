/*
 * LumVorax Bitcoin Mining - Kernel SHA256 Gen9 avec Near-Miss Detection
 * 
 * CORRECTION C291 : Ce kernel calcule ET retourne les leading zeros
 * 
 * OUTPUT FORMAT (dans output_buffer) :
 * - Bytes 0-3   : best_nonce (uint32_t)
 * - Bytes 4-7   : leading_zeros (uint32_t) 
 * - Bytes 8-39  : best_hash (32 bytes SHA256)
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
 * SHA-256 Transform OPTIMISÉ avec fenêtre glissante
 */
void sha256_transform_opt(uint* state, __private const uint* data) {
    uint a, b, c, d, e, f, g, h, t1, t2;
    uint w[16];  // Fenêtre glissante (-75% scratch memory)
    
    /* Initialize working variables */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    /* Main loop avec message schedule on-the-fly */
    #pragma unroll 4
    for (int i = 0; i < 64; ++i) {
        /* Calculer w[i] on-the-fly */
        if (i < 16) {
            w[i] = data[i];
        } else {
            int idx = i & 15;
            w[idx] = SIG1(w[(idx + 14) & 15]) + w[(idx + 9) & 15] + 
                     SIG0(w[(idx + 1) & 15]) + w[idx];
        }
        
        /* Compression */
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i & 15];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    /* Add compressed chunk to current hash value */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/**
 * SHA-256 Hash OPTIMISÉ pour Bitcoin (80 bytes header)
 */
void sha256_hash_btc(__private const uint* data, uint* hash) {
    uint state[8];
    
    /* Initialize state */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    /* Process first block (64 bytes) */
    sha256_transform_opt(state, data);
    
    /* Process second block (16 bytes + padding) */
    uint block2[16];
    block2[0] = data[16]; block2[1] = data[17];
    block2[2] = data[18]; block2[3] = data[19];
    block2[4] = 0x80000000;  // Padding
    #pragma unroll
    for (int i = 5; i < 15; i++) {
        block2[i] = 0;
    }
    block2[15] = 640;  // Length in bits (80 * 8)
    
    sha256_transform_opt(state, block2);
    
    /* Copy state to output */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * SHA-256 Hash OPTIMISÉ pour 32 bytes (second hash)
 */
void sha256_hash_32(__private const uint* data, uint* hash) {
    uint state[8];
    uint block[16];
    
    /* Initialize state */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    /* Prepare block (32 bytes + padding) */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        block[i] = data[i];
    }
    block[8] = 0x80000000;  // Padding
    #pragma unroll
    for (int i = 9; i < 15; i++) {
        block[i] = 0;
    }
    block[15] = 256;  // Length in bits (32 * 8)
    
    sha256_transform_opt(state, block);
    
    /* Copy state to output */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * Compte les leading zeros dans un hash SHA-256
 * Retourne le nombre de bits à 0 en partant du MSB
 */
uint count_leading_zeros(const uint* hash) {
    uint zeros = 0;
    
    /* C356 FIX: Bitcoin compte les leading zeros depuis le MSB (Most Significant Byte)
     * En little-endian, le MSB est à l'index 7, donc parcourir hash[7]→hash[0]
     * Exemple: hash Bitcoin 0x00000000...0x1dbd981f
     *   hash[7]=0x00000000 (MSB, 32 leading zeros)
     *   hash[6]=0x00000000 (32 leading zeros)
     *   hash[0]=0x1dbd981f (LSB, 3 leading zeros)
     * Total: 64 leading zeros (hash[7] + hash[6]) */
    for (int i = 7; i >= 0; i--) {
        uint word = hash[i];
        
        if (word == 0) {
            /* Tous les bits sont à 0 */
            zeros += 32;
        } else {
            /* Compter les bits à 0 dans ce word */
            zeros += clz(word);  // Count Leading Zeros (builtin OpenCL)
            break;
        }
    }
    
    return zeros;
}

/**
 * Bitcoin Mining Kernel Gen9 avec Near-Miss Detection
 * 
 * CORRECTION C291 : Calcule et retourne les leading zeros
 */
__attribute__((reqd_work_group_size(64, 1, 1)))
__attribute__((vec_type_hint(uint4)))
__kernel void btc_sha256_mining_gen9(
    __global const uint* block_header,   /* 20 uint32 (80 bytes) */
    __global const uint* target,         /* 8 uint32 (256 bits) */
    uint nonce_start,                    /* Nonce de départ */
    __global uint* output_buffer         /* Format: [best_nonce, leading_zeros, hash[8]] */
) {
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint nonce = nonce_start + gid;
    
    /* Copier block header en registres privés */
    uint header[20];
    #pragma unroll
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
    
    /* C356 FIX: Utiliser local memory pour trouver le meilleur dans le workgroup
     * ET sauvegarder le hash correspondant */
    __local uint best_nonce_local;
    __local uint best_zeros_local;
    __local uint best_hash_local[8];
    
    if (lid == 0) {
        best_nonce_local = 0;
        best_zeros_local = 0;
        for (int i = 0; i < 8; i++) {
            best_hash_local[i] = 0;
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* C323 FIX #2: Atomic update avec protection race condition
     * PROBLÈME C322: Tous work-items avec leading_zeros=0 écrivent → race condition
     * SOLUTION: Utiliser atomic_max qui retourne old_value pour identifier le gagnant */
    uint old_zeros = atomic_max(&best_zeros_local, leading_zeros);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* C356 FIX: Seul le work-item qui a AMÉLIORÉ best_zeros_local écrit le nonce ET le hash
     * Si leading_zeros > old_zeros → ce work-item a gagné → écrire nonce + hash */
    if (leading_zeros > old_zeros || (leading_zeros == old_zeros && leading_zeros == best_zeros_local && lid == 0)) {
        best_nonce_local = nonce;
        /* C356 FIX: Sauvegarder le hash correspondant au meilleur nonce */
        for (int i = 0; i < 8; i++) {
            best_hash_local[i] = hash2[i];
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* C356 TEST DIAGNOSTIC: Écrire SANS CONDITION pour tester si GPU calcule vraiment
     * Ce test temporaire écrit leading_zeros et nonce de CHAQUE work-item lid=0
     * pour vérifier si le GPU exécute et calcule des valeurs variées */
    if (lid == 0) {
        /* TEST: Écrire directement sans condition ni atomic */
        output_buffer[0] = leading_zeros;  // Leading zeros de ce work-item
        output_buffer[1] = nonce;          // Nonce de ce work-item
        
        /* Écrire aussi le hash pour inspection */
        #pragma unroll
        for (int i = 0; i < 8; i++) {
            output_buffer[2 + i] = hash2[i];
        }
        
        /* C312 FIX #4: Compteur de hashes calculés (validation exécution)
         * Position output_buffer[10] = compteur total de work-items exécutés */
        atomic_add(&output_buffer[10], (uint)get_local_size(0));
    }
}

/* Made with Bob */
