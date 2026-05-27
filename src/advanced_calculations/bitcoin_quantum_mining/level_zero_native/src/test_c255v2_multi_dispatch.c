/**
 * LumVorax C255v2 — Test Multi-Dispatch Parallèle CORRIGÉ
 * 
 * Test de l'architecture C255v2 avec soumission asynchrone réelle.
 * 
 * OBJECTIF: Valider hashrate ≥ 2 GH/s (×9 vs C255)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "btc_gen9_native_runner.h"

/* Déclaration fonction C255v2 */
extern int c255v2_execute_multi_dispatch_batch(btc_gen9_context_t* ctx, int num_batches);

/**
 * Main
 */
int main(int argc, char** argv) {
    printf("# LumVorax C255v2 — Multi-Dispatch Parallèle CORRIGÉ\n");
    printf("# Architecture: Soumission asynchrone + DRM reopen thread-safe\n");
    printf("# Device: Intel UHD Graphics 620 (Gen9)\n");
    printf("# Objectif: Hashrate ≥ 2 GH/s (×9 vs C255)\n\n");
    fflush(stdout);

    /* Nombre de batches (défaut: 1) */
    int num_batches = 1;
    if (argc > 1) {
        num_batches = atoi(argv[1]);
        if (num_batches <= 0) num_batches = 1;
    }

    printf("Configuration:\n");
    printf("  Batches: %d\n", num_batches);
    printf("  Dispatches per batch: 9\n");
    printf("  Total dispatches: %d\n\n", num_batches * 9);
    fflush(stdout);

    /* Initialiser contexte Gen9 */
    uint64_t ts_init_start = get_timestamp_ns();
    
    btc_gen9_context_t* ctx = btc_gen9_init_simple(
        268435456,  /* batch_size = 256M nonces */
        256         /* work_group_size */
    );

    if (!ctx) {
        fprintf(stderr, "ERROR: btc_gen9_init() failed\n");
        return 1;
    }

    uint64_t ts_init_end = get_timestamp_ns();
    double init_time = (ts_init_end - ts_init_start) / 1e9;
    printf("Initialization: %.6f sec\n\n", init_time);
    fflush(stdout);

    /* Exécuter multi-dispatch C255v2 */
    uint64_t ts_exec_start = get_timestamp_ns();
    
    int result = c255v2_execute_multi_dispatch_batch(ctx, num_batches);

    uint64_t ts_exec_end = get_timestamp_ns();
    double exec_time = (ts_exec_end - ts_exec_start) / 1e9;

    printf("\n");
    printf("Execution Result: %s\n", (result == 0) ? "SUCCESS" : "FAILED");
    printf("Total Execution Time: %.3f sec\n", exec_time);
    fflush(stdout);

    /* Cleanup */
    uint64_t ts_cleanup_start = get_timestamp_ns();
    btc_gen9_cleanup(ctx);
    uint64_t ts_cleanup_end = get_timestamp_ns();
    double cleanup_time = (ts_cleanup_end - ts_cleanup_start) / 1e9;

    printf("Cleanup: %.6f sec\n", cleanup_time);
    fflush(stdout);

    /* Résumé final */
    printf("\n");
    printf("=== RÉSUMÉ C255v2 ===\n");
    printf("Status: %s\n", (result == 0) ? "✓ SUCCESS" : "✗ FAILED");
    printf("Batches: %d\n", num_batches);
    printf("Total Time: %.3f sec\n", exec_time);
    printf("\n");
    printf("Comparaison:\n");
    printf("  C251 Baseline: 423.80 MH/s\n");
    printf("  C255 (séquentiel): 256.14 MH/s (0.6× vs C251)\n");
    printf("  C255v2 (parallèle): Voir logs ci-dessus\n");
    printf("  Objectif: ≥ 2 GH/s (×9 vs C255)\n");
    fflush(stdout);

    return (result == 0) ? 0 : 1;
}

// Made with Bob
