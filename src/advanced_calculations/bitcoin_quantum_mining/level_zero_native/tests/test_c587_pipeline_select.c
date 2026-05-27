/*
 * TEST C587 - AJOUT MEDIA_PIPELINE_SELECT (ROOT CAUSE #132)
 * 
 * PRIORITÉ ABSOLUE #1 (30% probabilité cause principale)
 * 
 * DÉCOUVERTE FEEDBACK UTILISATEUR C587:
 * Le batch C584 manque MEDIA_PIPELINE_SELECT avant MEDIA_VFE_STATE.
 * Sans cette commande, le GPU parse le batch mais ne lance jamais le pipeline compute.
 * 
 * ROOT CAUSE #132: MEDIA_PIPELINE_SELECT manquant
 * - Symptôme: GPU hang sur MEDIA_VFE_STATE (IPEHR: 0x70020002)
 * - Cause: Pipeline compute jamais activé
 * - Solution: Ajouter MEDIA_PIPELINE_SELECT(GPGPU) avant MEDIA_VFE_STATE
 * 
 * SÉQUENCE CORRECTE Gen9:
 * 1. PIPE_CONTROL (flush initial)
 * 2. STATE_BASE_ADDRESS
 * 3. MEDIA_PIPELINE_SELECT (GPGPU) ← NOUVEAU!
 * 4. MEDIA_VFE_STATE
 * 5. MEDIA_INTERFACE_DESCRIPTOR_LOAD
 * 6. GPGPU_WALKER
 * 7. PIPE_CONTROL (flush final)
 * 
 * OBJECTIF: Valider output[0] = 0x12345678 via kernel ISA Gen9
 * 
 * Référence: Feedback utilisateur C587, Intel PRM Vol 2a Part 1
 * Made with Bob - Cycle C587 ADVANCED MODE
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
#define GEN9_MEDIA_PIPELINE_SELECT  0x69040000  // Opcode base
#define GEN9_MEDIA_VFE_STATE        0x70000000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER           0x71050000
#define GEN9_MI_BATCH_BUFFER_END    0x05000000

/* MEDIA_PIPELINE_SELECT modes (bits 0-1) */
#define PIPELINE_SELECT_3D          0  // 3D pipeline
#define PIPELINE_SELECT_MEDIA       1  // Media pipeline
#define PIPELINE_SELECT_GPGPU       2  // GPGPU compute pipeline

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
    
    printf("🚀 TEST C587 - AJOUT MEDIA_PIPELINE_SELECT (ROOT CAUSE #132)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("PRIORITÉ ABSOLUE #1: Activer pipeline GPGPU avant MEDIA_VFE_STATE\n");
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
    printf("✅ DRM device opened (fd=%d, %.3f µs)\n\n", drm_fd, (t1 - t0) / 1000.0);
    
    // 2. Create heap buffer (64KB)
    printf("[ 10%%] Creating heap buffer (64KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create heap_create = {
        .size = HEAP_SIZE,
        .handle = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &heap_create) < 0) {
        printf("❌ Failed to create heap buffer: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Heap buffer created (handle=%u, size=%lu, %.3f µs)\n\n", 
           heap_create.handle, heap_create.size, (t1 - t0) / 1000.0);
    
    // 3. Create batch buffer (4KB)
    printf("[ 15%%] Creating batch buffer (4KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create batch_create = {
        .size = BATCH_SIZE,
        .handle = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        printf("❌ Failed to create batch buffer: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer created (handle=%u, size=%lu, %.3f µs)\n\n", 
           batch_create.handle, batch_create.size, (t1 - t0) / 1000.0);
    
    // 4. Map heap buffer
    printf("[ 20%%] Mapping heap buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap heap_mmap = {
        .handle = heap_create.handle,
        .offset = 0,
        .size = HEAP_SIZE,
        .addr_ptr = 0,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &heap_mmap) < 0) {
        printf("❌ Failed to mmap heap: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint8_t *heap = (uint8_t *)(uintptr_t)heap_mmap.addr_ptr;
    memset(heap, 0, HEAP_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Heap mapped at %p (%.3f µs)\n\n", heap, (t1 - t0) / 1000.0);
    
    // 5. Map batch buffer
    printf("[ 25%%] Mapping batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .addr_ptr = 0,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap) < 0) {
        printf("❌ Failed to mmap batch: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)(uintptr_t)batch_mmap.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Batch mapped at %p (%.3f µs)\n\n", batch, (t1 - t0) / 1000.0);
    
    // 6. Setup heap layout
    printf("[ 30%%] Setting up heap layout...\n");
    t0 = get_timestamp_ns();
    
    // 6.1. Copy kernel ISA
    memcpy(heap + OFFSET_KERNEL_ISA, kernel_isa_gen9, KERNEL_ISA_SIZE);
    printf("  ✅ Kernel ISA copied to offset 0x%04x (%d bytes)\n", OFFSET_KERNEL_ISA, KERNEL_ISA_SIZE);
    
    // 6.2. Setup Surface State (64 bytes)
    uint32_t *surface_state = (uint32_t *)(heap + OFFSET_SURFACE_STATE);
    surface_state[0] = 0x00000000;  // Surface Type: BUFFER
    surface_state[1] = 0x00000100;  // Width=256, Height=1
    surface_state[2] = OFFSET_OUTPUT;  // Base Address Low (offset relatif heap base)
    surface_state[3] = 0x00000000;  // Base Address High
    printf("  ✅ Surface State setup at offset 0x%04x (output @ 0x%04x)\n",
           OFFSET_SURFACE_STATE, OFFSET_OUTPUT);
    
    // 6.3. Setup Binding Table (4 bytes)
    uint32_t *binding_table = (uint32_t *)(heap + OFFSET_BINDING_TABLE);
    binding_table[0] = OFFSET_SURFACE_STATE;  // Offset relatif heap base
    printf("  ✅ Binding Table setup at offset 0x%04x (BTI[0] = 0x%04x)\n", 
           OFFSET_BINDING_TABLE, OFFSET_SURFACE_STATE);
    
    // 6.4. Setup IDRT (32 bytes = 8 DWords)
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
    
    // 6.5. Initialize output buffer (sentinel)
    uint32_t *output = (uint32_t *)(heap + OFFSET_OUTPUT);
    output[0] = 0xDEADBEEF;  // Sentinel value
    printf("  ✅ Output buffer initialized at offset 0x%04x (sentinel: 0x%08x)\n", 
           OFFSET_OUTPUT, output[0]);
    
    t1 = get_timestamp_ns();
    printf("✅ Heap layout complete (%.3f µs)\n\n", (t1 - t0) / 1000.0);
    
    // 7. Build batch commands
    printf("[ 40%%] Building batch commands...\n");
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
    
    /* 3. MEDIA_PIPELINE_SELECT (GPGPU) - 1 DWord ← NOUVEAU! */
    printf("    [%2d] MEDIA_PIPELINE_SELECT (GPGPU) ← ROOT CAUSE #132 FIX!\n", offset);
    // MEDIA_PIPELINE_SELECT: opcode | (length-2) | pipeline_select
    // Length = 1 DWord, donc (1-2) = -1 = 0 (pas de length field pour cette commande)
    // Pipeline select dans bits 0-1
    batch[offset++] = GEN9_MEDIA_PIPELINE_SELECT | (0 << 8) | PIPELINE_SELECT_GPGPU;
    
    /* 4. MEDIA_VFE_STATE - 9 DWords */
    printf("    [%2d] MEDIA_VFE_STATE\n", offset);
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;  // Scratch Space Base
    batch[offset++] = 0;  // Scratch Space High
    batch[offset++] = 0;  // Stack Size / Threads
    batch[offset++] = 0;  // Max Threads
    batch[offset++] = 0;  // URB Entry Allocation
    batch[offset++] = 0;  // CURBE Allocation
    batch[offset++] = 0x00000100;  // Max threads = 256
    batch[offset++] = 0;  // Reserved
    
    /* 5. MEDIA_INTERFACE_DESCRIPTOR_LOAD - 4 DWords */
    printf("    [%2d] MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", offset);
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;  // Descriptor length
    batch[offset++] = OFFSET_IDRT;  // IDRT offset (relatif heap base)
    
    /* 6. GPGPU_WALKER - 15 DWords */
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
    
    /* 7. PIPE_CONTROL (flush final) - 6 DWords */
    printf("    [%2d] PIPE_CONTROL (flush final)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 8. MI_BATCH_BUFFER_END - 1 DWord */
    printf("    [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    t1 = get_timestamp_ns();
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n\n", 
           offset, offset * 4, (t1 - t0) / 1000.0);
    
    // 8. Create relocation entry (1 relocation STATE_BASE_ADDRESS)
    printf("[ 50%%] Creating relocation entry...\n");
    t0 = get_timestamp_ns();
    
    // IMPORTANT: sba_offset pointe sur le DWord 0 de STATE_BASE_ADDRESS (opcode)
    // Le General State Base Address est aux DWords 1-2 (64-bit)
    // Donc relocation offset = (sba_offset + 1) * 4 bytes
    int reloc_offset_dword = sba_offset + 1;
    
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = heap_create.handle,
        .delta = 0,  // Heap base
        .offset = reloc_offset_dword * 4,  // DWord 1 STATE_BASE_ADDRESS (en bytes)
        .read_domains = I915_GEM_DOMAIN_RENDER,
        .write_domain = I915_GEM_DOMAIN_RENDER,
        .presumed_offset = 0
    };
    
    printf("  Relocation entry:\n");
    printf("    target_handle: %u (heap)\n", reloc.target_handle);
    printf("    delta: 0x%x\n", reloc.delta);
    printf("    offset: 0x%llx (batch DWord %d = byte %d)\n",
           reloc.offset, reloc_offset_dword, reloc_offset_dword * 4);
    printf("    sba_offset: %d (STATE_BASE_ADDRESS starts at DWord %d)\n",
           sba_offset, sba_offset);
    printf("    read_domains: 0x%08x (RENDER)\n", reloc.read_domains);
    printf("    write_domain: 0x%08x (RENDER)\n", reloc.write_domain);
    
    t1 = get_timestamp_ns();
    printf("✅ Relocation entry created (%.3f µs)\n\n", (t1 - t0) / 1000.0);
    
    // 9. Setup exec_objects
    printf("[ 60%%] Setting up exec_objects...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = heap_create.handle,
            .relocation_count = 0,
            .relocs_ptr = 0,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 1,
            .relocs_ptr = (uintptr_t)&reloc,
            .alignment = 0,
            .offset = 0,
            .flags = 0,
            .rsvd1 = 0,
            .rsvd2 = 0
        }
    };
    
    printf("  exec_objects[0] (heap):\n");
    printf("    handle: %u\n", exec_objects[0].handle);
    printf("    relocation_count: %llu\n", exec_objects[0].relocation_count);
    printf("  exec_objects[1] (batch):\n");
    printf("    handle: %u\n", exec_objects[1].handle);
    printf("    relocation_count: %llu\n", exec_objects[1].relocation_count);
    printf("    relocs_ptr: %p\n", (void *)exec_objects[1].relocs_ptr);
    
    t1 = get_timestamp_ns();
    printf("✅ exec_objects setup complete (%.3f µs)\n\n", (t1 - t0) / 1000.0);
    
    // 10. Execute batch
    printf("[ 70%%] Executing batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 2,
        .batch_start_offset = 0,
        .batch_len = offset * 4,
        .DR1 = 0,
        .DR4 = 0,
        .num_cliprects = 0,
        .cliprects_ptr = 0,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    printf("  EXECBUFFER2 parameters:\n");
    printf("    buffer_count: %u\n", execbuf.buffer_count);
    printf("    batch_len: %u bytes (%u DWords)\n", execbuf.batch_len, offset);
    printf("    flags: 0x%llx (RENDER)\n", execbuf.flags);
    printf("\n  Submitting to GPU...\n");
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    t1 = get_timestamp_ns();
    
    if (ret < 0) {
        printf("❌ EXECBUFFER2 failed: %s (errno=%d)\n", strerror(errno), errno);
        printf("   This indicates batch was rejected by i915 driver\n\n");
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        close(drm_fd);
        return 1;
    }
    
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("   Batch accepted by i915 driver\n");
    printf("   GPU execution started\n\n");
    
    // 11. Wait for GPU completion
    printf("[ 80%%] Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_create.handle,
        .flags = 0,
        .timeout_ns = 10000000000ULL  // 10 seconds
    };
    
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
    
    t1 = get_timestamp_ns();
    
    if (ret < 0) {
        printf("❌ GPU wait failed: %s (errno=%d)\n", strerror(errno), errno);
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        close(drm_fd);
        return 1;
    }
    
    printf("✅ GPU execution completed (%.3f ms)\n\n", (t1 - t0) / 1000000.0);
    
    // 12. Read output
    printf("[ 90%%] Reading output buffer...\n");
    t0 = get_timestamp_ns();
    
    uint32_t result = output[0];
    
    t1 = get_timestamp_ns();
    printf("✅ Output read (%.3f µs)\n\n", (t1 - t0) / 1000.0);
    
    // 13. Validate result
    printf("[100%%] Validating result...\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("Expected: 0x12345678\n");
    printf("Got:      0x%08x\n", result);
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    uint64_t t_end = get_timestamp_ns();
    uint64_t total_time = t_end - t_start;
    
    if (result == 0x12345678) {
        printf("✅ SUCCESS! PREMIER WRITE GPU NATIF i915 VALIDÉ!\n");
        printf("   ROOT CAUSE #132 RÉSOLU: MEDIA_PIPELINE_SELECT ajouté\n");
        printf("   Pipeline GPGPU correctement activé\n");
        printf("   Threads EU dispatchés et exécutés\n");
        printf("   Output buffer écrit par GPU: 0x%08x\n", result);
        printf("\n🎉 MILESTONE: GPU natif i915 fonctionnel sans OpenCL!\n");
        printf("   Total time: %.3f ms\n", total_time / 1000000.0);
        printf("═══════════════════════════════════════════════════════════════════════\n");
        
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        close(drm_fd);
        return 0;
    } else if (result == 0xDEADBEEF) {
        printf("⚠️  PARTIAL: Batch exécuté mais kernel pas lancé\n");
        printf("   Output = 0xDEADBEEF (sentinel inchangé)\n");
        printf("   Possible causes:\n");
        printf("   - MEDIA_PIPELINE_SELECT encodage incorrect\n");
        printf("   - MEDIA_VFE_STATE configuration invalide\n");
        printf("   - GPGPU_WALKER paramètres incorrects\n");
        printf("   - Cache coherency (has_snoop=no)\n");
        printf("   Total time: %.3f ms\n", total_time / 1000000.0);
        printf("═══════════════════════════════════════════════════════════════════════\n");
        
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        close(drm_fd);
        return 1;
    } else {
        printf("⚠️  UNEXPECTED: Output modifié mais valeur incorrecte\n");
        printf("   Expected: 0x12345678\n");
        printf("   Got:      0x%08x\n", result);
        printf("   Ceci indique que le GPU a écrit mais avec une valeur incorrecte\n");
        printf("   Total time: %.3f ms\n", total_time / 1000000.0);
        printf("═══════════════════════════════════════════════════════════════════════\n");
        
        munmap(heap, HEAP_SIZE);
        munmap(batch, BATCH_SIZE);
        close(drm_fd);
        return 1;
    }
}

// Made with Bob
