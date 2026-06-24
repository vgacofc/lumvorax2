/*
 * TEST C609 - ARCHITECTURE MINIMALE DE RÉFÉRENCE
 * 
 * Basé sur test_c583/c584 qui FONCTIONNENT
 * Simplifié au maximum pour identifier ROOT CAUSE #87
 * 
 * ARCHITECTURE:
 * - 1 buffer unifié (16KB)
 * - Batch simple: PIPE_CONTROL + MI_BATCH_BUFFER_END
 * - AUCUNE relocation (test baseline)
 * 
 * OBJECTIF: Valider que execbuffer2 passe sans relocation
 * 
 * Made with Bob - Cycle C609 ROOT CAUSE #87
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <drm/i915_drm.h>

#define BUFFER_SIZE (16 * 1024)

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define LOG(fmt, ...) do { \
    uint64_t ns = get_timestamp_ns(); \
    printf("[%llu.%09llu] " fmt "\n", \
           (unsigned long long)(ns / 1000000000ULL), \
           (unsigned long long)(ns % 1000000000ULL), \
           ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

int main(void) {
    LOG("═══════════════════════════════════════════════════════");
    LOG("TEST C609 - ARCHITECTURE MINIMALE DE RÉFÉRENCE");
    LOG("═══════════════════════════════════════════════════════");
    LOG("Objectif: Valider execbuffer2 sans relocation");
    LOG("");
    
    // 1. Open DRM
    LOG("[ 10%%] Opening DRM...");
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        drm_fd = open("/dev/dri/renderD128", O_RDWR);
        if (drm_fd < 0) {
            LOG("❌ Failed to open DRM: errno=%d", errno);
            return 1;
        }
        LOG("✅ Using renderD128");
    } else {
        LOG("✅ Using card1");
    }
    
    // 2. Create GEM Context
    LOG("");
    LOG("[ 20%%] Creating GEM Context...");
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        LOG("❌ GEM_CONTEXT_CREATE failed: errno=%d", errno);
        close(drm_fd);
        return 1;
    }
    LOG("✅ Context created: ctx_id=%u", ctx_create.ctx_id);
    
    // 3. Create buffer
    LOG("");
    LOG("[ 30%%] Creating buffer (16KB)...");
    struct drm_i915_gem_create gem_create = {
        .size = BUFFER_SIZE,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        LOG("❌ GEM_CREATE failed: errno=%d", errno);
        close(drm_fd);
        return 1;
    }
    LOG("✅ Buffer created: handle=%u", gem_create.handle);
    
    // 4. Map buffer
    LOG("");
    LOG("[ 40%%] Mapping buffer...");
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = gem_create.handle,
        .offset = 0,
        .size = BUFFER_SIZE,
        .flags = 0,
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        LOG("❌ GEM_MMAP failed: errno=%d", errno);
        close(drm_fd);
        return 1;
    }
    
    uint32_t *buffer = (uint32_t *)mmap_arg.addr_ptr;
    memset(buffer, 0, BUFFER_SIZE);
    LOG("✅ Buffer mapped: %p", buffer);
    
    // 5. Build minimal batch
    LOG("");
    LOG("[ 50%%] Building minimal batch...");
    int offset = 0;
    
    // PIPE_CONTROL (6 DWords)
    buffer[offset++] = 0x7a000004;  // PIPE_CONTROL | (6-2)
    buffer[offset++] = 0x00100000;  // CS_STALL
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    
    // MI_BATCH_BUFFER_END (1 DWord)
    buffer[offset++] = 0x0A000000;
    
    int batch_len = offset * 4;
    LOG("✅ Batch built: %d DWords (%d bytes)", offset, batch_len);
    LOG("  [0] = 0x%08x (PIPE_CONTROL)", buffer[0]);
    LOG("  [6] = 0x%08x (MI_BATCH_BUFFER_END)", buffer[6]);
    
    // 6. Setup exec_object2
    LOG("");
    LOG("[ 60%%] Setting up exec_object2...");
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = gem_create.handle,
        .relocation_count = 0,  // AUCUNE relocation
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS,
        .rsvd1 = 0,
        .rsvd2 = 0,
    };
    LOG("✅ exec_object2 configured");
    LOG("  handle: %u", exec_obj.handle);
    LOG("  flags: 0x%llx (SUPPORTS_48B_ADDRESS)", 
        (unsigned long long)exec_obj.flags);
    
    // 7. Setup execbuffer2
    LOG("");
    LOG("[ 70%%] Setting up execbuffer2...");
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)&exec_obj,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .cliprects_ptr = 0,
        .num_cliprects = 0,
        .DR1 = 0,
        .DR4 = 0,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_create.ctx_id,
        .rsvd2 = 0,
    };
    LOG("✅ execbuffer2 configured");
    LOG("  buffer_count: %u", execbuf.buffer_count);
    LOG("  batch_len: %u", execbuf.batch_len);
    LOG("  flags: 0x%llx (RENDER)", (unsigned long long)execbuf.flags);
    LOG("  ctx_id: %u", execbuf.rsvd1);
    
    // 8. Execute
    LOG("");
    LOG("═══════════════════════════════════════════════════════");
    LOG("[ 80%%] EXECUTING GPU...");
    LOG("═══════════════════════════════════════════════════════");
    
    uint64_t t0 = get_timestamp_ns();
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    uint64_t t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG("❌ EXECBUFFER2 FAILED: ret=%d, errno=%d (%s)", 
            ret, errno, strerror(errno));
        LOG("");
        LOG("DIAGNOSTIC:");
        LOG("  → Vérifier dmesg pour erreurs kernel i915");
        LOG("  → Vérifier permissions /dev/dri/card1");
        LOG("  → Vérifier que GPU est actif");
        
        munmap(buffer, BUFFER_SIZE);
        close(drm_fd);
        return 1;
    }
    
    LOG("✅ EXECBUFFER2 SUCCESS! (%.3f µs)", (t1 - t0) / 1000.0);
    LOG("  GTT offset assigned: 0x%016llx", 
        (unsigned long long)exec_obj.offset);
    LOG("");
    
    // 9. Wait GPU
    LOG("[ 90%%] Waiting GPU...");
    struct drm_i915_gem_wait wait_arg = {
        .bo_handle = gem_create.handle,
        .flags = 0,
        .timeout_ns = 1000000000,  // 1 second
    };
    
    t0 = get_timestamp_ns();
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_arg);
    t1 = get_timestamp_ns();
    
    if (ret != 0) {
        LOG("⚠️  GEM_WAIT failed: errno=%d", errno);
    } else {
        LOG("✅ GPU completed (%.3f ms)", (t1 - t0) / 1000000.0);
    }
    
    // 10. Cleanup
    LOG("");
    LOG("[100%%] Cleanup...");
    munmap(buffer, BUFFER_SIZE);
    
    struct drm_gem_close close_arg = {
        .handle = gem_create.handle,
    };
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = {
        .ctx_id = ctx_create.ctx_id,
    };
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(drm_fd);
    
    LOG("✅ Cleanup complete");
    LOG("");
    LOG("═══════════════════════════════════════════════════════");
    LOG("🎉 TEST C609 RÉUSSI!");
    LOG("═══════════════════════════════════════════════════════");
    LOG("✅ execbuffer2 fonctionne sans relocation");
    LOG("✅ Architecture minimale validée");
    LOG("✅ Prêt pour ajouter MI_STORE_DATA_IMM + relocation");
    LOG("");
    
    return 0;
}

// Made with Bob - ROOT CAUSE #87 investigation