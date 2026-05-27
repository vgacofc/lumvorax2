/*
 * LumVorax — Module LUM GPU Context Manager
 * lum_gpu_context.h — Gestion Contextes GPU i915 DRM Natif
 *
 * OBJECTIF: Encapsulation COMPLÈTE gestion GPU pour parallélisme
 * - Pool contextes GPU (9 contextes)
 * - Pool batch buffers (90 buffers)
 * - Pool I/O buffers (3×2 buffers pour parallélisme)
 * - Intégration traçage mémoire LUM
 * - Intégration logging LUM
 *
 * ARCHITECTURE:
 * Application → lum_gpu_context → i915 DRM → GPU Gen9
 */

#ifndef LUM_GPU_CONTEXT_H
#define LUM_GPU_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

/* Intégration modules LUM existants */
#include "lum_memory_tracer.h"
#include "lum_log_encoder.h"
#include "lum_aligned_alloc_safe.h"

/* Forward declarations pour types LUM */
typedef struct lum_memory_tracer lum_memory_tracer_t;
typedef struct lum_log_encoder lum_log_encoder_t;

/* ══════════════════════════════════════════════════════════════════════
 * CONSTANTES
 * ══════════════════════════════════════════════════════════════════════ */

#define LUM_GPU_CTX_POOL_SIZE       9    /* 9 contextes GPU */
#define LUM_GPU_BATCH_POOL_SIZE     90   /* 90 batch buffers */
#define LUM_GPU_PARALLEL_MAX        3    /* 3 dispatches parallèles max */
#define LUM_GPU_BUFFER_SIZE_1MB     (1024*1024)
#define LUM_GPU_BUFFER_SIZE_2MB     (2*1024*1024)

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Buffer GPU avec métadonnées LUM
 */
typedef struct {
    uint32_t handle;              /* Handle GEM i915 */
    void* map;                    /* Mapping CPU */
    size_t size;                  /* Taille buffer */
    uint64_t alloc_timestamp;     /* Timestamp allocation */
    bool in_use;                  /* Flag utilisation */
    int dispatch_id;              /* ID dispatch utilisant ce buffer */
} lum_gpu_buffer_t;

/**
 * Contexte GPU avec métadonnées LUM
 */
typedef struct {
    uint32_t ctx_id;              /* Context ID i915 */
    int usage_count;              /* Compteur utilisation */
    uint64_t last_used_ns;        /* Dernier usage (timestamp) */
    bool in_use;                  /* Flag utilisation */
    int dispatch_id;              /* ID dispatch utilisant ce contexte */
} lum_gpu_context_info_t;

/**
 * Dispatch parallèle
 */
typedef struct {
    int id;                       /* ID dispatch (0-2) */
    uint32_t ctx_id;              /* Contexte GPU utilisé */
    uint32_t batch_bo;            /* Batch buffer utilisé */
    uint32_t input_bo;            /* Input buffer utilisé */
    uint32_t output_bo;           /* Output buffer utilisé */
    uint64_t start_nonce;         /* Nonce de départ */
    uint32_t nonce_count;         /* Nombre de nonces */
    uint64_t start_time_ns;       /* Timestamp début */
    uint64_t end_time_ns;         /* Timestamp fin */
    bool completed;               /* Flag complétion */
    int error_code;               /* Code erreur (0 = succès) */
} lum_gpu_dispatch_t;

/**
 * Contexte GPU LUM complet
 */
typedef struct {
    /* Connexion DRM */
    int drm_fd;                   /* File descriptor /dev/dri/renderD128 */
    
    /* Pool contextes GPU */
    lum_gpu_context_info_t ctx_pool[LUM_GPU_CTX_POOL_SIZE];
    int ctx_pool_index;           /* Index rotation round-robin */
    pthread_mutex_t ctx_mutex;    /* Mutex protection pool */
    
    /* Pool batch buffers */
    lum_gpu_buffer_t batch_pool[LUM_GPU_BATCH_POOL_SIZE];
    int batch_pool_index;         /* Index rotation round-robin */
    pthread_mutex_t batch_mutex;  /* Mutex protection pool */
    
    /* Pool I/O buffers (pour parallélisme) */
    lum_gpu_buffer_t input_pool[LUM_GPU_PARALLEL_MAX];
    lum_gpu_buffer_t output_pool[LUM_GPU_PARALLEL_MAX];
    pthread_mutex_t io_mutex;     /* Mutex protection I/O */
    
    /* Kernel GPU */
    uint32_t kernel_bo;           /* Handle GEM kernel */
    void* kernel_map;             /* Mapping CPU kernel */
    size_t kernel_size;           /* Taille kernel (44248 bytes) */
    void* kernel_cache;           /* Cache mémoire kernel */
    
    /* Dispatches parallèles actifs */
    lum_gpu_dispatch_t active_dispatches[LUM_GPU_PARALLEL_MAX];
    int num_active_dispatches;    /* Nombre dispatches actifs */
    pthread_mutex_t dispatch_mutex; /* Mutex protection dispatches */
    
    /* Intégration LUM */
    lum_memory_tracer_t* mem_tracer;  /* Traçage mémoire */
    lum_log_encoder_t* logger;        /* Logging unifié */
    
    /* Métriques globales */
    uint64_t total_dispatches;    /* Total dispatches lancés */
    uint64_t total_hashes;        /* Total hashes calculés */
    double total_time_sec;        /* Temps total exécution */
    int thermal_throttle_count;   /* Compteur throttling */
    
    /* Configuration */
    uint32_t batch_size;          /* Nonces par batch (262144) */
    uint32_t work_group_size;     /* Threads par work-group (256) */
    bool enable_thp;              /* THP activé pour buffers >2MB */
    bool enable_profiling;        /* Profiling activé */
    
    /* Forensique */
    FILE* log_file;               /* Fichier log forensique */
    uint64_t event_counter;       /* Compteur événements */
} lum_gpu_context_t;

/**
 * Configuration initialisation
 */
typedef struct {
    const char* kernel_path;      /* Chemin kernel Gen9 ISA */
    const char* log_path;         /* Chemin log forensique */
    uint32_t batch_size;          /* Nonces par batch */
    uint32_t work_group_size;     /* Threads par work-group */
    bool enable_thp;              /* Activer THP */
    bool enable_profiling;        /* Activer profiling */
} lum_gpu_config_t;

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser contexte GPU LUM
 * 
 * @param config Configuration initialisation
 * @return Contexte GPU alloué, NULL si erreur
 */
lum_gpu_context_t* lum_gpu_init(const lum_gpu_config_t* config);

/**
 * Lancer N dispatches en parallèle
 * 
 * @param ctx Contexte GPU
 * @param header Block header Bitcoin (80 bytes)
 * @param start_nonce Nonce de départ
 * @param nonce_count Nombre de nonces par dispatch
 * @param n_parallel Nombre de dispatches parallèles (1-3)
 * @return 0 si succès, -1 si erreur
 */
int lum_gpu_dispatch_parallel(
    lum_gpu_context_t* ctx,
    const uint8_t* header,
    uint64_t start_nonce,
    uint32_t nonce_count,
    int n_parallel
);

/**
 * Attendre complétion de tous les dispatches actifs
 * 
 * @param ctx Contexte GPU
 * @param timeout_ms Timeout en millisecondes (0 = infini)
 * @return Nombre de dispatches complétés, -1 si erreur
 */
int lum_gpu_wait_all(lum_gpu_context_t* ctx, uint32_t timeout_ms);

/**
 * Récupérer résultats d'un dispatch
 * 
 * @param ctx Contexte GPU
 * @param dispatch_id ID dispatch (0-2)
 * @param out_nonce Nonce trouvé (output)
 * @param out_hash Hash résultant (output, 32 bytes)
 * @return 0 si succès, -1 si erreur
 */
int lum_gpu_get_results(
    lum_gpu_context_t* ctx,
    int dispatch_id,
    uint32_t* out_nonce,
    uint8_t* out_hash
);

/**
 * Obtenir métriques performance
 * 
 * @param ctx Contexte GPU
 * @param out_hashrate Hashrate moyen (MH/s) (output)
 * @param out_dispatches Total dispatches (output)
 * @param out_time Temps total (sec) (output)
 * @return 0 si succès, -1 si erreur
 */
int lum_gpu_get_metrics(
    lum_gpu_context_t* ctx,
    double* out_hashrate,
    uint64_t* out_dispatches,
    double* out_time
);

/**
 * Nettoyer et libérer contexte GPU
 * 
 * @param ctx Contexte GPU à libérer
 */
void lum_gpu_cleanup(lum_gpu_context_t* ctx);

/**
 * Obtenir statistiques mémoire LUM
 * 
 * @param ctx Contexte GPU
 * @param out_total_alloc Total mémoire allouée (bytes) (output)
 * @param out_peak_usage Pic utilisation (bytes) (output)
 * @return 0 si succès, -1 si erreur
 */
int lum_gpu_get_memory_stats(
    lum_gpu_context_t* ctx,
    size_t* out_total_alloc,
    size_t* out_peak_usage
);

#endif /* LUM_GPU_CONTEXT_H */

// Made with Bob
