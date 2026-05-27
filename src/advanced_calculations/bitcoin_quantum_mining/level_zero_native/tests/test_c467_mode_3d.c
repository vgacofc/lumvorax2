/*
 * TEST C467 - MODE 3D AU LIEU DE GPGPU (TEST CRITIQUE)
 * 
 * Objectif: Valider si PIPELINE_SELECT GPGPU est ROOT CAUSE du GPU HANG
 * 
 * Hypothèse: Mode GPGPU (bit 0 = 1) cause GPU HANG systématique
 * Test: Utiliser mode 3D (bit 0 = 0) avec même infrastructure
 * 
 * Contenu:
 * - PIPELINE_SELECT (MODE 3D - bit 0 = 0)
 * - STATE_BASE_ADDRESS (avec buffers réels + relocations)
 * - MI_BATCH_BUFFER_END
 * 
 * Si réussi → PIPELINE_SELECT GPGPU est ROOT CAUSE confirmée
 * Si échec → Problème encore plus profond (SBA ou autre)
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
#define GEN9_MI_BATCH_BUFFER_END       0x0a000000
#define GEN9_MI_NOOP                   0x00000000

/* PIPELINE_SELECT modes */
#define PIPELINE_SELECT_3D             (0 << 0)  /* MODE 3D - bit 0 = 0 */
#define PIPELINE_SELECT_MEDIA          (1 << 0)  /* MODE GPGPU - bit 0 = 1 */

/* Gen9 Constants */
#define CACHE_CTRL 0x3
#define BASE_ADDRESS_MODIFY 0x1

int create_buffer(int drm_fd, size_t size, uint32_t* handle) {
    struct drm_i915_gem_create create = {.size = size};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        return -1;
    }
    *handle = create.handle;
    return 0;
}

int main(void) {
    FORENSIC_LOG("🚀 TEST C467 - MODE 3D AU LIEU DE GPGPU (TEST CRITIQUE)");
    FORENSIC_LOG("🎯 Objectif: Valider si PIPELINE_SELECT GPGPU est ROOT CAUSE");
    
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
    
    /* Créer buffers GPU */
    uint32_t batch_bo, ssh_bo, dsh_bo, ioh_bo, instruction_bo;
    
    if (create_buffer(drm_fd, 4096, &batch_bo) < 0 ||
        create_buffer(drm_fd, 4096, &ssh_bo) < 0 ||
        create_buffer(drm_fd, 4096, &dsh_bo) < 0 ||
        create_buffer(drm_fd, 4096, &ioh_bo) < 0 ||
        create_buffer(drm_fd, 4096, &instruction_bo) < 0) {
        FORENSIC_LOG("❌ Buffer creation failed");
        close(drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ 5 buffers GPU créés (batch=%u, ssh=%u, dsh=%u, ioh=%u, inst=%u)",
                 batch_bo, ssh_bo, dsh_bo, ioh_bo, instruction_bo);
    
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
    
    /* Construire batch buffer MODE 3D */
    uint32_t* batch = (uint32_t*)batch_map;
    uint32_t offset = 0;
    
    /* MI_NOOP padding */
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    
    /* PIPELINE_SELECT - MODE 3D (bit 0 = 0) */
    batch[offset++] = GEN9_PIPELINE_SELECT | PIPELINE_SELECT_3D;
    FORENSIC_LOG("📝 [%02u] PIPELINE_SELECT = 0x%08X (MODE 3D - bit 0 = 0)", 
                 offset-1, batch[offset-1]);
    FORENSIC_LOG("🔍 DIFFÉRENCE CRITIQUE: bit 0 = 0 (3D) au lieu de 1 (GPGPU)");
    
    /* STATE_BASE_ADDRESS - Adresses GPU via relocations */
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    FORENSIC_LOG("📝 [%02u] STATE_BASE_ADDRESS (19 DWORDs, avec relocations)", offset-19);
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("📝 [%02u] MI_BATCH_BUFFER_END", offset-1);
    
    /* PADDING alignement 8-byte */
    while ((offset * 4) % 8 != 0) {
        batch[offset++] = GEN9_MI_NOOP;
    }
    
    uint32_t batch_len = offset * 4;
    FORENSIC_LOG("✅ Batch: %u DWORDs (%u bytes, aligné 8-byte)", offset, batch_len);
    FORENSIC_LOG("📊 MODE 3D - Infrastructure identique à C466 sauf bit 0");
    
    /* Préparer relocations */
    struct drm_i915_gem_relocation_entry relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    relocs[0].target_handle = dsh_bo;
    relocs[0].offset = 6 * 4;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    
    relocs[1].target_handle = ssh_bo;
    relocs[1].offset = 9 * 4;
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    
    relocs[2].target_handle = dsh_bo;
    relocs[2].offset = 11 * 4;
    relocs[2].read_domains = I915_GEM_DOMAIN_RENDER;
    
    relocs[3].target_handle = ioh_bo;
    relocs[3].offset = 13 * 4;
    relocs[3].read_domains = I915_GEM_DOMAIN_RENDER;
    
    relocs[4].target_handle = instruction_bo;
    relocs[4].offset = 15 * 4;
    relocs[4].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    
    FORENSIC_LOG("✅ 5 relocations configurées (identiques à C466)");
    
    /* EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ssh_bo;
    exec_objects[1].handle = dsh_bo;
    exec_objects[2].handle = ioh_bo;
    exec_objects[3].handle = instruction_bo;
    exec_objects[4].handle = batch_bo;
    exec_objects[4].relocation_count = 5;
    exec_objects[4].relocs_ptr = (uint64_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (MODE 3D)...");
    
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
    FORENSIC_LOG("🎉 TEST C467 RÉUSSI - GPU fonctionne en MODE 3D!");
    FORENSIC_LOG("📊 CONCLUSION CRITIQUE: PIPELINE_SELECT GPGPU (bit 0=1) est ROOT CAUSE!");
    FORENSIC_LOG("💡 SOLUTION: Rester en mode 3D OU implémenter infrastructure GPGPU complète");
    
    /* Cleanup */
    munmap(batch_map, 4096);
    close(drm_fd);
    
    return 0;
}

// Made with Bob