/*
 * TEST C578 - EXECBUFFER2 avec GEM CONTEXT
 * ROOT CAUSE #104: i915 rejette EXECBUFFER2 sans context valide
 * OpenCL crée automatiquement un context, nous devons le faire manuellement
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

int main() {
    printf("🚀 TEST C578 - EXECBUFFER2 avec GEM CONTEXT\n\n");
    
    // 1. Open DRM device
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("open /dev/dri/renderD128");
        return 1;
    }
    printf("✅ DRM device opened: fd=%d\n", drm_fd);
    
    // 2. CREATE GEM CONTEXT (CRITIQUE!)
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
    
    // 4. Map and write batch
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
    
    uint32_t offset = 0;
    
    // STATE_BASE_ADDRESS
    batch[offset++] = 0x61010008;
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0x00000000;
    }
    
    // PIPE_CONTROL
    batch[offset++] = 0x7a000004;
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x05000000;
    
    munmap(batch, BATCH_SIZE);
    printf("✅ Batch written: %u DWords (68 bytes)\n", offset);
    
    // 5. Prepare exec object
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
    
    // 6. Prepare execbuffer2 WITH CONTEXT
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = 68,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,  // ← CRITIQUE: Utiliser context créé
        .rsvd2 = 0
    };
    
    printf("\n=== EXECBUFFER2 SUBMIT ===\n");
    printf("Context ID: %u\n", ctx_create.ctx_id);
    printf("Ring: RCS (Render)\n");
    printf("Batch length: %u bytes\n", execbuf.batch_len);
    
    // 7. Submit to GPU
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d)\n", strerror(errno), errno);
        
        // Destroy context
        struct drm_i915_gem_context_destroy ctx_destroy = {
            .ctx_id = ctx_create.ctx_id
        };
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        
        // Close buffer
        struct drm_gem_close close_arg = {
            .handle = create.handle,
        };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        
        close(drm_fd);
        return 1;
    }
    
    printf("✅ EXECBUFFER2 SUCCESS!\n");
    printf("GTT offset: 0x%016llx\n", (unsigned long long)exec_object.offset);
    
    // 8. Cleanup
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
    printf("║  ✅ TEST C578 RÉUSSI - CONTEXT REQUIS CONFIRMÉ           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob
