/* ============================================================================
 * test_diff_zero_random.c — Cycle C136
 * ----------------------------------------------------------------------------
 * Test diff=0 avec patterns aléatoires de haute qualité (xoshiro256**).
 *
 * Pourquoi xoshiro256** : période 2^256 - 1, équidistribution prouvée jusqu'à
 *   4 dimensions, 64 bits par sortie ; bien meilleur que xorshift32 ou rand()
 *   pour stresser la chaîne snapshot/reconstruct (élimine biais statistiques
 *   qui pourraient masquer un bug dans certains motifs).
 *
 * Référence : Vigna & Blackman, "Scrambled Linear Pseudorandom Number
 *   Generators", ACM Trans. Math. Softw. 2018.
 *
 * Sortie CSV :
 *   c136_random,seed=S,buffer_size=N,granularity=G,
 *   diff_bytes=X,diff_bits=Y,verdict=PASS|FAIL
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

/* xoshiro256** — Vigna & Blackman 2018. */
typedef struct { uint64_t s[4]; } xoshiro256ss_t;

static inline uint64_t rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* Bootstrap via splitmix64 pour éviter état all-zero. */
static uint64_t splitmix64_next(uint64_t* st) {
    uint64_t z = (*st += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void xoshiro256ss_init(xoshiro256ss_t* x, uint64_t seed) {
    uint64_t s = seed ? seed : 0xC136000000000001ULL;
    for (int i = 0; i < 4; i++) x->s[i] = splitmix64_next(&s);
}

static uint64_t xoshiro256ss_next(xoshiro256ss_t* x) {
    const uint64_t result = rotl64(x->s[1] * 5, 7) * 9;
    const uint64_t t = x->s[1] << 17;
    x->s[2] ^= x->s[0];
    x->s[3] ^= x->s[1];
    x->s[1] ^= x->s[2];
    x->s[0] ^= x->s[3];
    x->s[2] ^= t;
    x->s[3] = rotl64(x->s[3], 45);
    return result;
}

static void fill_random(uint8_t* buf, size_t n, uint64_t seed) {
    xoshiro256ss_t st;
    xoshiro256ss_init(&st, seed);
    size_t i = 0;
    while (i + 8 <= n) {
        uint64_t v = xoshiro256ss_next(&st);
        memcpy(buf + i, &v, 8);
        i += 8;
    }
    if (i < n) {
        uint64_t v = xoshiro256ss_next(&st);
        memcpy(buf + i, &v, n - i);
    }
}

static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t bits = 0;
    for (size_t i = 0; i < n; i++) bits += (uint64_t)__builtin_popcount(a[i] ^ b[i]);
    return bits;
}

static int run_one(const char* outdir, uint64_t seed, size_t buf_size,
                   lum_trace_granularity_t g, const char* gname) {
    uint8_t* src = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    uint8_t* dst = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    if (!src || !dst) { free(src); free(dst); return 1; }

    fill_random(src, buf_size, seed);
    memset(dst, 0, buf_size);

    char path[512];
    snprintf(path, sizeof(path), "%s/rnd_%016" PRIx64 "_%zu_%s.lum",
             outdir, seed, buf_size, gname);

    lum_trace_stats_t st;
    memset(&st, 0, sizeof(st));
    if (lum_memory_snapshot_buffer(src, buf_size, path, g, &st) != 0) {
        free(src); free(dst); return 2;
    }
    size_t br = 0;
    if (lum_memory_reconstruct(path, dst, buf_size, &br) != 0) {
        free(src); free(dst); return 3;
    }

    uint64_t db = 0;
    for (size_t i = 0; i < buf_size; i++) if (src[i] != dst[i]) db++;
    uint64_t dbits = count_diff_bits(src, dst, buf_size);
    int verdict = (db == 0 && dbits == 0) ? 0 : 1;

    printf("c136_random,seed=%016" PRIx64 ",buffer_size=%zu,granularity=%s,"
           "diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64 ",verdict=%s\n",
           seed, buf_size, gname, db, dbits, verdict == 0 ? "PASS" : "FAIL");

    free(src); free(dst);
    return verdict;
}

int main(int argc, char** argv) {
    const char* outdir = (argc > 1) ? argv[1] : "/tmp/c136_random";
    if (mkdir(outdir, 0755) != 0 && errno != EEXIST) { perror("mkdir"); return 2; }

    /* 5 seeds × 3 granularités × 2 tailles = 30 tests. */
    static const uint64_t SEEDS[] = {
        0xC136000000000001ULL, 0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL,
        0xFFFFFFFFFFFFFFFFULL, 0x5555555555555555ULL
    };
    static const size_t SIZES[] = { 4096, 65536 };
    static const lum_trace_granularity_t GRANS[] = {
        LUM_TRACE_GRANULARITY_PAGE,
        LUM_TRACE_GRANULARITY_BYTE,
        LUM_TRACE_GRANULARITY_BIT
    };
    static const char* GNAMES[] = { "PAGE", "BYTE", "BIT" };

    int total_fail = 0;
    for (size_t s = 0; s < sizeof(SEEDS)/sizeof(SEEDS[0]); s++) {
        for (size_t z = 0; z < sizeof(SIZES)/sizeof(SIZES[0]); z++) {
            for (size_t g = 0; g < 3; g++) {
                if (run_one(outdir, SEEDS[s], SIZES[z], GRANS[g], GNAMES[g]) != 0) {
                    total_fail++;
                }
            }
        }
    }
    printf("[C136-RND-VERDICT] %s (%d failures)\n",
           total_fail == 0 ? "PASS" : "FAIL", total_fail);
    return total_fail == 0 ? 0 : 1;
}
