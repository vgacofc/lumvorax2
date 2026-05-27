/**
 * @file btc_warmup_post_reopen.c
 * @brief Module de warm-up GPU après reopen DRM
 * 
 * Ce module implémente une stratégie de réchauffement du cache GPU après
 * la réouverture du file descriptor DRM. Le problème identifié en C241 montre
 * que le premier dispatch après reopen prend 1.5-3.5 secondes au lieu de 0.7s.
 * 
 * Stratégie:
 * 1. Dispatch minimal pour réchauffer le cache GPU
 * 2. Attendre la fin de l'exécution
 * 3. Vérifier que le GPU est prêt
 * 
 * Cycle: C242
 * Objectif: Éliminer la pénalité de performance post-reopen
 */

#include "btc_warmup_post_reopen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>

/**
 * Obtient le timestamp actuel en nanosecondes
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Crée un batch buffer minimal pour le warm-up
 * 
 * Ce batch buffer contient uniquement les commandes essentielles:
 * - PIPE_CONTROL pour synchronisation
 * - MI_NOOP pour padding
 * - MI_BATCH_BUFFER_END pour terminer
 */
static int create_warmup_batch(void *batch_ptr, size_t batch_size) {
    if (!batch_ptr || batch_size < 64) {
        return -1;
    }
    
    uint32_t *batch = (uint32_t *)batch_ptr;
    int offset = 0;
    
    // PIPE_CONTROL: Flush caches
    batch[offset++] = 0x7a000004;  // PIPE_CONTROL command
    batch[offset++] = 0x00100000;  // Flags: CS stall
    batch[offset++] = 0x00000000;  // Address low
    batch[offset++] = 0x00000000;  // Address high
    batch[offset++] = 0x00000000;  // Data low
    batch[offset++] = 0x00000000;  // Data high
    
    // MI_NOOP: Padding
    for (int i = 0; i < 8; i++) {
        batch[offset++] = 0x00000000;  // MI_NOOP
    }
    
    // MI_BATCH_BUFFER_END: Terminer
    batch[offset++] = 0x0a000000;  // MI_BATCH_BUFFER_END
    
    return offset * 4;  // Retourner la taille en bytes
}

/**
 * Exécute un dispatch de warm-up minimal
 */
static int execute_warmup_dispatch(struct btc_warmup_context *ctx) {
    if (!ctx) {
        return -1;
    }
    
    uint64_t start_ns = get_timestamp_ns();
    
    // Créer le batch buffer minimal
    int batch_len = create_warmup_batch(ctx->batch_ptr, ctx->batch_size);
    if (batch_len < 0) {
        fprintf(stderr, "[WARMUP] Failed to create warmup batch\n");
        return -1;
    }
    
    // Préparer les exec objects
    struct drm_i915_gem_exec_object2 exec_objects[1];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx->batch_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 0;
    exec_objects[0].offset = 0;
    exec_objects[0].flags = EXEC_OBJECT_PINNED;
    exec_objects[0].rsvd1 = 0;
    exec_objects[0].rsvd2 = 0;
    
    // Préparer l'execbuffer
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 1;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_len;
    execbuf.cliprects_ptr = 0;
    execbuf.num_cliprects = 0;
    execbuf.DR1 = 0;
    execbuf.DR4 = 0;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx->context_id;
    execbuf.rsvd2 = 0;
    
    // Exécuter le batch
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        fprintf(stderr, "[WARMUP] Failed to execute warmup batch: %s\n", strerror(errno));
        return -1;
    }
    
    uint64_t exec_ns = get_timestamp_ns();
    
    // Attendre la fin de l'exécution
    struct drm_i915_gem_wait wait;
    memset(&wait, 0, sizeof(wait));
    wait.bo_handle = ctx->batch_handle;
    wait.timeout_ns = 1000000000;  // 1 seconde max
    wait.flags = 0;
    
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    if (ret != 0) {
        fprintf(stderr, "[WARMUP] Failed to wait for warmup completion: %s\n", strerror(errno));
        return -1;
    }
    
    uint64_t end_ns = get_timestamp_ns();
    
    // Log des timings
    fprintf(stderr, "[WARMUP] Dispatch completed:\n");
    fprintf(stderr, "  - Batch creation: %.3f µs\n", (exec_ns - start_ns) / 1000.0);
    fprintf(stderr, "  - GPU execution:  %.3f µs\n", (end_ns - exec_ns) / 1000.0);
    fprintf(stderr, "  - Total time:     %.3f µs\n", (end_ns - start_ns) / 1000.0);
    
    return 0;
}

/**
 * Initialise le contexte de warm-up
 */
int btc_warmup_init(struct btc_warmup_context *ctx, int drm_fd, 
                    uint32_t batch_handle, void *batch_ptr, size_t batch_size,
                    uint32_t context_id) {
    if (!ctx) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(*ctx));
    
    ctx->drm_fd = drm_fd;
    ctx->batch_handle = batch_handle;
    ctx->batch_ptr = batch_ptr;
    ctx->batch_size = batch_size;
    ctx->context_id = context_id;
    ctx->warmup_enabled = 1;
    ctx->warmup_count = 0;
    
    fprintf(stderr, "[WARMUP] Initialized:\n");
    fprintf(stderr, "  - DRM fd:       %d\n", drm_fd);
    fprintf(stderr, "  - Batch handle: %u\n", batch_handle);
    fprintf(stderr, "  - Batch size:   %zu bytes\n", batch_size);
    fprintf(stderr, "  - Context ID:   %u\n", context_id);
    
    return 0;
}

/**
 * Exécute le warm-up après reopen
 * 
 * Cette fonction doit être appelée immédiatement après la réouverture
 * du file descriptor DRM et la recréation des buffers.
 */
int btc_warmup_execute(struct btc_warmup_context *ctx) {
    if (!ctx || !ctx->warmup_enabled) {
        return -1;
    }
    
    uint64_t start_ns = get_timestamp_ns();
    
    fprintf(stderr, "[WARMUP] Starting GPU warm-up (count: %d)...\n", ctx->warmup_count);
    
    // Exécuter plusieurs dispatches de warm-up pour garantir le réchauffement
    const int num_warmup_dispatches = 3;
    
    for (int i = 0; i < num_warmup_dispatches; i++) {
        int ret = execute_warmup_dispatch(ctx);
        if (ret != 0) {
            fprintf(stderr, "[WARMUP] Dispatch %d/%d failed\n", i + 1, num_warmup_dispatches);
            return -1;
        }
        
        // Petit délai entre les dispatches
        usleep(1000);  // 1ms
    }
    
    uint64_t end_ns = get_timestamp_ns();
    double total_ms = (end_ns - start_ns) / 1000000.0;
    
    ctx->warmup_count++;
    ctx->last_warmup_time_ms = total_ms;
    
    fprintf(stderr, "[WARMUP] Completed successfully:\n");
    fprintf(stderr, "  - Dispatches:   %d\n", num_warmup_dispatches);
    fprintf(stderr, "  - Total time:   %.3f ms\n", total_ms);
    fprintf(stderr, "  - Warmup count: %d\n", ctx->warmup_count);
    
    return 0;
}

/**
 * Désactive le warm-up (pour tests comparatifs)
 */
void btc_warmup_disable(struct btc_warmup_context *ctx) {
    if (ctx) {
        ctx->warmup_enabled = 0;
        fprintf(stderr, "[WARMUP] Disabled\n");
    }
}

/**
 * Active le warm-up
 */
void btc_warmup_enable(struct btc_warmup_context *ctx) {
    if (ctx) {
        ctx->warmup_enabled = 1;
        fprintf(stderr, "[WARMUP] Enabled\n");
    }
}

/**
 * Obtient les statistiques de warm-up
 */
void btc_warmup_get_stats(const struct btc_warmup_context *ctx,
                          struct btc_warmup_stats *stats) {
    if (!ctx || !stats) {
        return;
    }
    
    memset(stats, 0, sizeof(*stats));
    stats->total_warmups = ctx->warmup_count;
    stats->last_warmup_time_ms = ctx->last_warmup_time_ms;
    stats->warmup_enabled = ctx->warmup_enabled;
}

/**
 * Affiche les statistiques de warm-up
 */
void btc_warmup_print_stats(const struct btc_warmup_context *ctx) {
    if (!ctx) {
        return;
    }
    
    fprintf(stderr, "[WARMUP_STATS]\n");
    fprintf(stderr, "  - Enabled:          %s\n", ctx->warmup_enabled ? "Yes" : "No");
    fprintf(stderr, "  - Total warmups:    %d\n", ctx->warmup_count);
    fprintf(stderr, "  - Last warmup time: %.3f ms\n", ctx->last_warmup_time_ms);
}

/**
 * Libère les ressources du contexte de warm-up
 */
void btc_warmup_cleanup(struct btc_warmup_context *ctx) {
    if (!ctx) {
        return;
    }
    
    fprintf(stderr, "[WARMUP] Cleanup (total warmups: %d)\n", ctx->warmup_count);
    memset(ctx, 0, sizeof(*ctx));
}

// Made with Bob
