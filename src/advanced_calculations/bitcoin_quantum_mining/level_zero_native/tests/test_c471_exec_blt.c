/*
 * TEST C471 - I915_EXEC_BLT AU LIEU DE I915_EXEC_RENDER
 * 
 * DÉCOUVERTE C467-C470: GPU HANG systématique avec I915_EXEC_RENDER
 * → Render engine nécessite pipeline 3D complet
 * → Preemption timeout après ~700ms sans pipeline valide
 * 
 * Hypothèse: Blitter engine (I915_EXEC_BLT) ne nécessite pas pipeline 3D
 * Test: Batch minimal avec I915_EXEC_BLT au lieu de I915_EXEC_RENDER
 * 
 * Contenu:
 * - MI_NOOP (padding)
 * - MI_BATCH_BUFFER_END
 * 
 * Flags:
 * - I915_EXEC_BLT (blitter engine au lieu de render engine)
 * 
 * Si réussi → Confirme que render engine était le problème
 * Si échec → Problème au niveau contexte ou driver
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
#define GEN9_MI_BATCH_BUFFER_END       0x0a000000
#define GEN9_MI_NOOP                   0x00000000

int main(void) {
    FORENSIC_LOG("🚀 TEST C471 - I915_EXEC_BLT AU LIEU DE I915_EXEC_RENDER");
    FORENSIC_LOG("🎯 Découverte C467-C470: I915_EXEC_RENDER cause GPU HANG systématique");
    FORENSIC_LOG("🔍 Hypothèse: Blitter engine ne nécessite pas pipeline 3D");
    
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
    
    /* Créer batch buffer */
    struct drm_i915_gem_create create = {.size = 4096};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        FORENSIC_LOG("❌ GEM_CREATE failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    uint32_t batch_bo = create.handle;
    FORENSIC_LOG("✅ Batch buffer créé: handle=%u", batch_bo);
    
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
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    FORENSIC_LOG("📝 [00-03] MI_NOOP padding");
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("📝 [%02u] MI_BATCH_BUFFER_END", offset-1);
    
    /* PADDING alignement 8-byte */
    while ((offset * 4) % 8 != 0) {
        batch[offset++] = GEN9_MI_NOOP;
    }
    
    uint32_t batch_len = offset * 4;
    FORENSIC_LOG("✅ Batch: %u DWORDs (%u bytes, aligné 8-byte)", offset, batch_len);
    FORENSIC_LOG("📊 Batch identique à C470 MAIS avec I915_EXEC_BLT");
    
    /* EXECBUFFER2 avec I915_EXEC_BLT */
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = batch_bo
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_BLT,  // ← DIFFÉRENCE CRITIQUE: BLT au lieu de RENDER
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (I915_EXEC_BLT)...");
    FORENSIC_LOG("🔍 DIFFÉRENCE CRITIQUE: flags = I915_EXEC_BLT (blitter engine)");
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        munmap(batch_map, 4096);
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi!");
    
    /* Attendre GPU idle */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 2000000000
    };
    
    FORENSIC_LOG("⏳ Attente GPU idle (timeout 2s)...");
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        munmap(batch_map, 4096);
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    FORENSIC_LOG("🎉 TEST C471 RÉUSSI - GPU fonctionne avec I915_EXEC_BLT!");
    FORENSIC_LOG("📊 CONCLUSION: Render engine (I915_EXEC_RENDER) était ROOT CAUSE!");
    FORENSIC_LOG("💡 SOLUTION: Utiliser blitter engine OU configurer pipeline 3D/GPGPU");
    FORENSIC_LOG("⚠️  PROBLÈME: Blitter engine ne supporte PAS compute/GPGPU");
    
    /* Cleanup */
    munmap(batch_map, 4096);
    close(drm_fd);
    
    return 0;
}

// Made with Bob