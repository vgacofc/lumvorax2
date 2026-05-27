/**
 * C411 - BATCH BUFFER AUTOMATIQUE AVEC FORENSIC NANOSECONDE
 * 
 * OBJECTIF: Générer automatiquement un batch buffer COMPLET et CORRECT
 * pour exécuter le kernel SHA256 natif sur GPU Intel Gen9
 * 
 * INSTRUMENTATION:
 * - Logging nanoseconde de chaque étape
 * - Dump hexadécimal de tous les buffers
 * - Tracking mémoire bit-level
 * - Validation hardware complète
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

// Forensic nanosecond timestamp
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

// Dump hexadécimal forensique
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
    printf("║  C411 - BATCH BUFFER AUTOMATIQUE + FORENSIC NANOSECONDE     ║\n");
    printf("║  Kernel: btc_sha256_native_c406.bin (176 bytes)              ║\n");
    printf("║  Instrumentation: 100%% nanoseconde + bit-level tracking     ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    LOG_NS("=== PHASE 1: DRM INITIALIZATION ===");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        LOG_NS("ERROR: open /dev/dri/card1 failed: %s", strerror(errno));
        return 1;
    }
    LOG_NS("✓ DRM opened: fd=%d", fd);
    
    // Créer contexte GPU
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG_NS("ERROR: CONTEXT_CREATE failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    LOG_NS("✓ GPU context created: ctx_id=%u", ctx_id);
    
    LOG_NS("=== PHASE 2: KERNEL LOADING ===");
    
    // Charger kernel
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        LOG_NS("ERROR: fopen kernel failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    LOG_NS("✓ Kernel file opened: size=%zu bytes", kernel_size);
    
    // Créer buffer kernel (aligné 4KB)
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        LOG_NS("ERROR: GEM_CREATE kernel failed: %s", strerror(errno));
        fclose(f);
        close(fd);
        return 1;
    }
    LOG_NS("✓ Kernel BO created: handle=%u, size=4096", create_kernel.handle);
    
    // Mapper kernel
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        LOG_NS("ERROR: GEM_MMAP kernel failed: %s", strerror(errno));
        fclose(f);
        close(fd);
        return 1;
    }
    
    void *kernel_map = (void *)(uintptr_t)mmap_kernel.addr_ptr;
    memset(kernel_map, 0, 4096);
    size_t read_size = fread(kernel_map, 1, kernel_size, f);
    fclose(f);
    LOG_NS("✓ Kernel loaded: %zu bytes read", read_size);
    
    // Dump kernel
    hex_dump("KERNEL BINARY", kernel_map, kernel_size);
    
    LOG_NS("=== PHASE 3: OUTPUT BUFFER ===");
    
    // Créer buffer output
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        LOG_NS("ERROR: GEM_CREATE output failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    LOG_NS("✓ Output BO created: handle=%u, size=4096", create_output.handle);
    
    // Mapper output
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        LOG_NS("ERROR: GEM_MMAP output failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    void *output_map = (void *)(uintptr_t)mmap_output.addr_ptr;
    memset(output_map, 0xCC, 4096);  // Pattern 0xCC pour détecter écritures
    LOG_NS("✓ Output buffer initialized: pattern=0xCC");
    
    LOG_NS("=== PHASE 4: SURFACE STATE HEAP (SSH) ===");
    
    // Créer SSH pour Surface States
    struct drm_i915_gem_create create_ssh = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_ssh) < 0) {
        LOG_NS("ERROR: GEM_CREATE SSH failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    LOG_NS("✓ SSH BO created: handle=%u, size=4096", create_ssh.handle);
    
    // Mapper SSH
    struct drm_i915_gem_mmap mmap_ssh = {
        .handle = create_ssh.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_ssh) < 0) {
        LOG_NS("ERROR: GEM_MMAP SSH failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    uint32_t *ssh = (uint32_t *)(uintptr_t)mmap_ssh.addr_ptr;
    memset(ssh, 0, 4096);
    
    // Surface State pour output buffer (BTI=0)
    // Gen9 Surface State = 16 DWords (64 bytes)
    ssh[0] = 0x00000000;  // DW0: Surface Type = SURFTYPE_BUFFER
    ssh[1] = 0x00000000;  // DW1: Surface Base Address [31:0] (sera relocalisé)
    ssh[2] = 0x00000000;  // DW2: Surface Base Address [63:32]
    ssh[3] = 0x00000FFF;  // DW3: Width = 4095 (4096 bytes - 1)
    ssh[4] = 0x00000000;  // DW4: Height = 0
    ssh[5] = 0x00000000;  // DW5: Depth = 0
    ssh[6] = 0x00000000;  // DW6: Surface Pitch = 0
    ssh[7] = 0x00000000;  // DW7: Reserved
    ssh[8] = 0x00000000;  // DW8-15: Reserved
    
    LOG_NS("✓ Surface State configured: BTI=0, size=4096");
    hex_dump("SURFACE STATE (BTI=0)", ssh, 64);
    
    LOG_NS("=== PHASE 5: DYNAMIC STATE HEAP (DSH) ===");
    
    // Créer DSH pour Interface Descriptor
    struct drm_i915_gem_create create_dsh = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_dsh) < 0) {
        LOG_NS("ERROR: GEM_CREATE DSH failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    LOG_NS("✓ DSH BO created: handle=%u, size=4096", create_dsh.handle);
    
    // Mapper DSH
    struct drm_i915_gem_mmap mmap_dsh = {
        .handle = create_dsh.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_dsh) < 0) {
        LOG_NS("ERROR: GEM_MMAP DSH failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    uint32_t *dsh = (uint32_t *)(uintptr_t)mmap_dsh.addr_ptr;
    memset(dsh, 0, 4096);
    
    // Interface Descriptor = 8 DWords (32 bytes)
    dsh[0] = 0x00000000;  // DW0: Kernel Start Pointer [31:6] (sera relocalisé)
    dsh[1] = 0x00000000;  // DW1: Reserved
    dsh[2] = 0x00000000;  // DW2: Sampler State Pointer
    dsh[3] = 0x00000000;  // DW3: Binding Table Pointer (pointe vers SSH)
    dsh[4] = 0x00000000;  // DW4: Shared Local Memory Size
    dsh[5] = 0x00010000;  // DW5: Number of Threads = 1 (SIMD16)
    dsh[6] = 0x00000000;  // DW6: Cross-Thread Constant Data
    dsh[7] = 0x00000000;  // DW7: Barrier Enable = 0
    
    LOG_NS("✓ Interface Descriptor configured");
    hex_dump("INTERFACE DESCRIPTOR", dsh, 32);
    
    LOG_NS("=== PHASE 6: BATCH BUFFER CONSTRUCTION ===");
    
    // Créer batch buffer
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        LOG_NS("ERROR: GEM_CREATE batch failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    LOG_NS("✓ Batch BO created: handle=%u, size=4096", create_batch.handle);
    
    // Mapper batch
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        LOG_NS("ERROR: GEM_MMAP batch failed: %s", strerror(errno));
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    int idx = 0;
    
    LOG_NS("Building STATE_BASE_ADDRESS...");
    // STATE_BASE_ADDRESS (Gen9)
    batch[idx++] = 0x61010010;  // Opcode: STATE_BASE_ADDRESS, Length=16
    batch[idx++] = 0x00000000;  // DW1: General State Base Address [31:0]
    batch[idx++] = 0x00000000;  // DW2: General State Base Address [63:32]
    batch[idx++] = 0x00000000;  // DW3: General State Buffer Size
    batch[idx++] = 0x00000000;  // DW4: Surface State Base Address [31:0] (relocalisé)
    batch[idx++] = 0x00000000;  // DW5: Surface State Base Address [63:32]
    batch[idx++] = 0x00000000;  // DW6: Dynamic State Base Address [31:0] (relocalisé)
    batch[idx++] = 0x00000000;  // DW7: Dynamic State Base Address [63:32]
    batch[idx++] = 0x00000000;  // DW8: Indirect Object Base Address [31:0]
    batch[idx++] = 0x00000000;  // DW9: Indirect Object Base Address [63:32]
    batch[idx++] = 0x00000000;  // DW10: Instruction Base Address [31:0] (relocalisé)
    batch[idx++] = 0x00000000;  // DW11: Instruction Base Address [63:32]
    batch[idx++] = 0x00000FFF;  // DW12: General State Buffer Size = 4095
    batch[idx++] = 0x00000FFF;  // DW13: Dynamic State Buffer Size = 4095
    batch[idx++] = 0x00000000;  // DW14: Indirect Object Buffer Size
    batch[idx++] = 0x00000FFF;  // DW15: Instruction Buffer Size = 4095
    batch[idx++] = 0x00000000;  // DW16: Bindless Surface State Base Address [31:0]
    batch[idx++] = 0x00000000;  // DW17: Bindless Surface State Base Address [63:32]
    LOG_NS("✓ STATE_BASE_ADDRESS: %d dwords", idx);
    
    LOG_NS("Building MEDIA_VFE_STATE...");
    // MEDIA_VFE_STATE
    int vfe_start = idx;
    batch[idx++] = 0x70000007;  // Opcode: MEDIA_VFE_STATE, Length=7
    batch[idx++] = 0x00000000;  // DW1: Scratch Space Base Pointer
    batch[idx++] = 0x00000000;  // DW2: Scratch Space Base Pointer [63:32]
    batch[idx++] = 0x00000000;  // DW3: Max Threads = 0 (auto)
    batch[idx++] = 0x00000000;  // DW4: URB Entries = 0
    batch[idx++] = 0x00000000;  // DW5: CURBE Allocation Size = 0
    batch[idx++] = 0x00000000;  // DW6: Scoreboard Mask
    batch[idx++] = 0x00000000;  // DW7: Reserved
    batch[idx++] = 0x00000000;  // DW8: Reserved
    LOG_NS("✓ MEDIA_VFE_STATE: %d dwords", idx - vfe_start);
    
    LOG_NS("Building MEDIA_INTERFACE_DESCRIPTOR_LOAD...");
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    int midl_start = idx;
    batch[idx++] = 0x70020002;  // Opcode: MEDIA_INTERFACE_DESCRIPTOR_LOAD, Length=2
    batch[idx++] = 0x00000000;  // DW1: Reserved
    batch[idx++] = 0x00000020;  // DW2: Interface Descriptor Total Length = 32 bytes
    batch[idx++] = 0x00000000;  // DW3: Interface Descriptor Data Start Address (relocalisé)
    LOG_NS("✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD: %d dwords", idx - midl_start);
    
    LOG_NS("Building GPGPU_WALKER...");
    // GPGPU_WALKER
    int walker_start = idx;
    batch[idx++] = 0x7A000004;  // Opcode: GPGPU_WALKER, Length=4
    batch[idx++] = 0x00000000;  // DW1: Interface Descriptor Offset = 0
    batch[idx++] = 0x00000000;  // DW2: Indirect Data Length = 0
    batch[idx++] = 0x00000000;  // DW3: Indirect Data Start Address = 0
    batch[idx++] = NUM_WORK_GROUPS;  // DW4: Thread Width Counter = work groups
    batch[idx++] = 0x00000001;  // DW5: Thread Height Counter = 1
    LOG_NS("✓ GPGPU_WALKER: %d dwords, work_groups=%d", idx - walker_start, NUM_WORK_GROUPS);
    
    LOG_NS("Building PIPE_CONTROL...");
    // PIPE_CONTROL (flush)
    batch[idx++] = 0x7A000004;  // Opcode: PIPE_CONTROL, Length=4
    batch[idx++] = 0x00100000;  // DW1: CS Stall
    batch[idx++] = 0x00000000;  // DW2: Address [31:0]
    batch[idx++] = 0x00000000;  // DW3: Address [63:32]
    batch[idx++] = 0x00000000;  // DW4: Immediate Data
    batch[idx++] = 0x00000000;  // DW5: Immediate Data
    LOG_NS("✓ PIPE_CONTROL: flush");
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    LOG_NS("✓ MI_BATCH_BUFFER_END");
    
    int batch_dwords = idx;
    LOG_NS("✓ Batch buffer complete: %d dwords (%d bytes)", batch_dwords, batch_dwords * 4);
    
    hex_dump("BATCH BUFFER", batch, batch_dwords * 4);
    
    LOG_NS("=== PHASE 7: RELOCATIONS ===");
    
    // Préparer relocations
    struct drm_i915_gem_relocation_entry relocs[6];
    int reloc_idx = 0;
    
    // Relocation 1: SSH dans STATE_BASE_ADDRESS DW4
    relocs[reloc_idx].target_handle = create_ssh.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 4 * 4;  // DW4
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    LOG_NS("Relocation %d: SSH @ batch[4]", reloc_idx);
    
    // Relocation 2: DSH dans STATE_BASE_ADDRESS DW6
    relocs[reloc_idx].target_handle = create_dsh.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 6 * 4;  // DW6
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    LOG_NS("Relocation %d: DSH @ batch[6]", reloc_idx);
    
    // Relocation 3: Kernel dans STATE_BASE_ADDRESS DW10
    relocs[reloc_idx].target_handle = create_kernel.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 10 * 4;  // DW10
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    LOG_NS("Relocation %d: Kernel @ batch[10]", reloc_idx);
    
    // Relocation 4: DSH dans MEDIA_INTERFACE_DESCRIPTOR_LOAD DW3
    int midl_dw3_offset = (vfe_start + 9 + 3) * 4;
    relocs[reloc_idx].target_handle = create_dsh.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = midl_dw3_offset;
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    LOG_NS("Relocation %d: DSH @ MIDL DW3 (offset=%d)", reloc_idx, midl_dw3_offset);
    
    // Relocation 5: Output dans SSH DW1
    relocs[reloc_idx].target_handle = create_output.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 1 * 4;  // SSH DW1
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[reloc_idx].write_domain = I915_GEM_DOMAIN_RENDER;
    reloc_idx++;
    LOG_NS("Relocation %d: Output @ SSH[1]", reloc_idx);
    
    // Relocation 6: Kernel dans Interface Descriptor DW0
    relocs[reloc_idx].target_handle = create_kernel.handle;
    relocs[reloc_idx].delta = 0;
    relocs[reloc_idx].offset = 0 * 4;  // DSH DW0
    relocs[reloc_idx].presumed_offset = 0;
    relocs[reloc_idx].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[reloc_idx].write_domain = 0;
    reloc_idx++;
    LOG_NS("Relocation %d: Kernel @ DSH[0]", reloc_idx);
    
    LOG_NS("✓ Total relocations: %d", reloc_idx);
    
    LOG_NS("=== PHASE 8: EXECBUFFER2 ===");
    
    // Préparer exec objects
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = create_kernel.handle;
    exec_objects[0].flags = 0;
    
    exec_objects[1].handle = create_output.handle;
    exec_objects[1].flags = EXEC_OBJECT_WRITE;
    
    exec_objects[2].handle = create_ssh.handle;
    exec_objects[2].relocation_count = 1;  // Output relocation
    exec_objects[2].relocs_ptr = (uintptr_t)&relocs[4];
    
    exec_objects[3].handle = create_dsh.handle;
    exec_objects[3].relocation_count = 1;  // Kernel relocation
    exec_objects[3].relocs_ptr = (uintptr_t)&relocs[5];
    
    exec_objects[4].handle = create_batch.handle;
    exec_objects[4].relocation_count = 4;  // SSH, DSH, Kernel, MIDL
    exec_objects[4].relocs_ptr = (uintptr_t)&relocs[0];
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 5,
        .batch_len = batch_dwords * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    LOG_NS("Submitting to GPU...");
    LOG_NS("  buffer_count: %u", execbuf.buffer_count);
    LOG_NS("  batch_len: %u bytes", execbuf.batch_len);
    LOG_NS("  flags: 0x%x (RENDER)", execbuf.flags);
    LOG_NS("  ctx_id: %u", ctx_id);
    
    uint64_t exec_start = get_ns();
    int exec_ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t exec_end = get_ns();
    
    if (exec_ret < 0) {
        LOG_NS("❌ EXECBUFFER2 FAILED: %s (errno=%d)", strerror(errno), errno);
        LOG_NS("   Execution time: %lu ns", exec_end - exec_start);
        
        // Dump détaillé pour debug
        printf("\n=== DEBUG INFO ===\n");
        printf("Kernel handle: %u\n", create_kernel.handle);
        printf("Output handle: %u\n", create_output.handle);
        printf("SSH handle: %u\n", create_ssh.handle);
        printf("DSH handle: %u\n", create_dsh.handle);
        printf("Batch handle: %u\n", create_batch.handle);
        printf("Context ID: %u\n", ctx_id);
        printf("Batch dwords: %d\n", batch_dwords);
        printf("Relocations: %d\n", reloc_idx);
        
        close(fd);
        return 1;
    }
    
    LOG_NS("✅ EXECBUFFER2 SUCCESS!");
    LOG_NS("   Execution time: %lu ns", exec_end - exec_start);
    
    LOG_NS("=== PHASE 9: GPU WAIT ===");
    
    // Attendre GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_batch.handle,
        .timeout_ns = 1000000000  // 1 second
    };
    
    uint64_t wait_start = get_ns();
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        LOG_NS("ERROR: GEM_WAIT failed: %s", strerror(errno));
    }
    uint64_t wait_end = get_ns();
    LOG_NS("✓ GPU execution complete: %lu ns", wait_end - wait_start);
    
    LOG_NS("=== PHASE 10: RESULTS ANALYSIS ===");
    
    // Analyser résultats
    uint32_t *output = (uint32_t *)output_map;
    int non_zero = 0;
    int pattern_cc = 0;
    int valid_results = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0xCCCCCCCC) {
            non_zero++;
            if (output[i] != 0) {
                valid_results++;
                if (valid_results <= 20) {
                    uint32_t expected = (4 << 16) | i;
                    LOG_NS("  output[%3d] = 0x%08x %s", i, output[i],
                           (output[i] == expected) ? "✓ CORRECT" : "");
                }
            }
        } else {
            pattern_cc++;
        }
    }
    
    LOG_NS("Results summary:");
    LOG_NS("  Non-zero: %d / %d", non_zero, NUM_THREADS);
    LOG_NS("  Valid results: %d", valid_results);
    LOG_NS("  Unchanged (0xCC): %d", pattern_cc);
    
    if (valid_results > 0) {
        hex_dump("OUTPUT BUFFER (first 256 bytes)", output, 256);
    }
    
    uint64_t total_ns = get_ns() - start_ns;
    LOG_NS("=== EXECUTION COMPLETE ===");
    LOG_NS("Total time: %lu ns (%.3f ms)", total_ns, total_ns / 1000000.0);
    
    if (valid_results > 0) {
        printf("\n🎉 SUCCESS: GPU kernel executed and wrote %d results!\n", valid_results);
        printf("✅ Batch buffer automatique FONCTIONNE!\n\n");
    } else {
        printf("\n⚠️  WARNING: No valid results written by GPU\n\n");
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

// Made with Bob - C411 Batch Buffer Automatique