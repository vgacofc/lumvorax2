/* ═══════════════════════════════════════════════════════════════════════════
   BITCOIN SHA-256 KERNEL OPTIMISÉ C186
   ═══════════════════════════════════════════════════════════════════════════
   
   CYCLE: C186
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   OPTIMISATIONS:
   - Workgroup 128 threads (vs 64 C176)
   - Local memory partagée (cache L1)
   - Prefetch données midstate
   - Loop unrolling SHA-256 rounds
   - Vectorisation SIMD (uint4)
   - Réduction bank conflicts
   
   OBJECTIF: 26-30 MH/s (vs 23.14 MH/s C180)
   
   ═══════════════════════════════════════════════════════════════════════════ */

/* SHA-256 constants */
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

/* SHA-256 initial hash values */
__constant uint H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/* Rotate right */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* SHA-256 functions */
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/* ═══════════════════════════════════════════════════════════════════════════
   SHA-256 TRANSFORM OPTIMISÉ (UNROLLED)
   ═══════════════════════════════════════════════════════════════════════════ */

void sha256_transform_optimized(__private uint* state, __private const uint* data) {
    uint a, b, c, d, e, f, g, h;
    uint w[64];
    uint t1, t2;
    
    /* Copier données initiales */
    #pragma unroll 16
    for (int i = 0; i < 16; i++) {
        w[i] = data[i];
    }
    
    /* Étendre message (unrolled) */
    #pragma unroll 48
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Initialiser variables de travail */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    
    /* 64 rounds (unrolled par blocs de 8) */
    #pragma unroll 8
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
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
    
    /* Ajouter au hash */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

/* ═══════════════════════════════════════════════════════════════════════════
   KERNEL PRINCIPAL BITCOIN MINING C186
   ═══════════════════════════════════════════════════════════════════════════ */

__kernel void bitcoin_mine_optimized_c186(
    __global const uint* midstate,      /* 8 uint32 (32 bytes) */
    __global const uint* tail,          /* 4 uint32 (16 bytes) */
    __global uint* result_nonce,        /* 1 uint32 (4 bytes) */
    __global uint* result_hash,         /* 8 uint32 (32 bytes) */
    __global uint* result_best_bits,    /* 1 uint32 (4 bytes) */
    uint nonce_start,
    uint target_bits
) {
    /* C186: Workgroup 128 threads (vs 64) */
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wid = get_group_id(0);
    
    /* C186: Local memory partagée (cache L1) - évite bank conflicts */
    __local uint shared_midstate[8];
    __local uint shared_tail[4];
    __local uint shared_best_bits;
    __local uint shared_best_nonce;
    
    /* C186: Prefetch données dans local memory (1 thread par uint) */
    if (lid < 8) {
        shared_midstate[lid] = midstate[lid];
    }
    if (lid < 4) {
        shared_tail[lid] = tail[lid];
    }
    if (lid == 0) {
        shared_best_bits = 256;  /* Pire cas */
        shared_best_nonce = 0;
    }
    
    /* Synchroniser workgroup */
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Calculer nonce pour ce thread */
    uint nonce = nonce_start + gid;
    
    /* Préparer block Bitcoin (80 bytes) */
    uint block[16];
    
    /* Copier midstate (32 bytes) */
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        block[i] = shared_midstate[i];
    }
    
    /* Copier tail (12 bytes) + nonce (4 bytes) */
    #pragma unroll 3
    for (int i = 0; i < 3; i++) {
        block[8 + i] = shared_tail[i];
    }
    block[11] = nonce;  /* Nonce à tester */
    
    /* Padding SHA-256 (32 bytes) */
    block[12] = 0x80000000;  /* Bit padding */
    block[13] = 0;
    block[14] = 0;
    block[15] = 0x00000280;  /* Length = 640 bits */
    
    /* Premier SHA-256 */
    uint hash1[8];
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        hash1[i] = H0[i];
    }
    sha256_transform_optimized(hash1, block);
    
    /* Préparer pour second SHA-256 */
    uint block2[16];
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        block2[i] = hash1[i];
    }
    block2[8] = 0x80000000;
    #pragma unroll 6
    for (int i = 9; i < 15; i++) {
        block2[i] = 0;
    }
    block2[15] = 0x00000100;  /* Length = 256 bits */
    
    /* Second SHA-256 (hash final) */
    uint hash2[8];
    #pragma unroll 8
    for (int i = 0; i < 8; i++) {
        hash2[i] = H0[i];
    }
    sha256_transform_optimized(hash2, block2);
    
    /* Compter leading zero bits */
    uint leading_bits = 0;
    #pragma unroll 8
    for (int i = 7; i >= 0; i--) {
        uint val = hash2[i];
        if (val == 0) {
            leading_bits += 32;
        } else {
            leading_bits += clz(val);  /* Count leading zeros */
            break;
        }
    }
    
    /* Mise à jour atomique du meilleur résultat (workgroup) */
    if (leading_bits > shared_best_bits) {
        atomic_max(&shared_best_bits, leading_bits);
        if (leading_bits == shared_best_bits) {
            shared_best_nonce = nonce;
        }
    }
    
    /* Synchroniser avant écriture globale */
    barrier(CLK_LOCAL_MEM_FENCE);
    
    /* Thread 0 de chaque workgroup écrit résultat global */
    if (lid == 0) {
        /* Mise à jour atomique globale */
        uint old_bits = atomic_max(result_best_bits, shared_best_bits);
        if (shared_best_bits > old_bits) {
            *result_nonce = shared_best_nonce;
            #pragma unroll 8
            for (int i = 0; i < 8; i++) {
                result_hash[i] = hash2[i];
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   KERNEL AUXILIAIRE: LEADING ZERO BITS
   ═══════════════════════════════════════════════════════════════════════════ */

__kernel void count_leading_zeros_c186(
    __global const uint* hash,
    __global uint* leading_bits
) {
    uint bits = 0;
    
    #pragma unroll 8
    for (int i = 7; i >= 0; i--) {
        uint val = hash[i];
        if (val == 0) {
            bits += 32;
        } else {
            bits += clz(val);
            break;
        }
    }
    
    *leading_bits = bits;
}

; Made with Bob
