/* ═══════════════════════════════════════════════════════════════════════════
   TEST GPU NATIF SEUL C176
   ═══════════════════════════════════════════════════════════════════════════
   
   Test uniquement la version NATIVE (btc_lumvorax_gpu_native.c)
   pour valider les performances et le profiling bit-level
   
   CYCLE: C176
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

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
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */

int main(int argc, char** argv) {
    printf("\n");
    print_separator();
    printf("TEST GPU NATIF LUMVORAX C176\n");
    printf("Version: btc_lumvorax_gpu_native.c (events asynchrones)\n");
    print_separator();
    
    /* Paramètres */
    double duration_sec = 30.0;
    size_t batch_size = 262144;
    
    if (argc > 1) {
        duration_sec = atof(argv[1]);
    }
    if (argc > 2) {
        batch_size = atoi(argv[2]);
    }
    
    printf("\nParamètres:\n");
    printf("  Durée test      : %.1f sec\n", duration_sec);
    printf("  Batch size      : %zu nonces\n", batch_size);
    printf("\n");
    
    /* Configuration GPU */
    print_separator();
    printf("INITIALISATION GPU\n");
    print_separator();
    
    lum_gpu_config_t config = {
        .batch_size = batch_size,
        .workgroup_size = 64,
        .target_bits = 32,
        .enable_profiling = 1,
        .enable_bit_level_log = 1,
        .log_dir = "src/advanced_calculations/bitcoin_quantum_mining/logs"
    };
    
    printf("\n[C176-INIT] Configuration:\n");
    printf("  batch_size      : %zu\n", config.batch_size);
    printf("  workgroup_size  : %zu\n", config.workgroup_size);
    printf("  target_bits     : %u\n", config.target_bits);
    printf("  profiling       : %s\n", config.enable_profiling ? "ON" : "OFF");
    printf("  bit_level_log   : %s\n", config.enable_bit_level_log ? "ON" : "OFF");
    printf("  log_dir         : %s\n", config.log_dir);
    printf("\n");
    
    int ret = lum_gpu_init(&config);
    if (ret != LUM_GPU_OK) {
        printf("[C176-ERROR] Init failed: %d\n", ret);
        return 1;
    }
    printf("[C176-INIT] ✅ GPU initialized successfully\n\n");
    
    /* Données test */
    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t tail[4] = { 0x61736400, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
    
    /* Benchmark */
    print_separator();
    printf("BENCHMARK GPU (%.1f sec)\n", duration_sec);
    print_separator();
    printf("\n");
    
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
                printf("[C176-BENCH] Batch %3d: %.3f ms, %.3f MH/s, GPU: %.1f%%, CPU free: %.1f%%\n",
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
    
    /* Résultats finaux */
    printf("\n");
    print_separator();
    printf("RÉSULTATS FINAUX\n");
    print_separator();
    
    lum_gpu_stats_t stats;
    lum_gpu_get_stats(&stats);
    
    printf("\nPerformance Globale:\n");
    printf("  Durée totale    : %.3f sec\n", actual_sec);
    printf("  Batches traités : %d\n", batches);
    printf("  Total hashes    : %lu (%.2f M)\n", total_hashes, total_hashes / 1000000.0);
    printf("  Hashrate moyen  : %.3f MH/s\n", (total_hashes / actual_sec) / 1000000.0);
    printf("  Hashrate stats  : %.3f MH/s (avg), %.3f MH/s (peak)\n",
           stats.average_mhs, stats.peak_mhs);
    
    printf("\nTemps Batch:\n");
    printf("  Batch min       : %.3f ms\n", min_batch_ns / 1000000.0);
    printf("  Batch max       : %.3f ms\n", max_batch_ns / 1000000.0);
    printf("  Batch moyen     : %.3f ms\n", (total_batch_ns / batches) / 1000000.0);
    
    printf("\nProfiling GPU (dernier batch):\n");
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
    
    printf("\nAnalyse:\n");
    if (stats.current_mhs >= 10.0) {
        printf("  ✅ Performance EXCELLENTE (≥10 MH/s)\n");
    } else if (stats.current_mhs >= 1.0) {
        printf("  ⚠️  Performance ACCEPTABLE (≥1 MH/s)\n");
    } else {
        printf("  ❌ Performance FAIBLE (<1 MH/s)\n");
    }
    
    if (stats.cpu_free_pct >= 50.0) {
        printf("  ✅ CPU bien libéré (≥50%%)\n");
    } else {
        printf("  ⚠️  CPU partiellement bloqué (<50%%)\n");
    }
    
    print_separator();
    
    /* Dump état GPU */
    printf("\nSauvegarde état GPU...\n");
    lum_gpu_dump_state("src/advanced_calculations/bitcoin_quantum_mining/logs/gpu_state_c176.txt");
    printf("✅ État sauvegardé: logs/gpu_state_c176.txt\n");
    
    /* Cleanup */
    printf("\nCleanup GPU...\n");
    lum_gpu_cleanup();
    printf("✅ Cleanup terminé\n\n");
    
    print_separator();
    printf("LOGS DISPONIBLES\n");
    print_separator();
    printf("\n");
    printf("  1. logs/lum_gpu_native_c176.log  - Logs bit-level complets\n");
    printf("  2. logs/gpu_state_c176.txt       - État GPU final\n");
    printf("\n");
    print_separator();
    
    return 0;
}

// Made with Bob
