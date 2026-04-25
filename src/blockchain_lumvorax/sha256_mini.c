/* sha256_mini.c — Implémentation portable SHA-256 (RFC 6234).
 *
 * Cycle C95. Évite la dépendance externe pour le squelette blockchain.
 * Sera remplacé par sha256_neural.c du minier BTC en production.
 *
 * Source : implémentation standard FIPS 180-4. Pas de dépendance externe.
 */
#include <stdint.h>
#include <string.h>

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_compress(uint32_t H[8], const uint8_t block[64]) {
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = ((uint32_t)block[i*4]   << 24) |
               ((uint32_t)block[i*4+1] << 16) |
               ((uint32_t)block[i*4+2] << 8)  |
                (uint32_t)block[i*4+3];
    }
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = ROTR(W[i-15],7) ^ ROTR(W[i-15],18) ^ (W[i-15] >> 3);
        const uint32_t s1 = ROTR(W[i-2],17) ^ ROTR(W[i-2],19)  ^ (W[i-2] >> 10);
        W[i] = W[i-16] + s0 + W[i-7] + s1;
    }
    uint32_t a=H[0],b=H[1],c=H[2],d=H[3],e=H[4],f=H[5],g=H[6],h=H[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t S1 = ROTR(e,6) ^ ROTR(e,11) ^ ROTR(e,25);
        const uint32_t ch = (e & f) ^ ((~e) & g);
        const uint32_t T1 = h + S1 + ch + K[i] + W[i];
        const uint32_t S0 = ROTR(a,2) ^ ROTR(a,13) ^ ROTR(a,22);
        const uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t T2 = S0 + mj;
        h=g; g=f; f=e; e=d+T1; d=c; c=b; b=a; a=T1+T2;
    }
    H[0]+=a; H[1]+=b; H[2]+=c; H[3]+=d;
    H[4]+=e; H[5]+=f; H[6]+=g; H[7]+=h;
}

void sha256_lumvorax(const uint8_t *data, size_t len, uint8_t out[32]) {
    uint32_t H[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };
    /* Process complete 64-byte blocks */
    size_t i = 0;
    while (i + 64 <= len) {
        sha256_compress(H, data + i);
        i += 64;
    }
    /* Padding */
    uint8_t pad[128];
    size_t rem = len - i;
    memcpy(pad, data + i, rem);
    pad[rem] = 0x80;
    size_t pad_len = (rem < 56) ? 64 : 128;
    memset(pad + rem + 1, 0, pad_len - rem - 1 - 8);
    const uint64_t bits = (uint64_t)len * 8;
    for (int b = 0; b < 8; ++b) {
        pad[pad_len - 1 - b] = (uint8_t)(bits >> (b * 8));
    }
    for (size_t p = 0; p < pad_len; p += 64) {
        sha256_compress(H, pad + p);
    }
    for (int j = 0; j < 8; ++j) {
        out[j*4]   = (uint8_t)(H[j] >> 24);
        out[j*4+1] = (uint8_t)(H[j] >> 16);
        out[j*4+2] = (uint8_t)(H[j] >> 8);
        out[j*4+3] = (uint8_t)(H[j]);
    }
}
