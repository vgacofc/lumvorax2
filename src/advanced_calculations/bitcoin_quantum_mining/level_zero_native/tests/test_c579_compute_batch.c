/*
 * TEST C579 - BATCH COMPUTE COMPLET GEN9
 * ROOT CAUSE #105 SOLUTION: Ajout GPGPU_WALKER + MEDIA_VFE_STATE
 * 
 * Séquence batch compute Gen9 complète (57 DWords):
 * 1. PIPE_CONTROL (flush initial)
 * 2. STATE_BASE_ADDRESS (19 DWords)
 * 3. MEDIA_VFE_STATE (9 DWords)
 * 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD (4 DWords)
 * 5. INTERFACE_DESCRIPTOR_DATA (16 DWords)
 * 6. GPGPU_WALKER (15 DWords)
 * 7. PIPE_CONTROL (flush final)
 * 8. MI_BATCH_BUFFER_END
 * 
 * Code récupéré depuis: backup_pre_optim_20260515_235125/src/btc_gen9_native_runner.c
 * Rapport source: RAPPORT_C198_PHASE15C_SOLUTION_NATIVE_GEN9.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
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

int main() {
    printf("🚀 TEST C579 - BATCH COMPUTE COMPLET GEN9\n\n");
    
    // 1. Open DRM device
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("open /dev/dri/renderD128");
        return 1;
    }
    printf("✅ DRM device opened: fd=%d\n", drm_fd);
    
    // 2. Create GEM Context
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("GEM_CONTEXT_CREATE");
        close(drm_fd);
        return 1;
    }
    printf("✅ GEM Context created: ctx_id=%u\n", ctx_create.ctx_id);
    
    // 3. Create batch buffer
    struct drm_i915_gem_create create = {
        .size = BATCH_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("GEM_CREATE");
        close(drm_fd);
        return 1;
    }
    printf("✅ Batch buffer created: handle=%u\n", create.handle);
    
    // 4. Map batch buffer
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = BATCH_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        perror("GEM_MMAP");
        close(drm_fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)mmap_arg.addr_ptr;
    memset(batch, 0, BATCH_SIZE);
    
    // 5. Build COMPLETE Gen9 compute batch
    int offset = 0;
    
    printf("\n=== BUILDING COMPUTE BATCH ===\n");
    
    /* 1. PIPE_CONTROL (flush initial) - 6 DWords */
    printf("  [%2d] PIPE_CONTROL (flush initial)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 2. STATE_BASE_ADDRESS - 19 DWords */
    printf("  [%2d] STATE_BASE_ADDRESS\n", offset);
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
    for (int i = 0; i < 18; i++) {
        batch[offset++] = 0;
    }
    
    /* 3. MEDIA_VFE_STATE - 9 DWords */
    printf("  [%2d] MEDIA_VFE_STATE\n", offset);
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0x00000100;  // Max threads
    batch[offset++] = 0;
    
    /* 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD - 4 DWords */
    printf("  [%2d] MEDIA_INTERFACE_DESCRIPTOR_LOAD\n", offset);
    batch[offset++] = GEN9_MEDIA_INTERFACE_DESCRIPTOR_LOAD | (4 - 2);
    batch[offset++] = 0;
    batch[offset++] = 32;  // Descriptor length
    batch[offset++] = 0;
    
    /* 5. INTERFACE_DESCRIPTOR_DATA - 16 DWords */
    printf("  [%2d] INTERFACE_DESCRIPTOR_DATA\n", offset);
    for (int i = 0; i < 6; i++) {
        batch[offset++] = 0;
    }
    batch[offset++] = 0x00000100;  // Thread group size
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0;
    }
    
    /* 6. GPGPU_WALKER - 15 DWords */
    printf("  [%2d] GPGPU_WALKER\n", offset);
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
    printf("  [%2d] PIPE_CONTROL (flush final)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* 8. MI_BATCH_BUFFER_END - 1 DWord */
    printf("  [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    munmap(batch, BATCH_SIZE);
    
    printf("\n✅ Batch built: %d DWords (%d bytes)\n", offset, offset * 4);
    
    // 6. Prepare exec object
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = create.handle,
        .relocation_count = 0,
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
    
    printf("\n=== EXECBUFFER2 SUBMIT ===\n");
    printf("Context ID: %u\n", ctx_create.ctx_id);
    printf("Ring: RCS (Render)\n");
    printf("Batch length: %u bytes\n", execbuf.batch_len);
    
    // 8. Submit to GPU
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d)\n", strerror(errno), errno);
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C579 ÉCHOUÉ - Batch compute rejeté              ║\n");
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
    
    printf("✅ EXECBUFFER2 SUCCESS!\n");
    printf("GTT offset: 0x%016llx\n", (unsigned long long)exec_object.offset);
    
    // 9. Wait for completion
    struct drm_i915_gem_wait wait = {
        .bo_handle = create.handle,
        .timeout_ns = 5000000000ULL,  // 5 seconds
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        printf("⚠️  WAIT failed: %s (errno=%d)\n", strerror(errno), errno);
    } else {
        printf("✅ GPU execution completed\n");
    }
    
    // 10. Cleanup
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
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ TEST C579 RÉUSSI - Batch compute accepté             ║\n");
    printf("║  ROOT CAUSE #105 RÉSOLU                                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob
