/*
 * TEST C581 - PREMIER WRITE GPU NATIF i915 - ARCHITECTURE UNIFIÉE
 * 
 * SOLUTION ROOT CAUSE #111: 1 BUFFER UNIFIÉ (comme OpenCL)
 * 
 * ARCHITECTURE MÉMOIRE UNIFIÉE:
 * - 1 seul buffer GEM (16KB) contenant TOUT
 * - Layout unifié garantit allocation GTT unique
 * - Pas de problème de références croisées entre buffers
 * 
 * LAYOUT BUFFER UNIFIÉ (16KB):
 *   Offset 0x0000: Kernel ISA Gen9 (128 bytes)
 *   Offset 0x0100: Surface State (64 bytes)
 *   Offset 0x0200: Binding Table (4 bytes)
 *   Offset 0x0300: IDRT (32 bytes)
 *   Offset 0x1000: Output buffer (4KB)
 *   Offset 0x2000: Batch commands (4KB)
 * 
 * OBJECTIF: Valider output[0] = 0x12345678 via kernel ISA Gen9
 * 
 * Made with Bob - Cycle C581
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

#define UNIFIED_BUFFER_SIZE (16 * 1024)  // 16KB total

// Offsets dans le buffer unifié
// CRITIQUE: Batch DOIT être à offset 0 (batch_start_offset=0)
#define OFFSET_BATCH         0x0000  // Batch commands (4KB)
#define OFFSET_KERNEL_ISA    0x1000  // Kernel ISA (128 bytes)
#define OFFSET_SURFACE_STATE 0x1100  // Surface State (64 bytes)
#define OFFSET_BINDING_TABLE 0x1200  // Binding Table (4 bytes)
#define OFFSET_IDRT          0x1300  // IDRT (32 bytes)
#define OFFSET_OUTPUT        0x2000  // Output buffer (4KB)

#define KERNEL_ISA_SIZE 128

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
    
    printf("🚀 TEST C581 - ARCHITECTURE UNIFIÉE (SOLUTION ROOT CAUSE #111)\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // 1. Open DRM device
    printf("[  5%%] Opening DRM device /dev/dri/card1...\n");
    uint64_t t0 = get_timestamp_ns();
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        printf("❌ Failed to open DRM device: %s (errno=%d)\n", strerror(errno), errno);
        return 1;
    }
    
    uint64_t t1 = get_timestamp_ns();
    printf("✅ DRM device opened: fd=%d (%.3f µs)\n\n", drm_fd, (t1-t0)/1000.0);
    
    // 2. Create GEM Context
    printf("[ 10%%] Creating GEM Context...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ Context creation failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GEM Context created: ctx_id=%u (%.3f µs)\n\n", ctx_create.ctx_id, (t1-t0)/1000.0);
    
    // 3. Create unified buffer (16KB)
    printf("[ 15%%] Creating unified buffer (16KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create unified_create = { .size = UNIFIED_BUFFER_SIZE };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &unified_create) < 0) {
        printf("❌ Unified buffer creation failed\n");
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Unified BO: handle=%u, size=%llu (%.3f µs)\n\n", 
           unified_create.handle, unified_create.size, (t1-t0)/1000.0);
    
    // 4. Map unified buffer
    printf("[ 20%%] Mapping unified buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap unified_mmap = {
        .handle = unified_create.handle,
        .offset = 0,
        .size = UNIFIED_BUFFER_SIZE,
        .flags = 0
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &unified_mmap) < 0) {
        printf("❌ Unified buffer mmap failed\n");
        close(drm_fd);
        return 1;
    }
    
    uint8_t *unified = (uint8_t *)unified_mmap.addr_ptr;
    
    t1 = get_timestamp_ns();
    printf("✅ Unified buffer mapped: %p (%.3f µs)\n\n", unified, (t1-t0)/1000.0);
    
    // 5. Setup unified buffer layout
    printf("[ 30%%] Setting up unified buffer layout...\n");
    t0 = get_timestamp_ns();
    
    memset(unified, 0, UNIFIED_BUFFER_SIZE);
    
    // Kernel ISA at offset 0x1000
    memcpy(unified + OFFSET_KERNEL_ISA, kernel_isa_gen9, KERNEL_ISA_SIZE);
    printf("  ✅ Kernel ISA at offset 0x%04x (128 bytes)\n", OFFSET_KERNEL_ISA);
    
    // Surface State at offset 0x100
    uint32_t *surface_state = (uint32_t *)(unified + OFFSET_SURFACE_STATE);
    surface_state[0] = OFFSET_OUTPUT;  // Base address (offset relatif dans buffer unifié)
    surface_state[1] = 0x00000000;     // Base address high
    surface_state[2] = 0x00000FFF;     // Width/Height (4KB)
    surface_state[3] = 0x00000000;     // Depth/Pitch
    surface_state[4] = 0x00000000;     // Min LOD
    surface_state[5] = 0x00000000;     // Mip count
    surface_state[6] = 0x00000000;     // X/Y offset
    surface_state[7] = 0x00000000;     // Surface format
    for (int i = 8; i < 16; i++) surface_state[i] = 0x00000000;
    printf("  ✅ Surface State at offset 0x%04x (64 bytes)\n", OFFSET_SURFACE_STATE);
    
    // Binding Table at offset 0x1200
    uint32_t *binding_table = (uint32_t *)(unified + OFFSET_BINDING_TABLE);
    binding_table[0] = OFFSET_SURFACE_STATE;  // Offset relatif vers Surface State
    printf("  ✅ Binding Table at offset 0x%04x (4 bytes)\n", OFFSET_BINDING_TABLE);
    
    // IDRT at offset 0x1300
    uint32_t *idrt = (uint32_t *)(unified + OFFSET_IDRT);
    idrt[0] = OFFSET_KERNEL_ISA;  // Kernel start pointer (offset relatif)
    idrt[1] = 0x00000000;         // Kernel start pointer high
    idrt[2] = 0x00000000;         // Reserved
    idrt[3] = 0x00000000;         // Sampler state pointer
    idrt[4] = 0x00000000;         // Sampler count
    idrt[5] = OFFSET_BINDING_TABLE;  // Binding table pointer (offset relatif)
    idrt[6] = 0x00000001;         // Binding table entry count
    idrt[7] = 0x00000000;         // Thread group size
    printf("  ✅ IDRT at offset 0x%04x (32 bytes)\n", OFFSET_IDRT);
    
    // Output buffer at offset 0x2000 (clear)
    memset(unified + OFFSET_OUTPUT, 0, 4096);
    printf("  ✅ Output buffer at offset 0x%04x (4KB, cleared)\n", OFFSET_OUTPUT);
    
    t1 = get_timestamp_ns();
    printf("  ⏱️  Layout setup: %.3f µs\n\n", (t1-t0)/1000.0);
    
    // 6. Build batch at offset 0x0000
    printf("[ 40%%] Building batch compute Gen9 at offset 0x%04x...\n", OFFSET_BATCH);
    t0 = get_timestamp_ns();
    
    uint32_t *batch = (uint32_t *)(unified + OFFSET_BATCH);
    int offset = 0;
    
    // Command 1: PIPE_CONTROL (flush initial)
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // Command 2: STATE_BASE_ADDRESS (19 DWords)
    // CRITIQUE: Toutes les adresses pointent vers le MÊME buffer unifié
    int state_base_addr_offset = offset;
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
    // DWord 1-2: General State Base Address (buffer unifié) - RELOCATION
    batch[offset++] = 0x00000000;  // Patched to unified GTT
    batch[offset++] = 0x00000000;
    // DWord 3-4: Surface State Base Address (buffer unifié) - RELOCATION
    batch[offset++] = 0x00000000;  // Patched to unified GTT
    batch[offset++] = 0x00000000;
    // DWord 5-6: Dynamic State Base Address (buffer unifié) - RELOCATION
    batch[offset++] = 0x00000000;  // Patched to unified GTT
    batch[offset++] = 0x00000000;
    // DWord 7-8: Indirect Object Base Address
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    // DWord 9-10: Instruction Base Address (buffer unifié) - RELOCATION
    batch[offset++] = 0x00000000;  // Patched to unified GTT
    batch[offset++] = 0x00000000;
    // DWord 11-18: Bounds and enables
    for (int i = 0; i < 8; i++) {
        batch[offset++] = 0x00000000;
    }
    
    // Command 3: MEDIA_VFE_STATE (9 DWords)
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // Command 4: MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0x00000000;
    batch[offset++] = 32;  // Interface descriptor length
    batch[offset++] = OFFSET_IDRT;  // Offset relatif vers IDRT
    
    // Command 5: GPGPU_WALKER (15 DWords)
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0x00000000;  // Interface descriptor offset
    batch[offset++] = 0x00000000;  // Indirect data length
    batch[offset++] = 0x00000000;  // Indirect data start address
    batch[offset++] = 0x00000001;  // Thread width (1 thread X)
    batch[offset++] = 0x00000001;  // Thread height (1 thread Y)
    batch[offset++] = 0x00000001;  // Thread depth (1 thread Z)
    batch[offset++] = 0x00000000;  // Thread group ID starting X
    batch[offset++] = 0x00000000;  // Thread group ID X dimension
    batch[offset++] = 0x00000000;  // Thread group ID starting Y
    batch[offset++] = 0x00000001;  // Thread group ID Y dimension
    batch[offset++] = 0x00000000;  // Thread group ID starting Z
    batch[offset++] = 0x00000001;  // Thread group ID Z dimension
    batch[offset++] = 0x00000000;  // Right execution mask
    batch[offset++] = 0xFFFFFFFF;  // Bottom execution mask
    
    // Command 6: PIPE_CONTROL (flush final)
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // Command 7: MI_BATCH_BUFFER_END
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    batch[offset++] = 0x00000000;
    
    t1 = get_timestamp_ns();
    printf("  ✅ Batch built: %d DWords (%d bytes, %.3f µs)\n\n", offset, offset * 4, (t1-t0)/1000.0);
    
    // 7. Setup relocations (4 entries: STATE_BASE_ADDRESS uniquement)
    printf("[ 50%%] Setting up relocations (4 entries → unified buffer)...\n");
    
    struct drm_i915_gem_relocation_entry relocs[4];
    
    // Relocation 1: General State Base Address
    // CRITIQUE: offset est RELATIF au batch_start_offset (0-based)
    relocs[0].target_handle = unified_create.handle;
    relocs[0].delta = 0;
    relocs[0].offset = (state_base_addr_offset + 1) * 4;  // Relatif au début du batch
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[0].write_domain = 0;
    
    // Relocation 2: Surface State Base Address
    relocs[1].target_handle = unified_create.handle;
    relocs[1].delta = 0;
    relocs[1].offset = (state_base_addr_offset + 3) * 4;
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[1].write_domain = 0;
    
    // Relocation 3: Dynamic State Base Address
    relocs[2].target_handle = unified_create.handle;
    relocs[2].delta = 0;
    relocs[2].offset = (state_base_addr_offset + 5) * 4;
    relocs[2].presumed_offset = 0;
    relocs[2].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[2].write_domain = 0;
    
    // Relocation 4: Instruction Base Address
    relocs[3].target_handle = unified_create.handle;
    relocs[3].delta = 0;
    relocs[3].offset = (state_base_addr_offset + 9) * 4;
    relocs[3].presumed_offset = 0;
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[3].write_domain = 0;
    
    printf("  ✅ All relocations point to unified buffer (handle=%u)\n\n", unified_create.handle);
    
    // 8. Prepare exec object (1 seul buffer!)
    printf("[ 55%%] Preparing execution (1 unified buffer)...\n");
    
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = unified_create.handle,
        .relocation_count = 4,  // 4 relocations STATE_BASE_ADDRESS
        .relocs_ptr = (uint64_t)relocs,
        .alignment = 0,
        .offset = 0,
        .flags = EXEC_OBJECT_WRITE,  // Buffer contient output
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // 9. Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,  // 1 SEUL BUFFER!
        .batch_start_offset = 0,  // Batch à offset 0 (OBLIGATOIRE)
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };
    
    printf("  Context ID: %u\n", ctx_create.ctx_id);
    printf("  Ring: RCS (Render)\n");
    printf("  Batch offset: 0x%04x (MUST be 0)\n", OFFSET_BATCH);
    printf("  Batch length: %u bytes\n", execbuf.batch_len);
    printf("  Buffer count: 1 (UNIFIED)\n\n");
    
    // 10. Submit to GPU
    printf("[ 65%%] Submitting to GPU...\n");
    t0 = get_timestamp_ns();
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1-t0)/1000.0);
    printf("  Unified GTT: 0x%016llx\n\n", (unsigned long long)exec_object.offset);
    
    // 11. Wait for completion
    printf("[ 75%%] Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = unified_create.handle,
        .timeout_ns = 5000000000ULL,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        printf("⚠️  WAIT failed: %s (errno=%d)\n", strerror(errno), errno);
    } else {
        t1 = get_timestamp_ns();
        printf("✅ GPU execution completed (%.3f ms)\n\n", (t1-t0)/1000000.0);
    }
    
    // 12. Read output from unified buffer
    printf("[ 85%%] Reading output from unified buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_set_domain set_domain = {
        .handle = unified_create.handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    
    uint32_t *output = (uint32_t *)(unified + OFFSET_OUTPUT);
    uint32_t result = output[0];
    t1 = get_timestamp_ns();
    printf("  Output[0] = 0x%08x (%.3f µs)\n\n", result, (t1-t0)/1000.0);
    
    // 13. Validate result
    printf("[ 92%%] Validating result...\n");
    
    uint64_t t_end = get_timestamp_ns();
    double total_ms = (t_end - t_start) / 1000000.0;
    
    if (result == 0x12345678) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  🎉 MILESTONE HISTORIQUE ATTEINT! 🎉                      ║\n");
        printf("║  ✅ PREMIER WRITE GPU NATIF i915 VALIDÉ                  ║\n");
        printf("║  ✅ Architecture unifiée fonctionnelle                   ║\n");
        printf("║  ✅ ROOT CAUSE #111 RÉSOLUE                              ║\n");
        printf("║  ✅ EU execution réelle confirmée                        ║\n");
        printf("║  ✅ Kernel ISA Gen9 fonctionnel                          ║\n");
        printf("║  ✅ Infrastructure i915 native opérationnelle            ║\n");
        printf("║  ⏱️  Temps total: %.3f ms                                ║\n", total_ms);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("║  Unified GTT: 0x%016llx                          ║\n", (unsigned long long)exec_object.offset);
        printf("║  ⏱️  Temps total: %.3f ms                                ║\n", total_ms);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    // 14. Cleanup
    printf("\n[ 98%%] Cleanup...\n");
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("  ✅ Context destroyed\n");
    
    struct drm_gem_close close_unified = { .handle = unified_create.handle };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_unified);
    
    munmap(unified, UNIFIED_BUFFER_SIZE);
    close(drm_fd);
    
    printf("\n✅ TEST C581 TERMINÉ\n");
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob - Solution ROOT CAUSE #111