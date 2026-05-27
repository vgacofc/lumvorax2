/**
 * LumVorax C255v2 — Soumission Asynchrone i915 DRM
 *
 * OBJECTIF: Séparer soumission GPU (asynchrone) et attente (synchrone)
 * pour permettre parallélisme réel de 9 dispatches simultanés.
 *
 * ARCHITECTURE:
 * - Phase 1: Soumettre 9 dispatches SANS attendre (retour immédiat)
 * - Phase 2: Attendre fin des 9 dispatches en parallèle
 * - Gain attendu: ×9 performance vs C255 (séquentiel)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

/* DRM headers */
#include <drm/i915_drm.h>
#include <xf86drm.h>

#include "btc_gen9_async_submit.h"
#include "btc_gen9_native_runner.h"

/* Mutex global pour DRM reopen (thread-safe) */
pthread_mutex_t g_drm_reopen_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Logger forensique
 */
static void log_async(const char* event, const char* details) {
    uint64_t ts = get_timestamp_ns();
    double ts_sec = ts / 1e9;
    printf("[%.9f] ASYNC_%s: %s\n", ts_sec, event, details);
    fflush(stdout);
}

/**
 * Soumettre dispatch asynchrone (retour immédiat, SANS GEM_WAIT)
 * 
 * DIFFÉRENCE vs btc_gen9_execute():
 * - btc_gen9_execute() = EXECBUFFER2 + GEM_WAIT (synchrone)
 * - btc_gen9_submit_async() = EXECBUFFER2 SEULEMENT (asynchrone)
 * 
 * @param ctx Contexte Gen9
 * @param fence_out Fence pour attente ultérieure
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_submit_async(btc_gen9_context_t* ctx, btc_gen9_fence_t* fence_out) {
    if (!ctx || !fence_out) {
        log_async("SUBMIT_ERROR", "ctx ou fence_out NULL");
        return -1;
    }

    uint64_t ts_start = get_timestamp_ns();
    
    /* C255v4: Batch buffer déjà pré-construit dans pool, pas besoin de rebuild */
    
    /* Obtenir handles via getters */
    uint32_t batch_bo = btc_gen9_get_batch_bo(ctx);
    uint32_t kernel_bo = btc_gen9_get_kernel_bo(ctx);
    uint32_t input_bo = btc_gen9_get_input_bo(ctx);
    uint32_t output_bo = btc_gen9_get_output_bo(ctx);
    uint32_t ctx_id = btc_gen9_get_ctx_id(ctx);
    int drm_fd = btc_gen9_get_drm_fd(ctx);
    size_t batch_size = btc_gen9_get_batch_size(ctx);
    
    /* Préparer exec_objects pour EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[4];
    memset(exec_objects, 0, sizeof(exec_objects));

    /* Kernel buffer */
    exec_objects[0].handle = kernel_bo;

    /* Input buffer */
    exec_objects[1].handle = input_bo;

    /* Output buffer */
    exec_objects[2].handle = output_bo;
    exec_objects[2].flags = EXEC_OBJECT_WRITE;

    /* Batch buffer */
    exec_objects[3].handle = batch_bo;

    /* Soumettre via EXECBUFFER2 */
    int ret;
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 4;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_size;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;

    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "EXECBUFFER2 failed errno=%d ctx_id=%u", errno, ctx_id);
        log_async("SUBMIT_FAILED", msg);
        return -1;
    }

    /* Sauvegarder fence pour attente ultérieure */
    fence_out->batch_bo = batch_bo;
    fence_out->drm_fd = drm_fd;
    fence_out->ctx_id = ctx_id;
    fence_out->submit_ts = ts_start;

    uint64_t ts_end = get_timestamp_ns();
    double submit_time = (ts_end - ts_start) / 1e9;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ctx_id=%u batch_bo=%u time=%.6f sec",
             ctx_id, batch_bo, submit_time);
    log_async("SUBMIT_SUCCESS", msg);

    /* RETOUR IMMÉDIAT (pas de GEM_WAIT) */
    return 0;
}

/**
 * Attendre fin dispatch via fence
 * 
 * @param fence Fence créée par btc_gen9_submit_async()
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_wait_fence(const btc_gen9_fence_t* fence) {
    if (!fence) {
        log_async("WAIT_ERROR", "fence NULL");
        return -1;
    }

    uint64_t ts_start = get_timestamp_ns();

    /* Attendre fin exécution GPU */
    struct drm_i915_gem_wait wait;
    memset(&wait, 0, sizeof(wait));
    wait.bo_handle = fence->batch_bo;
    wait.timeout_ns = 30000000000ULL;  /* 30 secondes */
    wait.flags = 0;

    int ret = ioctl(fence->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    if (ret != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "GEM_WAIT failed errno=%d batch_bo=%u", errno, fence->batch_bo);
        log_async("WAIT_FAILED", msg);
        return -1;
    }

    uint64_t ts_end = get_timestamp_ns();
    double wait_time = (ts_end - ts_start) / 1e9;
    double total_time = (ts_end - fence->submit_ts) / 1e9;

    char msg[256];
    snprintf(msg, sizeof(msg), "ctx_id=%u batch_bo=%u wait_time=%.6f total_time=%.6f", 
             fence->ctx_id, fence->batch_bo, wait_time, total_time);
    log_async("WAIT_SUCCESS", msg);

    return 0;
}

/**
 * DRM Reopen thread-safe (avec mutex global)
 * 
 * CORRECTION BUG C255: Multiples threads appelaient btc_gen9_reopen_drm()
 * simultanément → race condition sur destruction contextes.
 * 
 * SOLUTION: Mutex global pour sérialiser les reopen.
 * 
 * @param ctx Contexte Gen9
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_reopen_drm_threadsafe(btc_gen9_context_t* ctx) {
    if (!ctx) {
        log_async("REOPEN_ERROR", "ctx NULL");
        return -1;
    }

    /* Acquérir mutex (bloque si autre thread en train de reopen) */
    pthread_mutex_lock(&g_drm_reopen_mutex);
    
    log_async("REOPEN_MUTEX_ACQUIRED", "");

    /* Appeler reopen DRM existant */
    extern int btc_gen9_reopen_drm(btc_gen9_context_t* ctx);
    int ret = btc_gen9_reopen_drm(ctx);

    /* Libérer mutex */
    pthread_mutex_unlock(&g_drm_reopen_mutex);
    
    if (ret == 0) {
        log_async("REOPEN_MUTEX_RELEASED", "success");
    } else {
        log_async("REOPEN_MUTEX_RELEASED", "failed");
    }

    return ret;
}

/**
 * Attendre multiples fences en parallèle
 * 
 * @param fences Tableau de fences
 * @param count Nombre de fences
 * @return Nombre de fences réussies
 */
int btc_gen9_wait_fences_parallel(const btc_gen9_fence_t* fences, int count) {
    if (!fences || count <= 0) {
        log_async("WAIT_PARALLEL_ERROR", "fences NULL ou count <= 0");
        return 0;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "count=%d", count);
    log_async("WAIT_PARALLEL_START", msg);

    uint64_t ts_start = get_timestamp_ns();
    int success_count = 0;

    /* Attendre chaque fence */
    for (int i = 0; i < count; i++) {
        if (btc_gen9_wait_fence(&fences[i]) == 0) {
            success_count++;
        }
    }

    uint64_t ts_end = get_timestamp_ns();
    double total_time = (ts_end - ts_start) / 1e9;

    snprintf(msg, sizeof(msg), "success=%d/%d time=%.6f sec", success_count, count, total_time);
    log_async("WAIT_PARALLEL_COMPLETE", msg);

    return success_count;
}

// Made with Bob
