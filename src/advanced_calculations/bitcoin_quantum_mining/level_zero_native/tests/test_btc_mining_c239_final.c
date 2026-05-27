/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c239_final.c — Test Augmentation Progressive Batch Size
 *
 * CYCLE C239 — MAXIMISATION HASHRATE DYNAMIQUE
 *
 * OBJECTIF : Augmenter progressivement batch_size à chaque dispatch
 * - Départ : 262144 (256K)
 * - Progression : x2 à chaque dispatch
 * - Maximum : Limite GPU ou 100 dispatches
 * - Objectif : Trouver batch_size optimal pour hashrate maximal
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define KERNEL_PATH "kernels/btc_sha256_gen9.bin"
#define LOG_PATH "logs/forensic/btc_mining_c239_final.log"
#define INITIAL_BATCH_SIZE 262144    /* 256K nonces */
#define MAX_DISPATCHES 100

int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LumVorax C239 — Test Augmentation Progressive Batch Size\n");
    printf("  Objectif : Maximisation Hashrate Dynamique\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    /* Configuration */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = KERNEL_PATH;
    config.log_path = LOG_PATH;
    config.batch_size = INITIAL_BATCH_SIZE;
    
    printf("[CONFIG] Paramètres:\n");
    printf("  - kernel: %s\n", config.kernel_path);
    printf("  - batch_size initial: %u nonces\n", config.batch_size);
    printf("  - max_dispatches: %d\n", MAX_DISPATCHES);
    printf("  - log: %s\n", config.log_path);
    printf("\n");
    
    /* Initialisation contexte */
    printf("[PHASE 1/3] Initialisation Gen9 Native...\n");
    
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Warm-up : 5 dispatches */
    printf("[PHASE 2/3] Warm-up — 5 Dispatches Standard\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec warmup_start, warmup_end;
    clock_gettime(CLOCK_MONOTONIC, &warmup_start);
    
    int warmup_success = 0;
    for (int i = 0; i < 5; i++) {
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
    
    printf("✅ Warm-up complété : %d/5 dispatches (%.3f sec)\n", 
           warmup_success, warmup_time);
    printf("\n");
    
    /* Test augmentation progressive */
    printf("[PHASE 3/3] Test Augmentation Progressive — Max %d Dispatches\n", MAX_DISPATCHES);
    printf("─────────────────────────────────────────────────────────────\n");
    
    struct timespec test_start, test_end;
    clock_gettime(CLOCK_MONOTONIC, &test_start);
    
    uint64_t total_hashes = 0;
    int dispatch_count = 0;
    uint32_t current_batch_size = INITIAL_BATCH_SIZE;
    
    for (int i = 0; i < MAX_DISPATCHES; i++) {
        printf("\n[Dispatch %d/%d] batch_size=%u (%.2f M nonces)\n", 
               i + 1, MAX_DISPATCHES, current_batch_size, current_batch_size / 1e6);
        
        /* Exécution dispatch */
        ret = btc_gen9_execute(ctx);
        if (ret < 0) {
            printf("  ❌ Dispatch échoué\n");
            printf("  Limite GPU atteinte : batch_size=%u\n", current_batch_size);
            break;
        }
        
        /* Comptabiliser hashes */
        total_hashes += current_batch_size;
        dispatch_count++;
        
        printf("  ✅ Dispatch réussi (total: %lu hashes)\n", total_hashes);
        
        /* Doubler batch_size pour prochain dispatch */
        current_batch_size *= 2;
        
        /* Vérifier limite GPU (256M nonces = limite empirique Gen9) */
        if (current_batch_size > 268435456) {  /* 256M */
            printf("\n⚠️  Limite GPU atteinte (256M nonces)\n");
            break;
        }
        
        /* Pause courte entre dispatches */
        usleep(100000);  /* 100ms */
    }
    
    clock_gettime(CLOCK_MONOTONIC, &test_end);
    double test_time = (test_end.tv_sec - test_start.tv_sec) + 
                       (test_end.tv_nsec - test_start.tv_nsec) / 1e9;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS FINAUX C239\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Dispatches réussis : %d/%d\n", dispatch_count, MAX_DISPATCHES);
    printf("Total hashes calculés : %lu (%.2f M)\n", total_hashes, total_hashes / 1e6);
    printf("Temps total : %.3f secondes\n", test_time);
    printf("Hashrate moyen : %.2f MH/s\n", (total_hashes / test_time) / 1e6);
    printf("\n");
    
    if (dispatch_count > 0) {
        uint32_t max_batch = INITIAL_BATCH_SIZE * (1 << (dispatch_count - 1));
        printf("🏆 Batch size maximum validé : %u (%.2f M nonces)\n", 
               max_batch, max_batch / 1e6);
    }
    
    printf("\n");
    printf("📊 Logs forensiques : %s\n", LOG_PATH);
    printf("\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    printf("✅ Test C239 terminé avec succès\n");
    printf("\n");
    
    return 0;
}

// Made with Bob
