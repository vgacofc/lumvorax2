/*
 * TEST C582 - PASS 1: PREMIER WRITE GPU NATIF i915 (output[0] = 0x12345678)
 * 
 * SOLUTION ROOT CAUSE #115: Architecture C579 (SANS relocations) + kernel ISA
 * 
 * ARCHITECTURE UNIFIÉE (16KB):
 * - 1 buffer unifié contenant TOUT
 * - AUCUNE relocation (clé du succès C579)
 * - Offsets absolus codés en dur
 * - Batch compute Gen9 complet
 * 
 * LAYOUT BUFFER UNIFIÉ:
 *   Offset 0x0000: Batch commands (4KB)
 *   Offset 0x1000: Kernel ISA Gen9 (128 bytes)
 *   Offset 0x1100: Surface State (64 bytes)
 *   Offset 0x1200: Binding Table (4 bytes)
 *   Offset 0x1300: IDRT (32 bytes)
 *   Offset 0x2000: Output buffer (4KB)
 * 
 * OBJECTIF: Valider output[0] = 0x12345678 via kernel ISA Gen9
 * 
 * Made with Bob - Cycle C582 PASS 1
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
    
    printf("🚀 TEST C582 PASS 1 - PREMIER WRITE GPU NATIF i915\n");
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
    printf("✅ DRM device opened: fd=%d (%.3f µs)\n", drm_fd, (t1 - t0) / 1000.0);
    
    // 2. Create GEM Context
    printf("\n[ 10%%] Creating GEM Context...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        printf("❌ GEM_CONTEXT_CREATE failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ GEM Context created: ctx_id=%u (%.3f µs)\n", ctx_create.ctx_id, (t1 - t0) / 1000.0);
    
    // 3. Create unified buffer
    printf("\n[ 20%%] Creating unified buffer (16KB)...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create unified_create = {
        .size = UNIFIED_BUFFER_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &unified_create) < 0) {
        printf("❌ GEM_CREATE failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Unified buffer created: handle=%u, size=%llu (%.3f µs)\n", 
           unified_create.handle, (unsigned long long)unified_create.size, (t1 - t0) / 1000.0);
    
    // 4. Map unified buffer
    printf("\n[ 30%%] Mapping unified buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = unified_create.handle,
        .offset = 0,
        .size = UNIFIED_BUFFER_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        printf("❌ GEM_MMAP failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint8_t *unified_buffer = (uint8_t *)mmap_arg.addr_ptr;
    memset(unified_buffer, 0, UNIFIED_BUFFER_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Unified buffer mapped: %p (%.3f µs)\n", unified_buffer, (t1 - t0) / 1000.0);
    
    // 5. Setup unified buffer layout
    printf("\n[ 40%%] Setting up unified buffer layout...\n");
    t0 = get_timestamp_ns();
    
    // Copy kernel ISA
    memcpy(unified_buffer + OFFSET_KERNEL_ISA, kernel_isa_gen9, KERNEL_ISA_SIZE);
    printf("  ✅ Kernel ISA copied to offset 0x%04x (%d bytes)\n", OFFSET_KERNEL_ISA, KERNEL_ISA_SIZE);
    
    // Setup Surface State (64 bytes) - Descripteur buffer output
    uint32_t *surface_state = (uint32_t *)(unified_buffer + OFFSET_SURFACE_STATE);
    surface_state[0] = 0x00000000;  // Surface Base Address (sera patché par GTT)
    surface_state[1] = 0x00000000;  // Surface Base Address High
    surface_state[2] = 0x00001000;  // Width = 4096 bytes
    surface_state[3] = 0x00000001;  // Height = 1
    surface_state[4] = 0x00000000;  // Depth = 1
    surface_state[5] = 0x00000000;  // Surface Pitch
    printf("  ✅ Surface State setup at offset 0x%04x\n", OFFSET_SURFACE_STATE);
    
    // Setup Binding Table (4 bytes) - Pointeur vers Surface State
    uint32_t *binding_table = (uint32_t *)(unified_buffer + OFFSET_BINDING_TABLE);
    binding_table[0] = OFFSET_SURFACE_STATE;  // Offset relatif vers Surface State
    printf("  ✅ Binding Table setup at offset 0x%04x\n", OFFSET_BINDING_TABLE);
    
    // Setup IDRT (32 bytes) - Interface Descriptor
    uint32_t *idrt = (uint32_t *)(unified_buffer + OFFSET_IDRT);
    idrt[0] = OFFSET_KERNEL_ISA;  // Kernel Start Pointer (offset relatif)
    idrt[1] = 0x00000000;         // Reserved
    idrt[2] = 0x00000000;         // Reserved
    idrt[3] = OFFSET_BINDING_TABLE;  // Binding Table Pointer (offset relatif)
    idrt[4] = 0x00000001;         // Binding Table Entry Count
    idrt[5] = 0x00000000;         // Constant URB Entry Read Length
    idrt[6] = 0x00000000;         // Number of Threads in GPGPU Thread Group
    idrt[7] = 0x00000000;         // Reserved
    printf("  ✅ IDRT setup at offset 0x%04x\n", OFFSET_IDRT);
    
    // Initialize output buffer with sentinel
    uint32_t *output = (uint32_t *)(unified_buffer + OFFSET_OUTPUT);
    output[0] = 0xDEADBEEF;  // Sentinel value
    printf("  ✅ Output buffer initialized at offset 0x%04x (sentinel: 0xDEADBEEF)\n", OFFSET_OUTPUT);
    
    t1 = get_timestamp_ns();
    printf("✅ Layout setup complete (%.3f µs)\n", (t1 - t0) / 1000.0);
    
    // 6. Build batch commands
    printf("\n[ 50%%] Building batch commands...\n");
    t0 = get_timestamp_ns();
    
    uint32_t *batch = (uint32_t *)(unified_buffer + OFFSET_BATCH);
    int offset = 0;
    
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
    printf("    [%2d] STATE_BASE_ADDRESS\n", offset);
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
    // NOTE: Offsets absolus seront calculés après obtention GTT address
    for (int i = 0; i < 18; i++) {
        batch[offset++] = 0;
    }
    
    /* 3. MEDIA_VFE_STATE - 9 DWords */
    printf("    [%2d] MEDIA_VFE_STATE\n", offset);
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0x00000100;  // Max threads = 256
    batch[offset++] = 0;
    
    /* 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD - 4 DWords */
    printf("    [%2d] MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", offset);
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;  // Descriptor length
    batch[offset++] = OFFSET_IDRT;  // IDRT offset
    
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
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n", 
           offset, offset * 4, (t1 - t0) / 1000.0);
    
    // 7. Prepare exec object (SANS relocations - clé du succès)
    printf("\n[ 70%%] Preparing EXECBUFFER2...\n");
    
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = unified_create.handle,
        .relocation_count = 0,  // ← CLÉ: AUCUNE relocation (architecture C579)
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = EXEC_OBJECT_WRITE,  // Buffer modifié par GPU
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // 8. Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,  // DOIT être 0 (batch à offset 0x0000)
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };
    
    printf("  Configuration:\n");
    printf("    Context ID: %u\n", ctx_create.ctx_id);
    printf("    Ring: RCS (Render)\n");
    printf("    Batch offset: 0x%04x\n", OFFSET_BATCH);
    printf("    Batch length: %u bytes\n", execbuf.batch_len);
    printf("    Relocations: %llu (AUCUNE - architecture C579)\n", 
           (unsigned long long)exec_object.relocation_count);
    
    // 9. Submit to GPU
    printf("\n[ 80%%] Submitting to GPU...\n");
    t0 = get_timestamp_ns();
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        t1 = get_timestamp_ns();
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d, %.3f µs)\n", 
               strerror(errno), errno, (t1 - t0) / 1000.0);
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C582 PASS 1 ÉCHOUÉ                               ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        
        munmap(unified_buffer, UNIFIED_BUFFER_SIZE);
        
        struct drm_i915_gem_context_destroy ctx_destroy = {
            .ctx_id = ctx_create.ctx_id
        };
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        
        struct drm_gem_close close_arg = {
            .handle = unified_create.handle,
        };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  GTT offset: 0x%016llx\n", (unsigned long long)exec_object.offset);
    
    // 10. Wait for completion
    printf("\n[ 90%%] Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = unified_create.handle,
        .timeout_ns = 5000000000ULL,  // 5 seconds
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        t1 = get_timestamp_ns();
        printf("⚠️  WAIT failed: %s (errno=%d, %.3f ms)\n", 
               strerror(errno), errno, (t1 - t0) / 1000000.0);
    } else {
        t1 = get_timestamp_ns();
        printf("✅ GPU execution completed (%.3f ms)\n", (t1 - t0) / 1000000.0);
    }
    
    // 11. Read results
    printf("\n[ 95%%] Reading results...\n");
    
    printf("  Output[0]: 0x%08X (attendu: 0x12345678)\n", output[0]);
    
    int success = (output[0] == 0x12345678);
    
    // 12. Cleanup
    printf("\n[100%%] Cleanup...\n");
    
    munmap(unified_buffer, UNIFIED_BUFFER_SIZE);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("✅ Context destroyed\n");
    
    struct drm_gem_close close_arg = {
        .handle = unified_create.handle,
    };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(drm_fd);
    
    uint64_t t_end = get_timestamp_ns();
    double total_ms = (t_end - t_start) / 1000000.0;
    
    if (success) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ TEST C582 PASS 1 RÉUSSI                               ║\n");
        printf("║  PREMIER WRITE GPU NATIF i915 VALIDÉ!                    ║\n");
        printf("║  output[0] = 0x12345678 ✅                                ║\n");
        printf("║  Total time: %.3f ms                                      ║\n", total_ms);
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 0;
    } else {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C582 PASS 1 ÉCHOUÉ                               ║\n");
        printf("║  output[0] = 0x%08X (attendu: 0x12345678)                ║\n", output[0]);
        printf("║  Total time: %.3f ms                                      ║\n", total_ms);
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 1;
    }
}

// Made with Bob - Cycle C582 PASS 1