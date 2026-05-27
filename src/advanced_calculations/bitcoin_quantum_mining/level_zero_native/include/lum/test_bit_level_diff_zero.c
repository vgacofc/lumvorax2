/* ============================================================================
 * test_bit_level_diff_zero.c — Cycle C133
 * ----------------------------------------------------------------------------
 * Preuve formelle (A) demandée depuis C129 : trace → reconstruct → diff=0
 *
 * Trois sous-tests indépendants sur un buffer DÉTERMINISTE :
 *   1) GRANULARITÉ PAGE  (4096 octets, 1 page)
 *   2) GRANULARITÉ BYTE  (4096 octets, 1 LUM par octet)
 *   3) GRANULARITÉ BIT-1b (4096 octets = 32 768 LUMs, 1 par bit)
 *
 * Pour chaque granularité :
 *   - Allouer un buffer
 *   - Le remplir avec un pattern non-trivial (b[i] = (i*37 + 13) & 0xFF)
 *   - lum_memory_snapshot_buffer() → fichier .lum
 *   - lum_memory_reconstruct() → buffer cible
 *   - Compare bit-à-bit le buffer source et le buffer reconstruit
 *   - Exige diff_bits == 0
 *
 * Le buffer est strictement immuable entre snapshot et reconstruction
 * (contrairement à snapshot_self qui voit des compteurs/timestamps évoluer).
 *
 * Sortie : exit 0 si TOUS les tests passent (diff=0), exit 1 sinon.
 * Affiche également un résumé CSV-style sur stdout pour traçabilité forensique.
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L
#include "lum_memory_tracer.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TEST_BUFFER_SIZE 4096u  /* 1 page = bon compromis pour les 3 modes */

static const char* granularity_name(lum_trace_granularity_t g) {
    switch (g) {
        case LUM_TRACE_GRANULARITY_PAGE:     return "PAGE-4KiB";
        case LUM_TRACE_GRANULARITY_BYTE:     return "BYTE-1B";
        case LUM_TRACE_GRANULARITY_BIT:      return "BIT-1b";
        case LUM_TRACE_GRANULARITY_HUGEPAGE: return "HUGEPAGE-2MiB";
        default:                             return "UNKNOWN";
    }
}

/* Pattern déterministe non-trivial : permutation linéaire affine modulo 256.
 * Évite les cas dégénérés (tout 0, tout 1, sequential trivial). */
static void fill_deterministic_pattern(uint8_t* buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = (uint8_t)((i * 37u + 13u) & 0xFFu);
    }
}

/* Compare bit-à-bit deux buffers et compte le nombre de bits différents. */
static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t diff_bits = 0;
    for (size_t i = 0; i < n; i++) {
        uint8_t x = (uint8_t)(a[i] ^ b[i]);
        diff_bits += (uint64_t)__builtin_popcount(x);
    }
    return diff_bits;
}

/* Exécute le test diff=0 pour une granularité donnée.
 * Retourne 0 si succès (diff=0), 1 sinon. */
static int run_diff_zero_test(lum_trace_granularity_t granularity,
                               const char* tmp_dir) {
    const char* gname = granularity_name(granularity);
    char lum_path[512];
    snprintf(lum_path, sizeof(lum_path), "%s/test_diff_zero_%s.lum",
             tmp_dir, gname);

    /* 1) Buffer source DÉTERMINISTE */
    uint8_t* src = (uint8_t*)aligned_alloc(64, TEST_BUFFER_SIZE);
    if (!src) {
        fprintf(stderr, "[C133-TEST] aligned_alloc src failed\n");
        return 1;
    }
    fill_deterministic_pattern(src, TEST_BUFFER_SIZE);

    /* 2) Snapshot du buffer */
    lum_trace_stats_t snap_stats;
    memset(&snap_stats, 0, sizeof(snap_stats));
    int rc = lum_memory_snapshot_buffer(src, TEST_BUFFER_SIZE,
                                         lum_path, granularity,
                                         &snap_stats);
    if (rc != 0) {
        fprintf(stderr,
                "[C133-TEST] snapshot_buffer FAILED rc=%d granularity=%s\n",
                rc, gname);
        free(src);
        return 1;
    }

    /* 3) Vérifier la taille fichier réelle (file_size_bytes, métrique C129) */
    struct stat st;
    long long file_size = -1;
    if (stat(lum_path, &st) == 0) {
        file_size = (long long)st.st_size;
    }

    /* 4) Buffer cible (zero-init) pour reconstruction */
    uint8_t* dst = (uint8_t*)aligned_alloc(64, TEST_BUFFER_SIZE);
    if (!dst) {
        fprintf(stderr, "[C133-TEST] aligned_alloc dst failed\n");
        free(src);
        return 1;
    }
    memset(dst, 0, TEST_BUFFER_SIZE);

    /* 5) Reconstruction */
    size_t bytes_restored = 0;
    rc = lum_memory_reconstruct(lum_path, dst, TEST_BUFFER_SIZE,
                                 &bytes_restored);
    if (rc != 0) {
        fprintf(stderr,
                "[C133-TEST] reconstruct FAILED rc=%d granularity=%s\n",
                rc, gname);
        free(src);
        free(dst);
        return 1;
    }

    /* 6) Diff bit-à-bit */
    uint64_t diff_bits  = count_diff_bits(src, dst, TEST_BUFFER_SIZE);
    uint64_t diff_bytes = 0;
    for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        if (src[i] != dst[i]) diff_bytes++;
    }

    /* 7) Émission ligne CSV traçable */
    printf("c133_diff_zero,granularity=%s,buffer_size=%u,"
           "lums_emitted=%" PRIu64 ",bytes_dumped=%" PRIu64 ","
           "file_size_bytes=%lld,bytes_restored=%zu,"
           "diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64 ","
           "snapshot_ns=%" PRIu64 "\n",
           gname,
           (unsigned)TEST_BUFFER_SIZE,
           snap_stats.total_lums_emitted,
           snap_stats.total_bytes_dumped,
           file_size,
           bytes_restored,
           diff_bytes,
           diff_bits,
           snap_stats.snapshot_ns);

    int verdict = (diff_bits == 0 && diff_bytes == 0) ? 0 : 1;
    if (verdict == 0) {
        printf("[C133-TEST] %s : PASS (diff=0)\n", gname);
    } else {
        printf("[C133-TEST] %s : FAIL (diff_bits=%" PRIu64
               " diff_bytes=%" PRIu64 ")\n",
               gname, diff_bits, diff_bytes);
        /* Dump des 16 premiers octets pour debug */
        printf("  src[0..16] :");
        for (int i = 0; i < 16; i++) printf(" %02x", src[i]);
        printf("\n  dst[0..16] :");
        for (int i = 0; i < 16; i++) printf(" %02x", dst[i]);
        printf("\n");
    }

    free(src);
    free(dst);
    return verdict;
}

int main(int argc, char* argv[]) {
    const char* tmp_dir = (argc > 1) ? argv[1] : "/tmp";

    /* Vérifier que le répertoire est utilisable */
    struct stat st;
    if (stat(tmp_dir, &st) != 0) {
        fprintf(stderr, "[C133-TEST] tmp_dir %s inaccessible\n", tmp_dir);
        return 2;
    }

    printf("# C133 — Test unitaire LUM trace→reconstruct→diff=0\n");
    printf("# tmp_dir=%s buffer_size=%u\n", tmp_dir, TEST_BUFFER_SIZE);
    printf("# CSV header : test,granularity,buffer_size,lums_emitted,"
           "bytes_dumped,file_size_bytes,bytes_restored,"
           "diff_bytes,diff_bits,snapshot_ns\n");

    int total_failures = 0;
    total_failures += run_diff_zero_test(LUM_TRACE_GRANULARITY_PAGE, tmp_dir);
    total_failures += run_diff_zero_test(LUM_TRACE_GRANULARITY_BYTE, tmp_dir);
    total_failures += run_diff_zero_test(LUM_TRACE_GRANULARITY_BIT,  tmp_dir);

    if (total_failures == 0) {
        printf("\n[C133-VERDICT] TOUS LES TESTS PASS — diff=0 prouvé "
               "sur PAGE+BYTE+BIT (preuve A formellement satisfaite).\n");
        return 0;
    }
    printf("\n[C133-VERDICT] ECHEC — %d test(s) en échec\n", total_failures);
    return 1;
}
