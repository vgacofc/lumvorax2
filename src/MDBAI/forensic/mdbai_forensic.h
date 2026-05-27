/* ============================================================================
 * MDBAI Forensic Library Header
 * Conforme STANDARD_NAMES_MDBAI.md Section 9
 * Conforme prompt.txt Règle #1 : traçabilité bit-level active à 100%
 *
 * MDBAI_FORENSIC_MAGIC = 0x4D444241 ('MDBA')
 * Intègre LumVorax C111: lum_memory_tracer + lum_log_encoder + memory_tracker
 * ============================================================================ */
#ifndef MDBAI_FORENSIC_H_INCLUDED
#define MDBAI_FORENSIC_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDBAI_FORENSIC_MAGIC      0x4D444241U  /* 'MDBA' */
#define MDBAI_LUM_GRANULARITY     0            /* PAGE = 0 (LUM_TRACE_GRANULARITY_PAGE) */
#define MDBAI_LOG_ROTATION_BYTES  (20 * 1024 * 1024)  /* 20 MB */
#define MDBAI_MAX_JOB_ID_LEN      128
#define MDBAI_MAX_PATH_LEN        512

typedef struct {
    uint32_t magic;
    char     job_id[MDBAI_MAX_JOB_ID_LEN];
    char     log_file[MDBAI_MAX_PATH_LEN];
    char     mem_file[MDBAI_MAX_PATH_LEN];
    uint64_t init_timestamp_ns;
    bool     initialized;
    uint64_t events_logged;
    uint64_t memory_snapshots;
} mdbai_forensic_t;

/* Initialise le contexte forensic MDBAI */
int  mdbai_forensic_init(mdbai_forensic_t* ctx,
                          const char* job_id,
                          const char* log_file,
                          const char* mem_file);

/* Libère les ressources du contexte forensic */
void mdbai_forensic_destroy(mdbai_forensic_t* ctx);

/* Snapshot mémoire process courant au format LUM */
int  mdbai_memory_snapshot(mdbai_forensic_t* ctx);

/* Log un événement au format LUM */
int  mdbai_log_event(mdbai_forensic_t* ctx, uint32_t level, const char* msg);

/* Détecte les fuites mémoire depuis /proc/self/maps */
int  mdbai_detect_leaks(mdbai_forensic_t* ctx, uint64_t* leak_count_out);

/* Retourne les statistiques forensic courantes */
void mdbai_get_stats(const mdbai_forensic_t* ctx,
                      uint64_t* events_out,
                      uint64_t* snapshots_out);

#ifdef __cplusplus
}
#endif

#endif /* MDBAI_FORENSIC_H_INCLUDED */
