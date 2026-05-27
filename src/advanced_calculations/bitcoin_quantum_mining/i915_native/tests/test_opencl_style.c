/**
 * LumVorax C198 Phase 15Y-F21 — TEST STYLE OPENCL
 * 
 * OBJECTIF: Reproduire EXACTEMENT l'approche OpenCL
 * MÉTHODE: 
 * - 10 objets GEM (9 buffers auxiliaires + 1 batch)
 * - flags = 0x18 sur objets (PINNED + 48B_ADDRESS)
 * - flags = 0x800 sur execbuffer2 (NO_RELOC)
 * - Pas de relocations (adresses GPU directes)
 * - Batch buffer minimal mais correct
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
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BOLD    "\033[1m"

/* Flags i915 découverts via OpenCL */
#define EXEC_OBJECT_PINNED              (1 << 4)  /* 0x10 */
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1 << 3)  /* 0x08 */
#define I915_EXEC_NO_RELOC              (1 << 11) /* 0x800 */

#define GEN9_MI_BATCH_BUFFER_END    0x0A000000
#define GEN9_PIPE_CONTROL           0x7A000000
#define GEN9_MI_NOOP                0x00000000

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    
    /* 10 objets GEM comme OpenCL */
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
    
    /* Créer contexte DRM */
    struct drm_i915_gem_context_create create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create) != 0) {
        fprintf(stderr, COLOR_RED "❌ CONTEXT_CREATE failed\n" COLOR_RESET);
        return -1;
    }
    ctx->ctx_id = create.ctx_id;
    
    /* Créer 10 objets GEM (9 buffers auxiliaires + 1 batch) */
    for (int i = 0; i < 10; i++) {
        struct drm_i915_gem_create gem_create = { .size = 4096 };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) != 0) {
            fprintf(stderr, COLOR_RED "❌ GEM_CREATE[%d] failed\n" COLOR_RESET, i);
            return -1;
        }
        ctx->handles[i] = gem_create.handle;
        
        /* Mapper buffer */
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
        
        /* Initialiser buffer (zéros pour buffers auxiliaires) */
        memset(ctx->ptrs[i], 0, 4096);
        
        /* Obtenir adresse GPU via SET_DOMAIN + GET_OFFSET */
        struct drm_i915_gem_set_domain set_domain = {
            .handle = ctx->handles[i],
            .read_domains = I915_GEM_DOMAIN_GTT,
            .write_domain = I915_GEM_DOMAIN_GTT
        };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
        
        /* Note: L'adresse GPU sera assignée par le kernel lors de l'exec */
        ctx->gpu_addrs[i] = 0;  /* Sera rempli par le kernel */
    }
    
    printf(COLOR_GREEN "✅ Init: ctx_id=%u, 10 objets GEM créés\n" COLOR_RESET, ctx->ctx_id);
    
    return 0;
}

static void build_batch_buffer(test_context_t *ctx) {
    uint32_t *batch = (uint32_t *)ctx->ptrs[9];  /* Dernier objet = batch */
    int idx = 0;
    
    /* PIPE_CONTROL simple (comme OpenCL ligne 77) */
    batch[idx++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[idx++] = (1 << 20);  // CS_STALL
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = GEN9_MI_BATCH_BUFFER_END;
    
    /* Padding avec MI_NOOP */
    while (idx < 16) {
        batch[idx++] = GEN9_MI_NOOP;
    }
}

static int execute_batch(test_context_t *ctx) {
    double t_start = get_timestamp();
    
    /* Construire batch buffer */
    build_batch_buffer(ctx);
    
    /* Préparer 10 exec_objects avec flags OpenCL */
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 10; i++) {
        exec_objects[i].handle = ctx->handles[i];
        exec_objects[i].relocation_count = 0;  /* PAS de relocations comme OpenCL */
        exec_objects[i].relocs_ptr = 0;
        exec_objects[i].alignment = 0;
        exec_objects[i].offset = ctx->gpu_addrs[i];  /* Adresse GPU précédente */
        
        /* PINNED seulement si on a déjà une adresse GPU valide */
        if (ctx->gpu_addrs[i] != 0) {
            exec_objects[i].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS;  /* 0x18 */
        } else {
            exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;  /* 0x08 seulement */
        }
        
        exec_objects[i].rsvd1 = 0;
        exec_objects[i].rsvd2 = 0;
    }
    
    /* Préparer execbuffer2 avec flags OpenCL */
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 10;  /* 10 objets comme OpenCL */
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 64;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;  /* 0x4 | 0x800 = 0x804 */
    execbuf.rsvd1 = ctx->ctx_id;
    
    /* Exécuter */
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        fprintf(stderr, COLOR_RED "❌ EXECBUFFER2 failed: %s (errno=%d)\n" COLOR_RESET,
                strerror(errno), errno);
        ctx->dispatches_failed++;
        return -1;
    }
    
    /* Sauvegarder adresses GPU assignées par le kernel */
    for (int i = 0; i < 10; i++) {
        ctx->gpu_addrs[i] = exec_objects[i].offset;
    }
    
    /* Attendre completion GPU */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx->handles[9],  /* Batch buffer */
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
    printf("║  LumVorax C198 Phase 15Y-F21 — TEST STYLE OPENCL         ║\n");
    printf("║  10 objets GEM + flags 0x18/0x800 + NO_RELOC             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_CYAN "[CONFIG] Dispatches : %d\n" COLOR_RESET, num_dispatches);
    printf(COLOR_CYAN "[CONFIG] Objets GEM : 10 (9 auxiliaires + 1 batch)\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] Flags objets : 0x18 (PINNED + 48B_ADDRESS)\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] Flags execbuf : 0x804 (RENDER + NO_RELOC)\n" COLOR_RESET);
    printf(COLOR_CYAN "[CONFIG] Relocations : AUCUNE (adresses GPU directes)\n" COLOR_RESET);
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
    printf("║  Résultats Test Style OpenCL                              ║\n");
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
        printf("║  🎯 SOLUTION OPENCL VALIDÉE                                ║\n");
        printf("║  Approche multi-objets + flags corrects fonctionne !      ║\n");
        printf("║  Prochaine étape : Intégrer dans production               ║\n");
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
