/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c234_optimized.c — Test avec Optimisations #4 et #5
 *
 * CYCLE C234 — OPTIMISATIONS INTELLIGENTES
 *
 * OBJECTIF : Valider optimisations batch intelligent + prédiction thermal
 * - Optimisation #4 : Batch processing adaptatif selon température
 * - Optimisation #5 : Prédiction thermal ML pour pauses préventives
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Déclarations externes des nouvelles fonctions */
extern int btc_gen9_execute_batch_intelligent(btc_gen9_context_t* ctx, int max_dispatches);
extern int btc_gen9_execute_with_thermal_prediction(btc_gen9_context_t* ctx, int max_dispatches);

#define TEST_DISPATCHES_SHORT 100   /* Test court pour validation */
#define TEST_DISPATCHES_LONG 1000   /* Test long pour benchmark */

int main(int argc, char** argv) {
    int test_dispatches = TEST_DISPATCHES_SHORT;
    int use_thermal_prediction = 0;
    
    /* Parser arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--long") == 0) {
            test_dispatches = TEST_DISPATCHES_LONG;
        } else if (strcmp(argv[i], "--thermal-prediction") == 0) {
            use_thermal_prediction = 1;
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LumVorax C234 — Test Optimisations Intelligentes\n");
    printf("  Dispatches : %d\n", test_dispatches);
    printf("  Mode : %s\n", use_thermal_prediction ? 
           "Prédiction Thermal ML (Opt #5)" : "Batch Intelligent (Opt #4)");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    /* Configuration */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/btc_sha256_mining_gen9.bin";
    config.log_path = "logs/forensic/btc_mining_c234_optimized.log";
    config.batch_size = 262144;  /* 256K nonces par dispatch */
    
    printf("[CONFIG] Paramètres:\n");
    printf("  - kernel: %s\n", config.kernel_path);
    printf("  - batch_size: %u nonces\n", config.batch_size);
    printf("  - dispatches: %d\n", test_dispatches);
    printf("  - log: %s\n", config.log_path);
    printf("\n");
    
    /* Initialisation */
    printf("[PHASE 1/4] Initialisation Gen9 Native...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Warm-up : 10 dispatches normaux */
    printf("[PHASE 2/4] Warm-up — 10 Dispatches Standard\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec warmup_start, warmup_end;
    clock_gettime(CLOCK_MONOTONIC, &warmup_start);
    
    int warmup_success = 0;
    for (int i = 0; i < 10; i++) {
        ret = btc_gen9_execute(ctx);
        if (ret < 0) {
            printf("❌ Warm-up échec au dispatch %d\n", i + 1);
            btc_gen9_cleanup(ctx);
            return 1;
        }
        warmup_success++;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &warmup_end);
    double warmup_time = (warmup_end.tv_sec - warmup_start.tv_sec) + 
                         (warmup_end.tv_nsec - warmup_start.tv_nsec) / 1e9;
    
    printf("✅ Warm-up complété : %d/10 dispatches (%.3f sec)\n", 
           warmup_success, warmup_time);
    printf("\n");
    
    /* Test optimisations */
    printf("[PHASE 3/4] Test Optimisations — %d Dispatches\n", test_dispatches);
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec test_start, test_end;
    clock_gettime(CLOCK_MONOTONIC, &test_start);
    
    int dispatches_done = 0;
    if (use_thermal_prediction) {
        printf("Mode : Prédiction Thermal ML (Optimisation #5)\n");
        dispatches_done = btc_gen9_execute_with_thermal_prediction(ctx, test_dispatches);
    } else {
        printf("Mode : Batch Processing Intelligent (Optimisation #4)\n");
        dispatches_done = btc_gen9_execute_batch_intelligent(ctx, test_dispatches);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &test_end);
    double test_time = (test_end.tv_sec - test_start.tv_sec) + 
                       (test_end.tv_nsec - test_start.tv_nsec) / 1e9;
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    /* Résultats */
    printf("[PHASE 4/4] Résultats\n");
    printf("\n");
    
    if (dispatches_done == test_dispatches) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — OPTIMISATIONS VALIDÉES                  ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (100%%)                        ║\n", 
               dispatches_done, test_dispatches);
        printf("║  Temps total : %.3f sec                                   ║\n", test_time);
        printf("║  Temps moyen : %.3f ms/dispatch                           ║\n", 
               (test_time / dispatches_done) * 1000.0);
        printf("║  Throughput : %.3f dispatches/sec                         ║\n", 
               dispatches_done / test_time);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  ÉCHEC PARTIEL                                          ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (%.1f%%)                       ║\n", 
               dispatches_done, test_dispatches, 
               (float)dispatches_done / test_dispatches * 100.0f);
        printf("║  Temps écoulé : %.3f sec                                  ║\n", test_time);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    /* Métriques détaillées */
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Logs forensiques : %s\n", config.log_path);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    return (dispatches_done == test_dispatches) ? 0 : 1;
}

// Made with Bob - C234 Optimisations Intelligentes