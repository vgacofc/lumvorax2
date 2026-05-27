/*
 * C197.22 - GPU EXECUTION PROOF WITH REAL ISA
 * 
 * Objectif: PROUVER exécution GPU réelle avec kernel ISA extrait d'OpenCL
 * Kernel: write_magic (écrit 0x12345678 dans output[0])
 * Validation: assert(output[0] == 0x12345678) - PREUVE GPU
 * 
 * ISA Source: /tmp/kernel_isa_raw.bin (256 bytes Gen9 ISA)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <drm/drm.h>
#include <drm/i915_drm.h>

// Gen9 Command Opcodes
#define GEN9_PIPE_CONTROL                   0x7A000002
#define GEN9_STATE_BASE_ADDRESS             0x61010010
#define GEN9_MEDIA_VFE_STATE                0x70000007  // Configure compute engine
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020002
#define GEN9_GPGPU_WALKER                   0x75020008
#define GEN9_BATCH_BUFFER_END               0x05000000

// Buffer sizes
#define BATCH_BUFFER_SIZE           16384
#define STATE_BUFFER_SIZE           16384
#define KERNEL_BUFFER_SIZE          4096
#define OUTPUT_BUFFER_SIZE          4096

// Magic values
#define MAGIC_INIT                  0xDEADBEEF
#define MAGIC_EXPECTED              0x12345678

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
    
    uint32_t kernel_handle;
    void* kernel_ptr;
    size_t kernel_size;
    
    uint32_t output_handle;
    void* output_ptr;
} c197_22_context_t;

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
// LOAD REAL GEN9 ISA FROM FILE
// ============================================================================

static int load_kernel_isa(c197_22_context_t* ctx, const char* isa_path) {
    printf("Loading kernel ISA from: %s\n", isa_path);
    
    // Open ISA file
    int fd = open(isa_path, O_RDONLY);
    if (fd < 0) {
        perror("open ISA file");
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }
    
    ctx->kernel_size = st.st_size;
    printf("  ISA size: %zu bytes\n", ctx->kernel_size);
    
    // Read ISA into kernel buffer
    ssize_t bytes_read = read(fd, ctx->kernel_ptr, ctx->kernel_size);
    if (bytes_read != (ssize_t)ctx->kernel_size) {
        perror("read ISA");
        close(fd);
        return -1;
    }
    
    close(fd);
    
    printf("✅ Kernel ISA loaded: %zu bytes\n", ctx->kernel_size);
    
    // Dump first 64 bytes
    printf("  First 64 bytes (hex):\n  ");
    uint8_t* isa = (uint8_t*)ctx->kernel_ptr;
    for (size_t i = 0; i < 64 && i < ctx->kernel_size; i++) {
        printf("%02x", isa[i]);
        if ((i + 1) % 16 == 0) printf("\n  ");
        else if ((i + 1) % 4 == 0) printf(" ");
    }
    printf("\n");
    
    return 0;
}

// ============================================================================
// BUILD STATE BUFFER (INTERFACE_DESCRIPTOR + BINDING TABLE + SURFACE STATE)
// ============================================================================

static void build_state_buffer(c197_22_context_t* ctx) {
    uint32_t* state = (uint32_t*)ctx->state_ptr;
    int idx = 0;
    
    printf("Building state buffer (complete compute state)...\n");
    
    uint64_t kernel_addr = (uintptr_t)ctx->kernel_ptr;
    uint64_t output_addr = (uintptr_t)ctx->output_ptr;
    
    // ========================================================================
    // INTERFACE_DESCRIPTOR_DATA (8 DWords = 32 bytes) @ offset 0
    // ========================================================================
    printf("  [0x%04x] INTERFACE_DESCRIPTOR_DATA\n", idx * 4);
    state[idx++] = (uint32_t)(kernel_addr & 0xFFFFFFFF);         // Kernel start (low)
    state[idx++] = (uint32_t)((kernel_addr >> 32) & 0xFFFFFFFF);  // Kernel start (high)
    state[idx++] = 0;  // Sampler state pointer
    state[idx++] = (32 << 16) | 1;  // Binding table pointer (offset 32) + count (1 entry)
    state[idx++] = 0;  // Constant URB entry read length
    state[idx++] = 1;  // Number of threads = 1
    state[idx++] = 0;  // Shared local memory size
    state[idx++] = 0;  // Barrier enable + rounding mode
    
    // ========================================================================
    // BINDING TABLE (1 entry = 1 DWord) @ offset 32
    // ========================================================================
    printf("  [0x%04x] BINDING_TABLE (1 entry)\n", idx * 4);
    state[idx++] = 64;  // Surface state offset (64 bytes from start)
    
    // Padding to align to 64 bytes
    while ((idx * 4) < 64) {
        state[idx++] = 0;
    }
    
    // ========================================================================
    // SURFACE_STATE (16 DWords = 64 bytes) @ offset 64
    // ========================================================================
    printf("  [0x%04x] SURFACE_STATE (output buffer)\n", idx * 4);
    
    // DWord 0: Surface type + format
    state[idx++] = (0 << 29) |  // Surface type: SURFTYPE_BUFFER
                   (6 << 18) |  // Surface format: RAW
                   (1 << 10);   // Surface array
    
    // DWord 1: Base address (low)
    state[idx++] = (uint32_t)(output_addr & 0xFFFFFFFF);
    
    // DWord 2: Width (buffer size - 1)
    state[idx++] = (OUTPUT_BUFFER_SIZE - 1) & 0x7F;
    
    // DWord 3: Height + Depth
    state[idx++] = (((OUTPUT_BUFFER_SIZE - 1) >> 7) & 0x3FFF) |
                   (((OUTPUT_BUFFER_SIZE - 1) >> 21) << 16);
    
    // DWord 4: Base address (high)
    state[idx++] = (uint32_t)((output_addr >> 32) & 0xFFFF);
    
    // DWord 5-15: Reserved/unused
    for (int i = 0; i < 11; i++) {
        state[idx++] = 0;
    }
    
    printf("✅ State buffer built: %d bytes\n", idx * 4);
    printf("   Kernel address: 0x%016lx\n", kernel_addr);
    printf("   Output address: 0x%016lx\n", output_addr);
    printf("   Binding table: offset 32, 1 entry\n");
    printf("   Surface state: offset 64, 64 bytes\n");
}

// ============================================================================
// GEN9 BATCH BUFFER BUILDER (COMPLETE COMPUTE PIPELINE)
// ============================================================================

static void build_c197_22_batch_buffer(c197_22_context_t* ctx) {
    uint32_t* batch = (uint32_t*)ctx->batch_ptr;
    int idx = 0;
    
    printf("Building C197.22 batch buffer (complete compute pipeline)...\n");
    
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
    batch[idx++] = GEN9_STATE_BASE_ADDRESS;
    
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    // Surface State Base Address (point to state buffer)
    batch[idx++] = (uint32_t)((uintptr_t)ctx->state_ptr & 0xFFFFFFFF);
    batch[idx++] = (uint32_t)(((uintptr_t)ctx->state_ptr >> 32) & 0xFFFFFFFF);
    batch[idx++] = 1;  // Modify enable
    
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    batch[idx++] = 0;
    batch[idx++] = 0;
    batch[idx++] = 0;
    
    batch[idx++] = 0xFFFFF;
    batch[idx++] = 0xFFFFF;
    batch[idx++] = 0xFFFFF;
    batch[idx++] = 0xFFFFF;
    
    // ========================================================================
    // MEDIA_VFE_STATE (8 DWords) - Configure compute engine
    // ========================================================================
    batch[idx++] = GEN9_MEDIA_VFE_STATE;
    batch[idx++] = 0;  // Scratch space base pointer
    batch[idx++] = 0;  // Scratch space base pointer (high)
    batch[idx++] = (1 << 16) | (64 << 8) | 64;  // Max threads + URB entries
    batch[idx++] = 0;  // CURBE allocation size
    batch[idx++] = 0;  // URB entry allocation size
    batch[idx++] = 0;  // Scoreboard mask
    batch[idx++] = 0;  // Scoreboard type
    
    // ========================================================================
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
    // ========================================================================
    batch[idx++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
    batch[idx++] = 0;      // Reserved
    batch[idx++] = 32;     // Interface descriptor length (32 bytes)
    batch[idx++] = 0;      // Interface descriptor offset (start of state buffer)
    
    // ========================================================================
    // GPGPU_WALKER (12 DWords) - Dispatch 1×1×1 threads
    // ========================================================================
    batch[idx++] = GEN9_GPGPU_WALKER;
    batch[idx++] = 0;           // Interface descriptor offset
    batch[idx++] = 0;           // Indirect data length
    batch[idx++] = 0;           // Indirect data start address
    batch[idx++] = 0;           // Thread group ID starting X
    batch[idx++] = 1;           // Thread group ID X dimension (1 thread)
    batch[idx++] = 0;           // Thread group ID starting Y
    batch[idx++] = 1;           // Thread group ID Y dimension
    batch[idx++] = 0;           // Thread group ID starting Z
    batch[idx++] = 1;           // Thread group ID Z dimension
    batch[idx++] = 1;           // Right execution mask
    batch[idx++] = 0xFFFFFFFF;  // Bottom execution mask
    
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
    
    // Padding
    while ((idx * 4) < 512) {
        batch[idx++] = 0;
    }
    
    printf("✅ Batch buffer built: %d DWords (%d bytes)\n", idx, idx * 4);
    printf("   Commands: PIPE + STATE_BASE + VFE_STATE + INTERFACE_LOAD + GPGPU_WALKER + PIPE + END\n");
}

// ============================================================================
// DISPATCH
// ============================================================================

static int dispatch_c197_22_batch(c197_22_context_t* ctx) {
    uint64_t t0 = get_timestamp_ns();
    
    struct drm_i915_gem_exec_object2 exec_objects[4];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx->state_handle;
    exec_objects[0].flags = 0;
    
    exec_objects[1].handle = ctx->kernel_handle;
    exec_objects[1].flags = 0;
    
    exec_objects[2].handle = ctx->output_handle;
    exec_objects[2].flags = EXEC_OBJECT_WRITE;
    
    exec_objects[3].handle = ctx->batch_handle;
    exec_objects[3].flags = 0;
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 4;
    execbuf.batch_len = 1024;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx->context_id;
    
    printf("Dispatching C197.22 batch via execbuffer2...\n");
    printf("  batch_handle: %u\n", ctx->batch_handle);
    printf("  state_handle: %u\n", ctx->state_handle);
    printf("  kernel_handle: %u\n", ctx->kernel_handle);
    printf("  output_handle: %u\n", ctx->output_handle);
    printf("  batch_len: %u bytes\n", execbuf.batch_len);
    
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        perror("EXECBUFFER2");
        fprintf(stderr, "errno: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    
    uint64_t t1 = get_timestamp_ns();
    printf("✅ Dispatch successful (%.3f µs)\n", (t1 - t0) / 1000.0);
    
    struct drm_i915_gem_wait wait;
    memset(&wait, 0, sizeof(wait));
    wait.bo_handle = ctx->batch_handle;
    wait.timeout_ns = 10000000000;  // 10 seconds timeout
    
    printf("Waiting for GPU completion (timeout: 10s)...\n");
    
    if (ioctl(ctx->fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
        fprintf(stderr, "⚠️ GPU HANG POSSIBLE\n");
        return -1;
    }
    
    uint64_t t2 = get_timestamp_ns();
    printf("✅ GPU completed (%.3f ms)\n", (t2 - t1) / 1000000.0);
    printf("✅ Total dispatch time: %.3f ms\n", (t2 - t0) / 1000000.0);
    
    return 0;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    c197_22_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    const char* isa_path = (argc > 1) ? argv[1] : "/tmp/kernel_isa_raw.bin";
    
    uint64_t t_start = get_timestamp_ns();
    
    printf("=== C197.22 - GPU EXECUTION PROOF WITH REAL ISA ===\n");
    printf("⚠️ ATTENTION: PREMIÈRE EXÉCUTION GPU AVEC ISA RÉEL\n");
    printf("⚠️ OBJECTIF: PROUVER output[0] == 0x12345678\n\n");
    
    printf("Step 1: Opening DRM device...\n");
    ctx.fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.fd < 0) {
        perror("open");
        return 1;
    }
    printf("✅ DRM device opened (fd=%d)\n\n", ctx.fd);
    
    printf("Step 2: Creating VM...\n");
    struct drm_i915_gem_vm_control vm_create = {0};
    if (ioctl(ctx.fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create) < 0) {
        perror("VM_CREATE");
        return 1;
    }
    ctx.vm_id = vm_create.vm_id;
    printf("✅ VM created (vm_id=%u)\n\n", ctx.vm_id);
    
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
    
    printf("Step 4: Creating buffers...\n");
    if (create_gem_buffer(ctx.fd, BATCH_BUFFER_SIZE, &ctx.batch_handle, &ctx.batch_ptr) < 0) {
        return 1;
    }
    printf("✅ Batch buffer created (handle=%u)\n", ctx.batch_handle);
    
    if (create_gem_buffer(ctx.fd, STATE_BUFFER_SIZE, &ctx.state_handle, &ctx.state_ptr) < 0) {
        return 1;
    }
    printf("✅ State buffer created (handle=%u)\n", ctx.state_handle);
    
    if (create_gem_buffer(ctx.fd, KERNEL_BUFFER_SIZE, &ctx.kernel_handle, &ctx.kernel_ptr) < 0) {
        return 1;
    }
    printf("✅ Kernel buffer created (handle=%u)\n", ctx.kernel_handle);
    
    if (create_gem_buffer(ctx.fd, OUTPUT_BUFFER_SIZE, &ctx.output_handle, &ctx.output_ptr) < 0) {
        return 1;
    }
    printf("✅ Output buffer created (handle=%u)\n\n", ctx.output_handle);
    
    printf("Step 5: Initializing output buffer...\n");
    uint32_t* output = (uint32_t*)ctx.output_ptr;
    output[0] = MAGIC_INIT;
    printf("✅ Output buffer initialized: output[0] = 0x%08X\n\n", output[0]);
    
    printf("Step 6: Loading kernel ISA...\n");
    if (load_kernel_isa(&ctx, isa_path) < 0) {
        fprintf(stderr, "❌ Failed to load kernel ISA\n");
        return 1;
    }
    printf("\n");
    
    printf("Step 7: Building state buffer...\n");
    build_state_buffer(&ctx);
    printf("\n");
    
    printf("Step 8: Building batch buffer...\n");
    build_c197_22_batch_buffer(&ctx);
    printf("\n");
    
    printf("Step 9: Dispatching batch...\n");
    if (dispatch_c197_22_batch(&ctx) < 0) {
        fprintf(stderr, "❌ Batch dispatch failed or GPU hang\n");
        return 1;
    }
    printf("\n");
    
    printf("Step 10: VALIDATION (MOMENT DE VÉRITÉ)...\n");
    printf("Reading output buffer...\n");
    printf("  output[0] = 0x%08X\n", output[0]);
    printf("\n");
    
    if (output[0] == MAGIC_EXPECTED) {
        printf("✅✅✅ SUCCÈS TOTAL ✅✅✅\n");
        printf("GPU a écrit 0x%08X dans output[0]\n", MAGIC_EXPECTED);
        printf("🎉🎉🎉 PREMIÈRE EXÉCUTION GPU RÉELLE PROUVÉE 🎉🎉🎉\n");
        printf("Pipeline DRM natif complet validé avec ISA réel\n");
    } else if (output[0] == MAGIC_INIT) {
        printf("❌ ÉCHEC: Output unchanged (0x%08X)\n", MAGIC_INIT);
        printf("GPU n'a pas exécuté le kernel ou ISA incorrect\n");
        return 1;
    } else {
        printf("⚠️ Valeur inattendue: 0x%08X\n", output[0]);
        printf("Possible corruption ou exécution partielle\n");
        return 1;
    }
    printf("\n");
    
    printf("Step 11: Cleanup...\n");
    munmap(ctx.batch_ptr, BATCH_BUFFER_SIZE);
    munmap(ctx.state_ptr, STATE_BUFFER_SIZE);
    munmap(ctx.kernel_ptr, KERNEL_BUFFER_SIZE);
    munmap(ctx.output_ptr, OUTPUT_BUFFER_SIZE);
    
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.batch_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.state_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.kernel_handle;
    ioctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.output_handle;
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
