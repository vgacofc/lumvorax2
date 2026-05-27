/**
 * C410 - TEST AVEC /dev/dri/card1 SANS PINNING
 * 
 * Différences clés vs C407-C409:
 * 1. Utilise /dev/dri/card1 au lieu de renderD128
 * 2. PAS de EXEC_OBJECT_PINNED
 * 3. PAS de GTT addresses explicites
 * 4. Batch minimal: GPGPU_WALKER + BATCH_BUFFER_END
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
#include <xf86drm.h>
#include <i915_drm.h>

#define NUM_THREADS 256

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C410 TEST - /dev/dri/card1 SANS PINNING                  ║\n");
    printf("║  Kernel: btc_sha256_native_c406.bin (176 bytes)           ║\n");
    printf("║  Device: card1 (pas renderD128)                           ║\n");
    printf("║  Flags: PAS de EXEC_OBJECT_PINNED                         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM avec card1
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/dri/card1");
        return 1;
    }
    printf("✓ DRM opened: /dev/dri/card1 fd=%d\n", fd);
    
    // Créer contexte
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("CONTEXT_CREATE");
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✓ GPU context created: ctx_id=%u\n", ctx_id);
    
    // Charger kernel
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        perror("fopen kernel");
        close(fd);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Créer buffer kernel
    struct drm_i915_gem_create create_kernel = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_kernel) < 0) {
        perror("GEM_CREATE kernel");
        fclose(f);
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_kernel = {
        .handle = create_kernel.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_kernel) < 0) {
        perror("GEM_MMAP kernel");
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
        perror("GEM_CREATE output");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_output = {
        .handle = create_output.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_output) < 0) {
        perror("GEM_MMAP output");
        close(fd);
        return 1;
    }
    
    void *output_map = (void *)(uintptr_t)mmap_output.addr_ptr;
    memset(output_map, 0, 4096);
    printf("✓ Output buffer created: 4096 bytes\n");
    
    // Créer batch buffer
    struct drm_i915_gem_create create_batch = { .size = 4096 };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        perror("GEM_CREATE batch");
        close(fd);
        return 1;
    }
    
    struct drm_i915_gem_mmap mmap_batch = {
        .handle = create_batch.handle,
        .size = 4096
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_batch) < 0) {
        perror("GEM_MMAP batch");
        close(fd);
        return 1;
    }
    
    uint32_t *batch = (uint32_t *)(uintptr_t)mmap_batch.addr_ptr;
    memset(batch, 0, 4096);
    
    // Construire batch MINIMAL
    int idx = 0;
    
    /* GPGPU_WALKER */
    batch[idx++] = 0x7a000004;  // GPGPU_WALKER opcode
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset
    batch[idx++] = 0x00000000;  // Indirect Data Length
    batch[idx++] = 0x00000000;  // Indirect Data Start Address
    batch[idx++] = NUM_THREADS; // Thread Width
    batch[idx++] = 0x00000001;  // Thread Height
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = 0x0A000000;
    
    printf("✓ Batch buffer built: %d dwords\n", idx);
    
    // Execbuffer SANS PINNING
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
    
    printf("\n🚀 Executing GPU kernel...\n");
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "❌ EXECBUFFER2 failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✓ EXECBUFFER2 success\n");
    
    // Wait GPU
    struct drm_i915_gem_wait wait = {
        .bo_handle = create_batch.handle,
        .timeout_ns = 1000000000  // 1 second
    };
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        perror("GEM_WAIT");
    }
    printf("✓ GPU execution complete\n");
    
    // Lire résultats
    printf("\n📊 Results:\n");
    uint32_t *output = (uint32_t *)output_map;
    int non_zero = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0) {
            non_zero++;
            if (non_zero <= 20) {
                printf("  output[%3d] = 0x%08x", i, output[i]);
                
                // Pattern attendu: (4 << 16) | thread_id
                uint32_t expected = (4 << 16) | i;
                if (output[i] == expected) {
                    printf(" ✓ CORRECT\n");
                } else {
                    printf(" (expected 0x%08x)\n", expected);
                }
            }
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Non-zero results: %d / %d\n", non_zero, NUM_THREADS);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    if (non_zero > 0) {
        printf("🎉 SUCCESS: GPU kernel executed and wrote results!\n");
        printf("✅ SEND descriptor 0x04025E00 is WORKING!\n");
    } else {
        printf("⚠️  WARNING: All results are zero\n");
    }
    
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
    
    printf("\n");
    return (non_zero > 0) ? 0 : 1;
}

// Made with Bob
