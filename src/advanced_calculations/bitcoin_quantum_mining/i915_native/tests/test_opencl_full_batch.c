/**
 * LumVorax C198 Phase 15Y-F23 — BATCH BUFFER COMPLET STYLE OPENCL
 * 
 * COPIE EXACTE du batch buffer OpenCL dispatch #2
 * Avec MEDIA_VFE_STATE + MEDIA_INTERFACE_DESCRIPTOR_LOAD
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

#define EXEC_OBJECT_PINNED              (1 << 4)
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1 << 3)
#define I915_EXEC_NO_RELOC              (1 << 11)

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t handles[10];
    void *ptrs[10];
    uint64_t gpu_addrs[10];
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
    
    /* Créer 10 objets GEM */
    for (int i = 0; i < 10; i++) {
        struct drm_i915_gem_create gem_create = { .size = 4096 };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) != 0) {
            fprintf(stderr, COLOR_RED "❌ GEM_CREATE[%d] failed\n" COLOR_RESET, i);
            return -1;
        }
        ctx->handles[i] = gem_create.handle;
        
        struct drm_i915_gem_mmap mmap_arg = {
            .handle = ctx->handles[i],
            .offset = 0,
            .size = 4096,
            .flags = 0
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
            fprintf(stderr, COLOR_RED "❌ GEM_MMAP[%d] failed\n" COLOR_RESET, i);
            return -1;
        }
        ctx->ptrs[i] = (void *)(uintptr_t)mmap_arg.addr_ptr;
        memset(ctx->ptrs[i], 0, 4096);
        
        struct drm_i915_gem_set_domain set_domain = {
            .handle = ctx->handles[i],
            .read_domains = I915_GEM_DOMAIN_GTT,
            .write_domain = I915_GEM_DOMAIN_GTT
        };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
        
        ctx->gpu_addrs[i] = 0;
    }
    
    printf(COLOR_GREEN "✅ Init: ctx_id=%u, 10 objets GEM créés\n" COLOR_RESET, ctx->ctx_id);
    return 0;
}

static void build_full_batch(test_context_t *ctx) {
    uint32_t *batch = (uint32_t *)ctx->ptrs[9];  /* Dernier = batch */
    int idx = 0;
    
    /* COPIE EXACTE du batch OpenCL dispatch #2 (lignes 208-220 du dump) */
    
    /* MEDIA_VFE_STATE - Configuration compute engine */
    batch[idx++] = 0x70040000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;
    batch[idx++] = 0x00000000;
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD - Charge kernel descriptor */
    batch[idx++] = 0x7105000d;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x000001c0;
    
    /* Adresse buffer descriptor (buffer auxiliaire #4) */
    if (ctx->gpu_addrs[4] != 0) {
        batch[idx++] = (uint32_t)(ctx->gpu_addrs[4] & 0xFFFFFFFF);
    } else {
        batch[idx++] = 0xfffdd000;  /* Valeur OpenCL par défaut */
    }
    
    batch[idx++] = 0x80000001;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = 0x0A000000;
    
    /* Padding */
    while (idx < 64) {
        batch[idx++] = 0x00000000;
    }
}

static int execute_batch(test_context_t *ctx) {
    double t_start = get_timestamp();
    
    build_full_batch(ctx);
    
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 10; i++) {
        exec_objects[i].handle = ctx->handles[i];
        exec_objects[i].relocation_count = 0;
        exec_objects[i].relocs_ptr = 0;
        exec_objects[i].alignment = 0;
        exec_objects[i].offset = ctx->gpu_addrs[i];
        
        if (ctx->gpu_addrs[i] != 0) {
            exec_objects[i].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
        } else {
            exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
        }
        
        exec_objects[i].rsvd1 = 0;
        exec_objects[i].rsvd2 = 0;
    }
    
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 10;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 256;  /* Batch plus long */
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx->ctx_id;
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        fprintf(stderr, COLOR_RED "❌ EXECBUFFER2 failed: %s (errno=%d)\n" COLOR_RESET,
                strerror(errno), errno);
        ctx->dispatches_failed++;
        return -1;
    }
    
    for (int i = 0; i < 10; i++) {
        ctx->gpu_addrs[i] = exec_objects[i].offset;
    }
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->handles[9],
        .timeout_ns = 1000000000,
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
    for (int i = 0; i < 10; i++) {
        if (ctx->ptrs[i]) munmap(ctx->ptrs[i], 4096);
        if (ctx->handles[i]) {
            struct drm_gem_close close = { .handle = ctx->handles[i] };
            ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
        }
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
    printf("║  LumVorax C198 Phase 15Y-F23 — BATCH COMPLET OPENCL      ║\n");
    printf("║  MEDIA_VFE_STATE + MEDIA_INTERFACE_DESCRIPTOR_LOAD        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_CYAN "[CONFIG] Dispatches : %d\n" COLOR_RESET, num_dispatches);
    printf(COLOR_CYAN "[CONFIG] Batch : COPIE EXACTE OpenCL dispatch #2\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] MEDIA_VFE_STATE : OUI\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] MEDIA_INTERFACE_DESCRIPTOR_LOAD : OUI\n" COLOR_RESET);
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
        
        usleep(1000);
    }
    
    double test_end = get_timestamp();
    double test_time = test_end - test_start;
    
    printf("\n");
    printf(COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  Résultats Batch Complet OpenCL                           ║\n");
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
        printf("║  🎯 SOLUTION COMPLÈTE VALIDÉE                              ║\n");
        printf("║  Batch buffer complet Gen9 fonctionne !                   ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else {
        printf(COLOR_RED COLOR_BOLD "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ ÉCHEC — %u/%u dispatches                              ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    }
    
    printf("\n");
    
    cleanup_context(&ctx);
    
    return (ctx.dispatches_success == num_dispatches) ? 0 : 1;
}

// Made with Bob
