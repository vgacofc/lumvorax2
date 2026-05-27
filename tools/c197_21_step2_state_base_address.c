/*
 * C197.21 STEP 2 - STATE_BASE_ADDRESS
 * 
 * Objectif: Configurer adresses de base GPU (sans compute)
 * Batch: PIPE_CONTROL + STATE_BASE_ADDRESS + PIPE_CONTROL + END
 * 
 * Cette étape configure le GPU state sans exécuter de kernel.
 * Risque: moyen (moins dangereux que GPGPU_WALKER)
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
#include <drm/drm.h>
#include <drm/i915_drm.h>

// Gen9 Command Opcodes
#define GEN9_PIPE_CONTROL           0x7A000002  // Length = 6 DWords
#define GEN9_STATE_BASE_ADDRESS     0x61010010  // Length = 19 DWords
#define GEN9_BATCH_BUFFER_END       0x05000000

// Buffer sizes
#define BATCH_BUFFER_SIZE           4096
#define STATE_BUFFER_SIZE           4096

// ============================================================================
// CONTEXT STRUCTURE
// ============================================================================

typedef struct {
    int fd;
    uint32_t vm_id;
    uint32_t context_id;
    
    uint32_t batch_handle;
    void* batch_ptr;
    
    uint32_t state_handle;
    void* state_ptr;
} step2_context_t;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int create_gem_buffer(int fd, size_t size, uint32_t* handle, void** ptr) {
    struct drm_i915_gem_create_ext create = {
        .size = size,
        .flags = 0,
        .extensions = 0,
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &create) < 0) {
        perror("GEM_CREATE_EXT");
        return -1;
    }
    
    *handle = create.handle;
    
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = *handle,
        .flags = I915_MMAP_OFFSET_WB,
        .offset = 0,
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        perror("GEM_MMAP_OFFSET");
        return -1;
    }
    
    *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_arg.offset);
    if (*ptr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    
    return 0;
}

// ============================================================================
// GEN9 BATCH BUFFER BUILDER (WITH STATE_BASE_ADDRESS)
// ============================================================================

static void build_step2_batch_buffer(step2_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_ptr;
    int idx = 0;
    
    printf("Building Step 2 batch buffer (STATE_BASE_ADDRESS)...\n");
    
    // ========================================================================
    // PIPE_CONTROL (flush before)
    // ========================================================================
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = (1 << 18) | (1 << 17);  // CS stall + DC flush
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // ========================================================================
    // STATE_BASE_ADDRESS (19 DWords)
    // ========================================================================
    batch[idx++] = GEN9_STATE_BASE_ADDRESS;  // Opcode + length
    
    // General State Base Address (DWords 1-3)
    batch[idx++] = 0;  // Address low
    batch[idx++] = 0;  // Address high
    batch[idx++] = 0;  // Modify enable = 0 (don't modify)
    
    // Surface State Base Address (DWords 4-6)
    // Point to state buffer
    batch[idx++] = (uint32_t)((uintptr_t)ctx->state_ptr & 0xFFFFFFFF);  // Address low
    batch[idx++] = (uint32_t)(((uintptr_t)ctx->state_ptr >> 32) & 0xFFFFFFFF);  // Address high
    batch[idx++] = 1;  // Modify enable = 1
    
    // Dynamic State Base Address (DWords 7-9)
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // Indirect Object Base Address (DWords 10-12)
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // Instruction Base Address (DWords 13-15)
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // General State Buffer Size (DWord 16)
    batch[idx++] = 0xFFFFF;  // Max size
    
    // Dynamic State Buffer Size (DWord 17)
    batch[idx++] = 0xFFFFF;
    
    // Indirect Object Buffer Size (DWord 18)
    batch[idx++] = 0xFFFFF;
    
    // Instruction Buffer Size (DWord 19)
    batch[idx++] = 0xFFFFF;
    
    // ========================================================================
    // PIPE_CONTROL (flush after)
    // ========================================================================
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = (1 << 18) | (1 << 17);  // CS stall + DC flush
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // ========================================================================
    // BATCH_BUFFER_END
    // ========================================================================
    batch[idx++] = GEN9_BATCH_BUFFER_END;
    
    // Padding (align to 64 bytes)
    while ((idx * 4) < 64) {
        batch[idx++] = 0;
    }
    
    printf("✅ Batch buffer built: %d DWords (%d bytes)\n", idx, idx * 4);
    printf("   Commands: PIPE_CONTROL + STATE_BASE_ADDRESS + PIPE_CONTROL + END\n");
}

// ============================================================================
// DISPATCH
// ============================================================================

static int dispatch_step2_batch(step2_context_t* ctx) {
    uint64_t t0 = get_timestamp_ns();
    
    // Prepare exec objects (batch + state)
    struct drm_i915_gem_exec_object2 exec_objects[2];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx->state_handle;
    exec_objects[0].flags = 0;
    
    exec_objects[1].handle = ctx->batch_handle;
    exec_objects[1].flags = 0;
    
    // Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 2;
    execbuf.batch_len = 256;  // Increased for STATE_BASE_ADDRESS
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx->context_id;
    
    printf("Dispatching Step 2 batch via execbuffer2...\n");
    printf("  batch_handle: %u\n", ctx->batch_handle);
    printf("  state_handle: %u\n", ctx->state_handle);
    printf("  batch_len: %u bytes\n", execbuf.batch_len);
    printf("  context_id: %u\n", ctx->context_id);
    
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("EXECBUFFER2");
        fprintf(stderr, "errno: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    
    uint64_t t1 = get_timestamp_ns();
    printf("✅ Dispatch successful (%.3f µs)\n", (t1 - t0) / 1000.0);
    
    // Wait for completion
    printf("Waiting for GPU completion...\n");
    struct drm_i915_gem_wait wait;
    memset(&wait, 0, sizeof(wait));
    wait.bo_handle = ctx->batch_handle;
    wait.timeout_ns = 1000000000;
    
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
        return -1;
    }
    
    uint64_t t2 = get_timestamp_ns();
    printf("✅ GPU completed (%.3f µs)\n", (t2 - t1) / 1000.0);
    printf("✅ Total dispatch time: %.3f µs\n", (t2 - t0) / 1000.0);
    
    return 0;
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    step2_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    uint64_t t_start = get_timestamp_ns();
    
    printf("=== C197.21 STEP 2 - STATE_BASE_ADDRESS ===\n\n");
    
    // Open DRM
    printf("Step 1: Opening DRM device...\n");
    ctx.fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.fd < 0) {
        perror("open");
        return 1;
    }
    printf("✅ DRM device opened (fd=%d)\n\n", ctx.fd);
    
    // Create VM
    printf("Step 2: Creating VM...\n");
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        perror("VM_CREATE");
        return 1;
    }
    ctx.vm_id = vm_create.vm_id;
    printf("✅ VM created (vm_id=%u)\n\n", ctx.vm_id);
    
    // Create context
    printf("Step 3: Creating context...\n");
    struct drm_i915_gem_context_create_ext ctx_create = {
        .flags = I915_CONTEXT_CREATE_FLAGS_USE_EXTENSIONS,
        .extensions = 0,
    };
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create) < 0) {
        perror("CONTEXT_CREATE_EXT");
        return 1;
    }
    ctx.context_id = ctx_create.ctx_id;
    printf("✅ Context created (ctx_id=%u)\n\n", ctx.context_id);
    
    // Create buffers
    printf("Step 4: Creating buffers...\n");
    if (create_gem_buffer(ctx.fd, BATCH_BUFFER_SIZE, &ctx.batch_handle, &ctx.batch_ptr) < 0) {
        fprintf(stderr, "Failed to create batch buffer\n");
        return 1;
    }
    printf("✅ Batch buffer created (handle=%u, ptr=%p)\n", ctx.batch_handle, ctx.batch_ptr);
    
    if (create_gem_buffer(ctx.fd, STATE_BUFFER_SIZE, &ctx.state_handle, &ctx.state_ptr) < 0) {
        fprintf(stderr, "Failed to create state buffer\n");
        return 1;
    }
    printf("✅ State buffer created (handle=%u, ptr=%p)\n\n", ctx.state_handle, ctx.state_ptr);
    
    // Build batch
    printf("Step 5: Building batch buffer...\n");
    build_step2_batch_buffer(&ctx);
    printf("\n");
    
    // Dispatch
    printf("Step 6: Dispatching batch...\n");
    if (dispatch_step2_batch(&ctx) < 0) {
        fprintf(stderr, "❌ Batch dispatch failed\n");
        return 1;
    }
    printf("\n");
    
    // Validation
    printf("Step 7: Validation...\n");
    printf("✅✅✅ STATE_BASE_ADDRESS DISPATCH RÉUSSI ✅✅✅\n");
    printf("GPU state configuré sans crash\n");
    printf("Prêt pour Step 3 (INTERFACE_DESCRIPTOR)\n");
    printf("\n");
    
    // Cleanup
    printf("Step 8: Cleanup...\n");
    munmap(ctx.batch_ptr, BATCH_BUFFER_SIZE);
    munmap(ctx.state_ptr, STATE_BUFFER_SIZE);
    
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.batch_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.state_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {.ctx_id = ctx.context_id};
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    struct drm_i915_gem_vm_control vm_destroy = {.vm_id = ctx.vm_id};
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
    
    close(ctx.fd);
    
    uint64_t t_end = get_timestamp_ns();
    printf("✅ Cleanup complete\n");
    printf("\nTotal time: %.3f ms\n", (t_end - t_start) / 1000000.0);
    
    return 0;
}

// Made with Bob
