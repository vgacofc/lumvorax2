/*
 * C197.21 - Gen9 Minimal Kernel Dispatch
 * 
 * Objectif: Prouver UNE exécution GPU réelle avec kernel trivial
 * Test: buffer[0] = 0x12345678 (écrit par GPU)
 * 
 * Architecture:
 * 1. Setup DRM (réutilise C197.19)
 * 2. Créer batch buffer avec GPGPU_WALKER
 * 3. Construire structures Gen9 (INTERFACE_DESCRIPTOR, BINDING_TABLE, SURFACE_STATE)
 * 4. Dispatcher via execbuffer2
 * 5. Attendre fence
 * 6. Valider: buffer[0] == 0x12345678
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
#include <xf86drm.h>
#include <i915_drm.h>

// ============================================================================
// GEN9 STRUCTURES (Intel Graphics PRM Vol 7)
// ============================================================================

// Gen9 Command Opcodes
#define GEN9_PIPE_CONTROL           0x7A000000
#define GEN9_STATE_BASE_ADDRESS     0x61010000
#define GEN9_MEDIA_INTERFACE_LOAD   0x70020000
#define GEN9_GPGPU_WALKER           0x71050000
#define GEN9_BATCH_BUFFER_END       0x05000000

// Gen9 Surface Types
#define GEN9_SURFACE_BUFFER         0
#define GEN9_SURFACE_1D             1
#define GEN9_SURFACE_2D             2
#define GEN9_SURFACE_3D             3

// Gen9 Surface Formats
#define GEN9_FORMAT_RAW             0x1FF  // Raw buffer

// Batch buffer size
#define BATCH_BUFFER_SIZE           4096
#define STATE_BUFFER_SIZE           4096
#define KERNEL_BUFFER_SIZE          4096

// ============================================================================
// CONTEXT STRUCTURE
// ============================================================================

typedef struct {
    int fd;
    uint32_t vm_id;
    uint32_t context_id;
    
    // Output buffer (GPU writes here)
    uint32_t output_handle;
    void* output_ptr;
    size_t output_size;
    
    // Batch buffer
    uint32_t batch_handle;
    void* batch_ptr;
    
    // State buffer (interface descriptor, binding table, surface state)
    uint32_t state_handle;
    void* state_ptr;
    
    // Kernel buffer (ISA code)
    uint32_t kernel_handle;
    void* kernel_ptr;
} gen9_context_t;

// ============================================================================
// GEN9 KERNEL ISA (TRIVIAL WRITE)
// ============================================================================

// Kernel Gen9 ISA: MOV r0, 0x12345678; SEND (write to buffer)
// Simplifié pour test minimal
static const uint32_t gen9_kernel_isa[] = {
    // Thread payload setup
    0x00000000,  // NOP
    0x00000000,  // NOP
    
    // MOV r1.0<1>:ud 0x12345678:ud
    0x01000000 | (1 << 18),  // MOV instruction
    0x12345678,               // Immediate value
    
    // SEND (write to surface)
    0x05000000,  // SEND instruction
    0x00000000,  // Descriptor
    
    // EOT (End of Thread)
    0x07000000,  // EOT
    0x00000000,  // Padding
};

#define KERNEL_ISA_SIZE (sizeof(gen9_kernel_isa))

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
        .flags = I915_MMAP_OFFSET_WB,  // Write-back caching
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
// GEN9 BATCH BUFFER BUILDER
// ============================================================================

static void build_gen9_batch_buffer(gen9_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_ptr;
    uint32_t* state = (uint32_t*)ctx->state_ptr;
    int batch_idx = 0;
    int state_idx = 0;
    
    printf("Building Gen9 batch buffer...\n");
    
    // ========================================================================
    // PART 1: PIPE_CONTROL (flush caches)
    // ========================================================================
    batch[batch_idx++] = GEN9_PIPE_CONTROL | (6 - 2);  // Length = 6 DWords
    batch[batch_idx++] = (1 << 18) | (1 << 17);  // CS stall + DC flush
    batch[batch_idx++] = 0;  // Address low
    batch[batch_idx++] = 0;  // Address high
    batch[batch_idx++] = 0;  // Immediate data low
    batch[batch_idx++] = 0;  // Immediate data high
    
    // ========================================================================
    // PART 2: STATE_BASE_ADDRESS (setup state buffer)
    // ========================================================================
    batch[batch_idx++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);  // Length = 19 DWords
    
    // General State Base Address
    batch[batch_idx++] = 0;  // Address low
    batch[batch_idx++] = 0;  // Address high
    batch[batch_idx++] = 0;  // Modify enable
    
    // Surface State Base Address (points to state buffer)
    batch[batch_idx++] = (uint32_t)(uintptr_t)ctx->state_ptr;  // Address low
    batch[batch_idx++] = 0;  // Address high
    batch[batch_idx++] = 1;  // Modify enable
    
    // Dynamic State Base Address
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    
    // Indirect Object Base Address
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    
    // Instruction Base Address (points to kernel buffer)
    batch[batch_idx++] = (uint32_t)(uintptr_t)ctx->kernel_ptr;  // Address low
    batch[batch_idx++] = 0;  // Address high
    batch[batch_idx++] = 1;  // Modify enable
    
    // Padding
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    
    // ========================================================================
    // PART 3: BUILD STATE STRUCTURES
    // ========================================================================
    
    // SURFACE_STATE (64 bytes = 16 DWords)
    // Describes output buffer
    state[state_idx++] = (GEN9_SURFACE_BUFFER << 29) | (GEN9_FORMAT_RAW << 18);
    state[state_idx++] = (uint32_t)(uintptr_t)ctx->output_ptr;  // Base address
    state[state_idx++] = (ctx->output_size - 1);  // Width (size - 1)
    state[state_idx++] = 0;  // Height
    state[state_idx++] = 0;  // Depth
    state[state_idx++] = 0;  // Surface pitch
    for (int i = 0; i < 10; i++) state[state_idx++] = 0;  // Padding
    
    // BINDING_TABLE (1 entry = 1 DWord)
    uint32_t binding_table_offset = state_idx * 4;
    state[state_idx++] = 0;  // Points to SURFACE_STATE at offset 0
    
    // INTERFACE_DESCRIPTOR_DATA (32 bytes = 8 DWords)
    uint32_t interface_descriptor_offset = state_idx * 4;
    state[state_idx++] = 0;  // Kernel start pointer (offset in kernel buffer)
    state[state_idx++] = 0;  // Sampler state pointer
    state[state_idx++] = (binding_table_offset << 5) | 1;  // Binding table pointer + count
    state[state_idx++] = 0;  // Constant URB entry read length
    state[state_idx++] = (1 << 16);  // Number of threads in group (1)
    state[state_idx++] = 0;  // Shared local memory size
    state[state_idx++] = 0;  // Barrier enable
    state[state_idx++] = 0;  // Rounding mode
    
    // ========================================================================
    // PART 4: MEDIA_INTERFACE_DESCRIPTOR_LOAD
    // ========================================================================
    batch[batch_idx++] = GEN9_MEDIA_INTERFACE_LOAD | (4 - 2);  // Length = 4 DWords
    batch[batch_idx++] = 0;  // Reserved
    batch[batch_idx++] = 32;  // Interface descriptor length (32 bytes)
    batch[batch_idx++] = interface_descriptor_offset;  // Interface descriptor offset
    
    // ========================================================================
    // PART 5: GPGPU_WALKER (dispatch kernel)
    // ========================================================================
    batch[batch_idx++] = GEN9_GPGPU_WALKER | (11 - 2);  // Length = 11 DWords
    batch[batch_idx++] = 0;  // Interface descriptor offset (use loaded)
    batch[batch_idx++] = 0;  // Indirect data length
    batch[batch_idx++] = 0;  // Indirect data start address
    
    // Thread group dimensions (1x1x1 = 1 thread)
    batch[batch_idx++] = 0;  // Thread width X (0 = 1 thread)
    batch[batch_idx++] = 0;  // Thread height Y
    batch[batch_idx++] = 0;  // Thread depth Z
    
    // Thread group start
    batch[batch_idx++] = 0;  // Thread group start X
    batch[batch_idx++] = 0;  // Thread group start Y
    batch[batch_idx++] = 0;  // Thread group start Z
    
    batch[batch_idx++] = 0;  // Right execution mask
    
    // ========================================================================
    // PART 6: PIPE_CONTROL (flush after execution)
    // ========================================================================
    batch[batch_idx++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[batch_idx++] = (1 << 18) | (1 << 17);  // CS stall + DC flush
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    batch[batch_idx++] = 0;
    
    // ========================================================================
    // PART 7: BATCH_BUFFER_END
    // ========================================================================
    batch[batch_idx++] = GEN9_BATCH_BUFFER_END;
    
    printf("Batch buffer built: %d DWords (%d bytes)\n", batch_idx, batch_idx * 4);
}

// ============================================================================
// GEN9 DISPATCH
// ============================================================================

static int dispatch_gen9_kernel(gen9_context_t* ctx) {
    uint64_t t0 = get_timestamp_ns();
    
    // Prepare exec objects
    struct drm_i915_gem_exec_object2 exec_objects[4];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx->output_handle;
    exec_objects[0].flags = EXEC_OBJECT_WRITE;
    
    exec_objects[1].handle = ctx->state_handle;
    exec_objects[1].flags = 0;
    
    exec_objects[2].handle = ctx->kernel_handle;
    exec_objects[2].flags = 0;
    
    exec_objects[3].handle = ctx->batch_handle;
    exec_objects[3].flags = 0;
    
    // Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 4;
    execbuf.batch_len = BATCH_BUFFER_SIZE;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx->context_id;
    
    printf("Dispatching kernel via execbuffer2...\n");
    
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
    wait.bo_handle = ctx->output_handle;
    wait.timeout_ns = 1000000000;  // 1 second
    
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
        return -1;
    }
    
    uint64_t t2 = get_timestamp_ns();
    printf("✅ GPU completed (%.3f µs)\n", (t2 - t1) / 1000.0);
    
    return 0;
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    gen9_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    uint64_t t_start = get_timestamp_ns();
    
    printf("=== C197.21 - Gen9 Minimal Kernel Dispatch ===\n\n");
    
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
    // STEP 4: Create buffers
    // ========================================================================
    printf("Step 4: Creating buffers...\n");
    
    ctx.output_size = 4096;
    if (create_gem_buffer(ctx.fd, ctx.output_size, &ctx.output_handle, &ctx.output_ptr) < 0) {
        fprintf(stderr, "Failed to create output buffer\n");
        return 1;
    }
    printf("✅ Output buffer created (handle=%u, ptr=%p)\n", ctx.output_handle, ctx.output_ptr);
    
    if (create_gem_buffer(ctx.fd, BATCH_BUFFER_SIZE, &ctx.batch_handle, &ctx.batch_ptr) < 0) {
        fprintf(stderr, "Failed to create batch buffer\n");
        return 1;
    }
    printf("✅ Batch buffer created (handle=%u, ptr=%p)\n", ctx.batch_handle, ctx.batch_ptr);
    
    if (create_gem_buffer(ctx.fd, STATE_BUFFER_SIZE, &ctx.state_handle, &ctx.state_ptr) < 0) {
        fprintf(stderr, "Failed to create state buffer\n");
        return 1;
    }
    printf("✅ State buffer created (handle=%u, ptr=%p)\n", ctx.state_handle, ctx.state_ptr);
    
    if (create_gem_buffer(ctx.fd, KERNEL_BUFFER_SIZE, &ctx.kernel_handle, &ctx.kernel_ptr) < 0) {
        fprintf(stderr, "Failed to create kernel buffer\n");
        return 1;
    }
    printf("✅ Kernel buffer created (handle=%u, ptr=%p)\n\n", ctx.kernel_handle, ctx.kernel_ptr);
    
    // ========================================================================
    // STEP 5: Initialize buffers
    // ========================================================================
    printf("Step 5: Initializing buffers...\n");
    
    // Clear output buffer
    memset(ctx.output_ptr, 0, ctx.output_size);
    printf("✅ Output buffer cleared\n");
    
    // Copy kernel ISA
    memcpy(ctx.kernel_ptr, gen9_kernel_isa, KERNEL_ISA_SIZE);
    printf("✅ Kernel ISA copied (%zu bytes)\n", KERNEL_ISA_SIZE);
    
    // Build batch buffer
    build_gen9_batch_buffer(&ctx);
    printf("✅ Batch buffer built\n\n");
    
    // ========================================================================
    // STEP 6: Dispatch kernel
    // ========================================================================
    printf("Step 6: Dispatching kernel...\n");
    if (dispatch_gen9_kernel(&ctx) < 0) {
        fprintf(stderr, "❌ Kernel dispatch failed\n");
        return 1;
    }
    printf("\n");
    
    // ========================================================================
    // STEP 7: Validate results
    // ========================================================================
    printf("Step 7: Validating results...\n");
    uint32_t* output = (uint32_t*)ctx.output_ptr;
    
    printf("Output buffer[0] = 0x%08x\n", output[0]);
    printf("Output buffer[1] = 0x%08x\n", output[1]);
    printf("Output buffer[2] = 0x%08x\n", output[2]);
    printf("Output buffer[3] = 0x%08x\n", output[3]);
    
    if (output[0] == 0x12345678) {
        printf("\n✅✅✅ GPU EXECUTION VALIDÉE ✅✅✅\n");
        printf("GPU a écrit 0x12345678 dans buffer[0]\n");
    } else {
        printf("\n❌ GPU execution échouée\n");
        printf("Attendu: 0x12345678, Reçu: 0x%08x\n", output[0]);
    }
    
    // ========================================================================
    // STEP 8: Cleanup
    // ========================================================================
    printf("\nStep 8: Cleanup...\n");
    
    munmap(ctx.output_ptr, ctx.output_size);
    munmap(ctx.batch_ptr, BATCH_BUFFER_SIZE);
    munmap(ctx.state_ptr, STATE_BUFFER_SIZE);
    munmap(ctx.kernel_ptr, KERNEL_BUFFER_SIZE);
    
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.output_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.batch_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.state_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.kernel_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {.ctx_id = ctx.context_id};
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    struct drm_i915_gem_vm_control vm_destroy = {.vm_id = ctx.vm_id};
    ioctl(ctx.fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
    
    close(ctx.fd);
    
    uint64_t t_end = get_timestamp_ns();
    printf("✅ Cleanup complete\n");
    printf("\nTotal time: %.3f ms\n", (t_end - t_start) / 1000000.0);
    
    return (output[0] == 0x12345678) ? 0 : 1;
}

// Made with Bob
