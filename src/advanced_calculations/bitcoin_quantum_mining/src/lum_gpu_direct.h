/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU DIRECT - Header
   ═══════════════════════════════════════════════════════════════════════════
   
   Accès GPU Intel UHD 620 via DRM (Direct Rendering Manager)
   BYPASS COMPLET OpenCL pour éliminer 64% overheads
   
   CYCLE: C180
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef LUM_GPU_DIRECT_H
#define LUM_GPU_DIRECT_H

#include <stdint.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Configuration GPU Direct */
typedef struct {
    size_t   batch_size;              /* Taille batch (nonces) */
    size_t   workgroup_size;          /* Taille workgroup */
    uint32_t target_bits;             /* Bits zéro requis */
} lum_gpu_direct_config_t;

/* Statistiques GPU Direct */
typedef struct {
    uint64_t total_batches;           /* Nombre de batches traités */
    uint64_t total_hashes;            /* Nombre de hashes calculés */
    uint64_t total_gpu_time_ns;       /* Temps GPU total (nanoseconds) */
    double   average_mhs;             /* MH/s moyen */
} lum_gpu_direct_stats_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE
   ═══════════════════════════════════════════════════════════════════════════ */

/* Initialisation GPU Direct avec configuration */
int lum_gpu_direct_init(const lum_gpu_direct_config_t* config);

/* Mining synchrone d'un batch */
int lum_gpu_direct_mine_batch(
    const uint32_t  midstate[8],      /* Midstate SHA-256 */
    const uint32_t  tail[4],          /* Tail du header */
    uint32_t        nonce_start,      /* Nonce de départ */
    size_t          batch_size,       /* Nombre de nonces */
    uint32_t        target_bits,      /* Bits zéro requis */
    uint32_t*       out_nonce,        /* Output: nonce trouvé */
    uint32_t        out_hash[8],      /* Output: hash final */
    uint32_t*       out_best_bits     /* Output: meilleur bits du batch */
);

/* Obtenir statistiques */
int lum_gpu_direct_get_stats(lum_gpu_direct_stats_t* stats);

/* Benchmark rapide (retourne MH/s) */
double lum_gpu_direct_benchmark_mhs(size_t batch_size, double duration_sec);

/* Cleanup GPU */
void lum_gpu_direct_cleanup(void);

#endif /* LUM_GPU_DIRECT_H */

// Made with  by Bob - LumVorax C180

// Made with Bob
