/*
 * TEST C473 - GEN9 NULL STATE (BATCH OFFICIEL INTEL)
 * 
 * Source: Linux kernel drivers/gpu/drm/i915/gt/gen9_renderstate.c
 * Généré par: intel-gpu-tools-1.19-177-g68e2eab2
 * 
 * Objectif: Utiliser le batch null_state officiel Intel pour initialiser
 * correctement le render engine Gen9 AVANT d'exécuter nos commandes
 * 
 * Ce batch configure l'état 3D minimal requis pour éviter GPU HANG
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

/* Gen9 null_state batch (premiers 50 DWORDs du kernel Linux) */
static const uint32_t gen9_null_state_batch[] = {
    0x7a000004,  /* STATE_BASE_ADDRESS */
    0x01000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x69040300,  /* PIPELINE_SELECT (3D mode avec bits supplémentaires) */
    0x78140000,  /* 3DSTATE_* */
    0x04000000,
    0x7820000a,  /* 3DSTATE_* */
    0x00000000,
    0x00000000,
    0x80000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x78130002,  /* 3DSTATE_* */
    0x00000000,
    0x00000000,
    0x02001808,
    0x781f0004,  /* 3DSTATE_* */
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x78510009,  /* 3DSTATE_* */
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x78100007,  /* 3DSTATE_* */
    0x00000000,
    0x00000000,
    0x00010000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x0a000000,  /* MI_BATCH_BUFFER_END */
};

#define BATCH_SIZE (sizeof(gen9_null_state_batch))

int main(void) {
    FORENSIC_LOG("🚀 TEST C473 - GEN9 NULL STATE (BATCH OFFICIEL INTEL)");
    FORENSIC_LOG("📚 Source: Linux kernel gen9_renderstate.c");
    FORENSIC_LOG("🎯 Objectif: Initialiser render engine avec état 3D minimal");
    
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
    
    /* Copier batch null_state officiel */
    memcpy(batch_map, gen9_null_state_batch, BATCH_SIZE);
    FORENSIC_LOG("✅ Batch null_state copié: %zu bytes (%zu DWORDs)", 
                 BATCH_SIZE, BATCH_SIZE/4);
    FORENSIC_LOG("📊 Batch officiel Intel - État 3D minimal Gen9");
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_object = {
        .handle = batch_bo
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_object,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = BATCH_SIZE,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch null_state...");
    
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
    FORENSIC_LOG("🎉 TEST C473 RÉUSSI - Batch null_state Intel fonctionne!");
    FORENSIC_LOG("📊 CONCLUSION: Utiliser ce batch comme base pour nos commandes");
    FORENSIC_LOG("💡 PROCHAINE ÉTAPE: Ajouter nos commandes APRÈS ce batch");
    
    /* Cleanup */
    munmap(batch_map, 4096);
    close(drm_fd);
    
    return 0;
}

// Made with Bob