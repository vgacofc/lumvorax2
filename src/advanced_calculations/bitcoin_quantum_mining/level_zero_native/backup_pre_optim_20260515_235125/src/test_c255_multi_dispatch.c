/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_c255_multi_dispatch.c — Programme Test CYCLE C255
 *
 * OBJECTIF : Valider architecture multi-dispatch optimisée
 * BASELINE : C251 = 423.80 MH/s
 * OBJECTIF : 410 GH/s (gain ×968)
 *
 * TEST PLAN :
 * 1. Exécuter 10 batches (10 × 9 = 90 dispatches)
 * 2. Mesurer hashrate moyen
 * 3. Analyser cache misses
 * 4. Comparer vs C251 baseline
 *
 * CYCLE : C255
 * DATE : 2026-05-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/btc_gen9_native_runner.h"

/* Déclaration fonction C255 */
extern int c255_execute_multi_dispatch_batch(btc_gen9_context_t* ctx, int num_batches);

int main(int argc, char** argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  LumVorax C255 Multi-Dispatch Test\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    /* Configuration */
    int num_batches = 10;  /* 10 batches × 9 dispatches = 90 dispatches */
    
    if (argc > 1) {
        num_batches = atoi(argv[1]);
        if (num_batches <= 0) {
            fprintf(stderr, "ERROR: Invalid num_batches: %s\n", argv[1]);
            return 1;
        }
    }
    
    printf("Configuration:\n");
    printf("  Batches: %d\n", num_batches);
    printf("  Dispatches per batch: 9 (3 contexts × 3 cycles)\n");
    printf("  Total dispatches: %d\n", num_batches * 9);
    printf("  Nonces per dispatch: 268,435,456 (268M)\n");
    printf("  Total nonces: %lu\n", (unsigned long)num_batches * 9 * 268435456UL);
    printf("\n");
    
    /* Initialiser contexte Gen9 */
    printf("Initializing Gen9 context...\n");
    
    btc_gen9_config_t config = {
        .batch_size = 268435456,  /* 268M nonces (config C240) */
        .work_group_size = 256,
        .kernel_path = "kernels/btc_sha256_gen9.bin",
        .log_path = "logs/forensic/test_c255_multi_dispatch.log",
        .enable_profiling = true
    };
    
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0 || !ctx) {
        fprintf(stderr, "ERROR: Failed to initialize Gen9 context\n");
        return 1;
    }
    
    printf("✓ Gen9 context initialized\n");
    printf("  DRM FD: %d\n", btc_gen9_get_drm_fd(ctx));
    printf("  Context ID: %u\n", btc_gen9_get_ctx_id(ctx));
    printf("  Log file: %s\n\n", config.log_path);
    
    /* Exécuter test C255 */
    printf("Starting C255 multi-dispatch test...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    int dispatches_completed = c255_execute_multi_dispatch_batch(ctx, num_batches);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    double elapsed_sec = (end_time.tv_sec - start_time.tv_sec) +
                         (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("  C255 Test Results\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    if (dispatches_completed > 0) {
        uint64_t total_hashes = (uint64_t)dispatches_completed * 268435456UL;
        double hashrate_hs = total_hashes / elapsed_sec;
        double hashrate_mhs = hashrate_hs / 1e6;
        double hashrate_ghs = hashrate_hs / 1e9;
        
        printf("✓ Test SUCCESSFUL\n\n");
        printf("Performance Metrics:\n");
        printf("  Dispatches completed: %d / %d\n", dispatches_completed, num_batches * 9);
        printf("  Total time: %.3f sec\n", elapsed_sec);
        printf("  Avg time per dispatch: %.6f sec\n", elapsed_sec / dispatches_completed);
        printf("  Total hashes: %lu\n", (unsigned long)total_hashes);
        printf("  Hashrate: %.2f H/s\n", hashrate_hs);
        printf("  Hashrate: %.2f MH/s\n", hashrate_mhs);
        printf("  Hashrate: %.2f GH/s\n", hashrate_ghs);
        printf("\n");
        
        /* Comparaison vs baselines */
        double c251_baseline_mhs = 423.80;
        double c240_target_ghs = 410.60;
        
        double improvement_vs_c251 = hashrate_mhs / c251_baseline_mhs;
        double progress_vs_c240 = (hashrate_ghs / c240_target_ghs) * 100.0;
        
        printf("Comparison:\n");
        printf("  C251 Baseline: %.2f MH/s\n", c251_baseline_mhs);
        printf("  C255 Result: %.2f MH/s\n", hashrate_mhs);
        printf("  Improvement: %.1fx\n", improvement_vs_c251);
        printf("\n");
        printf("  C240 Target: %.2f GH/s\n", c240_target_ghs);
        printf("  C255 Result: %.2f GH/s\n", hashrate_ghs);
        printf("  Progress: %.1f%%\n", progress_vs_c240);
        printf("\n");
        
        if (hashrate_ghs >= c240_target_ghs) {
            printf("🎉 OBJECTIF C240 ATTEINT! (%.2f GH/s >= %.2f GH/s)\n", hashrate_ghs, c240_target_ghs);
        } else {
            double gap_ghs = c240_target_ghs - hashrate_ghs;
            double additional_speedup_needed = c240_target_ghs / hashrate_ghs;
            printf("⚠️  Objectif C240 non atteint\n");
            printf("  Gap: %.2f GH/s\n", gap_ghs);
            printf("  Additional speedup needed: %.1fx\n", additional_speedup_needed);
        }
    } else {
        printf("✗ Test FAILED\n");
        printf("  No dispatches completed successfully\n");
    }
    
    printf("\n");
    printf("Log file: %s\n", config.log_path);
    printf("\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return (dispatches_completed > 0) ? 0 : 1;
}

// Made with Bob
