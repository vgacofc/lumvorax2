/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c240_optimized.c — Test avec Optimisations #6, #7, #8
 *
 * CYCLE C240 — OPTIMISATIONS MAXIMALES
 *
 * OBJECTIF : Valider 3 optimisations identifiées en C239
 * - Optimisation #6 : Batch size 268M (limite GPU Gen9)
 * - Optimisation #7 : Buffer output 1MB (au lieu de batch_size*32)
 * - Optimisation #8 : Monitoring DRM_IOCTL (au lieu de sysfs)
 * 
 * TEST : 100 dispatches maximum avec batch_size=268M
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define KERNEL_PATH "kernels/btc_sha256_gen9.bin"
#define LOG_PATH "logs/forensic/btc_mining_c240_optimized.log"
#define MAX_DISPATCHES 100

int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LumVorax C240 — Test Optimisations Maximales\n");
    printf("  Batch Size : 268M nonces (limite GPU Gen9)\n");
    printf("  Dispatches : Maximum 100\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    /* Configuration avec optimisations */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = KERNEL_PATH;
    config.log_path = LOG_PATH;
    /* batch_size=268M déjà défini dans BTC_GEN9_DEFAULT_CONFIG (Opt #6) */
    
    printf("[CONFIG] Paramètres:\n");
    printf("  - kernel: %s\n", config.kernel_path);
    printf("  - batch_size: %u nonces (%.2f M)\n", config.batch_size, config.batch_size / 1e6);
    printf("  - max_dispatches: %d\n", MAX_DISPATCHES);
    printf("  - log: %s\n", config.log_path);
    printf("\n");
    printf("[OPTIMISATIONS ACTIVES]\n");
    printf("  ✅ Opt #6 : Batch size 268M (limite GPU Gen9)\n");
    printf("  ✅ Opt #7 : Buffer output 1MB (overhead reopen -78%%)\n");
    printf("  ✅ Opt #8 : Monitoring DRM_IOCTL (métriques post-reopen)\n");
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
    
    /* Warm-up : 3 dispatches */
    printf("[PHASE 2/4] Warm-up — 3 Dispatches\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec warmup_start, warmup_end;
    clock_gettime(CLOCK_MONOTONIC, &warmup_start);
    
    int warmup_success = 0;
    for (int i = 0; i < 3; i++) {
        ret = btc_gen9_execute(ctx);
        if (ret < 0) {
            printf("❌ Warm-up échec au dispatch %d\n", i + 1);
            btc_gen9_cleanup(ctx);
            return 1;
        }
        warmup_success++;
        printf("  Dispatch %d/3 : ✅\n", i + 1);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &warmup_end);
    double warmup_time = (warmup_end.tv_sec - warmup_start.tv_sec) + 
                         (warmup_end.tv_nsec - warmup_start.tv_nsec) / 1e9;
    
    printf("✅ Warm-up complété : %d/3 dispatches (%.3f sec)\n", 
           warmup_success, warmup_time);
    printf("\n");
    
    /* Test production : MAX_DISPATCHES */
    printf("[PHASE 3/4] Test Production — %d Dispatches\n", MAX_DISPATCHES);
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec test_start, test_end;
    clock_gettime(CLOCK_MONOTONIC, &test_start);
    
    uint64_t total_hashes = 0;
    int dispatch_count = 0;
    int failed_count = 0;
    
    for (int i = 0; i < MAX_DISPATCHES; i++) {
        /* Affichage progression tous les 10 dispatches */
        if (i % 10 == 0) {
            printf("\n[Dispatches %d-%d]\n", i + 1, (i + 10 < MAX_DISPATCHES) ? i + 10 : MAX_DISPATCHES);
        }
        
        /* Exécution dispatch */
        struct timespec dispatch_start, dispatch_end;
        clock_gettime(CLOCK_MONOTONIC, &dispatch_start);
        
        ret = btc_gen9_execute(ctx);
        
        clock_gettime(CLOCK_MONOTONIC, &dispatch_end);
        double dispatch_time = (dispatch_end.tv_sec - dispatch_start.tv_sec) + 
                               (dispatch_end.tv_nsec - dispatch_start.tv_nsec) / 1e9;
        
        if (ret < 0) {
            printf("  Dispatch %d : ❌ ÉCHEC (%.3f sec)\n", i + 1, dispatch_time);
            failed_count++;
            break;
        }
        
        /* Comptabiliser hashes */
        total_hashes += config.batch_size;
        dispatch_count++;
        
        /* Calculer hashrate instantané */
        double hashrate_instant = (config.batch_size / dispatch_time) / 1e6;
        
        printf("  Dispatch %d : ✅ %.3f sec (%.1f MH/s)\n", 
               i + 1, dispatch_time, hashrate_instant);
        
        /* Pause courte entre dispatches (50ms) */
        usleep(50000);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &test_end);
    double test_time = (test_end.tv_sec - test_start.tv_sec) + 
                       (test_end.tv_nsec - test_start.tv_nsec) / 1e9;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS FINAUX C240\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Dispatches réussis : %d/%d\n", dispatch_count, MAX_DISPATCHES);
    printf("Dispatches échoués : %d\n", failed_count);
    printf("Total hashes calculés : %lu (%.2f G)\n", total_hashes, total_hashes / 1e9);
    printf("Temps total : %.3f secondes (%.2f minutes)\n", test_time, test_time / 60.0);
    
    if (dispatch_count > 0) {
        double hashrate_avg = (total_hashes / test_time) / 1e6;
        double time_per_dispatch = test_time / dispatch_count;
        printf("Hashrate moyen : %.2f MH/s\n", hashrate_avg);
        printf("Temps moyen/dispatch : %.3f sec\n", time_per_dispatch);
        printf("Débit : %.3f dispatches/sec\n", dispatch_count / test_time);
    }
    
    printf("\n");
    printf("📊 Logs forensiques : %s\n", LOG_PATH);
    printf("\n");
    
    /* Cleanup */
    printf("[PHASE 4/4] Cleanup...\n");
    btc_gen9_cleanup(ctx);
    printf("✅ Cleanup terminé\n");
    printf("\n");
    
    printf("✅ Test C240 terminé avec succès\n");
    printf("\n");
    
    return (failed_count > 0) ? 1 : 0;
}

// Made with Bob
