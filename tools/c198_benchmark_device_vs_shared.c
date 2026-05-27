/**
 * @file c198_benchmark_device_vs_shared.c
 * @brief Benchmark CRITIQUE : Device vs Shared Memory (UMA)
 * 
 * PRIORITÉ 1 : Sur iGPU Intel UHD 620 (UMA), ce benchmark est DÉCISIF
 * 
 * HYPOTHÈSE :
 *   Sur UMA, shared memory devrait être BEAUCOUP plus rapide
 *   car évite copies CPU↔GPU artificielles
 * 
 * DATASETS RÉALISTES :
 *   - 4MB   (1M floats)
 *   - 16MB  (4M floats)
 *   - 64MB  (16M floats)
 *   - 256MB (64M floats)
 * 
 * MÉTRIQUES :
 *   - Temps allocation (ns)
 *   - Temps upload (ns)
 *   - Temps download (ns)
 *   - Débit GB/s
 *   - Comparaison Device vs Shared
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10E Priorité 1)
 * DATE : 2026-05-09
 */

#include "../src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/src/btc_hybrid_opencl_levelzero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * CONFIGURATION BENCHMARK
 * ============================================================================ */

typedef struct {
    size_t size_bytes;
    size_t num_floats;
    const char* label;
} dataset_config_t;

static const dataset_config_t DATASETS[] = {
    { 4 * 1024 * 1024,   1024 * 1024,      "4MB" },
    { 16 * 1024 * 1024,  4 * 1024 * 1024,  "16MB" },
    { 64 * 1024 * 1024,  16 * 1024 * 1024, "64MB" },
    { 256 * 1024 * 1024, 64 * 1024 * 1024, "256MB" }
};

#define NUM_DATASETS (sizeof(DATASETS) / sizeof(DATASETS[0]))
#define NUM_RUNS 10  // Runs multiples pour stabilité

/* ============================================================================
 * RÉSULTATS BENCHMARK
 * ============================================================================ */

typedef struct {
    uint64_t alloc_time_ns;
    uint64_t upload_time_ns;
    uint64_t download_time_ns;
    double upload_gbps;
    double download_gbps;
} benchmark_result_t;

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static double calculate_gbps(size_t bytes, uint64_t time_ns) {
    if (time_ns == 0) return 0.0;
    return (bytes / (double)time_ns) * 1000.0;  // GB/s
}

/* ============================================================================
 * BENCHMARK DEVICE MEMORY
 * ============================================================================ */

static bool benchmark_device_memory(
    btc_hybrid_ctx_t* ctx,
    const dataset_config_t* dataset,
    benchmark_result_t* result)
{
    printf("  [DEVICE] Allocation %s...\n", dataset->label);

    // Allocation
    uint64_t start_alloc = get_time_ns();
    btc_hybrid_buffer_t* buffer = btc_hybrid_alloc_device(ctx, dataset->size_bytes);
    result->alloc_time_ns = get_time_ns() - start_alloc;

    if (!buffer) {
        fprintf(stderr, "  [DEVICE] ❌ Allocation échouée\n");
        return false;
    }

    // Préparer données host
    float* host_data = malloc(dataset->size_bytes);
    if (!host_data) {
        fprintf(stderr, "  [DEVICE] ❌ Allocation host échouée\n");
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    for (size_t i = 0; i < dataset->num_floats; i++) {
        host_data[i] = (float)i;
    }

    // Upload
    printf("  [DEVICE] Upload %s...\n", dataset->label);
    uint64_t start_upload = get_time_ns();
    bool upload_ok = btc_hybrid_upload(ctx, buffer, host_data, dataset->size_bytes);
    result->upload_time_ns = get_time_ns() - start_upload;
    result->upload_gbps = calculate_gbps(dataset->size_bytes, result->upload_time_ns);

    if (!upload_ok) {
        fprintf(stderr, "  [DEVICE] ❌ Upload échoué\n");
        free(host_data);
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    // Download
    printf("  [DEVICE] Download %s...\n", dataset->label);
    uint64_t start_download = get_time_ns();
    bool download_ok = btc_hybrid_download(ctx, buffer, host_data, dataset->size_bytes);
    result->download_time_ns = get_time_ns() - start_download;
    result->download_gbps = calculate_gbps(dataset->size_bytes, result->download_time_ns);

    if (!download_ok) {
        fprintf(stderr, "  [DEVICE] ❌ Download échoué\n");
        free(host_data);
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    // Cleanup
    free(host_data);
    btc_hybrid_free_buffer(ctx, buffer);

    printf("  [DEVICE] ✅ Benchmark %s terminé\n", dataset->label);
    return true;
}

/* ============================================================================
 * BENCHMARK SHARED MEMORY
 * ============================================================================ */

static bool benchmark_shared_memory(
    btc_hybrid_ctx_t* ctx,
    const dataset_config_t* dataset,
    benchmark_result_t* result)
{
    printf("  [SHARED] Allocation %s...\n", dataset->label);

    // Allocation
    uint64_t start_alloc = get_time_ns();
    btc_hybrid_buffer_t* buffer = btc_hybrid_alloc_shared(ctx, dataset->size_bytes);
    result->alloc_time_ns = get_time_ns() - start_alloc;

    if (!buffer) {
        fprintf(stderr, "  [SHARED] ❌ Allocation échouée\n");
        return false;
    }

    // Préparer données host
    float* host_data = malloc(dataset->size_bytes);
    if (!host_data) {
        fprintf(stderr, "  [SHARED] ❌ Allocation host échouée\n");
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    for (size_t i = 0; i < dataset->num_floats; i++) {
        host_data[i] = (float)i;
    }

    // Upload (copie directe pour shared)
    printf("  [SHARED] Upload %s...\n", dataset->label);
    uint64_t start_upload = get_time_ns();
    bool upload_ok = btc_hybrid_upload(ctx, buffer, host_data, dataset->size_bytes);
    result->upload_time_ns = get_time_ns() - start_upload;
    result->upload_gbps = calculate_gbps(dataset->size_bytes, result->upload_time_ns);

    if (!upload_ok) {
        fprintf(stderr, "  [SHARED] ❌ Upload échoué\n");
        free(host_data);
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    // Download (copie directe pour shared)
    printf("  [SHARED] Download %s...\n", dataset->label);
    uint64_t start_download = get_time_ns();
    bool download_ok = btc_hybrid_download(ctx, buffer, host_data, dataset->size_bytes);
    result->download_time_ns = get_time_ns() - start_download;
    result->download_gbps = calculate_gbps(dataset->size_bytes, result->download_time_ns);

    if (!download_ok) {
        fprintf(stderr, "  [SHARED] ❌ Download échoué\n");
        free(host_data);
        btc_hybrid_free_buffer(ctx, buffer);
        return false;
    }

    // Cleanup
    free(host_data);
    btc_hybrid_free_buffer(ctx, buffer);

    printf("  [SHARED] ✅ Benchmark %s terminé\n", dataset->label);
    return true;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 BENCHMARK CRITIQUE — DEVICE VS SHARED MEMORY (UMA)      ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    printf("HYPOTHÈSE : Sur iGPU UMA, shared memory devrait être BEAUCOUP plus rapide\n");
    printf("DATASETS : 4MB, 16MB, 64MB, 256MB\n");
    printf("RUNS : %d par dataset\n\n", NUM_RUNS);

    /* ========================================================================
     * INITIALISATION
     * ======================================================================== */

    printf("=== INITIALISATION ===\n");

    btc_hybrid_ctx_t* ctx = btc_hybrid_init("./cache_benchmark", stdout);
    if (!ctx) {
        fprintf(stderr, "❌ ERREUR : Initialisation échouée\n");
        return 1;
    }

    printf("✅ Contexte initialisé\n\n");

    /* ========================================================================
     * BENCHMARKS
     * ======================================================================== */

    for (size_t d = 0; d < NUM_DATASETS; d++) {
        const dataset_config_t* dataset = &DATASETS[d];

        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║  DATASET : %s (%zu floats)                                    \n", 
               dataset->label, dataset->num_floats);
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");

        benchmark_result_t device_results[NUM_RUNS];
        benchmark_result_t shared_results[NUM_RUNS];

        // Benchmarks Device Memory
        printf("--- DEVICE MEMORY ---\n");
        for (int run = 0; run < NUM_RUNS; run++) {
            printf("\n[Run %d/%d]\n", run + 1, NUM_RUNS);
            if (!benchmark_device_memory(ctx, dataset, &device_results[run])) {
                fprintf(stderr, "❌ Benchmark device échoué (run %d)\n", run + 1);
                continue;
            }
        }

        // Benchmarks Shared Memory
        printf("\n--- SHARED MEMORY ---\n");
        for (int run = 0; run < NUM_RUNS; run++) {
            printf("\n[Run %d/%d]\n", run + 1, NUM_RUNS);
            if (!benchmark_shared_memory(ctx, dataset, &shared_results[run])) {
                fprintf(stderr, "❌ Benchmark shared échoué (run %d)\n", run + 1);
                continue;
            }
        }

        // Calcul moyennes
        uint64_t device_alloc_avg = 0, device_upload_avg = 0, device_download_avg = 0;
        uint64_t shared_alloc_avg = 0, shared_upload_avg = 0, shared_download_avg = 0;

        for (int run = 0; run < NUM_RUNS; run++) {
            device_alloc_avg += device_results[run].alloc_time_ns;
            device_upload_avg += device_results[run].upload_time_ns;
            device_download_avg += device_results[run].download_time_ns;

            shared_alloc_avg += shared_results[run].alloc_time_ns;
            shared_upload_avg += shared_results[run].upload_time_ns;
            shared_download_avg += shared_results[run].download_time_ns;
        }

        device_alloc_avg /= NUM_RUNS;
        device_upload_avg /= NUM_RUNS;
        device_download_avg /= NUM_RUNS;

        shared_alloc_avg /= NUM_RUNS;
        shared_upload_avg /= NUM_RUNS;
        shared_download_avg /= NUM_RUNS;

        // Affichage résultats
        printf("\n╔════════════════════════════════════════════════════════════════╗\n");
        printf("║  RÉSULTATS %s (moyenne %d runs)                              \n", 
               dataset->label, NUM_RUNS);
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");

        printf("DEVICE MEMORY :\n");
        printf("  Allocation : %.3f µs\n", device_alloc_avg / 1000.0);
        printf("  Upload     : %.3f ms (%.2f GB/s)\n", 
               device_upload_avg / 1000000.0,
               calculate_gbps(dataset->size_bytes, device_upload_avg));
        printf("  Download   : %.3f ms (%.2f GB/s)\n\n",
               device_download_avg / 1000000.0,
               calculate_gbps(dataset->size_bytes, device_download_avg));

        printf("SHARED MEMORY :\n");
        printf("  Allocation : %.3f µs\n", shared_alloc_avg / 1000.0);
        printf("  Upload     : %.3f ms (%.2f GB/s)\n",
               shared_upload_avg / 1000000.0,
               calculate_gbps(dataset->size_bytes, shared_upload_avg));
        printf("  Download   : %.3f ms (%.2f GB/s)\n\n",
               shared_download_avg / 1000000.0,
               calculate_gbps(dataset->size_bytes, shared_download_avg));

        printf("COMPARAISON (Shared vs Device) :\n");
        printf("  Allocation : %.2f× %s\n",
               (double)device_alloc_avg / shared_alloc_avg,
               device_alloc_avg > shared_alloc_avg ? "plus rapide" : "plus lent");
        printf("  Upload     : %.2f× %s\n",
               (double)device_upload_avg / shared_upload_avg,
               device_upload_avg > shared_upload_avg ? "plus rapide" : "plus lent");
        printf("  Download   : %.2f× %s\n\n",
               (double)device_download_avg / shared_download_avg,
               device_download_avg > shared_download_avg ? "plus rapide" : "plus lent");
    }

    /* ========================================================================
     * CLEANUP
     * ======================================================================== */

    printf("=== CLEANUP ===\n");
    btc_hybrid_destroy(ctx);
    printf("✅ Cleanup terminé\n\n");

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ BENCHMARK DEVICE VS SHARED TERMINÉ                         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Made with Bob