/*
 * LumVorax C198 Phase 15Y-F13 — Test Batch Niveaux Progressifs
 * 
 * Objectif : Identifier EXACTEMENT quelle commande cause GPU hang
 * Méthode : Test 10000 dispatches par niveau A→F
 * Code root : emmaus
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
    int level;
} test_context_t;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int init_context(test_context_t* ctx) {
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) return -1;
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = ctx_create.ctx_id;
    
    struct drm_i915_gem_create gem_create = { .size = BATCH_SIZE };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        close(ctx->drm_fd);
        return -1;
    }
    ctx->batch_handle = gem_create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = ctx->batch_handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        close(ctx->drm_fd);
        return -1;
    }
    ctx->batch_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    return 0;
}

static void build_batch_level(test_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    memset(batch, 0, BATCH_SIZE);
    int idx = 0;
    
    switch (ctx->level) {
        case 0: // Phase A : END seul avec padding
            batch[idx++] = 0x00000000;  // MI_NOOP
            batch[idx++] = 0x00000000;  // MI_NOOP
            batch[idx++] = 0x0A000000;  // MI_BATCH_BUFFER_END
            break;
            
        case 1: // Phase B : PIPE_CONTROL + END
            batch[idx++] = 0x7A000004;  // PIPE_CONTROL | (6-2)
            batch[idx++] = (1 << 20);   // CS_STALL
            batch[idx++] = 0;
            batch[idx++] = 0;
            batch[idx++] = 0;
            batch[idx++] = 0;
            batch[idx++] = 0x0A000000;  // END
            break;
            
        case 2: // Phase C : STATE_BASE_ADDRESS + END
            batch[idx++] = 0x61010011;  // STATE_BASE_ADDRESS | (19-2)
            for (int i = 0; i < 18; i++) batch[idx++] = 0;
            batch[idx++] = 0x0A000000;  // END
            break;
            
        case 3: // Phase D : MEDIA_VFE_STATE + END
            batch[idx++] = 0x70000007;  // MEDIA_VFE_STATE | (9-2)
            for (int i = 0; i < 8; i++) batch[idx++] = 0;
            batch[idx++] = 0x0A000000;  // END
            break;
            
        case 4: // Phase E : INTERFACE_DESCRIPTOR_LOAD + END
            batch[idx++] = 0x70020000;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD
            batch[idx++] = 0;
            batch[idx++] = 0;
            batch[idx++] = 0;
            batch[idx++] = 0x0A000000;  // END
            break;
            
        case 5: // Phase F : GPGPU_WALKER + END
            batch[idx++] = 0x7105000D;  // GPGPU_WALKER | (15-2)
            for (int i = 0; i < 14; i++) batch[idx++] = 0;
            batch[idx++] = 0x0A000000;  // END
            break;
    }
}

static int execute_batch(test_context_t* ctx) {
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = ctx->batch_handle,
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,  // Correction #13 : Supprimer EXEC_OBJECT_PINNED
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)&exec_obj,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = 64,  // Correction #15 : Taille réaliste
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
    
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = ctx->batch_handle,
        .flags = 0,
        .timeout_ns = 5000000000LL
    };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        return -1;
    }
    
    return 0;
}

static void cleanup(test_context_t* ctx) {
    if (ctx->batch_map) munmap(ctx->batch_map, BATCH_SIZE);
    if (ctx->batch_handle) {
        struct drm_gem_close close_arg = { .handle = ctx->batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    }
    if (ctx->ctx_id) {
        struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx->ctx_id };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    }
    if (ctx->drm_fd >= 0) close(ctx->drm_fd);
}

static int test_level(int level) {
    const char* level_names[] = {
        "Phase A : MI_BATCH_BUFFER_END seul",
        "Phase B : PIPE_CONTROL + END",
        "Phase C : STATE_BASE_ADDRESS + END",
        "Phase D : MEDIA_VFE_STATE + END",
        "Phase E : INTERFACE_DESCRIPTOR_LOAD + END",
        "Phase F : GPGPU_WALKER + END"
    };
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %s\n", level_names[level]);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    test_context_t ctx = { .level = level };
    
    if (init_context(&ctx) < 0) {
        printf("❌ Init failed\n");
        return -1;
    }
    
    build_batch_level(&ctx);
    
    printf("Test %d dispatches...\n", NUM_DISPATCHES);
    double start = get_time_sec();
    int success = 0;
    
    for (int i = 0; i < NUM_DISPATCHES; i++) {
        if (execute_batch(&ctx) == 0) {
            success++;
            if ((i + 1) % 1000 == 0) {
                printf("  [%5d/%d] ✅ OK (%.1f%%)\n", i+1, NUM_DISPATCHES, (i+1) * 100.0 / NUM_DISPATCHES);
            }
        } else {
            printf("  [%5d/%d] ❌ ÉCHEC (errno=%d)\n", i+1, NUM_DISPATCHES, errno);
            break;
        }
    }
    
    double elapsed = get_time_sec() - start;
    
    printf("\nRésultats :\n");
    printf("  Succès : %d/%d (%.1f%%)\n", success, NUM_DISPATCHES, success * 100.0 / NUM_DISPATCHES);
    printf("  Temps  : %.3f sec\n", elapsed);
    printf("  Moyen  : %.3f ms/dispatch\n", elapsed * 1000.0 / success);
    
    cleanup(&ctx);
    
    if (success == NUM_DISPATCHES) {
        printf("\n✅ NIVEAU %d VALIDÉ — Aucun GPU hang\n", level);
        return 0;
    } else {
        printf("\n❌ NIVEAU %d ÉCHOUÉ — GPU hang détecté\n", level);
        printf("🔍 COUPABLE IDENTIFIÉ : %s\n", level_names[level]);
        return -1;
    }
}

int main(int argc, char** argv) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15Y-F13 — Test Niveaux Progressifs  ║\n");
    printf("║  Objectif : Identifier commande exacte qui cause hang    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    int start_level = 0;
    if (argc > 1) {
        start_level = atoi(argv[1]);
        if (start_level < 0 || start_level > 5) start_level = 0;
    }
    
    for (int level = start_level; level <= 5; level++) {
        if (test_level(level) < 0) {
            printf("\n╔════════════════════════════════════════════════════════════╗\n");
            printf("║  ⚠️  ARRÊT — Coupable identifié au niveau %d              ║\n", level);
            printf("╚════════════════════════════════════════════════════════════╝\n");
            
            // Capturer dmesg avec code root emmaus
            printf("\nCapture logs kernel (code root: emmaus)...\n");
            char cmd[256];
            snprintf(cmd, sizeof(cmd),
                     "echo emmaus | sudo -S dmesg | tail -50 > logs/forensic/dmesg_level_%d.txt 2>&1",
                     level);
            system(cmd);
            
            return 1;
        }
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TOUS LES NIVEAUX VALIDÉS                               ║\n");
    printf("║  Aucun GPU hang détecté sur 60000 dispatches             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob
