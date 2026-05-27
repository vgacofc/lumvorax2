/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU OPTIMIZATIONS C191 — HEADER
   ═══════════════════════════════════════════════════════════════════════════
   
   OPTIMISATIONS FORENSIQUES GPU NATIF
   
   CYCLE: C191
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   OBJECTIF:
   - Implémenter dispatch GPU réel (vs stub CPU C190)
   - Command buffers i915 natifs
   - Allocation mémoire GPU directe
   - Batching registres pour réduire latence
   - Cache status EU pour éviter lectures répétées
   - Benchmark 90s avec logging forensique complet
   
   ARCHITECTURE:
   - Utilise driver C189 + dispatcher C190
   - Command buffers i915 format natif
   - Zero-copy GPU memory via mmap
   - Dispatch 8 EU × 7 threads = 56 threads parallèles
   - Logging bit-level chaque opération
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef LUM_GPU_OPTIMIZATIONS_C191_H
#define LUM_GPU_OPTIMIZATIONS_C191_H

#include "lum_gpu_native_driver_c189.h"
#include "lum_gpu_kernel_dispatcher_c190.h"
#include <stdint.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
   CONSTANTES OPTIMISATIONS
   ═══════════════════════════════════════════════════════════════════════════ */

#define C191_COMMAND_BUFFER_SIZE    (4 * 1024)      /* 4 KB command buffer */
#define C191_BATCH_REGISTERS        32              /* Batch 32 registres */
#define C191_CACHE_EU_STATUS        1               /* Cache EU status */
#define C191_BENCHMARK_DURATION     90              /* 90s benchmark */
#define C191_LOG_INTERVAL_MS        1000            /* Log chaque 1s */

/* Flags optimisations */
#define C191_OPT_BATCH_REGISTERS    (1 << 0)        /* Batching registres */
#define C191_OPT_CACHE_STATUS       (1 << 1)        /* Cache status */
#define C191_OPT_ZERO_COPY          (1 << 2)        /* Zero-copy mmap */
#define C191_OPT_COMMAND_BUFFER     (1 << 3)        /* Command buffers natifs */

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Command buffer i915 natif */
typedef struct {
    uint32_t    cmd_type;               /* Type commande */
    uint32_t    cmd_size;               /* Taille commande */
    uint32_t    cmd_flags;              /* Flags commande */
    uint32_t    batch_start_offset;     /* Offset début batch */
    uint32_t    batch_len;              /* Longueur batch */
    uint32_t    num_cliprects;          /* Nombre cliprects */
    uint64_t    cliprects_ptr;          /* Pointeur cliprects */
    uint64_t    flags;                  /* Flags exécution */
    uint64_t    rsvd1;                  /* Réservé */
    uint64_t    rsvd2;                  /* Réservé */
} c191_command_buffer_t;

/* Flags mémoire GPU */
#define C191_GPU_MEM_FLAG_COHERENT  (1 << 0)  /* Mémoire cohérente CPU-GPU */

/* Mémoire GPU native */
typedef struct {
    uint64_t    physical_addr;          /* Adresse physique GPU */
    void*       virtual_addr;           /* Adresse virtuelle CPU */
    size_t      size;                   /* Taille buffer */
    uint32_t    handle;                 /* Handle GEM */
    uint32_t    flags;                  /* Flags allocation */
} c191_gpu_memory_t;

/* Configuration optimisations */
typedef struct {
    uint32_t    optimization_flags;     /* Flags optimisations */
    uint32_t    batch_size_registers;   /* Taille batch registres */
    uint32_t    cache_ttl_ms;           /* TTL cache (ms) */
    uint32_t    benchmark_duration_s;   /* Durée benchmark (s) */
    const char* log_file_path;          /* Chemin fichier log */
} c191_optimization_config_t;

/* Résultat benchmark */
typedef struct {
    uint64_t    total_hashes;           /* Hashes totaux */
    uint64_t    total_time_ns;          /* Temps total (ns) */
    double      average_mhs;            /* MH/s moyen */
    double      peak_mhs;               /* MH/s pic */
    double      min_mhs;                /* MH/s min */
    uint64_t    total_dispatches;       /* Dispatches totaux */
    uint64_t    gpu_time_ns;            /* Temps GPU (ns) */
    uint64_t    cpu_time_ns;            /* Temps CPU (ns) */
    uint32_t    cache_hits;             /* Cache hits */
    uint32_t    cache_misses;           /* Cache misses */
} c191_benchmark_result_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — INITIALISATION
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser optimisations C191
 * 
 * @param config Configuration optimisations
 * @return 0 si succès, -1 si erreur
 * 
 * NOTE: Nécessite driver C189 + dispatcher C190 déjà initialisés
 */
int c191_optimizations_init(const c191_optimization_config_t* config);

/**
 * Cleanup optimisations
 */
void c191_optimizations_cleanup(void);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — COMMAND BUFFERS
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Créer command buffer i915 natif
 * 
 * @param cmd_buffer_out Pointeur pour stocker command buffer
 * @return 0 si succès, -1 si erreur
 */
int c191_create_command_buffer(c191_command_buffer_t* cmd_buffer_out);

/**
 * Soumettre command buffer au GPU
 * 
 * @param cmd_buffer Command buffer à soumettre
 * @return 0 si succès, -1 si erreur
 */
int c191_submit_command_buffer(const c191_command_buffer_t* cmd_buffer);

/**
 * Libérer command buffer
 * 
 * @param cmd_buffer Command buffer à libérer
 */
void c191_free_command_buffer(c191_command_buffer_t* cmd_buffer);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — MÉMOIRE GPU
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Allouer mémoire GPU native
 * 
 * @param size Taille à allouer
 * @param memory_out Pointeur pour stocker mémoire
 * @return 0 si succès, -1 si erreur
 */
int c191_allocate_gpu_memory(size_t size, c191_gpu_memory_t* memory_out);

/**
 * Libérer mémoire GPU
 * 
 * @param memory Mémoire à libérer
 */
void c191_free_gpu_memory(c191_gpu_memory_t* memory);

/**
 * Mapper mémoire GPU en CPU (zero-copy)
 * 
 * @param memory Mémoire GPU à mapper
 * @return Pointeur CPU, NULL si erreur
 */
void* c191_map_gpu_memory(c191_gpu_memory_t* memory);

/**
 * Unmapper mémoire GPU
 * 
 * @param memory Mémoire à unmapper
 */
void c191_unmap_gpu_memory(c191_gpu_memory_t* memory);

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — DISPATCH GPU RÉEL
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Dispatcher kernel SHA-256 sur GPU réel
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
 * - Command buffers i915 natifs
 * - Zero-copy GPU memory
 * - Batching registres
 * - Cache EU status
 * - Dispatch 8 EU × 7 threads = 56 threads
 */
int c191_dispatch_gpu_real(
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
   API PUBLIQUE — BENCHMARK
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Lancer benchmark 90s avec logging forensique
 * 
 * @param result_out Pointeur pour stocker résultat
 * @return 0 si succès, -1 si erreur
 * 
 * ACTIONS:
 * - Benchmark 90s continu
 * - Logging bit-level chaque seconde
 * - Métriques détaillées (MH/s, latence, cache)
 * - Analyse forensique complète
 */
int c191_run_benchmark_90s(c191_benchmark_result_t* result_out);

/**
 * Analyser résultats benchmark
 * 
 * @param result Résultat benchmark
 * @param analysis_file Fichier pour écrire analyse
 * @return 0 si succès, -1 si erreur
 */
int c191_analyze_benchmark(const c191_benchmark_result_t* result,
                           const char* analysis_file);

#endif /* LUM_GPU_OPTIMIZATIONS_C191_H */

// Made with Bob
