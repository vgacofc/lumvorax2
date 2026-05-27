/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_parallel.c — CYCLE C249 Parallélisation Multi-Contextes
 *
 * OBJECTIF : Exécuter 3 dispatches simultanés sur les 3 contextes GPU
 * GAIN ESTIMÉ : 2-3x throughput
 * RISQUES : Contention mémoire, saturation PCIe, complexité synchronisation
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "../include/btc_gen9_native_runner.h"

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES PARALLÉLISATION
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Contexte thread dispatch parallèle
 */
typedef struct {
    btc_gen9_context_t* ctx;       /* Contexte GPU principal */
    int ctx_index;                  /* Index contexte (0, 1, 2) */
    uint32_t ctx_id;                /* ID contexte GPU */
    int dispatch_number;            /* Numéro dispatch global */
    int result;                     /* Résultat exécution */
    double exec_time_sec;           /* Temps exécution */
    pthread_t thread_id;            /* ID thread */
} parallel_dispatch_t;

/* ══════════════════════════════════════════════════════════════════════
 * FONCTION THREAD DISPATCH
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Thread worker : exécute 1 dispatch sur 1 contexte
 */
static void* parallel_dispatch_worker(void* arg) {
    parallel_dispatch_t* pd = (parallel_dispatch_t*)arg;
    
    uint64_t start_ts = get_timestamp_ns();
    
    LOG_EVENT(pd->ctx, "PARALLEL_DISPATCH_START: ctx_index=%d ctx_id=%u dispatch=%d thread=%lu",
             pd->ctx_index, pd->ctx_id, pd->dispatch_number, (unsigned long)pthread_self());
    
    /* Sélectionner contexte dans le pool */
    uint32_t old_ctx_id = pd->ctx->ctx_id;
    pd->ctx->ctx_id = pd->ctx_id;
    
    /* Exécuter dispatch sur ce contexte */
    pd->result = btc_gen9_execute(pd->ctx);
    
    /* Restaurer contexte original */
    pd->ctx->ctx_id = old_ctx_id;
    
    uint64_t end_ts = get_timestamp_ns();
    pd->exec_time_sec = ns_to_sec(end_ts - start_ts);
    
    if (pd->result == 0) {
        LOG_EVENT(pd->ctx, "PARALLEL_DISPATCH_SUCCESS: ctx_index=%d ctx_id=%u dispatch=%d time=%.6f sec",
                 pd->ctx_index, pd->ctx_id, pd->dispatch_number, pd->exec_time_sec);
    } else {
        LOG_EVENT(pd->ctx, "PARALLEL_DISPATCH_FAILED: ctx_index=%d ctx_id=%u dispatch=%d error=%d",
                 pd->ctx_index, pd->ctx_id, pd->dispatch_number, pd->result);
    }
    
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════
 * FONCTION PRINCIPALE PARALLÉLISATION
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C249 OPTIMISATION #3 : Parallélisation Multi-Contextes
 * 
 * Exécute 3 dispatches simultanés sur les 3 contextes GPU
 * 
 * ARCHITECTURE :
 * - Thread 1 : Dispatch sur ctx_pool[0]
 * - Thread 2 : Dispatch sur ctx_pool[1]
 * - Thread 3 : Dispatch sur ctx_pool[2]
 * 
 * SYNCHRONISATION :
 * - pthread_create() pour lancer les 3 threads
 * - pthread_join() pour attendre la fin des 3 dispatches
 * 
 * GAIN ESTIMÉ : 2-3x throughput (si pas de contention)
 */
int btc_gen9_execute_parallel_batch(btc_gen9_context_t* ctx, int total_dispatches) {
    if (!ctx) return -1;
    
    uint64_t batch_start_ts = get_timestamp_ns();
    
    LOG_EVENT(ctx, "PARALLEL_BATCH_START: total_dispatches=%d contexts=%d",
             total_dispatches, CTX_POOL_SIZE);
    
    int dispatches_completed = 0;
    int parallel_rounds = 0;
    
    /* Exécuter par groupes de 3 dispatches parallèles */
    while (dispatches_completed < total_dispatches) {
        parallel_rounds++;
        
        /* Calculer combien de dispatches dans ce round */
        int remaining = total_dispatches - dispatches_completed;
        int dispatches_this_round = (remaining >= CTX_POOL_SIZE) ? CTX_POOL_SIZE : remaining;
        
        LOG_EVENT(ctx, "PARALLEL_ROUND_START: round=%d dispatches=%d remaining=%d",
                 parallel_rounds, dispatches_this_round, remaining);
        
        /* Préparer contextes threads */
        parallel_dispatch_t pd[CTX_POOL_SIZE];
        
        for (int i = 0; i < dispatches_this_round; i++) {
            pd[i].ctx = ctx;
            pd[i].ctx_index = i;
            pd[i].ctx_id = ctx->ctx_pool[i];
            pd[i].dispatch_number = dispatches_completed + i + 1;
            pd[i].result = -1;
            pd[i].exec_time_sec = 0.0;
        }
        
        /* Lancer threads parallèles */
        uint64_t threads_start_ts = get_timestamp_ns();
        
        for (int i = 0; i < dispatches_this_round; i++) {
            int ret = pthread_create(&pd[i].thread_id, NULL, parallel_dispatch_worker, &pd[i]);
            if (ret != 0) {
                LOG_EVENT(ctx, "PARALLEL_THREAD_CREATE_FAILED: ctx_index=%d error=%d", i, ret);
                return -1;
            }
        }
        
        LOG_EVENT(ctx, "PARALLEL_THREADS_LAUNCHED: count=%d", dispatches_this_round);
        
        /* Attendre fin de tous les threads */
        for (int i = 0; i < dispatches_this_round; i++) {
            pthread_join(pd[i].thread_id, NULL);
        }
        
        uint64_t threads_end_ts = get_timestamp_ns();
        double round_time_sec = ns_to_sec(threads_end_ts - threads_start_ts);
        
        /* Vérifier résultats */
        int success_count = 0;
        double max_exec_time = 0.0;
        double min_exec_time = 999999.0;
        double total_exec_time = 0.0;
        
        for (int i = 0; i < dispatches_this_round; i++) {
            if (pd[i].result == 0) {
                success_count++;
                total_exec_time += pd[i].exec_time_sec;
                if (pd[i].exec_time_sec > max_exec_time) max_exec_time = pd[i].exec_time_sec;
                if (pd[i].exec_time_sec < min_exec_time) min_exec_time = pd[i].exec_time_sec;
            } else {
                LOG_EVENT(ctx, "PARALLEL_ROUND_DISPATCH_FAILED: ctx_index=%d dispatch=%d",
                         i, pd[i].dispatch_number);
                return -1;
            }
        }
        
        double avg_exec_time = total_exec_time / dispatches_this_round;
        
        LOG_EVENT(ctx, "PARALLEL_ROUND_SUCCESS: round=%d dispatches=%d/%d time=%.6f sec min=%.6f max=%.6f avg=%.6f",
                 parallel_rounds, success_count, dispatches_this_round, round_time_sec,
                 min_exec_time, max_exec_time, avg_exec_time);
        
        dispatches_completed += success_count;
        
        /* Vérifier si reopen DRM nécessaire (tous les 9 dispatches) */
        if (dispatches_completed % 9 == 0 && dispatches_completed < total_dispatches) {
            LOG_EVENT(ctx, "PARALLEL_REOPEN_TRIGGER: dispatches=%d (multiple de 9)", dispatches_completed);
            
            int ret = reopen_drm_connection(ctx);
            if (ret < 0) {
                LOG_EVENT(ctx, "PARALLEL_REOPEN_FAILED");
                return -1;
            }
        }
    }
    
    uint64_t batch_end_ts = get_timestamp_ns();
    double total_time_sec = ns_to_sec(batch_end_ts - batch_start_ts);
    
    LOG_EVENT(ctx, "PARALLEL_BATCH_SUCCESS: dispatches=%d rounds=%d time=%.3f sec avg_per_dispatch=%.6f sec",
             dispatches_completed, parallel_rounds, total_time_sec, total_time_sec / dispatches_completed);
    
    return dispatches_completed;
}

/**
 * C249 OPTIMISATION #3 VARIANTE : Parallélisation avec Pipelining
 * 
 * Exécute dispatches en pipeline : pendant que GPU exécute sur ctx[0],
 * CPU prépare batch pour ctx[1], etc.
 * 
 * GAIN ESTIMÉ : 3-4x throughput (si CPU/GPU bien équilibrés)
 */
int btc_gen9_execute_pipelined(btc_gen9_context_t* ctx, int total_dispatches) {
    if (!ctx) return -1;
    
    LOG_EVENT(ctx, "PIPELINED_BATCH_START: total_dispatches=%d (NOT IMPLEMENTED YET)",
             total_dispatches);
    
    /* TODO C249 : Implémenter pipelining CPU/GPU */
    /* Pour l'instant, fallback sur parallélisation simple */
    return btc_gen9_execute_parallel_batch(ctx, total_dispatches);
}

// Made with Bob
