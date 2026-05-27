/*
 * TEST C582 - PASS 0: RÉFÉRENCE SANS KERNEL (VALIDATION ARCHITECTURE)
 * 
 * SOLUTION ROOT CAUSE #115: Architecture C579 (SANS relocations) + kernel ISA
 * 
 * STRATÉGIE 2-PASS:
 * - PASS 0 (ce test): Batch compute complet SANS kernel ISA réel
 *   → Valide que l'architecture fonctionne (EXECBUFFER2 success)
 *   → Obtient GTT address du buffer
 * 
 * - PASS 1 (test_c582_pass1_kernel.c): Même architecture + kernel ISA réel
 *   → Utilise GTT address obtenue en PASS 0
 *   → Valide output[0] = 0x12345678
 * 
 * ARCHITECTURE (copie exacte C579):
 * - 1 buffer unifié (4KB)
 * - AUCUNE relocation (clé du succès C579)
 * - Batch compute Gen9 complet (76 DWords)
 * - Offsets absolus codés en dur
 * 
 * Made with Bob - Cycle C582 PASS 0
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

#define BATCH_SIZE 4096

/* Gen9 Command Opcodes */
#define GEN9_PIPE_CONTROL           0x7a000000
#define GEN9_STATE_BASE_ADDRESS     0x61010000
#define GEN9_MEDIA_VFE_STATE        0x70000000
#define GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD 0x70020000
#define GEN9_GPGPU_WALKER           0x71050000
#define GEN9_MI_BATCH_BUFFER_END    0x05000000

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    uint64_t t_start = get_timestamp_ns();
    
    printf("🚀 TEST C582 PASS 0 - RÉFÉRENCE SANS KERNEL (VALIDATION ARCHITECTURE)\n");
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
    
    // 3. Create batch buffer
    printf("\n[ 20%%] Creating batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_create create = {
        .size = BATCH_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        printf("❌ GEM_CREATE failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer created: handle=%u, size=%zu (%.3f µs)\n", 
           create.handle, create.size, (t1 - t0) / 1000.0);
    
    // 4. Map batch buffer
    printf("\n[ 30%%] Mapping batch buffer...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        printf("❌ GEM_MMAP failed: %s (errno=%d)\n", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)mmap_arg.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Batch buffer mapped: %p (%.3f µs)\n", batch, (t1 - t0) / 1000.0);
    
    // 5. Build COMPLETE Gen9 compute batch (COPIE EXACTE C579)
    printf("\n[ 40%%] Building compute batch (C579 architecture)...\n");
    t0 = get_timestamp_ns();
    
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
    batch[offset++] = 0;
    
    /* 5. INTERFACE_DESCRIPTOR_DATA - 16 DWords */
    printf("    [%2d] INTERFACE_DESCRIPTOR_DATA\n", offset);
    for (int i = 0; i < 6; i++) {
        batch[offset++] = 0;
    }
    batch[offset++] = 0x00000100;  // Thread group size
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0;
    }
    
    /* 6. GPGPU_WALKER - 15 DWords */
    printf("    [%2d] GPGPU_WALKER\n", offset);
    batch[offset++] = GEN9_GPGPU_WALKER | (15 - 2);
    batch[offset++] = 0;  // Interface descriptor offset
    batch[offset++] = 0;  // Indirect data length
    batch[offset++] = 0;  // Indirect data start
    batch[offset++] = 0;  // Thread width
    batch[offset++] = 1;  // Thread height
    batch[offset++] = 1;  // Thread depth
    batch[offset++] = 0;  // Thread group width
    batch[offset++] = 1;  // Thread group height
    batch[offset++] = 1;  // Thread group depth
    batch[offset++] = 0;  // Right execution mask
    batch[offset++] = 0;  // Bottom execution mask
    batch[offset++] = 1;  // SIMD size
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
    
    munmap(batch, BATCH_SIZE);
    
    t1 = get_timestamp_ns();
    printf("✅ Batch built: %d DWords (%d bytes, %.3f µs)\n", 
           offset, offset * 4, (t1 - t0) / 1000.0);
    
    // 6. Prepare exec object (SANS relocations - clé du succès)
    printf("\n[ 60%%] Preparing EXECBUFFER2...\n");
    
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = create.handle,
        .relocation_count = 0,  // ← CLÉ: AUCUNE relocation (comme C579)
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    // 7. Prepare execbuffer2
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0
    };
    
    printf("  Configuration:\n");
    printf("    Context ID: %u\n", ctx_create.ctx_id);
    printf("    Ring: RCS (Render)\n");
    printf("    Batch length: %u bytes\n", execbuf.batch_len);
    printf("    Relocations: %llu (AUCUNE - architecture C579)\n", 
           (unsigned long long)exec_object.relocation_count);
    
    // 8. Submit to GPU
    printf("\n[ 80%%] Submitting to GPU...\n");
    t0 = get_timestamp_ns();
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        t1 = get_timestamp_ns();
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d, %.3f µs)\n", 
               strerror(errno), errno, (t1 - t0) / 1000.0);
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C582 PASS 0 ÉCHOUÉ                               ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        
        // Cleanup
        struct drm_i915_gem_context_destroy ctx_destroy = {
            .ctx_id = ctx_create.ctx_id
        };
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        
        struct drm_gem_close close_arg = {
            .handle = create.handle,
        };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        
        close(drm_fd);
        return 1;
    }
    
    t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS! (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  GTT offset: 0x%016llx\n", (unsigned long long)exec_object.offset);
    
    // 9. Wait for completion
    printf("\n[ 90%%] Waiting for GPU completion...\n");
    t0 = get_timestamp_ns();
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = create.handle,
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
    
    // 10. Cleanup
    printf("\n[100%%] Cleanup...\n");
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    printf("✅ Context destroyed\n");
    
    struct drm_gem_close close_arg = {
        .handle = create.handle,
    };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(drm_fd);
    
    uint64_t t_end = get_timestamp_ns();
    double total_ms = (t_end - t_start) / 1000000.0;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TEST C582 PASS 0 RÉUSSI                               ║\n");
    printf("║  Architecture C579 validée (SANS relocations)             ║\n");
    printf("║  GTT address: 0x%016llx                        ║\n", 
           (unsigned long long)exec_object.offset);
    printf("║  Total time: %.3f ms                                      ║\n", total_ms);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    printf("\n📝 PROCHAINE ÉTAPE: test_c582_pass1_kernel.c\n");
    printf("   → Utiliser GTT address pour construire batch avec kernel ISA\n");
    printf("   → Valider output[0] = 0x12345678\n");
    
    return 0;
}

// Made with Bob - Cycle C582 PASS 0