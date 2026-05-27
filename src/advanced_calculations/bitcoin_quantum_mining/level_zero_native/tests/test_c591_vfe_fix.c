/*
 * TEST C591 - FIX MEDIA_VFE_STATE - PREMIER WRITE GPU NATIF i915 AVEC RELOCATIONS
 * 
 * SOLUTION ROOT CAUSE #117: 1 relocation STATE_BASE_ADDRESS + offsets relatifs
 * 
 * ARCHITECTURE (Spécifications i915 officielles):
 * - 2 buffers: heap (64KB) + batch (4KB)
 * - 1 SEULE relocation: STATE_BASE_ADDRESS DWord 1 → heap base
 * - Offsets relatifs internes: IDRT, Binding Table, Surface State
 * - Memory domains: I915_GEM_DOMAIN_RENDER (compute workloads)
 * 
 * HEAP LAYOUT (64KB):
 *   Offset 0x0000: Kernel ISA Gen9 (4KB, aligné 64 bytes)
 *   Offset 0x1000: Surface State (64 bytes, aligné 64 bytes)
 *   Offset 0x1100: Binding Table (4 bytes, aligné 32 bytes)
 *   Offset 0x1200: IDRT (32 bytes, aligné 64 bytes)
 *   Offset 0x2000: Output buffer (4KB)
 * 
 * OBJECTIF: Valider output[0] = 0x12345678 via kernel ISA Gen9
 * 
 * Référence: ANALYSE_INTEL_I915_DRM_C584.md
 * Made with Bob - Cycle C584 ADVANCED MODE
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <drm/i915_drm.h>
#include <errno.h>

// Heap layout (64KB)
#define HEAP_SIZE           (64 * 1024)
#define OFFSET_KERNEL_ISA   0x0000  // 4KB kernel ISA (aligné 64 bytes)
#define OFFSET_SURFACE_STATE 0x1000  // 64 bytes (aligné 64 bytes)
#define OFFSET_BINDING_TABLE 0x1100  // 4 bytes (aligné 32 bytes)
#define OFFSET_IDRT         0x1200  // 32 bytes (aligné 64 bytes)
#define OFFSET_OUTPUT       0x2000  // 4KB output buffer

// Batch buffer (4KB)
#define BATCH_SIZE          4096

// Kernel ISA size
#define KERNEL_ISA_SIZE     128

/* Gen9 Command Opcodes */
#define GEN9_PIPE_CONTROL           0x7a000000
#define GEN9_STATE_BASE_ADDRESS     0x61010000
#define GEN9_MEDIA_VFE_STATE        0x70000000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER           0x71050000
#define GEN9_MI_BATCH_BUFFER_END    0x05000000

/* Kernel ISA Gen9 (128 bytes) - output[0] = 0x12345678 */
static const unsigned char kernel_isa_gen9[KERNEL_ISA_SIZE] = {
    0x01, 0x00, 0x60, 0x00, 0x0c, 0x02, 0x40, 0x20,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x30,
    0x00, 0x10, 0x00, 0x16, 0xc0, 0x04, 0xc0, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x43, 0x80, 0x20,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x2c, 0x0e, 0xc0, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12,  // 0x12345678 little-endian
    0x01, 0x4d, 0x00, 0x20, 0x07, 0x7f, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    uint64_t t_start = get_timestamp_ns();
    
    printf("🚀 TEST C584 - PREMIER WRITE GPU NATIF i915 (RELOCATIONS)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    // 1. Open DRM device
    printf("[  5%%] Opening DRM device...\n");
    uint64_t t0 = get_timestamp_ns();
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        printf("❌ Failed to open DRM device: %s (errno=%d)\n", strerror(errno), errno);
        return 1;
    }
    
    uint64_t t1 = get_timestamp_ns();
    printf("✅ DRM device opened: fd=%d (%.3f µs)\n\n", drm_fd, (t1 - t0) / 1000.0);
    
    // 2. Create GEM Context
    printf("[ 10%%] Creating GEM Context...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ GEM_CONTEXT_CREATE failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GEM Context created: ctx_id=%u (%.3f µs)\n\n", ctx_create.ctx_id, (t1 - t0) / 1000.0);
    
    // 3. Create heap buffer (64KB)
    printf("[ 20%%] Creating heap buffer (64KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create heap_create = {
        .size = HEAP_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &heap_create) < 0) {
        printf("❌ GEM_CREATE (heap) failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Heap buffer created: handle=%u, size=%llu (%.3f µs)\n\n",
           heap_create.handle, heap_create.size, (t1 - t0) / 1000.0);
    
    // 4. Create batch buffer (4KB)
    printf("[ 25%%] Creating batch buffer (4KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create batch_create = {
        .size = BATCH_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        printf("❌ GEM_CREATE (batch) failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer created: handle=%u, size=%llu (%.3f µs)\n\n",
           batch_create.handle, batch_create.size, (t1 - t0) / 1000.0);
    
    // 5. Map heap buffer
    printf("[ 30%%] Mapping heap buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap heap_mmap = {
        .handle = heap_create.handle,
        .offset = 0,
        .size = HEAP_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &heap_mmap) < 0) {
        printf("❌ GEM_MMAP (heap) failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint8_t *heap = (uint8_t *)heap_mmap.addr_ptr;
    memset(heap, 0, HEAP_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Heap buffer mapped: %p (%.3f µs)\n\n", heap, (t1 - t0) / 1000.0);
    
    // 6. Map batch buffer
    printf("[ 35%%] Mapping batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap) < 0) {
        printf("❌ GEM_MMAP (batch) failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)batch_mmap.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer mapped: %p (%.3f µs)\n\n", batch, (t1 - t0) / 1000.0);
    
    // 7. Setup heap layout
    printf("[ 40%%] Setting up heap layout...\n");
    t0 = get_timestamp_ns();
    
    // 7.1. Copy kernel ISA
    memcpy(heap + OFFSET_KERNEL_ISA, kernel_isa_gen9, KERNEL_ISA_SIZE);
    printf("  ✅ Kernel ISA copied to offset 0x%04x (%d bytes)\n", OFFSET_KERNEL_ISA, KERNEL_ISA_SIZE);
    
    // 7.2. Setup Surface State (64 bytes)
    uint32_t *surface_state = (uint32_t *)(heap + OFFSET_SURFACE_STATE);
    surface_state[0] = 0x00000000;  // Surface Type: BUFFER
    surface_state[1] = 0x00000100;  // Width=256, Height=1
    surface_state[2] = OFFSET_OUTPUT;  // Base Address Low (offset relatif heap base)
    surface_state[3] = 0x00000000;  // Base Address High
    // Reste à 0
    printf("  ✅ Surface State setup at offset 0x%04x (output @ 0x%04x)\n",
           OFFSET_SURFACE_STATE, OFFSET_OUTPUT);
    
    // 7.3. Setup Binding Table (4 bytes)
    uint32_t *binding_table = (uint32_t *)(heap + OFFSET_BINDING_TABLE);
    binding_table[0] = OFFSET_SURFACE_STATE;  // Offset relatif heap base
    printf("  ✅ Binding Table setup at offset 0x%04x (BTI[0] = 0x%04x)\n", 
           OFFSET_BINDING_TABLE, OFFSET_SURFACE_STATE);
    
    // 7.4. Setup IDRT (32 bytes = 8 DWords)
    uint32_t *idrt = (uint32_t *)(heap + OFFSET_IDRT);
    idrt[0] = OFFSET_KERNEL_ISA;  // Kernel Start Pointer (relatif heap base)
    idrt[1] = 0x00000000;  // Reserved
    idrt[2] = 0x00000000;  // Sampler State Pointer
    idrt[3] = (OFFSET_BINDING_TABLE & 0xFFFFFFE0);  // Binding Table Pointer (5-bit aligned)
    idrt[4] = 0x00000000;  // CURBE Read Offset/Length
    idrt[5] = 0x00000100;  // Num Threads = 256
    idrt[6] = 0x00000000;  // Cross-thread Constant Data
    idrt[7] = 0x00000000;  // Reserved
    printf("  ✅ IDRT setup at offset 0x%04x\n", OFFSET_IDRT);
    printf("      Kernel Start: 0x%04x\n", idrt[0]);
    printf("      Binding Table: 0x%04x\n", idrt[3]);
    
    // 7.5. Initialize output buffer (sentinel)
    uint32_t *output = (uint32_t *)(heap + OFFSET_OUTPUT);
    output[0] = 0xDEADBEEF;  // Sentinel value
    printf("  ✅ Output buffer initialized at offset 0x%04x (sentinel: 0x%08x)\n", 
           OFFSET_OUTPUT, output[0]);
    
    t1 = get_timestamp_ns();
    printf("✅ Heap layout complete (%.3f µs)\n\n", (t1 - t0) / 1000.0);
    
    // 8. Build batch commands
    printf("[ 50%%] Building batch commands...\n");
    t0 = get_timestamp_ns();
    
    int offset = 0;
    int sba_offset = 0;  // STATE_BASE_ADDRESS offset (pour relocation)
    
    printf("  Building batch commands:\n");
    
    /* 1. PIPE_CONTROL (flush initial) - 6 DWords */
    printf("    [%2d] PIPE_CONTROL (flush initial)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 2. STATE_BASE_ADDRESS - 19 DWords */
    /* ← 1 RELOCATION ICI (DWord 1-2: General State Base) */
    printf("    [%2d] STATE_BASE_ADDRESS (relocation target)\n", offset);
    sba_offset = offset;
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
    batch[offset++] = 0;  // General State Base Low (patché par relocation)
    batch[offset++] = 0;  // General State Base High (patché par relocation)
    // DWords 3-18: Autres bases (tous à 0 pour version minimale)
    for (int i = 0; i < 16; i++) {
        batch[offset++] = 0;
    }
    
    /* 3. MEDIA_VFE_STATE - 9 DWords - FIX C591: Configuration OpenCL */
    printf("    [%2d] MEDIA_VFE_STATE (FIX C591 OpenCL config)\n", offset);
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;  // Scratch Space Base
    batch[offset++] = 0;  // Scratch Space High
    batch[offset++] = 0x00a70100;  // FIX C591: DWord 3 = Max Threads=256, URB Entries=167
    batch[offset++] = 0;  // Max Threads (unused in DWord 4)
    batch[offset++] = 0x07820000;  // FIX C591: DWord 5 = URB Entry Size
    batch[offset++] = 0;  // CURBE Allocation
    batch[offset++] = 0x00000100;  // Max threads = 256
    batch[offset++] = 0;  // Reserved
    
    /* 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD - 4 DWords */
    printf("    [%2d] MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", offset);
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;  // Descriptor length
    batch[offset++] = OFFSET_IDRT;  // IDRT offset (relatif heap base)
    
    /* 5. GPGPU_WALKER - 15 DWords */
    printf("    [%2d] GPGPU_WALKER\n", offset);
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;  // Interface descriptor offset
    batch[offset++] = 0;  // Indirect data length
    batch[offset++] = 0;  // Indirect data start
    batch[offset++] = 1;  // Thread width
    batch[offset++] = 1;  // Thread height
    batch[offset++] = 1;  // Thread depth
    batch[offset++] = 1;  // Thread group width
    batch[offset++] = 1;  // Thread group height
    batch[offset++] = 1;  // Thread group depth
    batch[offset++] = 0xFFFFFFFF;  // Right execution mask
    batch[offset++] = 0xFFFFFFFF;  // Bottom execution mask
    batch[offset++] = 0;  // SIMD size
    batch[offset++] = 0;  // Local X/Y/Z
    batch[offset++] = 0;  // Reserved
    
    /* 6. PIPE_CONTROL (flush final) - 6 DWords */
    printf("    [%2d] PIPE_CONTROL (flush final)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 7. MI_BATCH_BUFFER_END - 1 DWord */
    printf("    [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    t1 = get_timestamp_ns();
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n\n", 
           offset, offset * 4, (t1 - t0) / 1000.0);
    
    // 9. Create relocation entries (2 relocations)
    printf("[ 60%%] Creating relocation entries...\n");
    t0 = get_timestamp_ns();
    
    int midl_offset = sba_offset + 19 + 9 + 3;  // Après SBA(19) + VFE(9) + MIDL opcode+reserved+length(3)
    
    struct drm_i915_gem_relocation_entry relocs[2] = {
        {
            .target_handle = heap_create.handle,
            .delta = 0,  // Heap base
            .offset = (sba_offset + 1) * 4,  // DWord 7 STATE_BASE_ADDRESS
            .read_domains = I915_GEM_DOMAIN_RENDER,
            .write_domain = I915_GEM_DOMAIN_RENDER,
            .presumed_offset = 0
        },
        {
            .target_handle = heap_create.handle,
            .delta = OFFSET_IDRT,  // IDRT offset dans heap
            .offset = (midl_offset) * 4,  // DWord 3 MEDIA_INTERFACE_DESCRIPTOR_LOAD
            .read_domains = I915_GEM_DOMAIN_RENDER,
            .write_domain = 0,
            .presumed_offset = 0
        }
    };
    
    t1 = get_timestamp_ns();
    printf("✅ Relocations created (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  [0] STATE_BASE_ADDRESS:\n");
    printf("      Target: heap, Delta: 0x%llx, Offset: 0x%llx (DWord %d)\n",
           relocs[0].delta, relocs[0].offset, (int)(relocs[0].offset / 4));
    printf("  [1] MEDIA_INTERFACE_DESCRIPTOR_LOAD:\n");
    printf("      Target: heap, Delta: 0x%llx, Offset: 0x%llx (DWord %d)\n\n",
           relocs[1].delta, relocs[1].offset, (int)(relocs[1].offset / 4));
    
    // 10. Prepare exec objects
    printf("[ 70%%] Preparing EXECBUFFER2...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = heap_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = EXEC_OBJECT_WRITE,  // Heap modifié par GPU
            .rsvd1 = 0,
            .rsvd2 = 0
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 2,  // 2 relocations
            .relocs_ptr = (uint64_t)relocs,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };
    
    printf("  Configuration:\n");
    printf("    Context ID: %u\n", ctx_create.ctx_id);
    printf("    Ring: RCS (Render)\n");
    printf("    Buffer count: %u (heap + batch)\n", execbuf.buffer_count);
    printf("    Batch length: %u bytes\n", execbuf.batch_len);
    printf("    Relocations: 2 (STATE_BASE_ADDRESS + MIDL → heap)\n\n");
    
    // 11. Submit to GPU
    printf("[ 80%%] Submitting to GPU...\n");
    t0 = get_timestamp_ns();
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d)\n", strerror(errno), errno);
        t1 = get_timestamp_ns();
        printf("   Duration: %.3f µs\n\n", (t1 - t0) / 1000.0);
        
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C584 ÉCHOUÉ                                      ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        
        // Cleanup
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        
        struct drm_i915_gem_context_destroy ctx_destroy = {
            .ctx_id = ctx_create.ctx_id
        };
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        
        struct drm_gem_close close_heap = { .handle = heap_create.handle };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_heap);
        
        struct drm_gem_close close_batch = { .handle = batch_create.handle };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
        
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  Heap GTT:  0x%016llx\n", (unsigned long long)exec_objects[0].offset);
    printf("  Batch GTT: 0x%016llx\n\n", (unsigned long long)exec_objects[1].offset);
    
    // 12. Wait for GPU completion
    printf("[ 90%%] Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = heap_create.handle,
        .timeout_ns = 5000000000ULL,  // 5 seconds
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        printf("⚠️  WAIT failed: %s (errno=%d)\n", strerror(errno), errno);
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GPU execution completed (%.3f ms)\n\n", (t1 - t0) / 1000000.0);
    
    // 13. Read output buffer
    printf("[ 95%%] Reading output buffer...\n");
    t0 = get_timestamp_ns();
    
    uint32_t result = output[0];
    
    t1 = get_timestamp_ns();
    printf("  Output[0] = 0x%08x (%.3f µs)\n\n", result, (t1 - t0) / 1000.0);
    
    // 14. Validate result
    printf("[100%%] Validating result...\n");
    
    uint64_t t_total = get_timestamp_ns() - t_start;
    
    if (result == 0x12345678) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ TEST C584 RÉUSSI - PREMIER WRITE GPU NATIF i915!     ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  ⏱️  Temps total: %.3f ms                                 ║\n", t_total / 1000000.0);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  ⏱️  Temps total: %.3f ms                                 ║\n", t_total / 1000000.0);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    // 15. Cleanup
    printf("\nCleanup...\n");
    
    munmap(heap, HEAP_SIZE);
    munmap(batch, BATCH_SIZE);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("✅ Context destroyed\n");
    
    struct drm_gem_close close_heap = { .handle = heap_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_heap);
    
    struct drm_gem_close close_batch = { .handle = batch_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_batch);
    
    close(drm_fd);
    
    printf("\n✅ TEST C584 TERMINÉ\n");
    
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob - Cycle C584 ADVANCED MODE