#ifndef QUANTUM_FORENSIC_LOGGER_H
#define QUANTUM_FORENSIC_LOGGER_H

/*
 * LumVorax Quantum Forensic Logger — V4 NEXT — v1.0 — Cycle C33
 * ================================================================
 * Logging ultra-granulaire pour le simulateur quantique V4 NEXT.
 * Calqué sur ultra_forensic_logger.h (Hubbard HTS) — adapté aux qubits.
 *
 * Niveaux couverts (SANS FILTRE, SANS MODULO) :
 *   QF_LOG_MODULE_START/END   — Cycle de vie des modules
 *   QF_LOG_MODULE_METRIC      — Métriques par module (fidélité, cohérence…)
 *   QF_LOG_NANO               — Capture nanoseconde ring buffer 4096 entrées
 *   QF_LOG_HW_SAMPLE          — Snapshot CPU delta + RAM RSS
 *   QF_LOG_ANOMALY            — Anomalies : fidélité<0.95, déc. > seuil…
 *   QF_LOG_ALGO               — Algorithmes : Hadamard, Grover, Shor, QFT…
 *   QF_LOG_GATE               — Porte quantique step-by-step
 *   QF_LOG_QUBIT_STATE        — État amplitude qubit step-by-step
 *   QF_LOG_ENTANGLE           — Événement d'intrication
 *   QF_LOG_MEASURE            — Mesure avec collapse
 *   QF_LOG_SCENARIO           — Scénario fusion_v3 step-by-step
 *   QF_LOG_THREAD_START/END   — Lifecycle threads POSIX
 *   QF_LOG_PHASE_BRIDGE       — Bridge Python → CSV LumVorax
 *
 * INSTRUCTION PERMANENTE 2 : NE JAMAIS RÉDUIRE LES LOGS NI LA FRÉQUENCE.
 * Aucun filtre modulo. Aucune désactivation de fflush. Aucune réduction.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Niveaux de logging ─────────────────────────────────────────── */
typedef enum {
    QF_TRACE     = 0,
    QF_DEBUG     = 1,
    QF_INFO      = 2,
    QF_WARNING   = 3,
    QF_ERROR     = 4,
    QF_CRITICAL  = 5,
    QF_EMERGENCY = 6
} qf_level_e;

/* ── Types d'événements CSV ──────────────────────────────────────── */
typedef enum {
    QF_EVENT_INIT         = 0,
    QF_EVENT_MODULE_START = 1,
    QF_EVENT_MODULE_END   = 2,
    QF_EVENT_METRIC       = 3,
    QF_EVENT_SUMMARY      = 4,
    QF_EVENT_HW_SAMPLE    = 5,
    QF_EVENT_NANO_SAMPLE  = 6,
    QF_EVENT_ANOMALY      = 7,
    QF_EVENT_ALGO         = 8,
    QF_EVENT_PHASE_BRIDGE = 9,
    QF_EVENT_THREAD       = 10,
    QF_EVENT_GATE         = 11,   /* Porte quantique appliquée */
    QF_EVENT_QUBIT_STATE  = 12,   /* Amplitude qubit step-by-step */
    QF_EVENT_ENTANGLE     = 13,   /* Événement d'intrication */
    QF_EVENT_MEASURE      = 14,   /* Mesure avec collapse */
    QF_EVENT_SCENARIO     = 15    /* Scénario fusion_v3 */
} qf_event_type_e;

/* ── Ring buffer nanoseconde ─────────────────────────────────────── */
#define QF_NANO_RING_SIZE 4096

typedef struct {
    uint64_t  ts_ns;
    char      module[32];
    char      key[32];
    double    value;
} qf_nano_entry_t;

/* ── Snapshot hardware ───────────────────────────────────────────── */
typedef struct {
    double   cpu_delta_pct;
    double   mem_used_pct;
    long     mem_avail_kb;
    long     mem_total_kb;
    uint64_t ts_ns;
    pid_t    pid;
    long     vm_rss_kb;
    long     vm_peak_kb;
} qf_hw_snapshot_t;

/* ── Tracker par module ──────────────────────────────────────────── */
typedef struct {
    char     module_name[64];
    char     test_name[128];
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    uint64_t operations_count;
    uint64_t metric_events_count;
    uint64_t anomaly_count;
    uint64_t gate_count;
    uint64_t measure_count;
    uint64_t entangle_count;
    size_t   memory_used;
    FILE*    module_log_file;
    pthread_t thread_id;
    pthread_mutex_t mutex;
} qf_module_tracker_t;

/* ── Seuils de détection d'anomalies quantiques ─────────────────── */
typedef struct {
    double fidelity_min_threshold;      /* Si fidélité < seuil → ANOMALY */
    double coherence_min_threshold;     /* Si cohérence_time < seuil ns → ANOMALY */
    double gate_error_spike_threshold;  /* Si erreur_porte > seuil → ANOMALY */
    double prob_norm_deviation;         /* Si |sum(P)-1| > seuil → ANOMALY */
    double amplitude_jump_threshold;    /* Si |Δ|ψ|| > seuil → ANOMALY */
    double cpu_spike_threshold;
    double mem_spike_threshold;
    uint64_t stall_timeout_ns;
} qf_anomaly_thresholds_t;

/* ── Macros de logging ultra-granulaire — SANS FILTRE ────────────── */

/* Cycle de vie module */
#define QF_LOG_MODULE_START(module, test) \
    qf_log_module_start(__FILE__, __LINE__, __func__, module, test)

#define QF_LOG_MODULE_END(module, test, result) \
    qf_log_module_end(__FILE__, __LINE__, __func__, module, test, result)

/* Métrique par module (physique, fidélité, cohérence, win_rate…) */
#define QF_LOG_MODULE_METRIC(module, metric_name, value) \
    qf_log_module_metric(__FILE__, __LINE__, __func__, module, metric_name, (double)(value))

/* Ring buffer nanoseconde — sans I/O bloquante */
#define QF_LOG_NANO(module, key, value) \
    qf_log_nano(__func__, module, key, (double)(value))

/* Snapshot hardware complet */
#define QF_LOG_HW_SAMPLE(module) \
    qf_log_hw_sample(__func__, module)

/* Anomalie quantique détectée */
#define QF_LOG_ANOMALY(module, description, value) \
    qf_log_anomaly(__FILE__, __LINE__, __func__, module, description, (double)(value))

/* Algorithme quantique (Hadamard, Grover, Shor, QFT…) */
#define QF_LOG_ALGO(algo_name, metric_name, value) \
    qf_log_algo(__func__, algo_name, metric_name, (double)(value))

/* Porte quantique appliquée step-by-step */
#define QF_LOG_GATE(module, gate_name, qubit_id, fidelity_after) \
    qf_log_gate(__func__, module, gate_name, qubit_id, (double)(fidelity_after))

/* État amplitude qubit step-by-step */
#define QF_LOG_QUBIT_STATE(module, qubit_id, state_idx, re, im, prob) \
    qf_log_qubit_state(__func__, module, qubit_id, state_idx, (double)(re), (double)(im), (double)(prob))

/* Événement d'intrication */
#define QF_LOG_ENTANGLE(module, qubit_id1, qubit_id2, entangle_count) \
    qf_log_entangle(__func__, module, qubit_id1, qubit_id2, (uint64_t)(entangle_count))

/* Mesure quantique avec collapse */
#define QF_LOG_MEASURE(module, qubit_id, measured_state, probability) \
    qf_log_measure(__func__, module, qubit_id, measured_state, (double)(probability))

/* Scénario fusion_v3 step-by-step */
#define QF_LOG_SCENARIO(module, scenario_idx, step, nx_score, q_score, nx_wins) \
    qf_log_scenario(__func__, module, scenario_idx, step, (double)(nx_score), (double)(q_score), nx_wins)

/* Thread POSIX */
#define QF_LOG_THREAD_START(module, thread_label) \
    qf_log_thread(__func__, module, thread_label, "START")
#define QF_LOG_THREAD_END(module, thread_label) \
    qf_log_thread(__func__, module, thread_label, "END")

/* Bridge Python */
#define QF_LOG_PHASE_BRIDGE(phase_num, script_name, status) \
    qf_log_phase_bridge(phase_num, script_name, status)

/* ── Déclarations des fonctions ──────────────────────────────────── */

/* Initialisation / destruction */
void qf_logger_init(const char* run_id, const char* log_dir);
bool qf_logger_is_initialized(void);
void qf_logger_destroy(void);
const char* qf_logger_get_csv_path(void);

/* Fonctions de logging */
void qf_log_module_start(const char* file, int line, const char* func,
                         const char* module, const char* test);
void qf_log_module_end(const char* file, int line, const char* func,
                       const char* module, const char* test, bool success);
void qf_log_module_metric(const char* file, int line, const char* func,
                          const char* module, const char* metric_name, double value);

void qf_log_nano(const char* func, const char* module, const char* key, double value);
void qf_log_hw_sample(const char* func, const char* module);
void qf_log_anomaly(const char* file, int line, const char* func,
                    const char* module, const char* description, double value);
void qf_log_algo(const char* func, const char* algo_name,
                 const char* metric_name, double value);
void qf_log_gate(const char* func, const char* module, const char* gate_name,
                 uint64_t qubit_id, double fidelity_after);
void qf_log_qubit_state(const char* func, const char* module, uint64_t qubit_id,
                        size_t state_idx, double re, double im, double prob);
void qf_log_entangle(const char* func, const char* module,
                     uint64_t qubit_id1, uint64_t qubit_id2, uint64_t entangle_count);
void qf_log_measure(const char* func, const char* module,
                    uint64_t qubit_id, size_t measured_state, double probability);
void qf_log_scenario(const char* func, const char* module,
                     size_t scenario_idx, size_t step,
                     double nx_score, double q_score, size_t nx_wins);
void qf_log_thread(const char* func, const char* module,
                   const char* thread_label, const char* event);
void qf_log_phase_bridge(int phase_num, const char* script_name, const char* status);

/* Utilitaires */
qf_hw_snapshot_t qf_hw_snapshot(void);
void qf_set_anomaly_thresholds(const qf_anomaly_thresholds_t* t);
bool qf_check_anomaly_fidelity(const char* module, double fidelity_prev, double fidelity_curr);
bool qf_check_anomaly_prob_norm(const char* module, double* probs, size_t n);
void qf_flush_nano_ring(void);
void qf_generate_summary_report(void);
bool qf_validate_all_logs_exist(void);

/* Rotation CSV à 95 MB */
void qf_rotate_csv_if_needed(void);

#ifdef __cplusplus
}
#endif

#endif /* QUANTUM_FORENSIC_LOGGER_H */
