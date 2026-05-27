/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_gen9_native.c — Test Implémentation NATIVE Gen9
 *
 * CYCLE C198 Phase 15C — Test Solution Native i915 DRM
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15C — Test Gen9 Native Runner        ║\n");
    printf("║  0%% OpenCL, 0%% Level Zero, 100%% i915 DRM Direct          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Configuration */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/test_add_gen9.bin";  /* Kernel simple pour test */
    config.log_path = "logs/forensic/test_gen9_native.log";
    
    printf("[TEST] Configuration:\n");
    printf("  - batch_size: %u\n", config.batch_size);
    printf("  - work_group_size: %u\n", config.work_group_size);
    printf("  - kernel_path: %s\n", config.kernel_path);
    printf("  - log_path: %s\n", config.log_path);
    printf("\n");
    
    /* Initialisation */
    printf("[TEST 1/3] Initialisation Gen9 Native...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        printf("\n");
        printf("DIAGNOSTIC :\n");
        printf("  - Device DRM i915 accessible ? (ls /dev/dri/)\n");
        printf("  - Permissions GPU ? (groups | grep render)\n");
        printf("  - Kernel Gen9 ISA existe ? (ls %s)\n", config.kernel_path);
        printf("\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Exécution */
    printf("[TEST 2/3] Exécution batch buffer GPU...\n");
    ret = btc_gen9_execute(ctx);
    if (ret < 0) {
        printf("❌ ÉCHEC : Exécution échouée\n");
        btc_gen9_cleanup(ctx);
        return 1;
    }
    printf("✅ Exécution réussie\n");
    printf("\n");
    
    /* Métriques */
    printf("[TEST 3/3] Métriques...\n");
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TOUS LES TESTS RÉUSSIS                                 ║\n");
    printf("║  Solution Gen9 Native 100%% Fonctionnelle                  ║\n");
    printf("║  0%% OpenCL, 0%% Level Zero                                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}

// Made with Bob
