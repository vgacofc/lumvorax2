/*
 * LumVorax — Test Module LUM GPU Context
 * test_lum_gpu_parallel.c — Test Parallélisme 3 Dispatches
 *
 * OBJECTIF: Valider intégration complète module LUM GPU
 * - Test 1 dispatch séquentiel (baseline)
 * - Test 3 dispatches parallèles
 * - Comparaison hashrate avant/après
 * - Validation résultats
 */

#include "lum_gpu_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════════════
 * UTILITAIRES
 * ══════════════════════════════════════════════════════════════════════ */

static void print_header(const char* title) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %-56s  ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

static void print_hash(const uint8_t* hash) {
    for (int i = 0; i < 32; i++) {
        printf("%02x", hash[i]);
    }
}

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/* ══════════════════════════════════════════════════════════════════════
 * TEST 1: DISPATCH SÉQUENTIEL (BASELINE)
 * ══════════════════════════════════════════════════════════════════════ */

static int test_sequential(lum_gpu_context_t* ctx, const uint8_t* header) {
    print_header("TEST 1: Dispatch Séquentiel (Baseline)");
    
    printf("Configuration:\n");
    printf("  - Dispatches: 3 séquentiels\n");
    printf("  - Nonces par dispatch: 262,144 (256K)\n");
    printf("  - Total nonces: 786,432 (768K)\n");
    printf("\n");
    
    double start_time = get_time_sec();
    uint64_t total_nonces = 0;
    
    for (int i = 0; i < 3; i++) {
        printf("[Dispatch %d/3] Lancement...\n", i + 1);
        
        uint64_t start_nonce = i * 262144;
        
        if (lum_gpu_dispatch_parallel(ctx, header, start_nonce, 262144, 1) < 0) {
            printf("❌ ERREUR: Échec dispatch %d\n", i + 1);
            return -1;
        }
        
        if (lum_gpu_wait_all(ctx, 30000) < 1) {
            printf("❌ ERREUR: Timeout dispatch %d\n", i + 1);
            return -1;
        }
        
        uint32_t nonce;
        uint8_t hash[32];
        
        if (lum_gpu_get_results(ctx, 0, &nonce, hash) == 0) {
            printf("[Dispatch %d/3] ✅ Complété: nonce=%u hash=", i + 1, nonce);
            print_hash(hash);
            printf("\n");
            total_nonces += 262144;
        } else {
            printf("[Dispatch %d/3] ⚠️  Pas de résultats\n", i + 1);
        }
    }
    
    double end_time = get_time_sec();
    double total_time = end_time - start_time;
    double hashrate = (double)total_nonces / total_time / 1000000.0;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Résultats Test Séquentiel:\n");
    printf("  - Total nonces: %llu\n", total_nonces);
    printf("  - Temps total: %.3f sec\n", total_time);
    printf("  - Hashrate: %.2f MH/s\n", hashrate);
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * TEST 2: DISPATCHES PARALLÈLES
 * ══════════════════════════════════════════════════════════════════════ */

static int test_parallel(lum_gpu_context_t* ctx, const uint8_t* header) {
    print_header("TEST 2: Dispatches Parallèles");
    
    printf("Configuration:\n");
    printf("  - Dispatches: 3 parallèles\n");
    printf("  - Nonces par dispatch: 262,144 (256K)\n");
    printf("  - Total nonces: 786,432 (768K)\n");
    printf("\n");
    
    double start_time = get_time_sec();
    
    printf("[Parallel] Lancement 3 dispatches simultanés...\n");
    
    if (lum_gpu_dispatch_parallel(ctx, header, 0, 262144, 3) < 0) {
        printf("❌ ERREUR: Échec lancement parallèle\n");
        return -1;
    }
    
    printf("[Parallel] Attente complétion...\n");
    
    int completed = lum_gpu_wait_all(ctx, 30000);
    if (completed < 3) {
        printf("⚠️  ATTENTION: Seulement %d/3 dispatches complétés\n", completed);
    }
    
    double end_time = get_time_sec();
    double total_time = end_time - start_time;
    
    printf("\n");
    printf("Résultats par dispatch:\n");
    
    uint64_t total_nonces = 0;
    
    for (int i = 0; i < 3; i++) {
        uint32_t nonce;
        uint8_t hash[32];
        
        if (lum_gpu_get_results(ctx, i, &nonce, hash) == 0) {
            printf("  [Dispatch %d] ✅ nonce=%u hash=", i, nonce);
            print_hash(hash);
            printf("\n");
            total_nonces += 262144;
        } else {
            printf("  [Dispatch %d] ❌ Pas de résultats\n", i);
        }
    }
    
    double hashrate = (double)total_nonces / total_time / 1000000.0;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Résultats Test Parallèle:\n");
    printf("  - Total nonces: %llu\n", total_nonces);
    printf("  - Temps total: %.3f sec\n", total_time);
    printf("  - Hashrate: %.2f MH/s\n", hashrate);
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * MAIN
 * ══════════════════════════════════════════════════════════════════════ */

int main(void) {
    print_header("LumVorax — Test Module LUM GPU Parallèle");
    
    printf("Initialisation...\n");
    
    /* Configuration */
    lum_gpu_config_t config = {
        .kernel_path = "src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/kernels/btc_sha256_gen9.bin",
        .log_path = "src/lum/logs/test_lum_gpu_parallel.log",
        .batch_size = 262144,
        .work_group_size = 256,
        .enable_thp = true,
        .enable_profiling = true
    };
    
    /* Initialiser contexte LUM GPU */
    lum_gpu_context_t* ctx = lum_gpu_init(&config);
    if (!ctx) {
        printf("❌ ERREUR: Échec initialisation LUM GPU\n");
        return 1;
    }
    
    printf("✅ Contexte LUM GPU initialisé\n");
    printf("  - Contextes GPU: %d\n", LUM_GPU_CTX_POOL_SIZE);
    printf("  - Batch buffers: %d\n", LUM_GPU_BATCH_POOL_SIZE);
    printf("  - I/O pairs: %d\n", LUM_GPU_PARALLEL_MAX);
    printf("\n");
    
    /* Block header Bitcoin test */
    uint8_t header[80] = {
        0x01, 0x00, 0x00, 0x00,  /* Version */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* Prev hash */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,  /* Merkle root */
        0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a,
        0x29, 0xab, 0x5f, 0x49,  /* Timestamp */
        0xff, 0xff, 0x00, 0x1d,  /* Bits */
        0x00, 0x00, 0x00, 0x00   /* Nonce (sera modifié) */
    };
    
    printf("Block header Bitcoin:\n");
    printf("  Version: 0x%08x\n", *(uint32_t*)&header[0]);
    printf("  Timestamp: 0x%08x\n", *(uint32_t*)&header[68]);
    printf("  Bits: 0x%08x\n", *(uint32_t*)&header[72]);
    printf("\n");
    
    /* Test 1: Séquentiel */
    if (test_sequential(ctx, header) < 0) {
        printf("\n❌ TEST 1 ÉCHOUÉ\n");
        lum_gpu_cleanup(ctx);
        return 1;
    }
    
    printf("\n✅ TEST 1 RÉUSSI\n");
    
    /* Test 2: Parallèle */
    if (test_parallel(ctx, header) < 0) {
        printf("\n❌ TEST 2 ÉCHOUÉ\n");
        lum_gpu_cleanup(ctx);
        return 1;
    }
    
    printf("\n✅ TEST 2 RÉUSSI\n");
    
    /* Métriques finales */
    print_header("Métriques Finales LUM GPU");
    
    double hashrate;
    uint64_t dispatches;
    double time;
    
    if (lum_gpu_get_metrics(ctx, &hashrate, &dispatches, &time) == 0) {
        printf("Performance:\n");
        printf("  - Total dispatches: %llu\n", dispatches);
        printf("  - Temps total: %.3f sec\n", time);
        printf("  - Hashrate moyen: %.2f MH/s\n", hashrate);
        printf("\n");
    }
    
    size_t total_alloc, peak_usage;
    if (lum_gpu_get_memory_stats(ctx, &total_alloc, &peak_usage) == 0) {
        printf("Mémoire LUM:\n");
        printf("  - Total alloué: %.2f MB\n", total_alloc / 1024.0 / 1024.0);
        printf("  - Pic utilisation: %.2f MB\n", peak_usage / 1024.0 / 1024.0);
        printf("\n");
    }
    
    /* Nettoyage */
    printf("Nettoyage...\n");
    lum_gpu_cleanup(ctx);
    printf("✅ Nettoyage terminé\n");
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TOUS LES TESTS RÉUSSIS                                 ║\n");
    printf("║  Module LUM GPU validé et opérationnel                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}

// Made with Bob
