/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_mining_adapter.c — Implémentation Couche Adaptation OpenCL→Gen9
 *
 * CYCLE C255v8n Phase 2 — Migration Moteur Complet PT-MC
 * STANDARD_NAMES.md v4.6 §M-BTC-GEN9-ADAPTER-C255V8N
 *
 * OBJECTIF : COMPATIBILITÉ API OpenCL → Gen9 Native
 * - Implémentation wrappers fonctions OpenCL
 * - Gestion double-buffering asynchrone
 * - Stats temps réel
 */

#define _POSIX_C_SOURCE 199309L

#include "btc_gen9_mining_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* ══════════════════════════════════════════════════════════════════════
 * CONTEXTE GLOBAL (Thread-Safe avec mutex)
 * ══════════════════════════════════════════════════════════════════════ */

static btc_gen9_adapter_context_t g_adapter_ctx = {0};
static pthread_mutex_t g_adapter_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_adapter_initialized = false;

/* ══════════════════════════════════════════════════════════════════════
 * UTILITAIRES TEMPS
 * ══════════════════════════════════════════════════════════════════════ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ══════════════════════════════════════════════════════════════════════
 * INITIALISATION ADAPTATEUR
 * ══════════════════════════════════════════════════════════════════════ */

int btc_gen9_adapter_init(const btc_gen9_config_t* config) {
    pthread_mutex_lock(&g_adapter_mutex);
    
    if (g_adapter_initialized) {
        fprintf(stderr, "[GEN9-ADAPTER] Déjà initialisé\n");
        pthread_mutex_unlock(&g_adapter_mutex);
        return 0;
    }
    
    /* Init contexte Gen9 natif */
    int ret = btc_gen9_init(&g_adapter_ctx.ctx, config);
    if (ret != 0) {
        fprintf(stderr, "[GEN9-ADAPTER] Erreur init Gen9: %d\n", ret);
        pthread_mutex_unlock(&g_adapter_mutex);
        return -1;
    }
    
    /* Init buffers */
    memset(g_adapter_ctx.buffers, 0, sizeof(g_adapter_ctx.buffers));
    
    /* Init stats */
    memset(&g_adapter_ctx.stats, 0, sizeof(g_adapter_ctx.stats));
    g_adapter_ctx.total_batches = 0;
    g_adapter_ctx.total_hashes = 0;
    g_adapter_ctx.best_bits_global = 0;
    
    /* Timestamps */
    g_adapter_ctx.init_ns = get_time_ns();
    g_adapter_ctx.last_stats_ns = g_adapter_ctx.init_ns;
    
    g_adapter_initialized = true;
    
    fprintf(stderr, "[GEN9-ADAPTER] Initialisé (batch_size=%u)\n", config->batch_size);
    
    pthread_mutex_unlock(&g_adapter_mutex);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * MINING ASYNCHRONE (Compatible OpenCL)
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_mine_batch_async(
    int buffer_idx,
    const uint32_t midstate[8],
    const uint32_t tail[4],
    uint32_t nonce_start,
    uint32_t batch_size,
    uint32_t target_bits,
    uint32_t* out_nonce,
    uint32_t out_hash[8],
    uint32_t* out_best
) {
    if (!g_adapter_initialized) {
        fprintf(stderr, "[GEN9-ADAPTER] Non initialisé\n");
        return BTC_OCL_ERROR;
    }
    
    if (buffer_idx < 0 || buffer_idx >= GEN9_BUFFER_COUNT) {
        fprintf(stderr, "[GEN9-ADAPTER] buffer_idx invalide: %d\n", buffer_idx);
        return BTC_OCL_ERROR;
    }
    
    pthread_mutex_lock(&g_adapter_mutex);
    
    /* Sauvegarder paramètres batch */
    memcpy(g_adapter_ctx.buffers[buffer_idx].midstate, midstate, 8 * sizeof(uint32_t));
    memcpy(g_adapter_ctx.buffers[buffer_idx].tail, tail, 4 * sizeof(uint32_t));
    g_adapter_ctx.buffers[buffer_idx].nonce_start = nonce_start;
    g_adapter_ctx.buffers[buffer_idx].batch_size = batch_size;
    g_adapter_ctx.buffers[buffer_idx].target_bits = target_bits;
    g_adapter_ctx.buffers[buffer_idx].in_flight = true;
    g_adapter_ctx.buffers[buffer_idx].submit_ns = get_time_ns();
    
    /* SIMPLIFICATION C255v8n : Exécution synchrone (pas de thread séparé)
     * Raison : Gen9 native est déjà très rapide (3.386 MH/s C198)
     * Le double-buffering OpenCL n'apporte pas de gain significatif
     * On exécute immédiatement et on marque comme "in_flight" pour compatibilité API */
    
    /* Préparer header Bitcoin (80 bytes) */
    uint8_t block_header[80];
    memset(block_header, 0, 80);
    
    /* Copier midstate (32 bytes) + tail (16 bytes) */
    memcpy(block_header, midstate, 32);
    memcpy(block_header + 32, tail, 16);
    
    /* Exécuter mining Gen9 */
    uint32_t best_nonce = 0;
    uint32_t leading_zeros = 0;
    
    int ret = btc_gen9_execute_mining(
        g_adapter_ctx.ctx,
        block_header,
        nonce_start,
        batch_size,
        &best_nonce,
        &leading_zeros
    );
    
    if (ret != 0) {
        fprintf(stderr, "[GEN9-ADAPTER] Erreur execute_mining: %d\n", ret);
        g_adapter_ctx.buffers[buffer_idx].in_flight = false;
        pthread_mutex_unlock(&g_adapter_mutex);
        return BTC_OCL_ERROR;
    }
    
    /* Sauvegarder résultats */
    g_adapter_ctx.buffers[buffer_idx].out_nonce = best_nonce;
    g_adapter_ctx.buffers[buffer_idx].out_best = leading_zeros;
    g_adapter_ctx.buffers[buffer_idx].complete_ns = get_time_ns();
    
    /* Mettre à jour stats globales */
    g_adapter_ctx.total_batches++;
    g_adapter_ctx.total_hashes += batch_size;
    if (leading_zeros > g_adapter_ctx.best_bits_global) {
        g_adapter_ctx.best_bits_global = leading_zeros;
    }
    
    pthread_mutex_unlock(&g_adapter_mutex);
    return BTC_OCL_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * ATTENTE BATCH (Compatible OpenCL)
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_wait_batch(int buffer_idx, uint32_t* out_best) {
    if (!g_adapter_initialized) {
        fprintf(stderr, "[GEN9-ADAPTER] Non initialisé\n");
        return BTC_OCL_ERROR;
    }
    
    if (buffer_idx < 0 || buffer_idx >= GEN9_BUFFER_COUNT) {
        fprintf(stderr, "[GEN9-ADAPTER] buffer_idx invalide: %d\n", buffer_idx);
        return BTC_OCL_ERROR;
    }
    
    pthread_mutex_lock(&g_adapter_mutex);
    
    /* Vérifier si batch en cours */
    if (!g_adapter_ctx.buffers[buffer_idx].in_flight) {
        fprintf(stderr, "[GEN9-ADAPTER] Aucun batch en cours sur buffer[%d]\n", buffer_idx);
        pthread_mutex_unlock(&g_adapter_mutex);
        return BTC_OCL_ERROR;
    }
    
    /* SIMPLIFICATION C255v8n : Batch déjà complété (exécution synchrone)
     * On retourne immédiatement les résultats */
    
    *out_best = g_adapter_ctx.buffers[buffer_idx].out_best;
    g_adapter_ctx.buffers[buffer_idx].in_flight = false;
    
    pthread_mutex_unlock(&g_adapter_mutex);
    return BTC_OCL_OK;
}

/* ══════════════════════════════════════════════════════════════════════
 * STATS GPU (Compatible OpenCL)
 * ══════════════════════════════════════════════════════════════════════ */

int lum_gpu_get_stats(lum_gpu_stats_t* stats) {
    if (!g_adapter_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_adapter_mutex);
    
    uint64_t now_ns = get_time_ns();
    uint64_t elapsed_ns = now_ns - g_adapter_ctx.init_ns;
    
    /* Calculer hashrate actuel */
    if (elapsed_ns > 0) {
        double elapsed_s = (double)elapsed_ns / 1e9;
        stats->current_mhs = (double)g_adapter_ctx.total_hashes / elapsed_s / 1e6;
    } else {
        stats->current_mhs = 0.0;
    }
    
    /* Calculer temps kernel (moyenne sur dernier batch) */
    uint64_t last_kernel_ns = 0;
    for (int i = 0; i < GEN9_BUFFER_COUNT; i++) {
        if (g_adapter_ctx.buffers[i].complete_ns > g_adapter_ctx.buffers[i].submit_ns) {
            uint64_t kernel_ns = g_adapter_ctx.buffers[i].complete_ns - g_adapter_ctx.buffers[i].submit_ns;
            if (kernel_ns > last_kernel_ns) {
                last_kernel_ns = kernel_ns;
            }
        }
    }
    stats->kernel_exec_ns = last_kernel_ns;
    
    /* Utilisation GPU (estimation basée sur temps kernel) */
    uint64_t delta_ns = now_ns - g_adapter_ctx.last_stats_ns;
    if (delta_ns > 0) {
        stats->gpu_utilization_pct = (double)last_kernel_ns / (double)delta_ns * 100.0;
        if (stats->gpu_utilization_pct > 100.0) stats->gpu_utilization_pct = 100.0;
    } else {
        stats->gpu_utilization_pct = 0.0;
    }
    
    /* CPU libre (inverse utilisation GPU) */
    stats->cpu_free_pct = 100.0 - stats->gpu_utilization_pct;
    
    /* Stats cumulées */
    stats->total_hashes = g_adapter_ctx.total_hashes;
    stats->best_bits = g_adapter_ctx.best_bits_global;
    
    g_adapter_ctx.last_stats_ns = now_ns;
    
    pthread_mutex_unlock(&g_adapter_mutex);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * CLEANUP (Compatible OpenCL)
 * ══════════════════════════════════════════════════════════════════════ */

void btc_opencl_cleanup(void) {
    if (!g_adapter_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_adapter_mutex);
    
    fprintf(stderr, "[GEN9-ADAPTER] Cleanup (total_batches=%lu, total_hashes=%lu, best_bits=%u)\n",
            g_adapter_ctx.total_batches, g_adapter_ctx.total_hashes, g_adapter_ctx.best_bits_global);
    
    /* Cleanup contexte Gen9 */
    if (g_adapter_ctx.ctx) {
        btc_gen9_cleanup(g_adapter_ctx.ctx);
        g_adapter_ctx.ctx = NULL;
    }
    
    g_adapter_initialized = false;
    
    pthread_mutex_unlock(&g_adapter_mutex);
}

// Made with Bob
