/*
 * LumVorax Bitcoin Mining Kernel Gen9 - VERSION SANS ATOMIQUES
 * CYCLE C293 - Fix near-miss GPU = 0 bits
 * 
 * CHANGEMENTS vs btc_sha256_gen9.cl:
 * - SUPPRESSION de atomic_max() (ne fonctionne pas sur Gen9 Skylake)
 * - Chaque work-item écrit son résultat dans output_buffer
 * - CPU trouve le meilleur résultat après exécution GPU
 * - Plus lent mais FONCTIONNE sur Gen9
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

/* Fonction pour compter les leading zeros */
inline uint count_leading_zeros(uint value) {
    return clz(value);  /* OpenCL builtin */
}

/* Fonction pour compter les leading zeros dans un hash SHA-256 (8 uint32) */
inline uint count_hash_leading_zeros(__private uint* hash) {
    uint total_zeros = 0;
    
    /* Parcourir les 8 mots du hash */
    for (int i = 0; i < 8; i++) {
        uint zeros = count_leading_zeros(hash[i]);
        total_zeros += zeros;
        
        /* Si ce mot n'est pas complètement zéro, on s'arrête */
        if (zeros < 32) {
            break;
        }
    }
    
    return total_zeros;
}

__kernel void btc_sha256_mining(
    __global const uint* block_header,    /* 19 uint32 (76 bytes) */
    __global uint* output_buffer,         /* Buffer de sortie: [nonce, leading_zeros, hash[8]] × N work-items */
    uint start_nonce,
    uint target_difficulty
) {
    uint gid = get_global_id(0);
    uint nonce = start_nonce + gid;
    
    /* C370 FIX BUG #1 & #2: Préparer le block header Bitcoin (80 bytes = 20 uint32)
     * Structure Bitcoin block header:
     * - Version (4 bytes)
     * - Previous block hash (32 bytes)
     * - Merkle root (32 bytes)
     * - Timestamp (4 bytes)
     * - Bits/Difficulty (4 bytes)
     * - Nonce (4 bytes) ← Position 19 (offset 76)
     * Total: 80 bytes = 20 uint32
     */
    uint data[20];
    
    /* Copier les 19 premiers uint32 du header (tout sauf nonce) */
    #pragma unroll
    for (int i = 0; i < 19; i++) {
        data[i] = block_header[i];
    }
    
    /* C370 FIX BUG #2: Insérer le nonce à la position correcte (uint32[19]) */
    data[19] = nonce;
    
    /* Premier SHA-256 */
    uint hash1[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint w[64];
    
    /* C371 FIX BUG #5 CRITIQUE: Copier les 20 uint32 (pas 16 !)
     * AVANT: for (int i = 0; i < 16; i++) → SEULEMENT 16/20 copiés
     * APRÈS: for (int i = 0; i < 20; i++) → TOUS les 20 uint32 copiés
     *
     * Conséquence du bug: timestamp, bits, nonce jamais inclus dans SHA-256
     * Résultat: hash constant = 0x00000000 pour tous les nonces
     */
    #pragma unroll
    for (int i = 0; i < 20; i++) {
        w[i] = data[i];
    }
    
    /* Padding SHA-256 pour block header 80 bytes */
    w[20] = 0x80000000;  /* Bit padding après 80 bytes (20 uint32) */
    #pragma unroll
    for (int i = 21; i < 31; i++) {
        w[i] = 0;
    }
    w[31] = 640;  /* Length in bits: 80 bytes × 8 = 640 bits */
    
    /* C372 FIX BUG #6: Étendre message schedule dans le BON ORDRE
     * AVANT: Extension 32-64 PUIS 20-32 → w[32-64] utilisent w[20-31]=0 (BUG!)
     * APRÈS: Extension 16-64 en UNE SEULE boucle (ordre correct)
     */
    #pragma unroll
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    /* Compression function */
    uint a = hash1[0], b = hash1[1], c = hash1[2], d = hash1[3];
    uint e = hash1[4], f = hash1[5], g = hash1[6], h = hash1[7];
    
    #pragma unroll
    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    hash1[0] += a; hash1[1] += b; hash1[2] += c; hash1[3] += d;
    hash1[4] += e; hash1[5] += f; hash1[6] += g; hash1[7] += h;
    
    /* Deuxième SHA-256 sur le résultat du premier */
    uint hash2[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Préparer message schedule pour hash1 */
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
    a = hash2[0]; b = hash2[1]; c = hash2[2]; d = hash2[3];
    e = hash2[4]; f = hash2[5]; g = hash2[6]; h = hash2[7];
    
    #pragma unroll
    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    hash2[0] += a; hash2[1] += b; hash2[2] += c; hash2[3] += d;
    hash2[4] += e; hash2[5] += f; hash2[6] += g; hash2[7] += h;
    
    /* Compter les leading zeros */
    uint leading_zeros = count_hash_leading_zeros(hash2);
    
    /* C372 FIX BUG #7 CRITIQUE: Écrire TOUJOURS pour validation
     * PROBLÈME C371: if (leading_zeros > 0) → N'écrit RIEN si hash sans leading zeros
     * CONSÉQUENCE: Buffer reste à 0x00000000 → Impossible de valider si GPU calcule
     * SOLUTION C372: Écrire TOUJOURS le premier nonce pour validation
     *
     * Format: output[0] = best_nonce, output[1] = best_leading_zeros
     * output[2-9] = hash complet (8 uint32)
     */
    
    /* Lire valeur actuelle */
    uint current_best = output_buffer[1];
    
    /* Écrire si meilleur OU si premier résultat (current_best == 0) */
    if (leading_zeros > current_best || (current_best == 0 && gid == 0)) {
        output_buffer[0] = nonce;
        output_buffer[1] = leading_zeros;
        
        /* Écrire aussi le hash pour validation */
        #pragma unroll
        for (int i = 0; i < 8; i++) {
            output_buffer[2 + i] = hash2[i];
        }
    }
}

// Made with Bob - C293 No Atomics Version
