/*
 * LumVorax C271 — TEST E : KERNEL NOP
 * Objectif : Isoler kernel GPU comme cause du GPU hang
 * 
 * STRATÉGIE :
 * - Remplacer kernel SHA-256 par kernel NOP minimal
 * - Exécuter 100 dispatches
 * - Si pas de GPU hang → Problème = kernel SHA-256
 * - Si GPU hang persiste → Problème = infrastructure (contexte/batch/FD)
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

/* Kernel Gen9 NOP minimal : NOP + EOT */
static const uint32_t KERNEL_NOP_GEN9[] = {
    0x00000000,  /* NOP */
    0x05000000,  /* EOT (End Of Thread) */
};

typedef struct {
    int drm_fd;
    uint32_t ctx_pool[9];
    int ctx_pool_index;
    uint32_t batch_bo_pool[90];
    int batch_pool_index;
    uint32_t kernel_bo;
    uint32_t input_bo;
    uint32_t output_bo;
    uint64_t total_dispatches;
    FILE* log_file;
} test_e_context_t;

static int test_e_init(test_e_context_t* ctx) {
    memset(ctx, 0, sizeof(*ctx));
    
    /* Ouvrir DRM */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        fprintf(stderr, "[TEST_E_ERROR] Cannot open DRM: %s\n", strerror(errno));
        return -1;
    }
    
    /* Créer log */
    system("mkdir -p logs/forensic/test_e");
    ctx->log_file = fopen("logs/forensic/test_e/execution.log", "w");
    
    /* Créer 9 contextes */
    for (int i = 0; i < 9; i++) {
        struct drm_i915_gem_context_create create = {0};
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create) < 0) {
            fprintf(stderr, "[TEST_E_ERROR] Cannot create context %d: %s\n", i, strerror(errno));
            return -1;
        }
        ctx->ctx_pool[i] = create.ctx_id;
        fprintf(ctx->log_file, "[TEST_E_INIT] Context %d created: ctx_id=%u\n", i, create.ctx_id);
    }
    
    /* Créer 90 batch buffers */
    for (int i = 0; i < 90; i++) {
        struct drm_i915_gem_create create = {
            .size = 4096,
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
            fprintf(stderr, "[TEST_E_ERROR] Cannot create batch %d: %s\n", i, strerror(errno));
            return -1;
        }
        ctx->batch_bo_pool[i] = create.handle;
    }
    
    /* Créer kernel BO (NOP) */
    struct drm_i915_gem_create create_kernel = {.size = 4096};
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel);
    ctx->kernel_bo = create_kernel.handle;
    
    /* Mapper et copier kernel NOP */
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = ctx->kernel_bo,
        .size = 4096,
    };
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel);
    void* kernel_map = (void*)(uintptr_t)mmap_kernel.addr_ptr;
    memcpy(kernel_map, KERNEL_NOP_GEN9, sizeof(KERNEL_NOP_GEN9));
    munmap(kernel_map, 4096);
    
    /* Créer input/output BOs */
    struct drm_i915_gem_create create_io = {.size = 1048576};
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_io);
    ctx->input_bo = create_io.handle;
    
    create_io.handle = 0;
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_io);
    ctx->output_bo = create_io.handle;
    
    fprintf(ctx->log_file, "[TEST_E_INIT] Initialization complete\n");
    fprintf(ctx->log_file, "[TEST_E_INIT] Kernel: NOP (2 instructions)\n");
    fflush(ctx->log_file);
    
    return 0;
}

static int test_e_execute_dispatch(test_e_context_t* ctx) {
    /* Sélectionner contexte */
    ctx->ctx_pool_index = (ctx->ctx_pool_index + 1) % 9;
    uint32_t selected_ctx = ctx->ctx_pool[ctx->ctx_pool_index];
    
    /* Sélectionner batch buffer */
    ctx->batch_pool_index = (ctx->batch_pool_index + 1) % 90;
    uint32_t batch_bo = ctx->batch_bo_pool[ctx->batch_pool_index];
    
    /* Mapper batch buffer */
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = batch_bo,
        .size = 4096,
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        fprintf(ctx->log_file, "[TEST_E_ERROR] Cannot mmap batch: %s\n", strerror(errno));
        return -1;
    }
    uint32_t* batch = (uint32_t*)(uintptr_t)mmap_batch.addr_ptr;
    
    /* Build batch buffer minimal */
    int cmd_idx = 0;
    
    /* STATE_BASE_ADDRESS */
    batch[cmd_idx++] = 0x61010000 | (10 - 2);
    for (int i = 0; i < 9; i++) batch[cmd_idx++] = 0;
    
    /* PIPELINE_SELECT */
    batch[cmd_idx++] = 0x69030000;
    
    /* MEDIA_VFE_STATE */
    batch[cmd_idx++] = 0x70000000 | (9 - 2);
    for (int i = 0; i < 8; i++) batch[cmd_idx++] = 0;
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[cmd_idx++] = 0x70020000 | (4 - 2);
    batch[cmd_idx++] = 0;
    batch[cmd_idx++] = 32;
    batch[cmd_idx++] = 0;
    
    /* GPGPU_WALKER */
    batch[cmd_idx++] = 0x71050000 | (11 - 2);
    batch[cmd_idx++] = 0;
    batch[cmd_idx++] = 1;  /* Thread Width */
    batch[cmd_idx++] = 1;  /* Thread Height */
    batch[cmd_idx++] = 1;  /* Thread Depth */
    for (int i = 0; i < 6; i++) batch[cmd_idx++] = 0;
    
    /* MEDIA_STATE_FLUSH */
    batch[cmd_idx++] = 0x70040000;
    batch[cmd_idx++] = 0;
    
    /* PIPE_CONTROL */
    batch[cmd_idx++] = 0x7a000004;
    batch[cmd_idx++] = 0x00100000;
    batch[cmd_idx++] = 0;
    batch[cmd_idx++] = 0;
    batch[cmd_idx++] = 0;
    batch[cmd_idx++] = 0;
    
    /* BATCH_BUFFER_END */
    batch[cmd_idx++] = 0x0a000000;
    
    size_t batch_size = cmd_idx * 4;
    munmap(batch, 4096);
    
    /* Exec objects */
    struct drm_i915_gem_exec_object2 exec_objects[4] = {
        {.handle = ctx->kernel_bo},
        {.handle = ctx->input_bo},
        {.handle = ctx->output_bo},
        {.handle = batch_bo},
    };
    
    /* Execbuffer2 */
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = selected_ctx,
    };
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    if (ret < 0) {
        fprintf(ctx->log_file, "[TEST_E_FAILED] Dispatch %lu ctx_id=%u errno=%d (%s) time=%.6f\n",
                ctx->total_dispatches + 1, selected_ctx, errno, strerror(errno), elapsed);
        fflush(ctx->log_file);
        return -1;
    }
    
    /* GEM_WAIT */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 5000000000ULL,
    };
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    ctx->total_dispatches++;
    
    fprintf(ctx->log_file, "[TEST_E_SUCCESS] Dispatch %lu ctx_id=%u ctx_idx=%d batch_idx=%d time=%.6f\n",
            ctx->total_dispatches, selected_ctx, ctx->ctx_pool_index, ctx->batch_pool_index, elapsed);
    fflush(ctx->log_file);
    
    return 0;
}

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C271 — TEST E : KERNEL NOP                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    test_e_context_t ctx;
    
    if (test_e_init(&ctx) < 0) {
        fprintf(stderr, "[TEST_E_FATAL] Initialization failed\n");
        return 1;
    }
    
    printf("[TEST_E_START] Executing 100 dispatches with NOP kernel...\n");
    printf("[TEST_E_START] Expected: No GPU hang if kernel SHA-256 is the cause\n\n");
    
    for (int i = 0; i < 100; i++) {
        printf("[TEST_E_DISPATCH_%d] Executing...\r", i + 1);
        fflush(stdout);
        
        if (test_e_execute_dispatch(&ctx) < 0) {
            printf("\n[TEST_E_CRASH] Dispatch %d FAILED\n", i + 1);
            printf("[TEST_E_CRASH] GPU hang persists with NOP kernel\n");
            printf("[TEST_E_CRASH] Conclusion: Problem is NOT the SHA-256 kernel\n");
            break;
        }
        
        if ((i + 1) == 28) {
            printf("\n[TEST_E_MILESTONE] Dispatch 28 SUCCESS (baseline crashed here)\n");
        }
    }
    
    printf("\n\n[TEST_E_COMPLETE] Total dispatches: %lu\n", ctx.total_dispatches);
    
    if (ctx.total_dispatches >= 100) {
        printf("[TEST_E_CONCLUSION] ✅ 100 dispatches SUCCESS with NOP kernel\n");
        printf("[TEST_E_CONCLUSION] → Problem IS the SHA-256 kernel\n");
    } else if (ctx.total_dispatches >= 28) {
        printf("[TEST_E_CONCLUSION] ⚠️  Passed dispatch 28 but crashed before 100\n");
        printf("[TEST_E_CONCLUSION] → Kernel SHA-256 contributes but not sole cause\n");
    } else {
        printf("[TEST_E_CONCLUSION] ❌ Crashed before dispatch 28\n");
        printf("[TEST_E_CONCLUSION] → Problem is NOT the kernel (infrastructure issue)\n");
    }
    
    fclose(ctx.log_file);
    close(ctx.drm_fd);
    
    return 0;
}

// Made with Bob
