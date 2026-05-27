// LumVorax Bitcoin Mining - Kernel SHA256 OPTIMISATION REGISTRES
// Solution 3: Réduction 109 → 45 registres pour éliminer spills mémoire
// Objectif: 697ms → 450ms (sous timeout 650ms du driver i915)

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

// OPTIMISATION CLEF: Utiliser seulement 4 variables w au lieu de 16
// Réduction: 16 registres → 4 registres = -12 registres
#define SHA256_ROUND(round, data, w, a, b, c, d, e, f, g, h, t1, t2) \
{ \
    uint w_val; \
    if (round < 16) { \
        w_val = data[round]; \
    } else { \
        w_val = SIG1(w[(round-2)&3]) + w[(round-7)&3] + SIG0(w[(round-15)&3]) + w[(round-16)&3]; \
    } \
    w[round&3] = w_val; \
    t1 = h + EP1(e) + CH(e,f,g) + K[round] + w_val; \
    t2 = EP0(a) + MAJ(a,b,c); \
    h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
}

// SHA256 transform optimisé avec boucle au lieu de macro inline
// Réduction: Pas de duplication code = moins de pression registres
void sha256_transform_opt(__private uint* state, __private const uint* data) {
    uint a = state[0], b = state[1], c = state[2], d = state[3];
    uint e = state[4], f = state[5], g = state[6], h = state[7];
    uint w[4];  // Seulement 4 au lieu de 16
    uint t1, t2;
    
    // 64 rounds en boucle
    for (int round = 0; round < 64; round++) {
        SHA256_ROUND(round, data, w, a, b, c, d, e, f, g, h, t1, t2);
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

__attribute__((intel_reqd_sub_group_size(8)))
__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void btc_sha256_reg_opt(
    __global const uint* block_header,
    __global const uint* target,
    uint nonce_start,
    __global uint* results,
    __global uint* hashes
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;
    
    // OPTIMISATION: Réutiliser même buffer pour tous les blocs
    // Réduction: 20+16+16 = 52 registres → 20 registres = -32 registres
    uint data[20];  // Buffer réutilisable
    uint state[8];  // État SHA-256
    
    // === PREMIER SHA-256 (Block 1) ===
    // Charger header + nonce
    for (int i = 0; i < 19; i++) data[i] = block_header[i];
    data[19] = nonce;
    
    // Initialiser état
    for (int i = 0; i < 8; i++) state[i] = H0[i];
    
    // Transform bloc 1
    sha256_transform_opt(state, data);
    
    // === PREMIER SHA-256 (Block 2 - padding) ===
    // Réutiliser buffer data
    data[0] = data[16]; data[1] = data[17]; data[2] = data[18]; data[3] = data[19];
    data[4] = 0x80000000;
    for (int i = 5; i < 15; i++) data[i] = 0;
    data[15] = 640;  // 80 bytes * 8 bits
    
    // Transform bloc 2
    sha256_transform_opt(state, data);
    
    // === DEUXIÈME SHA-256 (Double hash Bitcoin) ===
    // Sauvegarder résultat premier hash
    uint hash1[8];
    for (int i = 0; i < 8; i++) hash1[i] = state[i];
    
    // Réinitialiser état pour deuxième hash
    for (int i = 0; i < 8; i++) state[i] = H0[i];
    
    // Préparer bloc avec résultat premier hash
    for (int i = 0; i < 8; i++) data[i] = hash1[i];
    data[8] = 0x80000000;
    for (int i = 9; i < 15; i++) data[i] = 0;
    data[15] = 256;  // 32 bytes * 8 bits
    
    // Transform final
    sha256_transform_opt(state, data);
    
    // === ÉCRITURE RÉSULTATS ===
    __global uint* hash_out = hashes + gid * 8;
    for (int i = 0; i < 8; i++) hash_out[i] = state[i];
    
    // Comparaison avec target (big-endian)
    uint result = 1;
    for (int i = 7; i >= 0; i--) {
        if (state[i] > target[i]) { 
            result = 0; 
            break; 
        }
        else if (state[i] < target[i]) break;
    }
    results[gid] = result;
}

// Made with Bob - LumVorax C255V14 Optimized Kernel
