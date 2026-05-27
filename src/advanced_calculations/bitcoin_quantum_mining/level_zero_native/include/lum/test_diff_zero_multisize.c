/* ============================================================================
 * test_diff_zero_multisize.c — Cycle C134
 * ----------------------------------------------------------------------------
 * Étend test_bit_level_diff_zero.c en variant la taille du buffer :
 *   4 KiB, 8 KiB, 16 KiB, 64 KiB, 256 KiB, 1 MiB
 *
 * Pour chaque taille, on teste granularités PAGE / BYTE / BIT et on exige
 * diff_bits == 0 ET diff_bytes == 0.
 *
 * Sortie CSV (compatible C133) — une ligne par couple (size, granularity) :
 *   c134_multisize,buffer_size=N,granularity=G,lums=K,bytes_restored=R,
 *   file_size=F,diff_bytes=X,diff_bits=Y,snapshot_ns=T,verdict=PASS|FAIL
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

static const size_t SIZES[] = {
    4 * 1024, 8 * 1024, 16 * 1024, 64 * 1024, 256 * 1024, 1024 * 1024
};
static const size_t N_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);

static const char* gran_name(lum_trace_granularity_t g) {
    switch (g) {
        case LUM_TRACE_GRANULARITY_PAGE: return "PAGE-4K";
        case LUM_TRACE_GRANULARITY_BYTE: return "BYTE-1B";
        case LUM_TRACE_GRANULARITY_BIT:  return "BIT-1b";
        default: return "?";
    }
}

static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t bits = 0;
    for (size_t i = 0; i < n; i++) bits += (uint64_t)__builtin_popcount(a[i] ^ b[i]);
    return bits;
}

static void fill_pattern(uint8_t* buf, size_t n, uint64_t seed) {
    uint64_t s = seed ? seed : 0xC134000000000001ULL;
    for (size_t i = 0; i < n; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        buf[i] = (uint8_t)(s & 0xFF);
    }
}

static int run_one(const char* outdir, size_t buf_size,
                   lum_trace_granularity_t gran) {
    uint8_t* src = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    uint8_t* dst = (uint8_t*)lum_aligned_alloc_safe(64, buf_size);
    if (!src || !dst) {
        fprintf(stderr, "[C134-MS] alloc fail size=%zu\n", buf_size);
        free(src); free(dst); return 1;
    }
    fill_pattern(src, buf_size, (uint64_t)buf_size ^ 0xDEADBEEF);
    memset(dst, 0, buf_size);

    char path[512];
    snprintf(path, sizeof(path), "%s/ms_%zu_%s.lum",
             outdir, buf_size, gran_name(gran));

    lum_trace_stats_t st;
    memset(&st, 0, sizeof(st));
    int rc = lum_memory_snapshot_buffer(src, buf_size, path, gran, &st);
    if (rc != 0) {
        fprintf(stderr, "[C134-MS] snapshot rc=%d size=%zu gran=%s\n",
                rc, buf_size, gran_name(gran));
        free(src); free(dst); return 2;
    }

    struct stat sb;
    uint64_t file_size = 0;
    if (stat(path, &sb) == 0) file_size = (uint64_t)sb.st_size;

    size_t bytes_restored = 0;
    rc = lum_memory_reconstruct(path, dst, buf_size, &bytes_restored);
    if (rc != 0) {
        fprintf(stderr, "[C134-MS] reconstruct rc=%d\n", rc);
        free(src); free(dst); return 3;
    }

    uint64_t db = 0;
    for (size_t i = 0; i < buf_size; i++) if (src[i] != dst[i]) db++;
    uint64_t dbits = count_diff_bits(src, dst, buf_size);

    int verdict = (db == 0 && dbits == 0) ? 0 : 1;
    printf("c134_multisize,buffer_size=%zu,granularity=%s,lums=%" PRIu64
           ",bytes_restored=%zu,file_size=%" PRIu64
           ",diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64
           ",snapshot_ns=%" PRIu64 ",verdict=%s\n",
           buf_size, gran_name(gran), st.total_lums_emitted,
           bytes_restored, file_size, db, dbits, st.snapshot_ns,
           verdict == 0 ? "PASS" : "FAIL");

    free(src); free(dst);
    return verdict;
}

int main(int argc, char** argv) {
    const char* outdir = (argc > 1) ? argv[1] : "/tmp/c134_multisize";
    if (mkdir(outdir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir"); return 2;
    }

    int total_fail = 0;
    lum_trace_granularity_t grans[] = {
        LUM_TRACE_GRANULARITY_PAGE,
        LUM_TRACE_GRANULARITY_BYTE,
        LUM_TRACE_GRANULARITY_BIT
    };
    for (size_t i = 0; i < N_SIZES; i++) {
        for (size_t g = 0; g < 3; g++) {
            int rc = run_one(outdir, SIZES[i], grans[g]);
            if (rc != 0) total_fail++;
        }
    }

    printf("[C134-MS-VERDICT] %s (%d failures)\n",
           total_fail == 0 ? "PASS" : "FAIL", total_fail);
    return total_fail == 0 ? 0 : 1;
}
