/* Feature test macros for POSIX functions */
/* FIX-BC25-03: guard _GNU_SOURCE — évite warning de redéfinition si déjà
 * fourni par le compilateur via -D_GNU_SOURCE dans les CFLAGS. */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "ultra_forensic_logger.h"
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

/*
 * LumVorax Ultra-Forensic Logger — v3.0 — Cycle 17
 * Corrections appliquées :
 *   NL-03  : metric_events_count → SUMMARY rapporte valeur réelle
 *   Q2-C15 : g_csv_mutex global → CSV thread-safe pour advanced_parallel
 *   AC-01  : cpu_percent_delta() → mesure CPU réelle par diff /proc/stat
 *   NV-HW  : lv_hw_snapshot_t → RSS/VmPeak processus réels
 *   NANO   : ring buffer 4096 entrées sans I/O bloquante
 *   ANOMALY: moteur de détection comportements non programmés
 *   ALGO   : tracking ED/VMC
 *   PHASE  : bridge Python → LumVorax CSV
 *   THREAD : événements POSIX thread
 */

#define MAX_MODULES      50
#define LOG_BUFFER_SIZE  4096

/* ── État global ────────────────────────────────────────────────── */
static module_forensic_tracker_t g_module_trackers[MAX_MODULES];
static int                       g_tracker_count        = 0;
static pthread_mutex_t           g_global_mutex         = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t           g_csv_mutex            = PTHREAD_MUTEX_INITIALIZER; /* NL-03/Q2 */
static bool                      g_forensic_initialized = false;
static char                      g_run_csv_path[4096]   = {0};

/* Ring buffer nanoseconde */
static lv_nano_entry_t           g_nano_ring[LV_NANO_RING_SIZE];
static volatile int              g_nano_head = 0;
static pthread_mutex_t           g_nano_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Seuils d'anomalie par défaut */
static lv_anomaly_thresholds_t   g_thresholds = {
    .energy_jump_threshold  = 0.20,   /* 20% variation d'énergie/step */
    .pairing_jump_threshold = 0.30,   /* 30% variation de pairing */
    .sign_ratio_threshold   = -0.95,  /* sign < -0.95 → signe fermionique extrême */
    .cpu_spike_threshold    = 95.0,   /* CPU > 95% */
    .mem_spike_threshold    = 95.0,   /* RAM > 95% */
    .stall_timeout_ns       = 30000000000ULL  /* 30 secondes */
};

/* ── Timestamp monotonique nanoseconde ──────────────────────────── */
static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    return 0;
}

/* FIX-BC25-04: get_realtime_ns marquée __attribute__((unused)) — fonction
 * utilitaire réservée pour audits futurs nécessitant l'heure absolue POSIX.
 * Sans cet attribut, le compilateur émet -Wunused-function. */
static uint64_t get_realtime_ns(void) __attribute__((unused));
static uint64_t get_realtime_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    return 0;
}

/* ── ISO 8601 UTC ───────────────────────────────────────────────── */
static void fill_iso(char* buf, size_t n) {
    time_t tnow = time(NULL);
    struct tm gmt;
    gmtime_r(&tnow, &gmt);
    snprintf(buf, n, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
             gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
}

/* ── Mesure CPU par delta /proc/stat (NV-HW / AC-01) ───────────── */
static double cpu_percent_delta(void) {
    static unsigned long long prev_idle  = 0;
    static unsigned long long prev_total = 0;
    static pthread_mutex_t    mu         = PTHREAD_MUTEX_INITIALIZER;

    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) return -1.0;
    unsigned long long u, n, s, i, iw, ir, si, st;
    int ok = (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                     &u, &n, &s, &i, &iw, &ir, &si, &st) == 8);
    fclose(fp);
    if (!ok) return -1.0;

    unsigned long long idle  = i + iw;
    unsigned long long total = u + n + s + i + iw + ir + si + st;

    pthread_mutex_lock(&mu);
    double result = -1.0;
    if (prev_total > 0 && total > prev_total) {
        unsigned long long d_idle  = idle  - prev_idle;
        unsigned long long d_total = total - prev_total;
        if (d_total > 0)
            result = 100.0 * (double)(d_total - d_idle) / (double)d_total;
    }
    prev_idle  = idle;
    prev_total = total;
    pthread_mutex_unlock(&mu);
    return (result < 0.0) ? 0.0 : result;
}

/* ── Mémoire système + processus (NV-HW) ───────────────────────── */
static double mem_system_used_pct(long* total_kb_out, long* avail_kb_out) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1.0;
    char k[64], u[32];
    long v = 0, total = 0, avail = 0;
    while (fscanf(fp, "%63s %ld %31s", k, &v, u) == 3) {
        if (!strcmp(k, "MemTotal:"))     total = v;
        if (!strcmp(k, "MemAvailable:")) avail = v;
        if (total && avail) break;
    }
    fclose(fp);
    if (total_kb_out)  *total_kb_out = total;
    if (avail_kb_out)  *avail_kb_out = avail;
    if (!total) return -1.0;
    return 100.0 * (double)(total - avail) / (double)total;
}

static void proc_self_status(long* rss_kb, long* peak_kb) {
    *rss_kb = 0; *peak_kb = 0;
    FILE* fp = fopen("/proc/self/status", "r");
    if (!fp) return;
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        if (!strncmp(line, "VmRSS:", 6))  sscanf(line + 6, "%ld", rss_kb);
        if (!strncmp(line, "VmPeak:", 7)) sscanf(line + 7, "%ld", peak_kb);
    }
    fclose(fp);
}

/* ── Snapshot hardware complet ──────────────────────────────────── */
lv_hw_snapshot_t ultra_forensic_hw_snapshot(void) {
    lv_hw_snapshot_t s = {0};
    s.ts_ns        = get_precise_timestamp_ns();
    s.pid          = getpid();
    s.cpu_delta_pct = cpu_percent_delta();
    s.mem_used_pct  = mem_system_used_pct(&s.mem_total_kb, &s.mem_avail_kb);
    proc_self_status(&s.vm_rss_kb, &s.vm_peak_kb);
    return s;
}

/* ── ROTATION FICHIER CSV : 20 MB max par partie ───────────────────
 * Quand le fichier courant atteint 20 MB, on le ferme et on ouvre
 * automatiquement une nouvelle partie (_part_ab, _part_ac, …) PENDANT
 * la génération — jamais après. Aucun log n'est supprimé ni throttlé.
 * Le découpage se fait ligne par ligne, sans bufferiser. */
#define LV_MAX_CSV_BYTES  (20LL * 1024LL * 1024LL)
static int  g_csv_part_num  = 0;          /* 0 = fichier original    */
static char g_csv_base[512] = {0};        /* chemin sans .csv        */

/* Génère le chemin de la partie N :
 *   0 → g_run_csv_path inchangé (fichier original)
 *   1 → _part_ab.csv
 *   2 → _part_ac.csv  …                                              */
static void lv_build_part_path(int part_num, char* out, size_t out_sz) {
    if (part_num == 0) {
        /* Première partie : chemin original (base + .csv) */
        snprintf(out, out_sz, "%s.csv", g_csv_base);
    } else {
        /* aa=1, ab=2, ac=3 … */
        int a = 'a' + (part_num - 1) / 26;
        int b = 'a' + (part_num - 1) % 26;
        snprintf(out, out_sz, "%s_part_%c%c.csv", g_csv_base, a, b);
    }
}

/* Ouvre la prochaine partie et écrit un en-tête de continuation.
 * Appelé sous g_csv_mutex. */
static void lv_rotate_csv(void) {
    g_csv_part_num++;
    lv_build_part_path(g_csv_part_num, g_run_csv_path, sizeof(g_run_csv_path));

    FILE* nf = fopen(g_run_csv_path, "w");
    if (nf) {
        uint64_t ts = get_precise_timestamp_ns();
        char iso[64]; fill_iso(iso, sizeof(iso));
        fprintf(nf,
            "event,timestamp_utc,timestamp_ns,pid,detail,value\n"
            "ROTATION,%s,%" PRIu64 ",%d,part_num,%d\n",
            iso, ts, getpid(), g_csv_part_num);
        fclose(nf);
    }
    fprintf(stderr,
        "[LUMVORAX] Rotation CSV → %s (partie %d, cap 20 MB atteint)\n",
        g_run_csv_path, g_csv_part_num);
}

/* ── Écriture thread-safe dans le CSV global ────────────────────── */
static void csv_write_line(const char* line) {
    if (!g_run_csv_path[0]) return;
    pthread_mutex_lock(&g_csv_mutex);
    /* Rotation si le fichier courant a atteint 95 MB */
    struct stat st;
    if (stat(g_run_csv_path, &st) == 0 && st.st_size >= LV_MAX_CSV_BYTES) {
        lv_rotate_csv();
    }
    FILE* csv = fopen(g_run_csv_path, "a");
    if (csv) {
        fputs(line, csv);
        fclose(csv);
    }
    pthread_mutex_unlock(&g_csv_mutex);
}

/* ── Création des répertoires forensiques ───────────────────────── */
static bool create_forensic_directories(void) {
    const char* dirs[] = {
        "logs/forensic", "logs/forensic/modules", "logs/forensic/tests",
        "logs/forensic/metrics", "logs/forensic/sessions",
        "logs/forensic/hw_samples", "logs/forensic/anomalies",
        "logs/forensic/algo", "logs/forensic/phase_bridge",
        "logs/forensic/nano"
    };
    for (size_t i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++) {
        struct stat st = {0};
        if (stat(dirs[i], &st) == -1)
            if (mkdir(dirs[i], 0755) != 0) return false;
    }
    return true;
}

/* ── Initialisation ─────────────────────────────────────────────── */
bool ultra_forensic_logger_init(void) {
    if (g_forensic_initialized) return true;
    if (!create_forensic_directories()) {
        fprintf(stderr, "[LUMVORAX] Erreur: impossible de créer les répertoires forensiques\n");
        return false;
    }
    g_forensic_initialized = true;
    printf("[ULTRA_FORENSIC] Système de logging forensique v3.0 initialisé\n");
    return true;
}

void ultra_forensic_logger_init_lum(const char* log_file) {
    if (!g_forensic_initialized) ultra_forensic_logger_init();
    if (!log_file || !log_file[0]) return;

    strncpy(g_run_csv_path, log_file, sizeof(g_run_csv_path) - 1);
    g_run_csv_path[sizeof(g_run_csv_path) - 1] = '\0';

    /* Initialise la base du chemin (sans .csv) pour la rotation */
    strncpy(g_csv_base, log_file, sizeof(g_csv_base) - 1);
    g_csv_base[sizeof(g_csv_base) - 1] = '\0';
    size_t blen = strlen(g_csv_base);
    if (blen > 4 && strcmp(g_csv_base + blen - 4, ".csv") == 0)
        g_csv_base[blen - 4] = '\0';
    g_csv_part_num = 0;  /* Remet à zéro à chaque nouveau run */

    /* Première mesure CPU pour initialiser le delta */
    (void)cpu_percent_delta();

    pthread_mutex_lock(&g_csv_mutex);
    FILE* lf = fopen(log_file, "w");
    if (lf) {
        uint64_t ts = get_precise_timestamp_ns();
        char iso[64]; fill_iso(iso, sizeof(iso));
        fprintf(lf, "event,timestamp_utc,timestamp_ns,pid,detail,value\n");
        fprintf(lf, "INIT,%s,%" PRIu64 ",%d,activation,100PCT_INCONDITIONNELLE\n",
                iso, ts, getpid());
        fprintf(lf, "INIT,%s,%" PRIu64 ",%d,modules_reels,ultra_forensic_logger_v3+memory_tracker\n",
                iso, ts, getpid());
        fprintf(lf, "INIT,%s,%" PRIu64 ",%d,version,3.0_cycle17_NL03_NV01_NV02_AC01_NANO_ANOMALY\n",
                iso, ts, getpid());
        /* Snapshot hardware initial */
        lv_hw_snapshot_t hw = ultra_forensic_hw_snapshot();
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:cpu_delta_pct,%.4f\n",
                iso, ts, getpid(), hw.cpu_delta_pct);
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:mem_used_pct,%.4f\n",
                iso, ts, getpid(), hw.mem_used_pct);
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:mem_total_kb,%ld\n",
                iso, ts, getpid(), hw.mem_total_kb);
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:mem_avail_kb,%ld\n",
                iso, ts, getpid(), hw.mem_avail_kb);
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:vm_rss_kb,%ld\n",
                iso, ts, getpid(), hw.vm_rss_kb);
        fprintf(lf, "HW_SAMPLE,%s,%" PRIu64 ",%d,init:vm_peak_kb,%ld\n",
                iso, ts, getpid(), hw.vm_peak_kb);
        fflush(lf);
        fclose(lf);
    } else {
        g_run_csv_path[0] = '\0';
        fprintf(stderr, "[LUMVORAX] AVERTISSEMENT: impossible d'écrire dans %s\n", log_file);
    }
    pthread_mutex_unlock(&g_csv_mutex);
    fprintf(stderr, "[LUMVORAX] init_lum: log_run=%s ACTIF v3.0\n", log_file);
}

void ultra_forensic_logger_destroy(void) {
    if (!g_forensic_initialized) return;

    /* Flush ring buffer avant fermeture */
    ultra_forensic_flush_nano_ring();

    /* C56-01 : Générer le rapport SUMMARY dans le CSV Lumvorax AVANT destruction
     * des trackers (NL-03 fix : assure que les SUMMARY sont écrits dans le CSV).
     * Doit être appelé ici pour que g_module_trackers soit encore valide. */
    ultra_forensic_generate_summary_report();

    /* Snapshot hardware final */
    if (g_run_csv_path[0]) {
        lv_hw_snapshot_t hw = ultra_forensic_hw_snapshot();
        char iso[64]; fill_iso(iso, sizeof(iso));
        char line[512];
        snprintf(line, sizeof(line),
                 "HW_SAMPLE,%s,%" PRIu64 ",%d,shutdown:cpu_delta_pct,%.4f\n"
                 "HW_SAMPLE,%s,%" PRIu64 ",%d,shutdown:mem_used_pct,%.4f\n"
                 "HW_SAMPLE,%s,%" PRIu64 ",%d,shutdown:vm_rss_kb,%ld\n",
                 iso, hw.ts_ns, getpid(), hw.cpu_delta_pct,
                 iso, hw.ts_ns, getpid(), hw.mem_used_pct,
                 iso, hw.ts_ns, getpid(), hw.vm_rss_kb);
        csv_write_line(line);
    }

    pthread_mutex_lock(&g_global_mutex);
    for (int i = 0; i < g_tracker_count; i++) {
        if (g_module_trackers[i].module_log_file) {
            fclose(g_module_trackers[i].module_log_file);
            g_module_trackers[i].module_log_file = NULL;
        }
        pthread_mutex_destroy(&g_module_trackers[i].mutex);
    }
    g_tracker_count = 0;
    g_forensic_initialized = false;
    pthread_mutex_unlock(&g_global_mutex);
    pthread_mutex_destroy(&g_global_mutex);

    printf("[ULTRA_FORENSIC] Système de logging forensique arrêté proprement\n");
}

/* ── Tracker par module ─────────────────────────────────────────── */
static module_forensic_tracker_t* get_or_create_module_tracker(const char* module) {
    pthread_mutex_lock(&g_global_mutex);
    for (int i = 0; i < g_tracker_count; i++) {
        if (strcmp(g_module_trackers[i].module_name, module) == 0) {
            pthread_mutex_unlock(&g_global_mutex);
            return &g_module_trackers[i];
        }
    }
    if (g_tracker_count >= MAX_MODULES) {
        pthread_mutex_unlock(&g_global_mutex);
        return NULL;
    }
    module_forensic_tracker_t* tracker = &g_module_trackers[g_tracker_count++];
    memset(tracker, 0, sizeof(*tracker));
    strncpy(tracker->module_name, module, sizeof(tracker->module_name) - 1);
    tracker->thread_id = pthread_self();

    char log_filename[512];
    uint64_t ts = get_precise_timestamp_ns();
    snprintf(log_filename, sizeof(log_filename),
             "logs/forensic/modules/%s_forensic_%" PRIu64 ".log", module, ts);
    tracker->module_log_file = fopen(log_filename, "w");
    if (!tracker->module_log_file) {
        fprintf(stderr, "[ULTRA_FORENSIC] ERREUR: impossible de créer log pour module %s\n", module);
        g_tracker_count--;
        pthread_mutex_unlock(&g_global_mutex);
        return NULL;
    }
    pthread_mutex_init(&tracker->mutex, NULL);
    fprintf(tracker->module_log_file,
            "=== LOG FORENSIQUE ULTRA-STRICT v3.0 MODULE %s ===\n"
            "Timestamp: %" PRIu64 " ns | PID: %d | Thread: %lu\n"
            "Standards: ISO/IEC 27037, NIST SP 800-86, IEEE 1012\n"
            "=====================================================\n",
            module, ts, getpid(), (unsigned long)pthread_self());
    fflush(tracker->module_log_file);
    pthread_mutex_unlock(&g_global_mutex);
    return tracker;
}

/* ── MODULE_START ───────────────────────────────────────────────── */
void ultra_forensic_log_module_start(const char* file, int line, const char* func,
                                     const char* module, const char* test) {
    if (!g_forensic_initialized) return;
    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    tracker->start_timestamp_ns = get_precise_timestamp_ns();
    strncpy(tracker->test_name, test, sizeof(tracker->test_name) - 1);
    if (tracker->module_log_file) {
        fprintf(tracker->module_log_file,
                "[%" PRIu64 "] TEST_START: %s | Source: %s:%d in %s()\n",
                tracker->start_timestamp_ns, test, file, line, func);
        fflush(tracker->module_log_file);
    }
    if (g_run_csv_path[0]) {
        char iso[64]; fill_iso(iso, sizeof(iso));
        char line_buf[512];
        snprintf(line_buf, sizeof(line_buf),
                 "MODULE_START,%s,%" PRIu64 ",%d,%s,%s\n",
                 iso, tracker->start_timestamp_ns, getpid(), module, test);
        csv_write_line(line_buf);
    }
    pthread_mutex_unlock(&tracker->mutex);
}

/* ── MODULE_END ─────────────────────────────────────────────────── */
void ultra_forensic_log_module_end(const char* file, int line, const char* func,
                                   const char* module, const char* test, bool success) {
    if (!g_forensic_initialized) return;
    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    tracker->end_timestamp_ns = get_precise_timestamp_ns();
    uint64_t duration_ns = tracker->end_timestamp_ns - tracker->start_timestamp_ns;
    if (tracker->module_log_file) {
        fprintf(tracker->module_log_file,
                "[%" PRIu64 "] TEST_END: %s | %s | Durée: %" PRIu64 " ns (%.3f ms) | Src: %s:%d %s()\n",
                tracker->end_timestamp_ns, test,
                success ? "SUCCÈS" : "ÉCHEC",
                duration_ns, duration_ns / 1e6,
                file, line, func);
        fflush(tracker->module_log_file);
    }
    if (g_run_csv_path[0]) {
        char iso[64]; fill_iso(iso, sizeof(iso));
        char line_buf[512];
        snprintf(line_buf, sizeof(line_buf),
                 "MODULE_END,%s,%" PRIu64 ",%d,%s:%s,%" PRIu64 "\n",
                 iso, tracker->end_timestamp_ns, getpid(), module, test, duration_ns);
        csv_write_line(line_buf);
    }
    pthread_mutex_unlock(&tracker->mutex);
}

/* ── OPERATION (incrémente operations_count) ────────────────────── */
void ultra_forensic_log_module_operation(const char* file, int line, const char* func,
                                         const char* module, const char* operation,
                                         const char* data) {
    if (!g_forensic_initialized) return;
    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    uint64_t ts = get_precise_timestamp_ns();
    tracker->operations_count++;
    if (tracker->module_log_file) {
        fprintf(tracker->module_log_file,
                "[%" PRIu64 "] OP #%" PRIu64 ": %s | %s | Src: %s:%d %s()\n",
                ts, tracker->operations_count, operation, data, file, line, func);
        fflush(tracker->module_log_file);
    }
    pthread_mutex_unlock(&tracker->mutex);
}

/* ── METRIC — NL-03 : incrémente metric_events_count ───────────── */
void ultra_forensic_log_module_metric(const char* file, int line, const char* func,
                                      const char* module, const char* metric_name,
                                      double value) {
    if (!g_forensic_initialized) return;
    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    uint64_t ts = get_precise_timestamp_ns();
    tracker->metric_events_count++;  /* NL-03 fix */

    if (tracker->module_log_file) {
        fprintf(tracker->module_log_file,
                "[%" PRIu64 "] METRIC #%" PRIu64 ": %s = %.10f | Src: %s:%d %s()\n",
                ts, tracker->metric_events_count, metric_name, value, file, line, func);
        fflush(tracker->module_log_file);
    }
    /* CSV global thread-safe (Q2-C15 fix) */
    if (g_run_csv_path[0]) {
        char iso[64]; fill_iso(iso, sizeof(iso));
        char line_buf[512];
        snprintf(line_buf, sizeof(line_buf),
                 "METRIC,%s,%" PRIu64 ",%d,%s:%s,%.10f\n",
                 iso, ts, getpid(), module, metric_name, value);
        csv_write_line(line_buf);  /* utilise g_csv_mutex → thread-safe */
    }
    /* Metrics log par module */
    char mf[512];
    snprintf(mf, sizeof(mf), "logs/forensic/metrics/%s_metrics.log", module);
    FILE* mfile = fopen(mf, "a");
    if (mfile) {
        fprintf(mfile, "%" PRIu64 ",%s,%.10f\n", ts, metric_name, value);
        fclose(mfile);
    }
    pthread_mutex_unlock(&tracker->mutex);
}

/* ── NANO — ring buffer sans I/O ────────────────────────────────── */
void ultra_forensic_log_nano(const char* func, const char* module,
                             const char* key, double value) {
    (void)func;
    pthread_mutex_lock(&g_nano_mutex);
    int idx = g_nano_head % LV_NANO_RING_SIZE;
    g_nano_ring[idx].ts_ns = get_precise_timestamp_ns();
    strncpy(g_nano_ring[idx].module, module, 31);
    strncpy(g_nano_ring[idx].key,    key,    31);
    g_nano_ring[idx].value = value;
    g_nano_head++;
    pthread_mutex_unlock(&g_nano_mutex);
}

void ultra_forensic_flush_nano_ring(void) {
    pthread_mutex_lock(&g_nano_mutex);
    int count = g_nano_head;
    int start = (count > LV_NANO_RING_SIZE) ? (count - LV_NANO_RING_SIZE) : 0;
    char path[256];
    snprintf(path, sizeof(path),
             "logs/forensic/nano/nano_ring_%" PRIu64 ".csv",
             get_precise_timestamp_ns());
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "seq,ts_ns,module,key,value\n");
        for (int s = start; s < count; s++) {
            int idx = s % LV_NANO_RING_SIZE;
            fprintf(f, "%d,%" PRIu64 ",%s,%s,%.10f\n",
                    s, g_nano_ring[idx].ts_ns,
                    g_nano_ring[idx].module, g_nano_ring[idx].key,
                    g_nano_ring[idx].value);
        }
        fclose(f);
        fprintf(stderr, "[LUMVORAX] Nano ring flushed: %d entrées → %s\n",
                count - start, path);
    }
    pthread_mutex_unlock(&g_nano_mutex);
}

/* ── HW_SAMPLE — snapshot CPU+mem dans CSV ──────────────────────── */
void ultra_forensic_log_hw_sample(const char* func, const char* module) {
    (void)func;
    lv_hw_snapshot_t hw = ultra_forensic_hw_snapshot();
    if (!g_run_csv_path[0]) return;
    char iso[64]; fill_iso(iso, sizeof(iso));
    char line_buf[1024];
    snprintf(line_buf, sizeof(line_buf),
             "HW_SAMPLE,%s,%" PRIu64 ",%d,%s:cpu_delta_pct,%.4f\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,%s:mem_used_pct,%.4f\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,%s:vm_rss_kb,%ld\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,%s:vm_peak_kb,%ld\n",
             iso, hw.ts_ns, getpid(), module, hw.cpu_delta_pct,
             iso, hw.ts_ns, getpid(), module, hw.mem_used_pct,
             iso, hw.ts_ns, getpid(), module, hw.vm_rss_kb,
             iso, hw.ts_ns, getpid(), module, hw.vm_peak_kb);
    csv_write_line(line_buf);

    /* Fichier hw_samples pour archivage complet */
    char hf[256];
    snprintf(hf, sizeof(hf), "logs/forensic/hw_samples/%s_hw.log", module);
    FILE* hfile = fopen(hf, "a");
    if (hfile) {
        fprintf(hfile,
                "%" PRIu64 ",cpu=%.4f%%,mem=%.4f%%,rss=%ldKB,peak=%ldKB,avail=%ldKB\n",
                hw.ts_ns, hw.cpu_delta_pct, hw.mem_used_pct,
                hw.vm_rss_kb, hw.vm_peak_kb, hw.mem_avail_kb);
        fclose(hfile);
    }
}

/* ── ANOMALY — détection et alerte ─────────────────────────────── */
void ultra_forensic_log_anomaly(const char* file, int line, const char* func,
                                const char* module, const char* description,
                                double value) {
    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (tracker) {
        pthread_mutex_lock(&tracker->mutex);
        tracker->anomaly_count++;
        pthread_mutex_unlock(&tracker->mutex);
    }

    uint64_t ts = get_precise_timestamp_ns();
    char iso[64]; fill_iso(iso, sizeof(iso));

    /* CSV global */
    char line_buf[512];
    snprintf(line_buf, sizeof(line_buf),
             "ANOMALY,%s,%" PRIu64 ",%d,%s:%s,%.10f\n",
             iso, ts, getpid(), module, description, value);
    csv_write_line(line_buf);

    /* Fichier anomalies dédié */
    char af[256];
    snprintf(af, sizeof(af), "logs/forensic/anomalies/%s_anomalies.log", module);
    FILE* afile = fopen(af, "a");
    if (afile) {
        fprintf(afile,
                "[%" PRIu64 "] ANOMALY: %s = %.10f | Src: %s:%d %s()\n",
                ts, description, value, file, line, func);
        fclose(afile);
    }

    fprintf(stderr, "[LUMVORAX:ANOMALY] module=%s desc=%s val=%.6f src=%s:%d\n",
            module, description, value, file, line);
}

bool ultra_forensic_check_anomaly_energy(const char* module, double energy_prev,
                                          double energy_curr, double u_eV) {
    if (energy_prev == 0.0 || u_eV == 0.0) return false;
    double rel_jump = fabs(energy_curr - energy_prev) / fabs(u_eV);
    if (rel_jump > g_thresholds.energy_jump_threshold) {
        char desc[128];
        snprintf(desc, sizeof(desc), "energy_jump_rel_u=%.6f", rel_jump);
        FORENSIC_LOG_ANOMALY(module, desc, rel_jump);
        return true;
    }
    return false;
}

bool ultra_forensic_check_anomaly_sign(const char* module, double sign_ratio) {
    if (sign_ratio < g_thresholds.sign_ratio_threshold) {
        char desc[128];
        snprintf(desc, sizeof(desc), "sign_ratio_extreme=%.6f", sign_ratio);
        FORENSIC_LOG_ANOMALY(module, desc, sign_ratio);
        return true;
    }
    return false;
}

void ultra_forensic_set_anomaly_thresholds(const lv_anomaly_thresholds_t* t) {
    if (t) g_thresholds = *t;
}

/* ── ALGO — tracking ED/VMC/etc. ───────────────────────────────── */
void ultra_forensic_log_algo(const char* func, const char* algo_name,
                              const char* metric_name, double value) {
    (void)func;
    uint64_t ts = get_precise_timestamp_ns();
    char iso[64]; fill_iso(iso, sizeof(iso));
    char line_buf[512];
    snprintf(line_buf, sizeof(line_buf),
             "ALGO,%s,%" PRIu64 ",%d,%s:%s,%.10f\n",
             iso, ts, getpid(), algo_name, metric_name, value);
    csv_write_line(line_buf);

    char af[256];
    snprintf(af, sizeof(af), "logs/forensic/algo/%s.log", algo_name);
    FILE* afile = fopen(af, "a");
    if (afile) {
        fprintf(afile, "%" PRIu64 ",%s,%.10f\n", ts, metric_name, value);
        fclose(afile);
    }
}

/* ── PHASE_BRIDGE — phases Python 8-39 ─────────────────────────── */
void ultra_forensic_log_phase_bridge(int phase_num, const char* script_name,
                                     const char* status) {
    uint64_t ts = get_precise_timestamp_ns();
    char iso[64]; fill_iso(iso, sizeof(iso));

    /* Snapshot HW à chaque phase Python */
    lv_hw_snapshot_t hw = ultra_forensic_hw_snapshot();

    char line_buf[1024];
    snprintf(line_buf, sizeof(line_buf),
             "PHASE_BRIDGE,%s,%" PRIu64 ",%d,phase%02d:%s,%s\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,phase%02d:cpu_delta_pct,%.4f\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,phase%02d:mem_used_pct,%.4f\n"
             "HW_SAMPLE,%s,%" PRIu64 ",%d,phase%02d:vm_rss_kb,%ld\n",
             iso, ts, getpid(), phase_num, script_name, status,
             iso, hw.ts_ns, getpid(), phase_num, hw.cpu_delta_pct,
             iso, hw.ts_ns, getpid(), phase_num, hw.mem_used_pct,
             iso, hw.ts_ns, getpid(), phase_num, hw.vm_rss_kb);
    csv_write_line(line_buf);

    char pf[256];
    snprintf(pf, sizeof(pf), "logs/forensic/phase_bridge/phase_%02d.log", phase_num);
    FILE* pfile = fopen(pf, "a");
    if (pfile) {
        fprintf(pfile, "%" PRIu64 " | phase%02d | %s | %s | cpu=%.2f%% mem=%.2f%%\n",
                ts, phase_num, script_name, status,
                hw.cpu_delta_pct, hw.mem_used_pct);
        fclose(pfile);
    }
}

/* ── THREAD ─────────────────────────────────────────────────────── */
void ultra_forensic_log_thread(const char* func, const char* module,
                               const char* thread_label, const char* event) {
    (void)func;
    uint64_t ts = get_precise_timestamp_ns();
    char iso[64]; fill_iso(iso, sizeof(iso));
    char line_buf[512];
    snprintf(line_buf, sizeof(line_buf),
             "THREAD,%s,%" PRIu64 ",%d,%s:%s_%s,%lu\n",
             iso, ts, getpid(), module, thread_label, event,
             (unsigned long)pthread_self());
    csv_write_line(line_buf);
}

/* ── Accès chemin CSV ───────────────────────────────────────────── */
const char* ultra_forensic_get_csv_path(void) {
    return g_run_csv_path[0] ? g_run_csv_path : NULL;
}

/* ── Validation logs ─────────────────────────────────────────────── */
bool ultra_forensic_validate_all_logs_exist(void) {
    char path[512];
    snprintf(path, sizeof(path),
             "logs/forensic/sessions/validation_%" PRIu64 ".txt",
             get_precise_timestamp_ns());
    FILE* f = fopen(path, "w");
    if (!f) return false;
    bool ok = true;
    fprintf(f, "=== VALIDATION LOGS FORENSIQUES ULTRA-STRICTS v3.0 ===\n");
    fprintf(f, "Timestamp: %" PRIu64 " | Modules: %d\n",
            get_precise_timestamp_ns(), g_tracker_count);
    for (int i = 0; i < g_tracker_count; i++) {
        module_forensic_tracker_t* t = &g_module_trackers[i];
        if (t->module_log_file) {
            fseek(t->module_log_file, 0, SEEK_END);
            long sz = ftell(t->module_log_file);
            fprintf(f, "  %s: %ld bytes | metrics=%" PRIu64 " | ops=%" PRIu64
                    " | anomalies=%" PRIu64 "\n",
                    t->module_name, sz, t->metric_events_count,
                    t->operations_count, t->anomaly_count);
            if (sz == 0) ok = false;
        } else {
            fprintf(f, "  %s: MANQUANT\n", t->module_name);
            ok = false;
        }
    }
    fprintf(f, "\nRésultat: %s\n", ok ? "TOUS VALIDES" : "ERREURS");
    fclose(f);
    return ok;
}

/* ── SUMMARY — NL-03 : metric_events_count réel ─────────────────── */
void ultra_forensic_generate_summary_report(void) {
    uint64_t ts = get_precise_timestamp_ns();
    char summary_path[512];
    snprintf(summary_path, sizeof(summary_path),
             "logs/forensic/sessions/summary_%" PRIu64 ".txt", ts);

    FILE* sf = fopen(summary_path, "w");
    if (!sf) return;

    fprintf(sf,
            "=== RAPPORT RÉSUMÉ FORENSIQUE ULTRA-STRICT v3.0 ===\n"
            "Timestamp: %" PRIu64 " | Modules: %d\n\n",
            ts, g_tracker_count);

    lv_hw_snapshot_t hw = ultra_forensic_hw_snapshot();
    fprintf(sf,
            "Hardware final: cpu_delta=%.2f%% mem_used=%.2f%% rss=%ldKB peak=%ldKB\n\n",
            hw.cpu_delta_pct, hw.mem_used_pct, hw.vm_rss_kb, hw.vm_peak_kb);

    char iso[64]; fill_iso(iso, sizeof(iso));
    FILE* csv = g_run_csv_path[0] ? NULL : NULL;
    pthread_mutex_lock(&g_csv_mutex);
    if (g_run_csv_path[0]) csv = fopen(g_run_csv_path, "a");

    uint64_t total_metrics = 0, total_ops = 0, total_anomalies = 0;
    for (int i = 0; i < g_tracker_count; i++) {
        module_forensic_tracker_t* t = &g_module_trackers[i];
        total_metrics   += t->metric_events_count;
        total_ops       += t->operations_count;
        total_anomalies += t->anomaly_count;
        fprintf(sf,
                "  %s: metrics=%" PRIu64 " ops=%" PRIu64
                " anomalies=%" PRIu64 " mem=%zu\n",
                t->module_name, t->metric_events_count,
                t->operations_count, t->anomaly_count, t->memory_used);
        if (csv) {
            /* NL-03 fix : rapporter metric_events_count, pas operations_count */
            fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,%s:metric_events,%" PRIu64 "\n",
                    iso, ts, getpid(), t->module_name, t->metric_events_count);
            fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,%s:operations,%" PRIu64 "\n",
                    iso, ts, getpid(), t->module_name, t->operations_count);
            fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,%s:anomalies,%" PRIu64 "\n",
                    iso, ts, getpid(), t->module_name, t->anomaly_count);
            fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,%s:memory_bytes,%zu\n",
                    iso, ts, getpid(), t->module_name, t->memory_used);
        }
    }
    if (csv) {
        /* Totaux globaux */
        fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,GLOBAL:total_metric_events,%" PRIu64 "\n",
                iso, ts, getpid(), total_metrics);
        fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,GLOBAL:total_operations,%" PRIu64 "\n",
                iso, ts, getpid(), total_ops);
        fprintf(csv, "SUMMARY,%s,%" PRIu64 ",%d,GLOBAL:total_anomalies,%" PRIu64 "\n",
                iso, ts, getpid(), total_anomalies);
        fprintf(csv, "HW_SAMPLE,%s,%" PRIu64 ",%d,GLOBAL:final_cpu_delta_pct,%.4f\n",
                iso, hw.ts_ns, getpid(), hw.cpu_delta_pct);
        fprintf(csv, "HW_SAMPLE,%s,%" PRIu64 ",%d,GLOBAL:final_mem_used_pct,%.4f\n",
                iso, hw.ts_ns, getpid(), hw.mem_used_pct);
        fclose(csv);
    }
    pthread_mutex_unlock(&g_csv_mutex);

    fprintf(sf, "\nTotaux: metrics=%" PRIu64 " ops=%" PRIu64
            " anomalies=%" PRIu64 "\n", total_metrics, total_ops, total_anomalies);
    fprintf(sf, "\n=== FIN RAPPORT RÉSUMÉ ===\n");
    fclose(sf);
    printf("Rapport résumé généré: %s\n", summary_path);
}
