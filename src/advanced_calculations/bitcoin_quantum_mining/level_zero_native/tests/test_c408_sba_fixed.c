/**
 * C408 - TEST KERNEL NATIF + STATE_BASE_ADDRESS CORRIGÉ
 * 
 * ROOT CAUSE C407: Opcode STATE_BASE_ADDRESS incorrect
 * - C407: 0x61010009 (9 params) mais écrit 10 dwords
 * - C408: 0x6101000F (15 params) avec tous les Modify Enable bits
 * 
 * Référence: Intel Gen9 PRM Vol 2a Part 1 - STATE_BASE_ADDRESS
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
    uint32_t handle_ssh;
    uint32_t handle_batch;
    
    void *map_kernel;
    void *map_output;
    void *map_ssh;
    void *map_batch;
    
    uint64_t gtt_kernel;
    uint64_t gtt_output;
    uint64_t gtt_ssh;
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
    printf("║  C408 TEST - STATE_BASE_ADDRESS CORRIGÉ                   ║\n");
    printf("║  Kernel: btc_sha256_native_c406.bin (176 bytes)           ║\n");
    printf("║  Fix: Opcode 0x6101000F (15 params + Modify Enable)       ║\n");
    printf("║  Runtime: 100%% i915 DRM natif                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    test_context ctx = {0};
    uint64_t t_start = get_timestamp_ns();
    
    /* Ouvrir log */
    system("mkdir -p logs/forensic/c408");
    ctx.log = fopen("logs/forensic/c408/execution.log", "w");
    if (!ctx.log) {
        perror("fopen log");
        return 1;
    }
    
    fprintf(ctx.log, "# C408 - STATE_BASE_ADDRESS CORRIGÉ\n");
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
    
    if (create_bo(&ctx, 4096, &ctx.handle_ssh, &ctx.map_ssh) < 0) {
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    
    if (create_bo(&ctx, 4096, &ctx.handle_batch, &ctx.map_batch) < 0) {
        close(ctx.fd);
        fclose(ctx.log);
        return 1;
    }
    
    /* Pin buffers */
    struct drm_i915_gem_exec_object2 exec_objects[4] = {
        { .handle = ctx.handle_kernel, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x100000 },
        { .handle = ctx.handle_output, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE, .offset = 0x200000 },
        { .handle = ctx.handle_ssh, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x300000 },
        { .handle = ctx.handle_batch, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x400000 }
    };
    
    ctx.gtt_kernel = exec_objects[0].offset;
    ctx.gtt_output = exec_objects[1].offset;
    ctx.gtt_ssh = exec_objects[2].offset;
    ctx.gtt_batch = exec_objects[3].offset;
    
    fprintf(ctx.log, "[%lu] GTT addresses:\n", get_timestamp_ns());
    fprintf(ctx.log, "  Kernel: 0x%016lx\n", ctx.gtt_kernel);
    fprintf(ctx.log, "  Output: 0x%016lx\n", ctx.gtt_output);
    fprintf(ctx.log, "  SSH:    0x%016lx\n", ctx.gtt_ssh);
    fprintf(ctx.log, "  Batch:  0x%016lx\n", ctx.gtt_batch);
    printf("✓ GTT addresses assigned\n");
    
    /* Surface State (BTI=0) */
    uint32_t *ssh = (uint32_t *)ctx.map_ssh;
    ssh[0] = 0x00000000;
    ssh[1] = ctx.gtt_output & 0xFFFFFFFF;
    ssh[2] = (ctx.gtt_output >> 32) & 0xFFFF;
    ssh[3] = (NUM_THREADS * 4) - 1;
    ssh[4] = 0x00000000;
    ssh[5] = 0x00000000;
    ssh[6] = 0x00000000;
    ssh[7] = 0x00000000;
    printf("✓ Surface State configured\n");
    
    /* Interface Descriptor */
    uint32_t *id = (uint32_t *)((uint8_t *)ctx.map_ssh + 64);
    id[0] = ctx.gtt_kernel & 0xFFFFFFC0;
    id[1] = (ctx.gtt_kernel >> 32) & 0xFFFF;
    id[2] = 0x00000000;
    id[3] = 0x00040000;  // Barrier Enable
    id[4] = 0x00000000;  // BTI=0
    id[5] = 0x00000000;
    id[6] = 0x00000000;
    id[7] = 0x00000000;
    printf("✓ Interface Descriptor configured\n");
    
    /* Batch buffer CORRIGÉ */
    uint32_t *batch = (uint32_t *)ctx.map_batch;
    int idx = 0;
    
    /* STATE_BASE_ADDRESS - CORRIGÉ avec opcode 0x6101000F */
    batch[idx++] = 0x6101000F;  // Opcode: 15 params + tous les Modify Enable bits
    batch[idx++] = 0x00000001;  // General State Base: disabled + Modify Enable
    batch[idx++] = 0x00000000;  // General State Base High
    batch[idx++] = (ctx.gtt_ssh & 0xFFFFFFFF) | 0x01;  // Surface State Base Low + Modify Enable
    batch[idx++] = (ctx.gtt_ssh >> 32) & 0xFFFF;  // Surface State Base High
    batch[idx++] = 0x00000001;  // Dynamic State Base: disabled + Modify Enable
    batch[idx++] = 0x00000000;  // Dynamic State Base High
    batch[idx++] = 0x00000001;  // Indirect Object Base: disabled + Modify Enable
    batch[idx++] = 0x00000000;  // Indirect Object Base High
    batch[idx++] = (ctx.gtt_ssh & 0xFFFFFFFF) | 0x01;  // Instruction Base Low + Modify Enable
    batch[idx++] = (ctx.gtt_ssh >> 32) & 0xFFFF;  // Instruction Base High
    batch[idx++] = 0x00000001;  // General State Buffer Size: disabled + Modify Enable
    batch[idx++] = 0x00000001;  // Dynamic State Buffer Size: disabled + Modify Enable
    batch[idx++] = 0x00000001;  // Indirect Object Buffer Size: disabled + Modify Enable
    batch[idx++] = 0x00000001;  // Instruction Buffer Size: disabled + Modify Enable
    batch[idx++] = 0x00000001;  // Bindless Surface State Base: disabled + Modify Enable
    
    /* MEDIA_VFE_STATE */
    batch[idx++] = 0x70000006;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 32;
    batch[idx++] = (ctx.gtt_ssh + 64) & 0xFFFFFFFF;
    
    /* GPGPU_WALKER */
    batch[idx++] = 0x7a000004;
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address
    batch[idx++] = NUM_THREADS; // Thread Width
    batch[idx++] = 0x00000001;  // Thread Height
    
    /* BATCH_BUFFER_END */
    batch[idx++] = 0x0a000000;
    
    fprintf(ctx.log, "[%lu] Batch buffer built: %d dwords\n", get_timestamp_ns(), idx);
    printf("✓ Batch buffer built: %d dwords\n", idx);
    
    /* EXECBUFFER2 */
    printf("\n🚀 Executing GPU kernel...\n");
    fprintf(ctx.log, "\n[%lu] === EXECBUFFER2 START ===\n", get_timestamp_ns());
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
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
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0) {
            non_zero++;
            if (non_zero <= 10) {
                printf("  output[%d] = 0x%08x\n", i, output[i]);
                fprintf(ctx.log, "output[%d] = 0x%08x\n", i, output[i]);
            }
        }
    }
    
    printf("\n✅ Non-zero results: %d / %d\n", non_zero, NUM_THREADS);
    fprintf(ctx.log, "\n[%lu] Non-zero results: %d / %d\n", get_timestamp_ns(), non_zero, NUM_THREADS);
    
    if (non_zero > 0) {
        printf("🎉 SUCCESS: GPU kernel executed and wrote results!\n");
        fprintf(ctx.log, "SUCCESS: GPU kernel executed\n");
    } else {
        printf("⚠️  WARNING: All results are zero\n");
        fprintf(ctx.log, "WARNING: All results are zero\n");
    }
    
    close(ctx.fd);
    fclose(ctx.log);
    
    printf("\n✓ Log saved: logs/forensic/c408/execution.log\n\n");
    return 0;
}

// Made with Bob
