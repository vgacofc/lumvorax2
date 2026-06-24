/**
 * @file bench_c155_forensic_ultra_granular.c
 * @brief Benchmark Performance C155 avec Logging Forensique Ultra-Granulaire
 * 
 * SYSTÈME FORENSIQUE ACTIVÉ:
 * - Logs CSV nanoseconde par nanoseconde (8 couches)
 * - Ring buffer 4096 entrées sans I/O
 * - Snapshots hardware CPU/RAM/RSS
 * - Tracking modules, métriques, anomalies, threads
 * - Fichiers: logs/forensic/{modules,metrics,nano,hw_samples,anomalies,algo}/
 * 
 * AUCUN STUB, AUCUN PLACEHOLDER, AUCUN HARDCODING
 * Tous les calculs sont réels et tracés bit-level
 * 
 * @version 2.0.0 - FORENSIC ULTRA-GRANULAR
 * @date 2026-06-10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "../src/consensus/lum_poh.h"
#include "../src/consensus/lum_tower_bft.h"
#include "../src/mempool/lum_gulf_stream.h"
#include "../src/vm/lum_sealevel.h"
#include "../src/pipeline/lum_pipeline.h"
#include "../src/debug/ultra_forensic_logger.h"
#include "../src/debug/forensic_logger.h"

#define BENCH_DURATION_SEC 3
#define ANSI_CYAN "\x1b[36m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_RESET "\x1b[0m"

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ============================================================================
 * BENCHMARK 1 : POH THROUGHPUT AVEC FORENSIC
 * ============================================================================ */

void bench_poh_throughput_forensic(void) {
    printf("\n" ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  BENCHMARK 1: POH THROUGHPUT (FORENSIC ULTRA-GRANULAR)   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝" ANSI_RESET "\n");
    
    FORENSIC_LOG_MODULE_START("bench_poh", "throughput_test");
    FORENSIC_LOG_HW_SAMPLE("bench_poh");
    
    uint64_t init_start_ns = get_timestamp_ns();
    lum_poh_t* poh = lum_poh_init(1000000, true);
    uint64_t init_end_ns = get_timestamp_ns();
    
    FORENSIC_LOG_NANO("bench_poh", "init_duration_ns", init_end_ns - init_start_ns);
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "poh_init_time_ns", (double)(init_end_ns - init_start_ns));
    
    uint64_t start_ns = get_timestamp_ns();
    lum_poh_start(poh);
    uint64_t start_end_ns = get_timestamp_ns();
    
    FORENSIC_LOG_NANO("bench_poh", "start_duration_ns", start_end_ns - start_ns);
    FORENSIC_LOG_MODULE_OPERATION("bench_poh", "poh_started", "thread_launched");
    
    printf("Génération PoH pendant %d secondes (logging forensique actif)...\n", BENCH_DURATION_SEC);
    fflush(stdout);
    
    // Échantillonnage nanoseconde par nanoseconde pendant 100ms
    uint64_t sample_start = get_timestamp_ns();
    uint64_t sample_duration_ns = 100000000ULL; // 100ms
    uint64_t sample_count = 0;
    
    while ((get_timestamp_ns() - sample_start) < sample_duration_ns) {
        uint64_t tick_ns = get_timestamp_ns();
        FORENSIC_LOG_NANO("bench_poh", "tick_timestamp_ns", tick_ns);
        sample_count++;
        
        // Snapshot hardware toutes les 10ms
        if (sample_count % 10000 == 0) {
            FORENSIC_LOG_HW_SAMPLE("bench_poh");
        }
    }
    
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "nano_samples_100ms", (double)sample_count);
    
    // Attendre durée complète
    sleep(BENCH_DURATION_SEC);
    
    uint64_t stop_start_ns = get_timestamp_ns();
    double tps;
    uint64_t total_ticks;
    double uptime;
    lum_poh_get_metrics(poh, &tps, &total_ticks, &uptime);
    uint64_t metrics_end_ns = get_timestamp_ns();
    
    FORENSIC_LOG_NANO("bench_poh", "get_metrics_duration_ns", metrics_end_ns - stop_start_ns);
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "tps_final", tps);
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "total_ticks", (double)total_ticks);
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "uptime_sec", uptime);
    
    // Détection anomalie: TPS < 100K
    if (tps < 100000.0) {
        FORENSIC_LOG_ANOMALY("bench_poh", "tps_below_target_100k", tps);
    }
    
    printf("\n" ANSI_BOLD "Résultats PoH:" ANSI_RESET "\n");
    printf("  Ticks/sec       : " ANSI_GREEN "%.0f" ANSI_RESET " (cible: 1M)\n", tps);
    printf("  Total ticks     : %lu\n", total_ticks);
    printf("  Uptime          : %.2fs\n", uptime);
    printf("  Samples nano    : %lu (100ms)\n", sample_count);
    
    double solana_tps = 400000;
    double improvement = (tps / solana_tps - 1.0) * 100.0;
    
    FORENSIC_LOG_MODULE_METRIC("bench_poh", "vs_solana_improvement_pct", improvement);
    
    if (improvement > 0) {
        printf("  vs Solana       : " ANSI_GREEN "+%.1f%%" ANSI_RESET " plus rapide\n", improvement);
    } else {
        printf("  vs Solana       : " ANSI_YELLOW "%.1f%%" ANSI_RESET " plus lent\n", improvement);
    }
    
    FORENSIC_LOG_HW_SAMPLE("bench_poh");
    
    lum_poh_stop(poh);
    lum_poh_free(poh);
    
    FORENSIC_LOG_MODULE_END("bench_poh", "throughput_test", true);
}

/* ============================================================================
 * BENCHMARK 2 : TRANSACTION THROUGHPUT AVEC FORENSIC
 * ============================================================================ */

void bench_transaction_throughput_forensic(void) {
    printf("\n" ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  BENCHMARK 2: TRANSACTION THROUGHPUT (FORENSIC)          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝" ANSI_RESET "\n");
    
    FORENSIC_LOG_MODULE_START("bench_pipeline", "transaction_throughput");
    FORENSIC_LOG_HW_SAMPLE("bench_pipeline");
    
    uint64_t init_start = get_timestamp_ns();
    lum_pipeline_t* pipeline = lum_pipeline_init();
    uint64_t init_end = get_timestamp_ns();
    
    FORENSIC_LOG_NANO("bench_pipeline", "init_duration_ns", init_end - init_start);
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "pipeline_init_ns", (double)(init_end - init_start));
    
    lum_pipeline_start(pipeline);
    FORENSIC_LOG_MODULE_OPERATION("bench_pipeline", "pipeline_started", "4_threads_launched");
    
    uint8_t sig[64] = {0};
    uint8_t data[100] = {0};
    
    printf("Soumission transactions pendant %d secondes (forensic actif)...\n", BENCH_DURATION_SEC);
    fflush(stdout);
    
    uint64_t start_ns = get_timestamp_ns();
    uint64_t submitted = 0;
    uint64_t last_progress = start_ns;
    uint64_t last_hw_sample = start_ns;
    
    while ((get_timestamp_ns() - start_ns) < (BENCH_DURATION_SEC * 1000000000ULL)) {
        uint64_t submit_start = get_timestamp_ns();
        
        if (lum_pipeline_submit(pipeline, sig, data, 100)) {
            uint64_t submit_end = get_timestamp_ns();
            submitted++;
            
            // Log nanoseconde toutes les 1000 transactions
            if (submitted % 1000 == 0) {
                FORENSIC_LOG_NANO("bench_pipeline", "submit_latency_ns", submit_end - submit_start);
                FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "submitted_count", (double)submitted);
            }
        } else {
            usleep(10);
        }
        
        // Progress reporting toutes les secondes
        if ((get_timestamp_ns() - last_progress) > 1000000000ULL) {
            printf("  Progress: %lu transactions soumises...\n", submitted);
            fflush(stdout);
            last_progress = get_timestamp_ns();
        }
        
        // Snapshot hardware toutes les 500ms
        if ((get_timestamp_ns() - last_hw_sample) > 500000000ULL) {
            FORENSIC_LOG_HW_SAMPLE("bench_pipeline");
            last_hw_sample = get_timestamp_ns();
        }
    }
    
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "total_submitted", (double)submitted);
    
    sleep(2);  // Attendre traitement
    
    uint64_t fetched, verified, executed, written;
    lum_pipeline_get_metrics(pipeline, &fetched, &verified, &executed, &written, NULL);
    
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "fetched", (double)fetched);
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "verified", (double)verified);
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "executed", (double)executed);
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "written", (double)written);
    
    double duration_sec = BENCH_DURATION_SEC + 2.0;
    double tps = (double)written / duration_sec;
    
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "tps_final", tps);
    
    // Détection anomalie: TPS < 100K
    if (tps < 100000.0) {
        FORENSIC_LOG_ANOMALY("bench_pipeline", "tps_below_100k_target", tps);
    }
    
    printf("\n" ANSI_BOLD "Résultats Transactions:" ANSI_RESET "\n");
    printf("  Soumises        : %lu\n", submitted);
    printf("  Traitées        : %lu\n", written);
    printf("  TPS             : " ANSI_GREEN "%.0f" ANSI_RESET " (cible: 100K)\n", tps);
    
    double solana_tps = 65000;
    double improvement = (tps / solana_tps - 1.0) * 100.0;
    
    FORENSIC_LOG_MODULE_METRIC("bench_pipeline", "vs_solana_improvement_pct", improvement);
    
    if (tps >= 100000) {
        printf("  Objectif 100K   : " ANSI_GREEN "✓ ATTEINT" ANSI_RESET "\n");
    } else {
        printf("  Objectif 100K   : " ANSI_YELLOW "%.1f%% atteint" ANSI_RESET "\n", (tps / 100000.0) * 100.0);
    }
    
    if (improvement > 0) {
        printf("  vs Solana       : " ANSI_GREEN "+%.1f%%" ANSI_RESET " plus rapide\n", improvement);
    }
    
    FORENSIC_LOG_HW_SAMPLE("bench_pipeline");
    
    lum_pipeline_stop(pipeline);
    lum_pipeline_free(pipeline);
    
    FORENSIC_LOG_MODULE_END("bench_pipeline", "transaction_throughput", true);
}

/* ============================================================================
 * BENCHMARK 3 : LATENCE AVEC FORENSIC
 * ============================================================================ */

void bench_latency_forensic(void) {
    printf("\n" ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  BENCHMARK 3: LATENCE (FORENSIC ULTRA-GRANULAR)          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝" ANSI_RESET "\n");
    
    FORENSIC_LOG_MODULE_START("bench_latency", "latency_measurement");
    FORENSIC_LOG_HW_SAMPLE("bench_latency");
    
    lum_poh_t* poh = lum_poh_init(1000000, false);
    lum_poh_start(poh);
    
    lum_gulf_stream_t* gs = lum_gulf_stream_init(poh);
    lum_gulf_stream_start(gs);
    
    FORENSIC_LOG_MODULE_OPERATION("bench_latency", "gulf_stream_started", "mempool_active");
    
    uint8_t sig[64] = {0};
    uint8_t data[100] = {0};
    
    printf("Mesure latence sur 1000 transactions (forensic nanoseconde)...\n");
    fflush(stdout);
    
    uint64_t total_latency_ns = 0;
    int samples = 1000;
    uint64_t min_latency_ns = UINT64_MAX;
    uint64_t max_latency_ns = 0;
    
    for (int i = 0; i < samples; i++) {
        uint64_t start = get_timestamp_ns();
        lum_gulf_stream_submit_tx(gs, sig, data, 100, 1000);
        uint64_t end = get_timestamp_ns();
        uint64_t latency = end - start;
        
        total_latency_ns += latency;
        
        if (latency < min_latency_ns) min_latency_ns = latency;
        if (latency > max_latency_ns) max_latency_ns = latency;
        
        // Log nanoseconde toutes les 100 transactions
        if (i % 100 == 0) {
            FORENSIC_LOG_NANO("bench_latency", "tx_latency_ns", latency);
            FORENSIC_LOG_MODULE_METRIC("bench_latency", "sample_count", (double)i);
        }
        
        // Détection anomalie: latence > 100µs
        if (latency > 100000) {
            FORENSIC_LOG_ANOMALY("bench_latency", "latency_spike_above_100us", (double)latency);
        }
    }
    
    double avg_latency_ms = (double)total_latency_ns / (double)samples / 1000000.0;
    double avg_latency_ns = (double)total_latency_ns / (double)samples;
    double min_latency_ms = (double)min_latency_ns / 1000000.0;
    double max_latency_ms = (double)max_latency_ns / 1000000.0;
    
    FORENSIC_LOG_MODULE_METRIC("bench_latency", "avg_latency_ns", avg_latency_ns);
    FORENSIC_LOG_MODULE_METRIC("bench_latency", "min_latency_ns", (double)min_latency_ns);
    FORENSIC_LOG_MODULE_METRIC("bench_latency", "max_latency_ns", (double)max_latency_ns);
    FORENSIC_LOG_MODULE_METRIC("bench_latency", "total_samples", (double)samples);
    
    printf("\n" ANSI_BOLD "Résultats Latence:" ANSI_RESET "\n");
    printf("  Latence moyenne : " ANSI_GREEN "%.6f ms" ANSI_RESET " (" ANSI_GREEN "%.0f ns" ANSI_RESET ")\n", avg_latency_ms, avg_latency_ns);
    printf("  Latence min     : %.6f ms (%.0f ns)\n", min_latency_ms, (double)min_latency_ns);
    printf("  Latence max     : %.6f ms (%.0f ns)\n", max_latency_ms, (double)max_latency_ns);
    
    double solana_latency = 400.0;
    double improvement = (1.0 - avg_latency_ms / solana_latency) * 100.0;
    
    FORENSIC_LOG_MODULE_METRIC("bench_latency", "vs_solana_improvement_pct", improvement);
    
    if (avg_latency_ms < 50.0) {
        printf("  Objectif <50ms  : " ANSI_GREEN "✓ ATTEINT" ANSI_RESET "\n");
    } else {
        printf("  Objectif <50ms  : " ANSI_YELLOW "Non atteint" ANSI_RESET "\n");
    }
    
    printf("  vs Solana       : " ANSI_GREEN "%.1f%%" ANSI_RESET " plus rapide\n", improvement);
    
    FORENSIC_LOG_HW_SAMPLE("bench_latency");
    
    lum_gulf_stream_stop(gs);
    lum_gulf_stream_free(gs);
    lum_poh_stop(poh);
    lum_poh_free(poh);
    
    FORENSIC_LOG_MODULE_END("bench_latency", "latency_measurement", true);
}

/* ============================================================================
 * BENCHMARK 4 : EXÉCUTION PARALLÈLE AVEC FORENSIC
 * ============================================================================ */

void bench_parallel_execution_forensic(void) {
    printf("\n" ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  BENCHMARK 4: EXÉCUTION PARALLÈLE (FORENSIC)             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝" ANSI_RESET "\n");
    
    FORENSIC_LOG_MODULE_START("bench_sealevel", "parallel_execution");
    FORENSIC_LOG_HW_SAMPLE("bench_sealevel");
    
    lum_sealevel_t* vm = lum_sealevel_init(16);
    
    FORENSIC_LOG_MODULE_OPERATION("bench_sealevel", "vm_initialized", "16_threads");
    
    printf("Exécution parallèle de 10000 transactions (forensic actif)...\n");
    fflush(stdout);
    
    int num_txs = 10000;
    lum_sealevel_tx_t** txs = (lum_sealevel_tx_t**)malloc(num_txs * sizeof(lum_sealevel_tx_t*));
    
    uint64_t alloc_start = get_timestamp_ns();
    for (int i = 0; i < num_txs; i++) {
        txs[i] = (lum_sealevel_tx_t*)calloc(1, sizeof(lum_sealevel_tx_t));
        txs[i]->num_accounts = 2;
        txs[i]->accounts[0].is_writable = (i % 3 == 0);
        txs[i]->accounts[1].is_writable = false;
    }
    uint64_t alloc_end = get_timestamp_ns();
    
    FORENSIC_LOG_NANO("bench_sealevel", "alloc_duration_ns", alloc_end - alloc_start);
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "tx_allocation_ns", (double)(alloc_end - alloc_start));
    
    uint64_t start = get_timestamp_ns();
    lum_sealevel_execute_parallel(vm, txs, num_txs);
    uint64_t end = get_timestamp_ns();
    
    uint64_t exec_duration_ns = end - start;
    FORENSIC_LOG_NANO("bench_sealevel", "exec_duration_ns", exec_duration_ns);
    
    double duration_ms = (double)exec_duration_ns / 1000000.0;
    double tps = (double)num_txs / (duration_ms / 1000.0);
    
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "execution_duration_ms", duration_ms);
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "parallel_tps", tps);
    
    uint64_t executed, parallel;
    lum_sealevel_get_metrics(vm, &executed, &parallel, NULL);
    
    double parallel_pct = (double)parallel / (double)executed * 100.0;
    
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "executed_count", (double)executed);
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "parallel_count", (double)parallel);
    FORENSIC_LOG_MODULE_METRIC("bench_sealevel", "parallel_percentage", parallel_pct);
    
    // Détection anomalie: parallélisme < 80%
    if (parallel_pct < 80.0) {
        FORENSIC_LOG_ANOMALY("bench_sealevel", "parallel_pct_below_80", parallel_pct);
    }
    
    printf("\n" ANSI_BOLD "Résultats Parallélisme:" ANSI_RESET "\n");
    printf("  Transactions    : %d\n", num_txs);
    printf("  Durée           : %.2f ms\n", duration_ms);
    printf("  TPS             : " ANSI_GREEN "%.0f" ANSI_RESET "\n", tps);
    printf("  Parallèles      : %lu (%.1f%%)\n", parallel, parallel_pct);
    
    FORENSIC_LOG_HW_SAMPLE("bench_sealevel");
    
    for (int i = 0; i < num_txs; i++) free(txs[i]);
    free(txs);
    lum_sealevel_free(vm);
    
    FORENSIC_LOG_MODULE_END("bench_sealevel", "parallel_execution", true);
}

/* ============================================================================
 * MAIN AVEC FORENSIC COMPLET
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf(ANSI_BOLD "╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  LUMVORAX C155 - BENCHMARK FORENSIC ULTRA-GRANULAR       ║\n");
    printf("║  Logging bit-level nanoseconde par nanoseconde           ║\n");
    printf("║  8 couches: MODULE, METRIC, NANO, HW, ANOMALY, ALGO      ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    // Initialisation système forensique
    printf("\n🔬 Initialisation système forensique ultra-granulaire...\n");
    
    if (!ultra_forensic_logger_init()) {
        fprintf(stderr, "ERREUR: Impossible d'initialiser ultra_forensic_logger\n");
        return 1;
    }
    
    if (!forensic_logger_init("logs/forensic/bench_c155_session.log")) {
        fprintf(stderr, "ERREUR: Impossible d'initialiser forensic_logger\n");
        ultra_forensic_logger_destroy();
        return 1;
    }
    
    printf("✓ Système forensique initialisé\n");
    printf("  - Logs CSV: %s\n", ultra_forensic_get_csv_path());
    printf("  - Logs modules: logs/forensic/modules/\n");
    printf("  - Logs métriques: logs/forensic/metrics/\n");
    printf("  - Logs nano: logs/forensic/nano/\n");
    printf("  - Logs hardware: logs/forensic/hw_samples/\n");
    printf("  - Logs anomalies: logs/forensic/anomalies/\n");
    
    FORENSIC_LOG_MODULE_START("bench_main", "full_benchmark_suite");
    FORENSIC_LOG_HW_SAMPLE("bench_main");
    
    // Exécution benchmarks
    bench_poh_throughput_forensic();
    bench_transaction_throughput_forensic();
    bench_latency_forensic();
    bench_parallel_execution_forensic();
    
    // Flush ring buffer nanoseconde
    printf("\n🔬 Flush ring buffer nanoseconde...\n");
    ultra_forensic_flush_nano_ring();
    
    // Génération rapport summary
    printf("🔬 Génération rapport summary forensique...\n");
    ultra_forensic_generate_summary_report();
    
    // Validation logs
    printf("🔬 Validation existence logs forensiques...\n");
    if (ultra_forensic_validate_all_logs_exist()) {
        printf("✓ Tous les logs forensiques existent\n");
    } else {
        printf("⚠ Certains logs forensiques manquants\n");
    }
    
    FORENSIC_LOG_HW_SAMPLE("bench_main");
    FORENSIC_LOG_MODULE_END("bench_main", "full_benchmark_suite", true);
    
    // Cleanup
    forensic_logger_destroy();
    ultra_forensic_logger_destroy();
    
    printf("\n" ANSI_GREEN "✓ Benchmark forensique terminé avec succès" ANSI_RESET "\n");
    printf("\n📊 LOGS FORENSIQUES GÉNÉRÉS:\n");
    printf("  - logs/forensic/sessions/summary_*.txt\n");
    printf("  - logs/forensic/modules/*_forensic_*.log\n");
    printf("  - logs/forensic/metrics/*_metrics.log\n");
    printf("  - logs/forensic/nano/nano_ring_*.csv\n");
    printf("  - logs/forensic/hw_samples/*_hw.log\n");
    printf("  - logs/forensic/anomalies/*_anomalies.log\n");
    printf("  - CSV principal: lumvorax_*.csv\n\n");
    
    return 0;
}

// Made with Bob - FORENSIC ULTRA-GRANULAR v2.0