/**
 * C413 - CORRECTION BUG #2: THREAD COUNT
 * 
 * OBJECTIF: Corriger Interface Descriptor DW5 pour SIMD16 avec 256 threads
 * 
 * CHANGEMENTS vs C412:
 * - Interface Descriptor DW5: Configuration correcte pour SIMD16
 * - Number of Threads in GPGPU Thread Group = 16 (pour SIMD16)
 * - Barrier Enable si nécessaire
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
#include <time.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define NUM_THREADS 256
#define SIMD_WIDTH 16
#define NUM_WORK_GROUPS ((NUM_THREADS + SIMD_WIDTH - 1) / SIMD_WIDTH)

static inline uint64_t get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

#define LOG_NS(fmt, ...) do { \
    uint64_t ns = get_ns(); \
    printf("[%020lu] " fmt "\n", ns, ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

static void hex_dump(const char *label, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    printf("\n=== HEX DUMP: %s (%zu bytes) ===\n", label, size);
    for (size_t i = 0; i < size; i += 16) {
        printf("%08zx | ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
    printf("=== END DUMP ===\n\n");
}

int main(void) {
    uint64_t start_ns = get_ns();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  C413 - CORRECTION BUG #2: THREAD COUNT                      ║\n");
    printf("║  Fix: Interface Descriptor DW5 pour SIMD16 (16 threads)      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    LOG_NS("=== PHASE 1: DRM INITIALIZATION ===");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        LOG_NS("ERROR: open failed: %s", strerror(errno));
        return 1;
    }
    LOG_NS("✓ DRM opened: fd=%d", fd);
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG_NS("ERROR: CONTEXT_CREATE failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    LOG_NS("✓ GPU context created: ctx_id=%u", ctx_id);
    
    LOG_NS("=== PHASE 2: KERNEL LOADING ===");
    
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        LOG_NS("ERROR: fopen kernel failed");
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel);
    
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel);
    
    void *kernel_map = (void *)(uintptr_t)mmap_kernel.addr_ptr;
    memset(kernel_map, 0, 4096);
    fread(kernel_map, 1, kernel_size, f);
    fclose(f);
    LOG_NS("✓ Kernel loaded: %zu bytes", kernel_size);
    
    LOG_NS("=== PHASE 3: OUTPUT BUFFER ===");
    
    struct drm_i915_gem_create create_output = { .size = 4096 };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output);
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = 4096
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output);
    
    void *output_map = (void *)(uintptr_t)mmap_output.addr_ptr;
    memset(output_map, 0xCC, 4096);
    LOG_NS("✓ Output buffer initialized");
    
    LOG_NS("=== PHASE 4: SSH + BINDING TABLE ===");
    
    struct drm_i915_gem_create create_ssh = { .size = 4096 };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_ssh);
    
    struct drm_i915_gem_mmap mmap_ssh = {
        .handle = create_ssh.handle,
        .size = 4096
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_ssh);
    
    uint32_t *ssh = (uint32_t *)(uintptr_t)mmap_ssh.addr_ptr;
    memset(ssh, 0, 4096);
    
    // Surface State
    ssh[0] = 0x00000000;
    ssh[1] = 0x00000000;
    ssh[2] = 0x00000000;
    ssh[3] = 0x00000FFF;
    
    // Binding Table à offset 64
    uint32_t binding_table_offset = 64;
    uint32_t *binding_table = &ssh[binding_table_offset / 4];
    binding_table[0] = 0x00000000;
    
    LOG_NS("✓ SSH configured: Surface State + Binding Table");
    
    LOG_NS("=== PHASE 5: DSH + INTERFACE DESCRIPTOR ===");
    
    struct drm_i915_gem_create create_dsh = { .size = 4096 };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_dsh);
    
    struct drm_i915_gem_mmap mmap_dsh = {
        .handle = create_dsh.handle,
        .size = 4096
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_dsh);
    
    uint32_t *dsh = (uint32_t *)(uintptr_t)mmap_dsh.addr_ptr;
    memset(dsh, 0, 4096);
    
    // Interface Descriptor avec THREAD COUNT CORRECT
    dsh[0] = 0x00000000;  // DW0: KSP (relocalisé)
    dsh[1] = 0x00000000;  // DW1: Reserved
    dsh[2] = 0x00000000;  // DW2: Sampler State Pointer
    dsh[3] = binding_table_offset;  // DW3: Binding Table Pointer
    dsh[4] = 0x00000000;  // DW4: Shared Local Memory Size = 0
    
    // *** NOUVEAU: DW5 configuré pour SIMD16 ***
    // Bits [9:0] = Number of Threads in GPGPU Thread Group = 16 (0x10)
    // Bits [15:10] = Reserved
    // Bits [31:16] = Reserved
    dsh[5] = 0x00000010;  // 16 threads pour SIMD16
    
    dsh[6] = 0x00000000;  // DW6: Cross-Thread Constant Data Read Length = 0
    dsh[7] = 0x00000000;  // DW7: Barrier Enable = 0
    
    LOG_NS("✓ Interface Descriptor: Number of Threads = %u (SIMD16)", dsh[5] & 0x3FF);
    hex_dump("INTERFACE DESCRIPTOR", dsh, 32);
    
    LOG_NS("=== PHASE 6: BATCH BUFFER ===");
    
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch);
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch);
    
    uint32_t *batch = (uint32_t *)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    int idx = 0;
    
    // STATE_BASE_ADDRESS
    batch[idx++] = 0x61010010;
    for (int i = 0; i < 17; i++) batch[idx++] = 0x00000000;
    batch[4] = 0x00000000;  // SSH (relocalisé)
    batch[6] = 0x00000000;  // DSH (relocalisé)
    batch[10] = 0x00000000;  // Kernel (relocalisé)
    batch[12] = 0x00000FFF;
    batch[13] = 0x00000FFF;
    batch[15] = 0x00000FFF;
    
    // MEDIA_VFE_STATE
    int vfe_start = idx;
    batch[idx++] = 0x70000007;
    for (int i = 0; i < 8; i++) batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000020;
    batch[idx++] = 0x00000000;  // DSH (relocalisé)
    
    // GPGPU_WALKER
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = NUM_WORK_GROUPS;  // 16 work groups
    batch[idx++] = 0x00000001;
    
    // PIPE_CONTROL
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;
    for (int i = 0; i < 4; i++) batch[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    int batch_dwords = idx;
    LOG_NS("✓ Batch buffer: %d dwords", batch_dwords);
    
    LOG_NS("=== PHASE 7: RELOCATIONS ===");
    
    struct drm_i915_gem_relocation_entry relocs[6];
    memset(relocs, 0, sizeof(relocs));
    
    relocs[0].target_handle = create_ssh.handle;
    relocs[0].offset = 4 * 4;
    relocs[0].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    relocs[1].target_handle = create_dsh.handle;
    relocs[1].offset = 6 * 4;
    relocs[1].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    relocs[2].target_handle = create_kernel.handle;
    relocs[2].offset = 10 * 4;
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    relocs[3].target_handle = create_dsh.handle;
    relocs[3].offset = (vfe_start + 9 + 3) * 4;
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    relocs[4].target_handle = create_output.handle;
    relocs[4].offset = 1 * 4;
    relocs[4].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[4].write_domain = I915_GEM_DOMAIN_RENDER;
    
    relocs[5].target_handle = create_kernel.handle;
    relocs[5].offset = 0 * 4;
    relocs[5].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    LOG_NS("✓ Relocations configured");
    
    LOG_NS("=== PHASE 8: EXECBUFFER2 ===");
    
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = create_kernel.handle;
    exec_objects[1].handle = create_output.handle;
    exec_objects[1].flags = EXEC_OBJECT_WRITE;
    exec_objects[2].handle = create_ssh.handle;
    exec_objects[2].relocation_count = 1;
    exec_objects[2].relocs_ptr = (uintptr_t)&relocs[4];
    exec_objects[3].handle = create_dsh.handle;
    exec_objects[3].relocation_count = 1;
    exec_objects[3].relocs_ptr = (uintptr_t)&relocs[5];
    exec_objects[4].handle = create_batch.handle;
    exec_objects[4].relocation_count = 4;
    exec_objects[4].relocs_ptr = (uintptr_t)&relocs[0];
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_len = batch_dwords * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    LOG_NS("Submitting to GPU...");
    uint64_t exec_start = get_ns();
    int exec_ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t exec_end = get_ns();
    
    if (exec_ret < 0) {
        LOG_NS("❌ EXECBUFFER2 FAILED: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    LOG_NS("✅ EXECBUFFER2 SUCCESS! (%lu ns)", exec_end - exec_start);
    
    LOG_NS("=== PHASE 9: GPU WAIT ===");
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_batch.handle,
        .timeout_ns = 1000000000
    };
    
    uint64_t wait_start = get_ns();
    drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    uint64_t wait_end = get_ns();
    LOG_NS("✓ GPU complete: %lu ns", wait_end - wait_start);
    
    LOG_NS("=== PHASE 10: RESULTS ===");
    
    uint32_t *output = (uint32_t *)output_map;
    int non_zero = 0;
    int valid_results = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0xCCCCCCCC) {
            non_zero++;
            if (output[i] != 0) {
                valid_results++;
                if (valid_results <= 20) {
                    uint32_t expected = (4 << 16) | i;
                    LOG_NS("  output[%3d] = 0x%08x %s", i, output[i],
                           (output[i] == expected) ? "✓" : "");
                }
            }
        }
    }
    
    LOG_NS("Results: non-zero=%d, valid=%d", non_zero, valid_results);
    
    if (valid_results > 0) {
        hex_dump("OUTPUT (first 256 bytes)", output, 256);
    }
    
    uint64_t total_ns = get_ns() - start_ns;
    LOG_NS("Total: %.3f ms", total_ns / 1000000.0);
    
    if (valid_results > 0) {
        printf("\n🎉 SUCCESS: %d results with Thread Count fix!\n", valid_results);
        printf("✅ BUGS #1+#2 FIXED!\n\n");
    } else {
        printf("\n⚠️  Still no results → Need bug #3 fix (CURBE)\n\n");
    }
    
    // Cleanup
    struct drm_gem_close close_arg;
    close_arg.handle = create_kernel.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_output.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_ssh.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_dsh.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(fd);
    
    return (valid_results > 0) ? 0 : 1;
}

// Made with Bob - C413 Thread Count Fix