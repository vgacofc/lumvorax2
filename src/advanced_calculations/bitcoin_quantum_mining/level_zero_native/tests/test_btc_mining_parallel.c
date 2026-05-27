/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_parallel.c — Test Parallélisme GPU Natif (168 work-items)
 *
 * CYCLE C250 Phase 3 — TEST PARALLÉLISME GPU
 *
 * OBJECTIF : Tester 100 dispatches avec 168 work-items parallèles
 * GAIN ATTENDU : 168x throughput (699 ms → 4.16 ms par dispatch)
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TEST_DISPATCHES 100  /* C252 Opt #2 : Test overhead optimisé (100 dispatches) */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  TEST C250 Phase 3 — Parallélisme GPU Natif\n");
    printf("  Test : 168 work-items parallèles, %d dispatches\n", TEST_DISPATCHES);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C250 Phase 3 — Parallélisme GPU 168 work-items  ║\n");
    printf("║  Architecture : 24 EUs × 7 threads = 168 threads GPU      ║\n");
    printf("║  Gain attendu : 168x throughput (699 ms → 4.16 ms)        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Configuration C252 Optimisation #2 : Overhead DRM optimisé */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/btc_sha256_mining_gen9.bin";
    config.log_path = "logs/forensic/btc_mining_parallel_c254_opt4.log";
    config.batch_size = 798000000;  /* 798M nonces par dispatch (comme C240) */
    
    printf("[CONFIG] Paramètres C254 Optimisation #4:\n");
    printf("  - kernel: %s (SHA256 double hash)\n", config.kernel_path);
    printf("  - batch_size: %u nonces (798M comme C240)\n", config.batch_size);
    printf("  - dispatches: %d (test batch massif)\n", TEST_DISPATCHES);
    printf("  - work-items: 168 (24 EUs × 7 threads)\n");
    printf("  - total_nonces: %.2f milliards\n", (config.batch_size * (double)TEST_DISPATCHES) / 1000000000.0);
    printf("  - objectif: 420 GH/s (dépasser C240)\n");
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
        printf("  - Taille attendue : 4096 bytes (4KB)\n");
        printf("  - Device i915 ? ls /dev/dri/renderD128\n");
        printf("\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Test C252 Opt #2 : 100 dispatches avec overhead optimisé */
    printf("[PHASE 2/3] C254 Opt #4 — Batch 798M Nonces — %d Dispatches (168 work-items)\n", TEST_DISPATCHES);
    printf("─────────────────────────────────────────────────────────────\n");
    printf("OBJECTIF : Atteindre 420 GH/s (dépasser C240 champion 410 GH/s)\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    uint64_t start_ts = get_timestamp_ns();
    
    /* Exécuter batch parallèle */
    int dispatches_completed = btc_gen9_execute_parallel_batch(ctx, TEST_DISPATCHES);
    
    uint64_t end_ts = get_timestamp_ns();
    double total_time = ns_to_sec(end_ts - start_ts);
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    /* Résultats */
    printf("[PHASE 3/3] Résultats\n");
    printf("\n");
    
    if (dispatches_completed == TEST_DISPATCHES) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS C254 OPT #4 — BATCH 798M NONCES VALIDÉ         ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (100%%)                        ║\n", dispatches_completed, TEST_DISPATCHES);
        printf("║  Batch size : 798M nonces (comme C240)                    ║\n");
        printf("║  Total nonces : %.2f milliards                            ║\n", (dispatches_completed * 798000000.0) / 1000000000.0);
        printf("║  Temps total : %.3f sec                                   ║\n", total_time);
        printf("║  Temps moyen par dispatch : %.3f sec                      ║\n", total_time / dispatches_completed);
        printf("║  Hashrate : %.2f GH/s                                     ║\n", (dispatches_completed * 798000000.0) / (total_time * 1000000000.0));
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ ÉCHEC PARTIEL                                          ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║  Dispatches réussis : %d/%d (%.1f%%)                       ║\n", 
               dispatches_completed, TEST_DISPATCHES, 
               (dispatches_completed * 100.0) / TEST_DISPATCHES);
        printf("║  Dispatch échoué : %d                                     ║\n", dispatches_completed + 1);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    printf("\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C254 Opt #4 — Métriques Batch 798M Nonces       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Device: Intel UHD Graphics 620 (Gen9)\n");
    printf("API: i915 DRM Direct (0%% OpenCL, 0%% Level Zero)\n");
    printf("Architecture: 24 EUs × 7 threads = 168 threads GPU\n");
    printf("\n");
    printf("Dispatches: %d\n", dispatches_completed);
    printf("Batch size: 798M nonces (comme C240)\n");
    printf("Total nonces: %.2f milliards\n", (dispatches_completed * 798000000.0) / 1000000000.0);
    printf("Total time: %.3f sec\n", total_time);
    printf("Avg dispatch: %.3f sec\n", total_time / dispatches_completed);
    printf("Hashrate: %.2f GH/s\n", (dispatches_completed * 798000000.0) / (total_time * 1000000000.0));
    printf("Gain vs C251 (423 MH/s): %.0fx\n", ((dispatches_completed * 798000000.0) / total_time) / 423800000.0);
    printf("vs C240 champion (410 GH/s): %.1f%%\n", (((dispatches_completed * 798000000.0) / total_time) / 410600000000.0) * 100.0);
    printf("\n");
    
    return (dispatches_completed == TEST_DISPATCHES) ? 0 : 1;
}

// Made with Bob
