/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU BENCHMARK COMPARATIF C192 — HEADER
   ═══════════════════════════════════════════════════════════════════════════
   
   BENCHMARK COMPARATIF FINAL
   
   CYCLE: C192
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   OBJECTIF:
   - Benchmark comparatif C191 vs C180 (OpenCL/DRM)
   - Benchmark comparatif C191 vs C187 (CPU natif)
   - Métriques détaillées (MH/s, latence, overhead)
   - Rapport final C189-C192
   
   ARCHITECTURE:
   - Utilise C191 (optimisations GPU)
   - Utilise C187 (kernel CPU natif)
   - Compare vs C180 (baseline OpenCL/DRM: 23.14 MH/s)
   - Logging forensique complet
   
   ═══════════════════════════════════════════════════════════════════════════ */

#ifndef LUM_GPU_BENCHMARK_C192_H
#define LUM_GPU_BENCHMARK_C192_H

#include "lum_gpu_optimizations_c191.h"
#include "bitcoin_mining_kernel_native_c187.h"
#include <stdint.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
   CONSTANTES BENCHMARK
   ═══════════════════════════════════════════════════════════════════════════ */

#define C192_BENCHMARK_DURATION     90              /* 90s benchmark */
#define C192_WARMUP_DURATION        5               /* 5s warm-up */
#define C192_BATCH_SIZE             262144          /* 256k nonces */

/* Baselines historiques */
#define C192_BASELINE_C180_MHS      23.14           /* OpenCL/DRM */
#define C192_BASELINE_C187_MHS      3.87            /* CPU natif */

/* ═══════════════════════════════════════════════════════════════════════════
   STRUCTURES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Résultat benchmark comparatif */
typedef struct {
    /* Métriques C191 (GPU natif) */
    double      c191_average_mhs;
    double      c191_peak_mhs;
    double      c191_min_mhs;
    uint64_t    c191_total_hashes;
    
    /* Métriques C187 (CPU natif) */
    double      c187_average_mhs;
    double      c187_peak_mhs;
    double      c187_min_mhs;
    uint64_t    c187_total_hashes;
    
    /* Comparaison vs C180 (OpenCL/DRM baseline) */
    double      c180_baseline_mhs;
    double      c191_vs_c180_speedup;
    double      c191_vs_c180_percent;
    
    /* Comparaison C191 vs C187 */
    double      c191_vs_c187_speedup;
    double      c191_vs_c187_percent;
    
    /* Métriques globales */
    uint32_t    benchmark_duration_s;
    uint64_t    total_time_ns;
    
} c192_benchmark_result_t;

/* Configuration benchmark */
typedef struct {
    uint32_t    duration_s;                 /* Durée benchmark (s) */
    uint32_t    warmup_s;                   /* Durée warm-up (s) */
    uint32_t    batch_size;                 /* Taille batch nonces */
    const char* log_file_path;              /* Chemin fichier log */
    const char* report_file_path;           /* Chemin rapport final */
} c192_benchmark_config_t;

/* ═══════════════════════════════════════════════════════════════════════════
   API PUBLIQUE — BENCHMARK
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser module benchmark C192
 * 
 * @param config Configuration benchmark
 * @return 0 si succès, -1 si erreur
 */
int c192_benchmark_init(const c192_benchmark_config_t* config);

/**
 * Cleanup module benchmark
 */
void c192_benchmark_cleanup(void);

/**
 * Lancer benchmark comparatif complet
 * 
 * @param result_out Pointeur pour stocker résultat
 * @return 0 si succès, -1 si erreur
 * 
 * ACTIONS:
 * - Warm-up 5s
 * - Benchmark C191 (GPU natif) 90s
 * - Benchmark C187 (CPU natif) 90s
 * - Comparaison vs C180 (baseline)
 * - Logging forensique complet
 */
int c192_run_comparative_benchmark(c192_benchmark_result_t* result_out);

/**
 * Générer rapport final C189-C192
 * 
 * @param result Résultat benchmark
 * @param report_file Fichier rapport
 * @return 0 si succès, -1 si erreur
 * 
 * CONTENU RAPPORT:
 * - Synthèse C189 (driver GPU natif)
 * - Synthèse C190 (dispatcher)
 * - Synthèse C191 (optimisations)
 * - Benchmark comparatif C192
 * - Métriques détaillées
 * - Conclusion finale
 */
int c192_generate_final_report(const c192_benchmark_result_t* result,
                                const char* report_file);

#endif /* LUM_GPU_BENCHMARK_C192_H */

// Made with Bob