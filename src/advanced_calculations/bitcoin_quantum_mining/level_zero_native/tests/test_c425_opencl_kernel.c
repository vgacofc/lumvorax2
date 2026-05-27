// C425: Test OpenCL C356 Extracted - 1 thread, 1 lane, 1 write
// Objectif: Valider dataport write fonctionne RÉELLEMENT
// Basé sur feedback expert: tester SIMD1 avec descriptor Intel PRM

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
#include <xmmintrin.h>

// i915 DRM headers
#include <drm/i915_drm.h>

// Logging forensic nanoseconde
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG_NS(fmt, ...) do { \
    uint64_t ts = get_timestamp_ns(); \
    printf("[%lu ns] " fmt "\n", ts, ##__VA_ARGS__); \
} while(0)

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C425 TEST - OpenCL C356 Extracted (Intel PRM Descriptor)        ║\n");
    printf("║  1 thread, 1 lane, 1 write - Validation dataport         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    LOG_NS("C425 START - SIMD1 Minimal Test");

    // Ouvrir DRM
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        LOG_NS("ERROR: Cannot open DRM device: %s", strerror(errno));
        return 1;
    }
    LOG_NS("DRM opened: fd=%d", drm_fd);

    // Créer contexte GPU
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG_NS("ERROR: Cannot create GPU context: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    LOG_NS("GPU context created: ctx_id=%u", ctx_create.ctx_id);

    // Charger kernel SIMD1
    FILE* f = fopen("kernels/test_c356_extracted.bin", "rb");
    if (!f) {
        LOG_NS("ERROR: Cannot open kernel file");
        close(drm_fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* kernel_code = malloc(kernel_size);
    fread(kernel_code, 1, kernel_size, f);
    fclose(f);
    LOG_NS("Kernel loaded: %zu bytes", kernel_size);

    // Créer buffer kernel
    struct drm_i915_gem_create gem_create = {
        .size = 4096  // 1 page
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        LOG_NS("ERROR: Cannot create kernel buffer: %s", strerror(errno));
        free(kernel_code);
        close(drm_fd);
        return 1;
    }
    uint32_t kernel_bo = gem_create.handle;
    LOG_NS("Kernel buffer: handle=%u", kernel_bo);

    // Mapper et copier kernel
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = kernel_bo,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        LOG_NS("ERROR: Cannot mmap kernel: %s", strerror(errno));
        free(kernel_code);
        close(drm_fd);
        return 1;
    }
    void* kernel_map = (void*)(uintptr_t)mmap_kernel.addr_ptr;
    memcpy(kernel_map, kernel_code, kernel_size);
    munmap(kernel_map, 4096);
    free(kernel_code);

    // Créer output buffer (64 bytes = 16 × uint32_t)
    struct drm_i915_gem_create gem_create_output = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create_output) < 0) {
        LOG_NS("ERROR: Cannot create output buffer: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t output_bo = gem_create_output.handle;

    // Mapper output buffer
    struct drm_i915_gem_mmap mmap_output = {
        .handle = output_bo,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        LOG_NS("ERROR: Cannot mmap output: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t* output_map = (uint32_t*)(uintptr_t)mmap_output.addr_ptr;

    // Remplir avec pattern 0xCC
    memset(output_map, 0xCC, 64);
    LOG_NS("Output buffer: handle=%u (filled 0xCC)", output_bo);

    // Créer Surface State Heap (SSH)
    struct drm_i915_gem_create gem_create_ssh = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create_ssh) < 0) {
        LOG_NS("ERROR: Cannot create SSH: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t ssh_bo = gem_create_ssh.handle;

    // Mapper SSH
    struct drm_i915_gem_mmap mmap_ssh = {
        .handle = ssh_bo,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_ssh) < 0) {
        LOG_NS("ERROR: Cannot mmap SSH: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t* ssh_map = (uint32_t*)(uintptr_t)mmap_ssh.addr_ptr;
    memset(ssh_map, 0, 4096);

    // Configurer Surface State (BTI 1)
    // Offset 64 bytes (BTI 1 = 2 × 32 bytes)
    uint32_t* surface_state = &ssh_map[16];  // 16 dwords = 64 bytes
    surface_state[0] = 0x00000000;  // SURFTYPE_BUFFER (0x00)
    surface_state[1] = 0x00000000;  // Base address (sera relocalisé)
    surface_state[2] = 0x0000003F;  // Width = 64 bytes - 1
    surface_state[3] = 0x00000000;  // Height = 0
    surface_state[4] = 0x00000000;  // Depth = 0
    surface_state[5] = 0x0000000D;  // FORMAT = R32_UINT (0x0D)
    surface_state[6] = 0x00000000;  // Reserved
    surface_state[7] = 0x00000002;  // MOCS = LLC (0x02)

    munmap(ssh_map, 4096);
    LOG_NS("SSH configured - BTI 1 = output buffer");

    // Créer Dynamic State Heap (DSH)
    struct drm_i915_gem_create gem_create_dsh = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create_dsh) < 0) {
        LOG_NS("ERROR: Cannot create DSH: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t dsh_bo = gem_create_dsh.handle;

    // Mapper DSH
    struct drm_i915_gem_mmap mmap_dsh = {
        .handle = dsh_bo,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_dsh) < 0) {
        LOG_NS("ERROR: Cannot mmap DSH: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t* dsh_map = (uint32_t*)(uintptr_t)mmap_dsh.addr_ptr;
    memset(dsh_map, 0, 4096);

    // Configurer Interface Descriptor
    dsh_map[0] = 0x00000000;  // Kernel start pointer (sera relocalisé)
    dsh_map[1] = 0x00000000;  // Reserved
    dsh_map[2] = 0x00000000;  // Reserved
    dsh_map[3] = 0x00000002;  // Binding Table offset = 2 (64 bytes ÷ 32)
    dsh_map[4] = 0x00000000;  // Sampler State offset
    dsh_map[5] = 0x00000001;  // Number of Threads = 1 (SIMD1)
    dsh_map[6] = 0x00000000;  // Reserved
    dsh_map[7] = 0x00000000;  // Reserved

    munmap(dsh_map, 4096);
    LOG_NS("DSH configured - Interface Descriptor ready (1 thread)");

    // Créer batch buffer
    struct drm_i915_gem_create gem_create_batch = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create_batch) < 0) {
        LOG_NS("ERROR: Cannot create batch: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t batch_bo = gem_create_batch.handle;

    // Mapper batch
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = batch_bo,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        LOG_NS("ERROR: Cannot mmap batch: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t* batch = (uint32_t*)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);

    int idx = 0;

    // PIPELINE_SELECT
    batch[idx++] = 0x69040000;  // GPGPU pipeline

    // STATE_BASE_ADDRESS (17 dwords)
    batch[idx++] = 0x61010015;  // Opcode
    batch[idx++] = 0x00000000;  // General State Base Address
    batch[idx++] = 0x00000000;  // General State Base Address Upper
    batch[idx++] = 0x00000000;  // Stateless Data Port Access MOCS
    batch[idx++] = 0x00000000;  // Surface State Base Address (sera relocalisé)
    batch[idx++] = 0x00000000;  // Surface State Base Address Upper
    batch[idx++] = 0x00000000;  // Dynamic State Base Address (sera relocalisé)
    batch[idx++] = 0x00000000;  // Dynamic State Base Address Upper
    batch[idx++] = 0x00000000;  // Indirect Object Base Address
    batch[idx++] = 0x00000000;  // Indirect Object Base Address Upper
    batch[idx++] = 0x00000000;  // Instruction Base Address (sera relocalisé)
    batch[idx++] = 0x00000000;  // Instruction Base Address Upper
    batch[idx++] = 0x00000FFF;  // General State Buffer Size
    batch[idx++] = 0x00000FFF;  // Dynamic State Buffer Size
    batch[idx++] = 0x00000FFF;  // Indirect Object Buffer Size
    batch[idx++] = 0x00000FFF;  // Instruction Buffer Size
    batch[idx++] = 0x00000000;  // Reserved

    // MEDIA_VFE_STATE (8 dwords)
    batch[idx++] = 0x70000006;  // Opcode
    batch[idx++] = 0x00000000;  // Scratch Space Base Pointer
    batch[idx++] = 0x00000000;  // Scratch Space Base Pointer Upper
    batch[idx++] = 0x000000C0;  // Max Threads = 192 (24 EU × 8)
    batch[idx++] = 0x00000001;  // URB Entries = 1
    batch[idx++] = 0x00000000;  // CURBE Allocation Size
    batch[idx++] = 0x00000000;  // Scoreboard Mask
    batch[idx++] = 0x00000000;  // Reserved

    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 dwords)
    batch[idx++] = 0x70020002;  // Opcode
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000020;  // Interface Descriptor Length = 32 bytes
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset

    // GPGPU_WALKER (15 dwords)
    batch[idx++] = 0x7002000D;  // Opcode
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address
    batch[idx++] = 0x00000000;  // Thread Group ID Starting X
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000000;  // Thread Group ID X Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Y
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000000;  // Thread Group ID Y Dimension
    batch[idx++] = 0x00000000;  // Thread Group ID Starting Z
    batch[idx++] = 0x00000000;  // Reserved
    batch[idx++] = 0x00000000;  // Thread Group ID Z Dimension
    batch[idx++] = 0x00000001;  // Thread Width X = 1 (SIMD1)
    batch[idx++] = 0x00000001;  // Thread Group Width X = 1

    // PIPE_CONTROL (6 dwords)
    batch[idx++] = 0x7A000004;  // Opcode
    batch[idx++] = 0x001F0000;  // All flush bits
    batch[idx++] = 0x00000000;  // Address
    batch[idx++] = 0x00000000;  // Address Upper
    batch[idx++] = 0x00000000;  // Immediate Data
    batch[idx++] = 0x00000000;  // Immediate Data

    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;

    munmap(batch, 4096);
    LOG_NS("Batch buffer: %d dwords (SIMD1 configuration)", idx);

    // Préparer relocations
    struct drm_i915_gem_relocation_entry relocs[4];
    memset(relocs, 0, sizeof(relocs));

    // Reloc 0: Surface State Base Address (batch[4])
    relocs[0].target_handle = ssh_bo;
    relocs[0].delta = 0;
    relocs[0].offset = 4 * 4;  // batch[4]
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = 0;

    // Reloc 1: Dynamic State Base Address (batch[6])
    relocs[1].target_handle = dsh_bo;
    relocs[1].delta = 0;
    relocs[1].offset = 6 * 4;  // batch[6]
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = 0;

    // Reloc 2: Instruction Base Address (batch[10])
    relocs[2].target_handle = kernel_bo;
    relocs[2].delta = 0;
    relocs[2].offset = 10 * 4;  // batch[10]
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;

    // Reloc 3: Surface State BTI 1 base address (SSH offset 68)
    relocs[3].target_handle = output_bo;
    relocs[3].delta = 0;
    relocs[3].offset = 68;  // SSH[17] = surface_state[1]
    relocs[3].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[3].write_domain = I915_GEM_DOMAIN_RENDER;

    // Préparer exec objects
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));

    exec_objects[0].handle = kernel_bo;
    exec_objects[0].relocation_count = 0;
    exec_objects[0].relocs_ptr = 0;

    exec_objects[1].handle = output_bo;
    exec_objects[1].relocation_count = 0;
    exec_objects[1].relocs_ptr = 0;

    exec_objects[2].handle = ssh_bo;
    exec_objects[2].relocation_count = 1;
    exec_objects[2].relocs_ptr = (uintptr_t)&relocs[3];

    exec_objects[3].handle = dsh_bo;
    exec_objects[3].relocation_count = 0;
    exec_objects[3].relocs_ptr = 0;

    exec_objects[4].handle = batch_bo;
    exec_objects[4].relocation_count = 3;
    exec_objects[4].relocs_ptr = (uintptr_t)&relocs[0];

    // Exécuter
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };

    LOG_NS("Executing C425 with SIMD1 minimal...");
    uint64_t exec_start = get_timestamp_ns();
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        LOG_NS("ERROR: EXECBUFFER2 failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint64_t exec_end = get_timestamp_ns();
    LOG_NS("✓ EXECBUFFER2 success: %lu ns", exec_end - exec_start);

    // Attendre GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = -1
    };
    uint64_t wait_start = get_timestamp_ns();
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        LOG_NS("ERROR: GEM_WAIT failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint64_t wait_end = get_timestamp_ns();
    LOG_NS("✓ GPU execution complete: %lu ns", wait_end - wait_start);

    // Invalider cache CPU
    LOG_NS("Invalidating CPU cache...");
    uint64_t flush_start = get_timestamp_ns();
    for (int i = 0; i < 16; i++) {
        _mm_clflush(&output_map[i]);
    }
    _mm_mfence();
    uint64_t flush_end = get_timestamp_ns();
    LOG_NS("✓ CPU cache invalidated: %lu ns", flush_end - flush_start);

    // Analyser résultats
    printf("\n📊 RÉSULTATS C425 (OpenCL C356 Extracted):\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("═══════════════════════════════════════════════════════════\n");

    int valid_count = 0;
    int unchanged_count = 0;
    int unexpected_count = 0;

    for (int i = 0; i < 16; i++) {
        if (output_map[i] == 0xDEADBEEF) {
            valid_count++;
        } else if (output_map[i] == 0xCCCCCCCC) {
            unchanged_count++;
        } else {
            unexpected_count++;
        }
    }

    printf("  Valid (0xDEADBEEF): %d / 16\n", valid_count);
    printf("  Unchanged (0xCC):   %d / 16\n", unchanged_count);
    printf("  Unexpected:         %d / 16\n", unexpected_count);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    if (valid_count > 0) {
        printf("✅ SUCCESS: GPU wrote %d value(s)!\n", valid_count);
        printf("   Dataport write VALIDATED\n");
        LOG_NS("C425 SUCCESS - First GPU write validated!");
    } else {
        printf("❌ CRITICAL: Still 0 writes with SIMD1 + Intel PRM descriptor\n");
        printf("   Problem is deeper than SIMD mode\n");
        printf("   Likely: descriptor still invalid OR payload format wrong\n");
        LOG_NS("C425 FAILED - SIMD1 still produces 0 writes");
    }

    // Cleanup
    munmap(output_map, 4096);
    close(drm_fd);

    return (valid_count > 0) ? 0 : 1;
}

// Made with Bob
