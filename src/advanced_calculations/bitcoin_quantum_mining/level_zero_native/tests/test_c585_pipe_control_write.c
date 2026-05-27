/* TEST C585-A2 - PIPE_CONTROL POST-SYNC WRITE
 * 
 * Test encore plus minimal: PIPE_CONTROL avec post-sync write
 * - Pas de MI_STORE_DATA_IMM
 * - Pas de relocation
 * - Juste PIPE_CONTROL write immediate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>

#define GEN9_PIPE_CONTROL           0x7A000000
#define GEN9_MI_BATCH_BUFFER_END    0x0A000000

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    printf("🚀 TEST C585-A2 - PIPE_CONTROL POST-SYNC WRITE\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("❌ open");
        return 1;
    }
    printf("✅ DRM opened\n");
    
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        perror("❌ context");
        return 1;
    }
    printf("✅ Context: %u\n", ctx_create.ctx_id);
    
    // Output buffer
    struct drm_i915_gem_create output_create = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &output_create) < 0) {
        perror("❌ output");
        return 1;
    }
    
    struct drm_i915_gem_mmap output_mmap = {
        .handle = output_create.handle,
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &output_mmap) < 0) {
        perror("❌ mmap output");
        return 1;
    }
    uint32_t *output = (uint32_t *)output_mmap.addr_ptr;
    output[0] = 0xDEADBEEF;
    printf("✅ Output: handle=%u, sentinel=0x%08x\n", output_create.handle, output[0]);
    
    // Batch buffer
    struct drm_i915_gem_create batch_create = { .size = 4096 };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &batch_create) < 0) {
        perror("❌ batch");
        return 1;
    }
    
    struct drm_i915_gem_mmap batch_mmap = {
        .handle = batch_create.handle,
        .size = 4096
    };
    if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &batch_mmap) < 0) {
        perror("❌ mmap batch");
        return 1;
    }
    uint32_t *batch = (uint32_t *)batch_mmap.addr_ptr;
    memset(batch, 0, 4096);
    printf("✅ Batch: handle=%u\n\n", batch_create.handle);
    
    // Build batch - PIPE_CONTROL avec post-sync write immediate
    int offset = 0;
    
    printf("Building batch:\n");
    
    // PIPE_CONTROL avec write immediate (6 DWords)
    printf("  [%2d] PIPE_CONTROL (post-sync write immediate)\n", offset);
    batch[offset++] = GEN9_PIPE_CONTROL | (6 - 2);
    batch[offset++] = 0x00100000 | (1 << 14) | (1 << 21);  // CS_STALL + Write Immediate + Post-Sync Op
    batch[offset++] = 0;  // Address Low (sera patché par relocation)
    batch[offset++] = 0;  // Address High
    batch[offset++] = 0x12345678;  // Immediate Data Low
    batch[offset++] = 0;  // Immediate Data High
    
    printf("  [%2d] MI_BATCH_BUFFER_END\n", offset);
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    printf("✅ Batch: %d DWords (%d bytes)\n\n", offset, offset * 4);
    
    // Relocation pour PIPE_CONTROL address
    struct drm_i915_gem_relocation_entry reloc = {
        .target_handle = output_create.handle,
        .delta = 0,
        .offset = 2 * 4,  // DWord 2 de PIPE_CONTROL
        .read_domains = I915_GEM_DOMAIN_RENDER,
        .write_domain = I915_GEM_DOMAIN_RENDER,
        .presumed_offset = 0
    };
    
    printf("Relocation:\n");
    printf("  Target: output (handle=%u)\n", reloc.target_handle);
    printf("  Offset: 0x%llx (DWord %d)\n\n", reloc.offset, (int)(reloc.offset / 4));
    
    // EXECBUFFER2
    struct drm_i915_gem_exec_object2 exec_objects[2] = {
        {
            .handle = output_create.handle,
            .flags = EXEC_OBJECT_WRITE
        },
        {
            .handle = batch_create.handle,
            .relocation_count = 1,
            .relocs_ptr = (uint64_t)&reloc
        }
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 2,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id
    };
    
    printf("Submitting...\n");
    uint64_t t0 = get_timestamp_ns();
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        uint64_t t1 = get_timestamp_ns();
        printf("❌ EXECBUFFER2 FAILED: %s (errno=%d, %.3f µs)\n", 
               strerror(errno), errno, (t1 - t0) / 1000.0);
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST C585-A2 ÉCHOUÉ                                   ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 1;
    }
    
    uint64_t t1 = get_timestamp_ns();
    printf("✅ EXECBUFFER2 SUCCESS (%.3f µs)\n", (t1 - t0) / 1000.0);
    printf("  Output GTT: 0x%016llx\n", exec_objects[0].offset);
    printf("  Batch GTT:  0x%016llx\n\n", exec_objects[1].offset);
    
    // Wait
    struct drm_i915_gem_wait wait_args = {
        .bo_handle = batch_create.handle,
        .timeout_ns = 1000000000
    };
    
    t0 = get_timestamp_ns();
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait_args) < 0) {
        perror("❌ wait");
        return 1;
    }
    t1 = get_timestamp_ns();
    printf("✅ GPU completed (%.3f ms)\n\n", (t1 - t0) / 1000000.0);
    
    // Read result
    uint32_t result = output[0];
    printf("Result: 0x%08x\n\n", result);
    
    if (result == 0x12345678) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ TEST C585-A2 RÉUSSI!                                  ║\n");
        printf("║  PIPE_CONTROL write fonctionne!                           ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  Output inchangé (0x%08x)                            ║\n", result);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    close(fd);
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob
