/*
 * TEST C372 - NONCE EFFECT VALIDATION
 * 
 * TEST CRITIQUE : Valider que le nonce affecte réellement le hash
 * 
 * Ce test calcule SHA-256 pour 2 nonces différents :
 * - nonce = 0x00000000
 * - nonce = 0x00000001
 * 
 * ATTENDU : hash(0) != hash(1)
 * 
 * Si hash(0) == hash(1) → pipeline SHA-256 encore cassé
 * Si hash(0) != hash(1) → mining réel activé ✅
 */

/* Constantes SHA-256 */
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

__kernel void test_nonce_effect(
    __global const uint* block_header,  /* 19 uint32 (sans nonce) */
    __global uint* output               /* 16 uint32: hash[0..7] puis hash[8..15] */
) {
    uint gid = get_global_id(0);
    
    /* Thread 0: nonce = 0x00000000 */
    /* Thread 1: nonce = 0x00000001 */
    if (gid > 1) return;
    
    uint nonce = gid;  /* 0 ou 1 */
    
    /* C372 TEST: Construire block header Bitcoin complet (20 uint32) */
    uint data[20];
    
    /* Copier les 19 premiers uint32 */
    for (int i = 0; i < 19; i++) {
        data[i] = block_header[i];
    }
    
    /* Insérer le nonce à la position 19 */
    data[19] = nonce;
    
    /* Préparer message schedule w[64] */
    uint w[64];
    
    /* C371 FIX: Copier les 20 uint32 (pas 16 !) */
    for (int i = 0; i < 20; i++) {
        w[i] = data[i];
    }
    
    /* Padding SHA-256 pour 80 bytes */
    w[20] = 0x80000000;
    for (int i = 21; i < 31; i++) {
        w[i] = 0;
    }
    w[31] = 640;  /* 80 bytes × 8 = 640 bits */
    
    /* Extend message schedule */
    for (int i = 20; i < 32; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Initialize hash values (SHA-256 IV) */
    uint h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Compression function */
    uint a = h[0], b = h[1], c = h[2], d = h[3];
    uint e = h[4], f = h[5], g = h[6], h_var = h[7];
    
    for (int i = 0; i < 64; i++) {
        uint t1 = h_var + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h_var = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += h_var;
    
    /* Premier SHA-256 terminé, maintenant second SHA-256 */
    uint hash2[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Préparer message pour second SHA-256 */
    for (int i = 0; i < 8; i++) {
        w[i] = h[i];
    }
    w[8] = 0x80000000;
    for (int i = 9; i < 15; i++) {
        w[i] = 0;
    }
    w[15] = 256;  /* 32 bytes × 8 = 256 bits */
    
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Compression function */
    a = hash2[0]; b = hash2[1]; c = hash2[2]; d = hash2[3];
    e = hash2[4]; f = hash2[5]; g = hash2[6]; h_var = hash2[7];
    
    for (int i = 0; i < 64; i++) {
        uint t1 = h_var + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h_var = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    hash2[0] += a; hash2[1] += b; hash2[2] += c; hash2[3] += d;
    hash2[4] += e; hash2[5] += f; hash2[6] += g; hash2[7] += h_var;
    
    /* Écrire résultat dans output */
    /* Thread 0 écrit à output[0..7] */
    /* Thread 1 écrit à output[8..15] */
    uint offset = gid * 8;
    for (int i = 0; i < 8; i++) {
        output[offset + i] = hash2[i];
    }
}


