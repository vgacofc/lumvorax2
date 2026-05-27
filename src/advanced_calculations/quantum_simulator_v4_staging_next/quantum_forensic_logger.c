/*
 * LumVorax Quantum Forensic Logger — V4 NEXT — v1.0 — Cycle C33
 * ==============================================================
 * Implémentation du logger ultra-granulaire pour quantum_simulator_v4_staging_next.
 * Calqué sur ultra_forensic_logger.c (Hubbard HTS) — adapté aux qubits.
 *
 * NOMS OFFICIELS : Voir STANDARD_NAMES.md Section I
 * INSTRUCTION PERMANENTE 2 : Aucun filtre modulo, aucune désactivation,
 *   aucune réduction de fréquence — augmenter à chaque cycle.
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "quantum_forensic_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h>

/* ── Constantes ─────────────────────────────────────────────────── */
#define QF_MAX_MODULES      64
#define QF_LOG_BUF_SIZE     4096
#define QF_CSV_ROTATE_BYTES (95ULL * 1024ULL * 1024ULL) /* 95 MB */
#define QF_CSV_HEADER \
    "event_type,timestamp_utc,timestamp_ns,pid,module,metric,value,file,line,func\n"

/* ── État global ────────────────────────────────────────────────── */
static qf_module_tracker_t g_qf_trackers[QF_MAX_MODULES];
static int                 g_qf_tracker_count = 0;
static pthread_mutex_t     g_qf_global_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t     g_qf_csv_mutex     = PTHREAD_MUTEX_INITIALIZER;
static bool                g_qf_initialized   = false;
static char                g_qf_csv_path[4096] = {0};
static char                g_qf_run_id[256]    = {0};
static FILE*               g_qf_csv_file       = NULL;
static int                 g_qf_csv_part        = 0;

/* Ring buffer nanoseconde */
static qf_nano_entry_t     g_qf_nano_ring[QF_NANO_RING_SIZE];
static volatile int        g_qf_nano_head = 0;
static pthread_mutex_t     g_qf_nano_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Seuils d'anomalie par défaut (quantiques) */
static qf_anomaly_thresholds_t g_qf_thresholds = {
    .fidelity_min_threshold      = 0.95,
    .coherence_min_threshold     = 100.0,   /* ns */
    .gate_error_spike_threshold  = 0.10,
    .prob_norm_deviation         = 1e-6,
    .amplitude_jump_threshold    = 0.30,
    .cpu_spike_threshold         = 95.0,
    .mem_spike_threshold         = 95.0,
    .stall_timeout_ns            = 30000000000ULL
};

/* ── Timestamp monotonique nanoseconde ──────────────────────────── */
static uint64_t qf_now_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    return 0;
}

/* ── Timestamp UTC ISO-8601 ─────────────────────────────────────── */
static void qf_utc_str(char* buf, size_t buflen) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_val;
    gmtime_r(&ts.tv_sec, &tm_val);
    snprintf(buf, buflen, "%04d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
             tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
             tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec,
             ts.tv_nsec / 1000L);
}

/* ── Lecture CPU delta depuis /proc/stat ────────────────────────── */
typedef struct { unsigned long long user, nice, sys, idle, iowait, irq, softirq; } cpu_stat_t;

static bool read_cpu_stat(cpu_stat_t* s) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return false;
    int r = fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &s->user, &s->nice, &s->sys, &s->idle,
                   &s->iowait, &s->irq, &s->softirq);
    fclose(f);
    return (r == 7);
}

static double cpu_percent_delta(const cpu_stat_t* prev, const cpu_stat_t* curr) {
    unsigned long long busy_diff =
        (curr->user - prev->user) + (curr->nice - prev->nice) +
        (curr->sys  - prev->sys)  + (curr->irq  - prev->irq)  +
        (curr->softirq - prev->softirq);
    unsigned long long idle_diff = (curr->idle - prev->idle) + (curr->iowait - prev->iowait);
    unsigned long long total = busy_diff + idle_diff;
    return (total > 0) ? (100.0 * (double)busy_diff / (double)total) : 0.0;
}

/* ── Lecture RAM depuis /proc/meminfo ───────────────────────────── */
static void read_mem_info(long* total_kb, long* avail_kb) {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) { *total_kb = 0; *avail_kb = 0; return; }
    char line[256];
    *total_kb = 0; *avail_kb = 0;
    while (fgets(line, sizeof(line), f)) {
        long val = 0;
        if (sscanf(line, "MemTotal: %ld kB", &val) == 1) *total_kb = val;
        else if (sscanf(line, "MemAvailable: %ld kB", &val) == 1) *avail_kb = val;
    }
    fclose(f);
}

/* ── Lecture RSS processus ──────────────────────────────────────── */
static void read_proc_rss(long* rss_kb, long* peak_kb) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", (int)getpid());
    FILE* f = fopen(path, "r");
    *rss_kb = 0; *peak_kb = 0;
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        long val = 0;
        if (sscanf(line, "VmRSS: %ld kB", &val) == 1) *rss_kb = val;
        else if (sscanf(line, "VmPeak: %ld kB", &val) == 1) *peak_kb = val;
    }
    fclose(f);
}

/* ── Rotation CSV à 95 MB ───────────────────────────────────────── */
void qf_rotate_csv_if_needed(void) {
    if (!g_qf_csv_file) return;
    long pos = ftell(g_qf_csv_file);
    if (pos < 0 || (unsigned long long)pos < QF_CSV_ROTATE_BYTES) return;

    fclose(g_qf_csv_file);
    g_qf_csv_part++;

    snprintf(g_qf_csv_path, sizeof(g_qf_csv_path),
             "logs/lumvorax_quantum_v4next_%s_part%02d.csv",
             g_qf_run_id, g_qf_csv_part);
    g_qf_csv_file = fopen(g_qf_csv_path, "w");
    if (g_qf_csv_file) {
        fprintf(g_qf_csv_file, QF_CSV_HEADER);
        fflush(g_qf_csv_file);
    }
}

/* ── Écriture ligne CSV (thread-safe) ───────────────────────────── */
static void qf_write_csv(const char* event_type, const char* utc,
                          uint64_t ts_ns, const char* module,
                          const char* metric, double value,
                          const char* file, int line, const char* func) {
    if (!g_qf_csv_file || !g_qf_initialized) return;
    pthread_mutex_lock(&g_qf_csv_mutex);
    qf_rotate_csv_if_needed();
    fprintf(g_qf_csv_file,
            "%s,%s,%" PRIu64 ",%d,%s,%s,%.15g,%s,%d,%s\n",
            event_type, utc, ts_ns, (int)getpid(),
            module ? module : "unknown",
            metric ? metric : "",
            value,
            file ? file : "", line,
            func ? func : "");
    fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

/* ══════════════════════════════════════════════════════════════════
 * INITIALISATION / DESTRUCTION
 * ══════════════════════════════════════════════════════════════════ */

void qf_logger_init(const char* run_id, const char* log_dir) {
    pthread_mutex_lock(&g_qf_global_mutex);
    if (g_qf_initialized) { pthread_mutex_unlock(&g_qf_global_mutex); return; }

    /* Créer le répertoire logs si nécessaire */
    const char* dir = log_dir ? log_dir : "logs";
    mkdir(dir, 0755);

    /* Stocker run_id */
    if (run_id && run_id[0]) {
        snprintf(g_qf_run_id, sizeof(g_qf_run_id), "%s", run_id);
    } else {
        /* run_id auto depuis timestamp si non fourni */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm_val;
        gmtime_r(&ts.tv_sec, &tm_val);
        snprintf(g_qf_run_id, sizeof(g_qf_run_id),
                 "%04d%02d%02dT%02d%02d%02dZ_%05d",
                 tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
                 tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec,
                 (int)(getpid() % 100000));
    }

    /* Chemin CSV initial */
    snprintf(g_qf_csv_path, sizeof(g_qf_csv_path),
             "%s/lumvorax_quantum_v4next_%s_part00.csv", dir, g_qf_run_id);

    g_qf_csv_file = fopen(g_qf_csv_path, "w");
    if (g_qf_csv_file) {
        fprintf(g_qf_csv_file, QF_CSV_HEADER);
        fflush(g_qf_csv_file);
    }

    /* Initialiser trackers */
    memset(g_qf_trackers, 0, sizeof(g_qf_trackers));
    g_qf_tracker_count = 0;

    /* Initialiser ring buffer */
    memset(g_qf_nano_ring, 0, sizeof(g_qf_nano_ring));
    g_qf_nano_head = 0;

    g_qf_initialized = true;

    /* Événement INIT dans le CSV */
    char utc[64];
    qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    qf_write_csv("INIT", utc, ts, "qf_logger", "run_id", 0.0, __FILE__, __LINE__, __func__);

    /* Log run_id comme string via valeur pid */
    fprintf(g_qf_csv_file,
            "INIT,%s,%" PRIu64 ",%d,qf_logger,run_id_str,0.0,%s,%d,%s\n",
            utc, ts, (int)getpid(), g_qf_run_id, __LINE__, __func__);
    fflush(g_qf_csv_file);

    pthread_mutex_unlock(&g_qf_global_mutex);
}

bool qf_logger_is_initialized(void) {
    return g_qf_initialized;
}

const char* qf_logger_get_csv_path(void) {
    return g_qf_csv_path;
}

void qf_logger_destroy(void) {
    if (!g_qf_initialized) return;
    qf_flush_nano_ring();
    qf_generate_summary_report();
    pthread_mutex_lock(&g_qf_csv_mutex);
    if (g_qf_csv_file) {
        fflush(g_qf_csv_file);
        fclose(g_qf_csv_file);
        g_qf_csv_file = NULL;
    }
    pthread_mutex_unlock(&g_qf_csv_mutex);
    g_qf_initialized = false;
}

/* ══════════════════════════════════════════════════════════════════
 * MODULE START / END
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_module_start(const char* file, int line, const char* func,
                          const char* module, const char* test) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    /* Enregistrer tracker */
    pthread_mutex_lock(&g_qf_global_mutex);
    if (g_qf_tracker_count < QF_MAX_MODULES) {
        qf_module_tracker_t* t = &g_qf_trackers[g_qf_tracker_count++];
        memset(t, 0, sizeof(*t));
        snprintf(t->module_name, sizeof(t->module_name), "%s", module ? module : "");
        snprintf(t->test_name,   sizeof(t->test_name),   "%s", test   ? test   : "");
        t->start_timestamp_ns = ts;
        t->thread_id = pthread_self();
        pthread_mutex_init(&t->mutex, NULL);
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("MODULE_START", utc, ts, module, test, 0.0, file, line, func);
}

void qf_log_module_end(const char* file, int line, const char* func,
                        const char* module, const char* test, bool success) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    /* Mettre à jour tracker */
    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].end_timestamp_ns = ts;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("MODULE_END", utc, ts, module, test,
                 success ? 1.0 : 0.0, file, line, func);
}

/* ══════════════════════════════════════════════════════════════════
 * METRIC — LOG PRINCIPAL
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_module_metric(const char* file, int line, const char* func,
                           const char* module, const char* metric_name, double value) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    /* Incrémenter compteur metric_events */
    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].metric_events_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("METRIC", utc, ts, module, metric_name, value, file, line, func);
}

/* ══════════════════════════════════════════════════════════════════
 * RING BUFFER NANOSECONDE — SANS I/O BLOQUANTE
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_nano(const char* func, const char* module, const char* key, double value) {
    if (!g_qf_initialized) return;
    pthread_mutex_lock(&g_qf_nano_mutex);
    int idx = g_qf_nano_head % QF_NANO_RING_SIZE;
    qf_nano_entry_t* e = &g_qf_nano_ring[idx];
    e->ts_ns = qf_now_ns();
    snprintf(e->module, sizeof(e->module), "%s", module ? module : "");
    snprintf(e->key,    sizeof(e->key),    "%s", key    ? key    : "");
    e->value = value;
    g_qf_nano_head++;
    pthread_mutex_unlock(&g_qf_nano_mutex);
    (void)func;
}

void qf_flush_nano_ring(void) {
    if (!g_qf_initialized || !g_qf_csv_file) return;
    pthread_mutex_lock(&g_qf_nano_mutex);
    int total = g_qf_nano_head;
    int start = (total > QF_NANO_RING_SIZE) ? (total - QF_NANO_RING_SIZE) : 0;
    pthread_mutex_unlock(&g_qf_nano_mutex);

    pthread_mutex_lock(&g_qf_csv_mutex);
    for (int i = start; i < total; i++) {
        qf_nano_entry_t* e = &g_qf_nano_ring[i % QF_NANO_RING_SIZE];
        if (e->ts_ns == 0) continue;
        fprintf(g_qf_csv_file,
                "NANO_SAMPLE,UTC_APPROX,%" PRIu64 ",%d,%s,%s,%.15g,,0,\n",
                e->ts_ns, (int)getpid(), e->module, e->key, e->value);
    }
    fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

/* ══════════════════════════════════════════════════════════════════
 * HARDWARE SNAPSHOT
 * ══════════════════════════════════════════════════════════════════ */

qf_hw_snapshot_t qf_hw_snapshot(void) {
    qf_hw_snapshot_t snap = {0};
    snap.ts_ns = qf_now_ns();
    snap.pid   = getpid();

    /* CPU delta */
    static cpu_stat_t prev_cpu = {0};
    cpu_stat_t curr_cpu = {0};
    if (read_cpu_stat(&curr_cpu)) {
        if (prev_cpu.user > 0)
            snap.cpu_delta_pct = cpu_percent_delta(&prev_cpu, &curr_cpu);
        prev_cpu = curr_cpu;
    }

    /* RAM */
    read_mem_info(&snap.mem_total_kb, &snap.mem_avail_kb);
    if (snap.mem_total_kb > 0)
        snap.mem_used_pct = 100.0 * (double)(snap.mem_total_kb - snap.mem_avail_kb)
                           / (double)snap.mem_total_kb;

    /* RSS processus */
    read_proc_rss(&snap.vm_rss_kb, &snap.vm_peak_kb);
    return snap;
}

void qf_log_hw_sample(const char* func, const char* module) {
    if (!g_qf_initialized) return;
    qf_hw_snapshot_t snap = qf_hw_snapshot();
    char utc[64]; qf_utc_str(utc, sizeof(utc));

    pthread_mutex_lock(&g_qf_csv_mutex);
    qf_rotate_csv_if_needed();
    fprintf(g_qf_csv_file,
            "HW_SAMPLE,%s,%" PRIu64 ",%d,%s,cpu_delta_pct,%.4f,,0,%s\n",
            utc, snap.ts_ns, (int)snap.pid, module ? module : "", snap.cpu_delta_pct, func);
    fprintf(g_qf_csv_file,
            "HW_SAMPLE,%s,%" PRIu64 ",%d,%s,mem_used_pct,%.4f,,0,%s\n",
            utc, snap.ts_ns, (int)snap.pid, module ? module : "", snap.mem_used_pct, func);
    fprintf(g_qf_csv_file,
            "HW_SAMPLE,%s,%" PRIu64 ",%d,%s,vm_rss_kb,%ld,,0,%s\n",
            utc, snap.ts_ns, (int)snap.pid, module ? module : "", snap.vm_rss_kb, func);
    fprintf(g_qf_csv_file,
            "HW_SAMPLE,%s,%" PRIu64 ",%d,%s,vm_peak_kb,%ld,,0,%s\n",
            utc, snap.ts_ns, (int)snap.pid, module ? module : "", snap.vm_peak_kb, func);
    fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

/* ══════════════════════════════════════════════════════════════════
 * ANOMALIES QUANTIQUES
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_anomaly(const char* file, int line, const char* func,
                     const char* module, const char* description, double value) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    /* Incrémenter compteur anomalies */
    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].anomaly_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("ANOMALY", utc, ts, module, description, value, file, line, func);

    /* Flush immédiat sur anomalie — aucun filtre */
    pthread_mutex_lock(&g_qf_csv_mutex);
    if (g_qf_csv_file) fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

void qf_set_anomaly_thresholds(const qf_anomaly_thresholds_t* t) {
    if (!t) return;
    pthread_mutex_lock(&g_qf_global_mutex);
    g_qf_thresholds = *t;
    pthread_mutex_unlock(&g_qf_global_mutex);
}

bool qf_check_anomaly_fidelity(const char* module, double fidelity_prev, double fidelity_curr) {
    if (!g_qf_initialized) return false;
    double drop = fidelity_prev - fidelity_curr;
    if (fidelity_curr < g_qf_thresholds.fidelity_min_threshold || drop > g_qf_thresholds.amplitude_jump_threshold) {
        char desc[128];
        snprintf(desc, sizeof(desc), "fidelity_drop:%.8f->%.8f", fidelity_prev, fidelity_curr);
        QF_LOG_ANOMALY(module, desc, drop);
        return true;
    }
    return false;
}

bool qf_check_anomaly_prob_norm(const char* module, double* probs, size_t n) {
    if (!g_qf_initialized || !probs || n == 0) return false;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += probs[i];
    double dev = fabs(sum - 1.0);
    if (dev > g_qf_thresholds.prob_norm_deviation) {
        char desc[128];
        snprintf(desc, sizeof(desc), "prob_norm_deviation:%.15g", dev);
        QF_LOG_ANOMALY(module, desc, dev);
        return true;
    }
    return false;
}

/* ══════════════════════════════════════════════════════════════════
 * ALGORITHME QUANTIQUE
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_algo(const char* func, const char* algo_name,
                  const char* metric_name, double value) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[256];
    snprintf(metric, sizeof(metric), "quantum_algo:%s:%s",
             algo_name ? algo_name : "", metric_name ? metric_name : "");
    qf_write_csv("ALGO", utc, ts, algo_name, metric, value, "", 0, func);
}

/* ══════════════════════════════════════════════════════════════════
 * PORTES QUANTIQUES — STEP BY STEP
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_gate(const char* func, const char* module, const char* gate_name,
                  uint64_t qubit_id, double fidelity_after) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[128];
    snprintf(metric, sizeof(metric), "quantum_gate:%s:qubit%" PRIu64,
             gate_name ? gate_name : "", qubit_id);

    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].gate_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("GATE", utc, ts, module, metric, fidelity_after, "", 0, func);

    /* Avertissement si fidélité trop basse */
    if (fidelity_after < g_qf_thresholds.fidelity_min_threshold) {
        char desc[128];
        snprintf(desc, sizeof(desc), "gate_low_fidelity:%s:q%" PRIu64, gate_name ? gate_name : "", qubit_id);
        QF_LOG_ANOMALY(module, desc, fidelity_after);
    }
}

/* ══════════════════════════════════════════════════════════════════
 * ÉTAT AMPLITUDE QUBIT — STEP BY STEP
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_qubit_state(const char* func, const char* module, uint64_t qubit_id,
                         size_t state_idx, double re, double im, double prob) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    pthread_mutex_lock(&g_qf_csv_mutex);
    qf_rotate_csv_if_needed();
    fprintf(g_qf_csv_file,
            "QUBIT_STATE,%s,%" PRIu64 ",%d,%s,quantum_lum:q%" PRIu64 ":state%zu:re,%.15g,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", qubit_id, state_idx, re, func);
    fprintf(g_qf_csv_file,
            "QUBIT_STATE,%s,%" PRIu64 ",%d,%s,quantum_lum:q%" PRIu64 ":state%zu:im,%.15g,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", qubit_id, state_idx, im, func);
    fprintf(g_qf_csv_file,
            "QUBIT_STATE,%s,%" PRIu64 ",%d,%s,quantum_lum:q%" PRIu64 ":state%zu:prob,%.15g,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", qubit_id, state_idx, prob, func);
    fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

/* ══════════════════════════════════════════════════════════════════
 * INTRICATION
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_entangle(const char* func, const char* module,
                      uint64_t qubit_id1, uint64_t qubit_id2, uint64_t entangle_count) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[128];
    snprintf(metric, sizeof(metric),
             "quantum_entangle:q%" PRIu64 "_q%" PRIu64 ":count", qubit_id1, qubit_id2);

    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].entangle_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("ENTANGLE", utc, ts, module, metric, (double)entangle_count, "", 0, func);
}

/* ══════════════════════════════════════════════════════════════════
 * MESURE AVEC COLLAPSE
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_measure(const char* func, const char* module,
                     uint64_t qubit_id, size_t measured_state, double probability) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[128];
    snprintf(metric, sizeof(metric),
             "quantum_measure:q%" PRIu64 ":state%zu", qubit_id, measured_state);

    pthread_mutex_lock(&g_qf_global_mutex);
    for (int i = 0; i < g_qf_tracker_count; i++) {
        if (strncmp(g_qf_trackers[i].module_name, module ? module : "", 63) == 0) {
            g_qf_trackers[i].measure_count++;
            break;
        }
    }
    pthread_mutex_unlock(&g_qf_global_mutex);

    qf_write_csv("MEASURE", utc, ts, module, metric, probability, "", 0, func);

    /* Anomalie si probabilité collapse anormalement basse */
    if (probability < 0.01) {
        char desc[128];
        snprintf(desc, sizeof(desc), "collapse_low_prob:q%" PRIu64 ":state%zu", qubit_id, measured_state);
        QF_LOG_ANOMALY(module, desc, probability);
    }
}

/* ══════════════════════════════════════════════════════════════════
 * SCÉNARIO FUSION_V3 — STEP BY STEP
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_scenario(const char* func, const char* module,
                      size_t scenario_idx, size_t step,
                      double nx_score, double q_score, size_t nx_wins) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    pthread_mutex_lock(&g_qf_csv_mutex);
    qf_rotate_csv_if_needed();
    fprintf(g_qf_csv_file,
            "SCENARIO,%s,%" PRIu64 ",%d,%s,fusion_v3_scenario:s%zu:step%zu:nx_score,%.15g,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", scenario_idx, step, nx_score, func);
    fprintf(g_qf_csv_file,
            "SCENARIO,%s,%" PRIu64 ",%d,%s,fusion_v3_scenario:s%zu:step%zu:q_score,%.15g,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", scenario_idx, step, q_score, func);
    fprintf(g_qf_csv_file,
            "SCENARIO,%s,%" PRIu64 ",%d,%s,fusion_v3_scenario:s%zu:step%zu:nx_wins,%.0f,,0,%s\n",
            utc, ts, (int)getpid(), module ? module : "", scenario_idx, step, (double)nx_wins, func);
    fflush(g_qf_csv_file);
    pthread_mutex_unlock(&g_qf_csv_mutex);
}

/* ══════════════════════════════════════════════════════════════════
 * THREADS POSIX
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_thread(const char* func, const char* module,
                    const char* thread_label, const char* event) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[128];
    snprintf(metric, sizeof(metric), "thread:%s:%s",
             thread_label ? thread_label : "", event ? event : "");
    qf_write_csv("THREAD", utc, ts, module, metric,
                 (double)pthread_self(), "", 0, func);
}

/* ══════════════════════════════════════════════════════════════════
 * BRIDGE PYTHON
 * ══════════════════════════════════════════════════════════════════ */

void qf_log_phase_bridge(int phase_num, const char* script_name, const char* status) {
    if (!g_qf_initialized) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();
    char metric[128];
    snprintf(metric, sizeof(metric), "phase_bridge:phase%02d:%s",
             phase_num, script_name ? script_name : "");
    double status_val = (status && strcmp(status, "OK") == 0) ? 1.0 : 0.0;
    qf_write_csv("PHASE_BRIDGE", utc, ts, script_name, metric,
                 status_val, "", phase_num, "python_bridge");
}

/* ══════════════════════════════════════════════════════════════════
 * RAPPORT SUMMARY FINAL
 * ══════════════════════════════════════════════════════════════════ */

void qf_generate_summary_report(void) {
    if (!g_qf_csv_file) return;
    char utc[64]; qf_utc_str(utc, sizeof(utc));
    uint64_t ts = qf_now_ns();

    pthread_mutex_lock(&g_qf_global_mutex);
    pthread_mutex_lock(&g_qf_csv_mutex);

    uint64_t total_metrics   = 0;
    uint64_t total_anomalies = 0;
    uint64_t total_gates     = 0;
    uint64_t total_measures  = 0;
    uint64_t total_entangle  = 0;

    for (int i = 0; i < g_qf_tracker_count; i++) {
        qf_module_tracker_t* t = &g_qf_trackers[i];
        total_metrics   += t->metric_events_count;
        total_anomalies += t->anomaly_count;
        total_gates     += t->gate_count;
        total_measures  += t->measure_count;
        total_entangle  += t->entangle_count;

        uint64_t elapsed = (t->end_timestamp_ns > t->start_timestamp_ns)
                         ? (t->end_timestamp_ns - t->start_timestamp_ns) : 0;
        fprintf(g_qf_csv_file,
                "SUMMARY,%s,%" PRIu64 ",%d,%s,module_elapsed_ns,%" PRIu64 ",,0,qf_summary\n",
                utc, ts, (int)getpid(), t->module_name, elapsed);
        fprintf(g_qf_csv_file,
                "SUMMARY,%s,%" PRIu64 ",%d,%s,metric_events_count,%" PRIu64 ",,0,qf_summary\n",
                utc, ts, (int)getpid(), t->module_name, t->metric_events_count);
        fprintf(g_qf_csv_file,
                "SUMMARY,%s,%" PRIu64 ",%d,%s,anomaly_count,%" PRIu64 ",,0,qf_summary\n",
                utc, ts, (int)getpid(), t->module_name, t->anomaly_count);
        fprintf(g_qf_csv_file,
                "SUMMARY,%s,%" PRIu64 ",%d,%s,gate_count,%" PRIu64 ",,0,qf_summary\n",
                utc, ts, (int)getpid(), t->module_name, t->gate_count);
    }

    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,total_metrics,%" PRIu64 ",,0,qf_summary\n",
            utc, ts, (int)getpid(), total_metrics);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,total_anomalies,%" PRIu64 ",,0,qf_summary\n",
            utc, ts, (int)getpid(), total_anomalies);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,total_gates,%" PRIu64 ",,0,qf_summary\n",
            utc, ts, (int)getpid(), total_gates);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,total_measures,%" PRIu64 ",,0,qf_summary\n",
            utc, ts, (int)getpid(), total_measures);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,total_entangle_events,%" PRIu64 ",,0,qf_summary\n",
            utc, ts, (int)getpid(), total_entangle);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,modules_tracked,%d,,0,qf_summary\n",
            utc, ts, (int)getpid(), g_qf_tracker_count);
    fprintf(g_qf_csv_file,
            "SUMMARY,%s,%" PRIu64 ",%d,qf_global,run_id_str,0.0,%s,0,qf_summary\n",
            utc, ts, (int)getpid(), g_qf_run_id);
    fflush(g_qf_csv_file);

    pthread_mutex_unlock(&g_qf_csv_mutex);
    pthread_mutex_unlock(&g_qf_global_mutex);
}

bool qf_validate_all_logs_exist(void) {
    if (!g_qf_initialized) return false;
    return (access(g_qf_csv_path, F_OK) == 0);
}
