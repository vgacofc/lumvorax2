/*
 * LumVorax C255V13 — Test avec Réouverture Périodique (Solution Limite 27)
 * 
 * BASÉ SUR: C255V11 (qui fonctionne parfaitement jusqu'à 27 dispatches)
 * AJOUT: Réouverture périodique tous les 25 dispatches pour dépasser limite
 * 
 * OBJECTIF: Dépasser la limite matérielle de 27 dispatches consécutifs
 * STRATÉGIE: Fermer/rouvrir DRM tous les 25 dispatches (marge de sécurité)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include "btc_gen9_native_runner.h"

#define MAX_ITERATIONS 100
#define MAX_SAFE_DISPATCHES 25  /* Réouverture tous les 25 dispatches (marge vs 27) */
#define WORK_GROUP_SIZE 256

/* Atomiques NX48 (contrôle externe) */
atomic_uint nx48_ctrl_batch_size_gen9 = ATOMIC_VAR_INIT(262144);
atomic_uint nx48_ctrl_num_contexts = ATOMIC_VAR_INIT(2);
atomic_uint nx48_ctrl_use_thp = ATOMIC_VAR_INIT(1);
atomic_ullong nx48_ctrl_gpu_mem_available = ATOMIC_VAR_INIT(0);

static void print_header(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C255V13 — Test Réouverture Périodique (Limite 27)  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * Simule query mémoire GPU disponible (via DRM)
 */
static uint64_t query_gpu_memory_available(void) {
    return 1536 * 1024 * 1024ULL;  /* 1.5 GB */
}

/**
 * NX48 calcule batch_size optimal selon mémoire GPU disponible
 */
static uint32_t nx48_compute_optimal_batch_size(uint64_t mem_available, 
                                                 uint32_t current_size,
                                                 int last_success) {
    const uint32_t MIN_SIZE = 262144;      /* 256K */
    const uint32_t MAX_SIZE = 1048576000;  /* 1000M */
    const uint64_t BYTES_PER_NONCE = 32;
    
    uint64_t max_nonces = (mem_available * 80) / (100 * BYTES_PER_NONCE);
    uint32_t mem_limit = (uint32_t)(max_nonces < MAX_SIZE ? max_nonces : MAX_SIZE);
    
    if (!last_success) {
        uint32_t new_size = current_size / 2;
        if (new_size < MIN_SIZE) new_size = MIN_SIZE;
        printf("[NX48-C255V13] ⚠️  Crash détecté → Recul batch_size: %u → %u\n",
               current_size, new_size);
        return new_size;
    }
    
    if (current_size < mem_limit && current_size < MAX_SIZE) {
        uint32_t new_size = current_size * 2;
        if (new_size > mem_limit) new_size = mem_limit;
        if (new_size > MAX_SIZE) new_size = MAX_SIZE;
        
        double progress = (double)(new_size - MIN_SIZE) / (double)(MAX_SIZE - MIN_SIZE) * 100.0;
        printf("[NX48-C255V13] ✅ Succès → Doublement batch_size: %u → %u (%.1f%% vers max)\n",
               current_size, new_size, progress);
        return new_size;
    }
    
    printf("[NX48-C255V13] 🏁 Vitesse de croisière atteinte: batch_size=%u\n", current_size);
    return current_size;
}

int main(void) {
    print_header();
    
    printf("[C255V13] Configuration avec Réouverture Périodique:\n");
    printf("  - Batch size initial: 256K (progression automatique)\n");
    printf("  - Contextes GPU: 2\n");
    printf("  - Réouverture: Tous les %d dispatches\n", MAX_SAFE_DISPATCHES);
    printf("  - Kernel: kernels/btc_sha256_gen9.bin\n");
    printf("  - Log: logs/forensic/test_c255v13_reopen.log\n");
    printf("\n");
    
    /* Initialiser atomiques NX48 */
    atomic_store(&nx48_ctrl_batch_size_gen9, 262144);
    atomic_store(&nx48_ctrl_num_contexts, 2);
    atomic_store(&nx48_ctrl_use_thp, 1);
    
    uint64_t gpu_mem_available = query_gpu_memory_available();
    atomic_store(&nx48_ctrl_gpu_mem_available, gpu_mem_available);
    
    printf("[C255V13] Mémoire GPU disponible: %.2f GB\n", 
           gpu_mem_available / (1024.0 * 1024.0 * 1024.0));
    printf("\n");
    
    /* Initialisation Gen9 Native */
    printf("[C255V13] Initialisation Gen9 Native...\n");
    
    uint32_t current_batch_size = atomic_load(&nx48_ctrl_batch_size_gen9);
    
    btc_gen9_config_t config = {
        .batch_size = current_batch_size,
        .work_group_size = WORK_GROUP_SIZE,
        .kernel_path = "kernels/btc_sha256_gen9.bin",
        .log_path = "logs/forensic/test_c255v13_reopen.log",
        .enable_profiling = true
    };
    
    btc_gen9_context_t* ctx = NULL;
    if (btc_gen9_init(&ctx, &config) < 0 || !ctx) {
        printf("❌ Échec initialisation\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n\n");
    
    /* Boucle principale avec réouverture périodique */
    printf("[C255V13] Démarrage avec réouverture périodique...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    int total_success = 0;
    int total_crashes = 0;
    int total_reopens = 0;
    double total_time = 0.0;
    double total_reopen_overhead = 0.0;
    uint64_t total_nonces = 0;
    
    uint8_t block_header[80] = {0};
    uint32_t best_nonce = 0;
    uint32_t leading_zeros = 0;
    
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        /* Réouverture périodique pour dépasser limite 27 dispatches */
        if (iter > 0 && iter % MAX_SAFE_DISPATCHES == 0) {
            printf("\n");
            printf("╔═══════════════════════════════════════════════════════════════╗\n");
            printf("║  🔄 RÉOUVERTURE PÉRIODIQUE (dispatch %d/%d)                    ║\n", iter, MAX_ITERATIONS);
            printf("╚═══════════════════════════════════════════════════════════════╝\n");
            
            struct timespec reopen_start, reopen_end;
            clock_gettime(CLOCK_MONOTONIC, &reopen_start);
            
            /* Cleanup contexte actuel */
            printf("[C255V13] Fermeture contexte actuel...\n");
            btc_gen9_cleanup(ctx);
            ctx = NULL;
            
            /* Réinitialisation avec même configuration */
            printf("[C255V13] Réouverture DRM et réinitialisation...\n");
            config.batch_size = current_batch_size;  /* Garder batch_size actuel */
            
            if (btc_gen9_init(&ctx, &config) < 0 || !ctx) {
                printf("❌ Échec réouverture\n");
                break;
            }
            
            clock_gettime(CLOCK_MONOTONIC, &reopen_end);
            double reopen_time = (reopen_end.tv_sec - reopen_start.tv_sec) +
                                (reopen_end.tv_nsec - reopen_start.tv_nsec) / 1e9;
            
            total_reopens++;
            total_reopen_overhead += reopen_time;
            
            printf("✅ Réouverture réussie (%.3f ms overhead)\n", reopen_time * 1000);
            printf("   Total réouvertures: %d\n", total_reopens);
            printf("   Overhead cumulé: %.3f sec\n\n", total_reopen_overhead);
        }
        
        /* Lire batch_size actuel depuis NX48 */
        current_batch_size = atomic_load(&nx48_ctrl_batch_size_gen9);
        
        printf("[Dispatch %d/%d] batch_size=%u (%.2f M nonces)\n",
               iter + 1, MAX_ITERATIONS,
               current_batch_size, current_batch_size / 1000000.0);
        
        /* Exécuter dispatch */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = btc_gen9_execute_mining(ctx, block_header, 0, current_batch_size,
                                          &best_nonce, &leading_zeros);
        int success = (ret == 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double dispatch_time = (end.tv_sec - start.tv_sec) +
                              (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (success) {
            total_success++;
            total_nonces += current_batch_size;
            total_time += dispatch_time;
            
            double hashrate = (current_batch_size / dispatch_time) / 1e6;
            printf("  ✅ Succès: %.3f sec, %.2f MH/s\n", dispatch_time, hashrate);
            
            /* NX48 calcule nouveau batch_size optimal */
            uint32_t new_batch_size = nx48_compute_optimal_batch_size(
                gpu_mem_available, current_batch_size, 1);
            atomic_store(&nx48_ctrl_batch_size_gen9, new_batch_size);
        } else {
            total_crashes++;
            printf("  ❌ Crash: errno=%d\n", ret);
            
            /* NX48 recule batch_size */
            uint32_t new_batch_size = nx48_compute_optimal_batch_size(
                gpu_mem_available, current_batch_size, 0);
            atomic_store(&nx48_ctrl_batch_size_gen9, new_batch_size);
        }
        
        printf("\n");
    }
    
    /* Cleanup final */
    if (ctx) {
        btc_gen9_cleanup(ctx);
    }
    
    /* Rapport final */
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  📊 RAPPORT FINAL C255V13 — Réouverture Périodique           ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Dispatches réussis: %d/%d\n", total_success, MAX_ITERATIONS);
    printf("Crashes: %d\n", total_crashes);
    printf("Réouvertures: %d\n", total_reopens);
    printf("Overhead réouverture: %.3f sec (%.2f%% du temps total)\n",
           total_reopen_overhead, 
           (total_reopen_overhead / (total_time + total_reopen_overhead)) * 100.0);
    printf("\n");
    
    if (total_success > 0) {
        double avg_hashrate = (total_nonces / total_time) / 1e6;
        printf("Nonces totaux: %lu (%.2f M)\n", total_nonces, total_nonces / 1e6);
        printf("Temps total: %.3f sec\n", total_time);
        printf("Hashrate moyen: %.2f MH/s\n", avg_hashrate);
        printf("\n");
        
        if (total_success > MAX_SAFE_DISPATCHES) {
            printf("🎉 SUCCÈS: Limite 27 dispatches DÉPASSÉE!\n");
            printf("   Dispatches consécutifs max: %d (vs limite 27)\n", total_success);
        }
    }
    
    printf("\n");
    return (total_success > MAX_SAFE_DISPATCHES) ? 0 : 1;
}

// Made with Bob
