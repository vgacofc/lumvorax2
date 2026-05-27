/*
 * NX-Perf — Performance Monitoring System
 * Compteurs de performance adaptés LumVorax
 * 
 * Différence clé vs Linux perf :
 *   Linux perf = compteurs hardware bruts
 *   NX-Perf = métriques sémantiques + IA adaptative
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#ifndef NX_PERF_H
#define NX_PERF_H

#include <stdint.h>
#include <stdbool.h>

/* Types de compteurs hardware */
typedef enum {
    NX_PERF_CPU_CYCLES = 0,
    NX_PERF_INSTRUCTIONS,
    NX_PERF_CACHE_REFERENCES,
    NX_PERF_CACHE_MISSES,
    NX_PERF_BRANCH_INSTRUCTIONS,
    NX_PERF_BRANCH_MISSES,
    NX_PERF_BUS_CYCLES,
    NX_PERF_STALLED_CYCLES_FRONTEND,
    NX_PERF_STALLED_CYCLES_BACKEND,
    NX_PERF_REF_CPU_CYCLES,
    NX_PERF_COUNTER_MAX
} nx_perf_counter_type_t;

/* Compteur de performance */
typedef struct {
    nx_perf_counter_type_t type;
    uint64_t value;
    uint64_t time_enabled_ns;
    uint64_t time_running_ns;
    int fd;  /* File descriptor perf_event */
} nx_perf_counter_t;

/* Métriques sémantiques dérivées */
typedef struct {
    double ipc;                    /* Instructions per cycle */
    double cache_miss_rate;        /* % cache misses */
    double branch_miss_rate;       /* % branch mispredictions */
    double frontend_stall_rate;    /* % cycles stalled frontend */
    double backend_stall_rate;     /* % cycles stalled backend */
    double cpu_utilization;        /* % CPU utilisé */
} nx_perf_semantic_metrics_t;

/* Hotspot (fonction la plus coûteuse) */
typedef struct {
    char function_name[128];
    uint64_t cpu_cycles;
    uint64_t cache_misses;
    uint64_t branch_misses;
    double percentage_total;
} nx_perf_hotspot_t;

/* Configuration NX-Perf */
typedef struct {
    bool enable_hardware_counters;
    bool enable_semantic_analysis;
    bool enable_hotspot_detection;
    uint32_t sampling_period;      /* Période échantillonnage (cycles) */
    uint32_t max_hotspots;         /* Nombre max hotspots à tracker */
    char output_path[256];
} nx_perf_config_t;

/* Contexte NX-Perf */
typedef struct {
    nx_perf_config_t config;
    nx_perf_counter_t counters[NX_PERF_COUNTER_MAX];
    nx_perf_semantic_metrics_t metrics;
    nx_perf_hotspot_t* hotspots;
    uint32_t num_hotspots;
    bool is_active;
    uint64_t start_ts_ns;
} nx_perf_context_t;

/* API publique */

/**
 * Initialise le système NX-Perf
 * @param config Configuration
 * @return Contexte alloué ou NULL si erreur
 */
nx_perf_context_t* nx_perf_init(const nx_perf_config_t* config);

/**
 * Démarre la collecte de métriques
 * @param ctx Contexte NX-Perf
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_start(nx_perf_context_t* ctx);

/**
 * Arrête la collecte de métriques
 * @param ctx Contexte NX-Perf
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_stop(nx_perf_context_t* ctx);

/**
 * Lit les compteurs hardware
 * @param ctx Contexte NX-Perf
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_read_counters(nx_perf_context_t* ctx);

/**
 * Calcule les métriques sémantiques
 * @param ctx Contexte NX-Perf
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_compute_semantic_metrics(nx_perf_context_t* ctx);

/**
 * Détecte les hotspots (fonctions les plus coûteuses)
 * @param ctx Contexte NX-Perf
 * @return Nombre de hotspots détectés, -1 si erreur
 */
int nx_perf_detect_hotspots(nx_perf_context_t* ctx);

/**
 * Génère un rapport de performance
 * @param ctx Contexte NX-Perf
 * @param output_path Chemin du fichier de sortie
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_generate_report(const nx_perf_context_t* ctx, const char* output_path);

/**
 * Obtient les métriques sémantiques actuelles
 * @param ctx Contexte NX-Perf
 * @param metrics Pointeur pour recevoir les métriques
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_get_semantic_metrics(const nx_perf_context_t* ctx, nx_perf_semantic_metrics_t* metrics);

/**
 * Obtient la valeur d'un compteur spécifique
 * @param ctx Contexte NX-Perf
 * @param type Type de compteur
 * @param value Pointeur pour recevoir la valeur
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_get_counter(const nx_perf_context_t* ctx, nx_perf_counter_type_t type, uint64_t* value);

/**
 * Libère les ressources NX-Perf
 * @param ctx Contexte NX-Perf
 */
void nx_perf_destroy(nx_perf_context_t* ctx);

/**
 * Benchmark une fonction avec NX-Perf
 * @param func Fonction à benchmarker
 * @param arg Argument de la fonction
 * @param metrics Pointeur pour recevoir les métriques
 * @return 0 si succès, -1 si erreur
 */
int nx_perf_benchmark_function(void (*func)(void*), void* arg, nx_perf_semantic_metrics_t* metrics);

/* Macros pour instrumentation facile */
#define NX_PERF_START_REGION(ctx, name) \
    do { \
        if (ctx) nx_perf_read_counters(ctx); \
    } while(0)

#define NX_PERF_END_REGION(ctx, name) \
    do { \
        if (ctx) { \
            nx_perf_read_counters(ctx); \
            nx_perf_compute_semantic_metrics(ctx); \
        } \
    } while(0)

#endif /* NX_PERF_H */

// Made with Bob
