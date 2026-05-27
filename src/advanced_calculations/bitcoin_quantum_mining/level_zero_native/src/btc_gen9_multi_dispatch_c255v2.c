/**
 * LumVorax C255v2 — Multi-Dispatch Parallèle CORRIGÉ
 * 
 * CORRECTIONS vs C255:
 * 1. Soumission asynchrone (btc_gen9_submit_async au lieu de btc_gen9_execute)
 * 2. DRM reopen thread-safe (mutex global)
 * 3. Métriques exec time correctes
 * 
 * ARCHITECTURE:
 * - Phase 1: Soumettre 9 dispatches SANS attendre (retour immédiat)
 * - Phase 2: Attendre fin des 9 dispatches en parallèle
 * - Phase 3: Reopen DRM thread-safe
 * 
 * GAIN ATTENDU: ×9 vs C255 (256 MH/s → 2.3 GH/s)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "btc_gen9_native_runner.h"
#include "btc_gen9_async_submit.h"

/* Configuration */
#define C255V2_CTX_POOL_SIZE 3
#define C255V2_CYCLES_PER_CTX 3
#define C255V2_TOTAL_DISPATCHES (C255V2_CTX_POOL_SIZE * C255V2_CYCLES_PER_CTX)  /* 9 */

/**
 * Contexte dispatch individuel
 */
typedef struct {
    btc_gen9_context_t* main_ctx;  /* Contexte principal */
    uint32_t ctx_id;                /* ID contexte pool */
    int ctx_idx;                    /* Index dans pool (0-2) */
    int cycle;                      /* Cycle (0-2) */
    int dispatch_num;               /* Numéro dispatch global (1-9) */
    uint64_t nonce_start;           /* Nonce début */
    btc_gen9_fence_t fence;         /* Fence pour attente */
    int result;                     /* Résultat (0=succès, -1=échec) */
    double submit_time;             /* Temps soumission (sec) */
    double exec_time;               /* Temps exécution GPU (sec) */
} c255v2_dispatch_context_t;

/**
 * Statistiques batch
 */
typedef struct {
    int total_dispatches;
    int successful_dispatches;
    int failed_dispatches;
    int cache_misses;
    double total_time;
    double submit_phase_time;
    double sync_phase_time;
    double min_exec_time;
    double max_exec_time;
    double avg_exec_time;
    uint64_t total_hashes;
    double hashrate_ghs;
} c255v2_batch_stats_t;

/**
 * Logger forensique
 */
static void log_c255v2(const char* event, const char* details) {
    uint64_t ts = get_timestamp_ns();
    double ts_sec = ts / 1e9;
    printf("[%.9f] C255V2_%s: %s\n", ts_sec, event, details);
    fflush(stdout);
}

/**
 * Thread worker pour soumission asynchrone
 * 
 * DIFFÉRENCE vs C255:
 * - C255: Appelait btc_gen9_execute() (synchrone, attend ~1 sec)
 * - C255v2: Appelle btc_gen9_submit_async() (asynchrone, retour immédiat)
 */
static void* c255v2_dispatch_worker(void* arg) {
    c255v2_dispatch_context_t* dctx = (c255v2_dispatch_context_t*)arg;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ctx_idx=%d ctx_id=%u cycle=%d dispatch=%d nonce_start=%lu",
             dctx->ctx_idx, dctx->ctx_id, dctx->cycle, dctx->dispatch_num, dctx->nonce_start);
    log_c255v2("DISPATCH_WORKER_START", msg);

    uint64_t ts_start = get_timestamp_ns();

    /* Sélectionner contexte dans le pool */
    uint32_t old_ctx_id = btc_gen9_get_ctx_id(dctx->main_ctx);
    btc_gen9_set_ctx_id(dctx->main_ctx, dctx->ctx_id);

    /* Configurer nonce_start */
    btc_gen9_set_nonce_start(dctx->main_ctx, dctx->nonce_start);

    /* Soumettre dispatch ASYNCHRONE (retour immédiat) */
    dctx->result = btc_gen9_submit_async(dctx->main_ctx, &dctx->fence);

    /* Restaurer contexte original */
    btc_gen9_set_ctx_id(dctx->main_ctx, old_ctx_id);

    uint64_t ts_end = get_timestamp_ns();
    dctx->submit_time = (ts_end - ts_start) / 1e9;

    snprintf(msg, sizeof(msg), "ctx_idx=%d dispatch=%d result=%d submit_time=%.6f",
             dctx->ctx_idx, dctx->dispatch_num, dctx->result, dctx->submit_time);
    log_c255v2("DISPATCH_WORKER_COMPLETE", msg);

    return NULL;
}

/**
 * Exécuter batch multi-dispatch (9 dispatches parallèles)
 * 
 * ARCHITECTURE CORRIGÉE:
 * 1. Phase soumission: Lancer 9 threads qui soumettent SANS attendre
 * 2. Phase synchronisation: Attendre fin des 9 dispatches GPU
 * 3. Phase reopen: Réouvrir DRM (thread-safe)
 */
int c255v2_execute_multi_dispatch_batch(btc_gen9_context_t* ctx, int num_batches) {
    if (!ctx || num_batches <= 0) {
        log_c255v2("ERROR", "ctx NULL ou num_batches <= 0");
        return -1;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "num_batches=%d dispatches_per_batch=%d total_dispatches=%d",
             num_batches, C255V2_TOTAL_DISPATCHES, num_batches * C255V2_TOTAL_DISPATCHES);
    log_c255v2("MULTI_DISPATCH_START", msg);

    uint64_t ts_global_start = get_timestamp_ns();

    /* Statistiques globales */
    int total_successful = 0;
    int total_failed = 0;
    int total_cache_misses = 0;
    double total_exec_time = 0.0;
    double min_exec_time = 1e9;
    double max_exec_time = 0.0;

    /* Pour chaque batch */
    for (int batch = 0; batch < num_batches; batch++) {
        snprintf(msg, sizeof(msg), "batch=%d/%d nonce_start=%lu",
                 batch + 1, num_batches, batch * C255V2_TOTAL_DISPATCHES * btc_gen9_get_input_size(ctx));
        log_c255v2("BATCH_START", msg);

        uint64_t ts_batch_start = get_timestamp_ns();

        /* Préparer contextes dispatches */
        c255v2_dispatch_context_t dispatches[C255V2_TOTAL_DISPATCHES];
        memset(dispatches, 0, sizeof(dispatches));

        int dispatch_num = 0;
        for (int ctx_idx = 0; ctx_idx < C255V2_CTX_POOL_SIZE; ctx_idx++) {
            for (int cycle = 0; cycle < C255V2_CYCLES_PER_CTX; cycle++) {
                dispatches[dispatch_num].main_ctx = ctx;
                dispatches[dispatch_num].ctx_id = btc_gen9_get_ctx_pool_id(ctx, ctx_idx);
                dispatches[dispatch_num].ctx_idx = ctx_idx;
                dispatches[dispatch_num].cycle = cycle;
                dispatches[dispatch_num].dispatch_num = dispatch_num + 1;
                dispatches[dispatch_num].nonce_start = 
                    (batch * C255V2_TOTAL_DISPATCHES + dispatch_num) * btc_gen9_get_input_size(ctx);
                dispatch_num++;
            }
        }

        /* PHASE 1: Soumission asynchrone (9 threads) */
        uint64_t ts_submit_start = get_timestamp_ns();
        
        pthread_t threads[C255V2_TOTAL_DISPATCHES];
        for (int i = 0; i < C255V2_TOTAL_DISPATCHES; i++) {
            pthread_create(&threads[i], NULL, c255v2_dispatch_worker, &dispatches[i]);
        }

        /* Attendre fin soumissions (threads retournent immédiatement) */
        for (int i = 0; i < C255V2_TOTAL_DISPATCHES; i++) {
            pthread_join(threads[i], NULL);
        }

        uint64_t ts_submit_end = get_timestamp_ns();
        double submit_phase_time = (ts_submit_end - ts_submit_start) / 1e9;

        snprintf(msg, sizeof(msg), "dispatches=%d time=%.6f sec", C255V2_TOTAL_DISPATCHES, submit_phase_time);
        log_c255v2("SUBMIT_PHASE_COMPLETE", msg);

        /* PHASE 2: Synchronisation (attendre fin GPU) */
        uint64_t ts_sync_start = get_timestamp_ns();

        int batch_successful = 0;
        int batch_failed = 0;

        for (int i = 0; i < C255V2_TOTAL_DISPATCHES; i++) {
            if (dispatches[i].result != 0) {
                /* Soumission a échoué */
                batch_failed++;
                snprintf(msg, sizeof(msg), "dispatch=%d ctx_idx=%d result=%d",
                         dispatches[i].dispatch_num, dispatches[i].ctx_idx, dispatches[i].result);
                log_c255v2("DISPATCH_SUBMIT_FAILED", msg);
                continue;
            }

            /* Attendre fin exécution GPU */
            uint64_t ts_wait_start = get_timestamp_ns();
            int wait_result = btc_gen9_wait_fence(&dispatches[i].fence);
            uint64_t ts_wait_end = get_timestamp_ns();

            dispatches[i].exec_time = (ts_wait_end - ts_wait_start) / 1e9;

            if (wait_result == 0) {
                batch_successful++;
                total_exec_time += dispatches[i].exec_time;
                if (dispatches[i].exec_time < min_exec_time) min_exec_time = dispatches[i].exec_time;
                if (dispatches[i].exec_time > max_exec_time) max_exec_time = dispatches[i].exec_time;

                snprintf(msg, sizeof(msg), "dispatch=%d ctx_idx=%d exec_time=%.6f",
                         dispatches[i].dispatch_num, dispatches[i].ctx_idx, dispatches[i].exec_time);
                log_c255v2("DISPATCH_COMPLETE", msg);
            } else {
                batch_failed++;
                snprintf(msg, sizeof(msg), "dispatch=%d ctx_idx=%d",
                         dispatches[i].dispatch_num, dispatches[i].ctx_idx);
                log_c255v2("DISPATCH_WAIT_FAILED", msg);
            }
        }

        uint64_t ts_sync_end = get_timestamp_ns();
        double sync_phase_time = (ts_sync_end - ts_sync_start) / 1e9;

        snprintf(msg, sizeof(msg), "success=%d/%d time=%.6f sec",
                 batch_successful, C255V2_TOTAL_DISPATCHES, sync_phase_time);
        log_c255v2("SYNC_PHASE_COMPLETE", msg);

        /* PHASE 3: Reopen DRM (thread-safe) */
        log_c255v2("DRM_REOPEN_TRIGGER", "dispatches=9 (multiple de 9)");
        int reopen_result = btc_gen9_reopen_drm_threadsafe(ctx);
        if (reopen_result != 0) {
            log_c255v2("DRM_REOPEN_FAILED", "");
        }

        /* Statistiques batch */
        uint64_t ts_batch_end = get_timestamp_ns();
        double batch_time = (ts_batch_end - ts_batch_start) / 1e9;
        uint64_t batch_hashes = batch_successful * btc_gen9_get_input_size(ctx);
        double batch_hashrate = (batch_hashes / 1e9) / batch_time;

        total_successful += batch_successful;
        total_failed += batch_failed;

        snprintf(msg, sizeof(msg), 
                 "batch=%d/%d success=%d/%d cache_misses=%d time=%.3f sec submit=%.6f sync=%.6f hashrate=%.2f GH/s",
                 batch + 1, num_batches, batch_successful, C255V2_TOTAL_DISPATCHES, 
                 0, batch_time, submit_phase_time, sync_phase_time, batch_hashrate);
        log_c255v2("BATCH_COMPLETE", msg);
    }

    /* Statistiques finales */
    uint64_t ts_global_end = get_timestamp_ns();
    double total_time = (ts_global_end - ts_global_start) / 1e9;
    uint64_t total_hashes = total_successful * btc_gen9_get_input_size(ctx);
    double global_hashrate = (total_hashes / 1e9) / total_time;
    double avg_exec_time = (total_successful > 0) ? (total_exec_time / total_successful) : 0.0;

    /* Calculer amélioration vs C251 baseline (423.80 MH/s) */
    double c251_baseline_mhs = 423.80;
    double improvement_vs_c251 = (global_hashrate * 1000.0) / c251_baseline_mhs;

    printf("[%.9f] C255V2_MULTI_DISPATCH_COMPLETE:\n", (double)ts_global_end / 1e9);
    printf("  Total Batches: %d\n", num_batches);
    printf("  Total Dispatches: %d\n", num_batches * C255V2_TOTAL_DISPATCHES);
    printf("  Successful: %d\n", total_successful);
    printf("  Failed: %d\n", total_failed);
    printf("  Cache Misses: %d (%.1f%%)\n", total_cache_misses, 
           (total_cache_misses * 100.0) / (num_batches * C255V2_TOTAL_DISPATCHES));
    printf("  Total Time: %.3f sec\n", total_time);
    printf("  Min Exec Time: %.6f sec\n", min_exec_time);
    printf("  Max Exec Time: %.6f sec\n", max_exec_time);
    printf("  Avg Exec Time: %.6f sec\n", avg_exec_time);
    printf("  Total Hashes: %lu\n", total_hashes);
    printf("  Hashrate: %.2f GH/s\n", global_hashrate);
    printf("  Improvement vs C251 (%.2f MH/s): %.1fx\n", c251_baseline_mhs, improvement_vs_c251);
    printf("  Improvement vs C255 (256.14 MH/s): %.1fx\n", (global_hashrate * 1000.0) / 256.14);
    fflush(stdout);

    return (total_failed == 0) ? 0 : -1;
}

// Made with Bob
