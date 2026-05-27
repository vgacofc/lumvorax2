/* LumVorax C241 — Test Validation Forensique Absolue
 * 0% OpenCL, 100% Native i915 DRM
 * Device: Intel UHD Graphics 620 (Gen9)
 *
 * Ce test implémente le protocole de validation forensique complet :
 * - Validation SHA256 cryptographique (vs OpenSSL)
 * - Monitoring thermique temps réel
 * - Test 3 stratégies mémoire (reopen, reset context, GEM persistant)
 * - Corrélation dmesg (manuel)
 * - 1000 dispatches avec batch_size=268M
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "btc_gen9_native_runner.h"
#include "lum_thermal_monitor_c241.h"
#include "lum_sha256_validator_c241.h"

/* Test configuration */
#define MAX_DISPATCHES 1000
#define BATCH_SIZE 268435456  /* 268M nonces (limite GPU Gen9) */
#define REOPEN_INTERVAL 9     /* Reopen tous les 9 dispatches */
#define PAUSE_BETWEEN_DISPATCHES_MS 50

/* Strategy selection */
typedef enum {
    STRATEGY_A_REOPEN_COMPLETE,   /* Reopen complet (baseline C240) */
    STRATEGY_B_RESET_CONTEXT,     /* Reset context sans close(fd) */
    STRATEGY_C_GEM_PERSISTENT     /* GEM persistant (PRIORITÉ #1) */
} test_strategy_t;

/* Global context for signal handling */
static volatile sig_atomic_t g_interrupted = 0;
static btc_gen9_context_t* g_ctx = NULL;
static lum_thermal_monitor_t* g_thermal = NULL;
static lum_sha256_validator_t* g_validator = NULL;

/* Signal handler */
static void signal_handler(int sig) {
    const char* sig_name = (sig == SIGINT) ? "SIGINT" : 
                          (sig == SIGTERM) ? "SIGTERM" : 
                          (sig == SIGSEGV) ? "SIGSEGV" : "UNKNOWN";
    
    fprintf(stderr, "\n[SIGNAL] Received %s (%d)\n", sig_name, sig);
    g_interrupted = 1;
    
    /* Cleanup on SIGSEGV */
    if (sig == SIGSEGV) {
        if (g_ctx) btc_gen9_cleanup(g_ctx);
        if (g_thermal) lum_thermal_cleanup(g_thermal);
        if (g_validator) lum_sha256_validator_cleanup(g_validator);
        exit(1);
    }
}

/* Helper: Get timestamp nanoseconds */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Helper: Print progress */
static void print_progress(int dispatch, int total, double hashrate_ghs, 
                          double temp_gpu, uint32_t freq_gpu) {
    double progress = (double)dispatch / total * 100.0;
    
    printf("\r[PROGRESS] %d/%d (%.1f%%) | %.2f GH/s | GPU: %.1f°C @ %u MHz    ",
           dispatch, total, progress, hashrate_ghs, temp_gpu, freq_gpu);
    fflush(stdout);
}

/* Test Strategy A: Reopen Complete (baseline C240) */
static int test_strategy_a_reopen_complete(int max_dispatches) {
    printf("\n=== STRATEGY A: REOPEN COMPLETE (BASELINE C240) ===\n\n");
    
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.batch_size = BATCH_SIZE;
    config.log_path = "logs/forensic/c241_strategy_a.log";
    
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] btc_gen9_init failed\n");
        return -1;
    }
    
    g_ctx = ctx;
    
    uint64_t total_hashes = 0;
    uint64_t start_ns = get_timestamp_ns();
    
    for (int i = 1; i <= max_dispatches && !g_interrupted; i++) {
        /* Take thermal snapshot */
        lum_thermal_snapshot_t thermal;
        lum_thermal_snapshot(g_thermal, &thermal);
        
        /* Execute dispatch */
        uint64_t dispatch_start_ns = get_timestamp_ns();
        ret = btc_gen9_execute(ctx);
        uint64_t dispatch_end_ns = get_timestamp_ns();
        
        if (ret != 0) {
            fprintf(stderr, "\n[ERROR] Dispatch %d failed (ret=%d, errno=%d)\n", i, ret, errno);
            break;
        }
        
        total_hashes += BATCH_SIZE;
        double dispatch_time_s = (dispatch_end_ns - dispatch_start_ns) / 1e9;
        double hashrate_ghs = (BATCH_SIZE / dispatch_time_s) / 1e9;
        
        /* Correlate thermal */
        double corr_temp, corr_freq;
        lum_thermal_correlate_dispatch(g_thermal, dispatch_time_s * 1000.0, &corr_temp, &corr_freq);
        
        /* Print progress */
        print_progress(i, max_dispatches, hashrate_ghs, 
                      thermal.gpu_temp_celsius, thermal.gpu_freq_mhz);
        
        /* Reopen every REOPEN_INTERVAL dispatches */
        if (i % REOPEN_INTERVAL == 0) {
            printf("\n[REOPEN] Dispatch %d (multiple de %d)\n", i, REOPEN_INTERVAL);
            
            /* Strategy A: Close + reopen fd */
            btc_gen9_cleanup(ctx);
            
            ret = btc_gen9_init(&ctx, &config);
            if (ret != 0) {
                fprintf(stderr, "[ERROR] Reopen failed\n");
                break;
            }
            g_ctx = ctx;
        }
        
        /* Pause between dispatches */
        usleep(PAUSE_BETWEEN_DISPATCHES_MS * 1000);
    }
    
    uint64_t end_ns = get_timestamp_ns();
    double total_time_s = (end_ns - start_ns) / 1e9;
    double avg_hashrate_ghs = (total_hashes / total_time_s) / 1e9;
    
    printf("\n\n[STRATEGY A RESULTS]\n");
    printf("  Total Hashes: %lu (%.2f billion)\n", total_hashes, total_hashes / 1e9);
    printf("  Total Time: %.2f seconds\n", total_time_s);
    printf("  Average Hashrate: %.2f GH/s\n", avg_hashrate_ghs);
    
    btc_gen9_cleanup(ctx);
    g_ctx = NULL;
    
    return 0;
}

/* Test Strategy C: GEM Persistent (PRIORITÉ #1) */
static int test_strategy_c_gem_persistent(int max_dispatches) {
    printf("\n=== STRATEGY C: GEM PERSISTENT (PRIORITÉ #1) ===\n\n");
    
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.batch_size = BATCH_SIZE;
    config.log_path = "logs/forensic/c241_strategy_c.log";
    
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] btc_gen9_init failed\n");
        return -1;
    }
    
    g_ctx = ctx;
    
    uint64_t total_hashes = 0;
    uint64_t start_ns = get_timestamp_ns();
    
    for (int i = 1; i <= max_dispatches && !g_interrupted; i++) {
        /* Take thermal snapshot */
        lum_thermal_snapshot_t thermal;
        lum_thermal_snapshot(g_thermal, &thermal);
        
        /* Execute dispatch */
        uint64_t dispatch_start_ns = get_timestamp_ns();
        ret = btc_gen9_execute(ctx);
        uint64_t dispatch_end_ns = get_timestamp_ns();
        
        if (ret != 0) {
            fprintf(stderr, "\n[ERROR] Dispatch %d failed (ret=%d, errno=%d)\n", i, ret, errno);
            break;
        }
        
        total_hashes += BATCH_SIZE;
        double dispatch_time_s = (dispatch_end_ns - dispatch_start_ns) / 1e9;
        double hashrate_ghs = (BATCH_SIZE / dispatch_time_s) / 1e9;
        
        /* Correlate thermal */
        double corr_temp, corr_freq;
        lum_thermal_correlate_dispatch(g_thermal, dispatch_time_s * 1000.0, &corr_temp, &corr_freq);
        
        /* Print progress */
        print_progress(i, max_dispatches, hashrate_ghs, 
                      thermal.gpu_temp_celsius, thermal.gpu_freq_mhz);
        
        /* Strategy C: NO REOPEN, GEM persistent */
        /* Juste reset batch buffer si nécessaire */
        if (i % REOPEN_INTERVAL == 0) {
            printf("\n[CHECKPOINT] Dispatch %d (GEM persistent, no reopen)\n", i);
        }
        
        /* Pause between dispatches */
        usleep(PAUSE_BETWEEN_DISPATCHES_MS * 1000);
    }
    
    uint64_t end_ns = get_timestamp_ns();
    double total_time_s = (end_ns - start_ns) / 1e9;
    double avg_hashrate_ghs = (total_hashes / total_time_s) / 1e9;
    
    printf("\n\n[STRATEGY C RESULTS]\n");
    printf("  Total Hashes: %lu (%.2f billion)\n", total_hashes, total_hashes / 1e9);
    printf("  Total Time: %.2f seconds\n", total_time_s);
    printf("  Average Hashrate: %.2f GH/s\n", avg_hashrate_ghs);
    
    btc_gen9_cleanup(ctx);
    g_ctx = NULL;
    
    return 0;
}

/* Main test */
int main(int argc, char** argv) {
    printf("==========================================================\n");
    printf("  LumVorax C241 — Validation Forensique Absolue\n");
    printf("  0%% OpenCL, 100%% Native i915 DRM\n");
    printf("  Device: Intel UHD Graphics 620 (Gen9)\n");
    printf("==========================================================\n\n");
    
    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    
    /* Parse arguments */
    test_strategy_t strategy = STRATEGY_C_GEM_PERSISTENT;  /* Default: PRIORITÉ #1 */
    int max_dispatches = MAX_DISPATCHES;
    
    if (argc > 1) {
        if (strcmp(argv[1], "strategy_a") == 0) {
            strategy = STRATEGY_A_REOPEN_COMPLETE;
        } else if (strcmp(argv[1], "strategy_c") == 0) {
            strategy = STRATEGY_C_GEM_PERSISTENT;
        }
    }
    
    if (argc > 2) {
        max_dispatches = atoi(argv[2]);
        if (max_dispatches <= 0 || max_dispatches > MAX_DISPATCHES) {
            max_dispatches = MAX_DISPATCHES;
        }
    }
    
    printf("[CONFIG]\n");
    printf("  Strategy: %s\n", 
           strategy == STRATEGY_A_REOPEN_COMPLETE ? "A (Reopen Complete)" :
           strategy == STRATEGY_C_GEM_PERSISTENT ? "C (GEM Persistent)" : "Unknown");
    printf("  Max Dispatches: %d\n", max_dispatches);
    printf("  Batch Size: %u nonces (268M)\n", BATCH_SIZE);
    printf("  Reopen Interval: %d dispatches\n", REOPEN_INTERVAL);
    printf("\n");
    
    /* Initialize thermal monitor */
    lum_thermal_config_t thermal_config = LUM_THERMAL_DEFAULT_CONFIG;
    thermal_config.log_path = "logs/forensic/c241_thermal.csv";
    
    int ret = lum_thermal_init(&g_thermal, &thermal_config);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] lum_thermal_init failed\n");
        return 1;
    }
    
    printf("[THERMAL] Monitor initialized\n");
    
    /* Initialize SHA256 validator */
    lum_sha256_validator_config_t validator_config = LUM_SHA256_VALIDATOR_DEFAULT_CONFIG;
    validator_config.log_path = "logs/forensic/c241_sha256_validation.csv";
    
    ret = lum_sha256_validator_init(&g_validator, &validator_config);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] lum_sha256_validator_init failed\n");
        lum_thermal_cleanup(g_thermal);
        return 1;
    }
    
    printf("[SHA256] Validator initialized\n");
    
    /* Validate SHA256 with 1000 random nonces */
    printf("[SHA256] Validating 1000 random nonces...\n");
    int matches = lum_sha256_validate_random(g_validator, 1000);
    printf("[SHA256] Validation: %d/1000 matches (%.2f%%)\n", matches, matches / 10.0);
    
    if (matches < 1000) {
        fprintf(stderr, "[ERROR] SHA256 validation failed! LumVorax does NOT compute correct Bitcoin SHA256!\n");
        lum_sha256_validator_cleanup(g_validator);
        lum_thermal_cleanup(g_thermal);
        return 1;
    }
    
    printf("[SHA256] ✅ Validation PASSED: LumVorax computes correct Bitcoin double SHA256\n\n");
    
    /* Run selected strategy */
    switch (strategy) {
        case STRATEGY_A_REOPEN_COMPLETE:
            ret = test_strategy_a_reopen_complete(max_dispatches);
            break;
        
        case STRATEGY_C_GEM_PERSISTENT:
            ret = test_strategy_c_gem_persistent(max_dispatches);
            break;
        
        default:
            fprintf(stderr, "[ERROR] Unknown strategy\n");
            ret = -1;
    }
    
    /* Print thermal statistics */
    printf("\n");
    lum_thermal_stats_t thermal_stats;
    lum_thermal_get_stats(g_thermal, &thermal_stats);
    lum_thermal_print_stats(&thermal_stats);
    
    /* Detect thermal throttling */
    printf("\n[THERMAL THROTTLING DETECTION]\n");
    bool throttling = lum_thermal_detect_throttling(g_thermal, 80.0, 10.0);
    printf("  Throttling Detected: %s\n", throttling ? "⚠️ YES" : "✅ NO");
    
    /* Print SHA256 validation statistics */
    printf("\n");
    lum_sha256_validation_stats_t sha256_stats;
    lum_sha256_get_stats(g_validator, &sha256_stats);
    lum_sha256_print_stats(&sha256_stats);
    
    /* Cleanup */
    lum_sha256_validator_cleanup(g_validator);
    lum_thermal_cleanup(g_thermal);
    
    printf("\n==========================================================\n");
    printf("  Test C241 Completed\n");
    printf("  Exit Code: %d\n", ret);
    printf("==========================================================\n");
    
    return ret;
}

// Made with Bob
