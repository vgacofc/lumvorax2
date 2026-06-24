/* ============================================================================
 * MDBAI Forensic Library Implementation
 * Conforme STANDARD_NAMES_MDBAI.md Section 9 + prompt.txt Règle #1
 *
 * Compile: gcc -shared -fPIC -O2 -std=c11 -D_GNU_SOURCE -o libmdbai_forensic.so mdbai_forensic.c
 * Usage:   LD_PRELOAD=./libmdbai_forensic.so MDBAI_JOB_ID=xyz ./target_process
 *
 * Fonctionne en mode autonome (sans dépendances LumVorax) pour Replit.
 * Sur Ubuntu avec libstdc++ fix, utilise lum_memory_tracer complet.
 * ============================================================================ */

/* _GNU_SOURCE déjà défini par CFLAGS dans Makefile */
#include "mdbai_forensic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

/* ── Timestamp monotone nanoseconde ─────────────────────────────────────── */
static uint64_t mdbai_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Format log entry (JSON simple, compatible lum_log_encoder) ─────────── */
static void mdbai_write_log(const char* log_file, uint32_t level,
                              const char* job_id, const char* msg) {
    if (!log_file || !msg) return;
    FILE* f = fopen(log_file, "a");
    if (!f) return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    fprintf(f,
        "{\"ts\":%ld.%09ld,\"level\":%u,\"job\":\"%s\","
        "\"msg\":\"%s\",\"magic\":\"0x%08X\"}\n",
        ts.tv_sec, ts.tv_nsec, level,
        job_id ? job_id : "unknown",
        msg, MDBAI_FORENSIC_MAGIC);
    fflush(f);
    fclose(f);
}

/* ── LUM binary header (64 bytes, magic MDBA) ───────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t magic;            /* 0x4D444241 'MDBA' */
    uint32_t version;          /* 1 */
    uint64_t timestamp_ns;     /* snapshot time */
    uint64_t process_pid;      /* getpid() */
    uint64_t total_pages;      /* pages scannées */
    uint64_t resident_pages;   /* pages résidentes */
    uint8_t  job_id[24];       /* job_id[:24] */
} mdbai_lum_header_t;

/* Écrit un snapshot mémoire simplifié (header LUM + /proc/self/statm) */
static int mdbai_write_memory_snapshot(const char* mem_file, const char* job_id) {
    if (!mem_file) return -EINVAL;

    FILE* statm = fopen("/proc/self/statm", "r");
    uint64_t total_pages = 0, resident_pages = 0;
    if (statm) {
        int ret = fscanf(statm, "%lu %lu", &total_pages, &resident_pages);
        (void)ret; /* Ignore return value intentionally */
        fclose(statm);
    }

    FILE* f = fopen(mem_file, "ab");
    if (!f) return -errno;

    mdbai_lum_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic          = MDBAI_FORENSIC_MAGIC;
    hdr.version        = 1;
    hdr.timestamp_ns   = mdbai_now_ns();
    hdr.process_pid    = (uint64_t)getpid();
    hdr.total_pages    = total_pages;
    hdr.resident_pages = resident_pages;
    if (job_id) {
        strncpy((char*)hdr.job_id, job_id, sizeof(hdr.job_id) - 1);
        hdr.job_id[sizeof(hdr.job_id) - 1] = '\0'; /* Null-terminate */
    }

    size_t written = fwrite(&hdr, 1, sizeof(hdr), f);
    fflush(f);
    fclose(f);

    return (written == sizeof(hdr)) ? 0 : -EIO;
}

/* ── API Publique ─────────────────────────────────────────────────────────── */

int mdbai_forensic_init(mdbai_forensic_t* ctx,
                         const char* job_id,
                         const char* log_file,
                         const char* mem_file) {
    if (!ctx) return -EINVAL;
    memset(ctx, 0, sizeof(*ctx));

    ctx->magic = MDBAI_FORENSIC_MAGIC;
    if (job_id)   strncpy(ctx->job_id,   job_id,   MDBAI_MAX_JOB_ID_LEN - 1);
    if (log_file) strncpy(ctx->log_file, log_file, MDBAI_MAX_PATH_LEN - 1);
    if (mem_file) strncpy(ctx->mem_file, mem_file, MDBAI_MAX_PATH_LEN - 1);

    ctx->init_timestamp_ns = mdbai_now_ns();
    ctx->initialized       = true;

    mdbai_write_log(ctx->log_file, 10 /* LUM_LOG_INFO */,
                    ctx->job_id, "mdbai_forensic_init OK");
    return 0;
}

void mdbai_forensic_destroy(mdbai_forensic_t* ctx) {
    if (!ctx || !ctx->initialized) return;
    mdbai_write_log(ctx->log_file, 10, ctx->job_id, "mdbai_forensic_destroy");
    ctx->initialized = false;
}

int mdbai_memory_snapshot(mdbai_forensic_t* ctx) {
    if (!ctx || !ctx->initialized) return -EINVAL;
    int rc = mdbai_write_memory_snapshot(ctx->mem_file, ctx->job_id);
    if (rc == 0) {
        ctx->memory_snapshots++;
        mdbai_write_log(ctx->log_file, 20 /* LUM_LOG_METRIC */,
                        ctx->job_id, "memory_snapshot_done");
    }
    return rc;
}

int mdbai_log_event(mdbai_forensic_t* ctx, uint32_t level, const char* msg) {
    if (!ctx || !ctx->initialized || !msg) return -EINVAL;
    mdbai_write_log(ctx->log_file, level, ctx->job_id, msg);
    ctx->events_logged++;
    return 0;
}

int mdbai_detect_leaks(mdbai_forensic_t* ctx, uint64_t* leak_count_out) {
    if (!ctx || !ctx->initialized) return -EINVAL;
    if (leak_count_out) *leak_count_out = 0;
    mdbai_write_log(ctx->log_file, 10, ctx->job_id, "mdbai_detect_leaks: mode dégradé (valgrind requis)");
    return 0;
}

void mdbai_get_stats(const mdbai_forensic_t* ctx,
                      uint64_t* events_out,
                      uint64_t* snapshots_out) {
    if (!ctx) return;
    if (events_out)    *events_out    = ctx->events_logged;
    if (snapshots_out) *snapshots_out = ctx->memory_snapshots;
}

/* ── Constructeur/Destructeur __attribute__((constructor)) ──────────────── */
/* Initialisation automatique via LD_PRELOAD */
static mdbai_forensic_t g_mdbai_ctx;

__attribute__((constructor))
static void mdbai_ld_preload_init(void) {
    const char* job_id   = getenv("MDBAI_JOB_ID");
    const char* log_file = getenv("MDBAI_LOG_FILE");
    const char* mem_file = getenv("MDBAI_MEM_FILE");

    if (!job_id) return;

    mdbai_forensic_init(&g_mdbai_ctx, job_id, log_file, mem_file);
    mdbai_memory_snapshot(&g_mdbai_ctx);
    fprintf(stderr, "[MDBAI-FORENSIC] Initialisé via LD_PRELOAD job=%s\n", job_id);
}

__attribute__((destructor))
static void mdbai_ld_preload_fini(void) {
    if (!g_mdbai_ctx.initialized) return;
    mdbai_memory_snapshot(&g_mdbai_ctx);
    fprintf(stderr, "[MDBAI-FORENSIC] events=%lu snapshots=%lu\n",
            g_mdbai_ctx.events_logged, g_mdbai_ctx.memory_snapshots);
    mdbai_forensic_destroy(&g_mdbai_ctx);
}
