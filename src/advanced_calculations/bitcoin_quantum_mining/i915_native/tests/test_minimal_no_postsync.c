/**
 * LumVorax C198 Phase 15Y-F19 — TEST MINIMAL SANS POST_SYNC
 * 
 * OBJECTIF: Tester batch buffer MINIMAL qui fonctionne sur Gen9
 * MÉTHODE: AUCUN POST_SYNC_WRITE, juste PIPE_CONTROL + END
 * 
 * HYPOTHÈSE: POST_SYNC_WRITE est le problème, pas le batch lui-même
 * OpenCL ne vérifie probablement PAS l'exécution via POST_SYNC
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

#define GEN9_MI_BATCH_BUFFER_END    0x0A000000
#define GEN9_PIPE_CONTROL           0x7A000000
#define GEN9_MI_NOOP                0x00000000

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t batch_handle;
    void *batch_ptr;
    uint32_t dispatches_success;
    uint32_t dispatches_failed;
    double total_time;
} test_context_t;

static double get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int init_context(test_context_t *ctx) {
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        fprintf(stderr, COLOR_RED "❌ Impossible d'ouvrir /dev/dri/renderD128\n" COLOR_RESET);
        return -1;
    }
    
    struct drm_i915_gem_context_create create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create) != 0) {
        fprintf(stderr, COLOR_RED "❌ CONTEXT_CREATE failed\n" COLOR_RESET);
        return -1;
    }
    ctx->ctx_id = create.ctx_id;
    
    /* Allouer batch buffer via GEM */
    struct drm_i915_gem_create gem_create = { .size = 4096 };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_CREATE failed\n" COLOR_RESET);
        return -1;
    }
    ctx->batch_handle = gem_create.handle;
    
    /* Mapper batch buffer */
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = ctx->batch_handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_MMAP failed\n" COLOR_RESET);
        return -1;
    }
    ctx->batch_ptr = (void *)(uintptr_t)mmap_arg.addr_ptr;
    
    printf(COLOR_GREEN "✅ Init: ctx_id=%u batch_handle=%u\n" COLOR_RESET,
           ctx->ctx_id, ctx->batch_handle);
    
    return 0;
}

static void build_minimal_batch(test_context_t *ctx) {
    uint32_t *batch = (uint32_t *)ctx->batch_ptr;
    int idx = 0;
    
    /* PIPE_CONTROL simple (juste CS_STALL, pas de POST_SYNC) */
    batch[idx++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[idx++] = (1 << 20);  // CS_STALL uniquement
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = GEN9_MI_BATCH_BUFFER_END;
    
    /* Padding */
    while (idx < 16) {
        batch[idx++] = GEN9_MI_NOOP;
    }
}

static int execute_batch(test_context_t *ctx) {
    double t_start = get_timestamp();
    
    /* Construire batch */
    build_minimal_batch(ctx);
    
    /* Préparer exec_object2 */
    struct drm_i915_gem_exec_object2 exec_object = {0};
    exec_object.handle = ctx->batch_handle;
    exec_object.relocation_count = 0;
    exec_object.relocs_ptr = 0;
    exec_object.flags = 0;
    
    /* Préparer execbuffer2 */
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)&exec_object;
    execbuf.buffer_count = 1;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 64;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx->ctx_id;
    
    /* Exécuter */
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        fprintf(stderr, COLOR_RED "❌ EXECBUFFER2 failed: %s (errno=%d)\n" COLOR_RESET,
                strerror(errno), errno);
        ctx->dispatches_failed++;
        return -1;
    }
    
    /* Attendre completion GPU (via GEM_WAIT sur batch buffer) */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->batch_handle,
        .timeout_ns = 1000000000,  // 1 sec
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_WAIT failed: %s\n" COLOR_RESET, strerror(errno));
        ctx->dispatches_failed++;
        return -1;
    }
    
    double t_end = get_timestamp();
    double elapsed = t_end - t_start;
    
    ctx->dispatches_success++;
    ctx->total_time += elapsed;
    
    if (ctx->dispatches_success % 100 == 0) {
        printf(COLOR_GREEN "  [%4u/1000] ✅ OK (%.3f ms avg)\n" COLOR_RESET,
               ctx->dispatches_success, (ctx->total_time / ctx->dispatches_success) * 1000.0);
    }
    
    return 0;
}

static void cleanup_context(test_context_t *ctx) {
    if (ctx->batch_ptr) munmap(ctx->batch_ptr, 4096);
    
    if (ctx->batch_handle) {
        struct drm_gem_close close = { .handle = ctx->batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    
    if (ctx->ctx_id) {
        struct drm_i915_gem_context_destroy destroy = { .ctx_id = ctx->ctx_id };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
    }
    
    if (ctx->drm_fd >= 0) close(ctx->drm_fd);
}

int main(int argc, char **argv) {
    int num_dispatches = 1000;
    
    if (argc > 1) {
        num_dispatches = atoi(argv[1]);
    }
    
    printf("\n");
    printf(COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15Y-F19 — TEST MINIMAL NO POST_SYNC  ║\n");
    printf("║  Batch : PIPE_CONTROL (CS_STALL) + MI_BATCH_BUFFER_END   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_CYAN "[CONFIG] Dispatches : %d\n" COLOR_RESET, num_dispatches);
    printf(COLOR_CYAN "[CONFIG] Batch : PIPE_CONTROL (CS_STALL seul) + END\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] POST_SYNC : AUCUN (désactivé)\n" COLOR_RESET);
    printf("\n");
    
    test_context_t ctx = {0};
    if (init_context(&ctx) != 0) {
        return 1;
    }
    
    printf(COLOR_CYAN "[TEST] Exécution %d dispatches...\n" COLOR_RESET, num_dispatches);
    printf("\n");
    
    double test_start = get_timestamp();
    
    for (int i = 0; i < num_dispatches; i++) {
        if (execute_batch(&ctx) != 0) {
            fprintf(stderr, COLOR_RED "\n❌ Dispatch #%d failed, arrêt test\n" COLOR_RESET, i + 1);
            break;
        }
        
        usleep(1000);  // 1ms entre dispatches
    }
    
    double test_end = get_timestamp();
    double test_time = test_end - test_start;
    
    printf("\n");
    printf(COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  Résultats Test Minimal No POST_SYNC                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    printf("Dispatches réussis:  %s%u%s\n", COLOR_GREEN, ctx.dispatches_success, COLOR_RESET);
    printf("Dispatches échoués:  %s%u%s\n",
           ctx.dispatches_failed > 0 ? COLOR_RED : COLOR_GREEN,
           ctx.dispatches_failed, COLOR_RESET);
    printf("Temps total:         %.3f sec\n", test_time);
    
    if (ctx.dispatches_success > 0) {
        printf("Temps moyen/dispatch: %.3f ms\n",
               (ctx.total_time / ctx.dispatches_success) * 1000.0);
        printf("Throughput:          %.2f dispatches/sec\n",
               ctx.dispatches_success / test_time);
    }
    
    printf("\n");
    
    if (ctx.dispatches_success == num_dispatches) {
        printf(COLOR_GREEN COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — 1000 DISPATCHES STABLES !              ║\n");
        printf("║                                                            ║\n");
        printf("║  🎯 SOLUTION TROUVÉE : Pas de POST_SYNC_WRITE             ║\n");
        printf("║  Le GPU Gen9 est stable sans POST_SYNC                    ║\n");
        printf("║  Prochaine étape : Ajouter compute kernel                 ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else if (ctx.dispatches_success > 100) {
        printf(COLOR_CYAN COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  AMÉLIORATION — %u/%u dispatches                      ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("║  Continuer investigation                                  ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else {
        printf(COLOR_RED COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ ÉCHEC — Seulement %u/%u dispatches                    ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("║  Problème persiste                                        ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    }
    
    printf("\n");
    
    cleanup_context(&ctx);
    
    return (ctx.dispatches_success == num_dispatches) ? 0 : 1;
}

// Made with Bob
