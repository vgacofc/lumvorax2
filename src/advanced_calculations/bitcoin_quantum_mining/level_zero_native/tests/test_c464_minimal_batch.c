/*
 * TEST C464 - BATCH BUFFER MINIMAL ABSOLU
 * Objectif: Valider infrastructure i915 de base SANS kernel
 * 
 * Test progressif:
 * - PIPELINE_SELECT
 * - STATE_BASE_ADDRESS
 * - MEDIA_VFE_STATE
 * - MI_BATCH_BUFFER_END
 * 
 * Pas de:
 * - Kernel
 * - GPGPU_WALKER
 * - CURBE
 * - SSH
 * - Mémoire
 * 
 * Succès = EXECBUFFER2 réussit + GPU idle + PAS de GPU HANG
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
#include <i915_drm.h>
#include <xf86drm.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

/* Gen9 Command Opcodes */
#define GEN9_PIPELINE_SELECT           ((0x3 << 29) | (0x1 << 27) | (0x1 << 24) | (0x4 << 16))
#define GEN9_STATE_BASE_ADDRESS        0x61010000
#define GEN9_MEDIA_VFE_STATE           0x70000000
#define GEN9_MI_BATCH_BUFFER_END       0x0a000000
#define PIPELINE_SELECT_MEDIA          (1 << 0)

/* Gen9 Constants */
#define CACHE_CTRL 0x3
#define BASE_ADDRESS_MODIFY 0x1

/* Soft-pinning addresses */
#define GPU_ADDR_BATCH      0x0000000000100000ULL

#ifndef EXEC_OBJECT_PINNED
#define EXEC_OBJECT_PINNED (1 << 4)
#endif
#ifndef EXEC_OBJECT_SUPPORTS_48B_ADDRESS
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1 << 3)
#endif

int main(void) {
    FORENSIC_LOG("🚀 TEST C464 - BATCH BUFFER MINIMAL ABSOLU");
    
    /* Ouvrir DRM */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        FORENSIC_LOG("❌ Impossible d'ouvrir /dev/dri/renderD128: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", drm_fd);
    
    /* Créer contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ GEM_CONTEXT_CREATE failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx_id);
    
    /* Créer batch buffer (4KB) */
    struct drm_i915_gem_create create = {
        .size = 4096
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        FORENSIC_LOG("❌ GEM_CREATE failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t batch_bo = create.handle;
    FORENSIC_LOG("✅ Batch buffer créé: handle=%u size=%zu", batch_bo, create.size);
    
    /* Mapper batch buffer */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = batch_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        FORENSIC_LOG("❌ GEM_MMAP_OFFSET failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    void* batch_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
    if (batch_map == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Batch buffer mappé: %p", batch_map);
    
    /* Construire batch buffer MINIMAL */
    uint32_t* batch = (uint32_t*)batch_map;
    uint32_t offset = 0;
    
    /* MI_NOOP padding */
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    /* PIPELINE_SELECT - GPGPU mode */
    batch[offset++] = GEN9_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA;
    FORENSIC_LOG("📝 [%02u] PIPELINE_SELECT = 0x%08X", offset-1, batch[offset-1]);
    
    /* STATE_BASE_ADDRESS - Adresses NULL (pas de heap nécessaire) */
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    /* SSH base = NULL */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* DSH base = NULL */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* IOH base = NULL */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* Instruction base = NULL */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    FORENSIC_LOG("📝 [%02u] STATE_BASE_ADDRESS (19 DWORDs)", offset-19);
    
    /* MEDIA_VFE_STATE - Configuration minimale */
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (8 - 2);
    batch[offset++] = 0x00000000;  /* Scratch Space = 0 */
    batch[offset++] = 0x00000000;
    batch[offset++] = (168 << 0) | (64 << 16);  /* Max Threads=168, URB=64 */
    batch[offset++] = 0x00000000;  /* Slice Disable = 0 */
    batch[offset++] = (1 << 0) | (2 << 16);  /* CURBE=1, URB=2 */
    batch[offset++] = 0x00000000;  /* Scoreboard */
    batch[offset++] = 0x00000000;
    FORENSIC_LOG("📝 [%02u] MEDIA_VFE_STATE (8 DWORDs)", offset-8);
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("📝 [%02u] MI_BATCH_BUFFER_END", offset-1);
    
    uint32_t batch_len = offset * 4;
    FORENSIC_LOG("✅ Batch buffer construit: %u DWORDs (%u bytes)", offset, batch_len);
    
    /* EXECBUFFER2 avec soft-pinning */
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = batch_bo,
        .offset = GPU_ADDR_BATCH,
        .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_obj,
        .buffer_count = 1,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch buffer...");
    FORENSIC_LOG("   buffer_count = %u", execbuf.buffer_count);
    FORENSIC_LOG("   batch_len = %u bytes", execbuf.batch_len);
    FORENSIC_LOG("   flags = 0x%08llx", (unsigned long long)execbuf.flags);
    FORENSIC_LOG("   ctx_id = %u", ctx_id);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        munmap(batch_map, 4096);
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi!");
    
    /* Attendre GPU idle */
    FORENSIC_LOG("⏳ Attente GPU idle...");
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 1000000000  /* 1 seconde */
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        munmap(batch_map, 4096);
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    FORENSIC_LOG("✅ TEST C464 RÉUSSI - Batch minimal fonctionne!");
    
    /* Cleanup */
    munmap(batch_map, 4096);
    close(drm_fd);
    
    return 0;
}

// Made with Bob
