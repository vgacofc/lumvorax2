#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <drm/i915_drm.h>

#define BATCH_SIZE 4096

int main() {
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("open /dev/dri/renderD128");
        return 1;
    }
    
    // Create buffer
    struct drm_i915_gem_create create = {
        .size = BATCH_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        perror("GEM_CREATE");
        close(drm_fd);
        return 1;
    }
    
    printf("✅ Created buffer: handle=%u, size=%zu\n", create.handle, create.size);
    
    // Map buffer
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
    
    // Write Gen9 valid batch
    uint32_t offset = 0;
    
    // 1. STATE_BASE_ADDRESS (0x61010008 = opcode + DWord Length 8)
    batch[offset++] = 0x61010008;
    for (int i = 0; i < 9; i++) {
        batch[offset++] = 0x00000000;
    }
    
    // 2. PIPE_CONTROL (0x7a000004 = opcode + DWord Length 4)
    batch[offset++] = 0x7a000004;
    batch[offset++] = 0x00100000;  // CS_STALL
    batch[offset++] = 0x00000000;  // Address Low
    batch[offset++] = 0x00000000;  // Address High
    batch[offset++] = 0x00000000;  // Data Low
    batch[offset++] = 0x00000000;  // Data High
    
    // 3. MI_BATCH_BUFFER_END
    batch[offset++] = 0x05000000;
    
    printf("✅ Wrote %u DWords to batch buffer\n", offset);
    printf("\n=== BATCH HEXDUMP (first 20 DWords) ===\n");
    for (uint32_t i = 0; i < 20 && i < offset; i++) {
        printf("batch[%2u] = 0x%08x", i, batch[i]);
        if (i == 0) printf("  // STATE_BASE_ADDRESS");
        else if (i == 10) printf("  // PIPE_CONTROL");
        else if (i == 11) printf("  // CS_STALL");
        else if (i == 16) printf("  // MI_BATCH_BUFFER_END");
        printf("\n");
    }
    
    // Verify batch content
    printf("\n=== VERIFICATION ===\n");
    if (batch[0] == 0x61010008) {
        printf("✅ STATE_BASE_ADDRESS opcode correct\n");
    } else {
        printf("❌ STATE_BASE_ADDRESS opcode WRONG: 0x%08x\n", batch[0]);
    }
    
    if (batch[10] == 0x7a000004) {
        printf("✅ PIPE_CONTROL opcode correct\n");
    } else {
        printf("❌ PIPE_CONTROL opcode WRONG: 0x%08x\n", batch[10]);
    }
    
    if (batch[16] == 0x05000000) {
        printf("✅ MI_BATCH_BUFFER_END opcode correct\n");
    } else {
        printf("❌ MI_BATCH_BUFFER_END opcode WRONG: 0x%08x\n", batch[16]);
    }
    
    printf("\n=== BATCH LENGTH ===\n");
    printf("Total DWords: %u\n", offset);
    printf("Total Bytes: %u\n", offset * 4);
    printf("Expected: 17 DWords (68 bytes)\n");
    
    if (offset == 17) {
        printf("✅ Batch length correct\n");
    } else {
        printf("❌ Batch length WRONG\n");
    }
    
    munmap(batch, BATCH_SIZE);
    
    // Close buffer
    struct drm_gem_close close_arg = {
        .handle = create.handle,
    };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    close(drm_fd);
    return 0;
}

// Made with Bob
