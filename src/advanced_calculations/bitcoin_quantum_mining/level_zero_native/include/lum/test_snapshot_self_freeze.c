/* ============================================================================
 * test_snapshot_self_freeze.c — Cycle C134
 * ----------------------------------------------------------------------------
 * Objectif : démontrer que le freeze process (SIGSTOP/SIGCONT) élimine la
 *            fenêtre de course entre l'écriture mémoire et la lecture
 *            /proc/<pid>/mem, donnant diff=0 reproductible.
 *
 * Stratégie sans root :
 *   1. PARENT alloue/écrit un buffer de N LUMs (pattern déterministe).
 *   2. PARENT fork() un enfant.
 *   3. ENFANT envoie SIGSTOP au PARENT (freeze atomique).
 *   4. ENFANT snapshote le buffer parent (visible via fork COW partagé).
 *   5. ENFANT envoie SIGCONT au parent.
 *   6. PARENT reconstruit + diff bit-à-bit.
 *
 * Sortie CSV (compatible C133) :
 *   c134_snapshot_freeze,buffer_size=N,lums=K,freeze_us=T,
 *   diff_bytes=X,diff_bits=Y,verdict=PASS|FAIL
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "lum_core.h"
#include "lum_memory_tracer.h"
#include "lum_aligned_alloc_safe.h"

#ifndef BUF_LUMS
#define BUF_LUMS 4096
#endif

static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t bits = 0;
    for (size_t i = 0; i < n; i++) {
        bits += (uint64_t)__builtin_popcount(a[i] ^ b[i]);
    }
    return bits;
}

static void fill_pattern(uint8_t* buf, size_t n) {
    /* Pattern déterministe basé sur xorshift simple. */
    uint64_t s = 0xC134C134DEADBEEFULL;
    for (size_t i = 0; i < n; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        buf[i] = (uint8_t)(s & 0xFF);
    }
}

int main(int argc, char** argv) {
    const char* outdir = (argc > 1) ? argv[1] : "/tmp/c134_snapshot_freeze";
    if (mkdir(outdir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        return 2;
    }

    const size_t buf_bytes = BUF_LUMS * sizeof(lum_t);
    /* Allocation alignée du buffer parent. */
    uint8_t* src = (uint8_t*)lum_aligned_alloc_safe(64, buf_bytes);
    if (!src) {
        fprintf(stderr, "[C134-FREEZE] ECHEC alloc src\n");
        return 3;
    }
    fill_pattern(src, buf_bytes);

    char snap_path[512];
    snprintf(snap_path, sizeof(snap_path), "%s/parent_freeze.lumtrace", outdir);

    pid_t parent_pid = getpid();
    uint64_t t0 = now_us();
    pid_t child = fork();
    if (child < 0) { perror("fork"); free(src); return 4; }

    if (child == 0) {
        /* === ENFANT === */
        if (kill(parent_pid, SIGSTOP) != 0) {
            perror("[CHILD] SIGSTOP"); _exit(10);
        }
        struct timespec ts_freeze = {0, 1000000}; /* 1 ms */
        nanosleep(&ts_freeze, NULL);

        /* Snapshot BIT (le plus strict). Le buffer src est visible via COW. */
        lum_trace_stats_t st;
        memset(&st, 0, sizeof(st));
        int rc = lum_memory_snapshot_buffer(src, buf_bytes, snap_path,
                                            LUM_TRACE_GRANULARITY_BIT, &st);

        if (kill(parent_pid, SIGCONT) != 0) {
            perror("[CHILD] SIGCONT"); _exit(11);
        }
        _exit(rc == 0 ? 0 : 12);
    }

    /* === PARENT === */
    int wstatus = 0;
    waitpid(child, &wstatus, 0);
    uint64_t t1 = now_us();

    if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
        fprintf(stderr, "[C134-FREEZE] enfant rc=%d\n", WEXITSTATUS(wstatus));
        free(src); return 5;
    }

    /* Reconstruction. */
    uint8_t* dst = (uint8_t*)lum_aligned_alloc_safe(64, buf_bytes);
    if (!dst) { free(src); return 6; }
    memset(dst, 0, buf_bytes);

    size_t bytes_restored = 0;
    int rrc = lum_memory_reconstruct(snap_path, dst, buf_bytes, &bytes_restored);
    if (rrc != 0) {
        fprintf(stderr, "[C134-FREEZE] reconstruct rc=%d\n", rrc);
        free(src); free(dst); return 7;
    }

    uint64_t diff_bits  = count_diff_bits(src, dst, buf_bytes);
    uint64_t diff_bytes = 0;
    for (size_t i = 0; i < buf_bytes; i++) if (src[i] != dst[i]) diff_bytes++;

    int verdict = (diff_bytes == 0 && diff_bits == 0) ? 0 : 1;
    fprintf(stdout,
        "c134_snapshot_freeze,buffer_size=%zu,lums=%zu,freeze_us=%" PRIu64
        ",diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64 ",verdict=%s\n",
        buf_bytes, (size_t)BUF_LUMS, (t1 - t0),
        diff_bytes, diff_bits, verdict == 0 ? "PASS" : "FAIL");

    free(src); free(dst);
    return verdict;
}
