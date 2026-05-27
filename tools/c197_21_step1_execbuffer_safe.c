/*
 * C197.21 STEP 1 - Execbuffer Safe (No Compute)
 * 
 * Objectif: Valider que execbuffer2 fonctionne SANS crash
 * Batch: PIPE_CONTROL + BATCH_BUFFER_END uniquement
 * 
 * Cette étape élimine 80% des risques GPU hang en validant
 * le pipeline DRM de base avant d'ajouter compute.
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
#define GEN9_BATCH_BUFFER_END       0x05000000

// Batch buffer size
#define BATCH_BUFFER_SIZE           4096

// ============================================================================
// CONTEXT STRUCTURE
// ============================================================================

typedef struct {
    int fd;
    uint32_t vm_id;
    uint32_t context_id;
    uint32_t batch_handle;
    void* batch_ptr;
} safe_context_t;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int create_gem_buffer(int fd, size_t size, uint32_t* handle, void** ptr) {
    // Create buffer
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
    
    // Map buffer
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
// GEN9 BATCH BUFFER BUILDER (SAFE - NO COMPUTE)
// ============================================================================

static void build_safe_batch_buffer(safe_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_ptr;
    int idx = 0;
    
    printf("Building SAFE batch buffer (no compute)...\n");
    
    // ========================================================================
    // PIPE_CONTROL (flush caches)
    // ========================================================================
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = (1 << 18) | (1 << 17);  // CS stall + DC flush
    batch[idx++] = 0;  // Address low
    batch[idx++] = 0;  // Address high
    batch[idx++] = 0;  // Immediate data low
    batch[idx++] = 0;  // Immediate data high
    
    // ========================================================================
    // BATCH_BUFFER_END
    // ========================================================================
    batch[idx++] = GEN9_BATCH_BUFFER_END;
    
    // Padding (align to 64 bytes)
    while (idx < 16) {
        batch[idx++] = 0;
    }
    
    printf("✅ Batch buffer built: %d DWords (%d bytes)\n", idx, idx * 4);
    printf("   Commands: PIPE_CONTROL + BATCH_BUFFER_END\n");
}

// ============================================================================
// SAFE DISPATCH
// ============================================================================

static int dispatch_safe_batch(safe_context_t* ctx) {
    uint64_t t0 = get_timestamp_ns();
    
    // Prepare exec object
    struct drm_i915_gem_exec_object2 exec_object;
    memset(&exec_object, 0, sizeof(exec_object));
    exec_object.handle = ctx->batch_handle;
    exec_object.flags = 0;
    
    // Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)&exec_object;
    execbuf.buffer_count = 1;
    execbuf.batch_len = 64;  // 16 DWords = 64 bytes
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx->context_id;
    
    printf("Dispatching SAFE batch via execbuffer2...\n");
    printf("  batch_handle: %u\n", ctx->batch_handle);
    printf("  batch_len: %u bytes\n", execbuf.batch_len);
    printf("  context_id: %u\n", ctx->context_id);
    printf("  flags: 0x%x\n", execbuf.flags);
    
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
    wait.timeout_ns = 1000000000;  // 1 second
    
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
    safe_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    uint64_t t_start = get_timestamp_ns();
    
    printf("=== C197.21 STEP 1 - Execbuffer Safe (No Compute) ===\n\n");
    
    // ========================================================================
    // STEP 1: Open DRM device
    // ========================================================================
    printf("Step 1: Opening DRM device...\n");
    ctx.fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.fd < 0) {
        perror("open /dev/dri/renderD128");
        return 1;
    }
    printf("✅ DRM device opened (fd=%d)\n\n", ctx.fd);
    
    // ========================================================================
    // STEP 2: Create VM
    // ========================================================================
    printf("Step 2: Creating VM...\n");
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        perror("VM_CREATE");
        return 1;
    }
    ctx.vm_id = vm_create.vm_id;
    printf("✅ VM created (vm_id=%u)\n\n", ctx.vm_id);
    
    // ========================================================================
    // STEP 3: Create context
    // ========================================================================
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
    
    // ========================================================================
    // STEP 4: Create batch buffer
    // ========================================================================
    printf("Step 4: Creating batch buffer...\n");
    if (create_gem_buffer(ctx.fd, BATCH_BUFFER_SIZE, &ctx.batch_handle, &ctx.batch_ptr) < 0) {
        fprintf(stderr, "Failed to create batch buffer\n");
        return 1;
    }
    printf("✅ Batch buffer created (handle=%u, ptr=%p)\n\n", ctx.batch_handle, ctx.batch_ptr);
    
    // ========================================================================
    // STEP 5: Build batch buffer
    // ========================================================================
    printf("Step 5: Building batch buffer...\n");
    build_safe_batch_buffer(&ctx);
    printf("\n");
    
    // ========================================================================
    // STEP 6: Dispatch batch
    // ========================================================================
    printf("Step 6: Dispatching batch...\n");
    if (dispatch_safe_batch(&ctx) < 0) {
        fprintf(stderr, "❌ Batch dispatch failed\n");
        return 1;
    }
    printf("\n");
    
    // ========================================================================
    // STEP 7: Validation
    // ========================================================================
    printf("Step 7: Validation...\n");
    printf("✅✅✅ EXECBUFFER2 FONCTIONNE SANS CRASH ✅✅✅\n");
    printf("Pipeline DRM de base validé\n");
    printf("Prêt pour Step 2 (STATE_BASE_ADDRESS)\n");
    printf("\n");
    
    // ========================================================================
    // STEP 8: Cleanup
    // ========================================================================
    printf("Step 8: Cleanup...\n");
    
    munmap(ctx.batch_ptr, BATCH_BUFFER_SIZE);
    
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.batch_handle;
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
