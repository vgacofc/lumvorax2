/**
 * C342 TEST MINIMAL DIAGNOSTIC - Identifier le vrai bug Gen9 compute state
 * 
 * TESTS PROGRESSIFS:
 * 1. Hardcoded write → test binding/surface state
 * 2. get_global_id() → test CURBE/payload
 * 3. Input read → test input surface
 * 4. Simple calcul → test ALU
 * 5. All threads → test dispatch complet
 * 6. Workgroup ID → test dimensions
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define LOG(fmt, ...) fprintf(stderr, "[C342] " fmt "\n", ##__VA_ARGS__)

typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    
    // Kernel
    uint32_t kernel_bo;
    void* kernel_map;
    size_t kernel_size;
    
    // Buffers
    uint32_t input_bo;
    void* input_map;
    uint32_t output_bo;
    void* output_map;
    
    // Batch buffer
    uint32_t batch_bo;
    void* batch_map;
    
    // Surface State Heap
    uint32_t ssh_bo;
    void* ssh_map;
    
    // Dynamic State Heap
    uint32_t dsh_bo;
    void* dsh_map;
    
    // Interface Descriptor Heap
    uint32_t ioh_bo;
    void* ioh_map;
} test_context_t;

static int alloc_gpu_buffer(test_context_t* ctx, size_t size, uint32_t* bo_out, void** map_out) {
    struct drm_i915_gem_create create = { .size = size };
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        LOG("GEM_CREATE failed: %s", strerror(errno));
        return -1;
    }
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        LOG("GEM_MMAP failed: %s", strerror(errno));
        struct drm_gem_close close_arg = { .handle = create.handle };
        ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        return -1;
    }
    
    *bo_out = create.handle;
    *map_out = (void*)(uintptr_t)mmap_arg.addr_ptr;
    memset(*map_out, 0, size);
    
    LOG("Allocated buffer: handle=%u size=%zu addr=%p", create.handle, size, *map_out);
    return 0;
}

static int load_kernel(test_context_t* ctx, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        LOG("Failed to open kernel: %s", path);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    ctx->kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (alloc_gpu_buffer(ctx, ctx->kernel_size, &ctx->kernel_bo, &ctx->kernel_map) < 0) {
        fclose(f);
        return -1;
    }
    
    fread(ctx->kernel_map, 1, ctx->kernel_size, f);
    fclose(f);
    
    LOG("Loaded kernel: %s size=%zu", path, ctx->kernel_size);
    return 0;
}

static void create_surface_states(test_context_t* ctx) {
    uint32_t* ssh = (uint32_t*)ctx->ssh_map;
    
    // Surface State 0: Input buffer (RAW)
    ssh[0] = 0x87fc0000;  // RAW buffer, 32-bit elements
    ssh[1] = 0;           // Base address (will be relocated)
    ssh[2] = 0x00ffffff;  // Width-1 (16MB)
    ssh[3] = 0;
    ssh[4] = 0;
    ssh[5] = 0;
    ssh[6] = 0;
    ssh[7] = 0;
    
    // Surface State 1: Output buffer (RAW)
    ssh[8] = 0x87fc0000;
    ssh[9] = 0;
    ssh[10] = 0x00ffffff;
    ssh[11] = 0;
    ssh[12] = 0;
    ssh[13] = 0;
    ssh[14] = 0;
    ssh[15] = 0;
    
    LOG("Created surface states");
}

static void create_interface_descriptor(test_context_t* ctx) {
    uint32_t* ioh = (uint32_t*)ctx->ioh_map;
    
    // Interface Descriptor
    ioh[0] = 0;  // Kernel start pointer (will be relocated)
    ioh[1] = 0;
    ioh[2] = 0;  // Sampler state pointer
    ioh[3] = 0x00000040;  // Binding table pointer (offset 64 in SSH)
    ioh[4] = 0;  // CURBE read length
    ioh[5] = 0x00400000;  // Number of threads in GPGPU thread group (256)
    ioh[6] = 0;
    ioh[7] = 0;
    
    LOG("Created interface descriptor");
}

static int run_test(test_context_t* ctx, const char* test_name, int kernel_offset) {
    LOG("========================================");
    LOG("TEST: %s", test_name);
    LOG("========================================");
    
    // Clear output buffer
    memset(ctx->output_map, 0, 4096);
    
    // Build batch buffer
    uint32_t* batch = (uint32_t*)ctx->batch_map;
    int offset = 0;
    
    // PIPELINE_SELECT
    batch[offset++] = 0x69040000;
    batch[offset++] = 0x00000000;
    
    // STATE_BASE_ADDRESS (C304 fix)
    batch[offset++] = 0x61010009;  // Opcode + length
    batch[offset++] = 0x00000001;  // General State Base Address Modify Enable
    batch[offset++] = 0;
    batch[offset++] = 0x00000001;  // Surface State Base Address Modify Enable
    batch[offset++] = 0;           // Surface State Base Address (relocated)
    batch[offset++] = 0x00000001;  // Dynamic State Base Address Modify Enable
    batch[offset++] = 0;           // Dynamic State Base Address (relocated)
    batch[offset++] = 0x00000001;  // Indirect Object Base Address Modify Enable
    batch[offset++] = 0;           // Indirect Object Base Address (relocated)
    batch[offset++] = 0x00000001;  // Instruction Base Address Modify Enable
    batch[offset++] = 0;           // Instruction Base Address (relocated)
    
    // MEDIA_VFE_STATE
    batch[offset++] = 0x70000006;
    batch[offset++] = 0;
    batch[offset++] = 0x00040040;  // Max threads = 64
    batch[offset++] = 0;
    batch[offset++] = 0x00180000;  // CURBE allocation size
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x70020002;
    batch[offset++] = 0;
    batch[offset++] = 0x00000020;  // Interface Descriptor Total Length
    batch[offset++] = 0;           // Interface Descriptor Data Start Address (relocated)
    
    // GPGPU_WALKER (C341 fix: dimensions 256/1/1)
    batch[offset++] = 0x70050000 | 13;  // Opcode + length
    batch[offset++] = 0;                // Interface Descriptor Offset
    batch[offset++] = 0x00000001;       // Indirect Data Length
    batch[offset++] = 0;                // Indirect Data Start Address
    batch[offset++] = 0xffffffff;       // Thread Width Counter Maximum
    batch[offset++] = 0xffffffff;       // Thread Height Counter Maximum
    batch[offset++] = 0xffffffff;       // Thread Depth Counter Maximum
    batch[offset++] = 256;              // Thread Group ID X Dimension (C341 FIX)
    batch[offset++] = 1;                // Thread Group ID Y Dimension (C341 FIX)
    batch[offset++] = 1;                // Thread Group ID Z Dimension (C341 FIX)
    batch[offset++] = 0xffffffff;       // Right Execution Mask
    batch[offset++] = 0xffffffff;       // Bottom Execution Mask
    batch[offset++] = 1;                // Thread Group ID Starting X
    batch[offset++] = 0;                // Thread Group ID Starting Y
    batch[offset++] = 0;                // Thread Group ID Starting Z
    
    // PIPE_CONTROL
    batch[offset++] = 0x7a000004;
    batch[offset++] = 0x00100000;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // BATCH_BUFFER_END
    batch[offset++] = 0x0a000000;
    
    int batch_size = offset * 4;
    LOG("Batch buffer size: %d bytes", batch_size);
    
    // Setup exec_objects
    struct drm_i915_gem_exec_object2 exec_objects[7];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx->kernel_bo;
    exec_objects[0].flags = EXEC_OBJECT_PINNED;
    
    exec_objects[1].handle = ctx->input_bo;
    exec_objects[1].flags = EXEC_OBJECT_PINNED;
    
    exec_objects[2].handle = ctx->output_bo;
    exec_objects[2].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE;
    
    exec_objects[3].handle = ctx->ssh_bo;
    exec_objects[4].handle = ctx->dsh_bo;
    exec_objects[5].handle = ctx->ioh_bo;
    exec_objects[6].handle = ctx->batch_bo;
    
    // Setup relocations
    struct drm_i915_gem_relocation_entry relocs[6];
    memset(relocs, 0, sizeof(relocs));
    
    // Relocation 0: Kernel start pointer in IOH
    relocs[0].target_handle = ctx->kernel_bo;
    relocs[0].delta = kernel_offset;
    relocs[0].offset = 0;  // IOH offset 0
    relocs[0].presumed_offset = 0;
    
    // Relocation 1: Input surface in SSH
    relocs[1].target_handle = ctx->input_bo;
    relocs[1].delta = 0;
    relocs[1].offset = 4;  // SSH offset 4 (dw1 of surface 0)
    relocs[1].presumed_offset = 0;
    
    // Relocation 2: Output surface in SSH
    relocs[2].target_handle = ctx->output_bo;
    relocs[2].delta = 0;
    relocs[2].offset = 36;  // SSH offset 36 (dw1 of surface 1)
    relocs[2].presumed_offset = 0;
    
    // Relocation 3: SSH base address in batch
    relocs[3].target_handle = ctx->ssh_bo;
    relocs[3].delta = 0;
    relocs[3].offset = 12;  // STATE_BASE_ADDRESS dw3
    relocs[3].presumed_offset = 0;
    
    // Relocation 4: DSH base address in batch
    relocs[4].target_handle = ctx->dsh_bo;
    relocs[4].delta = 0;
    relocs[4].offset = 20;  // STATE_BASE_ADDRESS dw5
    relocs[4].presumed_offset = 0;
    
    // Relocation 5: IOH base address in batch
    relocs[5].target_handle = ctx->ioh_bo;
    relocs[5].delta = 0;
    relocs[5].offset = 28;  // STATE_BASE_ADDRESS dw7
    relocs[5].presumed_offset = 0;
    
    exec_objects[6].relocation_count = 6;
    exec_objects[6].relocs_ptr = (uintptr_t)relocs;
    
    // Execute
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 7;
    execbuf.batch_len = batch_size;
    execbuf.flags = I915_EXEC_RENDER;
    execbuf.rsvd1 = ctx->ctx_id;
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        LOG("EXECBUFFER2 failed: %s", strerror(errno));
        return -1;
    }
    
    // Wait for completion
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = ctx->output_bo,
        .timeout_ns = 1000000000,  // 1 second
        .flags = 0
    };
    
    if (ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg) < 0) {
        LOG("GEM_WAIT failed: %s", strerror(errno));
        return -1;
    }
    
    // Read results
    uint32_t* output = (uint32_t*)ctx->output_map;
    LOG("Results:");
    for (int i = 0; i < 16; i++) {
        LOG("  output[%d] = 0x%08x (%u)", i, output[i], output[i]);
    }
    
    return 0;
}

int main(int argc, char** argv) {
    test_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Open DRM device
    ctx.drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.drm_fd < 0) {
        LOG("Failed to open DRM device");
        return 1;
    }
    
    // Create context
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG("Failed to create context");
        close(ctx.drm_fd);
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    LOG("Created context: ctx_id=%u", ctx.ctx_id);
    
    // Load kernel
    if (load_kernel(&ctx, "kernels/test_minimal_c342.bin_kbl.bin") < 0) {
        close(ctx.drm_fd);
        return 1;
    }
    
    // Allocate buffers
    alloc_gpu_buffer(&ctx, 4096, &ctx.input_bo, &ctx.input_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.output_bo, &ctx.output_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.batch_bo, &ctx.batch_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.ssh_bo, &ctx.ssh_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.dsh_bo, &ctx.dsh_map);
    alloc_gpu_buffer(&ctx, 4096, &ctx.ioh_bo, &ctx.ioh_map);
    
    // Initialize input buffer
    uint32_t* input = (uint32_t*)ctx.input_map;
    for (int i = 0; i < 256; i++) {
        input[i] = i * 100;
    }
    
    // Create GPU state
    create_surface_states(&ctx);
    create_interface_descriptor(&ctx);
    
    // Run tests
    run_test(&ctx, "TEST #1: Hardcoded Write", 0);
    run_test(&ctx, "TEST #2: get_global_id()", 1024);
    run_test(&ctx, "TEST #3: Input Read", 2048);
    run_test(&ctx, "TEST #4: Simple Calc", 3072);
    run_test(&ctx, "TEST #5: All Threads", 4096);
    run_test(&ctx, "TEST #6: Workgroup ID", 5120);
    
    LOG("========================================");
    LOG("All tests completed");
    LOG("========================================");
    
    close(ctx.drm_fd);
    return 0;
}

// Made with Bob
