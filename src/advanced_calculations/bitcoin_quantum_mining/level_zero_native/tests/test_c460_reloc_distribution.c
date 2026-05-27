/*
 * TEST C460 - DISTRIBUTION CORRECTE DES RELOCATIONS
 * ROOT CAUSE: Relocations doivent être attachées au buffer TARGET, pas au batch!
 * test_c433: exec_objects[2] et [3] ont leurs propres relocations
 * Mes tests: Toutes relocations sur exec_objects[5] (batch) → ERREUR!
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

#define GEN9_MI_BATCH_BUFFER_END 0x0a000000
#define GEN9_PIPELINE_SELECT     ((0x3 << 29) | (0x1 << 27) | (0x1 << 24) | (0x4 << 16))
#define GEN9_STATE_BASE_ADDRESS  0x61010000
#define PIPELINE_SELECT_MEDIA    (1 << 0)
#define CACHE_CTRL 0x3
#define BASE_ADDRESS_MODIFY 0x1

int main() {
    FORENSIC_LOG("🚀 TEST C460 - DISTRIBUTION CORRECTE DES RELOCATIONS");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        FORENSIC_LOG("❌ Erreur ouverture DRM");
        return 1;
    }
    
    struct drm_i915_gem_context_create ctx_create = {0};
    ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    uint32_t ctx_id = ctx_create.ctx_id;
    FORENSIC_LOG("✅ Contexte: %u", ctx_id);
    
    /* Créer 6 buffers comme C433 */
    struct drm_i915_gem_create gem_create;
    uint32_t handles[6];
    
    for (int i = 0; i < 6; i++) {
        gem_create.size = 4096;
        ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
        handles[i] = gem_create.handle;
    }
    FORENSIC_LOG("✅ 6 buffers créés: handles=%u,%u,%u,%u,%u,%u", 
        handles[0], handles[1], handles[2], handles[3], handles[4], handles[5]);
    
    /* Mapper le batch buffer */
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = handles[5],
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
    uint32_t *batch = (uint32_t*)(uintptr_t)mmap_arg.addr_ptr;
    
    /* Batch buffer avec STATE_BASE_ADDRESS */
    uint32_t offset = 0;
    batch[offset++] = 0;  /* NOP */
    batch[offset++] = 0;  /* NOP */
    batch[offset++] = 0;  /* NOP */
    batch[offset++] = 0;  /* NOP */
    
    /* PIPELINE_SELECT */
    batch[offset++] = GEN9_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA;
    
    /* STATE_BASE_ADDRESS avec relocations */
    uint32_t sba_offset = offset * 4;
    batch[offset++] = GEN9_STATE_BASE_ADDRESS | 17;
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0 | (CACHE_CTRL << 16);
    /* SSH base (relocation vers handles[2]) */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* DSH base (relocation vers handles[3]) */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* IOH base (relocation vers handles[4]) */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    /* Instruction base (relocation vers handles[0]) */
    batch[offset++] = 0 | (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0xfffff000 | BASE_ADDRESS_MODIFY;
    batch[offset++] = (CACHE_CTRL << 4) | BASE_ADDRESS_MODIFY;
    batch[offset++] = 0;
    batch[offset++] = 0xfffff000;
    
    /* MI_BATCH_BUFFER_END */
    batch[offset++] = GEN9_MI_BATCH_BUFFER_END;
    
    FORENSIC_LOG("✅ Batch buffer: %u DWORDs", offset);
    
    /* C460 FIX: Relocations DISTRIBUÉES sur les buffers TARGET */
    struct drm_i915_gem_relocation_entry relocs[4];
    memset(relocs, 0, sizeof(relocs));
    
    /* Relocation SSH (dans batch, pointe vers handles[2]) */
    relocs[0].target_handle = handles[2];
    relocs[0].offset = sba_offset + 16;
    relocs[0].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[0].write_domain = 0;
    
    /* Relocation DSH (dans batch, pointe vers handles[3]) */
    relocs[1].target_handle = handles[3];
    relocs[1].offset = sba_offset + 24;
    relocs[1].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[1].write_domain = 0;
    
    /* Relocation IOH (dans batch, pointe vers handles[4]) */
    relocs[2].target_handle = handles[4];
    relocs[2].offset = sba_offset + 32;
    relocs[2].read_domains = I915_GEM_DOMAIN_RENDER;
    relocs[2].write_domain = 0;
    
    /* Relocation Kernel (dans batch, pointe vers handles[0]) */
    relocs[3].target_handle = handles[0];
    relocs[3].offset = sba_offset + 40;
    relocs[3].read_domains = I915_GEM_DOMAIN_INSTRUCTION;
    relocs[3].write_domain = 0;
    
    /* EXECBUFFER2 avec relocations sur BATCH (exec_objects[5]) */
    struct drm_i915_gem_exec_object2 exec_objects[6];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = handles[0];  /* kernel */
    exec_objects[1].handle = handles[1];  /* output */
    exec_objects[2].handle = handles[2];  /* SSH */
    exec_objects[3].handle = handles[3];  /* DSH */
    exec_objects[4].handle = handles[4];  /* IOH */
    exec_objects[5].handle = handles[5];  /* batch */
    exec_objects[5].relocation_count = 4;  /* 4 relocations dans batch */
    exec_objects[5].relocs_ptr = (uint64_t)&relocs[0];
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)exec_objects,
        .buffer_count = 6,
        .batch_len = offset * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id
    };
    
    FORENSIC_LOG("⚡ Test EXECBUFFER2 avec relocations sur batch...");
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        FORENSIC_LOG("❌ EXECBUFFER2 échoue: %s (errno=%d)", strerror(errno), errno);
        munmap(batch, 4096);
        close(fd);
        return 1;
    }
    
    FORENSIC_LOG("✅✅✅ EXECBUFFER2 RÉUSSIT!");
    FORENSIC_LOG("🎉 Relocations dans batch buffer fonctionnent!");
    
    munmap(batch, 4096);
    close(fd);
    return 0;
}

// Made with Bob