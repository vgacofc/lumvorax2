/**
 * LumVorax C198 Phase 15Y-F18 — TEST CRITIQUE KERNEL ZERO COMPUTE
 * 
 * OBJECTIF: Prouver que le GPU hang est causé par GPGPU_WALKER/shader
 * MÉTHODE: Batch ultra-minimal SANS compute (juste POST_SYNC write)
 * 
 * RÉSULTAT ATTENDU:
 * - Si 1000 dispatches OK → Bug shader/walker confirmé (90% probabilité)
 * - Si crash → Problème pipeline/sync/batch
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
#define COLOR_YELLOW  "\033[33m"
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
    uint64_t batch_addr;
    
    uint32_t output_handle;
    void *output_ptr;
    uint64_t output_addr;
    
    uint32_t dispatches_success;
    uint32_t dispatches_failed;
    double total_time;
} test_context_t;

static double get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int gem_alloc(int fd, uint64_t size, uint32_t *handle, void **ptr, uint64_t *addr) {
    struct drm_i915_gem_create create = { .size = size };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_CREATE failed: %s\n" COLOR_RESET, strerror(errno));
        return -1;
    }
    
    *handle = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = *handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_MMAP failed: %s\n" COLOR_RESET, strerror(errno));
        return -1;
    }
    
    *ptr = (void *)(uintptr_t)mmap_arg.addr_ptr;
    *addr = mmap_arg.addr_ptr;
    
    return 0;
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
    
    if (gem_alloc(ctx->drm_fd, 4096, &ctx->batch_handle, &ctx->batch_ptr, &ctx->batch_addr) != 0) {
        return -1;
    }
    
    if (gem_alloc(ctx->drm_fd, 4096, &ctx->output_handle, &ctx->output_ptr, &ctx->output_addr) != 0) {
        return -1;
    }
    
    printf(COLOR_GREEN "✅ Init: ctx_id=%u batch=0x%lx output=0x%lx\n" COLOR_RESET,
           ctx->ctx_id, ctx->batch_addr, ctx->output_addr);
    
    return 0;
}

static void build_zero_compute_batch(test_context_t *ctx, uint32_t dispatch_id) {
    uint32_t *batch = (uint32_t *)ctx->batch_ptr;
    int idx = 0;
    
    /* PIPE_CONTROL avec POST_SYNC_WRITE (preuve exécution GPU) */
    batch[idx++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[idx++] =
        (1 << 20) |  // CS_STALL
        (1 << 6)  |  // DC_FLUSH
        (1 << 1);    // POST_SYNC_WRITE
    /* CORRECTION CRITIQUE : Mettre 0 ici, i915 relocalisera via exec_objects[0].offset */
    batch[idx++] = 0;  // Address Low (relocalisé par i915)
    batch[idx++] = 0;  // Address High (relocalisé par i915)
    batch[idx++] = 0xDEAD0000 | dispatch_id;  // Marker unique par dispatch
    batch[idx++] = 0;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = GEN9_MI_BATCH_BUFFER_END;
    
    /* Padding */
    while (idx < 16) {
        batch[idx++] = GEN9_MI_NOOP;
    }
}

static int execute_batch(test_context_t *ctx, uint32_t dispatch_id) {
    double t_start = get_timestamp();
    
    /* Construire batch */
    build_zero_compute_batch(ctx, dispatch_id);
    
    /* Reset output marker */
    uint32_t *output = (uint32_t *)ctx->output_ptr;
    output[0] = 0xFFFFFFFF;
    
    /* CORRECTION CRITIQUE : Ajouter relocation pour POST_SYNC address */
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = ctx->output_handle,
        .delta = 0,
        .offset = 8,  // Offset du dword address dans batch (3ème dword = 2*4 bytes)
        .presumed_offset = 0,
        .read_domains = I915_GEM_DOMAIN_RENDER,
        .write_domain = I915_GEM_DOMAIN_RENDER
    };
    
    /* Préparer exec_object2 */
    struct drm_i915_gem_exec_object2 exec_objects[2] = {0};
    
    exec_objects[0].handle = ctx->output_handle;
    exec_objects[0].flags = 0;
    
    exec_objects[1].handle = ctx->batch_handle;
    exec_objects[1].relocation_count = 1;
    exec_objects[1].relocs_ptr = (uintptr_t)&reloc;
    exec_objects[1].flags = 0;
    
    /* Préparer execbuffer2 */
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
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
    
    /* Attendre completion GPU */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->output_handle,
        .timeout_ns = 1000000000,  // 1 sec
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_WAIT failed: %s\n" COLOR_RESET, strerror(errno));
        ctx->dispatches_failed++;
        return -1;
    }
    
    /* Vérifier marker GPU */
    uint32_t expected = 0xDEAD0000 | dispatch_id;
    if (output[0] != expected) {
        fprintf(stderr, COLOR_RED "❌ Marker mismatch: got 0x%08x, expected 0x%08x\n" COLOR_RESET,
                output[0], expected);
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
    if (ctx->output_ptr) munmap(ctx->output_ptr, 4096);
    
    if (ctx->batch_handle) {
        struct drm_gem_close close = { .handle = ctx->batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    if (ctx->output_handle) {
        struct drm_gem_close close = { .handle = ctx->output_handle };
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
    printf("║  LumVorax C198 Phase 15Y-F18 — TEST KERNEL ZERO COMPUTE   ║\n");
    printf("║  Objectif : Prouver que le hang est causé par shader      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_CYAN "[CONFIG] Dispatches : %d\n" COLOR_RESET, num_dispatches);
    printf(COLOR_CYAN "[CONFIG] Batch : PIPE_CONTROL + POST_SYNC_WRITE + END\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] Compute : AUCUN (zero compute)\n" COLOR_RESET);
    printf("\n");
    
    test_context_t ctx = {0};
    if (init_context(&ctx) != 0) {
        return 1;
    }
    
    printf(COLOR_CYAN "[TEST] Exécution %d dispatches...\n" COLOR_RESET, num_dispatches);
    printf("\n");
    
    double test_start = get_timestamp();
    
    for (int i = 0; i < num_dispatches; i++) {
        if (execute_batch(&ctx, i + 1) != 0) {
            fprintf(stderr, COLOR_RED "\n❌ Dispatch #%d failed, arrêt test\n" COLOR_RESET, i + 1);
            break;
        }
        
        usleep(1000);  // 1ms entre dispatches
    }
    
    double test_end = get_timestamp();
    double test_time = test_end - test_start;
    
    printf("\n");
    printf(COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  Résultats Test Kernel Zero Compute                       ║\n");
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
        printf("║  🎯 CONCLUSION : Le problème est le SHADER/WALKER         ║\n");
        printf("║  Le GPU est stable sans compute                           ║\n");
        printf("║  Prochaine étape : Fix GPGPU_WALKER configuration         ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else if (ctx.dispatches_success > 100) {
        printf(COLOR_YELLOW COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  AMÉLIORATION MAJEURE — %u/%u dispatches             ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("║                                                            ║\n");
        printf("║  Le problème est PARTIELLEMENT le shader/walker           ║\n");
        printf("║  Mais aussi pipeline/sync                                 ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else {
        printf(COLOR_RED COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ ÉCHEC — Seulement %u/%u dispatches                    ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("║                                                            ║\n");
        printf("║  🎯 CONCLUSION : Le problème est PIPELINE/SYNC/BATCH      ║\n");
        printf("║  Pas le shader/walker                                     ║\n");
        printf("║  Prochaine étape : Fix PIPE_CONTROL + POST_SYNC           ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    }
    
    printf("\n");
    
    cleanup_context(&ctx);
    
    return (ctx.dispatches_success == num_dispatches) ? 0 : 1;
}

// Made with Bob
