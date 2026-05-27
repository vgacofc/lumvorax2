/*
 * KERNEL BITCOIN SHA256 - ADAPTÉ AU HARDWARE INTEL GEN9 RÉEL
 * 
 * PRINCIPE: S'adapter au hardware, PAS imposer des concepts !
 * 
 * CHANGEMENTS vs versions précédentes:
 * 1. PAS d'atomiques (Gen9 Skylake ne les supporte pas correctement)
 * 2. Chaque work-item écrit SON résultat dans SA zone mémoire
 * 3. CPU fait la réduction finale (plus fiable que GPU)
 * 4. Format simple: [nonce, leading_zeros] par work-item
 */

/* Constantes SHA-256 standard */
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

/* Compte les leading zeros dans un hash (8 uint32) */
inline uint count_leading_zeros(__private uint* hash) {
    uint total = 0;
    
    /* Parcourir du MSB au LSB (big-endian Bitcoin) */
    for (int i = 7; i >= 0; i--) {
        uint word = hash[i];
        if (word == 0) {
            total += 32;
        } else {
            /* Compter les zéros dans ce mot */
            total += clz(word);
            break;
        }
    }
    
    return total;
}

/*
 * KERNEL PRINCIPAL - VERSION HARDWARE RÉEL
 * 
 * Input buffer format:
 *   [0-79]: Block header (80 bytes)
 *   [80-83]: start_nonce (uint32)
 *   [84-87]: nonce_count (uint32)
 * 
 * Output buffer format (par work-item):
 *   [gid*2 + 0]: nonce testé
 *   [gid*2 + 1]: leading zeros trouvés
 */
__kernel void btc_sha256_mining(
    __global const uchar* input_buffer,
    __global uint* output_buffer
)
{
    uint gid = get_global_id(0);
    
    /* Lire paramètres depuis input buffer */
    __global const uint* params = (__global const uint*)(input_buffer + 80);
    uint start_nonce = params[0];
    uint nonce_count = params[1];
    
    /* Vérifier limites */
    if (gid >= nonce_count) {
        return;
    }
    
    uint nonce = start_nonce + gid;
    
    /* Copier block header (80 bytes = 20 uint32) */
    uint header[20];
    __global const uint* header_src = (__global const uint*)input_buffer;
    
    #pragma unroll
    for (int i = 0; i < 20; i++) {
        header[i] = header_src[i];
    }
    
    /* Insérer nonce à la position correcte (offset 76 = index 19) */
    header[19] = nonce;
    
    /* === PREMIER SHA-256 === */
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint w[64];
    
    /* Préparer message schedule (premier chunk: 16 premiers uint32) */
    #pragma unroll
    for (int i = 0; i < 16; i++) {
        w[i] = header[i];
    }
    
    #pragma unroll
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Compression function */
    uint a = state[0], b = state[1], c = state[2], d = state[3];
    uint e = state[4], f = state[5], g = state[6], h = state[7];
    
    #pragma unroll
    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    uint hash1[8];
    hash1[0] = state[0] + a;
    hash1[1] = state[1] + b;
    hash1[2] = state[2] + c;
    hash1[3] = state[3] + d;
    hash1[4] = state[4] + e;
    hash1[5] = state[5] + f;
    hash1[6] = state[6] + g;
    hash1[7] = state[7] + h;
    
    /* === DEUXIÈME SHA-256 === */
    state[0] = 0x6a09e667; state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
    state[4] = 0x510e527f; state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
    
    /* Message schedule pour hash1 */
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        w[i] = hash1[i];
    }
    w[8] = 0x80000000;  /* Padding */
    #pragma unroll
    for (int i = 9; i < 15; i++) {
        w[i] = 0;
    }
    w[15] = 256;  /* Length in bits */
    
    #pragma unroll
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Compression function */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    #pragma unroll
    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    uint hash2[8];
    hash2[0] = state[0] + a;
    hash2[1] = state[1] + b;
    hash2[2] = state[2] + c;
    hash2[3] = state[3] + d;
    hash2[4] = state[4] + e;
    hash2[5] = state[5] + f;
    hash2[6] = state[6] + g;
    hash2[7] = state[7] + h;
    
    /* Compter leading zeros */
    uint leading_zeros = count_leading_zeros(hash2);
    
    /* Écrire résultat dans SA zone mémoire (pas d'atomiques !) */
    uint offset = gid * 2;
    output_buffer[offset + 0] = nonce;
    output_buffer[offset + 1] = leading_zeros;
}

/* Made with Bob - Version HARDWARE RÉEL Intel Gen9 */

; Made with Bob
