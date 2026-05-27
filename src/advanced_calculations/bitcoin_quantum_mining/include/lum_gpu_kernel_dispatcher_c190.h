JA /* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU KERNEL DISPATCHER C190 — HEADER
   ═══════════════════════════════════════════════════════════════════════════
   
   DISPATCH KERNEL SHA-256 NATIF SUR GPU INTEL GEN9
   
   CYCLE: C190
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   OBJECTIF:
   - Dispatch kernel SHA-256 natif sur 8 EU
   - Command buffers natifs (sans OpenCL)
   - Synchronisation GPU native
   - Logging bit-level complet
   
   ARCHITECTURE:
   - Utilise driver C189 pour accès registres
   - Compile kernel C187 pour GPU Gen9
   - Dispatch 8 EU × 7 threads = 56 threads parallèles
   - Synchronisation via polling registres
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef LUM_GPU_KERNEL_DISPATCHER_C190_H
#define LUM_GPU_KERNEL_DISPATCHER_C190_H

#include "lum_gpu_native_driver_c189.h"
#include <stdint.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
   CONSTANTES DISPATCH
   ═══════════════════════════════════════════════════════════════════════════ */

#define C190_MAX_KERNEL_SIZE        (64 * 1024)     /* 64 KB max kernel */
#define C190_MAX_COMMAND_SIZE       (16 * 1024)     /* 16 KB max commands */
#define C190_WORKGROUP_SIZE         64              /* 64 work-items/group */
#define C190_MAX_WORKGROUPS         128             /* 128 workgroups max */

/* Flags dispatch */
#define C190_DISPATCH_ASYNC         (1 << 0)        /* Dispatch asynchrone */
#define C190_DISPATCH_WAIT          (1 << 1)        /* Attendre completion */
#define C190_DISPATCH_PROFILE       (1 << 2)        /* Profiling activé */

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Kernel compilé */
typedef struct {
    void*       code;                   /* Code kernel compilé */
    size_t      code_size;              /* Taille code */
    uint32_t    entry_point;            /* Point d'entrée */
    uint32_t    num_args;               /* Nombre arguments */
} c190_kernel_t;

/* Argument kernel */
typedef struct {
    void*       data;                   /* Pointeur données */
    size_t      size;                   /* Taille données */
    uint32_t    flags;                  /* Flags argument */
} c190_kernel_arg_t;

/* Configuration dispatch */
typedef struct {
    size_t      global_work_size[3];   /* Taille globale travail */
    size_t      local_work_size[3];    /* Taille locale travail */
    uint32_t    flags;                  /* Flags dispatch */
} c190_dispatch_config_t;

/* Résultat dispatch */
typedef struct {
    uint64_t    start_time_ns;          /* Timestamp début */
    uint64_t    end_time_ns;            /* Timestamp fin */
    uint64_t    gpu_cycles;             /* Cycles GPU */
    uint64_t    instructions;           /* Instructions exécutées */
    uint32_t    eu_mask_used;           /* EU utilisés */
    int         status;                 /* Status (0=success) */
} c190_dispatch_result_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — INITIALISATION
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser dispatcher C190
 * 
 * @return 0 si succès, -1 si erreur
 * 
 * NOTE: Nécessite driver C189 déjà initialisé
 */
int c190_dispatcher_init(void);

/**
 * Cleanup dispatcher
 */
void c190_dispatcher_cleanup(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — GESTION KERNELS
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Compiler kernel depuis source C
 * 
 * @param source_code Code source C du kernel
 * @param source_size Taille source
 * @param kernel_out Pointeur pour stocker kernel compilé
 * @return 0 si succès, -1 si erreur
 */
int c190_compile_kernel(const char* source_code, size_t source_size, 
                        c190_kernel_t* kernel_out);

/**
 * Charger kernel pré-compilé
 * 
 * @param binary_code Code binaire kernel
 * @param binary_size Taille binaire
 * @param kernel_out Pointeur pour stocker kernel
 * @return 0 si succès, -1 si erreur
 */
int c190_load_kernel(const void* binary_code, size_t binary_size,
                     c190_kernel_t* kernel_out);

/**
 * Libérer kernel
 * 
 * @param kernel Kernel à libérer
 */
void c190_free_kernel(c190_kernel_t* kernel);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — DISPATCH KERNEL
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Dispatcher kernel sur GPU
 * 
 * @param kernel Kernel à exécuter
 * @param args Arguments kernel
 * @param num_args Nombre arguments
 * @param config Configuration dispatch
 * @param result_out Pointeur pour stocker résultat (peut être NULL)
 * @return 0 si succès, -1 si erreur
 * 
 * ACTIONS:
 * - Prépare command buffer
 * - Configure EU pour dispatch
 * - Lance exécution sur GPU
 * - Synchronise si DISPATCH_WAIT
 * - Collecte métriques si DISPATCH_PROFILE
 */
int c190_dispatch_kernel(const c190_kernel_t* kernel,
                         const c190_kernel_arg_t* args,
                         uint32_t num_args,
                         const c190_dispatch_config_t* config,
                         c190_dispatch_result_t* result_out);

/**
 * Attendre completion dispatch
 * 
 * @param timeout_ms Timeout en millisecondes (0 = infini)
 * @return 0 si succès, -1 si timeout/erreur
 */
int c190_wait_completion(uint32_t timeout_ms);

/**
 * Vérifier si dispatch en cours
 * 
 * @return 1 si en cours, 0 sinon
 */
int c190_is_dispatching(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — BITCOIN MINING SPÉCIALISÉ
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Dispatcher mining Bitcoin optimisé
 * 
 * @param midstate Midstate SHA-256 (32 bytes)
 * @param tail Tail header (16 bytes)
 * @param nonce_start Nonce de départ
 * @param batch_size Nombre de nonces à tester
 * @param target_bits Bits zéro requis
 * @param out_nonce Pointeur pour stocker nonce trouvé
 * @param out_hash Pointeur pour stocker hash (32 bytes)
 * @param out_best_bits Pointeur pour stocker meilleur bits
 * @return 0 si succès, -1 si erreur
 * 
 * OPTIMISATIONS:
 * - Utilise kernel SHA-256 natif C187
 * - Dispatch 8 EU × 7 threads = 56 threads parallèles
 * - Batch size optimal pour GPU Gen9
 * - Logging bit-level automatique
 */
int c190_mine_bitcoin_batch(
    const uint32_t  midstate[8],
    const uint32_t  tail[4],
    uint32_t        nonce_start,
    uint32_t        batch_size,
    uint32_t        target_bits,
    uint32_t*       out_nonce,
    uint32_t        out_hash[8],
    uint32_t*       out_best_bits
);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — STATISTIQUES
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Statistiques dispatcher
 */
typedef struct {
    uint64_t    total_dispatches;       /* Nombre dispatches */
    uint64_t    total_gpu_time_ns;      /* Temps GPU total */
    uint64_t    total_instructions;     /* Instructions totales */
    double      average_throughput_mhs; /* Throughput moyen MH/s */
    uint32_t    active_eu_count;        /* EU actifs */
} c190_dispatcher_stats_t;

/**
 * Obtenir statistiques dispatcher
 * 
 * @param stats_out Pointeur pour stocker statistiques
 * @return 0 si succès, -1 si erreur
 */
int c190_get_stats(c190_dispatcher_stats_t* stats_out);

/**
 * Reset statistiques
 */
void c190_reset_stats(void);

#endif /* LUM_GPU_KERNEL_DISPATCHER_C190_H */

/* ═══════════════════════════════════════════════════════════════════════════
   Made with ❤️ by Bob — LumVorax C190
   RÉVOLUTION: Dispatch kernel natif, 0 dépendance OpenCL
   ═══════════════════════════════════════════════════════════════════════════ */

// Made with Bob
