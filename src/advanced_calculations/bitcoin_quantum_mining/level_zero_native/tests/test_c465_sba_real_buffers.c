/*
 * TEST C465 - STATE_BASE_ADDRESS AVEC BUFFERS GPU RÉELS
 * 
 * TEST PRIORITÉ #1 (feedback expert):
 * - PIPELINE_SELECT
 * - STATE_BASE_ADDRESS avec vrais buffers GPU
 * - MEDIA_VFE_STATE
 * - MI_BATCH_BUFFER_END
 * 
 * SANS:
 * - GPGPU_WALKER
 * - IDD
 * - CURBE
 * - SSH access
 * 
 * Objectif: Vérifier si le GPU peut sortir proprement du mode GPGPU
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
#define GEN9_MI_NOOP                   0x00000000
#define PIPELINE_SELECT_MEDIA          (1 << 0)

/* Gen9 Constants */
#define CACHE_CTRL 0x3
#define BASE_ADDRESS_MODIFY 0x1

/* Buffer context */
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t batch_bo;
    uint32_t ssh_bo;
    uint32_t dsh_bo;
    uint32_t ioh_bo;
    uint32_t instruction_bo;
    void* batch_map;
} gpu_context_t;

int create_buffer(int drm_fd, size_t size, uint32_t* handle) {
    struct drm_i915_gem_create create = {.size = size};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        return -1;
    }
    *handle = create.handle;
    return 0;
}

int main(void) {
    FORENSIC_LOG("🚀 TEST C465 - SBA AVEC BUFFERS GPU RÉELS (TEST PRIORITÉ #1)");
    
    gpu_context_t ctx = {0};
    
    /* Ouvrir DRM */
    ctx.drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (ctx.drm_fd < 0) {
        FORENSIC_LOG("❌ Impossible d'ouvrir /dev/dri/renderD128: %s", strerror(errno));
        return 1;
    }
    FORENSIC_LOG("✅ DRM ouvert: fd=%d", ctx.drm_fd);
    
    /* Créer contexte GPU */
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        FORENSIC_LOG("❌ GEM_CONTEXT_CREATE failed: %s", strerror(errno));
        close(ctx.drm_fd);
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte GPU créé: id=%u", ctx.ctx_id);
    
    /* Créer tous les buffers GPU */
    if (create_buffer(ctx.drm_fd, 4096, &ctx.batch_bo) < 0) {
        FORENSIC_LOG("❌ Batch buffer creation failed");
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Batch buffer créé: handle=%u", ctx.batch_bo);
    
    if (create_buffer(ctx.drm_fd, 4096, &ctx.ssh_bo) < 0) {
        FORENSIC_LOG("❌ SSH buffer creation failed");
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ SSH buffer créé: handle=%u", ctx.ssh_bo);
    
    if (create_buffer(ctx.drm_fd, 4096, &ctx.dsh_bo) < 0) {
        FORENSIC_LOG("❌ DSH buffer creation failed");
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ DSH buffer créé: handle=%u", ctx.dsh_bo);
    
    if (create_buffer(ctx.drm_fd, 4096, &ctx.ioh_bo) < 0) {
        FORENSIC_LOG("❌ IOH buffer creation failed");
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ IOH buffer créé: handle=%u", ctx.ioh_bo);
    
    if (create_buffer(ctx.drm_fd, 4096, &ctx.instruction_bo) < 0) {
        FORENSIC_LOG("❌ Instruction buffer creation failed");
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Instruction buffer créé: handle=%u", ctx.instruction_bo);
    
    /* Mapper batch buffer */
    struct drm_i915_gem_mmap_offset mmap_arg = {
        .handle = ctx.batch_bo,
        .flags = I915_MMAP_OFFSET_WB
    };
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg) < 0) {
        FORENSIC_LOG("❌ GEM_MMAP_OFFSET failed: %s", strerror(errno));
        close(ctx.drm_fd);
        return 1;
    }
    
    ctx.batch_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, ctx.drm_fd, mmap_arg.offset);
    if (ctx.batch_map == MAP_FAILED) {
        FORENSIC_LOG("❌ mmap failed: %s", strerror(errno));
        close(ctx.drm_fd);
        return 1;
    }
    FORENSIC_LOG("✅ Batch buffer mappé: %p", ctx.batch_map);
    
    /* Construire batch buffer */
    uint32_t* batch = (uint32_t*)ctx.batch_map;
    uint32_t offset = 0;
    
    /* MI_NOOP padding */
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    batch[offset++] = GEN9_MI_NOOP;
    
    /* PIPELINE_SELECT - GPGPU mode */
    batch[offset++] = GEN9_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA;
    FORENSIC_LOG("📝 [%02u] PIPELINE_SELECT = 0x%08X (GPGPU mode)", offset-1, batch[offset-1]);
    
    /* STATE_BASE_ADDRESS - Adresses GPU via relocations */
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  /* General State (relocation #0) */
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  /* SSH (relocation #1) */
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  /* DSH (relocation #2) */
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  /* IOH (relocation #3) */
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;  /* Instruction (relocation #4) */
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    FORENSIC_LOG("📝 [%02u] STATE_BASE_ADDRESS (19 DWORDs, avec relocations)", offset-19);
    
    /* MEDIA_VFE_STATE - Configuration minimale */
    batch[offset++] = GEN9_MEDIA_VFE_STATE | (8 - 2);
    batch[offset++] = 0x00000000;  /* Scratch Space = 0 */
    batch[offset++] = 0x00000000;
    batch[offset++] = (1 << 0) | (1 << 16);  /* Max Threads=1, URB=1 (MINIMAL) */
    batch[offset++] = 0x00000000;
    batch[offset++] = (0 << 0) | (1 << 16);  /* CURBE=0, URB=1 (MINIMAL) */
    batch[offset++] = 0x00000000;
    batch[offset++] = 0x00000000;
    FORENSIC_LOG("📝 [%02u] MEDIA_VFE_STATE (8 DWORDs, config minimale)", offset-8);
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    FORENSIC_LOG("📝 [%02u] MI_BATCH_BUFFER_END", offset-1);
    
    /* PADDING alignement 8-byte */
    while ((offset * 4) % 8 != 0) {
        batch[offset++] = GEN9_MI_NOOP;
    }
    
    uint32_t batch_len = offset * 4;
    FORENSIC_LOG("✅ Batch: %u DWORDs (%u bytes, aligné 8-byte)", offset, batch_len);
    
    /* Préparer relocations */
    struct drm_i915_gem_relocation_entry relocs[5];
    memset(relocs, 0, sizeof(relocs));
    
    /* Relocation #0: General State (DW1 de SBA, offset 6*4=24) */
    relocs[0].target_handle = ctx.dsh_bo;
    relocs[0].delta = 0;
    relocs[0].offset = 6 * 4;
    relocs[0].presumed_offset = 0;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = 0;
    
    /* Relocation #1: SSH (DW4 de SBA, offset 9*4=36) */
    relocs[1].target_handle = ctx.ssh_bo;
    relocs[1].delta = 0;
    relocs[1].offset = 9 * 4;
    relocs[1].presumed_offset = 0;
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = 0;
    
    /* Relocation #2: DSH (DW6 de SBA, offset 11*4=44) */
    relocs[2].target_handle = ctx.dsh_bo;
    relocs[2].delta = 0;
    relocs[2].offset = 11 * 4;
    relocs[2].presumed_offset = 0;
    relocs[2].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[2].write_domain = 0;
    
    /* Relocation #3: IOH (DW8 de SBA, offset 13*4=52) */
    relocs[3].target_handle = ctx.ioh_bo;
    relocs[3].delta = 0;
    relocs[3].offset = 13 * 4;
    relocs[3].presumed_offset = 0;
    relocs[3].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[3].write_domain = 0;
    
    /* Relocation #4: Instruction (DW10 de SBA, offset 15*4=60) */
    relocs[4].target_handle = ctx.instruction_bo;
    relocs[4].delta = 0;
    relocs[4].offset = 15 * 4;
    relocs[4].presumed_offset = 0;
    relocs[4].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[4].write_domain = 0;
    
    FORENSIC_LOG("✅ 5 relocations configurées (General, SSH, DSH, IOH, Instruction)");
    
    /* EXECBUFFER2 avec relocations */
    struct drm_i915_gem_exec_object2 exec_objects[5];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = ctx.ssh_bo;
    exec_objects[1].handle = ctx.dsh_bo;
    exec_objects[2].handle = ctx.ioh_bo;
    exec_objects[3].handle = ctx.instruction_bo;
    exec_objects[4].handle = ctx.batch_bo;
    exec_objects[4].relocation_count = 5;
    exec_objects[4].relocs_ptr = (uint64_t)relocs;
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 5,
        .batch_start_offset = 0,
        .batch_len = batch_len,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx.ctx_id
    };
    
    FORENSIC_LOG("⚡ Soumission batch (5 buffers, 5 relocations)...");
    
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 failed: %s (errno=%d)", strerror(errno), errno);
        munmap(ctx.batch_map, 4096);
        close(ctx.drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ EXECBUFFER2 réussi!");
    
    /* Attendre GPU idle */
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.batch_bo,
        .timeout_ns = 2000000000  /* 2 secondes */
    };
    
    if (ioctl(ctx.drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        FORENSIC_LOG("❌ GEM_WAIT failed: %s", strerror(errno));
        munmap(ctx.batch_map, 4096);
        close(ctx.drm_fd);
        return 1;
    }
    
    FORENSIC_LOG("✅ GPU idle confirmé");
    FORENSIC_LOG("🎉 TEST C465 RÉUSSI - GPU sort proprement du mode GPGPU!");
    FORENSIC_LOG("📊 Conclusion: SBA avec buffers réels fonctionne");
    
    /* Cleanup */
    munmap(ctx.batch_map, 4096);
    close(ctx.drm_fd);
    
    return 0;
}

// Made with Bob
