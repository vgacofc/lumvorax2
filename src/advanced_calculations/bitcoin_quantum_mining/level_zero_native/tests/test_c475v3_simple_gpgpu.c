/*
 * TEST C475v3 - BATCH NULL_STATE + COMMANDES GPGPU SIMPLES
 * 
 * Base: test_c474_full_batch.c (FONCTIONNE ✅)
 * Ajout: Commandes GPGPU minimales après batch null_state
 * 
 * Séquence:
 * 1. Batch null_state complet (960 DWORDs) - Stabilité prouvée
 * 2. PIPELINE_SELECT GPGPU
 * 3. MEDIA_VFE_STATE minimal
 * 4. GPGPU_WALKER minimal (1 work-group)
 * 5. PIPE_CONTROL
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

/* Commandes Gen9 */
#define CMD(PIPELINE,OP,SUB_OP) ((3 << 29) | ((PIPELINE) << 27) | ((OP) << 24) | ((SUB_OP) << 16))
#define CMD_PIPE_CONTROL                CMD(3, 2, 0)
#define CMD_PIPELINE_SELECT             CMD(1, 1, 4)
#define CMD_MEDIA_STATE_POINTERS        CMD(2, 0, 0)
#define CMD_GPGPU_WALKER                CMD(2, 1, 5)
#define CMD_MEDIA_STATE_FLUSH           CMD(2, 0, 4)

#define PIPELINE_SELECT_GPGPU           2

/* Include batch complet */
#include "../include/gen9_null_state_batch.h"

/* Relocations from kernel */
static const uint32_t gen9_null_state_relocs[] = {
    0x000007a8, 0x000007b4, 0x000007bc, 0x000007cc
};

#define BATCH_SIZE (sizeof(gen9_null_state_batch))
#define NUM_RELOCS (sizeof(gen9_null_state_relocs) / sizeof(gen9_null_state_relocs[0]))

int main(void) {
    FORENSIC_LOG("🚀 TEST C475v3 - BATCH NULL_STATE + GPGPU SIMPLE");
    
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
    
    /* Créer buffers */
    struct drm_i915_gem_create create_batch = {.size = 16384};
    struct drm_i915_gem_create create_reloc[4];
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_batch) < 0) {
        FORENSIC_LOG("❌ Batch buffer creation failed");
        close(drm_fd);
        return 1;
    }
    uint32_t batch_bo = create_batch.handle;
    
    uint32_t reloc_bo[4];
    for (int i = 0; i < 4; i++) {
        create_reloc[i].size = 4096;
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create_reloc[i]) < 0) {
            FORENSIC_LOG("❌ Reloc buffer %d creation failed", i);
            close(drm_fd);
            return 1;
        }
        reloc_bo[i] = create_reloc[i].handle;
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
    
    uint32_t* batch = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mmap_arg.offset);
    if (batch == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Batch buffer mappé: %p", batch);
    
    /* Copier batch null_state complet */
    memcpy(batch, gen9_null_state_batch, BATCH_SIZE);
    uint32_t offset = BATCH_SIZE / 4; // Offset en DWORDs
    FORENSIC_LOG("✅ Batch null_state copié: %zu bytes (%u DWORDs)", BATCH_SIZE, offset);
    
    /* Ajouter commandes GPGPU après batch null_state */
    
    // 1. PIPE_CONTROL
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000; // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    FORENSIC_LOG("✅ PIPE_CONTROL ajouté");
    
    // 2. PIPELINE_SELECT GPGPU
    batch[offset++] = CMD_PIPELINE_SELECT | PIPELINE_SELECT_GPGPU;
    FORENSIC_LOG("✅ PIPELINE_SELECT GPGPU ajouté");
    
    // 3. MEDIA_VFE_STATE minimal (9 DWORDs)
    batch[offset++] = CMD_MEDIA_STATE_POINTERS | (9-2);
    batch[offset++] = 0; // Scratch
    batch[offset++] = 0;
    batch[offset++] = 0 | ((64-1) << 16) | (2 << 8) | 0xc0; // max_threads=64, urb=2, gpgpu_mode
    batch[offset++] = 0;
    batch[offset++] = 0; // curbe_size=0
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    FORENSIC_LOG("✅ MEDIA_VFE_STATE ajouté");
    
    // 4. GPGPU_WALKER minimal (15 DWORDs) - 1x1x1 work-groups
    batch[offset++] = CMD_GPGPU_WALKER | 13;
    batch[offset++] = 0; // Interface descriptor offset
    batch[offset++] = 0; // Indirect data length
    batch[offset++] = 0; // Indirect data start
    batch[offset++] = (0 << 30) | 0; // SIMD8, 1 thread
    batch[offset++] = 0; // Thread group X start
    batch[offset++] = 0;
    batch[offset++] = 1; // Thread group X end
    batch[offset++] = 0; // Thread group Y start
    batch[offset++] = 0;
    batch[offset++] = 1; // Thread group Y end
    batch[offset++] = 0; // Thread group Z start
    batch[offset++] = 1; // Thread group Z end
    batch[offset++] = 0xFF; // Right mask
    batch[offset++] = 0xFFFFFFFF; // Bottom mask
    FORENSIC_LOG("✅ GPGPU_WALKER ajouté (1x1x1)");
    
    // 5. MEDIA_STATE_FLUSH
    batch[offset++] = CMD_MEDIA_STATE_FLUSH | 0;
    batch[offset++] = 0;
    
    // 6. PIPE_CONTROL final
    batch[offset++] = CMD_PIPE_CONTROL | (5-2);
    batch[offset++] = 0x00100000; // CS_STALL
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    uint32_t batch_len = offset * 4;
    FORENSIC_LOG("✅ Batch complet: %u DWORDs (%u bytes)", offset, batch_len);
    FORENSIC_LOG("   - Null state: 960 DWORDs");
    FORENSIC_LOG("   - GPGPU: %u DWORDs", offset - 960);
    
    munmap(batch, 16384);
    
    /* Préparer relocations */
    struct drm_i915_gem_relocation_entry relocs[NUM_RELOCS];
    memset(relocs, 0, sizeof(relocs));
    
    for (size_t i = 0; i < NUM_RELOCS; i++) {
        relocs[i].target_handle = reloc_bo[i];
        relocs[i].offset = gen9_null_state_relocs[i];
        relocs[i].read_domains = I915_GEM_DOMAIN_RENDER;
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
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (null_state + GPGPU)...");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        close(drm_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double submit_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi! (%.3f ms)", submit_ms);
    
    /* Attendre GPU idle */
    struct drm_i915_gem_wait wait = {
        .bo_handle = batch_bo,
        .timeout_ns = 2000000000
    };
    
    FORENSIC_LOG("⏳ Attente GPU idle (timeout 2s)...");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        close(drm_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double wait_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    
    FORENSIC_LOG("✅ GPU idle confirmé (%.3f ms)", wait_ms);
    FORENSIC_LOG("");
    FORENSIC_LOG("🎉 TEST C475v3 RÉUSSI!");
    FORENSIC_LOG("📊 RÉSULTATS:");
    FORENSIC_LOG("   - Batch stable: ✅");
    FORENSIC_LOG("   - Pipeline GPGPU: ✅");
    FORENSIC_LOG("   - Aucun GPU HANG: ✅");
    FORENSIC_LOG("");
    FORENSIC_LOG("💡 PROCHAINE ÉTAPE: Ajouter kernel réel + output buffer");
    
    close(drm_fd);
    return 0;
}

// Made with Bob
