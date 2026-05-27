/*
 * LumVorax C198 Phase 15Y-F10 — Test Dummy Batch Minimal
 * 
 * Objectif : Confirmer que le problème est le compute pipeline, pas le batch de base
 * Test : 10000 dispatches avec batch ultra-minimal (PIPE_CONTROL + END)
 * Résultat attendu : 10000/10000 sans GPU hang
 */

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
#include <drm/i915_drm.h>

#define BATCH_SIZE 4096
#define NUM_DISPATCHES 10000

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t batch_handle;
    void* batch_map;
} dummy_context_t;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int create_context(dummy_context_t* ctx) {
    // Ouvrir DRM
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        fprintf(stderr, "❌ DRM_OPEN_FAILED: errno=%d\n", errno);
        return -1;
    }
    
    // Créer contexte
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "❌ CONTEXT_CREATE_FAILED: errno=%d\n", errno);
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    
    // Allouer batch buffer
    struct drm_i915_gem_create gem_create = {
        .size = BATCH_SIZE
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        fprintf(stderr, "❌ GEM_CREATE_FAILED: errno=%d\n", errno);
        close(ctx->drm_fd);
        return -1;
    }
    ctx->batch_handle = gem_create.handle;
    
    // Mapper batch buffer
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = ctx->batch_handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(stderr, "❌ GEM_MMAP_FAILED: errno=%d\n", errno);
        close(ctx->drm_fd);
        return -1;
    }
    ctx->batch_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    printf("✅ INIT_SUCCESS: ctx_id=%u batch_handle=%u\n", ctx->ctx_id, ctx->batch_handle);
    return 0;
}

static void build_dummy_batch(dummy_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    memset(batch, 0, BATCH_SIZE);
    
    int idx = 0;
    
    // PIPE_CONTROL (flush minimal)
    batch[idx++] = 0x7A000004;  // PIPE_CONTROL | (6-2)
    batch[idx++] = (1 << 20);   // CS_STALL
    batch[idx++] = 0;           // Address low
    batch[idx++] = 0;           // Address high
    batch[idx++] = 0;           // Data low
    batch[idx++] = 0;           // Data high
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    printf("✅ BATCH_BUILD: commands=%d bytes=%d\n", idx, idx * 4);
}

static int execute_batch(dummy_context_t* ctx) {
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = ctx->batch_handle,
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = EXEC_OBJECT_PINNED,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)&exec_obj,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = 32,  // 7 commandes * 4 bytes
        .cliprects_ptr = 0,
        .num_cliprects = 0,
        .DR1 = 0,
        .DR4 = 0,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx->ctx_id,
        .rsvd2 = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        return -1;
    }
    
    // Attendre fin GPU
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = ctx->batch_handle,
        .flags = 0,
        .timeout_ns = 5000000000LL  // 5 secondes
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        return -1;
    }
    
    return 0;
}

static void cleanup(dummy_context_t* ctx) {
    if (ctx->batch_map) {
        munmap(ctx->batch_map, BATCH_SIZE);
    }
    if (ctx->batch_handle) {
        struct drm_gem_close close_arg = { .handle = ctx->batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    }
    if (ctx->ctx_id) {
        struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx->ctx_id };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    }
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
    }
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15Y-F10 — Test Dummy Batch          ║\n");
    printf("║  Objectif : Confirmer cause = compute pipeline           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    dummy_context_t ctx = {0};
    
    // Init
    printf("[PHASE 1/3] Initialisation...\n");
    if (create_context(&ctx) < 0) {
        return 1;
    }
    
    // Build batch
    printf("\n[PHASE 2/3] Construction batch dummy...\n");
    build_dummy_batch(&ctx);
    
    // Test 10000 dispatches
    printf("\n[PHASE 3/3] Test %d dispatches...\n", NUM_DISPATCHES);
    double start = get_time_sec();
    int success = 0;
    int failed = 0;
    
    for (int i = 0; i < NUM_DISPATCHES; i++) {
        if (execute_batch(&ctx) == 0) {
            success++;
        } else {
            failed++;
            fprintf(stderr, "  [%5d/%d] ❌ ÉCHEC (errno=%d)\n", i+1, NUM_DISPATCHES, errno);
            break;
        }
        
        // Progress bar
        if ((i + 1) % 1000 == 0) {
            printf("  [%5d/%d] ✅ OK (%.1f%%)\n", i+1, NUM_DISPATCHES, (i+1) * 100.0 / NUM_DISPATCHES);
        }
    }
    
    double elapsed = get_time_sec() - start;
    
    // Résultats
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST DUMMY BATCH                               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Dispatches réussis : %d/%d (%.1f%%)\n", success, NUM_DISPATCHES, success * 100.0 / NUM_DISPATCHES);
    printf("Dispatches échoués : %d/%d\n", failed, NUM_DISPATCHES);
    printf("Temps total        : %.3f sec\n", elapsed);
    printf("Temps moyen        : %.3f ms/dispatch\n", elapsed * 1000.0 / success);
    printf("\n");
    
    if (success == NUM_DISPATCHES) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — CAUSE CONFIRMÉE                        ║\n");
        printf("║  Le problème est le COMPUTE PIPELINE, pas le batch       ║\n");
        printf("║  Batch minimal fonctionne parfaitement                    ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  ÉCHEC PARTIEL — Problème plus profond               ║\n");
        printf("║  Même batch minimal cause des erreurs                     ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    // Cleanup
    cleanup(&ctx);
    
    return (success == NUM_DISPATCHES) ? 0 : 1;
}

// Made with Bob
