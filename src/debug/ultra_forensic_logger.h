#ifndef ULTRA_FORENSIC_LOGGER_H
#define ULTRA_FORENSIC_LOGGER_H

/*
 * LumVorax Ultra-Forensic Logger — v3.0 — Cycle 17
 * Upgrades cycle 17:
 *  - NL-03  : metric_events_count dans tracker (SUMMARY ne rapporte plus 0)
 *  - Q2-C15 : mutex CSV global → thread-safety pour advanced_parallel
 *  - NV-HW  : mesure CPU delta-based réelle (non cumulative)
 *  - NANO   : capture ring-buffer nanoseconde par nanoseconde
 *  - ANOMALY: détection comportements non programmés
 *  - ALGO   : tracking nouvel algorithme (ED, VMC, etc.)
 *  - PHASE  : bridge Python phases 8-39 → CSV LumVorax
 *  - THREAD : tracking threads POSIX
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

/* ── Niveaux de logging ─────────────────────────────────────────── */
typedef enum {
    FORENSIC_TRACE     = 0,
    FORENSIC_DEBUG     = 1,
    FORENSIC_INFO      = 2,
    FORENSIC_WARNING   = 3,
    FORENSIC_ERROR     = 4,
    FORENSIC_CRITICAL  = 5,
    FORENSIC_EMERGENCY = 6
} ultra_forensic_level_e;

/* ── Types d'événements CSV étendu ──────────────────────────────── */
typedef enum {
    LV_EVENT_INIT         = 0,
    LV_EVENT_MODULE_START = 1,
    LV_EVENT_MODULE_END   = 2,
    LV_EVENT_METRIC       = 3,
    LV_EVENT_SUMMARY      = 4,
    LV_EVENT_HW_SAMPLE    = 5,   /* Mesure CPU/mem/io réelle */
    LV_EVENT_NANO_SAMPLE  = 6,   /* Capture ns-level ring buffer */
    LV_EVENT_ANOMALY      = 7,   /* Comportement non programmé détecté */
    LV_EVENT_ALGO         = 8,   /* Tracking nouvel algorithme (ED/VMC) */
    LV_EVENT_PHASE_BRIDGE = 9,   /* Bridge Python phases 8-39 */
    LV_EVENT_THREAD       = 10   /* Thread POSIX start/end */
} lv_event_type_e;

/* ── Ring buffer nanoseconde ────────────────────────────────────── */
#define LV_NANO_RING_SIZE 4096

typedef struct {
    uint64_t  ts_ns;
    char      module[32];
    char      key[32];
    double    value;
} lv_nano_entry_t;

/* ── Snapshot hardware ──────────────────────────────────────────── */
typedef struct {
    double   cpu_delta_pct;   /* CPU % mesuré par delta /proc/stat */
    double   mem_used_pct;    /* RAM utilisée % depuis /proc/meminfo */
    long     mem_avail_kb;    /* KB disponibles */
    long     mem_total_kb;    /* KB totaux */
    uint64_t ts_ns;           /* Timestamp de la mesure */
    pid_t    pid;
    long     vm_rss_kb;       /* RSS du processus courant */
    long     vm_peak_kb;      /* VmPeak du processus courant */
} lv_hw_snapshot_t;

/* ── Tracker par module ─────────────────────────────────────────── */
typedef struct {
    char     module_name[64];
    char     test_name[128];
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    uint64_t operations_count;   /* Incrémenté par log_module_operation */
    uint64_t metric_events_count;/* NOUVEAU NL-03 : incrémenté par log_module_metric */
    uint64_t anomaly_count;      /* Nombre d'anomalies détectées */
    size_t   memory_used;
    FILE*    module_log_file;
    pthread_t thread_id;         /* Thread qui a créé ce tracker */
    pthread_mutex_t mutex;
} module_forensic_tracker_t;

/* ── Seuils de détection d'anomalies ───────────────────────────── */
typedef struct {
    double energy_jump_threshold;    /* Si |ΔE/E| > seuil → ANOMALY */
    double pairing_jump_threshold;   /* Si |Δpairing| > seuil → ANOMALY */
    double sign_ratio_threshold;     /* Si |sign| < seuil → ANOMALY */
    double cpu_spike_threshold;      /* Si CPU > seuil % → ANOMALY */
    double mem_spike_threshold;      /* Si mem > seuil % → ANOMALY */
    uint64_t stall_timeout_ns;       /* Si pas de METRIC depuis N ns → ANOMALY */
} lv_anomaly_thresholds_t;

/* ── Macros de logging ──────────────────────────────────────────── */
#define FORENSIC_LOG_MODULE_START(module, test) \
    ultra_forensic_log_module_start(__FILE__, __LINE__, __func__, module, test)

#define FORENSIC_LOG_MODULE_END(module, test, result) \
    ultra_forensic_log_module_end(__FILE__, __LINE__, __func__, module, test, result)

#define FORENSIC_LOG_MODULE_OPERATION(module, operation, data) \
    ultra_forensic_log_module_operation(__FILE__, __LINE__, __func__, module, operation, data)

#define FORENSIC_LOG_MODULE_METRIC(module, metric_name, value) \
    ultra_forensic_log_module_metric(__FILE__, __LINE__, __func__, module, metric_name, value)

/* NOUVEAU — Capture nanoseconde dans ring buffer (sans I/O) */
#define FORENSIC_LOG_NANO(module, key, value) \
    ultra_forensic_log_nano(__func__, module, key, (double)(value))

/* NOUVEAU — Snapshot hardware complet (CPU delta + mem RSS) */
#define FORENSIC_LOG_HW_SAMPLE(module) \
    ultra_forensic_log_hw_sample(__func__, module)

/* NOUVEAU — Anomalie comportementale détectée */
#define FORENSIC_LOG_ANOMALY(module, description, value) \
    ultra_forensic_log_anomaly(__FILE__, __LINE__, __func__, module, description, (double)(value))

/* NOUVEAU — Événement algorithme (ED, VMC, etc.) */
#define FORENSIC_LOG_ALGO(algo_name, metric_name, value) \
    ultra_forensic_log_algo(__func__, algo_name, metric_name, (double)(value))

/* NOUVEAU — Bridge Python phase (écrit depuis C ou Python via fichier) */
#define FORENSIC_LOG_PHASE_BRIDGE(phase_num, script_name, status) \
    ultra_forensic_log_phase_bridge(phase_num, script_name, status)

/* NOUVEAU — Thread lifecycle */
#define FORENSIC_LOG_THREAD_START(module, thread_label) \
    ultra_forensic_log_thread(__func__, module, thread_label, "START")
#define FORENSIC_LOG_THREAD_END(module, thread_label) \
    ultra_forensic_log_thread(__func__, module, thread_label, "END")

/* ── Déclarations de fonctions ──────────────────────────────────── */
void ultra_forensic_logger_init_lum(const char* log_file);
bool ultra_forensic_logger_init(void);
void ultra_forensic_logger_destroy(void);

void ultra_forensic_log_module_start(const char* file, int line, const char* func,
                                     const char* module, const char* test);
void ultra_forensic_log_module_end(const char* file, int line, const char* func,
                                   const char* module, const char* test, bool success);
void ultra_forensic_log_module_operation(const char* file, int line, const char* func,
                                         const char* module, const char* operation, const char* data);
void ultra_forensic_log_module_metric(const char* file, int line, const char* func,
                                      const char* module, const char* metric_name, double value);

/* Nouvelles fonctions cycle 17 */
void ultra_forensic_log_nano(const char* func, const char* module,
                             const char* key, double value);
void ultra_forensic_log_hw_sample(const char* func, const char* module);
void ultra_forensic_log_anomaly(const char* file, int line, const char* func,
                                const char* module, const char* description, double value);
void ultra_forensic_log_algo(const char* func, const char* algo_name,
                              const char* metric_name, double value);
void ultra_forensic_log_phase_bridge(int phase_num, const char* script_name, const char* status);
void ultra_forensic_log_thread(const char* func, const char* module,
                               const char* thread_label, const char* event);

/* Utilitaires hardware */
lv_hw_snapshot_t ultra_forensic_hw_snapshot(void);
void ultra_forensic_set_anomaly_thresholds(const lv_anomaly_thresholds_t* t);

/* Check anomalie sur valeur physique (retourne true si anomalie) */
bool ultra_forensic_check_anomaly_energy(const char* module, double energy_prev,
                                          double energy_curr, double u_eV);
bool ultra_forensic_check_anomaly_sign(const char* module, double sign_ratio);

/* Flush ring buffer vers CSV */
void ultra_forensic_flush_nano_ring(void);

bool ultra_forensic_validate_all_logs_exist(void);
void ultra_forensic_generate_summary_report(void);

/* Accès au chemin CSV courant (pour le bridge Python) */
const char* ultra_forensic_get_csv_path(void);

#endif /* ULTRA_FORENSIC_LOGGER_H */
