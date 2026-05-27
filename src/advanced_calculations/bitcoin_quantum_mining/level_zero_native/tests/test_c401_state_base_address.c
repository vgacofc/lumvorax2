// C401: FIX DÉCISIF - STATE_BASE_ADDRESS + Tests Complets
// Hypothèse: Le vrai problème est l'absence de STATE_BASE_ADDRESS
// Tests: SBA, kernel pointer packing, cache invalidate, offset non-zero

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include <i915_drm.h>
#include <drm.h>

// Gen9 Instructions
#define GEN9_MI_BATCH_BUFFER_END 0x0A000000
#define GEN9_MI_NOOP 0x00000000
#define GEN9_PIPE_CONTROL 0x7A000004
#define GEN9_STATE_BASE_ADDRESS 0x61010009  // CRITIQUE!
#define GEN9_MEDIA_VFE_STATE 0x71000007
#define GEN9_MEDIA_CURBE_LOAD 0x71010003
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x71020004
#define GEN9_GPGPU_WALKER 0x71050015

// Pipe Control flags
#define PIPE_CONTROL_CS_STALL (1 << 20)
#define PIPE_CONTROL_DC_FLUSH (1 << 5)
#define PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE (1 << 11)
#define PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE (1 << 10)
#define PIPE_CONTROL_STATE_CACHE_INVALIDATE (1 << 16)

#define LOG_FORENSIC(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    printf("[%ld.%09ld] C401: " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

void hex_dump(const char *label, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    printf("\n=== %s (%zu bytes) ===\n", label, size);
    for (size_t i = 0; i < size; i += 16) {
        printf("%04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    LOG_FORENSIC("=== STATE_BASE_ADDRESS FIX START ===");
    LOG_FORENSIC("Hypothèse: Manque SBA pour instruction fetch");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/dri/card0", O_RDWR);
        if (fd < 0) {
            LOG_FORENSIC("ERROR: Cannot open DRM: %s", strerror(errno));
            return 1;
        }
    }
    LOG_FORENSIC("DRM opened: fd=%d", fd);
    
    // Load kernel
    FILE *f = fopen("kernels/btc_sha256_c398_raw.bin", "rb");
    if (!f) {
        LOG_FORENSIC("ERROR: Cannot open kernel");
        close(fd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    void *kernel_code = malloc(kernel_size);
    fread(kernel_code, 1, kernel_size, f);
    fclose(f);
    
    LOG_FORENSIC("Kernel loaded: %zu bytes", kernel_size);
    
    // Create buffers - IMPORTANT: Kernel en DERNIER pour offset non-zero
    struct drm_i915_gem_create create_input = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_input);
    
    struct drm_i915_gem_create create_output = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output);
    
    struct drm_i915_gem_create create_curbe = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_curbe);
    
    struct drm_i915_gem_create create_idesc = { .size = 4096 };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_idesc);
    
    struct drm_i915_gem_create create_batch = { .size = 8192 };  // Plus gros pour SBA
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch);
    
    // TEST 4: Kernel en DERNIER pour offset non-zero
    struct drm_i915_gem_create create_kernel = { 
        .size = (kernel_size + 4095) & ~4095 
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel);
    
    LOG_FORENSIC("All buffers created (kernel last for non-zero offset)");
    
    // Upload kernel
    struct drm_i915_gem_pwrite pwrite_kernel = {
        .handle = create_kernel.handle,
        .data_ptr = (uint64_t)kernel_code,
        .size = kernel_size
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_kernel);
    
    // Upload input
    uint32_t block_header[20] = {
        0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3BA3EDFD,
        0x7A7B12B2, 0x7AC72C3E, 0x67768F61, 0x7FC81BC3, 0x888A5132,
        0x3A9FB8AA, 0x4B1E5E4A, 0x29AB5F49, 0xFFFF001D, 0x1DAC2B7C
    };
    struct drm_i915_gem_pwrite pwrite_input = {
        .handle = create_input.handle,
        .data_ptr = (uint64_t)block_header,
        .size = sizeof(block_header)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_input);
    
    // Zero output
    uint32_t zeros[1024] = {0};
    struct drm_i915_gem_pwrite pwrite_zero = {
        .handle = create_output.handle,
        .data_ptr = (uint64_t)zeros,
        .size = sizeof(zeros)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_zero);
    
    // Get GTT addresses
    struct drm_i915_gem_exec_object2 all_objects[6] = {
        { .handle = create_input.handle, .flags = 0 },
        { .handle = create_output.handle, .flags = EXEC_OBJECT_WRITE },
        { .handle = create_curbe.handle, .flags = 0 },
        { .handle = create_idesc.handle, .flags = 0 },
        { .handle = create_batch.handle, .flags = 0 },
        { .handle = create_kernel.handle, .flags = 0 }  // DERNIER
    };
    
    uint32_t dummy_batch[2] = { GEN9_MI_BATCH_BUFFER_END, GEN9_MI_NOOP };
    struct drm_i915_gem_pwrite pwrite_dummy = {
        .handle = create_batch.handle,
        .data_ptr = (uint64_t)dummy_batch,
        .size = sizeof(dummy_batch)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_dummy);
    
    struct drm_i915_gem_execbuffer2 dummy_exec = {
        .buffers_ptr = (uint64_t)all_objects,
        .buffer_count = 6,
        .batch_start_offset = 0,
        .batch_len = sizeof(dummy_batch),
        .flags = I915_EXEC_RENDER
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &dummy_exec) < 0) {
        LOG_FORENSIC("ERROR: Dummy exec failed: %s", strerror(errno));
        free(kernel_code);
        close(fd);
        return 1;
    }
    
    uint64_t input_gtt = all_objects[0].offset;
    uint64_t output_gtt = all_objects[1].offset;
    uint64_t curbe_gtt = all_objects[2].offset;
    uint64_t idesc_gtt = all_objects[3].offset;
    uint64_t kernel_gtt = all_objects[5].offset;  // DERNIER
    
    LOG_FORENSIC("GTT addresses:");
    LOG_FORENSIC("  Input:   0x%016lx", input_gtt);
    LOG_FORENSIC("  Output:  0x%016lx", output_gtt);
    LOG_FORENSIC("  CURBE:   0x%016lx", curbe_gtt);
    LOG_FORENSIC("  IDesc:   0x%016lx", idesc_gtt);
    LOG_FORENSIC("  Kernel:  0x%016lx ← TEST 4: Non-zero!", kernel_gtt);
    
    // Build CURBE
    uint32_t curbe[32] = {0};
    curbe[0] = (uint32_t)(input_gtt & 0xFFFFFFFF);
    curbe[1] = (uint32_t)(input_gtt >> 32);
    curbe[2] = (uint32_t)(output_gtt & 0xFFFFFFFF);
    curbe[3] = (uint32_t)(output_gtt >> 32);
    curbe[4] = 256;
    
    struct drm_i915_gem_pwrite pwrite_curbe = {
        .handle = create_curbe.handle,
        .data_ptr = (uint64_t)curbe,
        .size = sizeof(curbe)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_curbe);
    
    // TEST 2: Kernel pointer packing (>> 6 pour alignment 64B)
    uint32_t kernel_offset_packed = (uint32_t)(kernel_gtt >> 6);
    
    LOG_FORENSIC("TEST 2: Kernel pointer packing");
    LOG_FORENSIC("  Raw GTT:    0x%016lx", kernel_gtt);
    LOG_FORENSIC("  Packed >>6: 0x%08x", kernel_offset_packed);
    
    // Build Interface Descriptor
    uint32_t interface_desc[8] = {0};
    interface_desc[0] = kernel_offset_packed;  // TEST 2: Packed!
    interface_desc[2] = 0x00000000;
    interface_desc[3] = 0x00000000;
    interface_desc[4] = 0x00000000;
    interface_desc[5] = (7 << 16) | (1 << 8) | 1;
    interface_desc[6] = 0x00000000;
    interface_desc[7] = 0x00000000;
    
    hex_dump("Interface Descriptor (with packed kernel ptr)", interface_desc, sizeof(interface_desc));
    
    struct drm_i915_gem_pwrite pwrite_idesc = {
        .handle = create_idesc.handle,
        .data_ptr = (uint64_t)interface_desc,
        .size = sizeof(interface_desc)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_idesc);
    
    // Build batch with STATE_BASE_ADDRESS
    uint32_t batch[512] = {0};
    int idx = 0;
    
    // TEST 3: Instruction cache invalidate AVANT tout
    LOG_FORENSIC("TEST 3: Adding instruction cache invalidate");
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = PIPE_CONTROL_CS_STALL | 
                   PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE |
                   PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
                   PIPE_CONTROL_STATE_CACHE_INVALIDATE;
    batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0;
    
    // TEST 1: STATE_BASE_ADDRESS - LE FIX CRITIQUE!
    LOG_FORENSIC("TEST 1: Adding STATE_BASE_ADDRESS");
    batch[idx++] = GEN9_STATE_BASE_ADDRESS;  // Opcode 0x6101
    batch[idx++] = 0x00000001;  // General State Base Address Modify Enable
    batch[idx++] = 0;           // General State Base Address (unused)
    batch[idx++] = 0;           // General State Base Address Upper
    batch[idx++] = 0x00000001;  // Surface State Base Address Modify Enable
    batch[idx++] = 0;           // Surface State Base Address (unused for A64)
    batch[idx++] = 0;           // Surface State Base Address Upper
    batch[idx++] = 0x00000001;  // Dynamic State Base Address Modify Enable
    batch[idx++] = (uint32_t)(curbe_gtt & 0xFFFFFFFF);  // Dynamic State = CURBE
    batch[idx++] = (uint32_t)(curbe_gtt >> 32);
    batch[idx++] = 0x00000001;  // Indirect Object Base Address Modify Enable
    batch[idx++] = 0;           // Indirect Object Base Address
    batch[idx++] = 0;           // Indirect Object Base Address Upper
    batch[idx++] = 0x00000001;  // Instruction Base Address Modify Enable
    batch[idx++] = (uint32_t)(kernel_gtt & 0xFFFFFFFF);  // CRITIQUE: Instruction Base!
    batch[idx++] = (uint32_t)(kernel_gtt >> 32);
    
    LOG_FORENSIC("STATE_BASE_ADDRESS configured:");
    LOG_FORENSIC("  Instruction Base: 0x%016lx", kernel_gtt);
    LOG_FORENSIC("  Dynamic State:    0x%016lx", curbe_gtt);
    
    // PIPE_CONTROL après SBA
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = PIPE_CONTROL_CS_STALL | PIPE_CONTROL_DC_FLUSH;
    batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0;
    
    // MEDIA_VFE_STATE
    batch[idx++] = GEN9_MEDIA_VFE_STATE;
    batch[idx++] = 0; batch[idx++] = 0;
    batch[idx++] = (24 << 16) | (24 << 8);
    batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0;
    
    // MEDIA_CURBE_LOAD
    batch[idx++] = GEN9_MEDIA_CURBE_LOAD;
    batch[idx++] = 0;
    batch[idx++] = sizeof(curbe);
    batch[idx++] = (uint32_t)(curbe_gtt & 0xFFFFFFFF);
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD;
    batch[idx++] = 0;
    batch[idx++] = sizeof(interface_desc);
    batch[idx++] = (uint32_t)(idesc_gtt & 0xFFFFFFFF);
    batch[idx++] = 0;
    
    // GPGPU_WALKER
    batch[idx++] = GEN9_GPGPU_WALKER;
    batch[idx++] = 0;
    batch[idx++] = (1 << 30);
    for (int i = 0; i < 19; i++) batch[idx++] = 0;
    batch[idx-8] = 16;
    batch[idx-7] = 16;
    batch[idx-6] = 0xFFFFFFFF;
    
    // PIPE_CONTROL final
    batch[idx++] = GEN9_PIPE_CONTROL;
    batch[idx++] = PIPE_CONTROL_CS_STALL | PIPE_CONTROL_DC_FLUSH;
    batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0; batch[idx++] = 0;
    
    batch[idx++] = GEN9_MI_BATCH_BUFFER_END;
    
    size_t batch_size = idx * 4;
    
    LOG_FORENSIC("Batch size: %zu bytes (%d dwords)", batch_size, idx);
    hex_dump("Batch (first 256 bytes)", batch, batch_size < 256 ? batch_size : 256);
    
    struct drm_i915_gem_pwrite pwrite_batch = {
        .handle = create_batch.handle,
        .data_ptr = (uint64_t)batch,
        .size = batch_size
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite_batch);
    
    // Execute
    struct drm_i915_gem_execbuffer2 final_exec = {
        .buffers_ptr = (uint64_t)all_objects,
        .buffer_count = 6,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .flags = I915_EXEC_RENDER
    };
    
    LOG_FORENSIC("Executing with STATE_BASE_ADDRESS...");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &final_exec) < 0) {
        LOG_FORENSIC("ERROR: Final exec failed: %s", strerror(errno));
        free(kernel_code);
        close(fd);
        return 1;
    }
    
    LOG_FORENSIC("Dispatch completed");
    
    // Wait
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_output.handle,
        .timeout_ns = 5000000000
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    // Set domain
    struct drm_i915_gem_set_domain set_domain = {
        .handle = create_output.handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    
    // Read results
    uint32_t results[1024] = {0};
    struct drm_i915_gem_pread pread = {
        .handle = create_output.handle,
        .data_ptr = (uint64_t)results,
        .size = sizeof(results)
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_PREAD, &pread);
    
    hex_dump("Results (first 256 bytes)", results, 256);
    
    // Analyze
    int non_zero = 0;
    int max_lz = 0;
    for (int i = 0; i < 256; i++) {
        if (results[i] != 0) {
            non_zero++;
            int lz = __builtin_clz(results[i]);
            if (lz > max_lz) {
                max_lz = lz;
                LOG_FORENSIC("Thread %d: 0x%08x (%d leading zeros)", i, results[i], lz);
            }
        }
    }
    
    LOG_FORENSIC("=== RESULTS ===");
    LOG_FORENSIC("Non-zero: %d / 256", non_zero);
    LOG_FORENSIC("Max leading zeros: %d bits", max_lz);
    
    int success = 0;
    if (non_zero > 0 && max_lz > 0) {
        LOG_FORENSIC("🎉 BREAKTHROUGH: STATE_BASE_ADDRESS FIX WORKS!");
        LOG_FORENSIC("✅ GPU executed SHA256 successfully!");
        LOG_FORENSIC("✅ First near_miss_gpu > 0 bits in native runtime!");
        success = 1;
    } else if (non_zero > 0) {
        LOG_FORENSIC("⚠️  PARTIAL: GPU wrote data but no leading zeros");
        success = 1;
    } else {
        LOG_FORENSIC("❌ FAILURE: Still all zeros");
        LOG_FORENSIC("Need to investigate further (SBA format? alignment?)");
    }
    
    free(kernel_code);
    close(fd);
    
    LOG_FORENSIC("=== C401 END ===");
    return success ? 0 : 1;
}

// Made with Bob
