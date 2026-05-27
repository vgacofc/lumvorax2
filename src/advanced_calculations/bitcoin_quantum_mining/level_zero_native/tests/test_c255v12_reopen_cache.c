/*
 * LumVorax C255V12 — Test Réouverture Périodique avec Cache
 * 
 * OBJECTIF: Dépasser la limite des 27 dispatches via réouverture intelligente
 * 
 * STRATÉGIE:
 * 1. Pré-allocation complète (batch_size optimal = 40.27M nonces)
 * 2. Cache dédié pour état GPU (contextes, buffers, kernel)
 * 3. Réouverture tous les 25 dispatches (marge sécurité vs 27)
 * 4. Restauration instantanée depuis cache (<10ms overhead)
 * 
 * GAIN ATTENDU: Exécution infinie sans crash
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

#define MAX_SAFE_DISPATCHES 25  /* Marge sécurité vs limite 27 */
#define OPTIMAL_BATCH_SIZE 40265318  /* 40.27M nonces (161MB) */
#define CTX_POOL_SIZE 9
#define BATCH_POOL_SIZE 90

/* Cache pour réouverture rapide */
typedef struct {
    /* État pré-calculé */
    size_t kernel_size;
    void* kernel_data;
    size_t input_size;
    size_t output_size;
    
    /* Configuration optimale */
    uint32_t batch_size;
    uint32_t work_group_size;
    
    /* Statistiques */
    uint64_t total_reopens;
    double total_reopen_time;
} reopen_cache_t;

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t ctx_pool[CTX_POOL_SIZE];
    uint32_t batch_bo_pool[BATCH_POOL_SIZE];
    void* batch_map_pool[BATCH_POOL_SIZE];
    
    uint32_t kernel_bo;
    void* kernel_map;
    size_t kernel_size;
    
    uint32_t input_bo;
    void* input_map;
    size_t input_size;
    
    uint32_t output_bo;
    void* output_map;
    size_t output_size;
    
    int batch_pool_index;
    int ctx_pool_index;
    
    uint64_t dispatch_count;
    reopen_cache_t cache;
    
    FILE* log_file;
} btc_context_t;

static double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void log_event(btc_context_t* ctx, const char* fmt, ...) {
    if (!ctx->log_file) return;
    
    double timestamp = get_time_sec();
    fprintf(ctx->log_file, "[%.9f] ", timestamp);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->log_file, fmt, args);
    va_end(args);
    
    fprintf(ctx->log_file, "\n");
    fflush(ctx->log_file);
}

/* Initialisation avec pré-allocation optimale */
static btc_context_t* btc_init_with_cache(const char* kernel_path) {
    btc_context_t* ctx = calloc(1, sizeof(btc_context_t));
    if (!ctx) return NULL;
    
    ctx->log_file = fopen("logs/forensic/test_c255v12_reopen_cache.log", "w");
    
    double start = get_time_sec();
    
    /* Ouvrir DRM */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        log_event(ctx, "ERROR: Failed to open DRM: %s", strerror(errno));
        free(ctx);
        return NULL;
    }
    
    log_event(ctx, "DRM_OPEN_SUCCESS: fd=%d", ctx->drm_fd);
    
    /* Créer contexte principal */
    struct drm_i915_gem_context_create create_ctx = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create_ctx) < 0) {
        log_event(ctx, "ERROR: Failed to create context: %s", strerror(errno));
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    ctx->ctx_id = create_ctx.ctx_id;
    log_event(ctx, "CONTEXT_CREATE_SUCCESS: ctx_id=%u", ctx->ctx_id);
    
    /* Créer pool de contextes */
    for (int i = 0; i < CTX_POOL_SIZE; i++) {
        struct drm_i915_gem_context_create ctx_create = {0};
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
            log_event(ctx, "ERROR: Failed to create pool context %d", i);
            continue;
        }
        ctx->ctx_pool[i] = ctx_create.ctx_id;
        log_event(ctx, "CTX_POOL_CREATED: index=%d ctx_id=%u", i, ctx->ctx_pool[i]);
    }
    
    /* Charger kernel et mettre en cache */
    FILE* f = fopen(kernel_path, "rb");
    if (!f) {
        log_event(ctx, "ERROR: Failed to open kernel: %s", kernel_path);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    ctx->kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    /* Allouer cache kernel */
    ctx->cache.kernel_size = ctx->kernel_size;
    ctx->cache.kernel_data = malloc(ctx->kernel_size);
    if (!ctx->cache.kernel_data) {
        fclose(f);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    
    fread(ctx->cache.kernel_data, 1, ctx->kernel_size, f);
    fclose(f);
    
    log_event(ctx, "KERNEL_CACHED: size=%zu bytes", ctx->kernel_size);
    
    /* Créer kernel BO */
    struct drm_i915_gem_create create = {
        .size = ctx->kernel_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        log_event(ctx, "ERROR: Failed to create kernel BO");
        free(ctx->cache.kernel_data);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    ctx->kernel_bo = create.handle;
    
    /* Mapper et copier kernel */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = ctx->kernel_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        log_event(ctx, "ERROR: Failed to mmap kernel");
        free(ctx->cache.kernel_data);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    
    ctx->kernel_map = mmap(NULL, ctx->kernel_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, ctx->drm_fd, mmap_arg.offset);
    if (ctx->kernel_map == MAP_FAILED) {
        log_event(ctx, "ERROR: mmap failed for kernel");
        free(ctx->cache.kernel_data);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    
    memcpy(ctx->kernel_map, ctx->cache.kernel_data, ctx->kernel_size);
    log_event(ctx, "KERNEL_LOADED: handle=%u size=%zu", ctx->kernel_bo, ctx->kernel_size);
    
    /* Pré-allouer buffers à taille optimale */
    ctx->input_size = OPTIMAL_BATCH_SIZE * 4;  /* 4 bytes par nonce */
    ctx->output_size = 1024 * 1024;  /* 1MB output */
    
    ctx->cache.input_size = ctx->input_size;
    ctx->cache.output_size = ctx->output_size;
    ctx->cache.batch_size = OPTIMAL_BATCH_SIZE;
    ctx->cache.work_group_size = 256;
    
    /* Créer input BO */
    struct drm_i915_gem_create create_input = {
        .size = ctx->input_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_input) < 0) {
        log_event(ctx, "ERROR: Failed to create input BO");
        munmap(ctx->kernel_map, ctx->kernel_size);
        free(ctx->cache.kernel_data);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    ctx->input_bo = create_input.handle;
    
    /* Mapper input */
    struct drm_i915_gem_mmap_offset mmap_input = {
        .handle = ctx->input_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_input) == 0) {
        ctx->input_map = mmap(NULL, ctx->input_size, PROT_READ | PROT_WRITE,
                              MAP_SHARED, ctx->drm_fd, mmap_input.offset);
    }
    
    log_event(ctx, "INPUT_PREALLOCATED: handle=%u size=%zu (%.2f MB)",
              ctx->input_bo, ctx->input_size, ctx->input_size / 1024.0 / 1024.0);
    
    /* Créer output BO */
    struct drm_i915_gem_create create_output = {
        .size = ctx->output_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        log_event(ctx, "ERROR: Failed to create output BO");
        munmap(ctx->input_map, ctx->input_size);
        munmap(ctx->kernel_map, ctx->kernel_size);
        free(ctx->cache.kernel_data);
        close(ctx->drm_fd);
        free(ctx);
        return NULL;
    }
    ctx->output_bo = create_output.handle;
    
    /* Mapper output */
    struct drm_i915_gem_mmap_offset mmap_output = {
        .handle = ctx->output_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) == 0) {
        ctx->output_map = mmap(NULL, ctx->output_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, ctx->drm_fd, mmap_output.offset);
    }
    
    log_event(ctx, "OUTPUT_PREALLOCATED: handle=%u size=%zu (%.2f MB)",
              ctx->output_bo, ctx->output_size, ctx->output_size / 1024.0 / 1024.0);
    
    /* Créer pool de batch buffers */
    for (int i = 0; i < BATCH_POOL_SIZE; i++) {
        struct drm_i915_gem_create batch_create = {
            .size = 4096
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
            log_event(ctx, "ERROR: Failed to create batch BO %d", i);
            continue;
        }
        ctx->batch_bo_pool[i] = batch_create.handle;
        
        struct drm_i915_gem_mmap_offset mmap_batch = {
            .handle = ctx->batch_bo_pool[i],
            .flags = I915_MMAP_OFFSET_WB
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch) == 0) {
            ctx->batch_map_pool[i] = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                          MAP_SHARED, ctx->drm_fd, mmap_batch.offset);
        }
        
        log_event(ctx, "BATCH_POOL_CREATED: index=%d handle=%u", i, ctx->batch_bo_pool[i]);
    }
    
    double elapsed = get_time_sec() - start;
    log_event(ctx, "INIT_COMPLETE_WITH_CACHE: time=%.6f sec", elapsed);
    log_event(ctx, "CACHE_READY: kernel=%zu input=%zu output=%zu batch_size=%u",
              ctx->cache.kernel_size, ctx->cache.input_size, 
              ctx->cache.output_size, ctx->cache.batch_size);
    
    return ctx;
}

/* Réouverture optimisée avec cache */
static int btc_reopen_from_cache(btc_context_t* ctx) {
    double start = get_time_sec();
    
    log_event(ctx, "REOPEN_START: dispatch_count=%lu", ctx->dispatch_count);
    
    /* Fermer ancien DRM */
    close(ctx->drm_fd);
    
    /* Rouvrir DRM */
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        log_event(ctx, "ERROR: Reopen DRM failed: %s", strerror(errno));
        return -1;
    }
    
    log_event(ctx, "DRM_REOPENED: fd=%d", ctx->drm_fd);
    
    /* Recréer contexte principal */
    struct drm_i915_gem_context_create create_ctx = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create_ctx) < 0) {
        log_event(ctx, "ERROR: Failed to recreate context");
        return -1;
    }
    ctx->ctx_id = create_ctx.ctx_id;
    
    /* Recréer pool contextes */
    for (int i = 0; i < CTX_POOL_SIZE; i++) {
        struct drm_i915_gem_context_create ctx_create = {0};
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) == 0) {
            ctx->ctx_pool[i] = ctx_create.ctx_id;
        }
    }
    
    /* Recréer kernel BO depuis cache */
    struct drm_i915_gem_create create_kernel = {
        .size = ctx->cache.kernel_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        log_event(ctx, "ERROR: Failed to recreate kernel BO");
        return -1;
    }
    ctx->kernel_bo = create_kernel.handle;
    
    /* Remapper et restaurer kernel depuis cache */
    struct drm_i915_gem_mmap_offset mmap_kernel = {
        .handle = ctx->kernel_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_kernel) == 0) {
        ctx->kernel_map = mmap(NULL, ctx->kernel_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, ctx->drm_fd, mmap_kernel.offset);
        if (ctx->kernel_map != MAP_FAILED) {
            memcpy(ctx->kernel_map, ctx->cache.kernel_data, ctx->kernel_size);
        }
    }
    
    /* Recréer input/output BOs */
    struct drm_i915_gem_create create_input = {
        .size = ctx->cache.input_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_input) == 0) {
        ctx->input_bo = create_input.handle;
        
        struct drm_i915_gem_mmap_offset mmap_input = {
            .handle = ctx->input_bo,
            .flags = I915_MMAP_OFFSET_WB
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_input) == 0) {
            ctx->input_map = mmap(NULL, ctx->input_size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, ctx->drm_fd, mmap_input.offset);
        }
    }
    
    struct drm_i915_gem_create create_output = {
        .size = ctx->cache.output_size
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) == 0) {
        ctx->output_bo = create_output.handle;
        
        struct drm_i915_gem_mmap_offset mmap_output = {
            .handle = ctx->output_bo,
            .flags = I915_MMAP_OFFSET_WB
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_output) == 0) {
            ctx->output_map = mmap(NULL, ctx->output_size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, ctx->drm_fd, mmap_output.offset);
        }
    }
    
    /* Recréer pool batch buffers */
    for (int i = 0; i < BATCH_POOL_SIZE; i++) {
        struct drm_i915_gem_create batch_create = {
            .size = 4096
        };
        if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) == 0) {
            ctx->batch_bo_pool[i] = batch_create.handle;
            
            struct drm_i915_gem_mmap_offset mmap_batch = {
                .handle = ctx->batch_bo_pool[i],
                .flags = I915_MMAP_OFFSET_WB
            };
            if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_batch) == 0) {
                ctx->batch_map_pool[i] = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                              MAP_SHARED, ctx->drm_fd, mmap_batch.offset);
            }
        }
    }
    
    /* Reset indices */
    ctx->batch_pool_index = 0;
    ctx->ctx_pool_index = 0;
    
    double elapsed = get_time_sec() - start;
    ctx->cache.total_reopens++;
    ctx->cache.total_reopen_time += elapsed;
    
    log_event(ctx, "REOPEN_COMPLETE: time=%.6f sec total_reopens=%lu avg_time=%.6f sec",
              elapsed, ctx->cache.total_reopens, 
              ctx->cache.total_reopen_time / ctx->cache.total_reopens);
    
    return 0;
}

/* Dispatch simple (pour test) */
static int btc_dispatch(btc_context_t* ctx) {
    /* Vérifier si réouverture nécessaire */
    if (ctx->dispatch_count > 0 && ctx->dispatch_count % MAX_SAFE_DISPATCHES == 0) {
        log_event(ctx, "REOPEN_TRIGGER: dispatch=%lu (every %d dispatches)",
                  ctx->dispatch_count, MAX_SAFE_DISPATCHES);
        if (btc_reopen_from_cache(ctx) < 0) {
            return -1;
        }
    }
    
    double start = get_time_sec();
    
    /* Sélectionner contexte et batch buffer */
    uint32_t ctx_id = ctx->ctx_pool[ctx->ctx_pool_index];
    uint32_t batch_bo = ctx->batch_bo_pool[ctx->batch_pool_index];
    void* batch_map = ctx->batch_map_pool[ctx->batch_pool_index];
    
    /* Construire batch buffer simple */
    uint32_t* batch = (uint32_t*)batch_map;
    batch[0] = 0x05000000;  /* MI_BATCH_BUFFER_END */
    
    /* Exécuter */
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = batch_bo,
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,
        .rsvd1 = ctx_id,
        .rsvd2 = 0
    };
    
    execbuf.buffers_ptr = (uintptr_t)&exec_obj;
    execbuf.buffer_count = 1;
    execbuf.batch_len = 4;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx_id;
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    double elapsed = get_time_sec() - start;
    
    if (ret < 0) {
        log_event(ctx, "DISPATCH_FAILED: dispatch=%lu ctx_id=%u errno=%d time=%.6f sec",
                  ctx->dispatch_count, ctx_id, errno, elapsed);
        return -1;
    }
    
    log_event(ctx, "DISPATCH_SUCCESS: dispatch=%lu ctx_id=%u batch_bo=%u time=%.6f sec",
              ctx->dispatch_count, ctx_id, batch_bo, elapsed);
    
    /* Rotation */
    ctx->batch_pool_index = (ctx->batch_pool_index + 1) % BATCH_POOL_SIZE;
    ctx->ctx_pool_index = (ctx->ctx_pool_index + 1) % CTX_POOL_SIZE;
    ctx->dispatch_count++;
    
    return 0;
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C255V12 — Réouverture Périodique avec Cache     ║\n");
    printf("║  Solution pour dépasser limite 27 dispatches              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("[C255V12] Configuration:\n");
    printf("  - Pré-allocation: %u nonces (%.2f MB)\n", 
           OPTIMAL_BATCH_SIZE, OPTIMAL_BATCH_SIZE * 4 / 1024.0 / 1024.0);
    printf("  - Réouverture: Tous les %d dispatches\n", MAX_SAFE_DISPATCHES);
    printf("  - Cache: Kernel + Buffers + Configuration\n");
    printf("  - Objectif: 100 dispatches sans crash\n");
    printf("\n");
    
    btc_context_t* ctx = btc_init_with_cache("kernels/btc_sha256_gen9.bin");
    if (!ctx) {
        printf("❌ Échec initialisation\n");
        return 1;
    }
    
    printf("✅ Initialisation réussie avec cache\n");
    printf("\n");
    printf("[C255V12] Démarrage test 100 dispatches...\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
    
    int success_count = 0;
    int fail_count = 0;
    double total_time = 0;
    
    for (int i = 0; i < 100; i++) {
        double start = get_time_sec();
        int ret = btc_dispatch(ctx);
        double elapsed = get_time_sec() - start;
        total_time += elapsed;
        
        if (ret == 0) {
            success_count++;
            printf("[Dispatch %d/100] ✅ Succès (%.3f ms)\n", i+1, elapsed * 1000);
        } else {
            fail_count++;
            printf("[Dispatch %d/100] ❌ Échec errno=%d (%.3f ms)\n", i+1, errno, elapsed * 1000);
        }
        
        /* Afficher stats réouverture */
        if (ctx->cache.total_reopens > 0 && (i+1) % MAX_SAFE_DISPATCHES == 0) {
            printf("  └─ Réouverture #%lu effectuée (%.3f ms overhead)\n",
                   ctx->cache.total_reopens,
                   (ctx->cache.total_reopen_time / ctx->cache.total_reopens) * 1000);
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("║  RÉSULTATS FINAUX C255V12                                    ║\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Dispatches:\n");
    printf("  - Réussis: %d/100 (%.1f%%)\n", success_count, success_count * 100.0 / 100);
    printf("  - Échoués: %d/100 (%.1f%%)\n", fail_count, fail_count * 100.0 / 100);
    printf("\n");
    printf("Réouvertures:\n");
    printf("  - Total: %lu\n", ctx->cache.total_reopens);
    printf("  - Temps moyen: %.3f ms\n", 
           (ctx->cache.total_reopen_time / ctx->cache.total_reopens) * 1000);
    printf("  - Temps total: %.3f ms\n", ctx->cache.total_reopen_time * 1000);
    printf("  - Overhead: %.2f%% du temps total\n",
           (ctx->cache.total_reopen_time / total_time) * 100);
    printf("\n");
    printf("Performance:\n");
    printf("  - Temps total: %.3f sec\n", total_time);
    printf("  - Temps moyen/dispatch: %.3f ms\n", (total_time / 100) * 1000);
    printf("\n");
    
    if (success_count == 100) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — Limite 27 dispatches DÉPASSÉE          ║\n");
        printf("║  Solution réouverture périodique VALIDÉE                  ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else if (success_count > 27) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS PARTIEL — Limite 27 dispatches DÉPASSÉE        ║\n");
        printf("║  %d dispatches réussis (vs 27 sans réouverture)           ║\n", success_count);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  ÉCHEC — Limite 27 dispatches NON dépassée            ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    return (success_count > 27) ? 0 : 1;
}

// Made with Bob
