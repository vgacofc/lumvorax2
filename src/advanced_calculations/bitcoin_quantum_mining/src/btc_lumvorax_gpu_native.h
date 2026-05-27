/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU NATIVE - Header
   ═══════════════════════════════════════════════════════════════════════════
   
   Module OpenCL natif LumVorax pour Bitcoin mining avec :
   - Events asynchrones (pas de clFinish bloquant)
   - Profiling hardware bit-level (clGetEventProfilingInfo)
   - Traçabilité JIT par JIT de toutes opérations GPU
   - CPU et GPU travaillent en parallèle
   
   CYCLE: C176
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ROOT CAUSE RÉSOLU:
   - btc_opencl_runner.c utilisait clFinish() bloquant (3 fois)
   - Performance: 0.003 MH/s au lieu de 50-200 MH/s
   - Ce module remplace l'ancien avec architecture asynchrone
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef BTC_LUMVORAX_GPU_NATIVE_H
#define BTC_LUMVORAX_GPU_NATIVE_H

#include <stdint.h>
#include <stddef.h>
#include <CL/cl.h>

/* ═══════════════════════════════════════════════════════════════════════════
   CODES RETOUR
   ═══════════════════════════════════════════════════════════════════════════ */

#define LUM_GPU_OK                  0
#define LUM_GPU_ERR_NO_PLATFORM    -1
#define LUM_GPU_ERR_NO_DEVICE      -2
#define LUM_GPU_ERR_CONTEXT        -3
#define LUM_GPU_ERR_QUEUE          -4
#define LUM_GPU_ERR_PROGRAM        -5
#define LUM_GPU_ERR_KERNEL         -6
#define LUM_GPU_ERR_BUFFER         -7
#define LUM_GPU_ERR_RUNTIME        -8
#define LUM_GPU_ERR_PROFILING      -9
#define LUM_GPU_ERR_NOT_INIT      -10

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Statistiques temps réel GPU */
typedef struct {
    /* Timings hardware (nanoseconds) */
    uint64_t upload_midstate_ns;      /* Temps upload midstate */
    uint64_t upload_tail_ns;          /* Temps upload tail */
    uint64_t upload_sentinel_ns;      /* Temps upload sentinel */
    uint64_t kernel_exec_ns;          /* Temps exécution kernel */
    uint64_t download_result_ns;      /* Temps download résultat */
    uint64_t download_hash_ns;        /* Temps download hash */
    uint64_t download_leading_ns;     /* Temps download leading zeros */
    
    /* C219-PIPELINE-METRICS : Métriques pipeline OpenCL complètes */
    uint64_t gpu_queue_wait_ns;       /* Temps attente dans queue (submit - queued) */
    uint64_t gpu_transfer_ns;         /* Temps transfert/setup (start - submit) */
    uint64_t gpu_total_pipeline_ns;   /* Temps pipeline complet (end - queued) */
    
    /* Totaux */
    uint64_t total_gpu_time_ns;       /* Temps GPU total (hardware) */
    uint64_t total_cpu_wait_ns;       /* Temps CPU wait (si bloquant) */
    
    /* Compteurs */
    uint64_t total_batches;           /* Nombre de batches traités */
    uint64_t total_hashes;            /* Nombre de hashes calculés */
    
    /* Performance */
    double   current_mhs;             /* MH/s actuel */
    double   average_mhs;             /* MH/s moyen */
    double   peak_mhs;                /* MH/s pic */
    
    /* Efficacité */
    double   gpu_utilization_pct;    /* % utilisation GPU */
    double   cpu_free_pct;            /* % CPU libre (parallélisme) */
    
    /* C213-BUG-FIX: Near-miss GPU */
    uint32_t best_leading_zeros_gpu;  /* Meilleur leading zeros trouvé par GPU */
    
} lum_gpu_stats_t;

/* Configuration GPU */
typedef struct {
    size_t   batch_size;              /* Taille batch (nonces) */
    size_t   workgroup_size;          /* Taille workgroup OpenCL */
    uint32_t target_bits;             /* Bits zéro requis */
    int      enable_profiling;        /* Activer profiling events */
    int      enable_bit_level_log;    /* Activer logging bit-level */
    char     log_dir[256];            /* Répertoire logs forensiques */
} lum_gpu_config_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE
   ═══════════════════════════════════════════════════════════════════════════ */

/* Initialisation GPU avec configuration */
int lum_gpu_init(const lum_gpu_config_t* config);

/* C214-DOUBLE-BUFFERING: Mining asynchrone avec support 2 buffers alternés
 * buffer_index: 0 ou 1 (permet pipeline GPU sans idle)
 * Utilisation:
 *   lum_gpu_mine_batch_async(0, ...) // Lance batch N sur buffer 0
 *   lum_gpu_mine_batch_async(1, ...) // Lance batch N+1 sur buffer 1 (pendant N)
 *   lum_gpu_wait_batch(0)            // Attend batch N
 *   lum_gpu_mine_batch_async(0, ...) // Lance batch N+2 sur buffer 0 (pendant N+1)
 *   lum_gpu_wait_batch(1)            // Attend batch N+1
 */
int lum_gpu_mine_batch_async(
    int             buffer_index,     /* C214: Index buffer (0 ou 1) */
    const uint32_t  midstate[8],      /* Midstate SHA-256 */
    const uint32_t  tail[4],          /* Tail du header */
    uint32_t        nonce_start,      /* Nonce de départ */
    size_t          batch_size,       /* Nombre de nonces */
    uint32_t        target_bits,      /* Bits zéro requis */
    uint32_t*       out_nonce,        /* Output: nonce trouvé */
    uint32_t        out_hash[8],      /* Output: hash final */
    uint32_t*       out_best_bits     /* Output: meilleur bits du batch */
);

/* C214-DOUBLE-BUFFERING: Attendre fin d'un batch spécifique
 * buffer_index: 0 ou 1 (quel buffer attendre) */
int lum_gpu_wait_batch(int buffer_index, uint32_t* out_best_bits);

/* Obtenir statistiques temps réel */
int lum_gpu_get_stats(lum_gpu_stats_t* stats);

/* Reset statistiques */
void lum_gpu_reset_stats(void);

/* Benchmark rapide (retourne MH/s) */
double lum_gpu_benchmark_mhs(size_t batch_size, double duration_sec);

/* Cleanup GPU */
void lum_gpu_cleanup(void);

/* ═══════════════════════════════════════════════════════════════════════════
   PROFILING BIT-LEVEL (optionnel)
   ═══════════════════════════════════════════════════════════════════════════ */

/* Activer/désactiver logging bit-level */
void lum_gpu_set_bit_level_logging(int enable);

/* Obtenir dernier event profiling (debug) */
int lum_gpu_get_last_event_info(
    uint64_t* queued_ns,
    uint64_t* submit_ns,
    uint64_t* start_ns,
    uint64_t* end_ns
);

/* Dump complet état GPU (forensique) */
void lum_gpu_dump_state(const char* filename);

#endif /* BTC_LUMVORAX_GPU_NATIVE_H */

// Made with Bob
