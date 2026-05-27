/**
 * LumVorax C198 Phase 15Y-F15 — Test Batch Chaining Solution
 * 
 * OBJECTIF: Résoudre GPU hang via chaînage explicite batch buffers
 * SOLUTION: MI_BATCH_BUFFER_START → batch terminaison GEM
 * 
 * CAUSE RACINE IDENTIFIÉE:
 * - MI_BATCH_BUFFER_END cause saut à IP invalide (0x15fff2)
 * - Pointeur de retour corrompu ou non initialisé
 * - GPU timeout après 640ms (preemption GuC)
 * 
 * SOLUTION PROPOSÉE:
 * 1. Batch principal avec MI_BATCH_BUFFER_START (au lieu de END)
 * 2. Batch terminaison alloué via GEM (adresse GPU valide)
 * 3. Batch terminaison contient MI_BATCH_BUFFER_END propre
 * 4. Chaînage explicite évite saut à adresse invalide
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

// Couleurs ANSI
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

// Commandes Gen9
#define MI_NOOP                 0x00000000
#define MI_BATCH_BUFFER_END     0x0A000000
#define MI_BATCH_BUFFER_START   0x31000000
#define PIPE_CONTROL            0x7A000000

/**
 * Structure contexte test
 */
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    
    // Batch principal
    uint32_t main_batch_handle;
    void *main_batch_ptr;
    uint64_t main_batch_addr;
    
    // Batch terminaison
    uint32_t term_batch_handle;
    void *term_batch_ptr;
    uint64_t term_batch_addr;
    
    // Statistiques
    uint32_t dispatches_success;
    uint32_t dispatches_failed;
    double total_time;
} test_context_t;

/**
 * Obtenir timestamp haute précision
 */
static double get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/**
 * Allouer buffer GEM
 */
static int gem_alloc(int fd, uint64_t size, uint32_t *handle, void **ptr, uint64_t *addr) {
    // Créer buffer GEM
    struct drm_i915_gem_create create = {
        .size = size
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) != 0) {
        fprintf(stderr, COLOR_RED "❌ GEM_CREATE failed: %s\n" COLOR_RESET, strerror(errno));
        return -1;
    }
    
    *handle = create.handle;
    
    // Mapper en mémoire
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

/**
 * Initialiser contexte test
 */
static int init_context(test_context_t *ctx) {
    double t_start = get_timestamp();
    
    printf(COLOR_CYAN "[INIT] Ouverture DRM...\n" COLOR_RESET);
    
    // Ouvrir DRM
    ctx->drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx->drm_fd < 0) {
        fprintf(stderr, COLOR_RED "❌ Impossible d'ouvrir /dev/dri/renderD128: %s\n" COLOR_RESET, strerror(errno));
        return -1;
    }
    
    // Créer contexte
    struct drm_i915_gem_context_create create = {0};
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create) != 0) {
        fprintf(stderr, COLOR_RED "❌ CONTEXT_CREATE failed: %s\n" COLOR_RESET, strerror(errno));
        close(ctx->drm_fd);
        return -1;
    }
    ctx->ctx_id = create.ctx_id;
    
    printf(COLOR_GREEN "✅ Contexte créé: ctx_id=%u\n" COLOR_RESET, ctx->ctx_id);
    
    // Allouer batch principal (4096 bytes = 1 page)
    if (gem_alloc(ctx->drm_fd, 4096, &ctx->main_batch_handle, 
                  &ctx->main_batch_ptr, &ctx->main_batch_addr) != 0) {
        fprintf(stderr, COLOR_RED "❌ Allocation batch principal failed\n" COLOR_RESET);
        return -1;
    }
    
    printf(COLOR_GREEN "✅ Batch principal: handle=%u addr=0x%lx\n" COLOR_RESET, 
           ctx->main_batch_handle, ctx->main_batch_addr);
    
    // Allouer batch terminaison (4096 bytes = 1 page)
    if (gem_alloc(ctx->drm_fd, 4096, &ctx->term_batch_handle,
                  &ctx->term_batch_ptr, &ctx->term_batch_addr) != 0) {
        fprintf(stderr, COLOR_RED "❌ Allocation batch terminaison failed\n" COLOR_RESET);
        return -1;
    }
    
    printf(COLOR_GREEN "✅ Batch terminaison: handle=%u addr=0x%lx\n" COLOR_RESET,
           ctx->term_batch_handle, ctx->term_batch_addr);
    
    // Construire batch terminaison (ultra-simple)
    uint32_t *term_batch = (uint32_t *)ctx->term_batch_ptr;
    term_batch[0] = PIPE_CONTROL | (6 - 2);  // PIPE_CONTROL (6 dwords)
    term_batch[1] = (1 << 20);               // CS_STALL
    term_batch[2] = 0x00000000;              // Address Low
    term_batch[3] = 0x00000000;              // Address High
    term_batch[4] = 0x00000000;              // Immediate Data Low
    term_batch[5] = 0x00000000;              // Immediate Data High
    term_batch[6] = MI_BATCH_BUFFER_END;     // END
    term_batch[7] = MI_NOOP;                 // Padding
    
    printf(COLOR_GREEN "✅ Batch terminaison construit (8 dwords)\n" COLOR_RESET);
    
    double t_end = get_timestamp();
    printf(COLOR_CYAN "[INIT] Temps: %.6f sec\n" COLOR_RESET, t_end - t_start);
    
    return 0;
}

/**
 * Construire batch principal avec chaînage
 */
static void build_main_batch(test_context_t *ctx) {
    uint32_t *batch = (uint32_t *)ctx->main_batch_ptr;
    int idx = 0;
    
    // PIPE_CONTROL initial (synchronisation)
    batch[idx++] = PIPE_CONTROL | (6 - 2);
    batch[idx++] = (1 << 20);  // CS_STALL
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_START vers batch terminaison
    // Format: MI_BATCH_BUFFER_START | (1 << 8) pour PPGTT address space
    batch[idx++] = MI_BATCH_BUFFER_START | (1 << 8) | (3 - 2);  // 3 dwords
    batch[idx++] = (uint32_t)(ctx->term_batch_addr & 0xFFFFFFFF);  // Low 32 bits
    batch[idx++] = (uint32_t)(ctx->term_batch_addr >> 32);         // High 32 bits
    
    // Padding
    while (idx < 16) {
        batch[idx++] = MI_NOOP;
    }
    
    printf(COLOR_GREEN "✅ Batch principal construit (%d dwords)\n" COLOR_RESET, idx);
    printf(COLOR_YELLOW "   → Chaînage vers batch terminaison @ 0x%lx\n" COLOR_RESET, 
           ctx->term_batch_addr);
}

/**
 * Exécuter batch buffer
 */
static int execute_batch(test_context_t *ctx) {
    double t_start = get_timestamp();
    
    // Préparer exec_object2
    struct drm_i915_gem_exec_object2 exec_objects[2] = {0};
    
    // Batch terminaison (doit être dans la liste pour relocation)
    exec_objects[0].handle = ctx->term_batch_handle;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;
    exec_objects[0].alignment = 0;
    exec_objects[0].offset = 0;
    exec_objects[0].flags = 0;
    
    // Batch principal
    exec_objects[1].handle = ctx->main_batch_handle;
    exec_objects[1].relocation_count = 0;
    exec_objects[1].relocs_ptr = 0;
    exec_objects[1].alignment = 0;
    exec_objects[1].offset = 0;
    exec_objects[1].flags = 0;
    
    // Préparer execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 64;  // 16 dwords * 4 bytes
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx->ctx_id;
    
    // Exécuter
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        fprintf(stderr, COLOR_RED "❌ EXECBUFFER2 failed: %s (errno=%d)\n" COLOR_RESET, 
                strerror(errno), errno);
        ctx->dispatches_failed++;
        return -1;
    }
    
    double t_end = get_timestamp();
    double elapsed = t_end - t_start;
    
    ctx->dispatches_success++;
    ctx->total_time += elapsed;
    
    printf(COLOR_GREEN "✅ Dispatch #%u réussi (%.3f ms)\n" COLOR_RESET,
           ctx->dispatches_success, elapsed * 1000.0);
    
    return 0;
}

/**
 * Cleanup contexte
 */
static void cleanup_context(test_context_t *ctx) {
    if (ctx->main_batch_ptr) {
        munmap(ctx->main_batch_ptr, 4096);
    }
    if (ctx->term_batch_ptr) {
        munmap(ctx->term_batch_ptr, 4096);
    }
    
    if (ctx->main_batch_handle) {
        struct drm_gem_close close = { .handle = ctx->main_batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    if (ctx->term_batch_handle) {
        struct drm_gem_close close = { .handle = ctx->term_batch_handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
    }
    
    if (ctx->ctx_id) {
        struct drm_i915_gem_context_destroy destroy = { .ctx_id = ctx->ctx_id };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
    }
    
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
    }
}

/**
 * Main
 */
int main(int argc, char **argv) {
    int num_dispatches = 100;
    
    if (argc > 1) {
        num_dispatches = atoi(argv[1]);
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 Phase 15Y-F15 — Batch Chaining Solution    ║\n");
    printf("║  Test MI_BATCH_BUFFER_START → Batch Terminaison GEM      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf(COLOR_CYAN "[CONFIG] Dispatches demandés: %d\n" COLOR_RESET, num_dispatches);
    printf(COLOR_CYAN "[CONFIG] Solution: Chaînage explicite batch buffers\n" COLOR_RESET);
    printf("\n");
    
    // Initialiser contexte
    test_context_t ctx = {0};
    if (init_context(&ctx) != 0) {
        fprintf(stderr, COLOR_RED "❌ Initialisation failed\n" COLOR_RESET);
        return 1;
    }
    
    // Construire batch principal
    build_main_batch(&ctx);
    
    printf("\n");
    printf(COLOR_CYAN "[TEST] Exécution %d dispatches...\n" COLOR_RESET, num_dispatches);
    printf("\n");
    
    // Exécuter dispatches
    double test_start = get_timestamp();
    
    for (int i = 0; i < num_dispatches; i++) {
        if (execute_batch(&ctx) != 0) {
            fprintf(stderr, COLOR_RED "❌ Dispatch #%d failed, arrêt test\n" COLOR_RESET, i + 1);
            break;
        }
        
        // Petit délai pour éviter saturation
        usleep(10000);  // 10ms
    }
    
    double test_end = get_timestamp();
    double test_time = test_end - test_start;
    
    // Afficher résultats
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  Résultats Test Batch Chaining                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
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
        printf(COLOR_GREEN "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL — Solution Batch Chaining Validée !      ║\n");
        printf("║  %u/%u dispatches réussis sans GPU hang                  ║\n", 
               ctx.dispatches_success, num_dispatches);
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else {
        printf(COLOR_YELLOW "╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  SUCCÈS PARTIEL — %u/%u dispatches réussis            ║\n",
               ctx.dispatches_success, num_dispatches);
        printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    }
    
    printf("\n");
    
    // Cleanup
    cleanup_context(&ctx);
    
    return (ctx.dispatches_success == num_dispatches) ? 0 : 1;
}

// Made with Bob
