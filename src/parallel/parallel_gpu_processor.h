/*
 * LumVorax — Module Parallel GPU Processor
 * parallel_gpu_processor.h — Traitement Parallèle GPU-Accelerated
 *
 * OBJECTIF: Remplacer pthread par dispatches GPU parallèles
 * - 100% i915 DRM natif (0% OpenCL, 0% Level Zero)
 * - Pool contextes GPU (9 contextes)
 * - Dispatches asynchrones avec synchronisation
 * - Intégration complète avec lum_gpu_context
 *
 * ARCHITECTURE:
 * Application → parallel_gpu_processor → lum_gpu_context → i915 DRM → GPU Gen9
 */

#ifndef PARALLEL_GPU_PROCESSOR_H
#define PARALLEL_GPU_PROCESSOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

/* Intégration module GPU */
#include "../lum/lum_gpu_context.h"

/* Intégration modules LUM existants */
#include "../lum/lum_memory_tracer.h"
#include "../lum/lum_log_encoder.h"

/* ══════════════════════════════════════════════════════════════════════
 * CONSTANTES
 * ══════════════════════════════════════════════════════════════════════ */

#define PARALLEL_GPU_MAX_TASKS          256     /* Max tâches en queue */
#define PARALLEL_GPU_MAX_DISPATCHES     9       /* Max dispatches parallèles (= nb contextes GPU) */
#define PARALLEL_GPU_DEFAULT_BATCH_SIZE 262144  /* 256K nonces par défaut */

/* ══════════════════════════════════════════════════════════════════════
 * TYPES DE TÂCHES GPU
 * ══════════════════════════════════════════════════════════════════════ */

typedef enum {
    PARALLEL_GPU_TASK_MINING,           /* Mining Bitcoin (SHA-256) */
    PARALLEL_GPU_TASK_HASH_COMPUTE,     /* Calcul hash générique */
    PARALLEL_GPU_TASK_MATRIX_MULT,      /* Multiplication matrices */
    PARALLEL_GPU_TASK_VECTOR_ADD,       /* Addition vecteurs */
    PARALLEL_GPU_TASK_CUSTOM            /* Tâche personnalisée */
} parallel_gpu_task_type_e;

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Tâche GPU parallèle
 */
typedef struct {
    /* Identification */
    int task_id;                        /* ID unique tâche */
    parallel_gpu_task_type_e type;      /* Type tâche */
    
    /* Données entrée */
    const void* input_data;             /* Pointeur données entrée */
    size_t input_size;                  /* Taille données entrée */
    
    /* Données sortie */
    void* output_data;                  /* Pointeur données sortie */
    size_t output_size;                 /* Taille données sortie */
    
    /* Paramètres exécution */
    uint64_t start_param;               /* Paramètre début (ex: nonce_start) */
    uint32_t count_param;               /* Paramètre compteur (ex: nonce_count) */
    
    /* État exécution */
    bool submitted;                     /* Tâche soumise au GPU */
    bool completed;                     /* Tâche complétée */
    int error_code;                     /* Code erreur (0 = succès) */
    
    /* Métriques */
    uint64_t submit_time_ns;            /* Timestamp soumission */
    uint64_t complete_time_ns;          /* Timestamp complétion */
    double execution_time_sec;          /* Temps exécution */
    
    /* Dispatch GPU associé */
    int dispatch_id;                    /* ID dispatch GPU (-1 si non assigné) */
} parallel_gpu_task_t;

/**
 * Queue tâches GPU (thread-safe)
 */
typedef struct {
    parallel_gpu_task_t tasks[PARALLEL_GPU_MAX_TASKS];
    int head;                           /* Index tête queue */
    int tail;                           /* Index queue queue */
    int count;                          /* Nombre tâches en queue */
    pthread_mutex_t mutex;              /* Mutex protection */
    pthread_cond_t not_empty;           /* Condition: queue non vide */
    pthread_cond_t not_full;            /* Condition: queue non pleine */
} parallel_gpu_queue_t;

/**
 * Statistiques processeur GPU
 */
typedef struct {
    uint64_t total_tasks_submitted;     /* Total tâches soumises */
    uint64_t total_tasks_completed;     /* Total tâches complétées */
    uint64_t total_tasks_failed;        /* Total tâches échouées */
    double total_execution_time_sec;    /* Temps exécution total */
    double avg_execution_time_sec;      /* Temps exécution moyen */
    double peak_throughput;             /* Débit peak (tâches/sec) */
} parallel_gpu_stats_t;

/**
 * Processeur GPU parallèle (contexte principal)
 */
typedef struct {
    /* Contexte GPU sous-jacent */
    lum_gpu_context_t* gpu_ctx;         /* Contexte GPU LUM */
    
    /* Queue tâches */
    parallel_gpu_queue_t task_queue;    /* Queue tâches à exécuter */
    
    /* Dispatches actifs */
    parallel_gpu_task_t* active_tasks[PARALLEL_GPU_MAX_DISPATCHES];
    int num_active_dispatches;          /* Nombre dispatches actifs */
    pthread_mutex_t dispatch_mutex;     /* Mutex protection dispatches */
    
    /* Thread scheduler */
    pthread_t scheduler_thread;         /* Thread ordonnanceur */
    bool scheduler_running;             /* Flag thread actif */
    pthread_mutex_t scheduler_mutex;    /* Mutex protection scheduler */
    
    /* Configuration */
    int max_parallel_dispatches;        /* Max dispatches parallèles */
    uint32_t default_batch_size;        /* Batch size par défaut */
    bool enable_adaptive_batch;         /* Adaptation batch dynamique */
    
    /* Statistiques */
    parallel_gpu_stats_t stats;         /* Statistiques globales */
    
    /* Intégration LUM */
    lum_memory_tracer_t* mem_tracer;    /* Traçage mémoire */
    lum_log_encoder_t* logger;          /* Logging unifié */
    
    /* Forensique */
    FILE* log_file;                     /* Fichier log forensique */
    uint64_t event_counter;             /* Compteur événements */
} parallel_gpu_processor_t;

/**
 * Configuration initialisation
 */
typedef struct {
    const char* gpu_kernel_path;        /* Chemin kernel GPU */
    const char* log_path;               /* Chemin log forensique */
    int max_parallel_dispatches;        /* Max dispatches parallèles (1-9) */
    uint32_t default_batch_size;        /* Batch size par défaut */
    bool enable_adaptive_batch;         /* Activer adaptation batch */
    bool enable_thp;                    /* Activer THP (recommandé: false) */
    bool enable_profiling;              /* Activer profiling */
} parallel_gpu_config_t;

/* ══════════════════════════════════════════════════════════════════════
 * API PUBLIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Créer processeur GPU parallèle
 * 
 * @param config Configuration initialisation
 * @return Pointeur processeur ou NULL si erreur
 */
parallel_gpu_processor_t* parallel_gpu_processor_create(
    const parallel_gpu_config_t* config
);

/**
 * Soumettre tâche GPU
 * 
 * @param processor Processeur GPU
 * @param type Type tâche
 * @param input_data Données entrée
 * @param input_size Taille données entrée
 * @param output_data Données sortie
 * @param output_size Taille données sortie
 * @param start_param Paramètre début (ex: nonce_start)
 * @param count_param Paramètre compteur (ex: nonce_count)
 * @return ID tâche ou -1 si erreur
 */
int parallel_gpu_submit_task(
    parallel_gpu_processor_t* processor,
    parallel_gpu_task_type_e type,
    const void* input_data,
    size_t input_size,
    void* output_data,
    size_t output_size,
    uint64_t start_param,
    uint32_t count_param
);

/**
 * Attendre complétion tâche
 * 
 * @param processor Processeur GPU
 * @param task_id ID tâche
 * @param timeout_sec Timeout secondes (0 = infini)
 * @return 0 si succès, -1 si erreur/timeout
 */
int parallel_gpu_wait_task(
    parallel_gpu_processor_t* processor,
    int task_id,
    double timeout_sec
);

/**
 * Attendre complétion toutes tâches
 * 
 * @param processor Processeur GPU
 * @param timeout_sec Timeout secondes (0 = infini)
 * @return Nombre tâches complétées
 */
int parallel_gpu_wait_all(
    parallel_gpu_processor_t* processor,
    double timeout_sec
);

/**
 * Obtenir statistiques
 * 
 * @param processor Processeur GPU
 * @param stats Pointeur structure stats (sortie)
 * @return 0 si succès, -1 si erreur
 */
int parallel_gpu_get_stats(
    parallel_gpu_processor_t* processor,
    parallel_gpu_stats_t* stats
);

/**
 * Détruire processeur GPU
 * 
 * @param processor Processeur GPU
 */
void parallel_gpu_processor_destroy(
    parallel_gpu_processor_t* processor
);

/* ══════════════════════════════════════════════════════════════════════
 * API AVANCÉE (OPTIONNELLE)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Soumettre batch tâches
 * 
 * @param processor Processeur GPU
 * @param tasks Tableau tâches
 * @param num_tasks Nombre tâches
 * @return Nombre tâches soumises
 */
int parallel_gpu_submit_batch(
    parallel_gpu_processor_t* processor,
    const parallel_gpu_task_t* tasks,
    int num_tasks
);

/**
 * Annuler tâche
 * 
 * @param processor Processeur GPU
 * @param task_id ID tâche
 * @return 0 si succès, -1 si erreur
 */
int parallel_gpu_cancel_task(
    parallel_gpu_processor_t* processor,
    int task_id
);

/**
 * Obtenir état tâche
 * 
 * @param processor Processeur GPU
 * @param task_id ID tâche
 * @param task Pointeur structure tâche (sortie)
 * @return 0 si succès, -1 si erreur
 */
int parallel_gpu_get_task_status(
    parallel_gpu_processor_t* processor,
    int task_id,
    parallel_gpu_task_t* task
);

#endif /* PARALLEL_GPU_PROCESSOR_H */

// Made with Bob
