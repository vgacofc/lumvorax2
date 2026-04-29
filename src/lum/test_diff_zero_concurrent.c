/* ============================================================================
 * test_diff_zero_concurrent.c — Cycle C135
 * ----------------------------------------------------------------------------
 * Multi-thread test : 4 threads font snapshot+reconstruct+diff sur leurs
 * propres buffers en parallèle. Vérifie l'absence de race condition dans
 * lum_memory_snapshot_buffer() / lum_memory_reconstruct() (qui sont
 * documentées thread-safe pour buffers DISJOINTS).
 *
 * Sortie CSV :
 *   c135_concurrent,thread=T,buffer_size=N,granularity=G,
 *   diff_bytes=X,diff_bits=Y,verdict=PASS|FAIL
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#include "lum_core.h"
#include "lum_memory_tracer.h"
#include "lum_aligned_alloc_safe.h"

#ifndef N_THREADS
#define N_THREADS 4
#endif
#ifndef BUF_SIZE
#define BUF_SIZE (64 * 1024)  /* 64 KiB par thread */
#endif

typedef struct {
    int tid;
    const char* outdir;
    int verdict;        /* 0=PASS, 1=FAIL */
    uint64_t diff_bytes;
    uint64_t diff_bits;
    uint64_t lums;
} worker_arg_t;

static uint64_t count_diff_bits(const uint8_t* a, const uint8_t* b, size_t n) {
    uint64_t bits = 0;
    for (size_t i = 0; i < n; i++) bits += (uint64_t)__builtin_popcount(a[i] ^ b[i]);
    return bits;
}

static void* worker(void* arg) {
    worker_arg_t* w = (worker_arg_t*)arg;
    uint8_t* src = (uint8_t*)lum_aligned_alloc_safe(64, BUF_SIZE);
    uint8_t* dst = (uint8_t*)lum_aligned_alloc_safe(64, BUF_SIZE);
    if (!src || !dst) { w->verdict = 1; free(src); free(dst); return NULL; }

    uint64_t s = 0xC135000000000001ULL ^ ((uint64_t)w->tid << 32);
    for (size_t i = 0; i < BUF_SIZE; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        src[i] = (uint8_t)(s & 0xFF);
    }
    memset(dst, 0, BUF_SIZE);

    char path[512];
    snprintf(path, sizeof(path), "%s/conc_t%d.lum", w->outdir, w->tid);

    lum_trace_stats_t st;
    memset(&st, 0, sizeof(st));
    int rc = lum_memory_snapshot_buffer(src, BUF_SIZE, path,
                                        LUM_TRACE_GRANULARITY_BIT, &st);
    if (rc != 0) { w->verdict = 1; free(src); free(dst); return NULL; }

    size_t br = 0;
    rc = lum_memory_reconstruct(path, dst, BUF_SIZE, &br);
    if (rc != 0) { w->verdict = 1; free(src); free(dst); return NULL; }

    uint64_t db = 0;
    for (size_t i = 0; i < BUF_SIZE; i++) if (src[i] != dst[i]) db++;
    w->diff_bytes = db;
    w->diff_bits  = count_diff_bits(src, dst, BUF_SIZE);
    w->lums       = st.total_lums_emitted;
    w->verdict    = (db == 0 && w->diff_bits == 0) ? 0 : 1;

    free(src); free(dst);
    return NULL;
}

int main(int argc, char** argv) {
    const char* outdir = (argc > 1) ? argv[1] : "/tmp/c135_concurrent";
    if (mkdir(outdir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir"); return 2;
    }

    pthread_t th[N_THREADS];
    worker_arg_t args[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        args[i] = (worker_arg_t){.tid = i, .outdir = outdir,
                                 .verdict = 0, .diff_bytes = 0,
                                 .diff_bits = 0, .lums = 0};
        if (pthread_create(&th[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create"); return 3;
        }
    }
    int total_fail = 0;
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(th[i], NULL);
        printf("c135_concurrent,thread=%d,buffer_size=%d,granularity=BIT-1b,"
               "lums=%" PRIu64 ",diff_bytes=%" PRIu64 ",diff_bits=%" PRIu64
               ",verdict=%s\n",
               args[i].tid, BUF_SIZE, args[i].lums,
               args[i].diff_bytes, args[i].diff_bits,
               args[i].verdict == 0 ? "PASS" : "FAIL");
        if (args[i].verdict != 0) total_fail++;
    }
    printf("[C135-CONC-VERDICT] %s (%d failures)\n",
           total_fail == 0 ? "PASS" : "FAIL", total_fail);
    return total_fail == 0 ? 0 : 1;
}
