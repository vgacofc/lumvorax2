/* ============================================================================
 * test_diff_zero_sha256_witness.c — Cycle C135
 * ----------------------------------------------------------------------------
 * Cross-witness SHA-256 : prouve que la reconstruction est bit-exacte par
 * comparaison de hash cryptographique en plus du diff bit-à-bit.
 *
 * Pourquoi : un attaquant ne peut pas simultanément forger un buffer dst tel
 * que diff_bits(src,dst)==0 ET sha256(src)==sha256(dst), car la collision
 * SHA-256 est computationnellement infaisible. Le test renforce donc la
 * preuve formelle (A) "trace → reconstruct → diff=0" demandée depuis C129.
 *
 * Implémentation SHA-256 : minimaliste embarquée (FIPS 180-4), pas de dépendance
 * externe. Validée contre vecteur "abc" → ba7816bf...
 *
 * Sortie CSV :
 *   c135_sha256_witness,buffer_size=N,granularity=G,
 *   sha256_src=HEX,sha256_dst=HEX,diff_bytes=X,diff_bits=Y,verdict=PASS|FAIL
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <errno.h>

#include "lum_core.h"
#include "lum_memory_tracer.h"
#include "lum_aligned_alloc_safe.h"

/* ── SHA-256 (FIPS 180-4) embarqué ──────────────────────────────────────── */
static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
#define ROR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#define S0(x) (ROR(x,2)^ROR(x,13)^ROR(x,22))
#define S1(x) (ROR(x,6)^ROR(x,11)^ROR(x,25))
#define s0(x) (ROR(x,7)^ROR(x,18)^((x) >> 3))
#define s1(x) (ROR(x,17)^ROR(x,19)^((x) >> 10))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))

static void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint32_t H[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };
    /* Padding */
    size_t total = len + 1 + 8;
    size_t pad = (total + 63) & ~((size_t)63);
    uint8_t* buf = (uint8_t*)calloc(pad, 1);
    if (!buf) { memset(out, 0, 32); return; }
    memcpy(buf, data, len);
    buf[len] = 0x80;
    uint64_t bitlen = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) buf[pad - 1 - i] = (uint8_t)(bitlen >> (i * 8));

    for (size_t off = 0; off < pad; off += 64) {
        uint32_t W[64];
        for (int i = 0; i < 16; i++) {
            W[i] = ((uint32_t)buf[off+4*i]<<24) | ((uint32_t)buf[off+4*i+1]<<16)
                 | ((uint32_t)buf[off+4*i+2]<<8)|  (uint32_t)buf[off+4*i+3];
        }
        for (int i = 16; i < 64; i++)
            W[i] = s1(W[i-2]) + W[i-7] + s0(W[i-15]) + W[i-16];

        uint32_t a=H[0],b=H[1],c=H[2],d=H[3],e=H[4],f=H[5],g=H[6],h=H[7];
        for (int i = 0; i < 64; i++) {
            uint32_t T1 = h + S1(e) + CH(e,f,g) + K256[i] + W[i];
            uint32_t T2 = S0(a) + MAJ(a,b,c);
            h=g; g=f; f=e; e=d+T1; d=c; c=b; b=a; a=T1+T2;
        }
        H[0]+=a; H[1]+=b; H[2]+=c; H[3]+=d;
        H[4]+=e; H[5]+=f; H[6]+=g; H[7]+=h;
    }
    free(buf);
    for (int i = 0; i < 8; i++) {
        out[4*i  ] = (uint8_t)(H[i] >> 24);
        out[4*i+1] = (uint8_t)(H[i] >> 16);
        out[4*i+2] = (uint8_t)(H[i] >> 8);
        out[4*i+3] = (uint8_t)(H[i]);
    }
}

static void hex32(const uint8_t h[32], char out[65]) {
    static const char* HX = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        out[2*i  ] = HX[h[i] >> 4];
        out[2*i+1] = HX[h[i] & 0xF];
    }
    out[64] = 0;
}

static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t bits = 0;
    for (size_t i = 0; i < n; i++) bits += (uint64_t)__builtin_popcount(a[i] ^ b[i]);
    return bits;
}

static int run_one(const char* outdir, size_t buf_size, lum_trace_granularity_t g,
                   const char* gname) {
    uint8_t* src = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    uint8_t* dst = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    if (!src || !dst) { free(src); free(dst); return 1; }

    uint64_t s = 0xC135C0DEBEEFFEEDULL;
    for (size_t i = 0; i < buf_size; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        src[i] = (uint8_t)(s & 0xFF);
    }
    memset(dst, 0, buf_size);

    char path[512];
    snprintf(path, sizeof(path), "%s/sha_%s.lum", outdir, gname);

    lum_trace_stats_t st;
    memset(&st, 0, sizeof(st));
    if (lum_memory_snapshot_buffer(src, buf_size, path, g, &st) != 0) {
        free(src); free(dst); return 2;
    }
    size_t br = 0;
    if (lum_memory_reconstruct(path, dst, buf_size, &br) != 0) {
        free(src); free(dst); return 3;
    }

    uint8_t hs[32], hd[32];
    sha256(src, buf_size, hs);
    sha256(dst, buf_size, hd);
    char hsx[65], hdx[65];
    hex32(hs, hsx); hex32(hd, hdx);

    uint64_t db = 0;
    for (size_t i = 0; i < buf_size; i++) if (src[i] != dst[i]) db++;
    uint64_t dbits = count_diff_bits(src, dst, buf_size);
    int sha_match = (memcmp(hs, hd, 32) == 0) ? 1 : 0;
    int verdict = (db == 0 && dbits == 0 && sha_match) ? 0 : 1;

    printf("c135_sha256_witness,buffer_size=%zu,granularity=%s,"
           "sha256_src=%s,sha256_dst=%s,sha_match=%d,"
           "diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64 ",verdict=%s\n",
           buf_size, gname, hsx, hdx, sha_match, db, dbits,
           verdict == 0 ? "PASS" : "FAIL");

    free(src); free(dst);
    return verdict;
}

int main(int argc, char** argv) {
    const char* outdir = (argc > 1) ? argv[1] : "/tmp/c135_sha256";
    if (mkdir(outdir, 0755) != 0 && errno != EEXIST) { perror("mkdir"); return 2; }

    /* Self-test SHA-256 sur "abc" → ba7816bf8f01cfea... */
    uint8_t h[32]; sha256((const uint8_t*)"abc", 3, h);
    char hx[65]; hex32(h, hx);
    static const char* EXPECT_ABC =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    if (strcmp(hx, EXPECT_ABC) != 0) {
        fprintf(stderr, "[C135-SHA] SELF-TEST FAIL: got %s\n", hx);
        return 9;
    }
    fprintf(stderr, "[C135-SHA] self-test \"abc\" OK (%s)\n", hx);

    int total_fail = 0;
    total_fail += (run_one(outdir, 4096,        LUM_TRACE_GRANULARITY_PAGE, "PAGE") != 0);
    total_fail += (run_one(outdir, 4096,        LUM_TRACE_GRANULARITY_BYTE, "BYTE") != 0);
    total_fail += (run_one(outdir, 4096,        LUM_TRACE_GRANULARITY_BIT,  "BIT")  != 0);
    total_fail += (run_one(outdir, 64 * 1024,   LUM_TRACE_GRANULARITY_BIT,  "BIT-64K") != 0);

    printf("[C135-SHA-VERDICT] %s (%d failures)\n",
           total_fail == 0 ? "PASS" : "FAIL", total_fail);
    return total_fail == 0 ? 0 : 1;
}
