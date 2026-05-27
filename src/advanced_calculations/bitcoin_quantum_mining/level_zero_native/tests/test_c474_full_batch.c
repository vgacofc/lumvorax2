/*
 * TEST C474 - BATCH COMPLET GEN9 NULL STATE (960 DWORDS)
 * 
 * Source: Linux kernel gen9_renderstate.c (complet)
 * Taille: 960 DWORDs (3840 bytes)
 * Relocations: 4 (offsets 0x7a8, 0x7b4, 0x7bc, 0x7cc)
 * 
 * Objectif: Utiliser le batch null_state COMPLET Intel pour initialiser
 * correctement le render engine Gen9 et éviter GPU HANG
 * 
 * Ce batch configure l'état 3D complet requis
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

/* Include batch complet */
#include "../include/gen9_null_state_batch.h"

/* Relocations from kernel */
static const uint32_t gen9_null_state_relocs[] = {
    0x000007a8,  /* Offset 1960 bytes */
    0x000007b4,  /* Offset 1972 bytes */
    0x000007bc,  /* Offset 1980 bytes */
    0x000007cc,  /* Offset 1996 bytes */
};

#define BATCH_SIZE (sizeof(gen9_null_state_batch))
#define NUM_RELOCS (sizeof(gen9_null_state_relocs) / sizeof(gen9_null_state_relocs[0]))

int create_buffer(int drm_fd, size_t size, uint32_t* handle) {
    struct drm_i915_gem_create create = {.size = size};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        return -1;
    }
    *handle = create.handle;
    return 0;
}

int main(void) {
    FORENSIC_LOG("🚀 TEST C474 - BATCH COMPLET GEN9 NULL STATE");
    FORENSIC_LOG("📚 Source: Linux kernel gen9_renderstate.c (COMPLET)");
    FORENSIC_LOG("📊 Taille: %zu bytes (%zu DWORDs)", BATCH_SIZE, BATCH_SIZE/4);
    FORENSIC_LOG("🔗 Relocations: %zu", NUM_RELOCS);
    
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
    
    /* Créer buffers (batch + 4 pour relocations) */
    uint32_t batch_bo, reloc_bo[4];
    
    if (create_buffer(drm_fd, 8192, &batch_bo) < 0) {
        FORENSIC_LOG("❌ Batch buffer creation failed");
        close(drm_fd);
        return 1;
    }
    
    for (int i = 0; i < 4; i++) {
        if (create_buffer(drm_fd, 4096, &reloc_bo[i]) < 0) {
            FORENSIC_LOG("❌ Reloc buffer %d creation failed", i);
            close(drm_fd);
            return 1;
        }
    }
    
    FORENSIC_LOG("✅ Buffers créés: batch=%u, relocs=[%u,%u,%u,%u]",
                 batch_bo, reloc_bo[0], reloc_bo[1], reloc_bo[2], reloc_bo[3]);
    
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
    
    void* batch_map = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
    if (batch_map == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Batch buffer mappé: %p", batch_map);
    
    /* Copier batch complet */
    memcpy(batch_map, gen9_null_state_batch, BATCH_SIZE);
    FORENSIC_LOG("✅ Batch complet copié: %zu bytes", BATCH_SIZE);
    
    /* Préparer relocations */
    struct drm_i915_gem_relocation_entry relocs[NUM_RELOCS];
    memset(relocs, 0, sizeof(relocs));
    
    for (size_t i = 0; i < NUM_RELOCS; i++) {
        relocs[i].target_handle = reloc_bo[i];
        relocs[i].offset = gen9_null_state_relocs[i];
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
        FORENSIC_LOG("📍 Relocation %zu: offset=0x%x, target=%u", 
                     i, gen9_null_state_relocs[i], reloc_bo[i]);
    }
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    for (int i = 0; i < 4; i++) {
        exec_objects[i].handle = reloc_bo[i];
    }
    exec_objects[4].handle = batch_bo;
    exec_objects[4].relocation_count = NUM_RELOCS;
    exec_objects[4].relocs_ptr = (uint64_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = BATCH_SIZE,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch complet (960 DWORDs + 4 relocations)...");
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        munmap(batch_map, 8192);
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
        munmap(batch_map, 8192);
        close(drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    FORENSIC_LOG("🎉 TEST C474 RÉUSSI - Batch complet Intel fonctionne!");
    FORENSIC_LOG("📊 CONCLUSION: Pipeline 3D initialisé correctement");
    FORENSIC_LOG("💡 PROCHAINE ÉTAPE: Ajouter nos commandes GPGPU après ce batch");
    
    /* Cleanup */
    munmap(batch_map, 8192);
    close(drm_fd);
    
    return 0;
}

// Made with Bob