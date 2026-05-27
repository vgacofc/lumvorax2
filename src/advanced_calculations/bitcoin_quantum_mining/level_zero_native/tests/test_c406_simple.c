/**
 * C406: TEST SIMPLIFIÉ - Kernel Natif Minimal
 * 
 * Version ultra-simplifiée pour isoler le problème EXECBUFFER2
 * Pas de STATE_BASE_ADDRESS, juste MEDIA_VFE_STATE + GPGPU_WALKER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

#define NUM_THREADS 16

int main() {
    printf("=== C406 TEST SIMPLIFIÉ ===\n\n");
    
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open DRM: %s\n", strerror(errno));
        return 1;
    }
    printf("✓ DRM opened\n");
    
    // Créer contexte
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "Failed to create context: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✓ Context created: %u\n", ctx_id);
    
    // Charger kernel
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        fprintf(stderr, "Failed to open kernel\n");
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Créer buffer kernel
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        fprintf(stderr, "Failed to create kernel buffer\n");
        fclose(f);
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        fprintf(stderr, "Failed to mmap kernel\n");
        fclose(f);
        close(fd);
        return 1;
    }
    
    void *kernel_map = (void *)(uintptr_t)mmap_kernel.addr_ptr;
    memset(kernel_map, 0, 4096);
    fread(kernel_map, 1, kernel_size, f);
    fclose(f);
    printf("✓ Kernel loaded: %zu bytes\n", kernel_size);
    
    // Créer buffer output
    struct drm_i915_gem_create create_output = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_output) < 0) {
        fprintf(stderr, "Failed to create output buffer\n");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        fprintf(stderr, "Failed to mmap output\n");
        close(fd);
        return 1;
    }
    
    void *output_map = (void *)(uintptr_t)mmap_output.addr_ptr;
    memset(output_map, 0, 4096);
    printf("✓ Output buffer created\n");
    
    // Créer batch buffer
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        fprintf(stderr, "Failed to create batch buffer\n");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        fprintf(stderr, "Failed to mmap batch\n");
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    
    // Construire batch MINIMAL
    int idx = 0;
    
    // MI_NOOP (pour alignement)
    batch[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    printf("✓ Batch built: %d dwords\n", idx);
    
    // Execbuffer
    struct drm_i915_gem_exec_object2 exec_objects[3] = {
        { .handle = create_kernel.handle },
        { .handle = create_output.handle, .flags = EXEC_OBJECT_WRITE },
        { .handle = create_batch.handle }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 3,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    printf("\n🚀 Executing...\n");
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✅ EXECBUFFER2 SUCCESS!\n");
    
    // Cleanup
    struct drm_gem_close close_arg;
    close_arg.handle = create_kernel.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_output.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = create_batch.handle;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(fd);
    
    return 0;
}

// Made with Bob
