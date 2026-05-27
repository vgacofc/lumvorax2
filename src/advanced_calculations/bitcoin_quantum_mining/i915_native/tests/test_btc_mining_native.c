/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_native.c — Test Kernel SHA256 Bitcoin Mining
 *
 * CYCLE C198 Phase 15T — TEST CRITIQUE Contexte Persistant
 *
 * OBJECTIF : Tester 1 contexte unique, AUCUN recycle, 1000 dispatches
 * HYPOTHÈSE : Le problème vient du recycle, pas du nombre de dispatches
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TEST_DISPATCHES 1000  /* TEST CRITIQUE : 1000 dispatches sans recycle */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TEST CRITIQUE C198 Phase 15T — Contexte Persistant\n");
    printf("  Test : 1 contexte unique, 0 recycle, %d dispatches\n", TEST_DISPATCHES);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15T — Test Contexte Persistant       ║\n");
    printf("║  Hypothèse : Échec vient du recycle, pas des dispatches   ║\n");
    printf("║  Test Stabilité : %d dispatches (1 contexte unique)       ║\n", TEST_DISPATCHES);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Configuration */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/btc_sha256_mining_gen9.bin";  /* NOUVEAU KERNEL SHA256 */
    config.log_path = "logs/forensic/btc_mining_native_sha256.log";
    config.batch_size = 262144;  /* 256K nonces par dispatch */
    
    printf("[CONFIG] Paramètres:\n");
    printf("  - kernel: %s (SHA256 double hash)\n", config.kernel_path);
    printf("  - batch_size: %u nonces\n", config.batch_size);
    printf("  - dispatches: %d (test stabilité)\n", TEST_DISPATCHES);
    printf("  - log: %s\n", config.log_path);
    printf("\n");
    
    /* Initialisation */
    printf("[PHASE 1/3] Initialisation Gen9 Native...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        printf("\n");
        printf("DIAGNOSTIC :\n");
        printf("  - Kernel existe ? ls -lh %s\n", config.kernel_path);
        printf("  - Taille attendue : 42672 bytes (42KB)\n");
        printf("  - Device i915 ? ls /dev/dri/renderD128\n");
        printf("\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Test stabilité : 20 dispatches */
    printf("[PHASE 2/3] Test Stabilité — %d Dispatches Consécutifs\n", TEST_DISPATCHES);
    printf("─────────────────────────────────────────────────────────────\n");
    
    int success_count = 0;
    int fail_dispatch = -1;
    
    for (int i = 0; i < TEST_DISPATCHES; i++) {
        printf("[Dispatch %2d/%d] Exécution... ", i + 1, TEST_DISPATCHES);
        fflush(stdout);
        
        ret = btc_gen9_execute(ctx);
        if (ret < 0) {
            printf("❌ ÉCHEC (errno=%d)\n", ret);
            fail_dispatch = i + 1;
            break;
        }
        
        success_count++;
        printf("✅ OK\n");
        
        /* Pause 10ms entre dispatches */
        usleep(10000);
    }
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    /* Résultats */
    printf("[PHASE 3/3] Résultats\n");
    printf("\n");
    
    if (success_count == TEST_DISPATCHES) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — KERNEL SHA256 STABLE                    ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (100%%)                        ║\n", success_count, TEST_DISPATCHES);
        printf("║  GPU hang : AUCUN                                         ║\n");
        printf("║  Kernel : btc_sha256_mining (42KB)                        ║\n");
        printf("║  Conclusion : PROBLÈME RÉSOLU                             ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  ÉCHEC PARTIEL — GPU HANG DÉTECTÉ                      ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (%.1f%%)                       ║\n", 
               success_count, TEST_DISPATCHES, 
               (float)success_count / TEST_DISPATCHES * 100.0f);
        printf("║  Échec au dispatch : %d                                   ║\n", fail_dispatch);
        printf("║  Pattern : %s                                             ║\n",
               fail_dispatch == 10 ? "Limite 9 dispatches (ancien bug)" : "Nouveau pattern");
        printf("║  Action : Analyser logs/forensic/btc_mining_native_sha256.log ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    /* Métriques détaillées */
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    return (success_count == TEST_DISPATCHES) ? 0 : 1;
}

// Made with Bob
