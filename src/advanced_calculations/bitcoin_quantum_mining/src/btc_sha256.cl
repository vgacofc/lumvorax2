/*
 * LumVorax C63 — Kernel OpenCL SHA-256 Double-Hash pour Intel UHD 620
 * =======================================================================
 * SHA-256 double-hash (SHA256(SHA256(data))) optimisé pour GPU intégré.
 * Compatible avec Intel OpenCL ICD (opencl-intel) + Mesa Clover/Rusticl.
 *
 * Stratégie UHD 620 :
 *   - Workgroup 64 threads (EU × SIMD8)
 *   - Midstate SHA-256 précalculé côté CPU (80 bytes header BTC)
 *   - GPU calcule uniquement le 2e bloc (nonce + padding)
 *   - NDRange 1D : batch_size work-items en parallèle
 *
 * STANDARD_NAMES.md v4.4 §M-BTC-GPU-C63
 */

/* ── Constantes SHA-256 ─────────────────────────────────────────── */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x,  2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x,  6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)(ROTR(x,  7) ^ ROTR(x, 18) ^ ((x) >>  3))
#define SIG1(x)(ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

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

/* ── SHA-256 single block (512 bits = 16 uint32) ─────────────────── */
void sha256_block(uint* hash, const uint* msg) {
    uint w[64];
    for (int i = 0; i < 16; i++) w[i] = msg[i];
    for (int i = 16; i < 64; i++)
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];

    uint a = hash[0], b = hash[1], c = hash[2], d = hash[3];
    uint e = hash[4], f = hash[5], g = hash[6], h = hash[7];

    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e,f,g) + K[i] + w[i];
        uint t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d;
    hash[4] += e; hash[5] += f; hash[6] += g; hash[7] += h;
}

/* ── Hash initial SHA-256 ─────────────────────────────────────────── */
void sha256_init(uint* hash) {
    hash[0] = 0x6a09e667; hash[1] = 0xbb67ae85;
    hash[2] = 0x3c6ef372; hash[3] = 0xa54ff53a;
    hash[4] = 0x510e527f; hash[5] = 0x9b05688c;
    hash[6] = 0x1f83d9ab; hash[7] = 0x5be0cd19;
}

/*
 * Kernel principal : btc_double_hash
 * ====================================
 * Chaque work-item calcule SHA256(SHA256(header || nonce)).
 * Le header fait 76 bytes, le nonce 4 bytes → 80 bytes total.
 *
 * midstate[8]  : état SHA-256 après le 1er bloc (bytes 0..63)
 * tail[4]      : bytes 64..79 du header (sans le nonce)
 * nonce_start  : premier nonce de ce batch
 * target_bits  : nombre de bits zéro requis (ex: 28)
 * result_nonce : nonce trouvé (-1 si rien)
 * result_hash  : hash final du nonce trouvé
 */
__kernel void btc_double_hash(
    __constant uint* midstate,       /* 8 uint32 — midstate après bloc 0 */
    __constant uint* tail,           /* 4 uint32 — bytes 64..75 du header */
    const uint       nonce_start,    /* nonce de départ */
    const uint       target_bits,    /* bits zéro requis */
    __global  uint*  result_nonce,   /* output: nonce trouvé (0xFFFFFFFF = rien) */
    __global  uint*  result_hash     /* output: 8 uint32 du hash final */
) {
    uint nonce = nonce_start + (uint)get_global_id(0);

    /* ── Bloc 1 : bytes 64..79 = tail[0..2] + nonce + padding ── */
    uint msg1[16];
    /* tail contient les 12 bytes restants du header avant le nonce */
    msg1[0] = tail[0];
    msg1[1] = tail[1];
    msg1[2] = tail[2];
    /* Nonce en little-endian BTC */
    msg1[3] = ((nonce & 0xFF) << 24) |
              (((nonce >> 8) & 0xFF) << 16) |
              (((nonce >> 16) & 0xFF) << 8) |
              ((nonce >> 24) & 0xFF);
    /* Padding SHA-256 pour 80 bytes (640 bits) */
    msg1[4]  = 0x80000000u;
    msg1[5]  = 0x00000000u; msg1[6]  = 0x00000000u;
    msg1[7]  = 0x00000000u; msg1[8]  = 0x00000000u;
    msg1[9]  = 0x00000000u; msg1[10] = 0x00000000u;
    msg1[11] = 0x00000000u; msg1[12] = 0x00000000u;
    msg1[13] = 0x00000000u; msg1[14] = 0x00000000u;
    msg1[15] = 0x00000280u; /* 640 bits en big-endian */

    /* SHA-256 premier round (depuis midstate) */
    uint hash1[8];
    for (int i = 0; i < 8; i++) hash1[i] = midstate[i];
    sha256_block(hash1, msg1);

    /* ── Double hash : SHA256(hash1) ── */
    uint msg2[16];
    /* hash1 big-endian → msg2 */
    for (int i = 0; i < 8; i++) msg2[i] = hash1[i];
    /* Padding SHA-256 pour 32 bytes (256 bits) */
    msg2[8]  = 0x80000000u;
    msg2[9]  = 0x00000000u; msg2[10] = 0x00000000u;
    msg2[11] = 0x00000000u; msg2[12] = 0x00000000u;
    msg2[13] = 0x00000000u; msg2[14] = 0x00000000u;
    msg2[15] = 0x00000100u; /* 256 bits en big-endian */

    uint hash2[8];
    sha256_init(hash2);
    sha256_block(hash2, msg2);

    /* ── Test du résultat : bits zéro en tête ── */
    /* Bitcoin target check : hash2[7] (little-endian last word = MSB du hash) */
    /* On teste les bits dans le dernier mot (big-endian) */
    uint leading = 0;
    for (int wi = 7; wi >= 0 && leading < target_bits; wi--) {
        uint word = hash2[wi];
        /* Compter les bits 0 en tête de ce mot (big-endian) */
        if (word == 0) {
            leading += 32;
        } else {
            /* clz sur uint — bit zéro en tête */
            uint mask = 0x80000000u;
            while (mask && !(word & mask)) { leading++; mask >>= 1; }
            break;
        }
    }

    if (leading >= target_bits) {
        /* Atomique pour éviter les races — premier nonce trouvé gagne */
        uint old = atomic_cmpxchg(result_nonce, 0xFFFFFFFFu, nonce);
        if (old == 0xFFFFFFFFu) {
            for (int i = 0; i < 8; i++) result_hash[i] = hash2[i];
        }
    }
}

/*
 * Kernel utilitaire : btc_count_leading
 * ========================================
 * Compte les leading zeros de chaque hash du batch.
 * Utilisé pour le benchmarking et le suivi du near-miss.
 */
__kernel void btc_count_leading(
    __constant uint* midstate,
    __constant uint* tail,
    const uint       nonce_start,
    __global  uint*  leading_zeros  /* output: 1 uint par work-item */
) {
    uint nonce = nonce_start + (uint)get_global_id(0);

    uint msg1[16];
    msg1[0] = tail[0]; msg1[1] = tail[1]; msg1[2] = tail[2];
    msg1[3] = ((nonce & 0xFF) << 24) | (((nonce >> 8) & 0xFF) << 16) |
              (((nonce >> 16) & 0xFF) << 8) | ((nonce >> 24) & 0xFF);
    msg1[4]  = 0x80000000u; msg1[5]  = 0; msg1[6]  = 0; msg1[7]  = 0;
    msg1[8]  = 0; msg1[9]  = 0; msg1[10] = 0; msg1[11] = 0;
    msg1[12] = 0; msg1[13] = 0; msg1[14] = 0; msg1[15] = 0x00000280u;

    uint hash1[8];
    for (int i = 0; i < 8; i++) hash1[i] = midstate[i];
    sha256_block(hash1, msg1);

    uint msg2[16];
    for (int i = 0; i < 8; i++) msg2[i] = hash1[i];
    msg2[8]  = 0x80000000u; msg2[9]  = 0; msg2[10] = 0; msg2[11] = 0;
    msg2[12] = 0; msg2[13] = 0; msg2[14] = 0; msg2[15] = 0x00000100u;

    uint hash2[8];
    sha256_init(hash2);
    sha256_block(hash2, msg2);

    uint leading = 0;
    for (int wi = 7; wi >= 0 && leading < 256; wi--) {
        uint word = hash2[wi];
        if (word == 0) { leading += 32; }
        else {
            uint mask = 0x80000000u;
            while (mask && !(word & mask)) { leading++; mask >>= 1; }
            break;
        }
    }
    leading_zeros[get_global_id(0)] = leading;
}
