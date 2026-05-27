/* ═══════════════════════════════════════════════════════════════════════════
   TEST COMPARATIF GPU C176
   ═══════════════════════════════════════════════════════════════════════════
   
   Compare performance entre :
   - Version EXISTANTE (btc_opencl_runner.c avec clFinish bloquants)
   - Version NATIVE (btc_lumvorax_gpu_native.c avec events asynchrones)
   
   CYCLE: C176
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

/* Headers des 2 versions */
#include "../src/btc_opencl_runner.h"
#include "../src/btc_lumvorax_gpu_native.h"

/* ═══════════════════════════════════════════════════════════════════════════
   UTILITAIRES
   ═══════════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void print_separator(void) {
    printf("═══════════════════════════════════════════════════════════════════════════\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
   TEST VERSION EXISTANTE
   ═══════════════════════════════════════════════════════════════════════════ */

static void test_existing_version(double duration_sec, size_t batch_size) {
    print_separator();
    printf("TEST VERSION EXISTANTE (btc_opencl_runner.c)\n");
    print_separator();
    
    printf("\n[C176-TEST] Initialisation GPU existant...\n");
    int ret = btc_opencl_init(batch_size);
    if (ret != BTC_OCL_OK) {
        printf("[C176-ERROR] Init failed: %d\n", ret);
        return;
    }
    printf("[C176-TEST] Init OK\n");
    
    /* Données test */
    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t tail[4] = { 0x61736400, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
    
    printf("\n[C176-TEST] Lancement benchmark %.1f sec...\n", duration_sec);
    uint64_t start_ns = get_timestamp_ns();
    uint64_t end_ns = start_ns + (uint64_t)(duration_sec * 1000000000.0);
    uint64_t total_hashes = 0;
    uint32_t nonce = 0;
    int batches = 0;
    
    uint64_t min_batch_ns = UINT64_MAX;
    uint64_t max_batch_ns = 0;
    uint64_t total_batch_ns = 0;
    
    while (get_timestamp_ns() < end_ns) {
        uint32_t out_nonce = 0xFFFFFFFFu;
        uint32_t out_hash[8] = {0};
        uint32_t out_bits = 0;
        
        uint64_t batch_start = get_timestamp_ns();
        
        ret = btc_opencl_mine_batch(midstate, tail, nonce, batch_size, 32,
                                    &out_nonce, out_hash, &out_bits);
        
        uint64_t batch_end = get_timestamp_ns();
        uint64_t batch_ns = batch_end - batch_start;
        
        if (ret == BTC_OCL_OK) {
            total_hashes += batch_size;
            batches++;
            
            if (batch_ns < min_batch_ns) min_batch_ns = batch_ns;
            if (batch_ns > max_batch_ns) max_batch_ns = batch_ns;
            total_batch_ns += batch_ns;
            
            if (batches % 10 == 0) {
                double current_mhs = (batch_size / (batch_ns / 1000000000.0)) / 1000000.0;
                printf("[C176-TEST] Batch %d: %.3f ms, %.3f MH/s\n",
                       batches, batch_ns / 1000000.0, current_mhs);
            }
        } else {
            printf("[C176-ERROR] Batch failed: %d\n", ret);
            break;
        }
        
        nonce += batch_size;
    }
    
    uint64_t actual_ns = get_timestamp_ns() - start_ns;
    double actual_sec = actual_ns / 1000000000.0;
    
    printf("\n[C176-RESULTS] VERSION EXISTANTE\n");
    print_separator();
    printf("Durée totale      : %.3f sec\n", actual_sec);
    printf("Batches traités   : %d\n", batches);
    printf("Total hashes      : %lu\n", total_hashes);
    printf("Batch size        : %zu\n", batch_size);
    printf("\nPerformance:\n");
    printf("  Hashrate moyen  : %.3f MH/s\n", (total_hashes / actual_sec) / 1000000.0);
    printf("  Temps batch min : %.3f ms\n", min_batch_ns / 1000000.0);
    printf("  Temps batch max : %.3f ms\n", max_batch_ns / 1000000.0);
    printf("  Temps batch moy : %.3f ms\n", (total_batch_ns / batches) / 1000000.0);
    print_separator();
    
    btc_opencl_cleanup();
}

/* ═══════════════════════════════════════════════════════════════════════════
   TEST VERSION NATIVE
   ═══════════════════════════════════════════════════════════════════════════ */

static void test_native_version(double duration_sec, size_t batch_size) {
    print_separator();
    printf("TEST VERSION NATIVE (btc_lumvorax_gpu_native.c)\n");
    print_separator();
    
    printf("\n[C176-TEST] Initialisation GPU natif...\n");
    
    lum_gpu_config_t config = {
        .batch_size = batch_size,
        .workgroup_size = 64,
        .target_bits = 32,
        .enable_profiling = 1,
        .enable_bit_level_log = 1,
        .log_dir = "src/advanced_calculations/bitcoin_quantum_mining/logs"
    };
    
    int ret = lum_gpu_init(&config);
    if (ret != LUM_GPU_OK) {
        printf("[C176-ERROR] Init failed: %d\n", ret);
        return;
    }
    printf("[C176-TEST] Init OK\n");
    
    /* Données test */
    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t tail[4] = { 0x61736400, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
    
    printf("\n[C176-TEST] Lancement benchmark %.1f sec...\n", duration_sec);
    uint64_t start_ns = get_timestamp_ns();
    uint64_t end_ns = start_ns + (uint64_t)(duration_sec * 1000000000.0);
    uint64_t total_hashes = 0;
    uint32_t nonce = 0;
    int batches = 0;
    
    uint64_t min_batch_ns = UINT64_MAX;
    uint64_t max_batch_ns = 0;
    uint64_t total_batch_ns = 0;
    
    while (get_timestamp_ns() < end_ns) {
        uint32_t out_nonce = 0xFFFFFFFFu;
        uint32_t out_hash[8] = {0};
        uint32_t out_bits = 0;
        
        uint64_t batch_start = get_timestamp_ns();
        
        ret = lum_gpu_mine_batch_async(midstate, tail, nonce, batch_size, 32,
                                       &out_nonce, out_hash, &out_bits);
        if (ret != LUM_GPU_OK) {
            printf("[C176-ERROR] Async failed: %d\n", ret);
            break;
        }
        
        ret = lum_gpu_wait_batch();
        
        uint64_t batch_end = get_timestamp_ns();
        uint64_t batch_ns = batch_end - batch_start;
        
        if (ret == LUM_GPU_OK) {
            total_hashes += batch_size;
            batches++;
            
            if (batch_ns < min_batch_ns) min_batch_ns = batch_ns;
            if (batch_ns > max_batch_ns) max_batch_ns = batch_ns;
            total_batch_ns += batch_ns;
            
            if (batches % 10 == 0) {
                lum_gpu_stats_t stats;
                lum_gpu_get_stats(&stats);
                printf("[C176-TEST] Batch %d: %.3f ms, %.3f MH/s, GPU util: %.1f%%, CPU free: %.1f%%\n",
                       batches, batch_ns / 1000000.0, stats.current_mhs,
                       stats.gpu_utilization_pct, stats.cpu_free_pct);
            }
        } else {
            printf("[C176-ERROR] Wait failed: %d\n", ret);
            break;
        }
        
        nonce += batch_size;
    }
    
    uint64_t actual_ns = get_timestamp_ns() - start_ns;
    double actual_sec = actual_ns / 1000000000.0;
    
    /* Obtenir stats finales */
    lum_gpu_stats_t stats;
    lum_gpu_get_stats(&stats);
    
    printf("\n[C176-RESULTS] VERSION NATIVE\n");
    print_separator();
    printf("Durée totale      : %.3f sec\n", actual_sec);
    printf("Batches traités   : %d\n", batches);
    printf("Total hashes      : %lu\n", total_hashes);
    printf("Batch size        : %zu\n", batch_size);
    printf("\nPerformance:\n");
    printf("  Hashrate moyen  : %.3f MH/s\n", (total_hashes / actual_sec) / 1000000.0);
    printf("  Hashrate stats  : %.3f MH/s (avg), %.3f MH/s (peak)\n",
           stats.average_mhs, stats.peak_mhs);
    printf("  Temps batch min : %.3f ms\n", min_batch_ns / 1000000.0);
    printf("  Temps batch max : %.3f ms\n", max_batch_ns / 1000000.0);
    printf("  Temps batch moy : %.3f ms\n", (total_batch_ns / batches) / 1000000.0);
    printf("\nProfiling GPU:\n");
    printf("  Upload midstate : %.3f us\n", stats.upload_midstate_ns / 1000.0);
    printf("  Upload tail     : %.3f us\n", stats.upload_tail_ns / 1000.0);
    printf("  Upload sentinel : %.3f us\n", stats.upload_sentinel_ns / 1000.0);
    printf("  Kernel exec     : %.3f ms\n", stats.kernel_exec_ns / 1000000.0);
    printf("  Download nonce  : %.3f us\n", stats.download_result_ns / 1000.0);
    printf("  Download hash   : %.3f us\n", stats.download_hash_ns / 1000.0);
    printf("  Download leading: %.3f ms\n", stats.download_leading_ns / 1000000.0);
    printf("  Total GPU time  : %.3f ms\n", stats.total_gpu_time_ns / 1000000.0);
    printf("  Total CPU wait  : %.3f ms\n", stats.total_cpu_wait_ns / 1000000.0);
    printf("\nEfficacité:\n");
    printf("  GPU utilization : %.1f%%\n", stats.gpu_utilization_pct);
    printf("  CPU free        : %.1f%%\n", stats.cpu_free_pct);
    print_separator();
    
    /* Dump état GPU */
    lum_gpu_dump_state("src/advanced_calculations/bitcoin_quantum_mining/logs/gpu_state_c176.txt");
    
    lum_gpu_cleanup();
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */

int main(int argc, char** argv) {
    printf("\n");
    print_separator();
    printf("TEST COMPARATIF GPU C176\n");
    printf("Comparaison: Version EXISTANTE vs Version NATIVE\n");
    print_separator();
    
    /* Paramètres */
    double duration_sec = 30.0;  /* 30 sec par version */
    size_t batch_size = 262144;  /* 262k nonces */
    
    if (argc > 1) {
        duration_sec = atof(argv[1]);
    }
    if (argc > 2) {
        batch_size = atoi(argv[2]);
    }
    
    printf("\nParamètres:\n");
    printf("  Durée par test  : %.1f sec\n", duration_sec);
    printf("  Batch size      : %zu nonces\n", batch_size);
    printf("\n");
    
    /* Test 1: Version existante */
    printf("\n=== TEST 1/2 : VERSION EXISTANTE ===\n\n");
    test_existing_version(duration_sec, batch_size);
    
    printf("\n\n");
    
    /* Test 2: Version native */
    printf("\n=== TEST 2/2 : VERSION NATIVE ===\n\n");
    test_native_version(duration_sec, batch_size);
    
    /* Comparaison finale */
    printf("\n\n");
    print_separator();
    printf("COMPARAISON FINALE\n");
    print_separator();
    printf("\nLes résultats détaillés sont disponibles dans :\n");
    printf("  - logs/lum_gpu_native_c176.log (logs bit-level)\n");
    printf("  - logs/gpu_state_c176.txt (état GPU final)\n");
    printf("\nAnalyse recommandée :\n");
    printf("  1. Comparer hashrates (MH/s)\n");
    printf("  2. Comparer temps batch (ms)\n");
    printf("  3. Vérifier CPU free %% (version native)\n");
    printf("  4. Analyser profiling GPU (version native)\n");
    print_separator();
    
    return 0;
}

// Made with Bob
