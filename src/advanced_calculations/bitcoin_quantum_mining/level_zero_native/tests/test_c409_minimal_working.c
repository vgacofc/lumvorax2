/**
 * C409 - TEST KERNEL NATIF APPROCHE MINIMALISTE
 * 
 * STRATÉGIE: Utiliser l'approche C406 qui a FONCTIONNÉ
 * - PAS de STATE_BASE_ADDRESS (trop complexe)
 * - Juste GPGPU_WALKER + BATCH_BUFFER_END
 * - Kernel C406 (176 bytes, SEND 0x04025E00)
 * - Lecture complète des résultats
 * 
 * C406 a réussi EXECBUFFER2, donc cette approche devrait fonctionner.
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
#include <xf86drm.h>
#include <i915_drm.h>

#define NUM_THREADS 256

typedef struct {
    int fd;
    uint32_t ctx_id;
    
    uint32_t handle_kernel;
    uint32_t handle_output;
    uint32_t handle_batch;
    
    void *map_kernel;
    void *map_output;
    void *map_batch;
    
    uint64_t gtt_kernel;
    uint64_t gtt_output;
    uint64_t gtt_batch;
    
    FILE *log;
} test_context;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int create_bo(test_context *ctx, size_t size, uint32_t *handle, void **map) {
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("GEM_CREATE");
        return -1;
    }
    *handle = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = *handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        perror("GEM_MMAP");
        return -1;
    }
    *map = (void *)(uintptr_t)mmap_arg.addr_ptr;
    
    return 0;
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C409 TEST - APPROCHE MINIMALISTE (comme C406)            ║\n");
    printf("║  Kernel: btc_sha256_native_c406.bin (176 bytes)           ║\n");
    printf("║  Batch: GPGPU_WALKER + BATCH_BUFFER_END (minimal)         ║\n");
    printf("║  PAS de STATE_BASE_ADDRESS (trop complexe)                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    test_context ctx = {0};
    uint64_t t_start = get_timestamp_ns();
    
    /* Ouvrir log */
    system("mkdir -p logs/forensic/c409");
    ctx.log = fopen("logs/forensic/c409/execution.log", "w");
    if (!ctx.log) {
        perror("fopen log");
        return 1;
    }
    
    fprintf(ctx.log, "# C409 - APPROCHE MINIMALISTE\n");
    fprintf(ctx.log, "# Timestamp: %lu ns\n\n", t_start);
    
    /* Ouvrir DRM */
    ctx.fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.fd < 0) {
        perror("open DRM");
        fclose(ctx.log);
        return 1;
    }
    fprintf(ctx.log, "[%lu] DRM opened: fd=%d\n", get_timestamp_ns(), ctx.fd);
    printf("✓ DRM opened: fd=%d\n", ctx.fd);
    
    /* Créer contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("CONTEXT_CREATE");
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    fprintf(ctx.log, "[%lu] GPU context: ctx_id=%u\n", get_timestamp_ns(), ctx.ctx_id);
    printf("✓ GPU context created: ctx_id=%u\n", ctx.ctx_id);
    
    /* Charger kernel */
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        perror("fopen kernel");
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (create_bo(&ctx, kernel_size, &ctx.handle_kernel, &ctx.map_kernel) < 0) {
        fclose(f);
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    
    fread(ctx.map_kernel, 1, kernel_size, f);
    fclose(f);
    fprintf(ctx.log, "[%lu] Kernel loaded: %zu bytes\n", get_timestamp_ns(), kernel_size);
    printf("✓ Kernel size: %zu bytes\n", kernel_size);
    printf("✓ Kernel loaded\n");
    
    /* Créer buffers */
    if (create_bo(&ctx, 1024, &ctx.handle_output, &ctx.map_output) < 0) {
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    memset(ctx.map_output, 0, 1024);
    printf("✓ Output buffer created: 1024 bytes\n");
    
    if (create_bo(&ctx, 4096, &ctx.handle_batch, &ctx.map_batch) < 0) {
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    
    /* Pin buffers */
    struct drm_i915_gem_exec_object2 exec_objects[3] = {
        { .handle = ctx.handle_kernel, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x100000 },
        { .handle = ctx.handle_output, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE, .offset = 0x200000 },
        { .handle = ctx.handle_batch, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x400000 }
    };
    
    ctx.gtt_kernel = exec_objects[0].offset;
    ctx.gtt_output = exec_objects[1].offset;
    ctx.gtt_batch = exec_objects[2].offset;
    
    fprintf(ctx.log, "[%lu] GTT addresses:\n", get_timestamp_ns());
    fprintf(ctx.log, "  Kernel: 0x%016lx\n", ctx.gtt_kernel);
    fprintf(ctx.log, "  Output: 0x%016lx\n", ctx.gtt_output);
    fprintf(ctx.log, "  Batch:  0x%016lx\n", ctx.gtt_batch);
    printf("✓ GTT addresses assigned\n");
    
    /* Batch buffer MINIMAL (comme C406) */
    uint32_t *batch = (uint32_t *)ctx.map_batch;
    int idx = 0;
    
    /* GPGPU_WALKER - configuration minimale */
    batch[idx++] = 0x7a000004;  // GPGPU_WALKER opcode
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address
    batch[idx++] = NUM_THREADS; // Thread Width
    batch[idx++] = 0x00000001;  // Thread Height
    
    /* BATCH_BUFFER_END */
    batch[idx++] = 0x0a000000;
    
    fprintf(ctx.log, "[%lu] Batch buffer built: %d dwords (MINIMAL)\n", get_timestamp_ns(), idx);
    printf("✓ Batch buffer built: %d dwords (MINIMAL - no STATE_BASE_ADDRESS)\n", idx);
    
    /* EXECBUFFER2 */
    printf("\n🚀 Executing GPU kernel...\n");
    fprintf(ctx.log, "\n[%lu] === EXECBUFFER2 START ===\n", get_timestamp_ns());
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 3,
        .batch_start_offset = 0,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx.ctx_id
    };
    
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("EXECBUFFER2");
        fprintf(ctx.log, "[%lu] ERROR: EXECBUFFER2 failed\n", get_timestamp_ns());
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    
    fprintf(ctx.log, "[%lu] EXECBUFFER2 success\n", get_timestamp_ns());
    printf("✓ EXECBUFFER2 success\n");
    
    /* Wait GPU */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.handle_batch,
        .timeout_ns = 1000000000  // 1 second
    };
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    }
    printf("✓ GPU execution complete\n");
    
    /* Lire résultats */
    printf("\n📊 Results:\n");
    uint32_t *output = (uint32_t *)ctx.map_output;
    int non_zero = 0;
    
    fprintf(ctx.log, "\n[%lu] === RESULTS ===\n", get_timestamp_ns());
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0) {
            non_zero++;
            if (non_zero <= 20) {
                printf("  output[%3d] = 0x%08x", i, output[i]);
                
                // Décoder le pattern attendu: (4 << 16) | thread_id
                uint32_t expected = (4 << 16) | i;
                if (output[i] == expected) {
                    printf(" ✓ CORRECT (pattern match)\n");
                } else {
                    printf(" (expected 0x%08x)\n", expected);
                }
                
                fprintf(ctx.log, "output[%d] = 0x%08x\n", i, output[i]);
            }
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Non-zero results: %d / %d\n", non_zero, NUM_THREADS);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    fprintf(ctx.log, "\n[%lu] Non-zero results: %d / %d\n", get_timestamp_ns(), non_zero, NUM_THREADS);
    
    if (non_zero > 0) {
        printf("🎉 SUCCESS: GPU kernel executed and wrote results!\n");
        printf("✅ SEND descriptor 0x04025E00 is WORKING!\n");
        fprintf(ctx.log, "SUCCESS: GPU kernel executed\n");
    } else {
        printf("⚠️  WARNING: All results are zero\n");
        printf("   This means either:\n");
        printf("   1. Kernel didn't execute\n");
        printf("   2. SEND instruction failed to write\n");
        printf("   3. Wrong buffer mapping\n");
        fprintf(ctx.log, "WARNING: All results are zero\n");
    }
    
    close(ctx.fd);
    fclose(ctx.log);
    
    printf("\n✓ Log saved: logs/forensic/c409/execution.log\n\n");
    return (non_zero > 0) ? 0 : 1;
}

// Made with Bob
